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

    O-simpleWavetable perf-check - PERF-02 harness (4-polish PLAN Task 17,
    D-AS; RESEARCH C 2).

    Built WITHOUT OSIW_TEST_HOOKS, so the timed processor code is the
    plugin's own (the hooks add branches in the voice loop). This file must
    never call a ForTesting member. Run the Release build on a quiet machine.

    Metric: "CPU per block as a fraction of the real-time budget" =
    thread-CPU time of one processBlock (CLOCK_THREAD_CPUTIME_ID) x fs / bs.

    Worst-case patch: 16 voices MIDI 84..99 vel 100 (note-ons in block 0),
    Interp On, Band-limit On, 3-bit quantizer, LFO Saw 5 Hz depth 1, Env
    Amount +1, mod env sustain 0.5, amp sustain 1.

    Cells: 44.1 / 48 / 88.2 / 96 kHz x bs 64 / 512 x {steady, crossfade
    once per block (bandlimit toggled before every block)}. Each cell: a
    fresh processor, params set before prepareToPlay, 1 s warm-up
    unmeasured, 4 s measured; median / p99 / max per block; 3 repetitions,
    the repetition with the lowest median is reported (best-of-3).
    Record-only: bs 16 / 32 at 96 kHz, steady and crossfade.

      G-PERF02-LIVE    output finite; RMS (16 voices) / RMS (1 voice) >= 3
                       (96 kHz, bs 512, steady; RESEARCH 4.37). Not timing.
      G-PERF02-SCALE   cost (16) / cost (1) >= 4 (RESEARCH 10.7).
      G-PERF02-STEADY  median AND p99 <= 25 % at every rate x {64, 512}
                       (RESEARCH worst p99 1.02 %).
      G-PERF02-XFADE   crossfade once per block, bs 512: median <= 25 % at
                       every rate (RESEARCH 1.97 % at 96 kHz). bs 64 and
                       below: printed, not gated.

    Duty witness: getrusage user time / steady wall time over the timed
    run must be >= 80 %; otherwise "SKIPPED (machine contended, duty N %)"
    and exit 77 - never a pass. Exit 0 = every gate passes, 1 = a gate
    failed.

    Off by default; -DOUARICON_BUILD_TESTS=ON. Build it in a Release tree.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "BuiltInBanks.h"
#include "PluginProcessor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <sys/resource.h>
#include <time.h>

namespace
{
    using Proc = OSimpleWavetableAudioProcessor;
    namespace ids = OSimpleWavetable::ParamIDs;

    constexpr double kBudget      = 0.25;   // 25 % of one core's real-time budget
    constexpr double kWarmSeconds = 1.0;
    constexpr double kMeasSeconds = 4.0;
    constexpr int    kReps        = 3;
    constexpr int    kVoices      = 16;
    constexpr int    kFirstNote   = 84;     // C6 .. D#7
    constexpr double kMinDuty     = 0.80;

    int gFailures = 0;
    long long gNonFinite = 0;
    long long gSamples = 0;

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
        std::cout.flush();
    }

    std::string fmt (double v, int prec = 2)
    {
        std::ostringstream s;
        s << std::fixed << std::setprecision (prec) << v;
        return s.str();
    }

    std::string pct (double fraction)
    {
        return fmt (100.0 * fraction, 3) + " %";
    }

    double threadCpuSeconds()
    {
        timespec t {};
        clock_gettime (CLOCK_THREAD_CPUTIME_ID, &t);
        return (double) t.tv_sec + 1.0e-9 * (double) t.tv_nsec;
    }

    double wallSeconds()
    {
        using SteadyClock = std::chrono::steady_clock;
        return std::chrono::duration<double> (SteadyClock::now().time_since_epoch()).count();
    }

    double userSeconds()
    {
        rusage ru {};
        getrusage (RUSAGE_SELF, &ru);
        return (double) ru.ru_utime.tv_sec + 1.0e-6 * (double) ru.ru_utime.tv_usec;
    }

    void setParam (Proc& p, const char* id, float realValue)
    {
        if (auto* rp = p.getAPVTS().getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
    }

    enum class Load { steady, crossfade };

    struct CellResult
    {
        double median = 0.0, p99 = 0.0, maxv = 0.0;   // fractions of the block budget
        double rmsOut = 0.0;
    };

    // One run: a fresh processor, the worst-case patch, `voices` note-ons in
    // block 0, warm-up unmeasured, then the measured blocks.
    CellResult runOnce (double fs, int bs, int voices, Load load)
    {
        auto proc = std::make_unique<Proc>();
        Proc& p = *proc;

        setParam (p, ids::bank,        0.0f);   // Sine -> Saw
        setParam (p, ids::position,    0.5f);
        setParam (p, ids::interp,      1.0f);
        setParam (p, ids::bandlimit,   1.0f);
        setParam (p, ids::bitDepth,    14.0f);  // "3"
        setParam (p, ids::lfoShape,    2.0f);   // Saw
        setParam (p, ids::lfoDepth,    1.0f);
        setParam (p, ids::lfoRate,     5.0f);
        setParam (p, ids::envAmount,   1.0f);
        setParam (p, ids::menvSustain, 0.5f);
        setParam (p, ids::ampSustain,  1.0f);

        p.setPlayConfigDetails (0, 2, fs, bs);
        p.prepareToPlay (fs, bs);

        juce::AudioBuffer<float> buf (2, bs);
        juce::MidiBuffer midi;
        midi.ensureSize (4096);

        const int warm = (int) std::lround (kWarmSeconds * fs / (double) bs);
        const int meas = (int) std::lround (kMeasSeconds * fs / (double) bs);
        std::vector<double> t;
        t.reserve ((size_t) meas);
        double ss = 0.0;
        long long n = 0;
        const double scale = fs / (double) bs;   // seconds of CPU -> fraction of this block's budget

        for (int b = 0; b < warm + meas; ++b)
        {
            midi.clear();
            if (b == 0)
                for (int v = 0; v < voices; ++v)
                    midi.addEvent (juce::MidiMessage::noteOn (1, kFirstNote + v, (juce::uint8) 100), 0);
            if (load == Load::crossfade)
                setParam (p, ids::bandlimit, (b & 1) != 0 ? 1.0f : 0.0f);   // a crossfade trigger every block (untimed)

            const double c0 = threadCpuSeconds();
            p.processBlock (buf, midi);
            const double c1 = threadCpuSeconds();

            if (b >= warm)
            {
                t.push_back ((c1 - c0) * scale);
                const float* l = buf.getReadPointer (0);
                for (int i = 0; i < bs; ++i)
                {
                    if (! std::isfinite (l[i]))
                        ++gNonFinite;
                    ss += (double) l[i] * (double) l[i];
                    ++n;
                }
                gSamples += bs;
            }
        }

        p.releaseResources();

        CellResult r;
        if (t.empty())
            return r;
        std::sort (t.begin(), t.end());
        const size_t last = t.size() - 1;
        r.median = t[t.size() / 2];
        r.p99    = t[std::min (last, (size_t) (0.99 * (double) t.size()))];
        r.maxv   = t[last];
        r.rmsOut = n > 0 ? std::sqrt (ss / (double) n) : 0.0;
        return r;
    }

    // Best-of-3: the repetition with the lowest median (its p99 / max with it).
    CellResult runCell (double fs, int bs, int voices, Load load)
    {
        CellResult best;
        bool have = false;
        for (int rep = 0; rep < kReps; ++rep)
        {
            const auto r = runOnce (fs, bs, voices, load);
            if (! have || r.median < best.median)
            {
                best = r;
                have = true;
            }
        }
        return best;
    }

    const char* loadName (Load load)
    {
        return load == Load::steady ? "steady   " : "crossfade";
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    // ONE BuiltInBanks for the whole run: the per-cell processors share it.
    juce::SharedResourcePointer<BuiltInBanks> banks;
    std::cout << "perf-check: built-in banks ready (buildMillis " << fmt (banks->buildMillis, 2) << " ms)\n";
   #if OSIW_TEST_HOOKS
    std::cout << "FAIL G-PERF02-CONFIG: built with OSIW_TEST_HOOKS; the timed code is not the plugin's\n";
    return 1;
   #endif

    const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0 };
    const int    sizes[] = { 64, 512 };
    constexpr int kNumRates = 4, kNumSizes = 2;

    CellResult steady[kNumRates][kNumSizes], xfade[kNumRates][kNumSizes];

    const double user0 = userSeconds();
    const double wall0 = wallSeconds();

    for (int r = 0; r < kNumRates; ++r)
        for (int s = 0; s < kNumSizes; ++s)
        {
            steady[r][s] = runCell (rates[r], sizes[s], kVoices, Load::steady);
            xfade[r][s]  = runCell (rates[r], sizes[s], kVoices, Load::crossfade);
        }

    const CellResult one = runCell (96000.0, 512, 1, Load::steady);   // SCALE / LIVE reference
    const CellResult& sixteen = steady[3][1];                          // 96 kHz, bs 512, 16 voices

    // Record-only: small blocks at 96 kHz.
    CellResult tinyBlocks[2][2];
    const int tinySizes[] = { 16, 32 };
    for (int s = 0; s < 2; ++s)
    {
        tinyBlocks[s][0] = runCell (96000.0, tinySizes[s], kVoices, Load::steady);
        tinyBlocks[s][1] = runCell (96000.0, tinySizes[s], kVoices, Load::crossfade);
    }

    const double userSpent = userSeconds() - user0;
    const double wallSpent = wallSeconds() - wall0;
    const double duty = wallSpent > 0.0 ? userSpent / wallSpent : 0.0;

    //==========================================================================
    // The table (RESEARCH C 2.1 A layout).
    std::cout << "PERF-02: CPU per block as a fraction of the real-time budget (thread CPU of one processBlock x fs / bs),\n"
              << "  16 voices MIDI 84..99 vel 100, worst-case patch; 1 s warm-up, 4 s measured, best-of-3 median (its p99 / max)\n";
    for (const Load load : { Load::steady, Load::crossfade })
        for (int s = 0; s < kNumSizes; ++s)
        {
            std::string row = std::string (loadName (load)) + " bs " + (sizes[s] < 100 ? " " : "") + std::to_string (sizes[s]) + " |";
            for (int r = 0; r < kNumRates; ++r)
            {
                const CellResult& c = load == Load::steady ? steady[r][s] : xfade[r][s];
                row += " " + fmt (rates[r] / 1000.0, 1) + "k med " + pct (c.median) + " p99 " + pct (c.p99)
                     + " max " + pct (c.maxv) + " |";
            }
            info (row);
        }
    for (int s = 0; s < 2; ++s)
        info ("record-only 96k bs " + std::to_string (tinySizes[s]) + ": steady med " + pct (tinyBlocks[s][0].median) + " p99 "
              + pct (tinyBlocks[s][0].p99) + " | crossfade med " + pct (tinyBlocks[s][1].median) + " p99 "
              + pct (tinyBlocks[s][1].p99) + " max " + pct (tinyBlocks[s][1].maxv));
    info ("scaling 96k bs 512 steady: 1 voice med " + pct (one.median) + ", 16 voices med " + pct (sixteen.median));
    info ("crossfade / steady median at 96k: bs 512 " + fmt (xfade[3][1].median / std::max (steady[3][1].median, 1.0e-12), 2)
          + "x, bs 64 " + fmt (xfade[3][0].median / std::max (steady[3][0].median, 1.0e-12), 2) + "x (record-only)");
    info ("duty witness: user " + fmt (userSpent, 2) + " s / wall " + fmt (wallSpent, 2) + " s = " + fmt (100.0 * duty, 1)
          + " % (>= 80 % required for the timing gates)");

    //==========================================================================
    // LIVE is deterministic (not timing): reported even on a contended machine.
    const double rmsRatio = sixteen.rmsOut / std::max (one.rmsOut, 1.0e-12);
    report ("G-PERF02-LIVE", gNonFinite == 0 && gSamples > 0 && rmsRatio >= 3.0,
            std::to_string (gNonFinite) + " non-finite of " + std::to_string (gSamples)
            + " measured samples; RMS 16 voices " + fmt (sixteen.rmsOut, 4) + " / 1 voice " + fmt (one.rmsOut, 4) + " = "
            + fmt (rmsRatio, 2) + " (>= 3; RESEARCH 4.37)");

    if (! (duty >= kMinDuty))
    {
        std::cout << "SKIPPED (machine contended, duty " << fmt (100.0 * duty, 0) << " %)\n";
        std::cout << "perf-check: SKIPPED - timing gates not evaluated; re-run on a quiet machine (exit 77 is never a pass)\n";
        return gFailures == 0 ? 77 : 1;
    }

    const double costRatio = sixteen.median / std::max (one.median, 1.0e-12);
    report ("G-PERF02-SCALE", costRatio >= 4.0,
            "96k bs 512 steady: cost 16 voices " + pct (sixteen.median) + " / 1 voice " + pct (one.median) + " = "
            + fmt (costRatio, 2) + " (>= 4; RESEARCH 10.7)");

    bool steadyOk = true;
    double worstMed = 0.0, worstP99 = 0.0;
    for (int r = 0; r < kNumRates; ++r)
        for (int s = 0; s < kNumSizes; ++s)
        {
            const CellResult& c = steady[r][s];
            if (! (c.median <= kBudget && c.p99 <= kBudget))
                steadyOk = false;
            worstMed = std::max (worstMed, c.median);
            worstP99 = std::max (worstP99, c.p99);
        }
    report ("G-PERF02-STEADY", steadyOk,
            "16 voices, 4 rates x bs {64, 512}: worst median " + pct (worstMed) + ", worst p99 " + pct (worstP99)
            + " (each <= 25 %; RESEARCH worst p99 1.02 %)");

    bool xfadeOk = true;
    double worstXf = 0.0;
    std::string xfRow;
    for (int r = 0; r < kNumRates; ++r)
    {
        const CellResult& c = xfade[r][1];
        if (! (c.median <= kBudget))
            xfadeOk = false;
        worstXf = std::max (worstXf, c.median);
        xfRow += fmt (rates[r] / 1000.0, 1) + "k " + pct (c.median) + "; ";
    }
    report ("G-PERF02-XFADE", xfadeOk,
            "crossfade once per block (bandlimit toggled), bs 512 median: " + xfRow + "worst " + pct (worstXf)
            + " (<= 25 %; RESEARCH 1.97 % at 96k); bs 64 and below printed, not gated");

    std::cout << "perf-check: " << (gFailures == 0 ? std::string ("ALL PASS") : "FAILURES = " + std::to_string (gFailures))
              << "\n";
    return gFailures == 0 ? 0 : 1;
}
