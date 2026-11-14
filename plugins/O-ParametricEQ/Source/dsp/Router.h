/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - placement router (Stage 2, Phases 2.2 and 2.3)

    Applies one band to the working lanes by placement
    (ARCHITECTURE "Processing Chain"). F is the band, lane 0 is L, lane 1 is R.

      Stereo : L <- F(L)        R <- F(R)        state lanes 0 and 1
      Left   : L <- F(L)        R untouched      state lane 0
      Right  : R <- F(R)        L untouched      state lane 1
      Mid    : m = 0.5 (L + R)   d = F(m) - m    L += d    R += d    state lane 0
      Side   : s = 0.5 (L - R)   d = F(s) - s    L += d    R -= d    state lane 0

    Mono layout: every placement runs lane 0 on state lane 0, through the
    same call as a Stereo band's lane 0. Placement is inert there, bit for bit.

    Mid and Side inject only the difference, so the other component
    (0.5 (L - R) for a Mid band, 0.5 (L + R) for a Side band) changes by
    rounding alone. No scratch lane: the loop has m and F(m) in hand.

    A lane the placement does not touch is never read and never written; its
    pointer may be null. The processor relies on this: it converts to double
    only the lanes an active band touches (PLAN-2.2 P4), since a float lane
    that is converted and written back loses its denormal samples under
    flush-to-zero.

    Ramping path (2.3, PLAN-2.3 P4). While a band interpolates between two
    control ticks its coefficients advance once per SAMPLE, whatever the
    placement: a Stereo band goes through Band::processPair (both lanes in
    one loop, on the same coefficients), and the Mid and Side loops step the
    coefficients themselves before each tick. On a settled band every
    placement runs the statements it ran in 2.2, bit for bit. The placement
    algebra above is the same on both paths.

    The placement passed in is the parameter's; the `stereo` flag (the
    negotiated layout, never the buffer width) decides whether it applies.

    Callers: the processor, the designer harness.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "Band.h"
#include "BandParams.h"

namespace eq
{

class Router
{
public:
    /** True when a band with this placement reads and writes lane `lane`
        (0 or 1) on the given layout. Mono: lane 0 only, for every placement. */
    static constexpr bool touchesLane (Placement placement, int lane, bool stereo) noexcept
    {
        if (! stereo)
            return lane == 0;

        if (placement == Placement::left)
            return lane == 0;

        if (placement == Placement::right)
            return lane == 1;

        return lane == 0 || lane == 1;
    }

    /** Runs `band` over `numSamples` samples of the lanes, in place.
        Every lane for which touchesLane is true must point at numSamples
        doubles; the other pointer is not used. A placement value outside the
        enum is treated as Stereo. */
    static void process (Band& band, Placement placement, double* lane0, double* lane1,
                         int numSamples, bool stereo) noexcept
    {
        if (numSamples <= 0)
            return;

        if (! stereo)
        {
            band.processLane (0, lane0, numSamples);
            return;
        }

        switch (placement)
        {
            case Placement::left:
                band.processLane (0, lane0, numSamples);
                return;

            case Placement::right:
                band.processLane (1, lane1, numSamples);
                return;

            case Placement::mid:
                processMid (band, lane0, lane1, numSamples);
                return;

            case Placement::side:
                processSide (band, lane0, lane1, numSamples);
                return;

            case Placement::stereo:
                break;
        }

        // Settled: processLane (0) then processLane (1), as before. Ramping:
        // one loop over both lanes, one coefficient step per sample.
        band.processPair (lane0, lane1, numSamples);
    }

    /** What process() does to the sample at an impulse's index when the band
        is at rest, as scalar algebra: `gain` is Band::firstSampleGain(), l and
        r are the two lane values at that index (r is not used on a mono
        layout). Apply it band by band, in index order, to predict the first
        output sample of a chain (gate G8). */
    static void applyFirstSample (double gain, Placement placement, bool stereo, double& l, double& r) noexcept
    {
        if (! stereo)
        {
            l = gain * l;
            return;
        }

        switch (placement)
        {
            case Placement::left:
                l = gain * l;
                return;

            case Placement::right:
                r = gain * r;
                return;

            case Placement::mid:
            {
                const double m = 0.5 * (l + r);
                const double d = gain * m - m;
                l += d;
                r += d;
                return;
            }

            case Placement::side:
            {
                const double s = 0.5 * (l - r);
                const double d = gain * s - s;
                l += d;
                r -= d;
                return;
            }

            case Placement::stereo:
                break;
        }

        l = gain * l;
        r = gain * r;
    }

private:
    /** m = 0.5 (L + R);  d = F(m) - m;  L += d;  R += d. State lane 0. */
    static void processMid (Band& band, double* left, double* right, int numSamples) noexcept
    {
        if (band.getNumSections() <= 0)
            return;

        Band::LaneState st;
        band.loadLane (0, st);

        if (! band.isInterpolating())
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const double l = left[i];
                const double r = right[i];
                const double m = 0.5 * (l + r);
                const double d = band.tick (st, m) - m;
                left[i] = l + d;
                right[i] = r + d;
            }
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
            {
                band.stepCoefficients();

                const double l = left[i];
                const double r = right[i];
                const double m = 0.5 * (l + r);
                const double d = band.tick (st, m) - m;
                left[i] = l + d;
                right[i] = r + d;
            }
        }

        band.storeLane (0, st);
    }

    /** s = 0.5 (L - R);  d = F(s) - s;  L += d;  R -= d. State lane 0. */
    static void processSide (Band& band, double* left, double* right, int numSamples) noexcept
    {
        if (band.getNumSections() <= 0)
            return;

        Band::LaneState st;
        band.loadLane (0, st);

        if (! band.isInterpolating())
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const double l = left[i];
                const double r = right[i];
                const double s = 0.5 * (l - r);
                const double d = band.tick (st, s) - s;
                left[i] = l + d;
                right[i] = r - d;
            }
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
            {
                band.stepCoefficients();

                const double l = left[i];
                const double r = right[i];
                const double s = 0.5 * (l - r);
                const double d = band.tick (st, s) - s;
                left[i] = l + d;
                right[i] = r - d;
            }
        }

        band.storeLane (0, st);
    }
};

} // namespace eq
