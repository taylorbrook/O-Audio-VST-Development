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

    O-simpleWavetable viz-check - Stage 3 gates (3-gui/PLAN Tasks 4, 9, 14).

    Task 4:
      G-LESSON    CTX-lessons, D-Y. All 21 params at fixed-seed random
                  normalised values, output_level -17.3 dB, an Imported bank
                  published. For each of the 5 lesson ids:
                    - applyFactoryPreset returns true;
                    - output_level still -17.3 dB (normalised, 1e-6), 0 gestures;
                    - every other param = its recipe value if listed, else its
                      default (normalised, 1e-6);
                    - each changed param got exactly 1 begin + 1 end gesture,
                      unchanged params 0;
                    - the getImportedBankSnapshot() pointer is unchanged.
                  Re-applying the same id -> 0 gestures. "nope" and "" ->
                  false, all 21 values unchanged, 0 gestures.
                  Negative control: a naive reset loop that ALSO resets
                  output_level, run through the same assertion function, must
                  FAIL the output_level check ("FAILS as designed").

    Task 9 (UI-01..03, PERF-03). Every comparison goes through the real wire
    (cycleToVar / bankToVar -> JSON::toString -> JSON::parse), asserting the
    exact key set and that every number is finite:
      G-VIZ-THUMBS    banks 0..4: thumbnails memcmp-equal to bank.thumbs,
                      wire within the 3 dp quantum.
      G-VIZ-EXACT     fs = 440 * 2048, note 69 (inc = 2^-11 exactly): 5 banks
                      x pos {0, 0.37, 1} x Interp On/Off x bit idx {0, 9, 14}
                      = 90 cases. cycle * g vs the output <= 2e-4 (g = least
                      squares), bars vs an INDEPENDENT double-precision direct
                      DFT of the output (ref = max over 1..1023) <= 0.02 dB,
                      level 1, kmax 512, frame, pos, note, f0, nyquistH.
      G-VIZ-N4-LIVE   every EXACT case anchored at 0 dB, max|cycle| > 0.5.
      G-VIZ-N1/N3/N4  negative controls (stale quantizer, one frame off, dead
                      payload): each must FAIL by > 10x ("FAILS as designed").
      G-VIZ-CEIL      fs 48000, Sine->Saw pos 1, note 96, Blackman-Harris +
                      single-bin DFT: On level 7 / kmax 8 / k > 8 at -60 /
                      k <= 8 +/- 0.5 dB / nyquistH 11.47; Off level 0 / kmax
                      1023 / k <= 11 +/- 0.5 dB. G-VIZ-N2: the level-0 ghost
                      bars sit >= 20 dB above the band-limited render.
      G-VIZ-SILENT    no note: knob pos, level 0, frame 19, note -1, and the
                      cycleHash is identical 100 blocks apart (idle-quiet);
                      NC: at lfo_depth 0.5 the hash moves.
      G-VIZ-NOTE      poly note + wheel (f0, nyquistH); Mono legato / return /
                      release (P5, D-X).
      G-VIZ-IMPORTED  the REAL import path: thumbs + filename from cachedBlob,
                      cycle = readSample on the snapshot; P8 (a failed import
                      keeps the bank's filename); empty Imported.
      G-VIZ-ALLOC     (a) 0 audio-thread allocations while a second thread
                      loops the viz path; (b) 0 allocations in 1000 x
                      (buildCycleView + cycleHash) on this thread. Liveness
                      probes (malloc(64) counts 1) before and after.
      G-VIZ-TIME      log only (no wall-clock verdict).

    Task 14 (UI-04, D-Z, P8), through the processor API the natives call:
      G-DROP[a]       a 3-frame 16-bit WAV as STANDARD base64 (Base64::toBase64)
                      -> importFromBase64 true -> done -> bank 5, thumbnails
                      "drop me.wav" / 3 frames, all 11 mip levels memcmp-equal
                      to importFromMemory of the same bytes (fresh processor).
      G-DROP[b]       cap + 1 chars -> false, error / tooLarge, snapshot same.
      G-DROP[c]       "@@@@" -> false, error / unreadable, snapshot same.
      G-DROP[d]       name "../../x\n.wav" -> status + thumbnails "x.wav".
      G-DROP-N1       negative control: the same bytes through JUCE's
                      non-standard MemoryBlock::toBase64Encoding must NOT
                      import ("FAILS as designed").
      G-IMPORT-ERR    2047 samples -> error / tooShort naming the new file;
                      bank index + thumbnails filename unchanged (P8);
                      importToVar keys exactly state, filename, frames, error;
                      every code PluginProcessor.cpp can emit (scanned from
                      the source) is in {tooShort, unreadable, tooLarge,
                      unsupported}.

    Scaffold: report() / info(); dsp-check's setParam + Rig; the O-Bells
    malloc_logger hook with a SETTABLE counted thread (gArmedThread).

    Off by default; -DOUARICON_BUILD_TESTS=ON (OSIW_TEST_HOOKS=1).

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "BankFactory.h"
#include "BuiltInBanks.h"
#include "CycleView.h"
#include "PluginProcessor.h"
#include "VizPayload.h"
#include "WavetableBank.h"
#include "WavetableImporter.h"
#include "WtRead.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if JUCE_MAC
 #include <execinfo.h>
 #include <pthread.h>

// libmalloc's stack-logging hook (what MallocStackLogging / Instruments attach to).
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

    constexpr double kPi = 3.14159265358979323846;
    constexpr int kNumParams = (int) ids::all.size();

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

    //==========================================================================
    // Alloc gate (O-Bells pattern). volatile: clang treats malloc as a builtin
    // that cannot touch globals and would fold "armed = true; malloc();
    // armed = false" otherwise. The counted thread is SETTABLE (gArmedThread):
    // the audio-thread gate counts the processBlock thread, the viz gates count
    // the message (main) thread.
   #if JUCE_MAC
    volatile bool allocArmed = false;
    volatile int  allocCount = 0;
    bool allocTrace = false;
    pthread_t gArmedThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        // Process-wide hook: only the armed thread counts.
        if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), gArmedThread))   // MALLOC_LOG_TYPE_ALLOCATE
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

    // Counts allocations made by the CALLING thread from now on.
    [[maybe_unused]] void setArmedThreadToCaller()
    {
       #if JUCE_MAC
        gArmedThread = pthread_self();
       #endif
    }

    void armAlloc (bool on)
    {
       #if JUCE_MAC
        allocArmed = on;
       #else
        juce::ignoreUnused (on);
       #endif
    }

    [[maybe_unused]] int allocCountNow()
    {
       #if JUCE_MAC
        return allocCount;
       #else
        return 0;
       #endif
    }

    [[maybe_unused]] void resetAllocCount()
    {
       #if JUCE_MAC
        allocCount = 0;
       #endif
    }

    // Installs the hook for the calling thread and proves it is live: a
    // deliberate malloc(64) must count exactly 1. Returns false (and the hook
    // stays uninstalled) when it does not.
    [[maybe_unused]] bool installAllocHook (bool trace)
    {
       #if JUCE_MAC
        setArmedThreadToCaller();
        malloc_logger = countAllocation;

        allocCount = 0;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);

        if (allocCount != 1)
        {
            malloc_logger = nullptr;
            return false;
        }
        allocCount = 0;
        allocTrace = trace;
        return true;
       #else
        juce::ignoreUnused (trace);
        return false;
       #endif
    }

    [[maybe_unused]] void removeAllocHook()
    {
       #if JUCE_MAC
        malloc_logger = nullptr;
       #endif
    }

    //==========================================================================
    struct Ev
    {
        int pos;
        juce::MidiMessage msg;
    };

    using Params = std::vector<std::pair<const char*, float>>;

    void setParam (Proc& p, const char* id, float realValue)
    {
        if (auto* rp = p.getAPVTS().getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
    }

    //==========================================================================
    // Processor rig (dsp-check): params set BEFORE prepare (so smoothers seed
    // on them), events at absolute sample positions from the rig start.
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
    // A small Imported bank through the real builder (frame f = harmonic f+1).
    std::shared_ptr<const WavetableBank> makeImportedBank (int frames)
    {
        ImportedPcm p;
        p.filename = "lesson-fixture.wav";
        p.numFrames = frames;
        p.pcm.resize ((size_t) frames * (size_t) WavetableBank::kTableSize);
        for (int f = 0; f < frames; ++f)
            for (int i = 0; i < WavetableBank::kTableSize; ++i)
            {
                const double ph = 2.0 * kPi * (double) (f + 1) * (double) i / (double) WavetableBank::kTableSize;
                p.pcm[(size_t) (f * WavetableBank::kTableSize + i)] = (std::int16_t) std::lround (std::sin (ph) * 30000.0);
            }
        return WavetableImporter::buildImportedBank (p);
    }

    //==========================================================================
    // G-LESSON. The expected recipes are transcribed here INDEPENDENTLY from
    // the checklist (v1-integration-checklist.md "Lesson preset ids"), not
    // read back from the processor.
    struct Want
    {
        const char* paramId;
        float value;   // real (denormalised) value / choice index / 0|1
    };

    struct Lesson
    {
        const char* id;
        std::vector<Want> recipe;
    };

    std::vector<Lesson> lessonTable()
    {
        return {
            { "steppedSmooth", { { "bank", 0.0f }, { "position", 0.5f }, { "interp", 0.0f }, { "lfo_sync", 0.0f },
                                 { "lfo_shape", 1.0f }, { "lfo_rate", 0.18f }, { "lfo_depth", 1.0f } } },
            { "aliasDemo",     { { "bank", 0.0f }, { "position", 1.0f }, { "bandlimit", 0.0f } } },
            { "driveSweep",    { { "bank", 4.0f }, { "position", 0.0f }, { "env_amount", 1.0f },
                                 { "menv_attack", 0.9f }, { "menv_decay", 1.6f }, { "menv_sustain", 0.25f },
                                 { "menv_release", 0.8f } } },
            { "vowelPad",      { { "bank", 3.0f }, { "position", 0.5f }, { "lfo_sync", 0.0f }, { "lfo_shape", 0.0f },
                                 { "lfo_rate", 0.12f }, { "lfo_depth", 0.9f }, { "amp_attack", 0.6f },
                                 { "amp_release", 1.4f } } },
            { "ppg8bit",       { { "bank", 1.0f }, { "position", 0.6f }, { "interp", 0.0f }, { "bit_depth", 9.0f },
                                 { "lfo_sync", 0.0f }, { "lfo_shape", 4.0f }, { "lfo_rate", 3.0f },
                                 { "lfo_depth", 0.4f } } },
        };
    }

    constexpr float kOutputDb  = -17.3f;
    constexpr float kTolNorm   = 1.0e-6f;

    // Counts begin / end gestures per parameter index.
    struct GestureCounter : public juce::AudioProcessorParameter::Listener
    {
        std::vector<int> begins, ends;

        explicit GestureCounter (int numParams)
            : begins ((size_t) numParams, 0), ends ((size_t) numParams, 0) {}

        void parameterValueChanged (int, float) override {}

        void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
        {
            if (parameterIndex < 0 || parameterIndex >= (int) begins.size())
                return;
            if (gestureIsStarting)
                ++begins[(size_t) parameterIndex];
            else
                ++ends[(size_t) parameterIndex];
        }

        void reset()
        {
            std::fill (begins.begin(), begins.end(), 0);
            std::fill (ends.begin(), ends.end(), 0);
        }
    };

    using Snapshot = std::array<float, (size_t) kNumParams>;   // normalised, ParamIDs::all order

    juce::RangedAudioParameter& paramAt (Proc& p, int i)
    {
        auto* rp = p.getAPVTS().getParameter (ids::all[(size_t) i]);
        jassert (rp != nullptr);
        return *rp;
    }

    Snapshot snapshot (Proc& p)
    {
        Snapshot s {};
        for (int i = 0; i < kNumParams; ++i)
            s[(size_t) i] = paramAt (p, i).getValue();
        return s;
    }

    bool isOutput (int i)
    {
        return std::strcmp (ids::all[(size_t) i], ids::outputLevel) == 0;
    }

    // Every param at a fixed-seed random normalised value, output_level -17.3 dB.
    void randomise (Proc& p, juce::Random& rng)
    {
        for (int i = 0; i < kNumParams; ++i)
        {
            auto& rp = paramAt (p, i);
            rp.setValueNotifyingHost (isOutput (i) ? rp.convertTo0to1 (kOutputDb) : rng.nextFloat());
        }
    }

    struct LessonResult
    {
        bool returnOk = false, outputOk = false, valuesOk = false, gesturesOk = false, importedOk = false;
        int changed = 0, unchanged = 0, listed = 0;
        std::string why;

        bool all() const { return returnOk && outputOk && valuesOk && gesturesOk && importedOk; }
    };

    // recipe == nullptr: the call must change nothing (unknown id / re-apply
    // with `before` already at target is covered by passing the recipe).
    LessonResult checkLesson (Proc& p, const GestureCounter& g, const Snapshot& before,
                              const std::vector<Want>* recipe, bool returned, bool wantReturn,
                              const WavetableBank* importedBefore)
    {
        LessonResult r;
        r.returnOk = (returned == wantReturn);
        if (! r.returnOk)
            r.why += std::string ("returned ") + (returned ? "true" : "false") + "; ";

        r.valuesOk = true;
        r.gesturesOk = true;
        r.outputOk = true;

        for (int i = 0; i < kNumParams; ++i)
        {
            auto& rp = paramAt (p, i);
            const int pi = rp.getParameterIndex();
            const int b = g.begins[(size_t) pi];
            const int e = g.ends[(size_t) pi];
            const float now = rp.getValue();
            const char* pid = ids::all[(size_t) i];

            if (isOutput (i))
            {
                const float wantOut = rp.convertTo0to1 (kOutputDb);
                if (std::abs (now - before[(size_t) i]) > kTolNorm || std::abs (now - wantOut) > kTolNorm
                    || b != 0 || e != 0)
                {
                    r.outputOk = false;
                    r.why += "output_level now " + fmt (rp.convertFrom0to1 (now), 2) + " dB (want "
                             + fmt (kOutputDb, 1) + "), gestures " + std::to_string (b) + "/"
                             + std::to_string (e) + "; ";
                }
                continue;
            }

            float want = before[(size_t) i];
            if (recipe != nullptr)
            {
                want = rp.getDefaultValue();
                for (const auto& w : *recipe)
                    if (std::strcmp (pid, w.paramId) == 0)
                    {
                        want = rp.convertTo0to1 (w.value);
                        ++r.listed;
                    }
            }

            if (std::abs (now - want) > kTolNorm)
            {
                r.valuesOk = false;
                r.why += std::string (pid) + " = " + fmt (rp.convertFrom0to1 (now), 4) + " (want "
                         + fmt (rp.convertFrom0to1 (want), 4) + "); ";
            }

            const bool shouldChange = std::abs (before[(size_t) i] - want) >= kTolNorm;
            if (shouldChange)
                ++r.changed;
            else
                ++r.unchanged;

            const bool gestureRight = shouldChange ? (b == 1 && e == 1) : (b == 0 && e == 0);
            if (! gestureRight)
            {
                r.gesturesOk = false;
                r.why += std::string (pid) + " gestures " + std::to_string (b) + "/" + std::to_string (e)
                         + (shouldChange ? " (want 1/1); " : " (want 0/0); ");
            }
        }

        r.importedOk = (p.getImportedBankSnapshot().get() == importedBefore);
        if (! r.importedOk)
            r.why += "Imported snapshot pointer changed; ";

        return r;
    }

    std::string describe (const LessonResult& r)
    {
        return "return " + std::string (r.returnOk ? "ok" : "BAD")
             + ", output_level " + (r.outputOk ? std::string ("-17.3 dB untouched, 0 gestures") : std::string ("MOVED"))
             + ", " + std::to_string (r.listed) + " recipe + " + std::to_string (kNumParams - 1 - r.listed)
             + " default targets " + (r.valuesOk ? "match" : "MISMATCH")
             + ", " + std::to_string (r.changed) + " changed (1+1 gestures) / " + std::to_string (r.unchanged)
             + " unchanged (0) " + (r.gesturesOk ? "ok" : "BAD")
             + ", Imported " + (r.importedOk ? "unchanged" : "CHANGED")
             + (r.why.empty() ? std::string() : " | " + r.why);
    }

    // Negative control: the O-simpleAdditive-style loop (P9) - every param,
    // output_level INCLUDED, to its recipe value or default, with gestures.
    void naiveApply (Proc& p, const std::vector<Want>& recipe)
    {
        for (int i = 0; i < kNumParams; ++i)
        {
            auto& rp = paramAt (p, i);
            float target = rp.getDefaultValue();
            for (const auto& w : recipe)
                if (std::strcmp (ids::all[(size_t) i], w.paramId) == 0)
                    target = rp.convertTo0to1 (w.value);
            if (std::abs (rp.getValue() - target) < kTolNorm)
                continue;
            rp.beginChangeGesture();
            rp.setValueNotifyingHost (target);
            rp.endChangeGesture();
        }
    }

    void gateLesson()
    {
        Rig rig (48000.0, 512, {});
        Proc& p = *rig.proc;

        auto imported = makeImportedBank (2);
        if (imported == nullptr)
        {
            report ("G-LESSON", false, "fixture: buildImportedBank returned nullptr");
            return;
        }
        p.publishImportedBankForTesting (imported);
        const WavetableBank* importedPtr = p.getImportedBankSnapshot().get();
        if (importedPtr == nullptr)
        {
            report ("G-LESSON", false, "fixture: Imported snapshot is empty after publish");
            return;
        }

        const auto lessons = lessonTable();

        // Every expected recipe id names a real parameter (typo guard on the
        // test side; the processor jasserts its own).
        {
            int bad = 0;
            for (const auto& l : lessons)
                for (const auto& w : l.recipe)
                    if (p.getAPVTS().getParameter (w.paramId) == nullptr)
                        ++bad;
            report ("G-LESSON[ids]", bad == 0, std::to_string (bad) + " recipe entries name no parameter (want 0)");
        }

        GestureCounter g ((int) p.getParameters().size());
        for (auto* prm : p.getParameters())
            prm->addListener (&g);

        juce::Random rng (0x05157A7Eull);

        for (const auto& l : lessons)
        {
            randomise (p, rng);
            const auto before = snapshot (p);
            g.reset();
            const bool ok = p.applyFactoryPreset (l.id);
            const auto r = checkLesson (p, g, before, &l.recipe, ok, true, importedPtr);
            const std::string gate = std::string ("G-LESSON[") + l.id + "]";
            report (gate.c_str(), r.all(), describe (r));

            // Re-apply the same id: everything already at target -> 0 gestures.
            const auto again = snapshot (p);
            g.reset();
            const bool ok2 = p.applyFactoryPreset (l.id);
            const auto r2 = checkLesson (p, g, again, &l.recipe, ok2, true, importedPtr);
            const bool zero = r2.changed == 0;
            const std::string gate2 = std::string ("G-LESSON[") + l.id + " reapply]";
            report (gate2.c_str(), r2.all() && zero,
                    std::to_string (r2.changed) + " params changed (want 0); " + describe (r2));
        }

        // Unknown ids: false, nothing changes, no gestures.
        for (const char* unknown : { "nope", "" })
        {
            randomise (p, rng);
            const auto before = snapshot (p);
            g.reset();
            const bool ok = p.applyFactoryPreset (unknown);
            const auto r = checkLesson (p, g, before, nullptr, ok, false, importedPtr);
            const std::string gate = std::string ("G-LESSON[unknown \"") + unknown + "\"]";
            report (gate.c_str(), r.all() && r.changed == 0, describe (r));
        }

        // Negative control: the naive loop must FAIL the output_level check.
        {
            const auto& l = lessons[1];   // aliasDemo
            randomise (p, rng);
            const auto before = snapshot (p);
            g.reset();
            naiveApply (p, l.recipe);
            const auto r = checkLesson (p, g, before, &l.recipe, true, true, importedPtr);
            if (! r.outputOk)
            {
                info ("G-LESSON-NC: naive reset loop (output_level included) FAILS as designed: " + r.why);
                report ("G-LESSON-NC", true, "naive loop FAILS as designed (output_level check caught it)");
            }
            else
            {
                report ("G-LESSON-NC", false, "naive loop PASSED the output_level check - the gate is vacuous");
            }
        }

        for (auto* prm : p.getParameters())
            prm->removeListener (&g);
    }

    //==========================================================================
    //==========================================================================
    // Stage 3.2 viz gates (PLAN Task 9). Every comparison goes through the
    // real wire: buildCycleView / getBankThumbnails -> cycleToVar / bankToVar
    // -> JSON::toString (one line, as emitEventIfBrowserIsVisible) ->
    // JSON::parse, with the exact key set and every number finite.

    constexpr int    kTable         = WavetableBank::kTableSize;      // 2048
    constexpr int    kBars          = Proc::CycleView::kHarmonics;    // 32
    constexpr int    kPts           = Proc::CycleView::kPoints;       // 256
    constexpr int    kThumbPts      = WavetableBank::kThumbSize;      // 128
    constexpr double kTolCycle      = 2.0e-4;                         // G-VIZ-EXACT cycle (abs)
    constexpr double kTolDb         = 0.02;                           // G-VIZ-EXACT bars (dB)
    constexpr double kTolCeilDb     = 0.5;                            // G-VIZ-CEIL bars (dB)
    constexpr double kTolThumb      = 5.0e-4 + 1.0e-12;               // 3 dp wire quantum / 2
    constexpr double kBarLive       = -59.5;                          // bars compared above this
    constexpr double kFloorDb       = -60.0;
    constexpr int    kGateRefMaxBin = 1023;                           // gate's own reference span (PLAN)
    constexpr double kExactFs       = 901120.0;                       // 440 * 2048: inc = 2^-11 exactly

    std::string sci (double v)
    {
        std::ostringstream s;
        s << std::scientific << std::setprecision (2) << v;
        return s.str();
    }

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

    // Steady single-voice baseline: instant attack, sustain 1, 0 dB out, no
    // modulation of the position.
    Params vizBase (int bank, float pos, bool interp, int bitIdx)
    {
        return { { ids::bank, (float) bank }, { ids::position, pos }, { ids::interp, interp ? 1.0f : 0.0f },
                 { ids::bandlimit, 1.0f }, { ids::bitDepth, (float) bitIdx },
                 { ids::ampAttack, 0.001f }, { ids::ampDecay, 0.001f }, { ids::ampSustain, 1.0f },
                 { ids::ampRelease, 0.05f }, { ids::voiceMode, 0.0f }, { ids::outputLevel, 0.0f },
                 { ids::lfoDepth, 0.0f }, { ids::envAmount, 0.0f } };
    }

    //==========================================================================
    // Wire helpers.
    bool isNumber (const juce::var& x)
    {
        return x.isInt() || x.isInt64() || x.isDouble();
    }

    bool allFinite (const juce::var& x)
    {
        if (isNumber (x))
            return std::isfinite ((double) x);
        if (const auto* arr = x.getArray())
        {
            for (const auto& e : *arr)
                if (! allFinite (e))
                    return false;
            return true;
        }
        if (auto* obj = x.getDynamicObject())
        {
            for (const auto& nv : obj->getProperties())
                if (! allFinite (nv.value))
                    return false;
            return true;
        }
        return x.isBool() || x.isString();
    }

    std::string joinKeys (const std::set<std::string>& s)
    {
        std::string out;
        for (const auto& k : s)
            out += (out.empty() ? "" : ",") + k;
        return out;
    }

    bool exactKeys (const juce::var& o, const std::vector<std::string>& want, std::string& why)
    {
        auto* obj = o.getDynamicObject();
        if (obj == nullptr)
        {
            why += "payload is not an object; ";
            return false;
        }
        std::set<std::string> have;
        for (const auto& nv : obj->getProperties())
            have.insert (nv.name.toString().toStdString());
        const std::set<std::string> expected (want.begin(), want.end());
        if (have == expected)
            return true;
        why += "keys {" + joinKeys (have) + "} != {" + joinKeys (expected) + "}; ";
        return false;
    }

    // var -> JSON::toString (one line) -> JSON::parse. true = parsed and every
    // number finite.
    bool throughWire (const juce::var& v, juce::var& parsed, std::size_t& bytes, std::string& why)
    {
        const auto text = juce::JSON::toString (v, true);
        bytes = text.getNumBytesAsUTF8();
        const auto res = juce::JSON::parse (text, parsed);
        if (res.failed())
        {
            why += "JSON::parse failed (" + res.getErrorMessage().toStdString() + "); ";
            return false;
        }
        if (! allFinite (parsed))
        {
            why += "non-finite number on the wire; ";
            return false;
        }
        return true;
    }

    bool readNumber (const juce::var& o, const char* key, double& out, std::string& why)
    {
        const auto x = o.getProperty (key, juce::var());
        if (! isNumber (x))
        {
            why += std::string (key) + " is not a number; ";
            return false;
        }
        out = (double) x;
        return true;
    }

    bool readInt (const juce::var& o, const char* key, int& out, std::string& why)
    {
        const auto x = o.getProperty (key, juce::var());
        if (! (x.isInt() || x.isInt64()))
        {
            why += std::string (key) + " is not an integer; ";
            return false;
        }
        out = (int) x;
        return true;
    }

    bool readBool (const juce::var& o, const char* key, bool& out, std::string& why)
    {
        const auto x = o.getProperty (key, juce::var());
        if (! x.isBool())
        {
            why += std::string (key) + " is not a bool; ";
            return false;
        }
        out = (bool) x;
        return true;
    }

    bool readNumbers (const juce::var& x, std::size_t n, std::vector<double>& out)
    {
        const auto* arr = x.getArray();
        if (arr == nullptr || (std::size_t) arr->size() != n)
            return false;
        out.assign (n, 0.0);
        for (int i = 0; i < arr->size(); ++i)
        {
            const juce::var e = (*arr)[i];
            if (! isNumber (e))
                return false;
            out[(std::size_t) i] = (double) e;
        }
        return true;
    }

    bool readArray (const juce::var& o, const char* key, std::size_t n, std::vector<double>& out, std::string& why)
    {
        if (readNumbers (o.getProperty (key, juce::var()), n, out))
            return true;
        why += std::string (key) + " is not an array of " + std::to_string (n) + " numbers; ";
        return false;
    }

    //==========================================================================
    // cycleUpdate as the page receives it.
    struct WireCycle
    {
        bool ok = false;
        std::string why;
        std::size_t bytes = 0;
        std::vector<double> cycle, harmonics, harmonicsRaw, preQ;
        double pos = 0.0, nyquistH = 0.0, f0 = 0.0, lfo = 0.0, menv = 0.0, amp = 0.0;
        int frame = 0, level = 0, kmax = 0, note = 0;
        bool sounding = false;
    };

    WireCycle wireCycle (const Proc::CycleView& v)
    {
        WireCycle w;
        juce::var o;
        bool ok = throughWire (viz::cycleToVar (v), o, w.bytes, w.why);

        std::vector<std::string> keys { "cycle", "harmonics", "harmonicsRaw", "pos", "frame", "level", "kmax",
                                        "nyquistH", "note", "f0", "lfo", "menv", "amp", "sounding" };
        if (v.quantized)
            keys.emplace_back ("preQ");               // only when quantized
        ok = ok && exactKeys (o, keys, w.why);

        if (ok)
        {
            auto need = [&ok] (bool b) { ok = ok && b; };
            need (readArray (o, "cycle", (std::size_t) kPts, w.cycle, w.why));
            need (readArray (o, "harmonics", (std::size_t) kBars, w.harmonics, w.why));
            need (readArray (o, "harmonicsRaw", (std::size_t) kBars, w.harmonicsRaw, w.why));
            if (v.quantized)
                need (readArray (o, "preQ", (std::size_t) kPts, w.preQ, w.why));
            need (readNumber (o, "pos", w.pos, w.why));
            need (readNumber (o, "nyquistH", w.nyquistH, w.why));
            need (readNumber (o, "f0", w.f0, w.why));
            need (readNumber (o, "lfo", w.lfo, w.why));
            need (readNumber (o, "menv", w.menv, w.why));
            need (readNumber (o, "amp", w.amp, w.why));
            need (readInt (o, "frame", w.frame, w.why));
            need (readInt (o, "level", w.level, w.why));
            need (readInt (o, "kmax", w.kmax, w.why));
            need (readInt (o, "note", w.note, w.why));
            need (readBool (o, "sounding", w.sounding, w.why));
        }
        w.ok = ok;
        return w;
    }

    // bankUpdate as the page receives it.
    struct WireBank
    {
        bool ok = false;
        std::string why;
        std::size_t bytes = 0;
        int bank = -1, numFrames = -1;
        bool imported = false;
        juce::String filename;
        std::vector<std::vector<double>> frames;
    };

    WireBank wireBank (const Proc::BankThumbs& t)
    {
        WireBank w;
        juce::var o;
        bool ok = throughWire (viz::bankToVar (t), o, w.bytes, w.why);
        ok = ok && exactKeys (o, { "bank", "imported", "numFrames", "filename", "frames" }, w.why);

        if (ok)
        {
            auto need = [&ok] (bool b) { ok = ok && b; };
            need (readInt (o, "bank", w.bank, w.why));
            need (readInt (o, "numFrames", w.numFrames, w.why));
            need (readBool (o, "imported", w.imported, w.why));

            const auto fn = o.getProperty ("filename", juce::var());
            need (fn.isString());
            w.filename = fn.toString();

            const auto fr = o.getProperty ("frames", juce::var());
            const auto* arr = fr.getArray();
            need (arr != nullptr);
            if (arr != nullptr)
            {
                w.frames.resize ((std::size_t) arr->size());
                for (int i = 0; i < arr->size(); ++i)
                    need (readNumbers ((*arr)[i], (std::size_t) kThumbPts, w.frames[(std::size_t) i]));
            }
            if (! ok)
                w.why += "bankUpdate fields malformed; ";
        }
        w.ok = ok;
        return w;
    }

    // Thumbnail points on the wire vs the bank's own thumbs (3 dp quantum).
    double thumbWireError (const WireBank& w, const std::vector<float>& thumbs)
    {
        double err = 0.0;
        for (std::size_t f = 0; f < w.frames.size(); ++f)
            for (std::size_t i = 0; i < w.frames[f].size(); ++i)
            {
                const std::size_t at = f * (std::size_t) kThumbPts + i;
                if (at >= thumbs.size())
                    return 1.0e9;
                err = std::max (err, std::abs (w.frames[f][i] - (double) thumbs[at]));
            }
        return err;
    }

    bool sameFloats (const std::vector<float>& a, const std::vector<float>& b)
    {
        return a.size() == b.size()
            && (a.empty() || std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0);
    }

    //==========================================================================
    // Independent reference for the bars: a direct double-precision DFT of one
    // rendered period (NOT the JUCE FFT the renderer uses). Bars 1..32 in dB
    // re max |X_k| over k = 1..1023, clamped [-60, 0].
    struct DftTable
    {
        std::vector<double> cosT, sinT;

        DftTable() : cosT ((std::size_t) kTable), sinT ((std::size_t) kTable)
        {
            for (int n = 0; n < kTable; ++n)
            {
                const double a = 2.0 * kPi * (double) n / (double) kTable;
                cosT[(std::size_t) n] = std::cos (a);
                sinT[(std::size_t) n] = std::sin (a);
            }
        }
    };

    std::vector<double> dftBarsDb (const std::vector<float>& y)
    {
        static const DftTable tab;
        std::vector<double> mag ((std::size_t) kGateRefMaxBin + 1, 0.0);
        for (int k = 1; k <= kGateRefMaxBin; ++k)
        {
            double re = 0.0, im = 0.0;
            for (int n = 0; n < kTable; ++n)
            {
                const auto at = (std::size_t) ((k * n) & (kTable - 1));
                const double s = (double) y[(std::size_t) n];
                re += s * tab.cosT[at];
                im -= s * tab.sinT[at];
            }
            mag[(std::size_t) k] = std::sqrt (re * re + im * im);
        }

        double ref = 0.0;
        for (int k = 1; k <= kGateRefMaxBin; ++k)
            ref = std::max (ref, mag[(std::size_t) k]);

        std::vector<double> db ((std::size_t) kBars, kFloorDb);
        for (int k = 1; k <= kBars; ++k)
            if (ref > 0.0 && mag[(std::size_t) k] > 0.0)
                db[(std::size_t) (k - 1)] = juce::jlimit (kFloorDb, 0.0, 20.0 * std::log10 (mag[(std::size_t) k] / ref));
        return db;
    }

    // Max |payload - reference| over bars kFrom..kTo (1-based) where EITHER
    // side is above -59.5 dB (a dead payload pinned at the floor is caught,
    // never skipped).
    double barError (const std::vector<double>& payload, const std::vector<double>& ref, int kFrom, int kTo,
                     int* worstK = nullptr)
    {
        double err = 0.0;
        for (int k = kFrom; k <= kTo; ++k)
        {
            const double p = payload[(std::size_t) (k - 1)];
            const double r = ref[(std::size_t) (k - 1)];
            if (p <= kBarLive && r <= kBarLive)
                continue;
            const double d = std::abs (p - r);
            if (d > err)
            {
                err = d;
                if (worstK != nullptr)
                    *worstK = k;
            }
        }
        return err;
    }

    // cycle[j] * g vs y[8j], g = least-squares gain (env * vel^2 * voice gain * out).
    double cycleError (const std::vector<double>& cyc, const std::vector<float>& y)
    {
        const int step = kTable / kPts;
        double num = 0.0, den = 0.0;
        for (int i = 0; i < kPts; ++i)
        {
            const double c = cyc[(std::size_t) i];
            const double t = (double) y[(std::size_t) (i * step)];
            num += c * t;
            den += c * c;
        }
        const double g = den > 0.0 ? num / den : 0.0;
        double err = 0.0;
        for (int i = 0; i < kPts; ++i)
            err = std::max (err, std::abs (cyc[(std::size_t) i] * g - (double) y[(std::size_t) (i * step)]));
        return err;
    }

    //==========================================================================
    // G-VIZ-EXACT rig: fs = 440 * 2048, note 69, so inc = 2^-11 exactly, the
    // phase is exactly j / 2048 and every output sample is a table point. One
    // period, 4 periods after note-on (envelope at sustain 1).
    struct ExactRun
    {
        std::vector<float> y;
        Proc::CycleView view;
        WireCycle w;
    };

    ExactRun runExact (int bank, float pos, bool interp, int bitIdx, int bitIdxAfterRender = -1)
    {
        Rig rig (kExactFs, 512, vizBase (bank, pos, interp, bitIdx));
        const auto out = rig.run (5 * kTable, { evOn (0, 69) });

        ExactRun r;
        r.y.assign (out.begin() + 4 * kTable, out.begin() + 5 * kTable);
        if (bitIdxAfterRender >= 0)
            rig.set (ids::bitDepth, (float) bitIdxAfterRender);   // N1: the payload no longer matches the audio
        rig.proc->buildCycleView (r.view);
        r.w = wireCycle (r.view);
        return r;
    }

    struct ExactCheck
    {
        bool ok = true;
        double cycErr = 0.0, dbErr = 0.0;
        std::string why;
    };

    ExactCheck checkExact (const ExactRun& r, float pos, bool interp, int bitIdx)
    {
        ExactCheck c;
        if (! r.w.ok)
        {
            c.ok = false;
            c.why = "wire: " + r.w.why;
            return c;
        }

        c.cycErr = cycleError (r.w.cycle, r.y);
        c.dbErr  = barError (r.w.harmonics, dftBarsDb (r.y), 1, kBars);

        auto need = [&c] (bool b, const std::string& what)
        {
            if (! b)
            {
                c.ok = false;
                c.why += what + "; ";
            }
        };
        const int wantFrame = interp ? -1 : wt::latchFrame (pos, BuiltInBanks::kFrames);
        need (c.cycErr <= kTolCycle, "cycle err " + sci (c.cycErr));
        need (c.dbErr <= kTolDb, "bar err " + fmt (c.dbErr, 4) + " dB");
        need (r.w.level == 1, "level " + std::to_string (r.w.level) + " (want 1)");
        need (r.w.kmax == 512, "kmax " + std::to_string (r.w.kmax) + " (want 512)");
        need (r.w.frame == wantFrame, "frame " + std::to_string (r.w.frame) + " (want " + std::to_string (wantFrame) + ")");
        need (std::abs (r.w.pos - (double) pos) <= 1.0e-4, "pos " + fmt (r.w.pos, 5));
        need (r.w.note == 69, "note " + std::to_string (r.w.note));
        need (std::abs (r.w.f0 - 440.0) <= 1.0e-3, "f0 " + fmt (r.w.f0, 4));
        need (std::abs (r.w.nyquistH - 1024.0) <= 1.0e-2, "nyquistH " + fmt (r.w.nyquistH, 4));
        need (r.w.sounding, "not sounding");
        need (r.view.quantized == (bitIdx > 0), "quantized flag");
        return c;
    }

    // N4 liveness: the dB scale is anchored (h1 = 0 dB; for Formant, whose
    // strongest partial is the F1 region, the strongest bar = 0 dB) and the
    // cycle is not silent.
    bool liveness (const WireCycle& w, int bank, std::string& why)
    {
        double maxBar = kFloorDb, peak = 0.0;
        for (const double h : w.harmonics)
            maxBar = std::max (maxBar, h);
        for (const double s : w.cycle)
            peak = std::max (peak, std::abs (s));
        const double anchor = bank == BankFactory::formant ? maxBar : w.harmonics[0];
        const bool ok = anchor >= -0.01 && peak > 0.5;
        if (! ok)
            why += "bank " + std::to_string (bank) + ": anchor bar " + fmt (anchor, 2) + " dB, max|cycle| " + fmt (peak, 3) + "; ";
        return ok;
    }

    void gateVizExact()
    {
        const std::array<float, 3> positions { 0.0f, 0.37f, 1.0f };
        const std::array<int, 3> bitIdxs { 0, 9, 14 };   // Full, 8 bits, 3 bits

        int cases = 0, passes = 0, liveCases = 0, livePasses = 0;
        double worstCyc = 0.0, worstDb = 0.0;
        std::string fails, liveWhy;

        for (int bank = 0; bank < BuiltInBanks::kCount; ++bank)
        {
            double bankCyc = 0.0, bankDb = 0.0;
            for (const float pos : positions)
                for (const bool interp : { true, false })
                    for (const int bitIdx : bitIdxs)
                    {
                        const auto r = runExact (bank, pos, interp, bitIdx);
                        const auto c = checkExact (r, pos, interp, bitIdx);
                        ++cases;
                        if (c.ok)
                            ++passes;
                        else if (fails.size() < 1200)
                            fails += "[bank " + std::to_string (bank) + " pos " + fmt (pos, 2)
                                   + (interp ? " On" : " Off") + " idx " + std::to_string (bitIdx) + "] " + c.why;
                        bankCyc = std::max (bankCyc, c.cycErr);
                        bankDb  = std::max (bankDb, c.dbErr);

                        ++liveCases;
                        if (r.w.ok && liveness (r.w, bank, liveWhy))
                            ++livePasses;
                    }
            worstCyc = std::max (worstCyc, bankCyc);
            worstDb  = std::max (worstDb, bankDb);
            info ("G-VIZ-EXACT bank " + std::to_string (bank) + ": worst cycle |err| " + sci (bankCyc)
                  + ", worst bar err " + fmt (bankDb, 4) + " dB (18 cases)");
        }

        report ("G-VIZ-EXACT", cases == 90 && passes == cases,
                std::to_string (passes) + "/" + std::to_string (cases) + " cases (5 banks x pos {0, 0.37, 1} x "
                "Interp On/Off x bit idx {0, 9, 14}); worst cycle |err| " + sci (worstCyc) + " (tol 2e-4), worst bar err "
                + fmt (worstDb, 4) + " dB vs direct DFT (tol 0.02); level 1, kmax 512, frame, pos, note 69, f0 440, "
                "nyquistH 1024, sounding" + (fails.empty() ? std::string() : " | " + fails));
        report ("G-VIZ-N4-LIVE", livePasses == liveCases,
                std::to_string (livePasses) + "/" + std::to_string (liveCases)
                + " cases anchored at 0 dB with max|cycle| > 0.5" + (liveWhy.empty() ? std::string() : " | " + liveWhy));
    }

    // N1, N3, N4: each must FAIL its comparison by more than 10x tolerance.
    void gateVizExactNegatives()
    {
        // N1: 3-bit audio, bit_depth set to Full AFTER the render -> the
        // payload is the unquantized cycle.
        {
            const auto r = runExact (0, 0.37f, true, 14, 0);
            const double err = r.w.ok ? cycleError (r.w.cycle, r.y) : 0.0;
            const double margin = err / kTolCycle;
            if (r.w.ok && margin > 10.0)
                report ("G-VIZ-N1", true, "stale quantizer FAILS as designed (x" + fmt (margin, 1) + "): cycle |err| "
                                          + sci (err) + " vs tol 2e-4");
            else
                report ("G-VIZ-N1", false, "stale quantizer did NOT fail by > 10x (|err| " + sci (err)
                                           + ") - G-VIZ-EXACT is vacuous" + (r.w.ok ? "" : " | wire: " + r.w.why));
        }

        // N3: Sine->Saw Interp Off; the payload at pos vs the audio one frame
        // later (pos + 1/31: frame k adds harmonic k + 1).
        {
            const auto a = runExact (0, 0.37f, false, 0);
            const auto b = runExact (0, 0.37f + 1.0f / 31.0f, false, 0);
            int worstK = 0;
            const double d = a.w.ok ? barError (a.w.harmonics, dftBarsDb (b.y), 1, kBars, &worstK) : 0.0;
            const double margin = d / kTolDb;
            if (a.w.ok && d >= 3.0 && margin > 10.0)
                report ("G-VIZ-N3", true, "one-frame-off position FAILS as designed (x" + fmt (margin, 1) + "): bar h"
                                          + std::to_string (worstK) + " differs by " + fmt (d, 2) + " dB (>= 3 dB)");
            else
                report ("G-VIZ-N3", false, "one-frame-off position differs by only " + fmt (d, 3)
                                           + " dB - G-VIZ-EXACT cannot see the frame" + (a.w.ok ? "" : " | wire: " + a.w.why));
        }

        // N4: a dead payload (zero cycle, every bar at the floor) through the
        // same comparison must fail both the cycle and the bar checks.
        {
            const auto r = runExact (0, 0.37f, true, 0);
            auto dead = r.view;
            dead.cycle.fill (0.0f);
            dead.preQ.fill (0.0f);
            dead.harmonicsDb.fill (-60.0f);
            dead.harmonicsRawDb.fill (-60.0f);
            const auto w = wireCycle (dead);
            const double cyc = w.ok ? cycleError (w.cycle, r.y) : 0.0;
            const double db  = w.ok ? barError (w.harmonics, dftBarsDb (r.y), 1, kBars) : 0.0;
            const double margin = std::min (cyc / kTolCycle, db / kTolDb);
            if (w.ok && margin > 10.0)
                report ("G-VIZ-N4", true, "dead payload FAILS as designed (x" + fmt (margin, 1) + "): cycle |err| "
                                          + sci (cyc) + ", bar err " + fmt (db, 2) + " dB");
            else
                report ("G-VIZ-N4", false, "dead payload was not rejected by > 10x (cycle " + sci (cyc) + ", bars "
                                           + fmt (db, 3) + " dB) - G-VIZ-EXACT is vacuous");
        }
    }

    //==========================================================================
    // G-VIZ-CEIL: fs 48000, Sine->Saw pos 1 (frame 32 = h1..h32), note 96.
    // 1 s render; Blackman-Harris window, double-precision single-bin DFT at
    // k * f0. Bars in dB re the strongest measured harmonic below fs / 2.
    struct CeilRun
    {
        WireCycle w;
        std::vector<double> renderDb;
        double f0 = 0.0, fs = 48000.0;
    };

    CeilRun runCeil (bool bandlimit)
    {
        CeilRun c;
        auto prm = vizBase (0, 1.0f, true, 0);
        prm.emplace_back (ids::bandlimit, bandlimit ? 1.0f : 0.0f);   // later entry wins
        Rig rig (c.fs, 512, prm);
        const auto y = rig.run ((int) c.fs, { evOn (0, 96) });

        Proc::CycleView view;
        rig.proc->buildCycleView (view);
        c.w = wireCycle (view);

        c.f0 = 440.0 * std::pow (2.0, (double) (96 - 69) / 12.0);
        constexpr int start = 8192, len = 32768;
        std::vector<double> win ((std::size_t) len);
        for (int n = 0; n < len; ++n)
        {
            const double x = 2.0 * kPi * (double) n / (double) (len - 1);
            win[(std::size_t) n] = 0.35875 - 0.48829 * std::cos (x) + 0.14128 * std::cos (2.0 * x)
                                 - 0.01168 * std::cos (3.0 * x);
        }

        std::vector<double> mag ((std::size_t) kBars + 1, 0.0);
        for (int k = 1; k <= kBars; ++k)
        {
            const double om = 2.0 * kPi * (double) k * c.f0 / c.fs;
            double re = 0.0, im = 0.0;
            for (int n = 0; n < len; ++n)
            {
                const double s = win[(std::size_t) n] * (double) y[(std::size_t) (start + n)];
                re += s * std::cos (om * (double) n);
                im -= s * std::sin (om * (double) n);
            }
            mag[(std::size_t) k] = std::sqrt (re * re + im * im);
        }

        double ref = 0.0;
        for (int k = 1; k <= kBars; ++k)
            if ((double) k * c.f0 < 0.5 * c.fs)
                ref = std::max (ref, mag[(std::size_t) k]);

        c.renderDb.assign ((std::size_t) kBars, kFloorDb);
        for (int k = 1; k <= kBars; ++k)
            if (ref > 0.0 && mag[(std::size_t) k] > 0.0)
                c.renderDb[(std::size_t) (k - 1)] = juce::jlimit (kFloorDb, 0.0, 20.0 * std::log10 (mag[(std::size_t) k] / ref));
        return c;
    }

    void gateVizCeil()
    {
        // Band-limit On: level 7 (x = 89.3), kmax 8.
        const auto on = runCeil (true);
        {
            std::string why = on.w.ok ? std::string() : "wire: " + on.w.why;
            const double nyqWant = 0.5 * on.fs / on.f0;
            double above = kFloorDb;
            for (int k = 9; k <= kBars && on.w.ok; ++k)
                above = std::max (above, on.w.harmonics[(std::size_t) (k - 1)]);
            const double err = on.w.ok ? barError (on.w.harmonics, on.renderDb, 1, 8) : 1.0e9;
            const bool ok = on.w.ok && on.w.level == 7 && on.w.kmax == 8 && above <= kFloorDb + 1.0e-9
                         && err <= kTolCeilDb && std::abs (on.w.nyquistH - nyqWant) <= 1.0e-2
                         && std::abs (on.w.nyquistH - 11.47) <= 1.0e-2;
            report ("G-VIZ-CEIL[On]", ok,
                    "level " + std::to_string (on.w.level) + " (want 7), kmax " + std::to_string (on.w.kmax)
                    + " (want 8), bars k > 8 max " + fmt (above, 2) + " dB (want -60), k <= 8 worst |payload - render| "
                    + fmt (err, 3) + " dB (tol 0.5), nyquistH " + fmt (on.w.nyquistH, 4) + " (want "
                    + fmt (nyqWant, 4) + " / 11.47 +/- 0.01)" + (why.empty() ? std::string() : " | " + why));

            // N2: the level-0 ghost bars (harmonicsRaw) vs the band-limited
            // render, k = 9..32 where the render sits at the floor.
            double minExcess = 1.0e9;
            int counted = 0;
            for (int k = 9; k <= kBars && on.w.ok; ++k)
                if (on.renderDb[(std::size_t) (k - 1)] <= kBarLive)
                {
                    minExcess = std::min (minExcess, on.w.harmonicsRaw[(std::size_t) (k - 1)] - on.renderDb[(std::size_t) (k - 1)]);
                    ++counted;
                }
            if (on.w.ok && counted > 0 && minExcess >= 20.0)
                report ("G-VIZ-N2", true, "level-0 ghost bars vs the band-limited render FAIL as designed (x"
                                          + fmt (minExcess / kTolCeilDb, 1) + "): over " + std::to_string (counted)
                                          + " floor bars k = 9..32 the ghost sits >= " + fmt (minExcess, 1)
                                          + " dB above the render (>= 20)");
            else
                report ("G-VIZ-N2", false, "ghost bars not >= 20 dB above the floor (min " + fmt (minExcess, 2)
                                           + " dB over " + std::to_string (counted) + " bars) - the level is invisible");
        }

        // Band-limit Off: level 0, kmax 1023; h1..h11 are below fs / 2.
        const auto off = runCeil (false);
        {
            const double err = off.w.ok ? barError (off.w.harmonics, off.renderDb, 1, 11) : 1.0e9;
            const bool ok = off.w.ok && off.w.level == 0 && off.w.kmax == 1023 && err <= kTolCeilDb;
            report ("G-VIZ-CEIL[Off]", ok,
                    "level " + std::to_string (off.w.level) + " (want 0), kmax " + std::to_string (off.w.kmax)
                    + " (want 1023), k <= 11 worst |payload - render| " + fmt (err, 3) + " dB (tol 0.5)"
                    + (off.w.ok ? std::string() : " | wire: " + off.w.why));
        }
    }

    //==========================================================================
    // G-VIZ-SILENT (P1, P4, D-R): no note, knob 0.6, Interp Off, Sine->Square,
    // LFO free 5 Hz at depth 0. NC: at depth 0.5 the hashes must differ.
    void gateVizSilent()
    {
        auto prm = vizBase (1, 0.6f, false, 0);
        prm.emplace_back (ids::lfoSync, 0.0f);
        prm.emplace_back (ids::lfoRate, 5.0f);
        Rig rig (48000.0, 512, prm);
        Proc& p = *rig.proc;
        rig.run (512 * 4);

        Proc::CycleView v1, v2;
        p.buildCycleView (v1);
        const auto w = wireCycle (v1);
        const auto h1 = viz::cycleHash (v1);
        rig.run (512 * 100);
        p.buildCycleView (v2);
        const auto h2 = viz::cycleHash (v2);

        const int wantFrame = wt::latchFrame (0.6f, BuiltInBanks::kFrames);   // lround (0.6 * 31) = 19
        const bool ok = w.ok && ! w.sounding && w.note == -1 && w.level == 0 && w.kmax == 1023
                     && w.frame == 19 && wantFrame == 19 && std::abs (w.pos - 0.6) <= 1.0e-4
                     && std::abs (w.f0) <= 0.0 && std::abs (w.nyquistH) <= 0.0 && std::abs (w.lfo) <= 0.0
                     && h1 == h2;
        report ("G-VIZ-SILENT", ok,
                std::string ("sounding ") + (w.sounding ? "true" : "false") + " (want false), note "
                + std::to_string (w.note) + " (want -1), level " + std::to_string (w.level) + ", frame "
                + std::to_string (w.frame) + " (want 19), pos " + fmt (w.pos, 4) + " (want 0.6), f0 " + fmt (w.f0, 2)
                + ", lfo " + fmt (w.lfo, 3) + "; cycleHash 100 blocks apart " + (h1 == h2 ? "EQUAL (idle-quiet)" : "DIFFERS")
                + (w.ok ? std::string() : " | wire: " + w.why));

        // NC: the LFO lamp is live at depth 0.5 -> the payload moves.
        rig.set (ids::lfoDepth, 0.5f);
        rig.run (512 * 4);
        Proc::CycleView a, b, c;
        p.buildCycleView (a);
        rig.run (512 * 37);
        p.buildCycleView (b);
        rig.run (512 * 63);
        p.buildCycleView (c);
        const auto ha = viz::cycleHash (a);
        const bool moved = ha != viz::cycleHash (b) || ha != viz::cycleHash (c);
        const double dl = std::max (std::abs ((double) b.lfo - (double) a.lfo), std::abs ((double) c.lfo - (double) a.lfo));
        if (moved)
            report ("G-VIZ-SILENT-NC", true, "depth 0.5 FAILS idle-quiet as designed (x" + fmt (dl / 0.0005, 1)
                                             + "): lfo moved by " + fmt (dl, 3) + " (3 dp quantum 0.001)");
        else
            report ("G-VIZ-SILENT-NC", false, "depth 0.5 hash did NOT change - the idle-quiet gate is vacuous");
    }

    //==========================================================================
    // G-VIZ-NOTE (P5, D-X): poly note + bend; Mono legato / return / release.
    void gateVizNote()
    {
        constexpr double fs = 48000.0;
        auto hzOf = [] (int n) { return 440.0 * std::pow (2.0, (double) (n - 69) / 12.0); };

        // Poly.
        {
            Rig rig (fs, 512, vizBase (0, 0.3f, true, 0));
            Proc::CycleView v;
            rig.run (2048, { evOn (0, 60) });
            rig.proc->buildCycleView (v);
            const auto a = wireCycle (v);
            rig.run (2048, { evWheel (rig.cursor, 16383) });             // +8191
            rig.proc->buildCycleView (v);
            const auto b = wireCycle (v);

            const double bent = hzOf (60) * std::pow (2.0, (8191.0 / 8192.0 * 2.0) / 12.0);
            const bool ok = a.ok && b.ok && a.sounding && a.note == 60 && std::abs (a.f0 - 261.63) <= 0.01
                         && b.note == 60 && std::abs (b.f0 - 293.66) <= 0.05 && std::abs (b.f0 - bent) <= 0.01
                         && b.f0 > 0.0 && std::abs (b.nyquistH - 0.5 * fs / b.f0) <= 1.0e-2;
            report ("G-VIZ-NOTE[poly]", ok,
                    "note " + std::to_string (a.note) + " f0 " + fmt (a.f0, 3) + " (want 60 / 261.63 +/- 0.01); wheel +8191: note "
                    + std::to_string (b.note) + " f0 " + fmt (b.f0, 3) + " (want 293.66 +/- 0.05), nyquistH "
                    + fmt (b.nyquistH, 3) + " (want " + fmt (b.f0 > 0.0 ? 0.5 * fs / b.f0 : 0.0, 3) + ")"
                    + (a.ok && b.ok ? std::string() : " | wire: " + a.why + b.why));
        }

        // Mono: hold 60, legato 64, release 64 (back to 60), release all.
        {
            auto prm = vizBase (0, 0.3f, true, 0);
            prm.emplace_back (ids::voiceMode, 1.0f);
            Rig rig (fs, 512, prm);
            Proc::CycleView v;
            std::vector<WireCycle> steps;

            rig.run (2048, { evOn (0, 60) });
            rig.proc->buildCycleView (v);
            steps.push_back (wireCycle (v));
            rig.run (2048, { evOn (rig.cursor, 64) });
            rig.proc->buildCycleView (v);
            steps.push_back (wireCycle (v));
            rig.run (2048, { evOff (rig.cursor, 64) });
            rig.proc->buildCycleView (v);
            steps.push_back (wireCycle (v));
            rig.run (2048 + (int) (0.5 * fs), { evOff (rig.cursor, 60) });   // amp release 0.05 s, then idle
            rig.proc->buildCycleView (v);
            steps.push_back (wireCycle (v));

            const int want[] = { 60, 64, 60, -1 };
            bool ok = true;
            std::string d;
            for (size_t i = 0; i < steps.size(); ++i)
            {
                const auto& s = steps[i];
                const bool sounding = want[i] >= 0;
                const bool f0ok = sounding ? std::abs (s.f0 - hzOf (want[i])) <= 0.01 : std::abs (s.f0) <= 0.0;
                const bool stepOk = s.ok && s.note == want[i] && s.sounding == sounding && f0ok
                                 && (i == 0 || s.note != steps[i - 1].note);
                ok = ok && stepOk;
                d += (i == 0 ? "" : " -> ") + std::to_string (s.note) + (s.sounding ? "" : " (silent)")
                   + " f0 " + fmt (s.f0, 2) + (stepOk ? "" : " BAD");
            }
            report ("G-VIZ-NOTE[mono]", ok, d + " (want 60 -> 64 legato -> 60 return -> -1 silent after the tail)");
        }
    }

    //==========================================================================
    // G-VIZ-THUMBS: the five built-in banks, thumbnails + wire.
    void gateVizThumbs()
    {
        Rig rig (48000.0, 512, vizBase (0, 0.0f, true, 0));
        Proc& p = *rig.proc;
        Proc::BankThumbs t;
        bool ok = true;
        double worst = 0.0;
        std::string why;
        for (int b = 0; b < BuiltInBanks::kCount; ++b)
        {
            rig.set (ids::bank, (float) b);
            p.getBankThumbnails (t);
            const auto* bank = p.getBuiltInBanks().get (b);
            const auto w = wireBank (t);
            const double err = w.ok ? thumbWireError (w, bank->thumbs) : 1.0e9;
            worst = std::max (worst, err);
            const bool bOk = t.bank == b && ! t.imported && t.numFrames == BuiltInBanks::kFrames && t.filename.isEmpty()
                          && sameFloats (t.points, bank->thumbs) && w.ok && w.bank == b && ! w.imported
                          && w.numFrames == BuiltInBanks::kFrames && (int) w.frames.size() == BuiltInBanks::kFrames
                          && w.filename.isEmpty() && err <= kTolThumb;
            if (! bOk)
                why += "bank " + std::to_string (b) + " BAD (" + w.why + "); ";
            ok = ok && bOk;
        }
        report ("G-VIZ-THUMBS", ok, "banks 0..4: 32 frames, memcmp-equal to bank.thumbs, no filename; wire 32 x 128, worst |err| "
                                    + sci (worst) + " (tol 5e-4)" + (why.empty() ? std::string() : " | " + why));
    }

    //==========================================================================
    // G-VIZ-IMPORTED (UI-01, P8, D-S): the REAL import path (the test publish
    // never sets cachedBlob, so it cannot prove the filename source).
    juce::MemoryBlock makeWav16 (const std::vector<std::int16_t>& pcm)
    {
        juce::MemoryBlock mb;
        {
            std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::MemoryOutputStream> (mb, false);
            juce::WavAudioFormat wav;
            auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (48000.0)
                                                  .withNumChannels (1).withBitsPerSample (16));
            if (w == nullptr)
                return {};
            std::vector<int> wide (pcm.size());
            for (size_t i = 0; i < pcm.size(); ++i)
                wide[i] = (int) pcm[i] * 65536;
            const int* ch[] = { wide.data(), nullptr };
            if (! w->write (ch, (int) wide.size()))
                return {};
        }   // the writer finalises the header and releases the stream here
        return mb;
    }

    // importFromMemory, poll the status until it settles (<= 10 s; no message
    // loop in the harness), then apply the pending auto-select (D-M).
    bool importMemAndWait (Proc& p, const juce::String& name, juce::MemoryBlock bytes, Proc::ImportStatus& st)
    {
        if (! p.importFromMemory (name, std::move (bytes)))
        {
            st = p.getImportStatus();
            return false;
        }
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        for (;;)
        {
            st = p.getImportStatus();
            if (st.state == Proc::ImportStatus::State::done || st.state == Proc::ImportStatus::State::error)
                break;
            if (juce::Time::getMillisecondCounterHiRes() - t0 > 10000.0)
                break;
            juce::Thread::sleep (2);
        }
        p.handleUpdateNowIfNeeded();
        return st.state == Proc::ImportStatus::State::done;
    }

    void gateVizImported()
    {
        Rig rig (48000.0, 512, vizBase (0, 0.5f, true, 0));
        Proc& p = *rig.proc;

        // 1. 3 x 2048 samples, frame f = a sine at harmonic f + 1.
        std::vector<std::int16_t> pcm ((std::size_t) (3 * kTable));
        for (int f = 0; f < 3; ++f)
            for (int i = 0; i < kTable; ++i)
                pcm[(std::size_t) (f * kTable + i)] = (std::int16_t) std::lround (
                    20000.0 * std::sin (2.0 * kPi * (double) (f + 1) * (double) i / (double) kTable));
        Proc::ImportStatus st;
        const bool done = importMemAndWait (p, "three frames.wav", makeWav16 (pcm), st);
        const auto snap = p.getImportedBankSnapshot();

        Proc::BankThumbs t;
        p.getBankThumbnails (t);
        const auto wb = wireBank (t);
        const double thumbErr = (wb.ok && snap != nullptr) ? thumbWireError (wb, snap->thumbs) : 1.0e9;
        const bool thumbsOk = done && snap != nullptr && p.getSelectedBankIndex() == Proc::kImportedIdx
                           && t.bank == Proc::kImportedIdx && t.imported && t.numFrames == 3
                           && t.filename == "three frames.wav" && sameFloats (t.points, snap->thumbs)
                           && wb.ok && wb.bank == Proc::kImportedIdx && wb.imported && wb.numFrames == 3
                           && wb.frames.size() == 3 && wb.filename == "three frames.wav" && thumbErr <= kTolThumb;

        Proc::CycleView v;
        p.buildCycleView (v);
        const auto wc = wireCycle (v);
        double cycErr = 1.0e9;
        if (wc.ok && snap != nullptr)
        {
            cycErr = 0.0;
            const int latched = wt::latchFrame (0.5f, snap->numFrames);
            for (int i = 0; i < kPts; ++i)
            {
                const double want = (double) wt::readSample (*snap, 0, (double) (i * (kTable / kPts)) / (double) kTable,
                                                             true, 0.5f, latched);
                cycErr = std::max (cycErr, std::abs (wc.cycle[(std::size_t) i] - want));
            }
        }
        const bool cycleOk = wc.ok && ! v.empty && ! wc.sounding && wc.level == 0 && wc.frame == -1
                          && std::abs (wc.pos - 0.5) <= 1.0e-4 && cycErr <= 1.0e-4;

        report ("G-VIZ-IMPORTED[import]", thumbsOk && cycleOk,
                std::string ("import ") + (done ? "done" : "NOT done") + ", bank " + std::to_string (p.getSelectedBankIndex())
                + " (want 5), thumbs imported " + (t.imported ? "yes" : "no") + ", " + std::to_string (t.numFrames)
                + " frames, \"" + t.filename.toStdString() + "\", points " + (snap != nullptr && sameFloats (t.points, snap->thumbs) ? "memcmp-equal" : "DIFFER")
                + ", wire |err| " + sci (thumbErr) + " (tol 5e-4); cycle (level 0, pos 0.5, Interp On) vs readSample on the snapshot |err| "
                + sci (cycErr) + " (tol 1e-4)" + (wb.ok ? std::string() : " | bank wire: " + wb.why)
                + (wc.ok ? std::string() : " | cycle wire: " + wc.why));

        // 2. P8: a failed import names itself on the status, NOT on the bank.
        {
            const auto hashBefore = viz::bankHash (t);
            juce::MemoryBlock junk (4096);
            auto* bytes = static_cast<unsigned char*> (junk.getData());
            for (size_t i = 0; i < junk.getSize(); ++i)
                bytes[i] = static_cast<unsigned char> ((i * 131u + 7u) & 0xffu);   // import-check's garbage
            Proc::ImportStatus bad;
            importMemAndWait (p, "bad.wav", std::move (junk), bad);
            Proc::BankThumbs t2;
            p.getBankThumbnails (t2);
            const auto w2 = wireBank (t2);
            const bool ok = bad.state == Proc::ImportStatus::State::error && bad.error == "unreadable"
                         && bad.filename == "bad.wav" && t2.filename == "three frames.wav" && t2.numFrames == 3
                         && w2.ok && w2.filename == "three frames.wav" && w2.numFrames == 3
                         && p.getImportedBankSnapshot() == snap && viz::bankHash (t2) == hashBefore;
            report ("G-VIZ-IMPORTED[P8]", ok,
                    "status " + std::string (viz::importStateName (bad.state)) + " / " + bad.error.toStdString() + " / \""
                    + bad.filename.toStdString() + "\" (want error / unreadable / \"bad.wav\"); thumbnails still \""
                    + t2.filename.toStdString() + "\", " + std::to_string (t2.numFrames) + " frames; bankHash "
                    + (viz::bankHash (t2) == hashBefore ? "unchanged (no resend)" : "CHANGED"));
        }

        // 3. Empty Imported: a fresh instance's state (no IMPORTED_BANK), then bank 5.
        {
            juce::MemoryBlock fresh;
            {
                Proc q;
                q.getStateInformation (fresh);
            }
            p.setStateInformation (fresh.getData(), (int) fresh.getSize());
            rig.set (ids::bank, (float) Proc::kImportedIdx);

            Proc::BankThumbs t3;
            p.getBankThumbnails (t3);
            const auto w3 = wireBank (t3);
            Proc::CycleView e;
            p.buildCycleView (e);
            const auto we = wireCycle (e);

            bool zeros = we.ok, floors = we.ok;
            for (const double s : we.cycle)
                zeros = zeros && std::abs (s) <= 0.0;
            for (const double h : we.harmonics)
                floors = floors && std::abs (h - kFloorDb) <= 1.0e-9;
            const bool ok = p.getImportedBankSnapshot() == nullptr && t3.imported && t3.numFrames == 0
                         && t3.filename.isEmpty() && t3.points.empty() && w3.ok && w3.imported && w3.numFrames == 0
                         && w3.frames.empty() && w3.filename.isEmpty() && e.empty && zeros && floors && ! we.sounding;
            report ("G-VIZ-IMPORTED[empty]", ok,
                    std::string ("thumbs imported ") + (t3.imported ? "yes" : "no") + ", " + std::to_string (t3.numFrames)
                    + " frames, filename \"" + t3.filename.toStdString() + "\" (want yes / 0 / \"\"); cycle view empty "
                    + (e.empty ? "yes" : "no") + ", cycle all 0 " + (zeros ? "yes" : "no") + ", bars all -60 "
                    + (floors ? "yes" : "no") + ", sounding " + (we.sounding ? "true" : "false")
                    + (w3.ok ? std::string() : " | bank wire: " + w3.why) + (we.ok ? std::string() : " | cycle wire: " + we.why));
        }
    }

    //==========================================================================
    // G-VIZ-ALLOC (a): the audio thread (this one) runs the stimulus while a
    // second thread loops the full viz path; 0 audio-thread allocations.
    void gateVizAllocConcurrent()
    {
        auto impA = makeImportedBank (3);
        auto impB = makeImportedBank (5);
        auto impC = makeImportedBank (2);

        auto prm = vizBase (0, 0.3f, true, 0);
        prm.emplace_back (ids::lfoSync, 0.0f);
        prm.emplace_back (ids::lfoShape, 4.0f);   // S&H
        prm.emplace_back (ids::lfoRate, 7.0f);
        prm.emplace_back (ids::lfoDepth, 1.0f);
        Rig rig (48000.0, 512, prm);
        Proc& p = *rig.proc;

        std::atomic<bool> stop { false };
        std::atomic<long> iterations { 0 };
        std::thread vizThread ([&p, &stop, &iterations]
        {
            Proc::CycleView cv;
            Proc::BankThumbs bt;
            bt.points.reserve ((std::size_t) WavetableBank::kMaxFrames * (std::size_t) WavetableBank::kThumbSize);
            while (! stop.load())
            {
                p.buildCycleView (cv);
                p.getBankThumbnails (bt);
                const auto json = juce::JSON::toString (viz::cycleToVar (cv), true);
                juce::ignoreUnused (json);
                iterations.fetch_add (1);
            }
        });

        const bool live1 = installAllocHook (true);
        if (! live1)
        {
            stop.store (true);
            vizThread.join();
            report ("G-VIZ-ALLOC(a)", false, "the malloc hook is not live (a deliberate malloc(64) did not count 1)");
            return;
        }

        gAllocMode = true;
        rig.run (512);                                                        // warm-up (unarmed)

        std::vector<Ev> evs;
        for (int i = 0; i < 16; ++i)
            evs.push_back (evOn (rig.cursor + i * 8, 48 + i));
        rig.run (4096, evs);                                                  // 16-note chord
        evs.clear();
        for (int i = 0; i < 16; ++i)
            evs.push_back (evOff (rig.cursor + i * 8, 48 + i));
        rig.run (4096, evs);

        rig.set (ids::voiceMode, 1.0f);                                       // Mono legato run
        evs.clear();
        evs.push_back (evOn (rig.cursor + 10, 60));
        evs.push_back (evOn (rig.cursor + 300, 64));
        evs.push_back (evOn (rig.cursor + 600, 67));
        evs.push_back (evOff (rig.cursor + 900, 67));
        evs.push_back (evWheel (rig.cursor + 1000, 12000));
        evs.push_back (evOff (rig.cursor + 1200, 64));
        evs.push_back (evOff (rig.cursor + 1500, 60));
        rig.run (4096, evs);
        rig.set (ids::voiceMode, 0.0f);

        for (int b = 0; b < Proc::kNumBanks; ++b)                             // bank cycling 0..5
        {
            if (b == Proc::kImportedIdx)
                p.publishImportedBankForTesting (impA);                       // from main, between blocks
            rig.set (ids::bank, (float) b);
            evs.clear();
            evs.push_back (evOn (rig.cursor + 5, 60 + b));
            evs.push_back (evOff (rig.cursor + 1500, 60 + b));
            rig.run (2048, evs);
        }
        p.publishImportedBankForTesting (impB);
        rig.run (2048, { evOn (rig.cursor + 3, 72) });
        p.publishImportedBankForTesting (nullptr);
        rig.run (2048);
        p.publishImportedBankForTesting (impC);
        rig.run (2048, { evOff (rig.cursor + 3, 72) });
        rig.run ((int) (0.3 * 48000.0));                                      // release tails

        gAllocMode = false;
        const int n = allocCountNow();
        const int armedBlocks = rig.blocksRun - 1;

        stop.store (true);
        vizThread.join();

        // Liveness after the run: the hook still counts on this thread.
        resetAllocCount();
        armAlloc (true);
        void* volatile probe = std::malloc (64);
        armAlloc (false);
        std::free (probe);
        const int live2 = allocCountNow();
        removeAllocHook();
        resetAllocCount();

        const long iters = iterations.load();
        report ("G-VIZ-ALLOC(a)", n == 0 && live2 == 1 && iters > 0 && armedBlocks > 0,
                std::to_string (n) + " audio-thread allocation(s) over " + std::to_string (armedBlocks)
                + " armed blocks (16-note chord, Mono legato run, LFO S&H depth 1, bank 0..5, Imported published "
                "between blocks) while a second thread ran " + std::to_string (iters)
                + " x (buildCycleView + getBankThumbnails + cycleToVar + JSON::toString) (want 0); liveness malloc(64) "
                "counted 1 before and " + std::to_string (live2) + " after");
    }

    // G-VIZ-ALLOC (b): an idle tick on THIS thread, no render running: 1000 x
    // (buildCycleView + cycleHash) for bank 0 and for a non-empty Imported bank.
    void gateVizAllocIdle()
    {
        Rig rig (48000.0, 512, vizBase (0, 0.37f, true, 9));   // 8 bits: the preQ path too
        Proc& p = *rig.proc;
        rig.run (2048, { evOn (0, 60) });                       // a held note: level > 0 -> ghost render too
        auto imported = makeImportedBank (4);

        if (! installAllocHook (true))
        {
            report ("G-VIZ-ALLOC(b)", false, "the malloc hook is not live (a deliberate malloc(64) did not count 1)");
            return;
        }

        Proc::CycleView v;
        juce::uint64 acc = 0;
        auto measure = [&p, &v, &acc]
        {
            p.buildCycleView (v);                                // warm-up (unarmed)
            acc ^= viz::cycleHash (v);
            resetAllocCount();
            armAlloc (true);
            for (int i = 0; i < 1000; ++i)
            {
                p.buildCycleView (v);
                acc ^= viz::cycleHash (v) + (juce::uint64) i;
            }
            armAlloc (false);
            return allocCountNow();
        };

        const int nBuiltIn = measure();
        const bool soundingBuiltIn = v.sounding && v.level > 0 && v.quantized;

        removeAllocHook();                                       // the publish + render below allocate (unarmed anyway)
        p.publishImportedBankForTesting (imported);
        rig.set (ids::bank, (float) Proc::kImportedIdx);
        rig.run (2048);
        const bool live2 = installAllocHook (true);
        const int nImported = live2 ? measure() : -1;
        const bool importedLive = ! v.empty && v.sounding;
        removeAllocHook();
        resetAllocCount();

        report ("G-VIZ-ALLOC(b)", nBuiltIn == 0 && nImported == 0 && live2 && soundingBuiltIn && importedLive,
                std::to_string (nBuiltIn) + " allocation(s) over 1000 x (buildCycleView + cycleHash) on bank 0 (sounding, level "
                "> 0, 8-bit), " + std::to_string (nImported) + " on a 4-frame Imported bank (want 0 / 0); liveness malloc(64) "
                "counted 1 before each run; hash fold " + std::to_string (acc % 1000u));
    }

    //==========================================================================
    // G-VIZ-TIME (log only: no wall-clock verdict).
    void logVizTime()
    {
        Rig rig (48000.0, 512, vizBase (0, 0.37f, true, 9));
        Proc& p = *rig.proc;
        rig.run (2048, { evOn (0, 60) });

        Proc::CycleView v;
        std::size_t cycleBytes = 0;
        juce::uint64 acc = 0;
        const auto t0 = juce::Time::getHighResolutionTicks();
        for (int i = 0; i < 1000; ++i)
        {
            p.buildCycleView (v);
            acc ^= viz::cycleHash (v);
            cycleBytes = juce::JSON::toString (viz::cycleToVar (v), true).getNumBytesAsUTF8();
        }
        const double usCycle = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) * 1.0e6 / 1000.0;

        auto big = makeImportedBank (WavetableBank::kMaxFrames);
        std::size_t bankBytes = 0;
        double usBank = 0.0;
        if (big != nullptr)
        {
            p.publishImportedBankForTesting (big);
            rig.set (ids::bank, (float) Proc::kImportedIdx);
            Proc::BankThumbs t;
            t.points.reserve ((std::size_t) WavetableBank::kMaxFrames * (std::size_t) WavetableBank::kThumbSize);
            const auto t1 = juce::Time::getHighResolutionTicks();
            for (int i = 0; i < 20; ++i)
            {
                p.getBankThumbnails (t);
                acc ^= viz::bankHash (t);
                bankBytes = juce::JSON::toString (viz::bankToVar (t), true).getNumBytesAsUTF8();
            }
            usBank = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t1) * 1.0e6 / 20.0;
        }

        std::cout << "LOG  G-VIZ-TIME (no verdict): cycleUpdate build + hash + var + JSON::toString "
                  << fmt (usCycle, 1) << " us/tick, " << cycleBytes << " bytes (8-bit, sounding); bankUpdate N = 256 "
                  << "thumbs + hash + var + JSON::toString " << fmt (usBank, 1) << " us, " << bankBytes
                  << " bytes; budget 33333 us/tick at 30 Hz (fold " << (acc % 1000u) << ")\n";
    }

    //==========================================================================
    // Task 14: G-DROP / G-IMPORT-ERR (UI-04, D-Z, P8).

    // numSamples of int16: frame f (2048 samples each) = a sine at harmonic
    // f + 1 (G-VIZ-IMPORTED's content; a partial last frame continues it).
    std::vector<std::int16_t> harmonicPcm (int numSamples)
    {
        std::vector<std::int16_t> pcm ((std::size_t) juce::jmax (0, numSamples));
        for (int i = 0; i < numSamples; ++i)
        {
            const int f = i / kTable;
            const int k = i % kTable;
            pcm[(std::size_t) i] = (std::int16_t) std::lround (
                20000.0 * std::sin (2.0 * kPi * (double) (f + 1) * (double) k / (double) kTable));
        }
        return pcm;
    }

    // Polls the status until it leaves busy (<= 10 s; no message loop in the
    // harness), then applies the pending auto-select (D-M).
    void waitImportSettled (Proc& p, Proc::ImportStatus& st)
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        for (;;)
        {
            st = p.getImportStatus();
            if (st.state == Proc::ImportStatus::State::done || st.state == Proc::ImportStatus::State::error)
                break;
            if (juce::Time::getMillisecondCounterHiRes() - t0 > 10000.0)
                break;
            juce::Thread::sleep (2);
        }
        p.handleUpdateNowIfNeeded();
    }

    // Number of the 11 mip levels that are memcmp-equal between two banks
    // (0 when the shapes differ).
    int equalMipLevels (const WavetableBank* a, const WavetableBank* b)
    {
        if (a == nullptr || b == nullptr || a->numFrames <= 0 || a->numFrames != b->numFrames)
            return 0;
        const std::size_t per = (std::size_t) a->numFrames * (std::size_t) WavetableBank::kStride;
        if (a->data.size() != per * (std::size_t) WavetableBank::kLevels || b->data.size() != a->data.size())
            return 0;
        int n = 0;
        for (int l = 0; l < WavetableBank::kLevels; ++l)
            if (std::memcmp (a->data.data() + (std::size_t) l * per, b->data.data() + (std::size_t) l * per,
                             per * sizeof (float)) == 0)
                ++n;
        return n;
    }

    // true = the name holds a control character (< 0x20, 0x7f) or a path separator.
    bool hasControlOrSeparator (const juce::String& s)
    {
        for (auto cp = s.getCharPointer(); ! cp.isEmpty();)
        {
            const juce::juce_wchar c = cp.getAndAdvance();
            if (c < 0x20 || c == 0x7f || c == '/' || c == '\\')
                return true;
        }
        return false;
    }

    // Printable form for the report lines (control characters as \xNN).
    std::string printable (const juce::String& s)
    {
        std::string out;
        for (auto cp = s.getCharPointer(); ! cp.isEmpty();)
        {
            const juce::juce_wchar c = cp.getAndAdvance();
            if (c >= 0x20 && c < 0x7f)
            {
                out += (char) c;
            }
            else
            {
                std::ostringstream h;
                h << "\\x" << std::hex << std::setw (2) << std::setfill ('0') << (unsigned int) c;
                out += h.str();
            }
        }
        return out;
    }

    std::string statusText (const Proc::ImportStatus& st)
    {
        return std::string (viz::importStateName (st.state)) + " / " + (st.error.isEmpty() ? std::string ("-") : st.error.toStdString())
             + " / \"" + printable (st.filename) + "\" / " + std::to_string (st.frames) + " frames";
    }

    void gateDrop()
    {
        using S = Proc::ImportStatus::State;

        Rig rig (48000.0, 512, vizBase (0, 0.5f, true, 0));
        Proc& p = *rig.proc;

        const juce::MemoryBlock wav = makeWav16 (harmonicPcm (3 * kTable));
        const juce::String b64 = juce::Base64::toBase64 (wav.getData(), wav.getSize());   // standard alphabet (btoa)

        // (a) The drop path end to end vs importFromMemory of the same bytes.
        const bool accepted = p.importFromBase64 ("drop me.wav", b64);
        Proc::ImportStatus st;
        waitImportSettled (p, st);
        const auto snap = p.getImportedBankSnapshot();
        Proc::BankThumbs t;
        p.getBankThumbnails (t);

        Rig refRig (48000.0, 512, vizBase (0, 0.5f, true, 0));
        Proc::ImportStatus refSt;
        const bool refDone = importMemAndWait (*refRig.proc, "drop me.wav", juce::MemoryBlock (wav), refSt);
        const auto refSnap = refRig.proc->getImportedBankSnapshot();
        const int levels = equalMipLevels (snap.get(), refSnap.get());
        const bool thumbsEq = snap != nullptr && refSnap != nullptr && sameFloats (snap->thumbs, refSnap->thumbs);

        const bool okA = wav.getSize() > 0 && accepted && st.state == S::done && st.filename == "drop me.wav"
                      && st.frames == 3 && snap != nullptr && p.getSelectedBankIndex() == Proc::kImportedIdx
                      && t.bank == Proc::kImportedIdx && t.imported && t.numFrames == 3 && t.filename == "drop me.wav"
                      && sameFloats (t.points, snap->thumbs) && refDone && levels == WavetableBank::kLevels && thumbsEq;
        report ("G-DROP[a]", okA,
                std::to_string (wav.getSize()) + " WAV bytes -> " + std::to_string (b64.length())
                + " standard base64 chars; importFromBase64 " + (accepted ? "true" : "FALSE") + ", status " + statusText (st)
                + "; bank " + std::to_string (p.getSelectedBankIndex()) + " (want 5), thumbnails \"" + printable (t.filename)
                + "\" / " + std::to_string (t.numFrames) + " frames; " + std::to_string (levels)
                + "/11 mip levels memcmp-equal to importFromMemory (fresh processor, " + (refDone ? "done" : "NOT done")
                + "), thumbs " + (thumbsEq ? "equal" : "DIFFER"));

        // (b) One char over the cap: refused BEFORE decoding.
        const auto before = p.getImportedBankSnapshot();
        {
            const std::size_t cap = WavetableImporter::kMaxMemoryBytes / 3 * 4 + 4;
            const juce::String big = juce::String::repeatedString ("A", (int) (cap + 1));
            const bool r = p.importFromBase64 ("big.wav", big);
            const auto sb = p.getImportStatus();
            const bool okB = (std::size_t) big.getNumBytesAsUTF8() == cap + 1 && ! r && sb.state == S::error
                          && sb.error == "tooLarge" && sb.filename == "big.wav"
                          && p.getImportedBankSnapshot() == before && before != nullptr
                          && p.getSelectedBankIndex() == Proc::kImportedIdx;
            report ("G-DROP[b]", okB,
                    std::to_string (cap + 1) + " chars (cap " + std::to_string (cap) + ") -> " + (r ? "TRUE" : "false")
                    + ", status " + statusText (sb) + " (want error / tooLarge / \"big.wav\"), snapshot "
                    + (p.getImportedBankSnapshot() == before ? "unchanged" : "CHANGED"));
        }

        // (c) Not base64 at all.
        {
            const bool r = p.importFromBase64 ("at.wav", "@@@@");
            const auto sc = p.getImportStatus();
            const bool okC = ! r && sc.state == S::error && sc.error == "unreadable" && sc.filename == "at.wav"
                          && p.getImportedBankSnapshot() == before && before != nullptr;
            report ("G-DROP[c]", okC,
                    std::string ("\"@@@@\" -> ") + (r ? "TRUE" : "false") + ", status " + statusText (sc)
                    + " (want error / unreadable / \"at.wav\"), snapshot "
                    + (p.getImportedBankSnapshot() == before ? "unchanged" : "CHANGED"));
        }

        // (d) A hostile name: basename only, control characters removed.
        {
            const juce::String hostile ("../../x\n.wav");
            const bool r = p.importFromBase64 (hostile, b64);
            Proc::ImportStatus sd;
            waitImportSettled (p, sd);
            Proc::BankThumbs td;
            p.getBankThumbnails (td);
            const auto wd = wireBank (td);
            const bool okD = r && sd.state == S::done && sd.filename == "x.wav" && ! hasControlOrSeparator (sd.filename)
                          && td.imported && td.filename == "x.wav" && ! hasControlOrSeparator (td.filename)
                          && wd.ok && wd.filename == "x.wav";
            report ("G-DROP[d]", okD,
                    "name \"" + printable (hostile) + "\" -> status \"" + printable (sd.filename) + "\" (" + statusText (sd)
                    + "), thumbnails \"" + printable (td.filename) + "\", wire \"" + printable (wd.filename)
                    + "\" (want \"x.wav\", no control chars / separators)" + (wd.ok ? std::string() : " | bank wire: " + wd.why));
        }

        // Negative control: the same bytes in JUCE's own MemoryBlock base64
        // ("<size>.<chars>", a different alphabet) must NOT import.
        {
            const auto snapNc = p.getImportedBankSnapshot();
            const juce::String juceEnc = wav.toBase64Encoding();
            const bool r = p.importFromBase64 ("nc.wav", juceEnc);
            Proc::ImportStatus sn;
            if (r)
                waitImportSettled (p, sn);
            else
                sn = p.getImportStatus();
            const bool unchanged = p.getImportedBankSnapshot() == snapNc && snapNc != nullptr;
            const bool rejected = juceEnc != b64 && unchanged
                               && sn.state == S::error && (! r || sn.error == "unreadable");
            if (rejected)
                report ("G-DROP-N1", true,
                        "MemoryBlock::toBase64Encoding payload FAILS as designed: importFromBase64 "
                        + std::string (r ? "true" : "false") + ", status " + statusText (sn) + ", snapshot unchanged");
            else
                report ("G-DROP-N1", false,
                        "the non-standard payload was NOT rejected (importFromBase64 " + std::string (r ? "true" : "false")
                        + ", status " + statusText (sn) + ", snapshot " + (unchanged ? "unchanged" : "CHANGED")
                        + "): the gate cannot tell the alphabets apart");
        }
    }

    // Every error code PluginProcessor.cpp can put on the status: the last
    // string literal of each setImportStatus / importStatus = / finishEmpty
    // line, plus WavetableImporter::errorCode of every ImportError the worker
    // can surface (cancelled only if the source no longer filters it).
    bool scanEmittedCodes (std::set<std::string>& codes, int& literalLines, std::string& why)
    {
        const juce::String here (__FILE__);
        if (! juce::File::isAbsolutePath (here))
        {
            why += "__FILE__ is not absolute (" + here.toStdString() + "); ";
            return false;
        }
        const auto cpp = juce::File (here).getParentDirectory().getParentDirectory().getParentDirectory()
                             .getChildFile ("Source").getChildFile ("PluginProcessor.cpp");
        if (! cpp.existsAsFile())
        {
            why += "cannot read " + cpp.getFullPathName().toStdString() + "; ";
            return false;
        }

        const juce::String text = cpp.loadFileAsString();
        juce::StringArray lines;
        lines.addLines (text);

        bool usesErrorCode = false;
        for (const auto& raw : lines)
        {
            const auto line = raw.upToFirstOccurrenceOf ("//", false, false);
            if (line.contains ("WavetableImporter::errorCode ("))
                usesErrorCode = true;
            if (! (line.contains ("setImportStatus (") || line.contains ("importStatus = ") || line.contains ("finishEmpty (")))
                continue;
            const int closeQ = line.lastIndexOfChar ('"');
            if (closeQ <= 0)
                continue;
            const int openQ = line.substring (0, closeQ).lastIndexOfChar ('"');
            if (openQ < 0)
                continue;
            codes.insert (line.substring (openQ + 1, closeQ).toStdString());
            ++literalLines;
        }

        if (usesErrorCode)
        {
            for (const auto e : { ImportError::unreadable, ImportError::tooShort, ImportError::tooLarge })
                codes.insert (WavetableImporter::errorCode (e));
            if (! text.contains ("res.error == ImportError::cancelled"))
                codes.insert (WavetableImporter::errorCode (ImportError::cancelled));   // no longer filtered: surfaced
        }
        return true;
    }

    void gateImportErr()
    {
        using S = Proc::ImportStatus::State;

        Rig rig (48000.0, 512, vizBase (0, 0.5f, true, 0));
        Proc& p = *rig.proc;

        // A good 2-frame import first, then a too-short one.
        Proc::ImportStatus good;
        const bool goodDone = importMemAndWait (p, "keep.wav", makeWav16 (harmonicPcm (2 * kTable)), good);
        Proc::BankThumbs before;
        p.getBankThumbnails (before);
        const auto snapBefore = p.getImportedBankSnapshot();
        const int bankBefore = p.getSelectedBankIndex();

        Proc::ImportStatus st;
        const bool shortDone = importMemAndWait (p, "short.wav", makeWav16 (harmonicPcm (kTable - 1)), st);
        Proc::BankThumbs after;
        p.getBankThumbnails (after);
        const auto wa = wireBank (after);

        const bool okShort = goodDone && bankBefore == Proc::kImportedIdx && before.filename == "keep.wav"
                          && before.numFrames == 2 && snapBefore != nullptr
                          && ! shortDone && st.state == S::error && st.error == "tooShort" && st.filename == "short.wav"
                          && st.frames == 0 && p.getSelectedBankIndex() == bankBefore
                          && after.filename == "keep.wav" && after.numFrames == 2 && sameFloats (after.points, before.points)
                          && wa.ok && wa.filename == "keep.wav" && p.getImportedBankSnapshot() == snapBefore;
        report ("G-IMPORT-ERR[tooShort]", okShort,
                std::to_string (kTable - 1) + " samples -> status " + statusText (st)
                + " (want error / tooShort / \"short.wav\"); bank " + std::to_string (p.getSelectedBankIndex())
                + " (was " + std::to_string (bankBefore) + "), thumbnails \"" + printable (after.filename) + "\" / "
                + std::to_string (after.numFrames) + " frames (want \"keep.wav\" / 2), snapshot "
                + (p.getImportedBankSnapshot() == snapBefore ? "unchanged" : "CHANGED")
                + (wa.ok ? std::string() : " | bank wire: " + wa.why));

        // importToVar: exactly 4 keys, on an error and on a done status.
        {
            std::string why;
            bool ok = true;
            for (const auto* s : { &st, &good })
            {
                juce::var parsed;
                std::size_t bytes = 0;
                const bool wireOk = throughWire (viz::importToVar (*s), parsed, bytes, why)
                                 && exactKeys (parsed, { "state", "filename", "frames", "error" }, why);
                const bool valuesOk = wireOk
                    && parsed.getProperty ("state", juce::var()).toString() == juce::String (viz::importStateName (s->state))
                    && parsed.getProperty ("filename", juce::var()).toString() == s->filename
                    && parsed.getProperty ("error", juce::var()).toString() == s->error
                    && parsed.getProperty ("frames", juce::var()).isInt()
                    && (int) parsed.getProperty ("frames", juce::var()) == s->frames;
                if (wireOk && ! valuesOk)
                    why += "values do not round-trip; ";
                ok = ok && valuesOk;
            }
            report ("G-IMPORT-ERR[keys]", ok,
                    "importToVar keys exactly {error,filename,frames,state} for the error and the done status, values round-trip"
                    + (why.empty() ? std::string() : " | " + why));
        }

        // The error vocabulary the page localizes (PluginProcessor.h ImportStatus::error).
        {
            const std::set<std::string> allowed { "tooShort", "unreadable", "tooLarge", "unsupported" };
            std::set<std::string> codes;
            int literalLines = 0;
            std::string why;
            const bool scanned = scanEmittedCodes (codes, literalLines, why);
            std::set<std::string> stray;
            for (const auto& c : codes)
                if (allowed.count (c) == 0)
                    stray.insert (c.empty() ? std::string ("<empty>") : c);
            const bool ok = scanned && literalLines > 0 && ! codes.empty() && stray.empty();
            report ("G-IMPORT-ERR[codes]", ok,
                    "PluginProcessor.cpp emits {" + joinKeys (codes) + "} (" + std::to_string (literalLines)
                    + " literal sites + errorCode), all in {tooLarge,tooShort,unreadable,unsupported}"
                    + (stray.empty() ? std::string() : " | STRAY {" + joinKeys (stray) + "}")
                    + (why.empty() ? std::string() : " | " + why));
        }
    }
}

int main (int, char**)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Keep ONE BuiltInBanks alive for the whole run, so short-lived processors
    // share it instead of rebuilding the banks each time.
    juce::SharedResourcePointer<BuiltInBanks> banks;
    std::cout << "viz-check: built-in banks ready (buildMillis " << fmt (banks->buildMillis, 2) << " ms)\n";

    gateLesson();

    gateVizThumbs();
    gateVizExact();
    gateVizExactNegatives();
    gateVizCeil();
    gateVizSilent();
    gateVizNote();
    gateVizImported();
    gateVizAllocConcurrent();
    gateVizAllocIdle();
    logVizTime();

    gateDrop();
    gateImportErr();

    report ("G-FINITE", gNonFinite == 0 && gChannelMismatch == 0,
            std::to_string (gNonFinite) + " non-finite, " + std::to_string (gChannelMismatch)
            + " L != R over " + std::to_string (gSamplesChecked) + " samples");

    std::cout << "viz-check: " << (gFailures == 0 ? std::string ("ALL PASS") : "FAILURES = " + std::to_string (gFailures))
              << "\n";
    return gFailures == 0 ? 0 : 1;
}
