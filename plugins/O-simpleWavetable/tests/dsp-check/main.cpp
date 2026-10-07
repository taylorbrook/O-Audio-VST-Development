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

    O-simpleWavetable dsp-check - Stage 2.2 gates (RESEARCH 6.6 + PLAN Task 8).

    Every gate renders audio through the real processor (or, for the
    voice-level arms, a WtVoice driven directly) and has a negative control
    or liveness check.

      G-PITCH     FUNC-01  MIDI 21..108, 44.1k + 48k, pos 0 and pos 1:
                           |cents| <= 1. Neg: wheel 8192+68 must FAIL.
      G-BEND               wheel 16383 / 0 on A4 -> 440 * 2^(+/-bend/12) +/- 1 cent
      G-Q2-C8     QUAL-02  Drive 32, Pulse frame 1, voice-level Saw-1023, MIDI 108,
                           44.1/48/96k, band-limit On: <= -100 dB
      G-Q2-SWEEP  QUAL-02  same frames, MIDI 21..108, 3 rates: <= -70 dB (needs D-A)
      G-Q2-PULSE  D-B      Pulse 16/24/32, equal-RMS sine metric: <= -60 dB
      G-DSP02     DSP-02   band-limit Off, Saw 32, C7, 44.1k: > -40 dB (analyzer
                           liveness); On: <= -100 dB
      G-DSP01     DSP-01   fs 56320, A4 (period 128), position stepped per block:
                           Off = 32 distinct cycles; On >= 500 and max spectral
                           step < 1/10 of Off's. Neg: Off != On.
      G-DSP03     DSP-03   3 bits: exactly the 8 values +/-{1,3,5,7}/16.
                           Neg: Full > 1000 distinct values.
      G-FULL      DSP-03   Full = bitwise bypass over 10^6 values; 3 -> Full render
                           == fresh Full render
      G-READ      viz seam voice output == wt::readSample + quant, within 1 ulp
      G-POLY      FUNC-07  16 notes present; 17th steals the oldest unprotected
      G-MONO      FUNC-07  true legato: pitch only, no re-attack; back to held
                           note. Neg: Poly shows both notes.
      G-RETRIG             Mono retrigger from a release tail: no reset click
      G-STEAL      QUAL-01 voice steal at the stolen voice's phase peak: max |step|
                           <= 1.5x steady; neg control (no tail fade) >= 4x  (W1)
      G-SWITCH-TAIL QUAL-01 Poly -> Mono with 4 held: same ratio rule  (W1)
      G-RETRIG-VEL QUAL-01 Mono vel 127 -> off -> vel 38 at a phase peak: same ratio
                           rule (neg: instant velGain); settled level unchanged  (W2)
      G-NOTEOFF            sustain 0 release tail survives the per-block push.
                           Neg: raw ADSR with per-block setParameters collapses.
      G-VEL                vel 64 vs 127 = -11.90 +/- 0.05 dB
      G-OUT                -60 dB = exact 0; 0 dB peak ~0.5 at 10 ms (seeded).
                           Neg: unseeded ramp gives ~0.25.
      G-IMPEMPTY           bank 5 = exact 0 while the env runs; sound after -> bank 0
      G-BLOCK              prepare(256): one 1024 block == 4 x 256, bitwise
      G-FINITE             every rendered sample finite, L == R

    Stage 4 (4-polish PLAN Tasks 2-3; each with a negative control):
      G-UNPREPARED  note 4  self-exec child (posix_spawn, never fork): exactly 0
                            before prepareToPlay, sound after prepare and after
                            releaseResources. NC (guard off): child dies by signal.
      G-MIDI-FLOOD  note 3  6000 events in one host block (Poly CC / Mono wheel):
                            wheel after the flood +2 st +/- 1 c, note-off lands.
                            --alloc-check: flood blocks 0 allocations; NC (guard
                            off, fresh rig) >= 1.
      G-MONO-WHEEL  S2 W3   Poly wheel history -> Mono: |cents| <= 1 (arms A, B, C).
                            NC (no seed): A >= +150 c, B >= +75 c.
      G-LEGATO-XF   S2 W4   5 pitch-change triggers: fade == (1-w) y_old + w y_hard
                            <= 1e-6; NC (keep-rate off) >= 0.1; fix vs NC
                            bit-identical outside [t, t+240).
      G-LEGATO-ALIAS        48 kHz, 2 s magnifier: <= no-jump floor + 1 dB; NC
                            >= floor + 40 dB.
      G-LEGATO-CLICK        max |dy| fade / steady <= 1.5; NC > 1.5.

    --alloc-check: O-Bells malloc_logger gate (volatile flag + counter,
    audio-thread scoped, liveness malloc(64) must count 1, warm-up block
    unarmed) over the G-ALLOC stimulus list. Exit 0 only if 0 allocations.

    Off by default; -DOUARICON_BUILD_TESTS=ON (OSIW_TEST_HOOKS=1).

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "BankFactory.h"
#include "BitQuantizer.h"
#include "BuiltInBanks.h"
#include "MipmapBuilder.h"
#include "PluginProcessor.h"
#include "PositionSmoother.h"
#include "WavetableBank.h"
#include "WtRead.h"
#include "WtVoice.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if JUCE_MAC
 #include <execinfo.h>
 #include <fcntl.h>
 #include <pthread.h>
 #include <spawn.h>
 #include <sys/wait.h>

// libmalloc's stack-logging hook (what MallocStackLogging / Instruments attach to).
extern "C"
{
    typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                    uintptr_t result, uint32_t numHotFramesToSkip);
    extern malloc_logger_t* malloc_logger;
}

extern char** environ;   // posix_spawn of the G-UNPREPARED child
#endif

namespace
{
    using Proc = OSimpleWavetableAudioProcessor;
    namespace ids = OSimpleWavetable::ParamIDs;

    constexpr double kPi = 3.14159265358979323846;

    int gFailures = 0;
    long long gNonFinite = 0;
    long long gChannelMismatch = 0;
    long long gSamplesChecked = 0;
    bool gAllocMode = false;

    void report (const char* gate, bool ok, const std::string& detail)
    {
        std::cout << (ok ? "PASS " : "FAIL ") << gate << (ok ? " " : ": ") << detail << "\n";
        std::cout.flush();
        if (! ok)
            ++gFailures;
    }

    void info (const std::string& line)
    {
        std::cout << "  " << line << "\n";
    }

    std::string fmt (double v, int prec = 2)
    {
        std::ostringstream s;
        s << std::fixed << std::setprecision (prec) << v;
        return s.str();
    }

    std::string sciStr (double v)
    {
        std::ostringstream s;
        s << std::scientific << std::setprecision (2) << v;
        return s.str();
    }

    double dB (double ratio)
    {
        return 20.0 * std::log10 (std::max (ratio, 1.0e-300));
    }

    int secs (double t, double fs)
    {
        return (int) std::lround (t * fs);
    }

    double noteHz (int note)
    {
        return 440.0 * std::pow (2.0, (double) (note - 69) / 12.0);
    }

    //==========================================================================
    // Alloc gate (O-Bells pattern). volatile: clang treats malloc as a builtin
    // that cannot touch globals and would fold "armed = true; malloc();
    // armed = false" otherwise.
   #if JUCE_MAC
    volatile bool allocArmed = false;
    volatile int  allocCount = 0;
    bool allocTrace = false;
    pthread_t audioThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        // Process-wide hook: only the thread that calls processBlock counts.
        if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), audioThread))   // MALLOC_LOG_TYPE_ALLOCATE
        {
            allocArmed = false;   // the first one is the finding; say where it came from
            allocCount = allocCount + 1;
            if (allocTrace)
            {
                void* frames[32];
                backtrace_symbols_fd (frames, backtrace (frames, 32), 2);   // does not malloc
            }
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

    int allocCountNow()
    {
       #if JUCE_MAC
        return allocCount;
       #else
        return 0;
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

    Ev evOff (int pos, int note)
    {
        return { pos, juce::MidiMessage::noteOff (1, note) };
    }

    Ev evWheel (int pos, int value)
    {
        return { pos, juce::MidiMessage::pitchWheel (1, juce::jlimit (0, 16383, value)) };
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
        return std::numeric_limits<float>::quiet_NaN();
    }

    // Steady single-voice baseline: instant attack, sustain 1, 0 dB out.
    Params baseParams (int bank, float pos)
    {
        return { { ids::bank, (float) bank }, { ids::position, pos }, { ids::interp, 1.0f },
                 { ids::bandlimit, 1.0f }, { ids::bitDepth, 0.0f },
                 { ids::ampAttack, 0.001f }, { ids::ampDecay, 0.001f }, { ids::ampSustain, 1.0f },
                 { ids::ampRelease, 0.05f }, { ids::voiceMode, 0.0f }, { ids::outputLevel, 0.0f } };
    }

    Params with (Params p, const char* id, float v)
    {
        p.emplace_back (id, v);   // applied in order: the later entry wins
        return p;
    }

    //==========================================================================
    // Processor rig: params set BEFORE prepare (so smoothers seed on them),
    // events at absolute sample positions from the rig start.
    struct Rig
    {
        std::unique_ptr<Proc> proc;
        int block;
        int cursor = 0;
        int blocksRun = 0;
        juce::AudioBuffer<float> buf;
        juce::MidiBuffer midi;

        Rig (double fs, int blockSize, const Params& params)
            : proc (std::make_unique<Proc>()), block (blockSize), buf (2, blockSize)
        {
            for (const auto& pr : params)
                setParam (*proc, pr.first, pr.second);
            proc->setPlayConfigDetails (0, 2, fs, blockSize);
            proc->prepareToPlay (fs, blockSize);
            midi.ensureSize (16384);
        }

        void set (const char* id, float v) { setParam (*proc, id, v); }

        std::vector<float> run (int n, const std::vector<Ev>& evs = {}, int blockOverride = 0)
        {
            std::vector<float> out;
            out.reserve ((size_t) juce::jmax (0, n));
            const int blk = blockOverride > 0 ? blockOverride : block;

            for (int done = 0; done < n;)
            {
                const int len = juce::jmin (blk, n - done);
                buf.setSize (2, len, false, false, true);
                for (int ch = 0; ch < 2; ++ch)
                    juce::FloatVectorOperations::fill (buf.getWritePointer (ch), 1.0f, len);   // processBlock must clear

                midi.clear();
                const int t0 = cursor + done;
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
    // Voice-level rig: a WtVoice driven directly with its own BlockContext.
    struct VoiceRig
    {
        WtVoice voice;
        WtSound sound;
        BlockContext ctx;
        std::vector<float> knob, zero;
        int blk;
        int cursor = 0;

        VoiceRig (const WavetableBank* bank, double fs, int blockSize, bool interp, bool bandlimit, int bitIdx,
                  const juce::ADSR::Parameters& amp)
            : knob ((size_t) blockSize, 0.0f), zero ((size_t) blockSize, 0.0f), blk (blockSize)
        {
            ctx.knobPos = knob.data();
            ctx.lfo = zero.data();
            ctx.lfoDepth = zero.data();
            ctx.envAmount = zero.data();
            ctx.bank = bank;
            ctx.interp = interp;
            ctx.bandlimit = bandlimit;
            ctx.bitDepthIndex = bitIdx;
            voice.setBlockContext (&ctx);
            voice.prepareToPlay (fs, blockSize, amp);
            voice.setBlockParams (amp);
        }

        void start (int note, float vel) { voice.startNote (note, vel, &sound, 8192); }

        std::vector<float> run (int n, const std::function<float (int)>& knobAt)
        {
            std::vector<float> out;
            out.reserve ((size_t) n);
            juce::AudioBuffer<float> b (1, blk);
            for (int done = 0; done < n;)
            {
                const int len = juce::jmin (blk, n - done);
                for (int i = 0; i < len; ++i)
                    knob[(size_t) i] = knobAt (cursor + done + i);
                b.clear();
                voice.renderNextBlock (b, 0, len);
                const float* d = b.getReadPointer (0);
                out.insert (out.end(), d, d + len);
                done += len;
            }
            cursor += n;
            return out;
        }
    };

    juce::ADSR::Parameters steadyAmp() { return { 0.001f, 0.001f, 1.0f, 0.05f }; }

    //==========================================================================
    // Analysis helpers
    double rms (const std::vector<float>& x, int from, int to)
    {
        from = juce::jlimit (0, (int) x.size(), from);
        to   = juce::jlimit (from, (int) x.size(), to);
        if (to <= from)
            return 0.0;
        double s = 0.0;
        for (int i = from; i < to; ++i)
            s += (double) x[(size_t) i] * (double) x[(size_t) i];
        return std::sqrt (s / (double) (to - from));
    }

    double peakAbs (const std::vector<float>& x, int from, int to)
    {
        double p = 0.0;
        for (int i = juce::jmax (0, from); i < juce::jmin ((int) x.size(), to); ++i)
            p = std::max (p, (double) std::abs (x[(size_t) i]));
        return p;
    }

    double maxStep (const std::vector<float>& x, int from, int to)
    {
        double m = 0.0;
        for (int i = juce::jmax (1, from); i < juce::jmin ((int) x.size(), to); ++i)
            m = std::max (m, (double) std::abs (x[(size_t) i] - x[(size_t) (i - 1)]));
        return m;
    }

    double blackmanHarris (int n, int len)
    {
        const double a = 2.0 * kPi * (double) n / (double) (len - 1);
        return 0.35875 - 0.48829 * std::cos (a) + 0.14128 * std::cos (2.0 * a) - 0.01168 * std::cos (3.0 * a);
    }

    // Blackman-Harris single-bin DFT at f, demodulated in ABSOLUTE time
    // (sample index from the start of x), so a component exactly at f has a
    // phase that does not advance between windows.
    std::complex<double> binPhasor (const std::vector<float>& x, int start, int len, double f, double fs)
    {
        std::complex<double> acc (0.0, 0.0);
        double wsum = 0.0;
        for (int n = 0; n < len; ++n)
        {
            const int idx = start + n;
            if (idx < 0 || idx >= (int) x.size())
                break;
            const double w = blackmanHarris (n, len);
            const double cyc = f * (double) idx / fs;
            const double ang = -2.0 * kPi * (cyc - std::floor (cyc));
            acc += w * (double) x[(size_t) idx] * std::complex<double> (std::cos (ang), std::sin (ang));
            wsum += w;
        }
        return wsum > 0.0 ? acc * (2.0 / wsum) : acc;
    }

    double binAmp (const std::vector<float>& x, int start, int len, double f, double fs)
    {
        return std::abs (binPhasor (x, start, len, f, fs));
    }

    // Frequency of the component near fExp: phase advance over 10 hops of
    // 50 ms (unambiguous within +/-10 Hz), W = 8192. Returns cents vs fExp.
    double measureCents (const std::vector<float>& x, double fs, double fExp, int startSample)
    {
        constexpr int kW = 8192, kHops = 10;
        const int hop = secs (0.05, fs);
        double total = 0.0;
        auto prev = binPhasor (x, startSample, kW, fExp, fs);
        for (int h = 1; h <= kHops; ++h)
        {
            const auto cur = binPhasor (x, startSample + h * hop, kW, fExp, fs);
            total += std::arg (cur * std::conj (prev));
            prev = cur;
        }
        const double dt = (double) (kHops * hop) / fs;
        const double df = total / (2.0 * kPi * dt);
        return 1200.0 * std::log2 ((fExp + df) / fExp);
    }

    //==========================================================================
    // QUAL-02 analyzer (RESEARCH 6.5): Kaiser beta 38, FFT 2^17, +/-14-bin mask
    // around every harmonic k*f0 < fs/2 and DC; metric = max inharmonic bin
    // relative to the max harmonic bin (or to an equal-RMS sine, D-B).
    struct Q2Analyzer
    {
        static constexpr int kOrder = 17;
        static constexpr int kSize  = 1 << kOrder;
        static constexpr int kGuard = 14;

        juce::dsp::FFT fft { kOrder };
        std::vector<float> window = std::vector<float> ((size_t) kSize, 0.0f);
        std::vector<float> work   = std::vector<float> ((size_t) (2 * kSize), 0.0f);
        std::vector<unsigned char> mask = std::vector<unsigned char> ((size_t) (kSize / 2 + 1), 0);

        Q2Analyzer()
        {
            juce::dsp::WindowingFunction<float>::fillWindowingTables (window.data(), (size_t) kSize,
                juce::dsp::WindowingFunction<float>::kaiser, false, 38.0f);
        }

        struct Result
        {
            double inhDb = 0.0;      // max inharmonic rel max harmonic
            double worstHz = 0.0;
            double maxHarm = 0.0;
            double maxInh = 0.0;
        };

        Result measure (const float* x, double f0, double fs)
        {
            for (int i = 0; i < kSize; ++i)
                work[(size_t) i] = x[i] * window[(size_t) i];
            std::fill (work.begin() + kSize, work.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform (work.data(), true);

            const int nb = kSize / 2;
            const double binHz = fs / (double) kSize;
            std::fill (mask.begin(), mask.end(), (unsigned char) 0);
            for (int b = 0; b <= kGuard && b <= nb; ++b)
                mask[(size_t) b] = 2;                       // DC: excluded from both
            for (int h = 1; (double) h * f0 < 0.5 * fs; ++h)
            {
                const int c = (int) std::lround ((double) h * f0 / binHz);
                for (int b = juce::jmax (0, c - kGuard); b <= juce::jmin (nb, c + kGuard); ++b)
                    if (mask[(size_t) b] == 0)
                        mask[(size_t) b] = 1;
            }

            Result r;
            int worstBin = 0;
            for (int b = 0; b <= nb; ++b)
            {
                const double m = (double) work[(size_t) b];
                if (mask[(size_t) b] == 1)
                    r.maxHarm = std::max (r.maxHarm, m);
                else if (mask[(size_t) b] == 0 && m > r.maxInh)
                {
                    r.maxInh = m;
                    worstBin = b;
                }
            }
            r.worstHz = (double) worstBin * binHz;
            r.inhDb = dB (r.maxInh / std::max (r.maxHarm, 1.0e-300));
            return r;
        }

        // Peak bin of a sine with the same RMS as x (D-B reference).
        double equalRmsSinePeak (const float* x, double f0, double fs)
        {
            double s = 0.0;
            for (int i = 0; i < kSize; ++i)
                s += (double) x[i] * (double) x[i];
            const double amp = std::sqrt (2.0 * s / (double) kSize);

            for (int i = 0; i < kSize; ++i)
            {
                const double cyc = f0 * (double) i / fs;
                work[(size_t) i] = (float) (amp * std::sin (2.0 * kPi * (cyc - std::floor (cyc)))) * window[(size_t) i];
            }
            std::fill (work.begin() + kSize, work.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform (work.data(), true);
            double peak = 0.0;
            for (int b = 0; b <= kSize / 2; ++b)
                peak = std::max (peak, (double) work[(size_t) b]);
            return peak;
        }
    };

    //==========================================================================
    // G-PITCH / G-BEND
    std::vector<float> renderNote (double fs, const Params& params, int note, double seconds,
                                   int wheel = 8192, int block = 4096)
    {
        Rig rig (fs, block, params);
        std::vector<Ev> evs;
        if (wheel != 8192)
            evs.push_back (evWheel (0, wheel));            // before the note: seeds startNote's wheel
        evs.push_back (evOn (0, note));
        return rig.run (secs (seconds, fs), evs);
    }

    void gatePitch()
    {
        double worst = 0.0;
        int worstNote = 0;
        double worstFs = 0.0;
        float worstPos = 0.0f;
        for (const double fs : { 44100.0, 48000.0 })
            for (const float pos : { 0.0f, 1.0f })
                for (int note = 21; note <= 108; ++note)
                {
                    const auto x = renderNote (fs, baseParams (BankFactory::sineSaw, pos), note, 1.0);
                    const double c = measureCents (x, fs, noteHz (note), secs (0.1, fs));
                    const double a = std::isfinite (c) ? std::abs (c) : 1.0e9;
                    if (a > worst)
                    {
                        worst = a;
                        worstNote = note;
                        worstFs = fs;
                        worstPos = pos;
                    }
                }

        // Negative control: +68 wheel steps = +1.66 cents must be detected.
        bool negOk = true;
        std::string negDetail;
        for (const int note : { 21, 69, 108 })
        {
            const auto x = renderNote (48000.0, baseParams (BankFactory::sineSaw, 0.0f), note, 1.0, 8192 + 68);
            const double c = measureCents (x, 48000.0, noteHz (note), secs (0.1, 48000.0));
            if (! (std::abs (c) > 1.0))
                negOk = false;
            negDetail += "MIDI " + std::to_string (note) + " " + fmt (c, 3) + " c; ";
        }

        report ("G-PITCH", worst <= 1.0 && negOk,
                "worst |cents| " + fmt (worst, 4) + " (MIDI " + std::to_string (worstNote) + ", "
                + fmt (worstFs, 0) + " Hz, pos " + fmt (worstPos, 0) + ") over MIDI 21..108 x {44.1k,48k} x pos {0,1}"
                + " (<= 1); neg control wheel+68 (expect ~+1.66 c, must exceed 1): " + negDetail);
    }

    void gateBend()
    {
        bool ok = true;
        std::string d;
        for (const int wheel : { 16383, 0 })
        {
            const double semis = ((double) wheel - 8192.0) / 8192.0 * 2.0;
            const double fExp = 440.0 * std::pow (2.0, semis / 12.0);
            const auto x = renderNote (48000.0, baseParams (BankFactory::sineSaw, 0.0f), 69, 1.0, wheel);
            const double c = measureCents (x, 48000.0, fExp, secs (0.1, 48000.0));
            if (! (std::abs (c) <= 1.0))
                ok = false;
            d += "wheel " + std::to_string (wheel) + ": expect " + fmt (fExp, 4) + " Hz (" + fmt (semis, 5)
               + " st), error " + fmt (c, 4) + " c; ";
        }
        report ("G-BEND", ok, d);
    }

    //==========================================================================
    // QUAL-02
    int q2Skip (double fs) { return secs (0.05, fs); }

    std::vector<float> q2Proc (double fs, int bank, float pos, int note, bool bandlimit = true)
    {
        // Interp Off: the latched frame is exactly round(pos * 31).
        auto p = with (with (baseParams (bank, pos), ids::interp, 0.0f), ids::bandlimit, bandlimit ? 1.0f : 0.0f);
        Rig rig (fs, 4096, p);
        const int skip = q2Skip (fs);
        auto x = rig.run (skip + Q2Analyzer::kSize, { evOn (0, note) });
        return std::vector<float> (x.begin() + skip, x.end());
    }

    std::vector<float> q2Voice (const WavetableBank& bank, double fs, int note)
    {
        VoiceRig vr (&bank, fs, 4096, true, true, 0, steadyAmp());
        vr.start (note, 1.0f);
        const int skip = q2Skip (fs);
        auto x = vr.run (skip + Q2Analyzer::kSize, [] (int) { return 0.0f; });
        return std::vector<float> (x.begin() + skip, x.end());
    }

    struct Worst
    {
        double db = -1000.0;
        int note = 0;
        double hz = 0.0;
        void take (double v, int n, double h)
        {
            if (v > db || ! std::isfinite (v)) { db = std::isfinite (v) ? v : 1000.0; note = n; hz = h; }
        }
    };

    void gateQual02 (Q2Analyzer& an, const WavetableBank& saw1023)
    {
        constexpr double kC8 = -100.0, kSweep = -70.0;

        // White-box: the selected level keeps every harmonic below Nyquist.
        int levelViolations = 0;
        for (const double fs : { 44100.0, 48000.0, 96000.0 })
            for (int note = 21; note <= 108; ++note)
            {
                const double f = noteHz (note);
                const int level = wt::selectLevel (f, fs, true);
                if (level < wt::kMinBandLimitedLevel || ! ((double) WavetableBank::kmax (level) * f < 0.5 * fs))
                    ++levelViolations;
            }
        report ("G-Q2-LEVEL", levelViolations == 0,
                "selectLevel: kmax(L) * f < fs/2 and L >= " + std::to_string (wt::kMinBandLimitedLevel)
                + " for MIDI 21..108 x 3 rates; violations = " + std::to_string (levelViolations));

        struct Arm { const char* name; int bank; float pos; bool voiceLevel; };
        const Arm arms[] = {
            { "Drive 32",              BankFactory::drive,      1.0f, false },
            { "Pulse frame 1",         BankFactory::pulseWidth, 0.0f, false },
            { "Saw-1023 (voice-level)", -1,                     0.0f, true  },
        };

        bool c8ok = true, sweepOk = true;
        double c8Worst = -1000.0, sweepWorst = -1000.0;
        for (const auto& arm : arms)
            for (const double fs : { 44100.0, 48000.0, 96000.0 })
            {
                Worst w;
                double c8 = 0.0, c8Hz = 0.0;
                for (int note = 21; note <= 108; ++note)
                {
                    const auto x = arm.voiceLevel ? q2Voice (saw1023, fs, note)
                                                  : q2Proc (fs, arm.bank, arm.pos, note);
                    const auto r = an.measure (x.data(), noteHz (note), fs);
                    w.take (r.inhDb, note, r.worstHz);
                    if (note == 108) { c8 = r.inhDb; c8Hz = r.worstHz; }
                }
                if (! (c8 <= kC8)) c8ok = false;
                if (! (w.db <= kSweep)) sweepOk = false;
                c8Worst = std::max (c8Worst, c8);
                sweepWorst = std::max (sweepWorst, w.db);
                info (std::string (arm.name) + " @ " + fmt (fs, 0) + " Hz: C8 " + fmt (c8, 1) + " dB (worst bin "
                      + fmt (c8Hz, 1) + " Hz, margin " + fmt (kC8 - c8, 1) + " dB); sweep worst " + fmt (w.db, 1)
                      + " dB at MIDI " + std::to_string (w.note) + " (" + fmt (w.hz, 1) + " Hz, margin "
                      + fmt (kSweep - w.db, 1) + " dB)");
            }

        report ("G-Q2-C8", c8ok, "worst C8 (MIDI 108) inharmonic " + fmt (c8Worst, 1)
                                 + " dB over Drive 32 / Pulse 1 / Saw-1023 x 44.1/48/96k (<= -100)");
        report ("G-Q2-SWEEP", sweepOk, "worst MIDI 21..108 inharmonic " + fmt (sweepWorst, 1)
                                       + " dB over the same frames and rates (<= -70; D-A floor L >= 1)");
    }

    void gatePulseException (Q2Analyzer& an)
    {
        std::cout << "NAMED EXCEPTION (D-B): narrow pulses judged on equal-RMS metric\n";

        std::vector<int> notes;
        for (int n = 21; n <= 33; ++n) notes.push_back (n);
        for (int n = 36; n <= 108; n += 3) notes.push_back (n);
        if (notes.back() != 108) notes.push_back (108);

        bool ok = true;
        double worstEq = -1000.0;
        for (const int k : { 16, 24, 32 })
        {
            const float pos = (float) (k - 1) / 31.0f;
            for (const double fs : { 44100.0, 48000.0, 96000.0 })
            {
                Worst eqW, harmW;
                for (const int note : notes)
                {
                    const auto x = q2Proc (fs, BankFactory::pulseWidth, pos, note);
                    const double f0 = noteHz (note);
                    const auto r = an.measure (x.data(), f0, fs);
                    const double ref = an.equalRmsSinePeak (x.data(), f0, fs);
                    const double eq = dB (r.maxInh / std::max (ref, 1.0e-300));
                    eqW.take (eq, note, r.worstHz);
                    harmW.take (r.inhDb, note, r.worstHz);
                }
                if (! (eqW.db <= -60.0)) ok = false;
                worstEq = std::max (worstEq, eqW.db);
                info ("Pulse " + std::to_string (k) + " @ " + fmt (fs, 0) + " Hz: equal-RMS worst " + fmt (eqW.db, 1)
                      + " dB at MIDI " + std::to_string (eqW.note) + " (margin " + fmt (-60.0 - eqW.db, 1)
                      + " dB); strongest-harmonic metric (record only) " + fmt (harmW.db, 1) + " dB at MIDI "
                      + std::to_string (harmW.note));
            }
        }
        report ("G-Q2-PULSE", ok, "NAMED EXCEPTION (D-B): narrow pulses judged on equal-RMS metric; worst "
                                  + fmt (worstEq, 1) + " dB rel equal-RMS sine (<= -60)");
    }

    void gateDsp02 (Q2Analyzer& an)
    {
        const double fs = 44100.0;
        const int note = 96;   // C7
        const auto off = q2Proc (fs, BankFactory::sineSaw, 1.0f, note, false);
        const auto on  = q2Proc (fs, BankFactory::sineSaw, 1.0f, note, true);
        const auto rOff = an.measure (off.data(), noteHz (note), fs);
        const auto rOn  = an.measure (on.data(), noteHz (note), fs);
        report ("G-DSP02", rOff.inhDb > -40.0 && rOn.inhDb <= -100.0,
                "Saw 32 @ C7 44.1k: band-limit Off " + fmt (rOff.inhDb, 1) + " dB at " + fmt (rOff.worstHz, 1)
                + " Hz (> -40, analyzer liveness); On " + fmt (rOn.inhDb, 1) + " dB (<= -100)");
    }

    //==========================================================================
    // G-DSP01: fs 56320, A4 -> inc = 2^-7 exactly, period 128 = block.
    std::vector<float> dsp01Render (bool interp)
    {
        const double fs = 56320.0;
        constexpr int kBlk = 128, kRamp = 2000, kHold = 40;
        auto p = with (baseParams (BankFactory::sineSaw, 0.0f), ids::interp, interp ? 1.0f : 0.0f);
        Rig rig (fs, kBlk, p);
        std::vector<float> all;
        for (int b = 0; b < kRamp + kHold; ++b)
        {
            const float pos = b < kRamp ? (float) b / (float) (kRamp - 1) : 1.0f;
            rig.set (ids::position, pos);
            std::vector<Ev> evs;
            if (b == 0)
                evs.push_back (evOn (0, 69));
            const auto x = rig.run (kBlk, evs);
            all.insert (all.end(), x.begin(), x.end());
        }
        return all;
    }

    void cycleSpectrum (const float* c, std::vector<double>& mag)
    {
        mag.assign (32, 0.0);
        for (int h = 1; h <= 32; ++h)
        {
            std::complex<double> acc (0.0, 0.0);
            for (int n = 0; n < 128; ++n)
            {
                const double ang = -2.0 * kPi * (double) (h * n) / 128.0;
                acc += (double) c[n] * std::complex<double> (std::cos (ang), std::sin (ang));
            }
            mag[(size_t) (h - 1)] = std::abs (acc) / 64.0;
        }
    }

    void dsp01Stats (const std::vector<float>& x, int& distinct, double& maxStepOut)
    {
        constexpr int kSkipCycles = 10;
        std::set<std::string> seen;
        std::vector<double> prev, cur;
        maxStepOut = 0.0;
        const int cycles = (int) x.size() / 128;
        for (int c = kSkipCycles; c < cycles; ++c)
        {
            const float* p = x.data() + (size_t) c * 128;
            seen.insert (std::string (reinterpret_cast<const char*> (p), 128 * sizeof (float)));
            cycleSpectrum (p, cur);
            if (! prev.empty())
            {
                double s = 0.0;
                for (size_t h = 0; h < cur.size(); ++h)
                    s += (cur[h] - prev[h]) * (cur[h] - prev[h]);
                maxStepOut = std::max (maxStepOut, std::sqrt (s));
            }
            prev = cur;
        }
        distinct = (int) seen.size();
    }

    void gateDsp01()
    {
        const auto off = dsp01Render (false);
        const auto on  = dsp01Render (true);
        int dOff = 0, dOn = 0;
        double sOff = 0.0, sOn = 0.0;
        dsp01Stats (off, dOff, sOff);
        dsp01Stats (on, dOn, sOn);
        const bool differ = off.size() != on.size()
                         || std::memcmp (off.data(), on.data(), off.size() * sizeof (float)) != 0;

        report ("G-DSP01", dOff == 32 && dOn >= 500 && sOn < sOff / 10.0 && differ,
                "Interp Off: " + std::to_string (dOff) + " distinct 128-sample cycles (want 32, so frames change only at"
                " cycle boundaries); On: " + std::to_string (dOn) + " distinct (>= 500); max spectral step On "
                + fmt (sOn, 5) + " vs Off " + fmt (sOff, 5) + " (On < Off/10); neg control Off != On: "
                + (differ ? "yes" : "NO"));
    }

    //==========================================================================
    void gateDsp03()
    {
        const double fs = 48000.0;
        auto run = [fs] (int bitIdx)
        {
            Rig rig (fs, 512, with (baseParams (BankFactory::sineSaw, 1.0f), ids::bitDepth, (float) bitIdx));
            const float outDb = rawOf (*rig.proc, ids::outputLevel);
            auto x = rig.run (secs (0.6, fs), { evOn (0, 69) });
            return std::make_pair (std::vector<float> (x.begin() + secs (0.1, fs), x.end()), outDb);
        };

        const auto q = run (14);   // "3"
        const auto full = run (0);

        std::set<float> vals (q.first.begin(), q.first.end());
        std::set<float> want;
        for (const float k : { 1.0f, 3.0f, 5.0f, 7.0f })
        {
            want.insert (k / 16.0f);
            want.insert (-k / 16.0f);
        }
        std::set<float> fullVals (full.first.begin(), full.first.end());

        std::string got;
        for (const float v : vals)
            got += fmt (v, 5) + " ";

        // The quantizer levels are exact; the output stage then applies the
        // host-normalised output_level (raw 0.000001 dB at default, i.e. a
        // gain of 1 + 1e-7), so match each level within 1e-6 after dividing
        // that gain out. The count must still be exactly 8.
        const float outGain = juce::Decibels::decibelsToGain (q.second, -60.0f);
        bool levelsMatch = vals.size() == want.size();
        if (levelsMatch)
        {
            auto w = want.begin();
            for (const float v : vals)
                levelsMatch = levelsMatch && std::abs (v / outGain - *w++) <= 1.0e-6f;
        }

        report ("G-DSP03", levelsMatch && fullVals.size() > 1000,
                "3 bits: " + std::to_string (vals.size()) + " distinct values {" + got + "} (want exactly +/-{1,3,5,7}/16; "
                "output_level raw " + fmt (q.second, 6) + " dB); neg control Full: " + std::to_string (fullVals.size())
                + " distinct (> 1000)");
    }

    void gateFull()
    {
        // (a) Quantizer bypass over 10^6 values, bitwise.
        BitQuantizer bq;
        bq.setChoiceIndex (0);
        juce::Random rng (0x5eed1234);
        int mismatches = 0;
        const float specials[] = { 0.0f, -0.0f, 1.3f, -1.3f, 1.0f, -1.0f,
                                   std::numeric_limits<float>::denorm_min(), -std::numeric_limits<float>::denorm_min(),
                                   std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
                                   std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
                                   std::numeric_limits<float>::quiet_NaN() };
        int count = 0;
        for (const float v : specials)
        {
            const float y = bq.apply (v);
            if (std::memcmp (&y, &v, sizeof (float)) != 0) ++mismatches;
            ++count;
        }
        for (; count < 1000000; ++count)
        {
            float v;
            if ((count & 1) == 0)
            {
                const auto bits = (std::uint32_t) rng.nextInt();
                std::memcpy (&v, &bits, sizeof (float));
            }
            else
            {
                v = (rng.nextFloat() * 2.6f) - 1.3f;
            }
            const float y = bq.apply (v);
            if (std::memcmp (&y, &v, sizeof (float)) != 0) ++mismatches;
        }

        // (b) Processor: 3 bits for 0.2 s then Full, vs a fresh Full render.
        const double fs = 48000.0;
        const int pre = secs (0.2, fs) / 512 * 512, post = secs (0.3, fs);
        Rig a (fs, 512, with (baseParams (BankFactory::sineSaw, 1.0f), ids::bitDepth, 14.0f));
        Rig b (fs, 512, baseParams (BankFactory::sineSaw, 1.0f));
        const std::vector<Ev> evs { evOn (0, 57) };
        a.run (pre, evs);
        b.run (pre, evs);
        a.set (ids::bitDepth, 0.0f);
        const auto ya = a.run (post, evs);
        const auto yb = b.run (post, evs);
        const bool same = ya.size() == yb.size()
                       && std::memcmp (ya.data(), yb.data(), ya.size() * sizeof (float)) == 0;

        report ("G-FULL", mismatches == 0 && same,
                "Full bypass: " + std::to_string (mismatches) + " bitwise mismatches over " + std::to_string (count)
                + " values (incl. +/-0, subnormals, +/-1.3, inf, NaN); processor 3->Full render == fresh Full: "
                + (same ? "yes" : "NO"));
    }

    //==========================================================================
    // G-READ: voice output == the reference read (viz seam), within 1 ulp.
    void gateRead (const BuiltInBanks& banks)
    {
        const double fs = 48000.0;
        const int total = secs (0.5, fs), cmpFrom = 200, note = 69;
        const WavetableBank& bank = *banks.get (BankFactory::sineSaw);
        auto knobAt = [total] (int i) { return (float) i / (float) total; };   // ramp 0 -> 1

        bool ok = true;
        std::string d;
        for (const bool interp : { true, false })
        {
            VoiceRig vr (&bank, fs, 256, interp, true, 14, steadyAmp());
            vr.start (note, 1.0f);
            const auto y = vr.run (total, knobAt);

            BitQuantizer q;
            q.setChoiceIndex (14);
            const double hz = juce::MidiMessage::getMidiNoteInHertz (note) * std::pow (2.0, 0.0);
            const double r = hz / fs;
            const double inc = r < 0.49 ? r : 0.49;
            const int level = wt::selectLevel (hz, fs, true);
            double phase = 0.0;
            int latched = 0;
            bool needsLatch = true;
            int bad = 0;
            double worstUlp = 0.0;
            // Stage 2.3: with Interp On the voice reads at the 2 ms smoothed
            // position (seeded at the first sample); with Off it reads raw.
            PositionSmoother sm;
            sm.prepare (fs);
            bool seeded = false;
            for (int i = 0; i < total; ++i)
            {
                const float raw = wt::clamp01 (knobAt (i) + 0.0f * 0.5f * 0.0f);
                if (! seeded) { sm.seed (raw); seeded = true; }
                const float pos = interp ? wt::clamp01 (sm.process (raw)) : raw;
                if (needsLatch) { latched = wt::latchFrame (raw, bank.numFrames); needsLatch = false; }
                const float e = q.apply (wt::readSample (bank, level, phase, interp, pos, latched)) * WtVoice::kVoiceGain;
                if (i >= cmpFrom)
                {
                    const float yi = y[(size_t) i];
                    const float ulp = std::nextafter (std::abs (e), std::numeric_limits<float>::infinity()) - std::abs (e);
                    const double diffUlp = ulp > 0.0f ? std::abs ((double) yi - (double) e) / (double) ulp : 0.0;
                    worstUlp = std::max (worstUlp, diffUlp);
                    if (diffUlp > 1.0)
                        ++bad;
                }
                phase += inc;
                if (phase >= 1.0)
                {
                    phase -= 1.0;
                    if (! interp)
                        latched = wt::latchFrame (raw, bank.numFrames);
                }
            }
            if (bad != 0)
                ok = false;
            d += std::string (interp ? "Interp On" : "Interp Off") + ": " + std::to_string (bad)
               + " samples > 1 ulp (worst " + fmt (worstUlp, 2) + " ulp); ";
        }
        report ("G-READ", ok, d + "bit depth 3, Sine->Saw, knob ramp 0->1, A4 48k");
    }

    //==========================================================================
    void gatePoly()
    {
        const double fs = 48000.0;
        Rig rig (fs, 512, baseParams (BankFactory::sineSaw, 0.0f));
        std::vector<Ev> evs;
        for (int i = 0; i < 16; ++i)
            evs.push_back (evOn (i * 64, 48 + i));
        const auto x1 = rig.run (secs (1.2, fs), evs);
        const int sounding1 = rig.proc->getSoundingVoiceCountForTesting();

        auto amps = [fs] (const std::vector<float>& x, const std::vector<int>& notes)
        {
            std::vector<double> a;
            for (const int n : notes)
                a.push_back (binAmp (x, secs (0.3, fs), secs (0.8, fs), noteHz (n), fs));
            return a;
        };

        std::vector<int> sixteen;
        for (int i = 0; i < 16; ++i) sixteen.push_back (48 + i);
        const auto a1 = amps (x1, sixteen);
        const double max1 = *std::max_element (a1.begin(), a1.end());
        double weakest = 0.0;
        for (const double v : a1) weakest = std::min (weakest, dB (v / max1));

        // 17th note: JUCE protects the lowest (48) and highest (63) held notes,
        // so the oldest UNPROTECTED voice (49) is stolen.
        const auto x2 = rig.run (secs (1.2, fs), { evOn (rig.cursor, 64) });
        const int sounding2 = rig.proc->getSoundingVoiceCountForTesting();
        const auto a2 = amps (x2, { 64, 49, 48 });
        const double max2 = *std::max_element (a1.begin(), a1.end());
        const double n64 = dB (a2[0] / max2), n49 = dB (a2[1] / max2), n48 = dB (a2[2] / max2);

        report ("G-POLY", sounding1 == 16 && weakest > -30.0 && sounding2 == 16 && n64 > -30.0 && n49 < -40.0 && n48 > -30.0,
                "16 notes: sounding " + std::to_string (sounding1) + ", weakest f0 " + fmt (weakest, 2)
                + " dB rel strongest (> -30); after 17th (MIDI 64): sounding " + std::to_string (sounding2)
                + ", MIDI 64 " + fmt (n64, 1) + " dB (> -30), stolen MIDI 49 " + fmt (n49, 1)
                + " dB (< -40), protected lowest MIDI 48 " + fmt (n48, 1) + " dB (> -30)");
    }

    //==========================================================================
    Params monoTestParams (bool mono)
    {
        auto p = baseParams (BankFactory::sineSaw, 0.0f);
        p = with (p, ids::voiceMode, mono ? 1.0f : 0.0f);
        p = with (p, ids::ampAttack, 0.005f);
        p = with (p, ids::ampDecay, 0.15f);
        p = with (p, ids::ampSustain, 0.4f);
        p = with (p, ids::ampRelease, 0.2f);
        return p;
    }

    void gateMono()
    {
        const double fs = 48000.0;
        const int c4 = 60, e4 = 64;
        const std::vector<Ev> evs { evOn (0, c4), evOn (secs (0.2, fs), e4), evOff (secs (0.6, fs), e4),
                                    evOff (secs (1.0, fs), c4) };

        Rig mono (fs, 512, monoTestParams (true));
        const auto x = mono.run (secs (1.6, fs), evs);

        const int w1a = secs (0.25, fs), w1n = secs (0.3, fs);
        const double e4a = binAmp (x, w1a, w1n, noteHz (e4), fs), c4a = binAmp (x, w1a, w1n, noteHz (c4), fs);
        const int w2a = secs (0.65, fs);
        const double c4b = binAmp (x, w2a, w1n, noteHz (c4), fs), e4b = binAmp (x, w2a, w1n, noteHz (e4), fs);

        const int t1 = secs (0.2, fs), t2 = secs (0.6, fs), w20 = secs (0.02, fs);
        const double ra1 = rms (x, t1, t1 + w20) / std::max (rms (x, t1 - w20, t1), 1.0e-12);
        const double ra2 = rms (x, t2, t2 + w20) / std::max (rms (x, t2 - w20, t2), 1.0e-12);
        const double releasing = rms (x, secs (1.0, fs), secs (1.02, fs));
        const double tail = rms (x, secs (1.3, fs), secs (1.6, fs));

        Rig poly (fs, 512, monoTestParams (false));
        const auto xp = poly.run (secs (1.6, fs), evs);
        const double pe4 = binAmp (xp, w1a, w1n, noteHz (e4), fs), pc4 = binAmp (xp, w1a, w1n, noteHz (c4), fs);

        const bool legato = e4a >= 4.0 * c4a && c4b >= 4.0 * e4b;
        const bool noReattack = ra1 >= 0.9 && ra1 <= 1.1 && ra2 >= 0.9 && ra2 <= 1.1;
        const bool tailOk = releasing > 1.0e-3 && tail < 1.0e-6;
        const bool negOk = pc4 > 0.25 * pe4;

        report ("G-MONO", legato && noReattack && tailOk && negOk,
                "Mono 0.25-0.55 s E4/C4 = " + fmt (e4a / std::max (c4a, 1.0e-12), 1) + " (>= 4); 0.65-0.95 s C4/E4 = "
                + fmt (c4b / std::max (e4b, 1.0e-12), 1) + " (>= 4); RMS after/before E4-on " + fmt (ra1, 3)
                + ", after/before E4-off " + fmt (ra2, 3) + " (in [0.9, 1.1]: no re-attack); release RMS "
                + fmt (releasing, 4) + ", tail RMS " + fmt (tail, 8) + " (< 1e-6); neg control Poly C4/E4 = "
                + fmt (pc4 / std::max (pe4, 1.0e-12), 2) + " (> 0.25: both notes)");
    }

    void gateRetrig()
    {
        const double fs = 48000.0;
        auto p = with (with (with (with (with (baseParams (BankFactory::sineSaw, 0.0f), ids::voiceMode, 1.0f),
                       ids::ampAttack, 0.005f), ids::ampDecay, 0.3f), ids::ampSustain, 1.0f), ids::ampRelease, 0.3f);
        Rig rig (fs, 512, p);
        const int tOn2 = secs (0.6, fs);
        const auto x = rig.run (secs (0.8, fs), { evOn (0, 48), evOff (secs (0.5, fs), 48), evOn (tOn2, 48) });

        const double steady = maxStep (x, secs (0.3, fs), secs (0.5, fs));
        const double around = maxStep (x, tOn2 - 32, tOn2 + secs (0.05, fs));
        const double tailLevel = peakAbs (x, tOn2 - secs (0.01, fs), tOn2);
        report ("G-RETRIG", around <= 1.5 * steady && tailLevel > 0.05,
                "max |step| around the retrigger " + fmt (around, 5) + " vs steady " + fmt (steady, 5)
                + " (<= 1.5x; a reset-to-0 would jump by ~" + fmt (tailLevel, 3) + ")");
    }

    //==========================================================================
    // Gap closure W1 / W2 (QUAL-01). Every stimulus lands at a phase peak of
    // the voice that is cut or re-scaled (phase starts at 0 at note-on), so a
    // hard step is as large as it can be. Ratio = max |step| in the 50 ms
    // after the event over max |step| at steady state; fix <= 1.5x, the
    // negative control (fade / ramp disabled through the test hook) >= 4x.
    int peakSampleNear (double t, int noteOnPos, int note, double fs)
    {
        const double f = noteHz (note);
        const double k = std::floor ((t * fs - (double) noteOnPos) * f / fs);
        return noteOnPos + (int) std::lround ((k + 0.25) * fs / f);
    }

    void gateSteal()
    {
        const double fs = 48000.0;
        auto run = [fs] (bool fade, double& steady, double& around)
        {
            Rig rig (fs, 512, baseParams (BankFactory::sineSaw, 0.0f));
            rig.proc->setTailFadeForTesting (fade);
            std::vector<Ev> evs;
            for (int i = 0; i < 16; ++i)
                evs.push_back (evOn (i * 64, 48 + i));
            // 17th note: the oldest unprotected voice (MIDI 49, on at 64) is stolen (G-POLY).
            const int tSteal = peakSampleNear (0.5, 64, 49, fs);
            evs.push_back (evOn (tSteal, 64));
            const auto x = rig.run (secs (0.7, fs), evs);
            steady = maxStep (x, secs (0.3, fs), tSteal - 64);
            around = maxStep (x, tSteal - 32, tSteal + secs (0.05, fs));
        };

        double s1, a1, s0, a0;
        run (true, s1, a1);
        run (false, s0, a0);
        const double r1 = a1 / std::max (s1, 1.0e-12), r0 = a0 / std::max (s0, 1.0e-12);
        report ("G-STEAL", r1 <= 1.5 && r0 >= 4.0,
                "16 held + 17th at the stolen voice's phase peak: max |step| / steady = " + fmt (r1, 3)
                + " (<= 1.5); neg control (no tail fade) " + fmt (r0, 2) + " (>= 4)");
    }

    void gateSwitchTail()
    {
        const double fs = 48000.0;
        auto run = [fs] (bool fade, double& steady, double& around)
        {
            Rig rig (fs, 512, baseParams (BankFactory::sineSaw, 0.0f));
            rig.proc->setTailFadeForTesting (fade);
            const std::vector<Ev> evs { evOn (0, 48), evOn (0, 55), evOn (0, 60), evOn (0, 64) };
            auto x = rig.run (512 * 47, evs);          // the switch lands on a block boundary
            rig.set (ids::voiceMode, 1.0f);            // Poly -> Mono: every voice hard-stops
            const int tSw = (int) x.size();
            const auto y = rig.run (secs (0.1, fs));
            x.insert (x.end(), y.begin(), y.end());
            steady = maxStep (x, secs (0.2, fs), tSw - 64);
            around = maxStep (x, tSw - 32, tSw + secs (0.05, fs));
        };

        double s1, a1, s0, a0;
        run (true, s1, a1);
        run (false, s0, a0);
        const double r1 = a1 / std::max (s1, 1.0e-12), r0 = a0 / std::max (s0, 1.0e-12);
        report ("G-SWITCH-TAIL", r1 <= 1.5 && r0 >= 4.0,
                "4 held, Poly -> Mono: max |step| / steady = " + fmt (r1, 3) + " (<= 1.5); neg control (no tail fade) "
                + fmt (r0, 2) + " (>= 4)");
    }

    void gateRetrigVel()
    {
        const double fs = 48000.0;
        auto run = [fs] (bool ramp, double& steady, double& around, double& level)
        {
            auto p = with (with (with (with (with (baseParams (BankFactory::sineSaw, 0.0f), ids::voiceMode, 1.0f),
                           ids::ampAttack, 0.005f), ids::ampDecay, 0.3f), ids::ampSustain, 1.0f), ids::ampRelease, 0.3f);
            Rig rig (fs, 512, p);
            rig.proc->setVelRampForTesting (ramp);
            // Mono keeps the phase on a sounding retrigger, so the peak is computed from note-on.
            const int tOn2 = peakSampleNear (0.6, 0, 48, fs);
            const auto x = rig.run (secs (0.8, fs), { evOn (0, 48, 127), evOff (secs (0.5, fs), 48), evOn (tOn2, 48, 38) });
            steady = maxStep (x, secs (0.3, fs), secs (0.5, fs));
            around = maxStep (x, tOn2 - 32, tOn2 + secs (0.05, fs));
            level  = rms (x, secs (0.7, fs), secs (0.8, fs));
        };

        double s1, a1, l1, s0, a0, l0;
        run (true, s1, a1, l1);
        run (false, s0, a0, l0);
        const double r1 = a1 / std::max (s1, 1.0e-12), r0 = a0 / std::max (s0, 1.0e-12);
        // The ramp only changes the first 3 ms: the settled vel-38 level must match the instant path.
        const double settled = std::abs (dB (l1 / std::max (l0, 1.0e-12)));
        report ("G-RETRIG-VEL", r1 <= 1.5 && r0 >= 4.0 && settled <= 0.01,
                "Mono vel 127 -> off -> vel 38 at a phase peak: max |step| / steady = " + fmt (r1, 3)
                + " (<= 1.5); neg control (instant velGain) " + fmt (r0, 2) + " (>= 4); settled level vs instant "
                + fmt (settled, 4) + " dB (<= 0.01)");
    }

    void gateNoteOff()
    {
        const double fs = 48000.0;
        auto p = with (with (with (with (baseParams (BankFactory::sineSaw, 0.0f), ids::ampAttack, 0.005f),
                       ids::ampDecay, 3.0f), ids::ampSustain, 0.0f), ids::ampRelease, 0.3f);
        Rig rig (fs, 512, p);
        const int tOff = secs (0.85, fs);
        const auto x = rig.run (secs (1.0, fs), { evOn (0, 69), evOff (tOff, 69) });
        const double pre = rms (x, tOff - secs (0.05, fs), tOff);
        const double post = rms (x, tOff + secs (0.01, fs), tOff + secs (0.06, fs));

        // Negative control (documented, not shipped): a raw ADSR whose params
        // are re-pushed every block collapses the sustain-0 release tail.
        juce::ADSR env;
        env.setSampleRate (fs);
        const juce::ADSR::Parameters ap { 0.005f, 3.0f, 0.0f, 0.3f };
        env.setParameters (ap);
        env.noteOn();
        std::vector<float> e;
        for (int i = 0; i < secs (1.0, fs); ++i)
        {
            if (i == tOff) env.noteOff();
            if (i % 512 == 0) env.setParameters (ap);   // the bug
            e.push_back (env.getNextSample());
        }
        const double ePre = rms (e, tOff - secs (0.05, fs), tOff);
        const double ePost = rms (e, tOff + secs (0.01, fs), tOff + secs (0.06, fs));

        report ("G-NOTEOFF", post > 0.2 * pre && ePost <= 0.2 * ePre,
                "tail RMS / pre RMS = " + fmt (post / std::max (pre, 1.0e-12), 3) + " (> 0.2); neg control "
                "(per-block setParameters on a raw ADSR) = " + fmt (ePost / std::max (ePre, 1.0e-12), 3) + " (<= 0.2)");
    }

    void gateVel()
    {
        const double fs = 48000.0;
        auto level = [fs] (int vel)
        {
            Rig rig (fs, 512, baseParams (BankFactory::sineSaw, 0.0f));
            const auto x = rig.run (secs (0.5, fs), { evOn (0, 69, vel) });
            return rms (x, secs (0.1, fs), secs (0.5, fs));
        };
        const double r = dB (level (64) / level (127));
        const double want = 20.0 * std::log10 (std::pow (64.0 / 127.0, 2.0));
        report ("G-VEL", std::abs (r - (-11.90)) <= 0.05,
                "vel 64 vs 127 = " + fmt (r, 3) + " dB (want -11.90 +/- 0.05; exact (64/127)^2 = " + fmt (want, 3) + " dB)");
    }

    void gateOut()
    {
        const double fs = 48000.0;
        const std::vector<Ev> evs { evOn (0, 69) };

        Rig silent (fs, 512, with (baseParams (BankFactory::sineSaw, 0.0f), ids::outputLevel, -60.0f));
        const auto xs = silent.run (secs (0.3, fs), evs);
        long long nonZero = 0;
        for (const float v : xs)
            if (! juce::exactlyEqual (v, 0.0f))
                ++nonZero;

        Rig loud (fs, 512, baseParams (BankFactory::sineSaw, 0.0f));
        const auto xl = loud.run (secs (0.05, fs), evs);
        const double seeded = peakAbs (xl, secs (0.009, fs), secs (0.011, fs) + 1);

        // Negative control: prepared at -60 dB, raised to 0 dB before the first
        // block -> a 20 ms ramp from silence (the unseeded behaviour).
        Rig ramp (fs, 512, with (baseParams (BankFactory::sineSaw, 0.0f), ids::outputLevel, -60.0f));
        ramp.set (ids::outputLevel, 0.0f);
        const auto xr = ramp.run (secs (0.05, fs), evs);
        const double ramped = peakAbs (xr, secs (0.009, fs), secs (0.011, fs) + 1);

        report ("G-OUT", nonZero == 0 && seeded >= 0.45 && seeded <= 0.55 && ramped < 0.4,
                "-60 dB: " + std::to_string (nonZero) + " samples not exactly 0.0f of " + std::to_string (xs.size())
                + "; 0 dB peak at 10 ms " + fmt (seeded, 4) + " (~0.5, seeded); neg control ramp-from-silence "
                + fmt (ramped, 4) + " (< 0.4)");
    }

    void gateImportedEmpty()
    {
        const double fs = 48000.0;
        Rig rig (fs, 512, baseParams (5, 0.0f));
        const auto x1 = rig.run (secs (0.2, fs), { evOn (0, 69) });
        const int sounding = rig.proc->getSoundingVoiceCountForTesting();
        long long nonZero = 0;
        for (const float v : x1)
            if (! juce::exactlyEqual (v, 0.0f))
                ++nonZero;
        rig.set (ids::bank, 0.0f);
        const auto x2 = rig.run (secs (0.2, fs));
        const double after = peakAbs (x2, 0, (int) x2.size());
        report ("G-IMPEMPTY", nonZero == 0 && sounding == 1 && after > 0.1,
                "bank Imported (empty): " + std::to_string (nonZero) + " non-zero samples, env running in "
                + std::to_string (sounding) + " voice(s); after switching to Sine->Saw mid-note peak " + fmt (after, 3) + " (> 0.1)");
    }

    void gateBlock()
    {
        const double fs = 48000.0;
        bool ok = true;
        std::string d;
        for (const bool mono : { false, true })
        {
            const auto p = with (baseParams (BankFactory::sineSaw, 0.6f), ids::voiceMode, mono ? 1.0f : 0.0f);
            const std::vector<Ev> evs { evOn (100, 60), evOn (300, 64), evWheel (900, 12000), evOn (700, 67),
                                        evOff (1500, 64), evWheel (2100, 4000), evOn (3333, 72), evOff (5000, 60),
                                        evOff (6000, 67), evOff (7000, 72) };
            Rig big (fs, 256, p), small (fs, 256, p);
            const auto a = big.run (8 * 1024, evs, 1024);     // oversized host blocks
            const auto b = small.run (8 * 1024, evs);         // 256-sample blocks
            const bool same = a.size() == b.size() && std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0;
            const double pk = peakAbs (a, 0, (int) a.size());
            if (! same || pk < 0.1)
                ok = false;
            d += std::string (mono ? "Mono" : "Poly") + ": 1024 vs 4x256 bit-identical " + (same ? "yes" : "NO")
               + " (peak " + fmt (pk, 3) + "); ";
        }
        report ("G-BLOCK", ok, d);
    }

    //==========================================================================
    //==========================================================================
    // Stage 4 (4-polish PLAN Tasks 2-3): Stage 2 notes 3 / 4, W3, W4. Every
    // gate has a negative control through its OSIW_TEST_HOOKS hook.

    constexpr int kFloodEvents  = 6000;    // one host block, all at one sample
    constexpr int kFloodReserve = 49160;   // chunkMidi.ensureSize (32768) reserves (n + n/2 + 8) & ~7 bytes

    // Frequency from rising zero crossings over [a, b) (linear interpolation).
    double zeroCrossHz (const std::vector<float>& x, int a, int b, double fs)
    {
        std::vector<double> crossings;
        const int end = juce::jmin (b, (int) x.size());
        for (int i = juce::jmax (a, 0); i + 1 < end; ++i)
        {
            const double x0 = (double) x[(size_t) i];
            const double x1 = (double) x[(size_t) (i + 1)];
            if (x0 < 0.0 && x1 >= 0.0)
                crossings.push_back ((double) i + x0 / (x0 - x1));
        }
        if (crossings.size() < 3)
            return 0.0;
        return (double) (crossings.size() - 1) / ((crossings.back() - crossings.front()) / fs);
    }

    double centsVs (double f, double fExp)
    {
        return (f > 0.0 && fExp > 0.0) ? 1200.0 * std::log2 (f / fExp) : 1.0e9;
    }

    //==========================================================================
    // G-UNPREPARED (note 4). The child runs in a SELF-EXEC process
    // (posix_spawn of this executable with --child-unprepared), never a fork:
    // a fork after JUCE's timer / pool threads start can deadlock the child on
    // an inherited lock. Exit 0 = pass; 3 / 4 / 5 = arm a / b / c failed.
   #if JUCE_MAC
    int runUnpreparedChild (bool guardOff)
    {
        auto p = std::make_unique<Proc>();
        p->setPreparedGuardForTesting (! guardOff);
        p->setPlayConfigDetails (0, 2, 48000.0, 512);

        juce::AudioBuffer<float> b (2, 512);
        juce::MidiBuffer m;
        auto renderOne = [&b, &m, &p] (int note)
        {
            for (int ch = 0; ch < 2; ++ch)
                juce::FloatVectorOperations::fill (b.getWritePointer (ch), 1.0f, 512);   // processBlock must clear
            m.clear();
            if (note >= 0)
                m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            p->processBlock (b, m);
            float mx = 0.0f;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float* d = b.getReadPointer (ch);
                for (int i = 0; i < 512; ++i)
                    mx = std::max (mx, std::isfinite (d[i]) ? std::abs (d[i]) : 1.0e9f);
            }
            return mx;
        };

        // (a) never prepared: two blocks with a note-on -> exactly 0.
        const float a1 = renderOne (60);
        const float a2 = renderOne (60);
        if (! (juce::exactlyEqual (a1, 0.0f) && juce::exactlyEqual (a2, 0.0f)))
            return 3;

        // (b) prepareToPlay -> note-on -> sound.
        p->prepareToPlay (48000.0, 512);
        const float b1 = renderOne (60);
        const float b2 = renderOne (-1);
        if (! (std::max (b1, b2) > 0.01f))
            return 4;

        // (c) releaseResources -> note-on -> sound (prepared is NOT cleared, D-AJ(1)).
        p->releaseResources();
        const float c1 = renderOne (64);
        const float c2 = renderOne (-1);
        if (! (std::max (c1, c2) > 0.01f))
            return 5;

        return 0;
    }

    struct ChildResult
    {
        bool spawned = false;
        bool signalled = false;
        int  sig = 0;
        int  exitCode = -1;
        std::string why;
    };

    std::string describeChild (const ChildResult& r)
    {
        if (! r.spawned)
            return "NOT RUN (" + r.why + ")";
        if (r.signalled)
            return "killed by signal " + std::to_string (r.sig);
        return "exit " + std::to_string (r.exitCode);
    }

    ChildResult spawnUnpreparedChild (const std::string& self, bool guardOff)
    {
        ChildResult r;
        std::vector<std::string> args { self, "--child-unprepared" };
        if (guardOff)
            args.emplace_back ("--guard-off");
        std::vector<char*> cargv;
        for (auto& s : args)
            cargv.push_back (s.data());
        cargv.push_back (nullptr);

        posix_spawn_file_actions_t fa;
        posix_spawn_file_actions_init (&fa);
        if (guardOff)
        {
            // The negative control dies mid-render: keep its output (and any
            // assertion text on the way down) out of this log.
            posix_spawn_file_actions_addopen (&fa, 1, "/dev/null", O_WRONLY, 0);
            posix_spawn_file_actions_addopen (&fa, 2, "/dev/null", O_WRONLY, 0);
        }

        std::cout.flush();
        std::fflush (stdout);
        std::fflush (stderr);

        pid_t pid = 0;
        const int rc = posix_spawn (&pid, self.c_str(), &fa, nullptr, cargv.data(), environ);
        posix_spawn_file_actions_destroy (&fa);
        if (rc != 0)
        {
            r.why = "posix_spawn error " + std::to_string (rc);
            return r;
        }

        int status = 0;
        pid_t waited = -1;
        do
        {
            waited = waitpid (pid, &status, 0);
        } while (waited < 0 && errno == EINTR);
        if (waited != pid)
        {
            r.why = "waitpid failed";
            return r;
        }

        r.spawned = true;
        if (WIFSIGNALED (status))
        {
            r.signalled = true;
            r.sig = WTERMSIG (status);
        }
        else if (WIFEXITED (status))
        {
            r.exitCode = WEXITSTATUS (status);
        }
        return r;
    }

    void gateUnprepared (const std::string& self)
    {
        const auto fixed = spawnUnpreparedChild (self, false);
        report ("G-UNPREPARED", fixed.spawned && ! fixed.signalled && fixed.exitCode == 0,
                "self-exec child: (a) 2 x processBlock before prepareToPlay on a 1.0-filled buffer + note-on -> exactly 0; "
                "(b) prepareToPlay -> note-on -> peak > 0.01; (c) releaseResources -> note-on -> peak > 0.01: child "
                + describeChild (fixed) + " (want exit 0; 3 = a, 4 = b, 5 = c failed)");

        const auto nc = spawnUnpreparedChild (self, true);
        if (nc.spawned && nc.signalled)
            report ("G-UNPREPARED-NC", true,
                    "guard off (setPreparedGuardForTesting (false)): child killed by signal " + std::to_string (nc.sig)
                    + " - unprepared render FAILS as designed");
        else
            report ("G-UNPREPARED-NC", false,
                    "guard off: child " + describeChild (nc) + " - not killed by a signal; the gate is vacuous");
    }
   #endif

    //==========================================================================
    // G-MIDI-FLOOD (note 3): one 512-sample host block with a held note, a
    // 6000-event flood at one sample (Poly: CC 1; Mono: pitch wheel, which
    // also fills wheelMidi), then a wheel +2 st after the flood. The wheel
    // and a later note-off must both still land.
    std::vector<Ev> floodEvents (int b0, bool mono)
    {
        std::vector<Ev> evs;
        evs.reserve ((size_t) kFloodEvents + 2);
        evs.push_back (evOn (b0, 69));
        for (int i = 0; i < kFloodEvents; ++i)
            evs.push_back (mono ? evWheel (b0 + 100, 8192 + (i % 2))
                                : Ev { b0 + 100, juce::MidiMessage::controllerEvent (1, 1, i & 127) });
        evs.push_back (evWheel (b0 + 101, 16383));
        return evs;
    }

    void gateMidiFlood()
    {
        const double fs = 48000.0;
        const double want = 440.0 * std::pow (2.0, 2.0 / 12.0);
        const bool exceeds = kFloodEvents * 9 > kFloodReserve;   // the flood must overflow the reserve
        const std::string reserve = std::to_string (kFloodEvents) + " x 9 B = " + std::to_string (kFloodEvents * 9)
                                  + " B > reserve " + std::to_string (kFloodReserve) + " B: " + (exceeds ? "yes" : "NO");

        for (const bool mono : { false, true })
        {
            Rig rig (fs, 512, with (baseParams (BankFactory::sineSaw, 0.0f), ids::voiceMode, mono ? 1.0f : 0.0f));
            rig.run (512);
            const int b0 = rig.cursor;
            rig.run (512, floodEvents (b0, mono));
            const int tOff = rig.cursor + secs (0.3, fs);
            const auto y = rig.run (secs (0.5, fs), { evOff (tOff, 69) });
            const double hz = zeroCrossHz (y, secs (0.05, fs), secs (0.28, fs), fs);
            const double c = centsVs (hz, want);
            const double tail = rms (y, secs (0.45, fs), secs (0.5, fs));
            const std::string gate = std::string ("G-MIDI-FLOOD[") + (mono ? "Mono" : "Poly") + "]";
            report (gate.c_str(), exceeds && std::abs (c) <= 1.0 && tail < 1.0e-6,
                    std::to_string (kFloodEvents) + (mono ? " wheel" : " CC") + " events at one sample + wheel 16383 after: "
                    "pitch " + fmt (hz, 3) + " Hz vs " + fmt (want, 3) + " (+2 st) = " + fmt (c, 3)
                    + " c (|c| <= 1); tail RMS after note-off " + sciStr (tail) + " (< 1e-6); " + reserve);
        }
    }

    //==========================================================================
    // G-MONO-WHEEL (Stage 2 W3): Poly wheel history -> Poly -> Mono on a block
    // boundary -> Mono A4 at +64 samples, f by zero crossings over 0.2-0.9 s.
    double monoWheelCents (const std::vector<Ev>& history, double expectHz, bool seed)
    {
        const double fs = 48000.0;
        Rig rig (fs, 512, baseParams (BankFactory::sineSaw, 0.0f));
        rig.proc->setMonoWheelSeedForTesting (seed);
        rig.run (512 * 56, history);                 // ~0.6 s of Poly history
        rig.set (ids::voiceMode, 1.0f);              // the switch lands on a block boundary
        const int t0 = rig.cursor;
        const auto y = rig.run (secs (1.0, fs), { evOn (t0 + 64, 69) });
        return centsVs (zeroCrossHz (y, secs (0.2, fs), secs (0.9, fs), fs), expectHz);
    }

    void gateMonoWheel()
    {
        const double fs = 48000.0;
        const std::vector<Ev> armA { evOn (0, 69), evWheel (secs (0.1, fs), 16383), evOff (secs (0.15, fs), 69),
                                     evWheel (secs (0.5, fs), 8192) };   // up, note off, back to centre while idle
        const std::vector<Ev> armB { evWheel (secs (0.1, fs), 4096) };  // -1 st while idle
        const std::vector<Ev> armC {};                                  // control: no wheel
        const double hzA = 440.0, hzB = 440.0 * std::pow (2.0, -1.0 / 12.0), hzC = 440.0;

        const double a = monoWheelCents (armA, hzA, true);
        const double b = monoWheelCents (armB, hzB, true);
        const double c = monoWheelCents (armC, hzC, true);
        report ("G-MONO-WHEEL", std::abs (a) <= 1.0 && std::abs (b) <= 1.0 && std::abs (c) <= 1.0,
                "Poly history -> Mono A4: [A] wheel up, off, centre while idle " + fmt (a, 2) + " c; [B] -1 st while idle "
                + fmt (b, 2) + " c; [C] no wheel " + fmt (c, 2) + " c (|c| <= 1 each)");

        const double na = monoWheelCents (armA, hzA, false);
        const double nb = monoWheelCents (armB, hzB, false);
        if (na >= 150.0 && nb >= 75.0)
            report ("G-MONO-WHEEL-NC", true,
                    "seed off (setMonoWheelSeedForTesting (false)): [A] " + fmt (na, 2) + " c (>= +150), [B] " + fmt (nb, 2)
                    + " c (>= +75) - stale Mono wheel FAILS as designed");
        else
            report ("G-MONO-WHEEL-NC", false,
                    "seed off: [A] " + fmt (na, 2) + " c (want >= +150), [B] " + fmt (nb, 2)
                    + " c (want >= +75) - the stale wheel was not reproduced; the gate is vacuous");
    }

    //==========================================================================
    // G-LEGATO-* (Stage 2 W4): the frozen outgoing cycle keeps the rate it
    // was heard at. 48 kHz, processor Rig, block 64, position 1.
    struct XfArm
    {
        const char* name;
        int bank;
        bool mono;
        std::vector<Ev> evs, evsOld;   // evsOld = the same render without the pitch change
        std::vector<int> pitchAt;      // every pitch-change event e in evs: its fade [e+1, e+239] may differ fix vs NC
    };

    constexpr double kXfFs = 48000.0;

    std::vector<float> renderXf (const XfArm& arm, int xfOverride, bool keepRate, bool old, double seconds)
    {
        Rig rig (kXfFs, 64, with (baseParams (arm.bank, 1.0f), ids::voiceMode, arm.mono ? 1.0f : 0.0f));
        if (xfOverride >= 0)
            rig.proc->setXfadeLenOverrideForTesting (xfOverride);
        rig.proc->setXfadeKeepRateForTesting (keepRate);
        return rig.run (secs (seconds, kXfFs), old ? arm.evsOld : arm.evs);
    }

    std::vector<XfArm> xfArms (int t)
    {
        return {
            { "legato 60->96 Sine->Saw", BankFactory::sineSaw, true, { evOn (0, 60), evOn (t, 96) }, { evOn (0, 60) }, { t } },
            { "legato 60->96 Drive", BankFactory::drive, true, { evOn (0, 60), evOn (t, 96) }, { evOn (0, 60) }, { t } },
            // The note-on 60 at 100 with 96 held is itself a (downward) legato pitch change.
            { "fallback 60->96 Drive", BankFactory::drive, true, { evOn (0, 96), evOn (100, 60), evOff (t, 60) },
                                                                 { evOn (0, 96), evOn (100, 60) }, { 100, t } },
            { "sounding retrigger 60->96 Drive", BankFactory::drive, true,
                { evOn (0, 60), evOff (t - 480, 60), evOn (t, 96) }, { evOn (0, 60), evOff (t - 480, 60), evOn (t, 60) }, { t } },
            { "Poly wheel F#4 +2 st Drive", BankFactory::drive, false, { evOn (0, 66), evWheel (t, 16383) }, { evOn (0, 66) }, { t } },
        };
    }

    // max |y - ((1 - w_k) yOld + w_k yHard)| over the fade k = 0 .. len-1.
    double xfResidual (const std::vector<float>& y, const std::vector<float>& yOld, const std::vector<float>& yHard,
                       int t, int len)
    {
        double r = 0.0;
        for (int k = 0; k < len; ++k)
        {
            const double w = 0.5 - 0.5 * std::cos (kPi * (double) k / (double) len);
            const auto at = (size_t) (t + k);
            const double ref = (1.0 - w) * (double) yOld[at] + w * (double) yHard[at];
            r = std::max (r, std::abs ((double) y[at] - ref));
        }
        return r;
    }

    void gateLegatoXf()
    {
        const int t   = secs (0.25, kXfFs);
        const int len = (int) std::lround (0.005 * kXfFs);       // 240 = the voice's real fade
        const double total = 0.6;

        bool fixOk = true, ncOk = true;
        long long outside = 0;
        std::string dFix, dNc;
        for (const auto& arm : xfArms (t))
        {
            const auto y      = renderXf (arm, -1, true, false, total);
            const auto yOld   = renderXf (arm, -1, true, true, total);
            const auto yHard  = renderXf (arm, 0, true, false, total);
            const auto yN     = renderXf (arm, -1, false, false, total);
            const auto yOldN  = renderXf (arm, -1, false, true, total);
            const auto yHardN = renderXf (arm, 0, false, false, total);

            const double rFix = xfResidual (y, yOld, yHard, t, len);
            const double rNc  = xfResidual (yN, yOldN, yHardN, t, len);
            if (! (rFix <= 1.0e-6)) fixOk = false;
            if (! (rNc >= 0.1))     ncOk = false;

            // Confinement: the fix changes nothing outside the arm's own
            // pitch-change fades [e+1, e+len-1] (every pitch change, W4).
            auto inFade = [&arm, len] (size_t i)
            {
                for (const int e : arm.pitchAt)
                    if (i >= (size_t) (e + 1) && i <= (size_t) (e + len - 1))
                        return true;
                return false;
            };
            long long diff = 0;
            for (size_t i = 0; i < y.size() && i < yN.size(); ++i)
                if (! inFade (i) && std::memcmp (&y[i], &yN[i], sizeof (float)) != 0)
                    ++diff;
            if (y.size() != yN.size())
                ++diff;
            outside += diff;

            std::string windows;
            for (const int e : arm.pitchAt)
                windows += (windows.empty() ? "" : ",") + std::string ("[") + std::to_string (e + 1) + "," + std::to_string (e + len - 1) + "]";

            dFix += std::string (arm.name) + " " + sciStr (rFix) + " (" + std::to_string (diff) + " diffs outside "
                  + windows + "); ";
            dNc  += std::string (arm.name) + " " + fmt (rNc, 3) + "; ";
        }

        report ("G-LEGATO-XF", fixOk && outside == 0,
                "max |y - ((1-w) y_old + w y_hard)| over the real " + std::to_string (len) + "-sample fade (<= 1e-6): " + dFix
                + "fix vs NC bit-identical outside each arm's pitch-change fades [e+1, e+" + std::to_string (len - 1) + "]: "
                + std::to_string (outside) + " differing samples (want 0)");
        if (ncOk)
            report ("G-LEGATO-XF-NC", true,
                    "keep-rate off (setXfadeKeepRateForTesting (false)): " + dNc + "(>= 0.1 each) - live-phase read FAILS as designed");
        else
            report ("G-LEGATO-XF-NC", false,
                    "keep-rate off: " + dNc + "(want >= 0.1 each) - the old read was not caught; the gate is vacuous");
    }

    // Magnifier (RESEARCH B 2, metric C): Kaiser-38 FFT 2^15 at `start`; the
    // max bin outside +/-14 bins of every k * f0 (and DC) relative to the max
    // harmonic bin.
    double inharmonicDb (const std::vector<float>& x, int start, double f0, double fs)
    {
        constexpr int kOrd = 15, kN = 1 << kOrd, kGuardBins = 14;
        if (start < 0 || start + kN > (int) x.size())
            return 1000.0;                                     // out of range: fails every verdict

        juce::dsp::FFT fft (kOrd);
        std::vector<float> win ((size_t) kN, 0.0f), work ((size_t) (2 * kN), 0.0f);
        juce::dsp::WindowingFunction<float>::fillWindowingTables (win.data(), (size_t) kN,
            juce::dsp::WindowingFunction<float>::kaiser, false, 38.0f);
        for (int i = 0; i < kN; ++i)
            work[(size_t) i] = x[(size_t) (start + i)] * win[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (work.data(), true);

        const int nb = kN / 2;
        const double binHz = fs / (double) kN;
        std::vector<unsigned char> mask ((size_t) (nb + 1), 0);
        for (int b = 0; b <= kGuardBins; ++b)
            mask[(size_t) b] = 2;                              // DC: excluded from both
        for (int h = 1; (double) h * f0 < 0.5 * fs; ++h)
        {
            const int c = (int) std::lround ((double) h * f0 / binHz);
            for (int b = juce::jmax (0, c - kGuardBins); b <= juce::jmin (nb, c + kGuardBins); ++b)
                if (mask[(size_t) b] == 0)
                    mask[(size_t) b] = 1;
        }

        double mh = 0.0, mi = 0.0;
        for (int b = 0; b <= nb; ++b)
        {
            const double m = (double) work[(size_t) b];
            if (mask[(size_t) b] == 1)
                mh = std::max (mh, m);
            else if (mask[(size_t) b] == 0)
                mi = std::max (mi, m);
        }
        return dB (mi / std::max (mh, 1.0e-300));
    }

    void gateLegatoAlias()
    {
        const int t = secs (0.25, kXfFs);
        const int magLen = secs (2.0, kXfFs);                  // 2 s fade (the magnifier)
        const int at = t + secs (0.35, kXfFs);
        const double f60 = juce::MidiMessage::getMidiNoteInHertz (60);

        bool fixOk = true, ncOk = true;
        std::string dFix, dNc;
        for (const int bank : { (int) BankFactory::sineSaw, (int) BankFactory::drive })
        {
            const XfArm arm { bank == (int) BankFactory::sineSaw ? "Sine->Saw" : "Drive", bank, true,
                              { evOn (0, 60), evOn (t, 96) }, { evOn (0, 60) }, { t } };
            const double floorDb = inharmonicDb (renderXf (arm, magLen, true, true, 2.0), at, f60, kXfFs);
            const double fixDb   = inharmonicDb (renderXf (arm, magLen, true, false, 2.0), at, f60, kXfFs);
            const double ncDb    = inharmonicDb (renderXf (arm, magLen, false, false, 2.0), at, f60, kXfFs);
            if (! (fixDb <= floorDb + 1.0)) fixOk = false;
            if (! (ncDb >= floorDb + 40.0)) ncOk = false;
            dFix += std::string (arm.name) + " " + fmt (fixDb, 1) + " dB vs no-jump floor " + fmt (floorDb, 1) + " dB; ";
            dNc  += std::string (arm.name) + " " + fmt (ncDb, 1) + " dB (floor " + fmt (floorDb, 1) + ", margin "
                  + fmt (ncDb - floorDb, 1) + " dB); ";
        }

        report ("G-LEGATO-ALIAS", fixOk,
                "48 kHz, Mono legato 60->96, pos 1, 2 s fade, Kaiser-38 2^15 at t+0.35 s, +/-14 bins around k*f60: " + dFix
                + "(<= floor + 1 dB)");
        if (ncOk)
            report ("G-LEGATO-ALIAS-NC", true,
                    "keep-rate off: " + dNc + "(>= floor + 40 dB) - folded alias FAILS as designed");
        else
            report ("G-LEGATO-ALIAS-NC", false,
                    "keep-rate off: " + dNc + "(want >= floor + 40 dB) - the alias was not seen; the gate is vacuous");
    }

    void gateLegatoClick()
    {
        const int t = secs (0.25, kXfFs);
        const XfArm arm { "legato 60->96 Drive", BankFactory::drive, true, { evOn (0, 60), evOn (t, 96) }, { evOn (0, 60) }, { t } };
        auto ratio = [t] (const std::vector<float>& y)
        {
            const double fade   = maxStep (y, t - 32, t + 2400);
            const double steady = maxStep (y, t + 2400, t + 7200);
            return fade / std::max (steady, 1.0e-12);
        };
        const double rFix = ratio (renderXf (arm, -1, true, false, 0.6));
        const double rNc  = ratio (renderXf (arm, -1, false, false, 0.6));

        report ("G-LEGATO-CLICK", rFix <= 1.5,
                "Drive Mono legato 60->96: max |dy| over [t-32, t+2400) / steady max |dy| over [t+2400, t+7200) = "
                + fmt (rFix, 3) + " (<= 1.5); keep-rate off " + fmt (rNc, 3));
        if (rNc > 1.5)
            report ("G-LEGATO-CLICK-NC", true,
                    "keep-rate off: ratio " + fmt (rNc, 3) + " (> 1.5) - the live-phase fade FAILS as designed");
        else
            report ("G-LEGATO-CLICK-NC", false,
                    "keep-rate off: ratio " + fmt (rNc, 3) + " (want > 1.5) - the click was not seen; the gate is vacuous");
    }

    //==========================================================================
    // G-ALLOC stimulus list (RESEARCH 6.6). Every render is armed except each
    // rig's warm-up block; param changes happen between blocks, unarmed.
    void allocScenario()
    {
        const double fs = 48000.0;
        Rig rig (fs, 512, baseParams (BankFactory::sineSaw, 0.3f));
        rig.run (512);                                                       // warm-up (unarmed)

        std::vector<Ev> evs;
        for (int i = 0; i < 8; ++i)
            evs.push_back (evOn (rig.cursor + i * 40, 48 + i));
        rig.run (2048, evs);                                                 // note-ons

        evs.clear();
        for (int i = 0; i < 20; ++i)
            evs.push_back (evOn (rig.cursor + i, 60 + i));
        rig.run (1024, evs);                                                 // 20-note steal

        evs.clear();
        for (int i = 0; i < 64; ++i)
            evs.push_back (evWheel (rig.cursor + i * 32, (i * 256) % 16384));
        rig.run (2048, evs);                                                 // wheel sweep

        for (const float b : { 4.0f, 5.0f, 0.0f, 2.0f, 3.0f, 1.0f })
        {
            rig.set (ids::bank, b);                                          // bank 0 -> 4 -> 5 -> 0 ...
            rig.run (1024);
        }
        for (const float v : { 0.0f, 1.0f })
        {
            rig.set (ids::interp, v);    rig.run (512);
            rig.set (ids::bandlimit, v); rig.run (512);
        }
        for (const float b : { 14.0f, 7.0f, 0.0f })
        {
            rig.set (ids::bitDepth, b);
            rig.run (512);
        }
        rig.set (ids::position, 0.9f);
        rig.run (1024);

        rig.set (ids::voiceMode, 1.0f);                                      // Poly -> Mono
        evs.clear();
        evs.push_back (evOn (rig.cursor + 10, 60));
        evs.push_back (evOn (rig.cursor + 200, 64));
        evs.push_back (evWheel (rig.cursor + 300, 10000));
        evs.push_back (evOff (rig.cursor + 600, 64));
        rig.run (2048, evs);
        rig.set (ids::voiceMode, 0.0f);                                      // Mono -> Poly
        evs.clear();
        for (int i = 0; i < 4; ++i)
            evs.push_back (evOn (rig.cursor + i * 10, 50 + i));
        rig.run (2048, evs);

        evs.clear();
        for (int i = 0; i < 4; ++i)
            evs.push_back (evOff (rig.cursor + i * 10, 50 + i));
        rig.run (secs (0.3, fs), evs);                                       // release tails

        evs.clear();
        evs.push_back (evOn (rig.cursor + 100, 70));
        evs.push_back (evOn (rig.cursor + 1500, 71));
        rig.run (4096, evs, 2048);                                           // oversized host block
        rig.run (512, {}, 1);                                                // 1-sample blocks

        // Stage 2 note 3: one host block carrying a 6000-event flood, Poly
        // then Mono (the capacity guard ends the chunk instead of growing).
        rig.set (ids::voiceMode, 0.0f);
        rig.run (512, floodEvents (rig.cursor, false));                      // Poly flood (CC)
        rig.set (ids::voiceMode, 1.0f);
        rig.run (512, floodEvents (rig.cursor, true));                       // Mono flood (wheel)
        rig.run (1024);
    }

    // Note 3 negative control: the same flood block on a fresh rig with the
    // capacity guard off. Returns the allocations counted in that block.
    int floodAllocsGuardOff (bool mono)
    {
        Rig rig (48000.0, 512, with (baseParams (BankFactory::sineSaw, 0.0f), ids::voiceMode, mono ? 1.0f : 0.0f));
        rig.proc->setMidiCapacityGuardForTesting (false);
        rig.run (512);                                                       // warm-up (unarmed)
        const int before = allocCountNow();
        rig.run (512, floodEvents (rig.cursor, mono));
        return allocCountNow() - before;
    }

    int runAllocCheck()
    {
       #if JUCE_MAC
        audioThread = pthread_self();
        malloc_logger = countAllocation;

        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);

        if (allocCount != 1)
        {
            std::cout << "FAIL G-ALLOC: the malloc hook is not live (a deliberate malloc(64) counted "
                      << allocCount << ", want 1)\n";
            return 1;
        }
        std::cout << "  liveness: deliberate malloc(64) counted 1\n";
        allocCount = 0;
        allocTrace = true;

        gAllocMode = true;
        allocScenario();
        gAllocMode = false;

        const int n = allocCountNow();
        report ("G-ALLOC", n == 0, std::to_string (n) + " audio-thread allocation(s) inside processBlock over note-ons, "
                                   "20-note steal, wheel sweep, bank 0->4->5->0, interp/bandlimit/bit toggles, "
                                   "Poly<->Mono, release tails, oversized + 1-sample blocks, 6000-event MIDI flood "
                                   "Poly + Mono (want 0)");

        // Note 3 negative control: guard off -> the flood grows the chunk buffers on the audio thread.
        allocTrace = false;
        gAllocMode = true;
        const int ncPoly = floodAllocsGuardOff (false);
        const int ncMono = floodAllocsGuardOff (true);
        gAllocMode = false;
        malloc_logger = nullptr;
        if (ncPoly >= 1 && ncMono >= 1)
            report ("G-MIDI-FLOOD[alloc-NC]", true,
                    "capacity guard off (setMidiCapacityGuardForTesting (false)): flood block allocations Poly "
                    + std::to_string (ncPoly) + ", Mono " + std::to_string (ncMono)
                    + " (>= 1 each) - uncapped copy FAILS as designed");
        else
            report ("G-MIDI-FLOOD[alloc-NC]", false,
                    "capacity guard off: flood block allocations Poly " + std::to_string (ncPoly) + ", Mono "
                    + std::to_string (ncMono) + " (want >= 1 each) - the growth was not seen; the alloc gate is vacuous");
        report ("G-FINITE", gNonFinite == 0 && gChannelMismatch == 0,
                std::to_string (gNonFinite) + " non-finite, " + std::to_string (gChannelMismatch)
                + " L != R over " + std::to_string (gSamplesChecked) + " samples");
        return gFailures == 0 ? 0 : 1;
       #else
        std::cout << "FAIL G-ALLOC: --alloc-check is macOS-only\n";
        return 1;
       #endif
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

   #if JUCE_MAC
    // G-UNPREPARED child (self-exec): prints nothing; the exit code is the verdict.
    for (int i = 1; i < argc; ++i)
        if (std::string (argv[i]) == "--child-unprepared")
        {
            bool guardOff = false;
            for (int j = 1; j < argc; ++j)
                if (std::string (argv[j]) == "--guard-off")
                    guardOff = true;
            return runUnpreparedChild (guardOff);
        }
   #endif

    // Keep ONE BuiltInBanks alive for the whole run, so the many short-lived
    // processors share it instead of rebuilding the banks each time.
    juce::SharedResourcePointer<BuiltInBanks> banks;
    std::cout << "dsp-check: built-in banks ready (buildMillis " << fmt (banks->buildMillis, 2) << " ms)\n";

    bool allocCheck = false;
    for (int i = 1; i < argc; ++i)
        if (std::string (argv[i]) == "--alloc-check")
            allocCheck = true;

    if (allocCheck)
        return runAllocCheck();

    // Saw-1023 test frame through the same builder (voice-level arm, D-F).
    WavetableBank saw1023;
    saw1023.name = "Saw-1023";
    saw1023.allocate (1);
    {
        MipmapBuilder builder;
        std::vector<float> spec ((size_t) MipmapBuilder::kPacked, 0.0f);
        BankFactory::fillSawSpectrum (spec.data(), 1023);
        builder.buildFromSpectrum (saw1023, 0, spec.data(), MipmapBuilder::Normalise::peakToUnity);
    }

    Q2Analyzer analyzer;

    gatePitch();
    gateBend();
    gateDsp02 (analyzer);          // analyzer liveness first
    gateQual02 (analyzer, saw1023);
    gatePulseException (analyzer);
    gateDsp01();
    gateDsp03();
    gateFull();
    gateRead (*banks);
    gatePoly();
    gateMono();
    gateRetrig();
    gateSteal();
    gateSwitchTail();
    gateRetrigVel();
    gateNoteOff();
    gateVel();
    gateOut();
    gateImportedEmpty();
    gateBlock();

    // Stage 4 (4-polish Tasks 2-3).
   #if JUCE_MAC
    {
        auto self = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName().toStdString();
        if (self.empty() && argc > 0)
            self = argv[0];
        gateUnprepared (self);
    }
   #else
    report ("G-UNPREPARED", false, "the self-exec child gate is macOS-only");
   #endif
    gateMidiFlood();
    gateMonoWheel();
    gateLegatoXf();
    gateLegatoAlias();
    gateLegatoClick();

    report ("G-FINITE", gNonFinite == 0 && gChannelMismatch == 0,
            std::to_string (gNonFinite) + " non-finite, " + std::to_string (gChannelMismatch)
            + " L != R over " + std::to_string (gSamplesChecked) + " samples");

    std::cout << "dsp-check: " << (gFailures == 0 ? std::string ("ALL PASS") : "FAILURES = " + std::to_string (gFailures))
              << "\n";
    return gFailures == 0 ? 0 : 1;
}
