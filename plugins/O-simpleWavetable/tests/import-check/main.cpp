/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-simpleWavetable import-check - Stage 2.4 gates (RESEARCH 7.1-7.5 +
    PLAN Task 19).

    Every gate renders through the real processor (OSIW_TEST_HOOKS) or runs
    the real importer, and has a negative control or a liveness check. Every
    measured value is printed. No wall-clock verdicts (sleeps only pace the
    worker threads; no assertion depends on timing).

      G-XF-EXACT-*   QUAL-03 exactness tier (D-G): y == (1-w)A + wB over the
                     5 ms raised-cosine fade, max |y - ideal| <= 1e-4, for: bank param,
                     import publish empty->X / X->Y / X->empty, bandlimit,
                     interp, level change (forceLevel), and the FOLD case
                     (2nd trigger 1 ms into the fade). Neg: xfadeLenOverride
                     0 gives >= 0.1.
      G-XF-RATIO-*   QUAL-03 ratio tier (RESEARCH 3.4): Sine->Saw(0) <->
                     Formant(0) at A1/A2 x 8 switch phases, import swap,
                     Imported -> empty, import during 16 held notes, fold.
                     Excess ratio <= 1.5, liveness |A-B| >= 0.25, neg >= 4.
      G-ALLOC-24     DSP-06 / PERF-01: 0 audio-thread allocations over >= 200
                     bank switches while a 2nd thread runs 50 imports and
                     restore-publishes. Liveness malloc(64) counts 1.
      G-REAP-*       REG-01 + D-C: quiescent, held, D-C graveyard negative
                     control (>= 1 without the amendment, 0 with it), +2 rule
                     and no quiescent fire in flight (midBlockCallback),
                     concurrent soak (held <= 3, deref 0, live = 5 + 1).
      G-FUNC03-*     frame counts 215 / 256 / 1 / 2, tooShort (bank
                     untouched), unreadable, tooLarge.
      G-COMPAT03     WAV / AIFF / FLAC x 44.1 / 48 / 96 kHz: identical int16
                     PCM and bit-identical banks; 24-bit, float + NaN,
                     stereo mean, 6-channel mean of 6.
      G-FUNC04-*     import -> save -> fresh restore: memcmp of all 11
                     levels, filename, numFrames, verbatim data string. Neg:
                     one int16 changed fails. pcm16gz bit-identical. Absent
                     child -> empty Imported, exact silence.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "BuiltInBanks.h"
#include "PluginProcessor.h"
#include "WavetableBank.h"
#include "WavetableImporter.h"
#include "WtVoice.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if JUCE_MAC
 #include <execinfo.h>
 #include <pthread.h>

extern "C"
{
    typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                    uintptr_t result, uint32_t numHotFramesToSkip);
    extern malloc_logger_t* malloc_logger;
}
#endif

namespace
{
    using Proc = OSimpleWavetableAudioProcessor;
    namespace ids = OSimpleWavetable::ParamIDs;
    namespace imp = WavetableImporter;

    constexpr double kFs = 48000.0;
    constexpr double kTwoPi = 6.28318530717958647692;
    constexpr int kXf = 240;                       // round(0.005 * 48000)

    // Built-in bank indices (parameter-spec order).
    constexpr int kSineSaw = 0, kFormant = 3, kDrive = 4, kImported = 5;

    int gFailures = 0;
    long long gNonFinite = 0;
    long long gChannelMismatch = 0;
    long long gSamplesChecked = 0;
    bool gAllocMode = false;
    juce::File gTemp;

    void report (const std::string& gate, bool ok, const std::string& detail)
    {
        std::cout << (ok ? "PASS " : "FAIL ") << gate << (ok ? " " : ": ") << detail << "\n";
        std::cout.flush();
        if (! ok)
            ++gFailures;
    }

    void info (const std::string& line)
    {
        std::cout << "  " << line << "\n";
        std::cout.flush();
    }

    std::string fmt (double v, int prec = 3)
    {
        std::ostringstream s;
        s << std::fixed << std::setprecision (prec) << v;
        return s.str();
    }

    std::string sci (double v)
    {
        std::ostringstream s;
        s << std::scientific << std::setprecision (3) << v;
        return s.str();
    }

    std::string str (const juce::String& s) { return s.toStdString(); }

    int secs (double t) { return (int) std::lround (t * kFs); }

    //==========================================================================
    // Alloc gate (O-Bells pattern; volatile flag + counter, audio-thread scoped).
   #if JUCE_MAC
    volatile bool allocArmed = false;
    volatile int  allocCount = 0;
    pthread_t audioThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), audioThread))   // MALLOC_LOG_TYPE_ALLOCATE
        {
            allocArmed = false;
            allocCount = allocCount + 1;
            void* frames[32];
            backtrace_symbols_fd (frames, backtrace (frames, 32), 2);   // does not malloc
        }
    }
   #endif

    void armAlloc (bool on)
    {
       #if JUCE_MAC
        allocArmed = on;
       #else
        juce::ignoreUnused (on);
       #endif
    }

    //==========================================================================
    struct Ev
    {
        int pos;
        juce::MidiMessage msg;
    };

    Ev evOn (int pos, int note, int vel = 127)
    {
        return { pos, juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, vel)) };
    }

    using Params = std::vector<std::pair<const char*, float>>;

    void setParam (Proc& p, const char* id, float realValue)
    {
        if (auto* rp = p.getAPVTS().getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
    }

    float rawOf (Proc& p, const char* id)
    {
        if (auto* v = p.getAPVTS().getRawParameterValue (id))
            return v->load();
        return std::nanf ("");
    }

    // Every parameter explicit: static position, no modulation, instant amp
    // attack, sustain 1, Full bit depth, 0 dB out.
    Params base (int bank, float pos)
    {
        return { { ids::bank, (float) bank }, { ids::position, pos }, { ids::interp, 1.0f },
                 { ids::bandlimit, 1.0f }, { ids::bitDepth, 0.0f },
                 { ids::lfoRate, 0.5f }, { ids::lfoSync, 0.0f }, { ids::lfoDiv, 2.0f },
                 { ids::lfoShape, 0.0f }, { ids::lfoDepth, 0.0f },
                 { ids::menvAttack, 0.5f }, { ids::menvDecay, 1.0f }, { ids::menvSustain, 0.0f },
                 { ids::menvRelease, 0.5f }, { ids::envAmount, 0.0f },
                 { ids::ampAttack, 0.001f }, { ids::ampDecay, 0.001f }, { ids::ampSustain, 1.0f },
                 { ids::ampRelease, 0.05f }, { ids::voiceMode, 0.0f }, { ids::outputLevel, 0.0f } };
    }

    //==========================================================================
    // Processor rig. Params set BEFORE prepare (smoothers seeded). Events at
    // absolute sample positions. hook(rig, blockStart) runs before each
    // processBlock (trigger injection between blocks).
    struct Rig
    {
        std::unique_ptr<Proc> proc;
        int block;
        int cursor = 0;
        int blocksRun = 0;
        juce::AudioBuffer<float> buf;
        juce::MidiBuffer midi;

        Rig (int prepBlock, const Params& params) : proc (std::make_unique<Proc>()), block (prepBlock), buf (2, prepBlock)
        {
            for (const auto& pr : params)
                setParam (*proc, pr.first, pr.second);
            proc->setPlayConfigDetails (0, 2, kFs, prepBlock);
            proc->prepareToPlay (kFs, prepBlock);
            midi.ensureSize (16384);
        }

        void set (const char* id, float v) { setParam (*proc, id, v); }

        std::vector<float> run (int n, const std::vector<Ev>& evs = {},
                                const std::function<void (Rig&, int)>& hook = nullptr)
        {
            std::vector<float> out;
            out.reserve ((size_t) juce::jmax (0, n));

            for (int done = 0; done < n;)
            {
                const int len = juce::jmin (block, n - done);
                const int t0 = cursor + done;
                if (hook)
                    hook (*this, t0);

                buf.setSize (2, len, false, false, true);
                for (int ch = 0; ch < 2; ++ch)
                    juce::FloatVectorOperations::fill (buf.getWritePointer (ch), 1.0f, len);   // processBlock must clear

                midi.clear();
                for (const auto& e : evs)
                    if (e.pos >= t0 && e.pos < t0 + len)
                        midi.addEvent (e.msg, e.pos - t0);

                armAlloc (gAllocMode && blocksRun > 0);   // warm-up block unarmed
                proc->processBlock (buf, midi);
                armAlloc (false);
                ++blocksRun;

                const float* l = buf.getReadPointer (0);
                const float* r = buf.getReadPointer (1);
                for (int i = 0; i < len; ++i)
                {
                    if (! std::isfinite (l[i]) || ! std::isfinite (r[i]))
                        ++gNonFinite;
                    if (std::memcmp (&l[i], &r[i], sizeof (float)) != 0)
                        ++gChannelMismatch;
                    out.push_back (l[i]);
                }
                gSamplesChecked += len;
                done += len;
            }

            cursor += n;
            return out;
        }
    };

    //==========================================================================
    // Analysis (RESEARCH 3.4 click detector).
    double maxAbsD2 (const std::vector<float>& y, int a, int b)
    {
        double m = 0.0;
        for (int n = juce::jmax (2, a); n < juce::jmin ((int) y.size(), b); ++n)
            m = std::max (m, std::abs ((double) y[(size_t) n] - 2.0 * (double) y[(size_t) (n - 1)] + (double) y[(size_t) (n - 2)]));
        return m;
    }

    struct ClickVerdict
    {
        double ratio = 0.0, event = 0.0, plateau = 0.0;
    };

    ClickVerdict clickRatio (const std::vector<float>& y, int t, int xf)
    {
        const int win = xf + secs (0.002), plat = secs (0.020), gap = secs (0.005);
        const double ev   = maxAbsD2 (y, t - 2, t + win);
        const double pre  = maxAbsD2 (y, t - gap - plat, t - gap);
        const double post = maxAbsD2 (y, t + win + gap, t + win + gap + plat);
        const double ref  = std::max ({ pre, post, 1.0e-9 });
        return { ev / ref, ev, ref };
    }

    bool allExactZero (const std::vector<float>& y, int from = 0)
    {
        for (size_t i = (size_t) juce::jmax (0, from); i < y.size(); ++i)
            if (! juce::exactlyEqual (y[i], 0.0f))
                return false;
        return true;
    }

    double peakAbs (const std::vector<float>& y, int from, int to)
    {
        double m = 0.0;
        for (int i = juce::jmax (0, from); i < juce::jmin ((int) y.size(), to); ++i)
            m = std::max (m, (double) std::abs (y[(size_t) i]));
        return m;
    }

    //==========================================================================
    // Synthetic imported banks (harness-built, through the ONE builder).
    // kind 0 = sine-like (h1 + 0.2 h2), 1 = formant-like (vowel-ish h3..h9
    // cluster), 2 = bright (h1..h40 1/k). Frames differ slightly per index.
    std::vector<std::int16_t> synthPcm (int kind, int frames)
    {
        std::vector<std::int16_t> v ((size_t) frames * 2048);
        for (int f = 0; f < frames; ++f)
        {
            std::vector<double> x (2048, 0.0);
            double peak = 0.0;
            for (int i = 0; i < 2048; ++i)
            {
                const double p = kTwoPi * (double) i / 2048.0;
                double s = 0.0;
                if (kind == 0)
                    s = std::sin (p) + 0.2 * std::sin (2.0 * p + 0.1 * f);
                else if (kind == 1)
                    for (int k = 3; k <= 9; ++k)
                        s += std::exp (-0.5 * std::pow ((k - 6.0 - 0.1 * f) / 1.5, 2.0)) * std::sin ((double) k * p + 0.3 * k);
                else
                    for (int k = 1; k <= 40; ++k)
                        s += std::sin ((double) k * p) / (double) k;
                x[(size_t) i] = s;
                peak = std::max (peak, std::abs (s));
            }
            for (int i = 0; i < 2048; ++i)
                v[(size_t) (f * 2048 + i)] = (std::int16_t) std::lround (x[(size_t) i] / peak * 32767.0);
        }
        return v;
    }

    std::shared_ptr<const WavetableBank> synthBank (int kind, int frames, const char* name)
    {
        ImportedPcm p;
        p.filename = name;
        p.numFrames = frames;
        p.pcm = synthPcm (kind, frames);
        return imp::buildImportedBank (p);
    }

    //==========================================================================
    // Fixture writers (JUCE writers, int API left-justified for 16/24-bit).
    enum class Fmt { wav, aiff, flac };

    std::unique_ptr<juce::AudioFormat> makeFormat (Fmt f)
    {
        if (f == Fmt::aiff) return std::make_unique<juce::AiffAudioFormat>();
        if (f == Fmt::flac) return std::make_unique<juce::FlacAudioFormat>();
        return std::make_unique<juce::WavAudioFormat>();
    }

    // channels[c][i] as int16 values; bits 16 or 24 (24-bit = value << 8).
    bool writeInt (const juce::File& file, Fmt f, double sr, int bits,
                   const std::vector<std::vector<std::int16_t>>& channels)
    {
        file.deleteFile();
        auto fos = std::make_unique<juce::FileOutputStream> (file);
        if (! fos->openedOk())
            return false;
        std::unique_ptr<juce::OutputStream> os = std::move (fos);
        auto fmtObj = makeFormat (f);
        const int nc = (int) channels.size();
        auto w = fmtObj->createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (sr)
                                                 .withNumChannels (nc).withBitsPerSample (bits));
        if (w == nullptr)
            return false;
        const size_t n = channels[0].size();
        std::vector<std::vector<int>> wide ((size_t) nc, std::vector<int> (n));
        std::vector<const int*> ptrs ((size_t) nc + 1, nullptr);
        for (int c = 0; c < nc; ++c)
        {
            for (size_t i = 0; i < n; ++i)
                wide[(size_t) c][i] = (int) channels[(size_t) c][i] * 65536;
            ptrs[(size_t) c] = wide[(size_t) c].data();
        }
        return w->write (ptrs.data(), (int) n);
    }

    bool writeMono16 (const juce::File& file, const std::vector<std::int16_t>& pcm, Fmt f = Fmt::wav, double sr = 48000.0)
    {
        return writeInt (file, f, sr, 16, { pcm });
    }

    // 32-bit IEEE float WAV.
    bool writeFloatWav (const juce::File& file, const std::vector<std::vector<float>>& channels)
    {
        file.deleteFile();
        auto fos = std::make_unique<juce::FileOutputStream> (file);
        if (! fos->openedOk())
            return false;
        std::unique_ptr<juce::OutputStream> os = std::move (fos);
        juce::WavAudioFormat wav;
        const int nc = (int) channels.size();
        auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (48000.0)
                                              .withNumChannels (nc).withBitsPerSample (32)
                                              .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
        if (w == nullptr)
            return false;
        std::vector<const float*> ptrs ((size_t) nc, nullptr);
        for (int c = 0; c < nc; ++c)
            ptrs[(size_t) c] = channels[(size_t) c].data();
        return w->writeFromFloatArrays (ptrs.data(), nc, (int) channels[0].size());
    }

    // Deterministic varied content (harmonic mixture drifting per frame).
    std::vector<std::int16_t> contentPcm (int numSamples, int seed)
    {
        std::vector<std::int16_t> v ((size_t) numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            const double p = kTwoPi * (double) i / 2048.0;
            const double x = 0.5 * std::sin (p * (double) (1 + seed)) + 0.3 * std::sin (3.0 * p + (double) seed)
                           + 0.15 * std::cos (7.0 * p * (double) (1 + (i / 2048) % 5));
            v[(size_t) i] = (std::int16_t) std::lround (juce::jlimit (-1.0, 1.0, x) * 30000.0);
        }
        return v;
    }

    // Direct decode through the real importer (no processor).
    ImportResult decodeFile (const juce::File& f)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr)
        {
            ImportResult res;
            res.error = ImportError::unreadable;
            return res;
        }
        return imp::decode (*r, f.getFileName(), [] { return false; });
    }

    // importFromFile, poll the status version until it settles (no message
    // loop in the console harness), then apply the pending auto-select on
    // this (message) thread. Sleeps pace the poll only.
    bool importAndWait (Proc& p, const juce::File& f, Proc::ImportStatus& st)
    {
        if (! p.importFromFile (f))
        {
            st = p.getImportStatus();
            return false;
        }
        for (int spins = 0; spins < 30000; ++spins)
        {
            st = p.getImportStatus();
            if (st.state == Proc::ImportStatus::State::done || st.state == Proc::ImportStatus::State::error)
                break;
            juce::Thread::sleep (2);
        }
        p.handleUpdateNowIfNeeded();
        return st.state == Proc::ImportStatus::State::done;
    }

    bool sameBank (const WavetableBank* a, const WavetableBank* b)
    {
        return a != nullptr && b != nullptr && a->numFrames == b->numFrames
            && a->data.size() == b->data.size()
            && std::memcmp (a->data.data(), b->data.data(), a->data.size() * sizeof (float)) == 0;
    }

    juce::MemoryBlock saveState (Proc& p)
    {
        juce::MemoryBlock mb;
        p.getStateInformation (mb);
        return mb;
    }

    void loadState (Proc& p, const juce::MemoryBlock& mb)
    {
        p.setStateInformation (mb.getData(), (int) mb.getSize());
    }

    std::unique_ptr<juce::XmlElement> blobToXml (const juce::MemoryBlock& mb)
    {
        return juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
    }

    juce::MemoryBlock xmlToBlob (const juce::XmlElement& x)
    {
        juce::MemoryBlock mb;
        juce::AudioProcessor::copyXmlToBinary (x, mb);
        return mb;
    }

    std::unique_ptr<Proc> makePrepared()
    {
        auto p = std::make_unique<Proc>();
        p->setPlayConfigDetails (0, 2, kFs, 256);
        p->prepareToPlay (kFs, 256);
        return p;
    }

    //==========================================================================
    // QUAL-03 (D-G). A switch scenario = K+1 cumulative configurations
    // (stage 0 = before any trigger) applied to the rig, and K trigger
    // instants (block boundaries). R_k = the steady render of stage k from
    // the start (same note, same onset, so the same phase). The ideal output
    // follows the frozen-cycle model exactly: before the first trigger R_0;
    // at each trigger the heard mix (1-w) F + w R_prev is frozen into F and
    // the fade restarts towards R_k (fold rule); w = k / 240.
    struct Scenario
    {
        std::string name;
        Params params;
        std::vector<int> notes;
        int block = 256;
        std::function<void (Rig&, int stage)> apply;   // sets the FULL config of a stage
        std::vector<int> triggerOffsets;               // relative to the first trigger (first = 0)
    };

    constexpr int kTrig = 768 * 14;                    // first trigger (0.224 s; a multiple of the 256 AND 48 blocks)
    constexpr int kRenderLen = kTrig + 4800;           // + 100 ms (fade + click plateaus)

    std::vector<Ev> chordAt (const std::vector<int>& notes, int onset)
    {
        std::vector<Ev> e;
        for (int n : notes)
            e.push_back (evOn (onset, n));
        return e;
    }

    std::vector<float> renderStage (const Scenario& sc, int stage, int onset)
    {
        Rig r (sc.block, sc.params);
        sc.apply (r, stage);
        return r.run (kRenderLen, chordAt (sc.notes, onset));
    }

    std::vector<float> renderSwitch (const Scenario& sc, int onset, int xfOverride)
    {
        Rig r (sc.block, sc.params);
        sc.apply (r, 0);
        if (xfOverride >= 0)
            r.proc->setXfadeLenOverrideForTesting (xfOverride);
        const int K = (int) sc.triggerOffsets.size();
        return r.run (kRenderLen, chordAt (sc.notes, onset), [&sc, K] (Rig& rig, int t0)
        {
            for (int k = 0; k < K; ++k)
                if (t0 == kTrig + sc.triggerOffsets[(size_t) k])
                    sc.apply (rig, k + 1);
        });
    }

    // The crossfade law under test: raised cosine over kXf samples
    // (independent transcription, not the voice's code).
    double xfW (int pos) { return 0.5 - 0.5 * std::cos (kTwoPi * 0.5 * (double) pos / (double) kXf); }

    std::vector<float> idealOutput (const Scenario& sc, const std::vector<std::vector<float>>& R)
    {
        const int K = (int) sc.triggerOffsets.size();
        const int S = K + 1;
        std::vector<double> F ((size_t) S, 0.0);       // frozen mix as coefficients over R_0..R_K
        int cur = 0, pos = 0;
        bool fading = false;
        int nextK = 0;
        std::vector<float> y ((size_t) kRenderLen, 0.0f);

        for (int t = 0; t < kRenderLen; ++t)
        {
            if (nextK < K && t == kTrig + sc.triggerOffsets[(size_t) nextK])
            {
                const double w = fading ? xfW (pos) : 1.0;
                std::vector<double> nf ((size_t) S, 0.0);
                for (int s = 0; s < S; ++s)
                    nf[(size_t) s] = fading ? (1.0 - w) * F[(size_t) s] : 0.0;
                nf[(size_t) cur] += w;
                F = nf;
                cur = nextK + 1;
                ++nextK;
                fading = true;
                pos = 0;
            }

            double v;
            if (fading)
            {
                const double w = xfW (pos);
                double fz = 0.0;
                for (int s = 0; s < S; ++s)
                    fz += F[(size_t) s] * (double) R[(size_t) s][(size_t) t];
                v = (1.0 - w) * fz + w * (double) R[(size_t) cur][(size_t) t];
                if (++pos >= kXf)
                    fading = false;
            }
            else
            {
                v = (double) R[(size_t) cur][(size_t) t];
            }
            y[(size_t) t] = (float) v;
        }
        return y;
    }

    double maxErr (const std::vector<float>& a, const std::vector<float>& b)
    {
        double m = 0.0;
        for (size_t i = 0; i < std::min (a.size(), b.size()); ++i)
            m = std::max (m, (double) std::abs (a[i] - b[i]));
        return m;
    }

    int periodOf (const Scenario& sc)
    {
        if (sc.notes.size() != 1)
            return secs (0.050);
        const double hz = juce::MidiMessage::getMidiNoteInHertz (sc.notes[0]);
        return (int) std::ceil (kFs / hz);
    }

    // Onsets that put the first trigger at a chosen point of the waveform:
    // shifting the onset by d moves the onset-0 waveform at t* = kTrig - d
    // under the trigger. Candidates = the last period before kTrig.
    std::vector<int> pickOnsets (const Scenario& sc, int count, double minLive)
    {
        const auto A = renderStage (sc, 0, 0);
        const auto B = renderStage (sc, 1, 0);
        const int per = periodOf (sc);
        std::vector<std::pair<int, double>> cand;
        for (int t = kTrig - per; t < kTrig; ++t)
            cand.emplace_back (t, std::abs ((double) A[(size_t) t] - (double) B[(size_t) t]));

        std::vector<int> onsets;
        if (count == 1)
        {
            auto best = std::max_element (cand.begin(), cand.end(), [] (auto& x, auto& y) { return x.second < y.second; });
            onsets.push_back (kTrig - best->first);
            return onsets;
        }
        std::vector<int> live;
        for (const auto& c : cand)
            if (c.second >= minLive)
                live.push_back (c.first);
        for (int i = 0; i < count && ! live.empty(); ++i)
            onsets.push_back (kTrig - live[(size_t) ((size_t) i * live.size() / (size_t) count)]);
        return onsets;
    }

    // Exactness tier: one onset (the largest |A - B| under the trigger, so
    // the negative control is as discriminating as the stimulus allows).
    void exactness (const Scenario& sc)
    {
        const int onset = pickOnsets (sc, 1, 0.0)[0];
        std::vector<std::vector<float>> R;
        for (int s = 0; s <= (int) sc.triggerOffsets.size(); ++s)
            R.push_back (renderStage (sc, s, onset));
        const auto ideal = idealOutput (sc, R);
        const auto Y = renderSwitch (sc, onset, -1);
        const auto N = renderSwitch (sc, onset, 0);
        const double e = maxErr (Y, ideal), en = maxErr (N, ideal);
        const double live = std::abs ((double) R[0][(size_t) kTrig] - (double) R[1][(size_t) kTrig]);
        report ("G-XF-EXACT-" + sc.name, e <= 1.0e-4 && en >= 0.1,
                "onset " + std::to_string (onset) + ", trigger(s) at " + std::to_string (kTrig) + " (+"
                + std::to_string (sc.triggerOffsets.back()) + "): max |y - ideal| " + sci (e) + " (<= 1e-4); |A-B| at trigger "
                + fmt (live, 4) + "; neg xfadeLenOverride 0: " + fmt (en, 4) + " (>= 0.1)");
    }

    // Ratio tier: count onsets with liveness >= 0.25 at the event.
    void ratioTier (const Scenario& sc, int count)
    {
        const auto onsets = pickOnsets (sc, count, 0.25);
        const int xfSpan = sc.triggerOffsets.back() + kXf;
        double worst = 0.0, minNeg = 1.0e9, minLive = 1.0e9;
        std::string per;
        for (int onset : onsets)
        {
            const auto A = renderStage (sc, 0, onset);
            const auto B = renderStage (sc, 1, onset);
            const auto Y = renderSwitch (sc, onset, -1);
            const auto N = renderSwitch (sc, onset, 0);
            const double live = std::abs ((double) A[(size_t) kTrig] - (double) B[(size_t) kTrig]);
            const auto v = clickRatio (Y, kTrig, xfSpan);
            const auto vn = clickRatio (N, kTrig, xfSpan);
            worst = std::max (worst, v.ratio);
            minNeg = std::min (minNeg, vn.ratio);
            minLive = std::min (minLive, live);
            per += fmt (v.ratio, 3) + "/" + fmt (vn.ratio, 1) + " ";
        }
        const bool ok = (int) onsets.size() == count && worst <= 1.5 && minLive >= 0.25 && minNeg >= 4.0;
        report ("G-XF-RATIO-" + sc.name, ok,
                std::to_string (onsets.size()) + " switch phases (want " + std::to_string (count) + "): max excess ratio "
                + fmt (worst, 3) + " (<= 1.5); min liveness |A-B| " + fmt (minLive, 4) + " (>= 0.25); min neg ratio "
                + fmt (minNeg, 2) + " (>= 4)");
        info ("ratio/neg per phase: " + per);
    }

    std::shared_ptr<const WavetableBank> gSineLike, gFormantLike, gMixed2;

    // Frame 0 sine-like, frame 1 formant-like: the Interp toggle at
    // Position 0.5 swaps a 50/50 blend for a whole frame.
    std::shared_ptr<const WavetableBank> mixedBank()
    {
        ImportedPcm p;
        p.filename = "mixed2";
        p.numFrames = 2;
        auto a = synthPcm (0, 1), b = synthPcm (1, 1);
        p.pcm = a;
        p.pcm.insert (p.pcm.end(), b.begin(), b.end());
        return imp::buildImportedBank (p);
    }

    void gateQual03()
    {
        gSineLike    = synthBank (0, 4, "sine-like");
        gFormantLike = synthBank (1, 4, "formant-like");
        gMixed2      = mixedBank();

        auto setBank = [] (int a, int b) { return [a, b] (Rig& r, int s) { r.set (ids::bank, (float) (s == 0 ? a : b)); }; };
        auto pubs = [] (std::shared_ptr<const WavetableBank> a, std::shared_ptr<const WavetableBank> b)
        {
            return [a, b] (Rig& r, int s) { r.proc->publishImportedBankForTesting (s == 0 ? a : b); };
        };

        // ---- Exactness tier: every trigger.
        exactness ({ "BANK", base (kSineSaw, 0.0f), { 45 }, 256, setBank (kSineSaw, kFormant), { 0 } });
        exactness ({ "IMPORT-EMPTY-TO-X", base (kImported, 0.3f), { 45 }, 256, pubs (nullptr, gSineLike), { 0 } });
        exactness ({ "IMPORT-X-TO-Y", base (kImported, 0.3f), { 45 }, 256, pubs (gSineLike, gFormantLike), { 0 } });
        exactness ({ "IMPORT-X-TO-EMPTY", base (kImported, 0.3f), { 45 }, 256, pubs (gFormantLike, nullptr), { 0 } });
        exactness ({ "BANDLIMIT", base (kDrive, 1.0f), { 96 }, 256,
                     [] (Rig& r, int s) { r.set (ids::bandlimit, s == 0 ? 1.0f : 0.0f); }, { 0 } });
        exactness ({ "INTERP", base (kImported, 0.5f), { 45 }, 256,
                     [] (Rig& r, int s) { if (r.proc->getImportedForAudioForTesting() != gMixed2.get())
                                              r.proc->publishImportedBankForTesting (gMixed2);   // same bank in every stage
                                          r.set (ids::interp, s == 0 ? 1.0f : 0.0f); }, { 0 } });
        exactness ({ "LEVEL", base (kSineSaw, 1.0f), { 45 }, 256,
                     [] (Rig& r, int s) { r.proc->setForceLevelForTesting (s == 0 ? 8 : 9); }, { 0 } });
        // FOLD: bank switch, then a bandlimit toggle 1 ms (48 samples) into the fade.
        const Scenario fold { "FOLD", base (kSineSaw, 0.0f), { 45 }, 48,
                              [] (Rig& r, int s) { r.set (ids::bank, (float) (s == 0 ? kSineSaw : kFormant));
                                                   r.set (ids::bandlimit, s == 2 ? 0.0f : 1.0f); }, { 0, 48 } };
        exactness (fold);

        // ---- Ratio tier: bank and import swaps (discriminating stimuli).
        for (int note : { 33, 45 })
        {
            const std::string tag = note == 33 ? "A1" : "A2";
            ratioTier ({ "BANK-SAW0-TO-FORMANT0-" + tag, base (kSineSaw, 0.0f), { note }, 256, setBank (kSineSaw, kFormant), { 0 } }, 8);
            ratioTier ({ "BANK-FORMANT0-TO-SAW0-" + tag, base (kSineSaw, 0.0f), { note }, 256, setBank (kFormant, kSineSaw), { 0 } }, 8);
        }
        ratioTier ({ "IMPORT-SWAP-A2", base (kImported, 0.0f), { 45 }, 256, pubs (gSineLike, gFormantLike), { 0 } }, 8);
        ratioTier ({ "IMPORT-TO-EMPTY-A2", base (kImported, 0.0f), { 45 }, 256, pubs (gSineLike, nullptr), { 0 } }, 8);
        {
            std::vector<int> chord;
            for (int n = 33; n < 33 + 16; ++n)
                chord.push_back (n);
            ratioTier ({ "IMPORT-16-HELD", base (kImported, 0.0f), chord, 256, pubs (gSineLike, gFormantLike), { 0 } }, 1);
        }
        ratioTier ({ "FOLD-A2", fold.params, fold.notes, fold.block, fold.apply, fold.triggerOffsets }, 8);
    }

    //==========================================================================
    // Bare-processor rendering (blocks of 256; events at absolute positions
    // from this call's start).
    std::vector<float> renderN (Proc& p, int n, const std::vector<Ev>& evs = {})
    {
        std::vector<float> out;
        juce::AudioBuffer<float> b (2, 256);
        juce::MidiBuffer m;
        for (int done = 0; done < n;)
        {
            const int len = juce::jmin (256, n - done);
            b.setSize (2, len, false, false, true);
            m.clear();
            for (const auto& e : evs)
                if (e.pos >= done && e.pos < done + len)
                    m.addEvent (e.msg, e.pos - done);
            p.processBlock (b, m);
            const float* l = b.getReadPointer (0);
            for (int i = 0; i < len; ++i)
            {
                if (! std::isfinite (l[i]))
                    ++gNonFinite;
                out.push_back (l[i]);
            }
            gSamplesChecked += len;
            done += len;
        }
        return out;
    }

    std::vector<Ev> chord16()
    {
        std::vector<Ev> e;
        for (int n = 33; n < 33 + 16; ++n)
            e.push_back (evOn (0, n));
        return e;
    }

    //==========================================================================
    // REG-01 + D-C reaper rules (RESEARCH 7.3).
    void gateReaperQuiescent()
    {
        auto p = makePrepared();
        setParam (*p, ids::bank, (float) kDrive);
        auto A = synthBank (0, 2, "A");
        std::weak_ptr<const WavetableBank> wa = A;
        p->publishImportedBankForTesting (std::move (A));
        p->publishImportedBankForTesting (synthBank (1, 2, "B"));
        const int retiredAfter = p->getRetiredBankCountForTesting();
        p->sweepNowForTesting();
        const int retiredSwept = p->getRetiredBankCountForTesting();
        report ("G-REAP-QUIESCENT", retiredAfter == 1 && retiredSwept == 0 && wa.expired(),
                "bank = Drive, no block ever rendered: after A -> B retired " + std::to_string (retiredAfter)
                + " (want 1), after sweep " + std::to_string (retiredSwept) + " (want 0), A freed "
                + (wa.expired() ? "yes" : "no"));
    }

    void gateReaperHeld()
    {
        auto p = makePrepared();
        setParam (*p, ids::bank, (float) kImported);
        auto X = synthBank (0, 2, "X");
        const WavetableBank* x = X.get();
        std::weak_ptr<const WavetableBank> wx = X;
        p->publishImportedBankForTesting (std::move (X));
        renderN (*p, 256, { evOn (0, 45) });                       // one block with a held note, then the host idles
        const bool heldX = p->getAudioHeldBankForTesting() == x;

        auto Y = synthBank (1, 2, "Y");
        const WavetableBank* y = Y.get();
        p->publishImportedBankForTesting (std::move (Y));
        p->sweepNowForTesting();
        const int retIdle = p->getRetiredBankCountForTesting();
        const bool xAliveIdle = ! wx.expired();

        renderN (*p, 256);                                         // the voice crossfades X -> Y
        p->sweepNowForTesting();
        const int retAfter = p->getRetiredBankCountForTesting();
        const bool heldY = p->getAudioHeldBankForTesting() == y;
        report ("G-REAP-HELD", heldX && retIdle == 1 && xAliveIdle && retAfter == 0 && wx.expired() && heldY,
                std::string ("held == X after the note block: ") + (heldX ? "yes" : "no") + "; idle import Y + sweep: retired "
                + std::to_string (retIdle) + " (want 1), X alive " + (xAliveIdle ? "yes" : "no") + "; after 1 block + sweep: retired "
                + std::to_string (retAfter) + " (want 0), X freed " + (wx.expired() ? "yes" : "no") + ", held == Y "
                + (heldY ? "yes" : "no"));
    }

    // D-C negative control (graveyard mode: reaped banks are parked and
    // flagged, so a use-after-free is COUNTED instead of crashing).
    int dcScenario (bool disableHeldExclusion, int& graveyardAfterIdleSweep)
    {
        auto p = makePrepared();
        p->setGraveyardModeForTesting (true);
        p->setDisableHeldExclusionForTesting (disableHeldExclusion);
        setParam (*p, ids::bank, (float) kImported);
        p->publishImportedBankForTesting (synthBank (0, 2, "X"));
        renderN (*p, 256, { evOn (0, 45) });                       // voice holds X; host goes idle
        p->publishImportedBankForTesting (synthBank (1, 2, "Y"));
        p->sweepNowForTesting();
        graveyardAfterIdleSweep = p->getGraveyardCountForTesting();
        Proc::resetDerefAfterReapForTesting();
        renderN (*p, 512);                                         // frozen-cycle capture reads the voice's old bank
        const int deref = Proc::getDerefAfterReapForTesting();
        p->clearGraveyardForTesting();                             // nothing in flight
        return deref;
    }

    void gateReaperDC()
    {
        int gNeg = 0, gFix = 0;
        const int derefNeg = dcScenario (true, gNeg);
        const int derefFix = dcScenario (false, gFix);
        Proc::resetDerefAfterReapForTesting();
        report ("G-REAP-DC", derefNeg >= 1 && derefFix == 0 && gNeg == 1 && gFix == 0,
                "idle host, voice on X, import Y, sweep, next block: WITHOUT the amendment (disableHeldExclusion) X reaped "
                + std::to_string (gNeg) + " (want 1), deref-after-reap " + std::to_string (derefNeg) + " (want >= 1); WITH it X reaped "
                + std::to_string (gFix) + " (want 0), deref-after-reap " + std::to_string (derefFix) + " (want 0)");
    }

    struct Plus2State
    {
        Proc* p = nullptr;
        std::weak_ptr<const WavetableBank> wx;
        int calls = 0;
        std::uint64_t stamp = 0;
        std::vector<std::uint64_t> exitsAt;
        std::vector<bool> inFlight, freed;
    };

    void plus2Callback (void* ctx)
    {
        auto& s = *static_cast<Plus2State*> (ctx);
        const auto entries = s.p->getBlockEntriesForTesting(), exits = s.p->getBlockExitsForTesting();
        if (s.calls == 0)
        {
            s.p->publishImportedBankForTesting (nullptr);          // retire X with stamp = exits = e
            s.stamp = exits;
        }
        s.p->sweepNowForTesting();
        s.exitsAt.push_back (exits);
        s.inFlight.push_back (entries != exits);
        s.freed.push_back (s.wx.expired());
        ++s.calls;
    }

    void gateReaperPlus2()
    {
        auto p = makePrepared();
        setParam (*p, ids::bank, (float) kDrive);                  // held is a built-in, never X
        auto X = synthBank (0, 2, "X");
        Plus2State s;
        s.p = p.get();
        s.wx = X;
        p->publishImportedBankForTesting (std::move (X));
        renderN (*p, 256 * 2);                                     // exits = 2 before the scenario
        p->setMidBlockCallbackForTesting (plus2Callback, &s);
        renderN (*p, 256 * 4);
        p->setMidBlockCallbackForTesting (nullptr, nullptr);

        bool ok = s.calls == 4;
        std::string trace;
        for (int i = 0; i < s.calls; ++i)
        {
            const bool want = s.exitsAt[(size_t) i] >= s.stamp + 2;
            ok = ok && s.inFlight[(size_t) i] && s.freed[(size_t) i] == want;
            trace += "exits " + std::to_string (s.exitsAt[(size_t) i]) + (s.inFlight[(size_t) i] ? " in-flight " : " IDLE ")
                   + (s.freed[(size_t) i] ? "freed" : "kept") + "; ";
        }
        report ("G-REAP-PLUS2", ok,
                "X retired at stamp e = " + std::to_string (s.stamp) + ", swept inside every block: " + trace
                + "(want kept while exits < e+2 although no other block is running - the quiescent rule never fires in flight - and freed at e+2)");
    }

    void gateReaperSoak (const std::vector<juce::File>& files)
    {
        const int liveBase = WavetableBank::testLiveCount.load();
        auto p = makePrepared();
        p->setGraveyardModeForTesting (true);                      // a UAF is counted, not a crash
        setParam (*p, ids::bank, (float) kImported);
        Proc::resetDerefAfterReapForTesting();

        std::atomic<bool> importsDone { false }, stopSweeper { false };
        std::thread importer ([&]
        {
            juce::Random rng (0x50414b);                           // pacing only
            for (int i = 0; i < 50; ++i)
            {
                p->importFromFile (files[(size_t) (i % 3)]);
                juce::Thread::sleep (rng.nextInt (21));
            }
            importsDone = true;
        });
        std::thread sweeper ([&]
        {
            while (! stopSweeper.load())
            {
                p->sweepNowForTesting();
                juce::Thread::sleep (5);
            }
        });

        int maxHeld = 0, blocks = 0;
        long long nonFiniteBefore = gNonFinite;
        renderN (*p, 256, chord16());
        for (int guard = 0; guard < 2000000; ++guard)
        {
            renderN (*p, 256);
            ++blocks;
            maxHeld = std::max (maxHeld, p->getHeldBankCountForTesting());
            p->handleUpdateNowIfNeeded();
            if (importsDone.load() && p->getImportStatus().state != Proc::ImportStatus::State::busy && blocks > 50)
                break;
        }
        importer.join();
        stopSweeper = true;
        sweeper.join();

        renderN (*p, 256 * 3);
        p->sweepNowForTesting();
        const int retired = p->getRetiredBankCountForTesting();
        const int deref = Proc::getDerefAfterReapForTesting();
        const int parked = p->getGraveyardCountForTesting();
        p->clearGraveyardForTesting();
        const int live = WavetableBank::testLiveCount.load();
        const bool finite = gNonFinite == nonFiniteBefore;
        const auto st = p->getImportStatus();
        report ("G-REAP-SOAK", finite && deref == 0 && maxHeld <= 3 && retired == 0 && liveBase == 5 && live == liveBase + 1
                                   && st.state == Proc::ImportStatus::State::done && parked > 0,
                "16 held notes on Imported, 50 imports (3 files, 0-20 ms gaps) + a 5 ms sweeper thread, " + std::to_string (blocks)
                + " blocks: output finite " + (finite ? "yes" : "no") + ", deref-after-reap " + std::to_string (deref)
                + " (want 0), max held (owner + retired) " + std::to_string (maxHeld) + " (want <= 3); end: retired "
                + std::to_string (retired) + " (want 0), banks reaped " + std::to_string (parked) + " (liveness > 0), live banks "
                + std::to_string (live) + " (want 5 built-ins + 1 = " + std::to_string (liveBase + 1) + ", baseline " + std::to_string (liveBase) + ")");
        Proc::resetDerefAfterReapForTesting();
    }

    //==========================================================================
    // DSP-06 / PERF-01: alloc gate across bank switches and concurrent imports
    // plus restore-publishes. Harness-side "message thread" calls (param
    // sets, restore, auto-select) are serialised by msgLock, as a host would;
    // processBlock runs unlocked, concurrently with the worker publishes.
    void gateAlloc (const std::vector<juce::File>& files)
    {
       #if JUCE_MAC
        // Restore blob carrying an IMPORTED_BANK (made before the gate is armed).
        juce::MemoryBlock blob;
        {
            auto src = makePrepared();
            Proc::ImportStatus st;
            importAndWait (*src, files[2], st);
            blob = saveState (*src);
        }

        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int live = allocCount;
        allocCount = 0;

        int switches = 0, publishes = 0, restores = 0;
        if (live == 1)
        {
            Rig r (256, base (kSineSaw, 0.3f));
            std::mutex msgLock;
            std::atomic<bool> done { false };
            std::thread t ([&]
            {
                for (int i = 0; i < 50; ++i)
                {
                    if (i % 5 == 4)
                    {
                        const std::lock_guard<std::mutex> g (msgLock);
                        r.proc->setStateInformation (blob.getData(), (int) blob.getSize());   // restore-publish
                        ++restores;
                    }
                    else
                    {
                        r.proc->importFromFile (files[(size_t) (i % 2)]);
                        for (int spin = 0; spin < 20000 && r.proc->getImportStatus().state == Proc::ImportStatus::State::busy; ++spin)
                            juce::Thread::sleep (1);
                        ++publishes;
                    }
                }
                done = true;
            });

            gAllocMode = true;
            r.run (256, chord16());
            for (int guard = 0; guard < 2000000 && (! done.load() || switches < 200); ++guard)
            {
                r.run (256, {}, [&] (Rig& rig, int)
                {
                    const std::lock_guard<std::mutex> g (msgLock);
                    rig.set (ids::bank, (float) (switches % 6));
                    rig.proc->handleUpdateNowIfNeeded();
                    ++switches;
                });
            }
            gAllocMode = false;
            t.join();
        }
        malloc_logger = nullptr;

        const int n = allocCount;
        report ("G-ALLOC-24", live == 1 && n == 0 && switches >= 200 && publishes == 40 && restores == 10,
                "liveness malloc(64) counted " + std::to_string (live) + " (want 1); " + std::to_string (n)
                + " audio-thread allocation(s) (want 0) over " + std::to_string (switches) + " bank switches (want >= 200) with "
                + std::to_string (publishes) + " import publishes + " + std::to_string (restores)
                + " restore-publishes from a 2nd thread (want 40 + 10), 16 held notes");
       #else
        juce::ignoreUnused (files);
        report ("G-ALLOC-24", true, "skipped (macOS-only malloc_logger gate)");
       #endif
    }

    //==========================================================================
    // FUNC-03 (RESEARCH 7.4): frame counts and the error vocabulary.
    void gateFunc03()
    {
        auto p = makePrepared();
        struct Case { const char* name; int samples; double sr; int want; };
        const Case cases[] = { { "441000@44.1k", 441000, 44100.0, 215 }, { "600000", 600000, 48000.0, 256 },
                               { "2048", 2048, 48000.0, 1 }, { "2*2048+100", 2 * 2048 + 100, 48000.0, 2 } };
        std::string detail;
        bool ok = true;
        int seed = 20;
        for (const auto& c : cases)
        {
            const auto f = gTemp.getChildFile (juce::String ("count-") + juce::String (seed) + ".wav");
            writeMono16 (f, contentPcm (c.samples, seed++), Fmt::wav, c.sr);
            Proc::ImportStatus st;
            const bool done = importAndWait (*p, f, st);
            const auto b = p->getImportedBankSnapshot();
            const int got = b != nullptr ? b->numFrames : -1;
            ok = ok && done && st.frames == c.want && got == c.want && juce::exactlyEqual (rawOf (*p, ids::bank), (float) kImported);
            detail += std::string (c.name) + " -> " + std::to_string (got) + " (status " + std::to_string (st.frames) + ", want "
                    + std::to_string (c.want) + "); ";
        }
        report ("G-FUNC03-COUNTS", ok, detail + "auto-select Imported after each: " + (ok ? "yes" : "see above"));

        // Rejections leave the existing (2-frame) bank untouched.
        const auto before = p->getImportedBankSnapshot();
        auto untouched = [&p, &before] { return p->getImportedBankSnapshot() == before && p->getImportedForAudioForTesting() == before.get(); };

        const auto shortF = gTemp.getChildFile ("short.wav");
        writeMono16 (shortF, contentPcm (2047, 3));
        Proc::ImportStatus s1;
        importAndWait (*p, shortF, s1);
        const bool u1 = untouched();

        const auto garbage = gTemp.getChildFile ("garbage.wav");
        {
            juce::MemoryBlock g (4096);
            auto* bytes = static_cast<unsigned char*> (g.getData());
            for (size_t i = 0; i < g.getSize(); ++i)
                bytes[i] = static_cast<unsigned char> ((i * 131u + 7u) & 0xffu);
            garbage.replaceWithData (g.getData(), g.getSize());
        }
        Proc::ImportStatus s2;
        importAndWait (*p, garbage, s2);
        const bool u2 = untouched();

        juce::MemoryBlock huge ((size_t) 97 * 1024 * 1024, false);
        const bool accepted = p->importFromMemory ("huge.wav", std::move (huge));
        const auto s3 = p->getImportStatus();
        const bool u3 = untouched();

        report ("G-FUNC03-REJECT", s1.error == "tooShort" && u1 && s2.error == "unreadable" && u2
                                       && ! accepted && s3.error == "tooLarge" && u3 && before != nullptr,
                "2047 samples -> '" + str (s1.error) + "' (want tooShort), bank untouched " + (u1 ? "yes" : "no")
                + "; garbage .wav -> '" + str (s2.error) + "' (want unreadable), untouched " + (u2 ? "yes" : "no")
                + "; 97 MB memory import -> accepted " + (accepted ? "yes" : "no") + ", '" + str (s3.error)
                + "' (want tooLarge), untouched " + (u3 ? "yes" : "no"));
    }

    //==========================================================================
    // COMPAT-03: format x rate identity, bit depths, NaN scrub, channel mean.
    int maxPcmDiff (const std::vector<std::int16_t>& a, const std::vector<std::int16_t>& b)
    {
        if (a.size() != b.size())
            return 1 << 20;
        int m = 0;
        for (size_t i = 0; i < a.size(); ++i)
            m = std::max (m, std::abs ((int) a[i] - (int) b[i]));
        return m;
    }

    std::vector<float> asFloat (const std::vector<std::int16_t>& v)
    {
        std::vector<float> f (v.size());
        for (size_t i = 0; i < v.size(); ++i)
            f[i] = (float) v[i] / 32768.0f;                        // exactly what the 16-bit readers produce
        return f;
    }

    void gateCompat03()
    {
        const auto content = contentPcm (3 * 2048 + 333, 7);
        std::vector<std::int16_t> refPcm;
        std::shared_ptr<const WavetableBank> refBank;
        int matchesPcm = 0, matchesBank = 0, total = 0;
        std::string detail;
        auto p = makePrepared();

        for (Fmt f : { Fmt::wav, Fmt::aiff, Fmt::flac })
            for (double sr : { 44100.0, 48000.0, 96000.0 })
            {
                const char* ext = f == Fmt::wav ? ".wav" : (f == Fmt::aiff ? ".aiff" : ".flac");
                const auto file = gTemp.getChildFile (juce::String ("compat-") + juce::String ((int) sr) + ext);
                const bool wrote = writeMono16 (file, content, f, sr);
                const auto res = decodeFile (file);
                Proc::ImportStatus st;
                importAndWait (*p, file, st);
                const auto b = p->getImportedBankSnapshot();
                if (total == 0)
                {
                    refPcm = res.pcm.pcm;
                    refBank = b;
                }
                const bool pcmSame = wrote && res.error == ImportError::none && res.pcm.numFrames == 3 && res.pcm.pcm == refPcm;
                const bool bankSame = st.frames == 3 && sameBank (b.get(), refBank.get());
                matchesPcm += pcmSame ? 1 : 0;
                matchesBank += bankSame ? 1 : 0;
                ++total;
                detail += std::string (ext + 1) + "@" + std::to_string ((int) sr) + (pcmSame && bankSame ? " ok" : " MISMATCH") + ", ";
            }
        report ("G-COMPAT03-FORMATS", total == 9 && matchesPcm == 9 && matchesBank == 9,
                detail + "identical int16 PCM " + std::to_string (matchesPcm) + "/9, bit-identical banks " + std::to_string (matchesBank) + "/9");

        // 24-bit WAV of the same content -> the same int16 PCM.
        {
            const auto file = gTemp.getChildFile ("compat-24.wav");
            writeInt (file, Fmt::wav, 48000.0, 24, { content });
            const auto res = decodeFile (file);
            report ("G-COMPAT03-24BIT", res.error == ImportError::none && res.pcm.pcm == refPcm,
                    "24-bit WAV -> " + std::to_string (res.pcm.numFrames) + " frames, int16 PCM identical to the 16-bit import: "
                    + (res.pcm.pcm == refPcm ? "yes" : "no"));
        }

        // Float WAV with a NaN and an inf: scrubbed to 0 (== the same file with
        // zeros written there), every bank sample finite.
        {
            auto fl = asFloat (content);
            auto zeroed = fl;
            fl[1000] = std::numeric_limits<float>::quiet_NaN();
            fl[5000] = std::numeric_limits<float>::infinity();
            zeroed[1000] = 0.0f;
            zeroed[5000] = 0.0f;
            const auto fNan = gTemp.getChildFile ("nan.wav"), fZero = gTemp.getChildFile ("nan-zero.wav");
            writeFloatWav (fNan, { fl });
            writeFloatWav (fZero, { zeroed });
            const auto rn = decodeFile (fNan), rz = decodeFile (fZero);
            auto bank = imp::buildImportedBank (rn.pcm);
            bool finite = bank != nullptr;
            if (bank != nullptr)
                for (float v : bank->data)
                    finite = finite && std::isfinite (v);
            const bool liveness = maxPcmDiff (rz.pcm.pcm, refPcm) > 0;   // the zeroed samples do change the frame
            report ("G-COMPAT03-FLOAT-NAN", rn.error == ImportError::none && rn.pcm.pcm == rz.pcm.pcm && finite && liveness,
                    std::string ("float WAV with NaN + inf -> imported ") + (rn.error == ImportError::none ? "yes" : "no")
                    + ", int16 PCM == the zero-substituted file: " + (rn.pcm.pcm == rz.pcm.pcm ? "yes" : "no")
                    + ", bank finite: " + (finite ? "yes" : "no") + "; liveness (zeros differ from the clean import): "
                    + (liveness ? "yes" : "no"));
        }

        // Channel mean: compare against a mono float file holding the exact
        // channel SUM (scale is irrelevant after the per-frame normalise;
        // +/-1 LSB for the 1/C rounding), and prove it is not left-only /
        // not the mean of the first two.
        auto meanCase = [] (int nc, const char* tag)
        {
            const int n = 2 * 2048;
            std::vector<std::vector<std::int16_t>> ch;
            for (int c = 0; c < nc; ++c)
            {
                auto v = contentPcm (n, 31 + 5 * c);
                for (auto& s : v)
                    s = (std::int16_t) (s / 2);                    // headroom: the sum of 6 still fits the float file
                ch.push_back (v);
            }
            std::vector<float> sumAll ((size_t) n, 0.0f), sum2 ((size_t) n, 0.0f);
            for (int c = 0; c < nc; ++c)
            {
                const auto f = asFloat (ch[(size_t) c]);
                for (int i = 0; i < n; ++i)
                {
                    sumAll[(size_t) i] += f[(size_t) i];
                    if (c < 2)
                        sum2[(size_t) i] += f[(size_t) i];
                }
            }
            const auto fMulti = gTemp.getChildFile (juce::String ("mean-") + tag + ".wav");
            const auto fAll   = gTemp.getChildFile (juce::String ("mean-") + tag + "-sum.wav");
            const auto fLeft  = gTemp.getChildFile (juce::String ("mean-") + tag + "-left.wav");
            const auto fTwo   = gTemp.getChildFile (juce::String ("mean-") + tag + "-two.wav");
            writeInt (fMulti, Fmt::wav, 48000.0, 16, ch);
            writeFloatWav (fAll, { sumAll });
            writeFloatWav (fLeft, { asFloat (ch[0]) });
            writeFloatWav (fTwo, { sum2 });
            const auto rM = decodeFile (fMulti), rA = decodeFile (fAll), rL = decodeFile (fLeft), rT = decodeFile (fTwo);
            const int dAll = maxPcmDiff (rM.pcm.pcm, rA.pcm.pcm);
            const int dLeft = maxPcmDiff (rM.pcm.pcm, rL.pcm.pcm);
            const int dTwo = maxPcmDiff (rM.pcm.pcm, rT.pcm.pcm);
            const bool ok = rM.error == ImportError::none && dAll <= 1 && dLeft >= 100 && (nc == 2 || dTwo >= 100);
            report (std::string ("G-COMPAT03-MEAN-") + tag, ok,
                    std::to_string (nc) + " channels: max |pcm - mean-of-all| " + std::to_string (dAll) + " LSB (<= 1); vs left-only "
                    + std::to_string (dLeft) + " LSB (>= 100)" + (nc == 2 ? std::string() : "; vs mean of first 2 " + std::to_string (dTwo) + " LSB (>= 100)"));
        };
        meanCase (2, "STEREO");
        meanCase (6, "6CH");
    }

    //==========================================================================
    // FUNC-04 (RESEARCH 7.5): bit-identical persistence.
    juce::XmlElement* bankChild (juce::XmlElement& x)
    {
        return x.getChildByName ("IMPORTED_BANK");
    }

    juce::MemoryBlock withBlob (const juce::MemoryBlock& save, const juce::String& encoding, const juce::String& data)
    {
        auto x = blobToXml (save);
        if (x == nullptr || bankChild (*x) == nullptr)
            return {};
        bankChild (*x)->setAttribute ("encoding", encoding);
        bankChild (*x)->setAttribute ("data", data);
        return xmlToBlob (*x);
    }

    juce::String dataOf (const juce::MemoryBlock& save, int* children = nullptr)
    {
        auto x = blobToXml (save);
        if (x == nullptr)
            return {};
        if (children != nullptr)
        {
            *children = 0;
            for (auto* c : x->getChildIterator())
                if (c->hasTagName ("IMPORTED_BANK"))
                    ++*children;
        }
        auto* c = bankChild (*x);
        return c != nullptr ? c->getStringAttribute ("data") : juce::String();
    }

    void gateFunc04()
    {
        const auto file = gTemp.getChildFile ("persist me.wav");
        writeMono16 (file, contentPcm (5 * 2048 + 999, 42));

        auto p1 = makePrepared();
        Proc::ImportStatus st;
        importAndWait (*p1, file, st);
        const auto b1 = p1->getImportedBankSnapshot();
        const auto save1 = saveState (*p1);
        int kids1 = 0;
        const auto data1 = dataOf (save1, &kids1);

        auto p2 = makePrepared();
        loadState (*p2, save1);
        const auto b2 = p2->getImportedBankSnapshot();
        const auto save2 = saveState (*p2);
        const auto data2 = dataOf (save2);
        const auto s2 = p2->getImportStatus();
        const bool levels = sameBank (b1.get(), b2.get());
        const bool meta = b1 != nullptr && b2 != nullptr && b1->numFrames == 5 && b2->numFrames == 5
                       && b2->name == "persist me.wav" && s2.filename == "persist me.wav" && s2.frames == 5;
        report ("G-FUNC04-ROUNDTRIP", st.state == Proc::ImportStatus::State::done && kids1 == 1 && levels && meta
                                          && data1.isNotEmpty() && data2 == data1,
                "import -> save (" + std::to_string (kids1) + " child, " + std::to_string (data1.length()) + " chars) -> fresh restore: memcmp all 11 levels x "
                + std::to_string (b1 != nullptr ? b1->numFrames : -1) + " frames " + (levels ? "equal" : "DIFFER") + " ("
                + std::to_string (b1 != nullptr ? b1->data.size() * sizeof (float) : 0) + " bytes); filename/numFrames "
                + (meta ? "equal" : "DIFFER") + "; re-saved data string verbatim: " + (data2 == data1 ? "yes" : "no"));

        // Negative control: one int16 changed must fail, through the builder
        // and through a real restore.
        ImportedPcm pcm;
        {
            juce::MemoryOutputStream mos;
            juce::Base64::convertFromBase64 (mos, data1);
            pcm.numFrames = 5;
            pcm.filename = "persist me.wav";
            imp::decodeFlac16 (mos.getMemoryBlock(), 5, pcm.pcm);
        }
        auto mutated = pcm;
        if (mutated.pcm.size() > 1234)
            mutated.pcm[1234] = (std::int16_t) (mutated.pcm[1234] ^ 1);
        const auto bm = imp::buildImportedBank (mutated);
        auto p3 = makePrepared();
        loadState (*p3, withBlob (save1, "flac16", imp::encodeFlac16 (mutated.pcm)));
        const auto b3 = p3->getImportedBankSnapshot();
        const bool sameAsImport = sameBank (imp::buildImportedBank (pcm).get(), b1.get());
        report ("G-FUNC04-NEG", sameAsImport && ! sameBank (bm.get(), b1.get()) && b3 != nullptr && ! sameBank (b3.get(), b1.get()),
                std::string ("decoded blob rebuilt == imported bank: ") + (sameAsImport ? "yes" : "no")
                + "; one int16 flipped -> builder compare fails: " + (! sameBank (bm.get(), b1.get()) ? "yes" : "no")
                + ", restore compare fails: " + (b3 != nullptr && ! sameBank (b3.get(), b1.get()) ? "yes" : "no"));

        // pcm16gz fallback path: same PCM -> bit-identical bank, data verbatim.
        const auto gz = imp::encodePcm16Gz (pcm.pcm);
        auto p4 = makePrepared();
        loadState (*p4, withBlob (save1, "pcm16gz", gz));
        const auto b4 = p4->getImportedBankSnapshot();
        const auto save4 = saveState (*p4);
        auto x4 = blobToXml (save4);
        const auto enc4 = x4 != nullptr && bankChild (*x4) != nullptr ? bankChild (*x4)->getStringAttribute ("encoding") : juce::String();
        report ("G-FUNC04-PCM16GZ", gz.isNotEmpty() && sameBank (b4.get(), b1.get()) && dataOf (save4) == gz && enc4 == "pcm16gz",
                "pcm16gz blob (" + std::to_string (gz.length()) + " chars vs flac16 " + std::to_string (data1.length())
                + ") restores bit-identical: " + (sameBank (b4.get(), b1.get()) ? "yes" : "no") + "; re-saved verbatim as "
                + str (enc4) + ": " + (dataOf (save4) == gz ? "yes" : "no"));

        // Absent child: empty Imported, Bank = Imported renders exact silence.
        auto p0 = makePrepared();
        setParam (*p0, ids::bank, (float) kImported);
        const auto save0 = saveState (*p0);
        int kids0 = -1;
        dataOf (save0, &kids0);
        auto p5 = makePrepared();
        p5->publishImportedBankForTesting (synthBank (0, 2, "stale"));   // must be cleared by the restore
        loadState (*p5, save0);
        const bool empty = p5->getImportedBankSnapshot() == nullptr;
        const auto quiet = renderN (*p5, 4096, { evOn (0, 57) });
        const bool silent = allExactZero (quiet);
        p5->publishImportedBankForTesting (synthBank (0, 2, "live"));
        const auto loud = renderN (*p5, 4096, { evOn (0, 57) });
        const double pk = peakAbs (loud, 0, (int) loud.size());
        report ("G-FUNC04-ABSENT", kids0 == 0 && empty && silent && pk > 0.1,
                "pre-import save has " + std::to_string (kids0) + " child (want 0); restore over a loaded bank -> Imported empty "
                + (empty ? "yes" : "no") + ", Bank = Imported note renders exact 0: " + (silent ? "yes" : "no")
                + "; liveness: with a bank published the same note peaks " + fmt (pk, 3) + " (> 0.1)");
    }
}

int main (int, char**)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    // ONE BuiltInBanks for the whole run (the 5 built-ins stay live).
    juce::SharedResourcePointer<BuiltInBanks> banks;
    std::cout << "import-check: built-in banks ready (buildMillis " << fmt (banks->buildMillis, 2) << " ms)\n";

    gTemp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                .getChildFile ("osiw-import-check-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
    if (! gTemp.createDirectory())
    {
        std::cout << "FAIL setup: cannot create temp dir\n";
        return 1;
    }

    gateQual03();
    gSineLike.reset();                    // harness-held banks released (soak counts live banks)
    gFormantLike.reset();
    gMixed2.reset();

    std::vector<juce::File> small;        // 2..4-frame fixtures for the soak / alloc gate
    for (int i = 0; i < 3; ++i)
    {
        small.push_back (gTemp.getChildFile ("small" + juce::String (i) + ".wav"));
        writeMono16 (small.back(), contentPcm ((2 + i) * 2048 + 17 * i, 10 + i));
    }

    gateReaperQuiescent();
    gateReaperHeld();
    gateReaperDC();
    gateReaperPlus2();
    gateReaperSoak (small);
    gateAlloc (small);
    gateFunc03();
    gateCompat03();
    gateFunc04();

    report ("G-FINITE", gNonFinite == 0 && gChannelMismatch == 0,
            std::to_string (gNonFinite) + " non-finite, " + std::to_string (gChannelMismatch)
            + " L != R over " + std::to_string (gSamplesChecked) + " samples");

    gTemp.deleteRecursively();
    std::cout << "import-check: " << (gFailures == 0 ? std::string ("ALL PASS") : "FAILURES = " + std::to_string (gFailures))
              << "\n";
    return gFailures == 0 ? 0 : 1;
}
