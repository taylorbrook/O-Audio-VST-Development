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

    O-simpleWavetable mod-check - Stage 2.3 gates (RESEARCH 7.6 + PLAN Task 15).

    Every gate renders through the real processor (OSIW_TEST_HOOKS) and has a
    negative control or a liveness check. Every measured value is printed.

      G-LFO-SHAPES   FUNC-05  5 shapes (free 3.7 Hz) vs analytic, |err| <= 1e-6;
                              triangle starts at 0 going up.
                              Neg: Sine vs Triangle analytic > 0.1; S&H vs other
                              seed > 0.1.
      G-LFO-FREE     FUNC-05  0.5 Hz Saw @ 48k: every wrap interval 96000 +/- 1,
                              >= 2 intervals. Neg: 0.6 Hz must miss the window.
      G-LFO-TEMPO    FUNC-05  TestPlayHead 120 / 97.3 BPM x 16 divisions, random
                              blocks 1..4096: block-start phase vs absolute
                              frac(t * bpm / 60 / beats) <= 1e-9; samples <= 1e-6;
                              wrap interval within 1 sample of beats*60/bpm*fs.
                              Neg (comparator): rounded triplet 1.3333f drifts > 1e-9.
      G-LFO-LOOP     FUNC-05  loop jump ppq 2.6 -> 0.25: next block re-locks
                              (phase 0.25 within 1e-9). Liveness: free-run
                              continuation differs by >= 0.1.
      G-LFO-STOPPED  FUNC-05  stopped -> host BPM; no playhead / no BPM / BPM 0
                              -> 120; no PPQ -> host BPM. |err| <= 1e-6.
                              Neg: stopped 97.3 vs the 120 analytic > 0.1.
      G-GLOBAL-PHASE FUNC-05  two voices 0.37 s apart: Interp Off effPos
                              bit-identical; Interp On <= 1e-6 after 20 ms.
                              Liveness: range >= 0.5, and voice 2 did NOT start
                              its LFO at phase 0 (>= 0.1 away).
      G-SH-DET       D-J      byte-identical across two runs and a re-prepare;
                              other seed differs; tempo loop replays the S&H
                              values bitwise (neg: stopped transport does not);
                              8 jobs with unique seeds give 8 distinct renders.
      G-DSP05-CLAMP  DSP-05   0 <= effPos <= 1 on every sample of 8 voices for
                              knob 1 / amt +1, knob 0 / amt -1, Square depth 1.
                              Liveness: max == 1, min == 0, range >= 0.99.
      G-DSP05-ZIPPER DSP-05   knob 0 -> 1 step at a block boundary, Interp On,
                              A2: excess ratio <= 1.5; liveness |A-B| >= 0.25;
                              neg (smootherBypass + knob ramp off) >= 4.
      G-CLICK-SQ     2.3      Square, depth 1, Sine->Saw, pos 0.5, 2 Hz, A2,
      G-CLICK-SH              Interp On, every edge in 0.3..3.3 s: ratio <= 1.5;
                              liveness |A-B| >= 0.25 per edge; neg (smoother
                              bypass) >= 4 per edge. D-N probe: 2-pole result
                              printed (info) next to the shipping verdict.
      G-FUNC06-SWEEP FUNC-06  amt +1 rises 0 -> 1 -> 0.5 following A/D/S; -1
                              falls 1 -> 0 -> 0.5; audio brightness follows.
                              Neg: amt 0 is static.
      G-FUNC06-LIFE  FUNC-06  amp rel 0.05 s, mod rel 10 s: inactive within
                              0.1 s, exact silence after, mod env still > 0.9
                              when the voice ended. Control: amp rel 5 s alive.
      G-FUNC06-REPUSH         per-block param push during release: block 64 vs
                              4096 bit-identical, linear release to 0.5x at
                              +0.25 s. Neg: raw ADSR re-pushed collapses.
      G-BLOCKSIZE    D-L      free mode, partitions 64 / 512 / 4096 / random
                              bit-identical (two configs). Liveness: modulation
                              changes the render; neg: 1-sample note shift differs.
      G-SEEDS                 every auto-assigned render-job seed is unique.
      G-MOD-ALLOC    PERF-01  macOS malloc_logger gate over tempo LFO, S&H, mod
                              env, Mono, oversized + 1-sample blocks (liveness
                              malloc(64) must count 1).
      G-FINITE                every rendered sample finite, L == R.

    Off by default; -DOUARICON_BUILD_TESTS=ON (OSIW_TEST_HOOKS=1).

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "BankFactory.h"
#include "BuiltInBanks.h"
#include "PluginProcessor.h"
#include "PositionLfo.h"
#include "PositionSmoother.h"
#include "WavetableBank.h"
#include "WtRead.h"
#include "WtVoice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

    constexpr double kFs = 48000.0;
    constexpr double kTwoPi = 6.28318530717958647692;
    constexpr std::uint64_t kRunSeed = 0x6D6F642D63686B31ull;   // "mod-chk1"

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

    int secs (double t, double fs = kFs)
    {
        return (int) std::lround (t * fs);
    }

    double frac (double x)
    {
        return x - std::floor (x);
    }

    // Distance on the unit circle (phase error that ignores the 0/1 seam).
    double circDiff (double a, double b)
    {
        const double d = std::abs (frac (a) - frac (b));
        return std::min (d, 1.0 - d);
    }

    bool sameBits (const std::vector<float>& a, const std::vector<float>& b)
    {
        return a.size() == b.size() && std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0;
    }

    long long countDiffering (const std::vector<float>& a, const std::vector<float>& b)
    {
        const size_t n = std::min (a.size(), b.size());
        long long c = (long long) (std::max (a.size(), b.size()) - n);
        for (size_t i = 0; i < n; ++i)
            if (std::memcmp (&a[i], &b[i], sizeof (float)) != 0)
                ++c;
        return c;
    }

    //==========================================================================
    // Seeds: independent transcriptions (not the plugin's code) of splitmix64
    // and of the S&H draw, so the analytic side is not the code under test.
    std::uint64_t splitmix64 (std::uint64_t x)
    {
        x += 0x9E3779B97F4A7C15ull;
        std::uint64_t z = x;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    float shRef (std::uint64_t seed, std::int64_t ci)
    {
        std::uint64_t z = seed ^ ((std::uint64_t) ci * 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        z ^= z >> 31;
        return (float) ((double) (z >> 40) * (1.0 / 16777216.0) * 2.0 - 1.0);
    }

    // Every render job gets its own seed (CONTEXT: no shared seeds). Jobs that
    // are DELIBERATE replays (determinism / partition gates) reuse a seed
    // explicitly and do not call this.
    std::vector<std::uint64_t> gIssuedSeeds;
    std::uint64_t gSeedCounter = 0;

    std::uint64_t nextJobSeed()
    {
        const std::uint64_t s = splitmix64 (kRunSeed + (gSeedCounter++));
        gIssuedSeeds.push_back (s);
        return s;
    }

    // Independent division table (exact doubles).
    const double kBeatsRef[16] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                   3.0, 1.5, 0.75, 0.375,
                                   4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
    const char* kDivNames[16] = { "4 bars", "2 bars", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                  "1/2.", "1/4.", "1/8.", "1/16.", "1/2T", "1/4T", "1/8T", "1/16T" };

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

    Params with (Params p, const char* id, float v)
    {
        p.emplace_back (id, v);   // applied in order: the later entry wins
        return p;
    }

    // Every parameter explicit. Instant amp attack, sustain 1, 0 dB out,
    // modulation off unless a gate turns it on.
    Params modBase (int bank, float pos)
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
    // Synthetic host transport (RESEARCH 2.5).
    struct TestPlayHead final : juce::AudioPlayHead
    {
        double bpm = 120.0, ppq = 0.0;
        bool playing = true, hasPpq = true, hasBpm = true;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            if (hasBpm) p.setBpm (bpm);
            if (hasPpq) p.setPpqPosition (ppq);
            p.setIsPlaying (playing);
            return p;
        }

        void advance (int n, double fs)
        {
            if (playing)
                ppq += (double) n * bpm / (60.0 * fs);
        }
    };

    //==========================================================================
    struct BlockInfo
    {
        int start = 0;
        int len = 0;
        double ppq = 0.0;
        double startPhase = 0.0;
    };

    // Processor rig: params and the S&H seed set BEFORE prepare; events at
    // absolute sample positions from the rig start. Captures require
    // host block <= prepared block (one chunk per block).
    struct Rig
    {
        std::unique_ptr<Proc> proc;
        double fs;
        int block;
        int cursor = 0;
        int blocksRun = 0;
        juce::AudioBuffer<float> buf;
        juce::MidiBuffer midi;
        TestPlayHead* head = nullptr;

        bool captureLfo = false;
        bool captureVoices = false;
        bool captureBlocks = false;
        std::vector<float> lfoTrace;
        std::array<std::vector<float>, (size_t) Proc::kNumVoices> voiceTrace;
        std::vector<BlockInfo> blocks;
        int captureErrors = 0;

        Rig (double sampleRate, int prepBlock, const Params& params, std::uint64_t seed, TestPlayHead* ph = nullptr)
            : proc (std::make_unique<Proc>()), fs (sampleRate), block (prepBlock), buf (2, prepBlock), head (ph)
        {
            for (const auto& pr : params)
                setParam (*proc, pr.first, pr.second);
            proc->setLfoSeedForTesting (seed);
            proc->setPlayHead (ph);
            proc->setPlayConfigDetails (0, 2, fs, prepBlock);
            proc->prepareToPlay (fs, prepBlock);
            midi.ensureSize (16384);
        }

        void set (const char* id, float v) { setParam (*proc, id, v); }

        void reprepare()
        {
            proc->prepareToPlay (fs, block);
            cursor = 0;
        }

        std::vector<float> run (int n, const std::vector<Ev>& evs = {}, const std::function<int()>& nextSize = nullptr)
        {
            std::vector<float> out;
            out.reserve ((size_t) juce::jmax (0, n));

            for (int done = 0; done < n;)
            {
                int len = nextSize ? nextSize() : block;
                len = juce::jlimit (1, n - done, len);
                const bool capturing = captureLfo || captureVoices || captureBlocks;
                if (capturing && len > block)
                    ++captureErrors;

                buf.setSize (2, len, false, false, true);
                for (int ch = 0; ch < 2; ++ch)
                    juce::FloatVectorOperations::fill (buf.getWritePointer (ch), 1.0f, len);   // processBlock must clear

                midi.clear();
                const int t0 = cursor + done;
                for (const auto& e : evs)
                    if (e.pos >= t0 && e.pos < t0 + len)
                        midi.addEvent (e.msg, e.pos - t0);

                BlockInfo bi;
                bi.start = t0;
                bi.len = len;
                bi.ppq = head != nullptr ? head->ppq : 0.0;

                armAlloc (gAllocMode && blocksRun > 0);   // warm-up block unarmed
                proc->processBlock (buf, midi);
                armAlloc (false);
                ++blocksRun;

                if (captureBlocks)
                {
                    bi.startPhase = proc->getLfoForTesting().getBlockStartPhase();
                    blocks.push_back (bi);
                }
                if (captureLfo)
                {
                    const float* l = proc->getLfoForTesting().data();
                    lfoTrace.insert (lfoTrace.end(), l, l + juce::jmin (len, block));
                }
                if (captureVoices)
                {
                    for (int v = 0; v < Proc::kNumVoices; ++v)
                    {
                        const auto& tr = proc->getVoiceForTesting (v)->getEffPosTrace();
                        const int m = juce::jmin (len, (int) tr.size());
                        voiceTrace[(size_t) v].insert (voiceTrace[(size_t) v].end(), tr.begin(), tr.begin() + m);
                    }
                }
                if (head != nullptr)
                    head->advance (len, fs);

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

    std::function<int()> randomSizes (std::uint64_t seed, int maxSize)
    {
        auto rng = std::make_shared<juce::Random> ((juce::int64) (seed & 0x7fffffffffffffffull));
        return [rng, maxSize] { return 1 + rng->nextInt (maxSize); };
    }

    std::vector<int> soundingVoices (const Proc& p)
    {
        std::vector<int> v;
        for (int i = 0; i < Proc::kNumVoices; ++i)
            if (p.getVoiceForTesting (i)->isSounding())
                v.push_back (i);
        std::sort (v.begin(), v.end(), [&p] (int a, int b)
        {
            return p.getVoiceForTesting (a)->getNoteAge() < p.getVoiceForTesting (b)->getNoteAge();
        });
        return v;   // oldest note first
    }

    //==========================================================================
    // Analysis
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

    // RMS of the first difference over RMS: higher = brighter.
    double brightness (const std::vector<float>& x, int from, int to)
    {
        from = juce::jlimit (1, (int) x.size(), from);
        to   = juce::jlimit (from, (int) x.size(), to);
        double s = 0.0;
        for (int i = from; i < to; ++i)
        {
            const double d = (double) x[(size_t) i] - (double) x[(size_t) (i - 1)];
            s += d * d;
        }
        const double r = rms (x, from, to);
        return to > from && r > 0.0 ? std::sqrt (s / (double) (to - from)) / r : 0.0;
    }

    // RESEARCH 3.4 click detector: max |y[n] - 2y[n-1] + y[n-2]| over [a, b).
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

    ClickVerdict clickRatio (const std::vector<float>& y, int t, int xf, double fs)
    {
        const int win = xf + secs (0.002, fs), plat = secs (0.020, fs), gap = secs (0.005, fs);
        const double ev   = maxAbsD2 (y, t - 2, t + win);
        const double pre  = maxAbsD2 (y, t - gap - plat, t - gap);
        const double post = maxAbsD2 (y, t + win + gap, t + win + gap + plat);
        const double ref  = std::max ({ pre, post, 1.0e-9 });
        return { ev / ref, ev, ref };
    }

    //==========================================================================
    // FUNC-05: shapes vs analytic (free mode).
    float shapeRef (int shape, double p, std::uint64_t seed, std::int64_t ci)
    {
        switch (shape)
        {
            case 0: return (float) std::sin (kTwoPi * p);
            case 1: return (float) (p < 0.25 ? 4.0 * p : (p < 0.75 ? 2.0 - 4.0 * p : 4.0 * p - 4.0));
            case 2: return (float) (2.0 * p - 1.0);
            case 3: return p < 0.5 ? 1.0f : -1.0f;
            default: return shRef (seed, ci);
        }
    }

    // Distance from the nearest discontinuity of the shape (phase units).
    double distToEdge (int shape, double p)
    {
        const double toWrap = std::min (p, 1.0 - p);
        if (shape == 2 || shape == 4) return toWrap;
        if (shape == 3) return std::min (toWrap, std::abs (p - 0.5));
        return 1.0;
    }

    void gateLfoShapes()
    {
        const double fs = kFs;
        const int n = secs (1.0, fs);
        const char* names[5] = { "Sine", "Triangle", "Saw", "Square", "S&H" };

        bool ok = true;
        std::string d;
        std::vector<float> sineTrace, shTrace;
        double incUsed = 0.0;
        bool triStart = false;
        std::uint64_t shSeed = 0;

        for (int shape = 0; shape < 5; ++shape)
        {
            const std::uint64_t seed = nextJobSeed();   // one job per shape
            if (shape == 4)
                shSeed = seed;
            Rig rig (fs, 512, with (with (modBase (0, 0.0f), ids::lfoShape, (float) shape), ids::lfoRate, 3.7f), seed);
            rig.captureLfo = true;
            const float rateF = rawOf (*rig.proc, ids::lfoRate);
            const double inc = (double) rateF / fs;
            incUsed = inc;
            rig.run (n);

            double worst = 0.0;
            int skipped = 0;
            for (int i = 0; i < (int) rig.lfoTrace.size(); ++i)
            {
                const double x = (double) i * inc;
                const double p = frac (x);
                if (distToEdge (shape, p) < 1.0e-7)
                {
                    ++skipped;
                    continue;
                }
                const float e = shapeRef (shape, p, seed, (std::int64_t) std::floor (x));
                worst = std::max (worst, (double) std::abs (rig.lfoTrace[(size_t) i] - e));
            }
            if (! (worst <= 1.0e-6) || rig.captureErrors != 0 || (int) rig.lfoTrace.size() != n)
                ok = false;
            if (shape == 0) sineTrace = rig.lfoTrace;
            if (shape == 4) shTrace = rig.lfoTrace;
            if (shape == 1)
                triStart = rig.lfoTrace.size() > 2 && juce::exactlyEqual (rig.lfoTrace[0], 0.0f) && rig.lfoTrace[1] > 0.0f;
            d += std::string (names[shape]) + " " + sci (worst) + " (" + std::to_string (skipped) + " edge samples skipped); ";
        }

        // Negative controls: the comparator must see a wrong shape / wrong seed.
        double negShape = 0.0, negSeed = 0.0;
        const std::uint64_t otherSeed = splitmix64 (shSeed ^ 0x5A5A5A5Aull);
        for (int i = 0; i < n; ++i)
        {
            const double x = (double) i * incUsed;
            const double p = frac (x);
            negShape = std::max (negShape, (double) std::abs (sineTrace[(size_t) i] - shapeRef (1, p, shSeed, 0)));
            if (distToEdge (4, p) >= 1.0e-7)
                negSeed = std::max (negSeed, (double) std::abs (shTrace[(size_t) i] - shRef (otherSeed, (std::int64_t) std::floor (x))));
        }

        report ("G-LFO-SHAPES", ok && triStart && negShape > 0.1 && negSeed > 0.1,
                "max |lfo - analytic| per shape (<= 1e-6): " + d + "triangle starts at 0 going up: "
                + (triStart ? "yes" : "NO") + "; neg Sine vs Triangle analytic " + fmt (negShape, 4)
                + " (> 0.1); neg S&H vs other seed " + fmt (negSeed, 4) + " (> 0.1)");
    }

    std::vector<int> wrapIndices (const std::vector<float>& saw)
    {
        std::vector<int> w;
        for (size_t i = 1; i < saw.size(); ++i)
            if ((double) saw[i] < (double) saw[i - 1] - 1.0)
                w.push_back ((int) i);
        return w;
    }

    void gateLfoFree()
    {
        const double fs = kFs;
        auto measure = [fs] (float rate, double seconds, float& rateUsed)
        {
            Rig rig (fs, 512, with (with (modBase (0, 0.0f), ids::lfoShape, 2.0f), ids::lfoRate, rate), nextJobSeed());
            rig.captureLfo = true;
            rateUsed = rawOf (*rig.proc, ids::lfoRate);
            rig.run (secs (seconds, fs));
            const auto w = wrapIndices (rig.lfoTrace);
            std::vector<int> intervals;
            for (size_t i = 1; i < w.size(); ++i)
                intervals.push_back (w[i] - w[i - 1]);
            return std::make_pair (w, intervals);
        };

        float r05 = 0.0f, r06 = 0.0f;
        const auto a = measure (0.5f, 6.1, r05);
        bool ok = a.second.size() >= 2;
        std::string d;
        for (const int iv : a.second)
        {
            if (std::abs (iv - 96000) > 1) ok = false;
            d += std::to_string (iv) + " ";
        }

        const auto b = measure (0.6f, 4.1, r06);
        bool negFails = b.second.empty() ? false : true;
        std::string dn;
        for (const int iv : b.second)
        {
            if (std::abs (iv - 96000) <= 1) negFails = false;
            dn += std::to_string (iv) + " ";
        }

        report ("G-LFO-FREE", ok && negFails,
                "0.5 Hz (raw " + fmt (r05, 7) + " Hz, exact period " + fmt (fs / (double) r05, 3) + ") Saw @ 48k: "
                + std::to_string (a.first.size()) + " wraps, intervals { " + d + "} (each 96000 +/- 1, >= 2 intervals); "
                "neg 0.6 Hz intervals { " + dn + "} must miss 96000 +/- 1: " + (negFails ? "yes" : "NO"));
    }

    //==========================================================================
    void gateLfoTempo()
    {
        const double fs = kFs;
        bool ok = true;
        double worstPhase = 0.0, worstVal = 0.0, worstPeriod = 0.0;
        int minIntervals = 1 << 30;
        long long totalBlocks = 0;

        for (const double bpm : { 120.0, 97.3 })
        {
            for (int div = 0; div < 16; ++div)
            {
                const double beats = kBeatsRef[div];
                const double period = beats * 60.0 / bpm * fs;
                const int n = juce::jmax (secs (0.25, fs), (int) std::ceil (2.2 * period));
                const double dppq = bpm / (60.0 * fs);

                TestPlayHead head;
                head.bpm = bpm;
                head.ppq = 0.0;
                head.playing = true;

                const std::uint64_t seed = nextJobSeed();
                Rig rig (fs, 4096, with (with (with (modBase (0, 0.0f), ids::lfoSync, 1.0f), ids::lfoDiv, (float) div),
                                         ids::lfoShape, 2.0f), seed, &head);
                rig.captureLfo = true;
                rig.captureBlocks = true;
                rig.run (n, {}, randomSizes (seed, 4096));
                totalBlocks += (long long) rig.blocks.size();

                double ph = 0.0, val = 0.0, per = 0.0;
                for (const auto& bi : rig.blocks)
                {
                    const double expected = frac (((double) bi.start * dppq) / beats);   // absolute time, not the head
                    ph = std::max (ph, circDiff (bi.startPhase, expected));
                }
                for (int i = 0; i < (int) rig.lfoTrace.size(); ++i)
                {
                    const double p = frac (((double) i * dppq) / beats);
                    if (std::min (p, 1.0 - p) < 1.0e-7)
                        continue;
                    val = std::max (val, (double) std::abs (rig.lfoTrace[(size_t) i] - (float) (2.0 * p - 1.0)));
                }
                const auto w = wrapIndices (rig.lfoTrace);
                int intervals = 0;
                for (size_t i = 1; i < w.size(); ++i, ++intervals)
                    per = std::max (per, std::abs ((double) (w[i] - w[i - 1]) - period));

                if (! (ph <= 1.0e-9) || ! (val <= 1.0e-6) || ! (per <= 1.0) || intervals < 1 || rig.captureErrors != 0)
                {
                    ok = false;
                    info ("FAIL detail: " + fmt (bpm, 1) + " BPM " + kDivNames[div] + ": phase " + sci (ph) + ", value "
                          + sci (val) + ", period err " + fmt (per, 3) + " samples, intervals " + std::to_string (intervals)
                          + ", capture errors " + std::to_string (rig.captureErrors));
                }
                worstPhase = std::max (worstPhase, ph);
                worstVal = std::max (worstVal, val);
                worstPeriod = std::max (worstPeriod, per);
                minIntervals = std::min (minIntervals, intervals);
                info (fmt (bpm, 1) + " BPM " + std::string (kDivNames[div]) + " (" + fmt (beats, 6) + " beats, period "
                      + fmt (period, 3) + " smp): block-start phase err " + sci (ph) + ", sample err " + sci (val)
                      + ", period err " + fmt (per, 3) + " smp over " + std::to_string (intervals) + " interval(s), "
                      + std::to_string (rig.blocks.size()) + " random blocks");
            }
        }

        // Comparator sensitivity: Prism's rounded triplet literal must be
        // visible at 1e-9 over the longest 1/2T job.
        const double bpmN = 97.3, dppqN = bpmN / (60.0 * fs);
        const double endPpq = (double) (int) std::ceil (2.2 * (4.0 / 3.0) * 60.0 / bpmN * fs) * dppqN;
        const double drift = circDiff (endPpq / (double) 1.3333f, endPpq / (4.0 / 3.0));

        report ("G-LFO-TEMPO", ok && drift > 1.0e-9,
                "120 + 97.3 BPM x 16 divisions, " + std::to_string (totalBlocks) + " random blocks (1..4096): worst block-start "
                "phase err " + sci (worstPhase) + " (<= 1e-9), worst sample err " + sci (worstVal) + " (<= 1e-6), worst wrap "
                "interval err " + fmt (worstPeriod, 3) + " smp (<= 1), min intervals " + std::to_string (minIntervals)
                + " (>= 1); neg comparator: rounded 1.3333f triplet drift " + sci (drift) + " (> 1e-9)");
    }

    void gateLfoLoop()
    {
        const double fs = kFs;
        TestPlayHead head;
        head.bpm = 120.0;
        head.ppq = 0.0;
        Rig rig (fs, 512, with (with (with (modBase (0, 0.0f), ids::lfoSync, 1.0f), ids::lfoDiv, 4.0f), ids::lfoShape, 2.0f),
                 nextJobSeed(), &head);
        rig.captureBlocks = true;
        rig.captureLfo = true;
        rig.run (secs (1.3, fs));

        const double ppqBefore = head.ppq;
        const double continuation = rig.proc->getLfoForTesting().getPhase();   // phase if the loop did NOT jump
        head.ppq = 0.25;                                                         // loop jump (beats = 1)
        const size_t lfoFrom = rig.lfoTrace.size();
        rig.run (512);

        const double startPhase = rig.blocks.back().startPhase;
        const double err = circDiff (startPhase, 0.25);
        const double live = circDiff (continuation, 0.25);
        const double dppq = 120.0 / (60.0 * fs);
        double val = 0.0;
        for (int i = 0; i < 512; ++i)
        {
            const double p = frac (0.25 + (double) i * dppq);
            if (std::min (p, 1.0 - p) < 1.0e-7)
                continue;
            val = std::max (val, (double) std::abs (rig.lfoTrace[lfoFrom + (size_t) i] - (float) (2.0 * p - 1.0)));
        }

        report ("G-LFO-LOOP", err <= 1.0e-9 && val <= 1.0e-6 && live >= 0.1,
                "jump ppq " + fmt (ppqBefore, 4) + " -> 0.25 (1/4, beats 1): first block phase " + fmt (startPhase, 12)
                + " err " + sci (err) + " (<= 1e-9), block samples err " + sci (val) + " (<= 1e-6); liveness: free-run "
                "continuation " + fmt (continuation, 4) + " is " + fmt (live, 4) + " away (>= 0.1)");
    }

    void gateLfoStopped()
    {
        const double fs = kFs;
        struct Case { const char* name; bool useHead; bool playing; bool hasBpm; bool hasPpq; double bpm; double expectBpm; };
        const Case cases[] = {
            { "stopped, host 97.3",     true,  false, true,  true,  97.3, 97.3  },
            { "no playhead",            false, false, false, false, 0.0,  120.0 },
            { "playing, no BPM",        true,  true,  false, true,  0.0,  120.0 },
            { "playing, BPM 0",         true,  true,  true,  true,  0.0,  120.0 },
            { "playing 97.3, no PPQ",   true,  true,  true,  false, 97.3, 97.3  },
        };

        bool ok = true;
        std::string d;
        std::vector<float> stoppedTrace;
        for (const auto& c : cases)
        {
            TestPlayHead head;
            head.playing = c.playing;
            head.hasBpm = c.hasBpm;
            head.hasPpq = c.hasPpq;
            head.bpm = c.bpm;
            head.ppq = 5.0;
            Rig rig (fs, 512, with (with (with (modBase (0, 0.0f), ids::lfoSync, 1.0f), ids::lfoDiv, 4.0f), ids::lfoShape, 2.0f),
                     nextJobSeed(), c.useHead ? &head : nullptr);
            rig.captureLfo = true;
            rig.run (secs (2.0, fs));

            const double inc = (c.expectBpm / 60.0 / 1.0) / fs;
            double worst = 0.0;
            for (int i = 0; i < (int) rig.lfoTrace.size(); ++i)
            {
                const double p = frac ((double) i * inc);
                if (std::min (p, 1.0 - p) < 1.0e-7)
                    continue;
                worst = std::max (worst, (double) std::abs (rig.lfoTrace[(size_t) i] - (float) (2.0 * p - 1.0)));
            }
            if (! (worst <= 1.0e-6))
                ok = false;
            if (stoppedTrace.empty())
                stoppedTrace = rig.lfoTrace;
            d += std::string (c.name) + " -> " + fmt (c.expectBpm, 1) + " BPM free-run err " + sci (worst) + "; ";
        }

        double neg = 0.0;
        const double inc120 = (120.0 / 60.0) / fs;
        for (int i = 0; i < (int) stoppedTrace.size(); ++i)
            neg = std::max (neg, (double) std::abs (stoppedTrace[(size_t) i] - (float) (2.0 * frac ((double) i * inc120) - 1.0)));

        report ("G-LFO-STOPPED", ok && neg > 0.1, d + "(each <= 1e-6); neg stopped-97.3 vs 120 analytic " + fmt (neg, 4) + " (> 0.1)");
    }

    //==========================================================================
    void gateGlobalPhase()
    {
        const double fs = kFs;
        const int tA = secs (0.1, fs), tB = tA + secs (0.37, fs), n = secs (1.2, fs);
        bool ok = true;
        std::string d;

        for (const bool interp : { false, true })
        {
            auto p = modBase (0, 0.5f);
            p = with (p, ids::interp, interp ? 1.0f : 0.0f);
            p = with (p, ids::lfoDepth, 1.0f);
            p = with (p, ids::lfoShape, 0.0f);
            p = with (p, ids::lfoRate, 2.66f);
            Rig rig (fs, 512, p, nextJobSeed());
            rig.captureVoices = true;
            rig.run (n, { evOn (tA, 57), evOn (tB, 64) });

            const auto sv = soundingVoices (*rig.proc);
            if (sv.size() != 2)
            {
                ok = false;
                d += std::string (interp ? "Interp On" : "Interp Off") + ": " + std::to_string (sv.size())
                   + " sounding voices (want 2); ";
                continue;
            }
            const auto& a = rig.voiceTrace[(size_t) sv[0]];
            const auto& b = rig.voiceTrace[(size_t) sv[1]];
            const int from = interp ? tB + secs (0.02, fs) : tB;

            double maxDiff = 0.0, lo = 2.0, hi = -1.0;
            long long notBitwise = 0;
            for (int k = from; k < n; ++k)
            {
                maxDiff = std::max (maxDiff, (double) std::abs (a[(size_t) k] - b[(size_t) k]));
                if (std::memcmp (&a[(size_t) k], &b[(size_t) k], sizeof (float)) != 0)
                    ++notBitwise;
            }
            for (int k = tB; k < n; ++k)
            {
                lo = std::min (lo, (double) a[(size_t) k]);
                hi = std::max (hi, (double) a[(size_t) k]);
            }
            const double restart = std::abs ((double) b[(size_t) tB] - 0.5);   // 0.5 + 0.5 * sin(0)

            const bool pass = (interp ? maxDiff <= 1.0e-6 : notBitwise == 0) && (hi - lo) >= 0.5 && restart >= 0.1;
            if (! pass)
                ok = false;
            d += std::string (interp ? "Interp On" : "Interp Off") + ": max |effA - effB| " + sci (maxDiff)
               + (interp ? " from tB+20 ms (<= 1e-6)" : " (" + std::to_string (notBitwise) + " samples not bit-identical, want 0)")
               + ", effPos range " + fmt (hi - lo, 3) + " (>= 0.5), voice 2 first effPos " + fmt (b[(size_t) tB], 4)
               + " vs a per-voice restart 0.5: " + fmt (restart, 4) + " (>= 0.1); ";
        }
        report ("G-GLOBAL-PHASE", ok, d + "Sine 2.66 Hz, depth 1, notes 0.37 s apart");
    }

    //==========================================================================
    void gateShDeterminism()
    {
        const double fs = kFs;
        auto p = modBase (0, 0.5f);
        p = with (p, ids::lfoDepth, 1.0f);
        p = with (p, ids::lfoShape, 4.0f);
        p = with (p, ids::lfoRate, 8.0f);
        const std::vector<Ev> evs { evOn (100, 45), evOn (2000, 52), evOn (5000, 60),
                                    evOff (secs (1.0, fs), 45), evOff (secs (1.0, fs), 52), evOff (secs (1.0, fs), 60) };
        const int n = secs (1.5, fs);

        const std::uint64_t seedA = nextJobSeed(), seedB = nextJobSeed();
        Rig r1 (fs, 512, p, seedA);
        r1.captureLfo = true;
        const auto y1 = r1.run (n, evs);
        const auto lfo1 = r1.lfoTrace;

        Rig r2 (fs, 512, p, seedA);                    // deliberate replay of job A
        const auto y2 = r2.run (n, evs);

        r1.reprepare();
        r1.lfoTrace.clear();
        const auto y3 = r1.run (n, evs);
        const bool lfoReprep = sameBits (r1.lfoTrace, lfo1);

        Rig r4 (fs, 512, p, seedB);
        r4.captureLfo = true;
        const auto y4 = r4.run (n, evs);

        std::set<float> distinct (lfo1.begin(), lfo1.end());
        const double peak = rms (y1, 0, n);

        const bool runsSame = sameBits (y1, y2);
        const bool reprepSame = sameBits (y1, y3) && lfoReprep;
        const long long seedDiff = countDiffering (y1, y4);
        const long long seedLfoDiff = countDiffering (lfo1, r4.lfoTrace);

        // Tempo loop: S&H addressed by floor(ppq / beats) replays on a loop.
        auto loopPasses = [fs] (bool playing, std::uint64_t seed)
        {
            TestPlayHead head;
            head.bpm = 120.0;
            head.ppq = 0.0;
            head.playing = playing;
            auto q = with (with (with (modBase (0, 0.5f), ids::lfoSync, 1.0f), ids::lfoDiv, 6.0f), ids::lfoShape, 4.0f);
            Rig rig (fs, 512, q, seed, &head);
            rig.captureLfo = true;
            rig.run (secs (2.0, fs));
            const auto pass1 = rig.lfoTrace;
            rig.lfoTrace.clear();
            head.ppq = 0.0;                            // loop back to the start
            rig.run (secs (2.0, fs));
            return std::make_pair (pass1, rig.lfoTrace);
        };
        const auto loopPlay = loopPasses (true, nextJobSeed());
        const auto loopStop = loopPasses (false, nextJobSeed());
        const bool loopReplays = sameBits (loopPlay.first, loopPlay.second);
        const std::set<float> loopDistinct (loopPlay.first.begin(), loopPlay.first.end());
        const bool stopDiffers = ! sameBits (loopStop.first, loopStop.second);

        // Unique seeds per render job: 8 jobs, 8 different S&H renders.
        std::vector<std::vector<float>> jobs;
        std::set<std::uint64_t> jobSeeds;
        for (int j = 0; j < 8; ++j)
        {
            const std::uint64_t s = nextJobSeed();
            jobSeeds.insert (s);
            Rig rig (fs, 512, p, s);
            rig.captureLfo = true;
            rig.run (secs (0.5, fs));
            jobs.push_back (rig.lfoTrace);
        }
        int samePairs = 0;
        for (size_t i = 0; i < jobs.size(); ++i)
            for (size_t k = i + 1; k < jobs.size(); ++k)
                if (sameBits (jobs[i], jobs[k]))
                    ++samePairs;

        report ("G-SH-DET",
                runsSame && reprepSame && seedDiff > 0 && seedLfoDiff > 0 && distinct.size() >= 8 && peak > 0.01
                    && loopReplays && loopDistinct.size() >= 8 && stopDiffers && jobSeeds.size() == 8 && samePairs == 0,
                "S&H 8 Hz depth 1, 3 notes: two runs byte-identical " + std::string (runsSame ? "yes" : "NO")
                + "; re-prepare byte-identical (audio + LFO) " + (reprepSame ? "yes" : "NO") + "; other seed differs in "
                + std::to_string (seedDiff) + " audio / " + std::to_string (seedLfoDiff) + " LFO samples (> 0); "
                + std::to_string (distinct.size()) + " distinct S&H values (>= 8), render RMS " + fmt (peak, 4)
                + "; tempo loop (1/16, 2 s x 2) replays bitwise " + (loopReplays ? "yes" : "NO") + " with "
                + std::to_string (loopDistinct.size()) + " distinct values (>= 8); neg stopped transport differs "
                + (stopDiffers ? "yes" : "NO") + "; 8 jobs: " + std::to_string (jobSeeds.size()) + " unique seeds, "
                + std::to_string (samePairs) + " identical render pairs (want 0)");
    }

    //==========================================================================
    void gateClamp()
    {
        const double fs = kFs;
        struct Case { const char* name; float knob; float amt; };
        const Case cases[] = { { "knob 1, amt +1", 1.0f, 1.0f }, { "knob 0, amt -1", 0.0f, -1.0f }, { "knob 0.5, amt 0", 0.5f, 0.0f } };

        bool ok = true;
        std::string d;
        int ci = 0;
        for (const auto& c : cases)
        {
            auto p = modBase (0, c.knob);
            p = with (p, ids::lfoDepth, 1.0f);
            p = with (p, ids::lfoShape, 3.0f);
            p = with (p, ids::lfoRate, 4.0f);
            p = with (p, ids::envAmount, c.amt);
            p = with (p, ids::menvAttack, 0.001f);
            p = with (p, ids::menvDecay, 0.001f);
            p = with (p, ids::menvSustain, 1.0f);
            Rig rig (fs, 512, p, nextJobSeed());
            rig.captureVoices = true;
            std::vector<Ev> evs;
            for (int k = 0; k < 8; ++k)
                evs.push_back (evOn (0, 45 + 3 * k));
            const int n = secs (1.0, fs);
            rig.run (n, evs);

            const auto sv = soundingVoices (*rig.proc);
            long long violations = 0;
            double lo = 2.0, hi = -1.0;
            for (const int v : sv)
                for (int k = 0; k < n; ++k)
                {
                    const float e = rig.voiceTrace[(size_t) v][(size_t) k];
                    if (! std::isfinite (e) || e < 0.0f || e > 1.0f)
                        ++violations;
                    lo = std::min (lo, (double) e);
                    hi = std::max (hi, (double) e);
                }
            bool live = false;
            if (ci == 0) live = juce::exactlyEqual ((float) hi, 1.0f);
            if (ci == 1) live = juce::exactlyEqual ((float) lo, 0.0f);
            if (ci == 2) live = (hi - lo) >= 0.99;
            if (violations != 0 || ! live || sv.size() != 8)
                ok = false;
            d += std::string (c.name) + ": " + std::to_string (sv.size()) + " voices, " + std::to_string (violations)
               + " samples outside [0,1], effPos min " + fmt (lo, 6) + " max " + fmt (hi, 6) + " (clamp engaged: "
               + (live ? "yes" : "NO") + "); ";
            ++ci;
        }
        report ("G-DSP05-CLAMP", ok, d + "Square 4 Hz depth 1, Interp On");
    }

    //==========================================================================
    // Steady-state A/B renders + the step render share one helper.
    std::vector<float> renderKnob (float knob, std::uint64_t seed, int n, int stepAt, float knobAfter, bool negControl)
    {
        Rig rig (kFs, 64, modBase (0, knob), seed);
        if (negControl)
        {
            rig.proc->setKnobRampOffForTesting (true);
            rig.proc->setSmootherBypassForTesting (true);
        }
        const std::vector<Ev> evs { evOn (0, 45) };
        if (stepAt <= 0)
            return rig.run (n, evs);
        auto y = rig.run (stepAt, evs);
        rig.set (ids::position, knobAfter);
        const auto y2 = rig.run (n - stepAt, evs);
        y.insert (y.end(), y2.begin(), y2.end());
        return y;
    }

    void gateZipper()
    {
        const double fs = kFs;
        const int n = secs (0.5, fs);
        const std::uint64_t seed = nextJobSeed();
        const auto A = renderKnob (0.0f, seed, n, 0, 0.0f, false);
        const auto B = renderKnob (1.0f, seed, n, 0, 0.0f, false);

        int t = 0;
        double best = -1.0;
        for (int k = secs (0.25, fs) / 64; k * 64 < secs (0.35, fs); ++k)
        {
            const int c = k * 64;
            const double diff = std::abs ((double) A[(size_t) c] - (double) B[(size_t) c]);
            if (diff > best) { best = diff; t = c; }
        }

        const int xf = secs (0.030, fs);   // 20 ms knob ramp + smoother settle
        const auto Y = renderKnob (0.0f, seed, n, t, 1.0f, false);
        const auto N = renderKnob (0.0f, seed, n, t, 1.0f, true);
        const auto v = clickRatio (Y, t, xf, fs);
        const auto vn = clickRatio (N, t, xf, fs);

        report ("G-DSP05-ZIPPER", v.ratio <= 1.5 && best >= 0.25 && vn.ratio >= 4.0,
                "position 0 -> 1 at sample " + std::to_string (t) + " (block boundary), Sine->Saw, Interp On, A2: excess ratio "
                + fmt (v.ratio, 3) + " (event " + fmt (v.event, 5) + " / plateau " + fmt (v.plateau, 5) + ", <= 1.5); liveness |A-B| "
                + fmt (best, 4) + " (>= 0.25); neg smootherBypass + knob ramp off: ratio " + fmt (vn.ratio, 3) + " (event "
                + fmt (vn.event, 5) + ", >= 4)");
    }

    //==========================================================================
    struct Edge
    {
        int t = 0;
        float posOld = 0.0f, posNew = 0.0f;
        double live = 0.0;
    };

    // |A - B| at each edge: the steady renders at the old and new positions
    // (Interp On read, A2 at 48k, sustain 1, velocity 1).
    struct LiveModel
    {
        const WavetableBank* bank = nullptr;
        int level = 0;
        double inc = 0.0;
        double gain = 0.0;

        double at (const Edge& e, int tOn) const
        {
            const double ph = frac ((double) (e.t - tOn) * inc);
            const double a = wt::readSample (*bank, level, ph, true, e.posOld, 0);
            const double b = wt::readSample (*bank, level, ph, true, e.posNew, 0);
            return std::abs (a - b) * gain;
        }

        double minLive (const std::vector<Edge>& edges, int tOn) const
        {
            double m = 1.0e9;
            for (const auto& e : edges)
                m = std::min (m, at (e, tOn));
            return edges.empty() ? 0.0 : m;
        }

        int chooseOnset (const std::vector<Edge>& edges, int from, double& bestOut) const
        {
            const int span = (int) std::ceil (1.0 / inc) + 1;
            int best = from;
            bestOut = -1.0;
            for (int t = from; t < from + span; ++t)
            {
                const double m = minLive (edges, t);
                if (m > bestOut) { bestOut = m; best = t; }
            }
            return best;
        }
    };

    std::vector<Edge> edgesFromLfo (const std::vector<float>& lfo, int from, int to)
    {
        std::vector<Edge> e;
        for (int i = juce::jmax (1, from); i < juce::jmin ((int) lfo.size(), to); ++i)
            if (! juce::exactlyEqual (lfo[(size_t) i], lfo[(size_t) (i - 1)]))
                e.push_back ({ i, wt::clamp01 (0.5f + 0.5f * lfo[(size_t) (i - 1)]), wt::clamp01 (0.5f + 0.5f * lfo[(size_t) i]), 0.0 });
        return e;
    }

    struct ClickRun
    {
        double maxRatio = 0.0, minNeg = 1.0e9;
        std::string perEdge;
    };

    Params clickParams (int shape)
    {
        auto p = modBase (0, 0.5f);
        p = with (p, ids::lfoDepth, 1.0f);
        p = with (p, ids::lfoShape, (float) shape);
        p = with (p, ids::lfoRate, 2.0f);
        return p;
    }

    ClickRun runClick (int shape, std::uint64_t seed, int tOn, const std::vector<Edge>& edges, int n)
    {
        const double fs = kFs;
        const int xf = secs (0.010, fs);   // 2 ms pole settle (5 tau)
        Rig y (fs, 512, clickParams (shape), seed);
        const auto Y = y.run (n, { evOn (tOn, 45) });
        Rig ng (fs, 512, clickParams (shape), seed);
        ng.proc->setSmootherBypassForTesting (true);
        const auto N = ng.run (n, { evOn (tOn, 45) });

        ClickRun r;
        for (const auto& e : edges)
        {
            const auto v = clickRatio (Y, e.t, xf, fs);
            const auto vn = clickRatio (N, e.t, xf, fs);
            r.maxRatio = std::max (r.maxRatio, v.ratio);
            r.minNeg = std::min (r.minNeg, vn.ratio);
            r.perEdge += "t " + std::to_string (e.t) + " pos " + fmt (e.posOld, 3) + "->" + fmt (e.posNew, 3) + " |A-B| "
                       + fmt (e.live, 3) + " ratio " + fmt (v.ratio, 3) + " (ev " + fmt (v.event, 5) + "/pl " + fmt (v.plateau, 5)
                       + ") neg " + fmt (vn.ratio, 2) + "; ";
        }
        return r;
    }

    void gateClick (const BuiltInBanks& banks)
    {
        const double fs = kFs;
        const int n = secs (3.4, fs), from = secs (0.3, fs), to = secs (3.3, fs);

        LiveModel lm;
        lm.bank = banks.get (BankFactory::sineSaw);
        const double hz = juce::MidiMessage::getMidiNoteInHertz (45);
        lm.level = wt::selectLevel (hz, fs, true);
        lm.inc = std::min (hz / fs, 0.49);
        {
            Rig probe (fs, 512, modBase (0, 0.5f), nextJobSeed());
            lm.gain = (double) WtVoice::kVoiceGain * (double) juce::Decibels::decibelsToGain (rawOf (*probe.proc, ids::outputLevel), -60.0f);
        }

        for (const int shape : { 3, 4 })
        {
            const char* gate = shape == 3 ? "G-CLICK-SQ" : "G-CLICK-SH";
            std::uint64_t seed = 0;
            bool setupOk = true;
            std::string setup;

            if (shape == 3)
            {
                seed = nextJobSeed();
            }
            else
            {
                // S&H: pick a seed whose held values make every edge in the
                // window a big position step (else the stimulus cannot click
                // and the gate would be vacuous). Rate 2 Hz -> cycle c starts
                // near c * 24000; edges in 0.3..3.3 s are cycles 1..6.
                const float rateF = [&] { Rig r (fs, 64, clickParams (4), 1); return rawOf (*r.proc, ids::lfoRate); }();
                bool found = false;
                for (std::uint64_t k = 0; k < 4000000 && ! found; ++k)
                {
                    const std::uint64_t s = splitmix64 (kRunSeed ^ (0xC11C000000000000ull + k));
                    bool bigSteps = true;
                    for (int c = 1; c <= 6 && bigSteps; ++c)
                        bigSteps = std::abs (shRef (s, c) - shRef (s, c - 1)) >= 1.0f;
                    if (! bigSteps)
                        continue;
                    std::vector<Edge> pred;
                    for (int c = 1; c <= 6; ++c)
                        pred.push_back ({ (int) std::lround ((double) c * fs / (double) rateF),
                                          wt::clamp01 (0.5f + 0.5f * shRef (s, c - 1)), wt::clamp01 (0.5f + 0.5f * shRef (s, c)), 0.0 });
                    double bestLive = 0.0;
                    lm.chooseOnset (pred, secs (0.05, fs), bestLive);
                    if (bestLive >= 0.3)
                    {
                        seed = s;
                        found = true;
                        setup = "seed search: k = " + std::to_string (k) + "; ";
                    }
                }
                if (! found)
                {
                    setupOk = false;
                    setup = "no S&H seed with live edges found; ";
                }
                gIssuedSeeds.push_back (seed);   // counted by G-SEEDS
            }

            // The LFO is global and note-independent: an LFO-only render gives
            // the exact edge samples of the real render.
            std::vector<Edge> edges;
            {
                Rig lfoOnly (fs, 512, clickParams (shape), seed);
                lfoOnly.captureLfo = true;
                lfoOnly.run (n);
                edges = edgesFromLfo (lfoOnly.lfoTrace, from, to);
            }
            const size_t wantEdges = shape == 3 ? 12 : 6;
            if (edges.size() != wantEdges)
                setupOk = false;

            double minLive = 0.0;
            const int tOn = lm.chooseOnset (edges, secs (0.05, fs), minLive);
            for (auto& e : edges)
                e.live = lm.at (e, tOn);

            const auto r = runClick (shape, seed, tOn, edges, n);

            const bool pass = setupOk && r.maxRatio <= 1.5 && minLive >= 0.25 && r.minNeg >= 4.0;
            report (gate, pass,
                    std::string (shape == 3 ? "Square" : "S&H") + " 2 Hz depth 1, Sine->Saw pos 0.5, A2 note-on at "
                    + std::to_string (tOn) + ", Interp On, one 2 ms pole: "
                    + setup + std::to_string (edges.size()) + " edges (want " + std::to_string (wantEdges) + "); max excess ratio "
                    + fmt (r.maxRatio, 3) + " (<= 1.5); min liveness |A-B| " + fmt (minLive, 4) + " (>= 0.25); neg smootherBypass min ratio "
                    + fmt (r.minNeg, 3) + " (>= 4)");
            info ("per edge: " + r.perEdge);
        }
    }

    //==========================================================================
    void gateFunc06Sweep()
    {
        const double fs = kFs;
        bool ok = true;
        std::string d;
        const int n = secs (1.0, fs);

        for (const float sign : { 1.0f, -1.0f })
        {
            const float knob = sign > 0.0f ? 0.0f : 1.0f;
            auto p = modBase (0, knob);
            p = with (p, ids::envAmount, sign);
            p = with (p, ids::menvAttack, 0.2f);
            p = with (p, ids::menvDecay, 0.3f);
            p = with (p, ids::menvSustain, 0.5f);
            p = with (p, ids::menvRelease, 0.5f);
            Rig rig (fs, 512, p, nextJobSeed());
            rig.captureVoices = true;
            const auto y = rig.run (n, { evOn (0, 57) });
            const auto sv = soundingVoices (*rig.proc);
            if (sv.size() != 1)
            {
                ok = false;
                d += "sounding voices " + std::to_string (sv.size()) + " (want 1); ";
                continue;
            }
            const auto& tr = rig.voiceTrace[(size_t) sv[0]];
            const double e0 = tr[0], eMid = tr[(size_t) secs (0.1, fs)], ePeak = tr[(size_t) secs (0.2, fs)],
                         eSus = tr[(size_t) secs (0.8, fs)];
            const double wantMid = knob + sign * 0.5, wantPeak = knob + sign * 1.0, wantSus = knob + sign * 0.5;
            long long nonMono = 0;
            for (int k = 1; k < secs (0.19, fs); ++k)
                if (sign * (tr[(size_t) k] - tr[(size_t) (k - 1)]) < -1.0e-6f)
                    ++nonMono;
            const double bStart = brightness (y, secs (0.005, fs), secs (0.025, fs));
            const double bPeak  = brightness (y, secs (0.19, fs), secs (0.21, fs));
            const double bRatio = sign > 0.0f ? bPeak / std::max (bStart, 1.0e-12) : bStart / std::max (bPeak, 1.0e-12);

            const bool pass = std::abs (e0 - knob) <= 0.01 && std::abs (eMid - wantMid) <= 0.03 && std::abs (ePeak - wantPeak) <= 0.03
                           && std::abs (eSus - wantSus) <= 0.03 && nonMono == 0 && bRatio >= 1.5;
            if (! pass)
                ok = false;
            d += std::string (sign > 0.0f ? "amt +1 (knob 0)" : "amt -1 (knob 1)") + ": effPos t0 " + fmt (e0, 4) + ", 0.1 s "
               + fmt (eMid, 4) + " (want " + fmt (wantMid, 2) + "), 0.2 s " + fmt (ePeak, 4) + " (want " + fmt (wantPeak, 2)
               + "), 0.8 s " + fmt (eSus, 4) + " (want " + fmt (wantSus, 2) + "; tol 0.03), non-monotonic steps in attack "
               + std::to_string (nonMono) + ", brightness ratio " + fmt (bRatio, 3) + " (>= 1.5); ";
        }

        // Neg: amount 0 -> static position.
        Rig rz (fs, 512, with (with (modBase (0, 0.3f), ids::menvAttack, 0.2f), ids::menvSustain, 0.5f), nextJobSeed());
        rz.captureVoices = true;
        rz.run (n, { evOn (0, 57) });
        const auto sz = soundingVoices (*rz.proc);
        double range = 1.0;
        if (sz.size() == 1)
        {
            const auto& tr = rz.voiceTrace[(size_t) sz[0]];
            const auto mm = std::minmax_element (tr.begin(), tr.begin() + n);
            range = (double) (*mm.second - *mm.first);
        }
        report ("G-FUNC06-SWEEP", ok && range <= 1.0e-6,
                d + "menv A 0.2 D 0.3 S 0.5; neg amt 0 effPos range " + sci (range) + " (<= 1e-6)");
    }

    void gateFunc06Life()
    {
        const double fs = kFs;
        auto make = [] (float ampRel)
        {
            auto p = modBase (0, 0.0f);
            p = with (p, ids::ampRelease, ampRel);
            p = with (p, ids::envAmount, 1.0f);
            p = with (p, ids::menvAttack, 0.001f);
            p = with (p, ids::menvDecay, 0.001f);
            p = with (p, ids::menvSustain, 1.0f);
            p = with (p, ids::menvRelease, 10.0f);
            return p;
        };
        const int tOff = secs (0.3, fs);
        const std::vector<Ev> evs { evOn (0, 57), evOff (tOff, 57) };

        Rig rig (fs, 256, make (0.05f), nextJobSeed());
        rig.run (secs (0.29, fs), evs);
        const auto sv = soundingVoices (*rig.proc);
        const int v = sv.size() == 1 ? sv[0] : -1;
        rig.run (secs (0.4, fs) - rig.cursor, evs);
        const int soundingAt = rig.proc->getSoundingVoiceCountForTesting();
        const float menvAtEnd = v >= 0 ? rig.proc->getVoiceForTesting (v)->getLastModEnv() : 0.0f;
        const auto tail = rig.run (secs (0.2, fs), evs);
        long long nonZero = 0;
        for (const float s : tail)
            if (! juce::exactlyEqual (s, 0.0f))
                ++nonZero;

        Rig ctl (fs, 256, make (5.0f), nextJobSeed());
        ctl.run (secs (0.4, fs), evs);
        const int ctlSounding = ctl.proc->getSoundingVoiceCountForTesting();

        report ("G-FUNC06-LIFE", v >= 0 && soundingAt == 0 && nonZero == 0 && menvAtEnd > 0.9f && ctlSounding == 1,
                "amp release 0.05 s, mod release 10 s, note-off at 0.3 s: sounding voices at 0.4 s = " + std::to_string (soundingAt)
                + " (want 0); non-zero samples 0.4-0.6 s = " + std::to_string (nonZero) + " (want 0); mod env when the voice ended "
                + fmt (menvAtEnd, 4) + " (> 0.9: the mod env did not hold the voice); control amp release 5 s: sounding "
                + std::to_string (ctlSounding) + " (want 1)");
    }

    void gateFunc06Repush()
    {
        const double fs = kFs;
        auto p = modBase (0, 0.0f);
        p = with (p, ids::interp, 0.0f);                // effPos = raw exactly
        p = with (p, ids::envAmount, 1.0f);
        p = with (p, ids::menvAttack, 0.01f);
        p = with (p, ids::menvDecay, 3.0f);
        p = with (p, ids::menvSustain, 0.0f);
        p = with (p, ids::menvRelease, 0.5f);
        p = with (p, ids::ampRelease, 1.0f);            // voice outlives the mod release
        const int tOff = secs (0.85, fs), n = tOff + secs (0.6, fs);
        const std::vector<Ev> evs { evOn (0, 57), evOff (tOff, 57) };
        const std::uint64_t seed = nextJobSeed();

        auto render = [&] (int blk)
        {
            Rig rig (fs, blk, p, seed);                 // same job at two partitions (deliberate replay)
            rig.captureVoices = true;
            rig.run (n, evs);
            int v = -1;
            std::uint64_t newest = 0;
            for (int i = 0; i < Proc::kNumVoices; ++i)
                if (rig.proc->getVoiceForTesting (i)->getNoteAge() >= newest)
                {
                    newest = rig.proc->getVoiceForTesting (i)->getNoteAge();
                    v = i;
                }
            return std::make_pair (rig.voiceTrace[(size_t) v], rig.proc->getVoiceForTesting (v)->isSounding());
        };
        const auto small = render (64);
        const auto big = render (4096);
        const auto& tr = small.first;

        const bool same = sameBits (small.first, big.first);
        const double atOff = tr[(size_t) (tOff - 1)];
        const double mid = tr[(size_t) (tOff + secs (0.25, fs))];
        const double end = tr[(size_t) (tOff + secs (0.55, fs))];
        const double want = 0.5 * atOff;

        // Neg control (documented, not shipped): raw ADSR re-pushed every 64 samples.
        juce::ADSR env;
        env.setSampleRate (fs);
        const juce::ADSR::Parameters ap { 0.01f, 3.0f, 0.0f, 0.5f };
        env.setParameters (ap);
        env.noteOn();
        double negMid = 0.0;
        for (int i = 0; i <= tOff + secs (0.25, fs); ++i)
        {
            if (i == tOff) env.noteOff();
            if (i % 64 == 0) env.setParameters (ap);   // the bug
            const float e = env.getNextSample();
            if (i == tOff + secs (0.25, fs)) negMid = e;
        }

        report ("G-FUNC06-REPUSH",
                same && atOff >= 0.5 && std::abs (mid - want) <= 0.02 && end <= 1.0e-6 && small.second && std::abs (negMid - want) >= 0.1,
                "mod env (A 0.01 D 3 S 0 R 0.5) with the processor pushing params every block: block 64 vs 4096 effPos "
                "bit-identical " + std::string (same ? "yes" : "NO") + "; at note-off " + fmt (atOff, 4) + " (>= 0.5), +0.25 s "
                + fmt (mid, 4) + " (want " + fmt (want, 4) + " +/- 0.02, linear release), +0.55 s " + sci (end) + " (0); voice alive "
                + (small.second ? "yes" : "NO") + "; neg raw ADSR re-pushed per block +0.25 s " + fmt (negMid, 4)
                + " (>= 0.1 away from " + fmt (want, 4) + ")");
    }

    //==========================================================================
    void gateBlockSize()
    {
        const double fs = kFs;
        const int n = secs (1.0, fs);
        bool ok = true;
        std::string d;

        for (int cfg = 0; cfg < 2; ++cfg)
        {
            auto p = modBase (1, 0.3f);
            p = with (p, ids::lfoDepth, 0.7f);
            p = with (p, ids::lfoShape, cfg == 0 ? 0.0f : 4.0f);
            p = with (p, ids::lfoRate, cfg == 0 ? 3.0f : 9.0f);
            p = with (p, ids::interp, cfg == 0 ? 1.0f : 0.0f);
            p = with (p, ids::envAmount, 0.6f);
            p = with (p, ids::menvAttack, 0.05f);
            p = with (p, ids::menvDecay, 0.2f);
            p = with (p, ids::menvSustain, 0.4f);
            p = with (p, ids::menvRelease, 0.3f);
            p = with (p, ids::ampAttack, 0.01f);
            p = with (p, ids::ampDecay, 0.1f);
            p = with (p, ids::ampSustain, 0.7f);
            p = with (p, ids::ampRelease, 0.2f);

            const std::vector<Ev> evs { evOn (100, 48), evOn (777, 55), evOn (3000, 60), evWheel (5000, 11000),
                                        evOff (9000, 55), evOn (12345, 67), evOff (20000, 48), evOff (30000, 60),
                                        evOff (40000, 67) };
            const std::uint64_t seed = nextJobSeed();   // ONE job replayed at every partition

            auto render = [&] (int prep, int fixed, bool random, const Params& pp, const std::vector<Ev>& e)
            {
                Rig rig (fs, prep, pp, seed);
                return random ? rig.run (n, e, randomSizes (seed ^ 0xB10Cull, 4096)) : rig.run (n, e, [fixed] { return fixed; });
            };

            const auto r64   = render (64, 64, false, p, evs);
            const auto r512  = render (512, 512, false, p, evs);
            const auto r4096 = render (4096, 4096, false, p, evs);
            const auto rRand = render (4096, 0, true, p, evs);

            auto pStatic = with (with (p, ids::lfoDepth, 0.0f), ids::envAmount, 0.0f);
            const auto rStatic = render (64, 64, false, pStatic, evs);
            auto evShift = evs;
            evShift[0].pos = 101;
            const auto rShift = render (64, 64, false, p, evShift);

            double pk = 0.0;
            for (const float v : r64)
                pk = std::max (pk, (double) std::abs (v));
            const bool s512 = sameBits (r64, r512), s4096 = sameBits (r64, r4096), sRand = sameBits (r64, rRand);
            const long long modDiff = countDiffering (r64, rStatic);
            const long long shiftDiff = countDiffering (r64, rShift);
            const bool pass = s512 && s4096 && sRand && pk > 0.1 && modDiff > 0 && shiftDiff > 0;
            if (! pass)
                ok = false;
            d += std::string (cfg == 0 ? "Sine LFO + Interp On" : "S&H LFO + Interp Off") + ": 64 == 512 " + (s512 ? "yes" : "NO")
               + ", 64 == 4096 " + (s4096 ? "yes" : "NO") + ", 64 == random(1..4096) " + (sRand ? "yes" : "NO") + " (bitwise); peak "
               + fmt (pk, 3) + "; liveness: modulation changes " + std::to_string (modDiff) + " samples; neg 1-sample note shift changes "
               + std::to_string (shiftDiff) + "; ";
        }
        report ("G-BLOCKSIZE", ok, d + "free mode, static params, mod env + wheel + 4 notes");
    }

    //==========================================================================
    void allocScenario()
    {
        const double fs = kFs;
        TestPlayHead head;
        head.bpm = 97.3;
        auto p = modBase (0, 0.3f);
        p = with (p, ids::lfoSync, 1.0f);
        p = with (p, ids::lfoDiv, 13.0f);
        p = with (p, ids::lfoShape, 4.0f);
        p = with (p, ids::lfoDepth, 1.0f);
        p = with (p, ids::envAmount, 0.5f);
        p = with (p, ids::menvAttack, 0.05f);
        Rig rig (fs, 512, p, nextJobSeed(), &head);
        rig.run (512);                                                       // warm-up (unarmed)

        std::vector<Ev> evs;
        for (int i = 0; i < 8; ++i)
            evs.push_back (evOn (rig.cursor + i * 40, 48 + i));
        rig.run (4096, evs);
        for (const float s : { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f })
        {
            rig.set (ids::lfoShape, s);
            rig.run (1024);
        }
        rig.set (ids::lfoSync, 0.0f);                                        // free
        rig.run (1024);
        head.playing = false;
        rig.set (ids::lfoSync, 1.0f);                                        // tempo, stopped
        rig.run (1024);
        head.playing = true;
        head.ppq = 3.0;                                                      // loop jump
        rig.run (1024);
        rig.set (ids::interp, 0.0f);
        rig.run (1024);
        rig.set (ids::interp, 1.0f);
        rig.set (ids::lfoDepth, 0.2f);
        rig.set (ids::envAmount, -1.0f);
        rig.set (ids::menvRelease, 2.0f);
        rig.run (1024);

        evs.clear();
        for (int i = 0; i < 8; ++i)
            evs.push_back (evOff (rig.cursor + i * 10, 48 + i));
        rig.run (secs (0.2, fs), evs);                                       // release tails

        rig.set (ids::voiceMode, 1.0f);                                      // Mono: retrigger + legato
        evs.clear();
        evs.push_back (evOn (rig.cursor + 10, 60));
        evs.push_back (evOn (rig.cursor + 300, 64));
        evs.push_back (evWheel (rig.cursor + 400, 12000));
        evs.push_back (evOff (rig.cursor + 700, 64));
        evs.push_back (evOff (rig.cursor + 900, 60));
        evs.push_back (evOn (rig.cursor + 1200, 62));                        // retrigger from the tail
        rig.run (2048, evs);
        rig.set (ids::voiceMode, 0.0f);

        evs.clear();
        evs.push_back (evOn (rig.cursor + 100, 70));
        evs.push_back (evOn (rig.cursor + 1500, 71));
        rig.run (4096, evs, [] { return 2048; });                            // oversized host block
        rig.run (512, {}, [] { return 1; });                                 // 1-sample blocks
    }

    void gateModAlloc()
    {
       #if JUCE_MAC
        audioThread = pthread_self();
        malloc_logger = countAllocation;

        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int live = allocCount;
        allocCount = 0;

        if (live == 1)
        {
            gAllocMode = true;
            allocScenario();
            gAllocMode = false;
        }
        malloc_logger = nullptr;

        const int n = allocCount;
        report ("G-MOD-ALLOC", live == 1 && n == 0,
                "liveness malloc(64) counted " + std::to_string (live) + " (want 1); " + std::to_string (n)
                + " audio-thread allocation(s) over tempo S&H LFO, shape/sync/transport changes, loop jump, mod env, "
                  "interp toggle, release tails, Mono retrigger + legato, oversized + 1-sample blocks (want 0)");
       #else
        report ("G-MOD-ALLOC", true, "skipped (macOS-only malloc_logger gate)");
       #endif
    }

    void gateSeeds()
    {
        std::set<std::uint64_t> u (gIssuedSeeds.begin(), gIssuedSeeds.end());
        report ("G-SEEDS", u.size() == gIssuedSeeds.size() && ! gIssuedSeeds.empty(),
                std::to_string (gIssuedSeeds.size()) + " render-job seeds issued, " + std::to_string (u.size())
                + " unique (every job distinct; deliberate replays reuse their own job's seed)");
    }
}

int main (int, char**)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    // ONE BuiltInBanks for the whole run (shared by the short-lived processors).
    juce::SharedResourcePointer<BuiltInBanks> banks;
    std::cout << "mod-check: built-in banks ready (buildMillis " << fmt (banks->buildMillis, 2) << " ms)\n";

    gateLfoShapes();
    gateLfoFree();
    gateLfoTempo();
    gateLfoLoop();
    gateLfoStopped();
    gateGlobalPhase();
    gateShDeterminism();
    gateClamp();
    gateZipper();
    gateClick (*banks);
    gateFunc06Sweep();
    gateFunc06Life();
    gateFunc06Repush();
    gateBlockSize();
    gateSeeds();
    gateModAlloc();

    report ("G-FINITE", gNonFinite == 0 && gChannelMismatch == 0,
            std::to_string (gNonFinite) + " non-finite, " + std::to_string (gChannelMismatch)
            + " L != R over " + std::to_string (gSamplesChecked) + " samples");

    std::cout << "mod-check: " << (gFailures == 0 ? std::string ("ALL PASS") : "FAILURES = " + std::to_string (gFailures))
              << "\n";
    return gFailures == 0 ? 0 : 1;
}
