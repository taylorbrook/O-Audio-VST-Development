/*
   This file is part of the Ouaricon grain-engine module.
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

    grain-engine - GrainScheduler

    Free-running onset clock with FRACTIONAL onsets. Intervals are doubles and
    are never truncated, so a nominal interval of 137.37 samples puts onset k at
    exactly k·137.37 rather than drifting to k·137. An onset due between two
    samples is reported on the later one, with `frac` = how far past the exact
    onset instant that sample is, in [0, 1). The grain starts with its window
    already `frac` samples in:

        windowPhase = (age + onset.frac) / lengthSamples      age = 0, 1, 2 ...

    Origin: O-Freeze 2.8.0's onset clock (onsetFrac, overshoot carried into the
    next interval), packaged as a class. The two contracts below come from
    O-ReverseDelay's integer scheduler (v1.3.0 / v1.7.2):

      * BLOCK-SIZE INVARIANT. All state advances per sample, and the interval
        callback is keyed on the onset INDEX, so the onset stream (absolute
        sample, frac, index) is identical at every host block size.
      * A DROPPED REQUEST IS A MISSING GRAIN, NEVER A MOVED ONE. The clock is
        re-armed before the capacity check, so overflow can't shift any later
        onset. Drops are counted and returned — count them, don't trust a bound.

    Intervals are latched at each onset (a knob change reaches the gap after the
    next onset, the same click-free rule as every other latched grain field).
    rescheduleNext() re-times the pending gap when that is too slow — e.g. a
    long interval cut short.

    Header-only, no JUCE dependency, no allocation. Audio thread only.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>

namespace ouaricon::grain
{

struct Onset
{
    int      offset = 0;     // sample index within this block
    double   frac   = 0.0;   // [0, 1): samples elapsed since the exact onset instant
    uint64_t index  = 0;     // 0, 1, 2 ... since reset() — use as the CounterRng counter
};

class GrainScheduler
{
public:
    struct Result
    {
        int count   = 0;     // onsets written to the output array
        int dropped = 0;     // onsets the capacity discarded (clock unaffected)
    };

    /** Shortest interval the clock will accept. At least one sample, so at most
        one onset per sample and `frac` stays in [0, 1). */
    static constexpr double kMinInterval = 1.0;

    /** Next onset fires on the next processed sample, at frac 0, index 0. */
    void reset() noexcept
    {
        untilNext   = 0.0;
        sinceOnset  = 0.0;
        latched     = 0.0;
        nextIndex   = 0;
    }

    /** Emits this block's onsets into out[0 .. capacity). `nextInterval` is
        called once per onset as nextInterval (onsetIndex) -> double and returns
        the gap in samples to the FOLLOWING onset; it must be a pure function of
        its argument (and parameters) for block-size invariance — draw jitter
        from CounterRng keyed on the index, never from a sequential generator. */
    template <typename IntervalFn,
              typename = std::enable_if_t<std::is_invocable_r_v<double, IntervalFn&, uint64_t>>>
    Result process (int numSamples, Onset* out, int capacity, IntervalFn&& nextInterval) noexcept
    {
        Result result;

        for (int i = 0; i < numSamples; ++i)
        {
            if (untilNext <= 0.0)
            {
                // untilNext is in (-1, 0] here whenever every gap is >= 1, so
                // frac = -untilNext is in [0, 1). A larger overshoot only arises
                // from rescheduleNext() shortening a gap that had already
                // elapsed: that onset starts clean, now (O-Freeze's rule).
                double frac = 0.0 - untilNext;   // not -untilNext: that is -0.0 at reset
                if (frac >= 1.0)
                    frac = 0.0;

                const uint64_t index = nextIndex++;

                // Re-arm BEFORE the capacity test — see header.
                latched    = std::max (kMinInterval, static_cast<double> (nextInterval (index)));
                untilNext  = latched - frac;
                sinceOnset = frac;

                if (result.count < capacity)
                    out[result.count++] = { i, frac, index };
                else
                    ++result.dropped;
            }

            untilNext  -= 1.0;
            sinceOnset += 1.0;
        }

        return result;
    }

    /** Fixed-interval convenience. */
    Result process (int numSamples, Onset* out, int capacity, double interval) noexcept
    {
        return process (numSamples, out, capacity, [interval] (uint64_t) { return interval; });
    }

    /** Re-times the pending gap as if `interval` had been latched at the last
        onset. If that instant has already passed, the onset fires on the next
        processed sample at frac 0. No effect before the first onset. */
    void rescheduleNext (double interval) noexcept
    {
        if (nextIndex == 0)
            return;

        latched   = std::max (kMinInterval, interval);
        untilNext = latched - sinceOnset;
    }

    /** Symmetric multiplicative jitter with a mean of EXACTLY `nominal` for
        uniform u01 in [0, 1): nominal · (1 + dev·(2u − 1)), dev = amount ·
        maxDeviation. maxDeviation < 1 floors the gap at (1 − maxDeviation) ×
        nominal; 0.9 (O-ReverseDelay's value) caps the instantaneous onset rate
        at 10x nominal. Returns `nominal` unchanged at amount <= 0 — callers that
        need "jitter 0 draws nothing" should skip the draw themselves. */
    static double jittered (double nominal, float amount, float u01,
                            float maxDeviation = 0.9f) noexcept
    {
        if (amount <= 0.0f)
            return nominal;

        const double dev = static_cast<double> (std::min (1.0f, amount) * maxDeviation);
        return std::max (kMinInterval, nominal * (1.0 + dev * (2.0 * static_cast<double> (u01) - 1.0)));
    }

    /** Onsets emitted (or dropped) since reset(). */
    uint64_t getOnsetCount() const noexcept     { return nextIndex; }

    /** Gap latched at the last onset. */
    double getLatchedInterval() const noexcept  { return latched; }

private:
    double   untilNext  = 0.0;   // samples from the current sample to the next onset
    double   sinceOnset = 0.0;   // samples from the last exact onset instant to the current sample
    double   latched    = 0.0;
    uint64_t nextIndex  = 0;
};

} // namespace ouaricon::grain
