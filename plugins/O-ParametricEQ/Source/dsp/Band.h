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

    O-ParametricEQ - band runtime, one slot (Stage 2, Phase 2.2)

    One of the 24 bands: the runtime sections of its current design and the
    filter state of two lanes. Static engine only:

      setDesign   converts a BandDesign to runtime coefficients; state kept
      reset       zeroes the state of both lanes
      processLane every section of the band inside one sample loop
                  (sample-major, PLAN-2.2 P5), in place, for one state lane

    What later phases add and this file does not have: parameter ramps and
    the control grid (2.3), the second slot, crossfades and the neutral sleep
    (2.4). Until then a continuous change is a setDesign with the state kept
    (an audible step) and a structural change is a setDesign plus reset
    (a click); the caller decides which.

    State layout: per section, the two lanes sit side by side
    (ic1[2], ic2[2], s[2]) under one shared coefficient set, so a 2-wide
    lane-pair path can be added later without restructuring
    (ARCHITECTURE "SIMD: deferred").

    Block-size invariance: the only thing carried from one call to the next
    is the filter state, and every sample runs the same statements, so n
    samples in one call and the same n split over several calls give the
    same bits.

    A lane index names a STATE lane. The Router decides which audio goes
    through which state lane (Mid and Side use lane 0).

    Callers: Router.h, the processor, the designer harness.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"
#include "SectionEngine.h"

#include <cstddef>

namespace eq
{

class Band
{
public:
    /** DSP-07, engine half: the dynamic-EQ gain offset of this band in dB,
        summed into the effective gain where the band's input is built
        (makeBandInput). Held at 0 in v1; a later detector only has to write
        it. Nothing in this phase writes it. */
    double dynOffsetDb = 0.0;

    /** A working copy of one lane's state, section by section. The sample
        loops run on a copy so the state can live in registers; see loadLane,
        storeLane and tick. */
    struct LaneState
    {
        double ic1[maxSections];
        double ic2[maxSections];
        double s[maxSections];
    };

    //==========================================================================
    /** Takes a design: converts every section to runtime coefficients. The
        filter state is kept. A section count outside 0 ... maxSections is
        clamped. */
    void setDesign (const BandDesign& design) noexcept
    {
        int count = design.numSections;

        if (count < 0)
            count = 0;

        if (count > maxSections)
            count = maxSections;

        numSections = count;

        for (int i = 0; i < count; ++i)
            sections[i] = toRuntime (design.params[static_cast<std::size_t> (i)]);

        for (int i = count; i < maxSections; ++i)
            sections[i] = SectionRuntime();
    }

    /** Zeroes the filter state of both lanes. The design is kept. */
    void reset() noexcept
    {
        for (int i = 0; i < maxSections; ++i)
        {
            state[i].ic1[0] = 0.0;  state[i].ic1[1] = 0.0;
            state[i].ic2[0] = 0.0;  state[i].ic2[1] = 0.0;
            state[i].s[0]   = 0.0;  state[i].s[1]   = 0.0;
        }
    }

    //==========================================================================
    int getNumSections() const noexcept                         { return numSections; }

    /** Runtime coefficients of section `index` (0 ... maxSections - 1; an
        index outside that range reads section 0). */
    const SectionRuntime& getSection (int index) const noexcept
    {
        return sections[(index >= 0 && index < maxSections) ? index : 0];
    }

    /** The band's first-sample gain: the product over its sections of
        (mL a3 + mB a2 + mH (1 - k a2 - a3)) for an SVF half and
        (cL Gc + cH (1 - Gc)) for a one-pole half. With the band at rest, the
        output sample at an impulse's index is the impulse times this value
        (gate G8). 1 for a band with no section. */
    double firstSampleGain() const noexcept
    {
        double gain = 1.0;

        for (int i = 0; i < numSections; ++i)
            gain *= sectionFirstSampleGain (sections[i]);

        return gain;
    }

    //==========================================================================
    /** Runs the band over `numSamples` samples in place, on state lane `lane`
        (0 or 1). Sample-major: every section inside one sample loop. */
    void processLane (int lane, double* samples, int numSamples) noexcept
    {
        if (numSamples <= 0 || numSections <= 0)
            return;

        LaneState st;
        loadLane (lane, st);

        for (int i = 0; i < numSamples; ++i)
            samples[i] = tick (st, samples[i]);

        storeLane (lane, st);
    }

    /** Copies state lane `lane` (0 or 1) out. */
    void loadLane (int lane, LaneState& out) const noexcept
    {
        const int l = (lane != 0) ? 1 : 0;

        for (int i = 0; i < maxSections; ++i)
        {
            out.ic1[i] = state[i].ic1[l];
            out.ic2[i] = state[i].ic2[l];
            out.s[i]   = state[i].s[l];
        }
    }

    /** Copies a working state back into state lane `lane` (0 or 1). */
    void storeLane (int lane, const LaneState& in) noexcept
    {
        const int l = (lane != 0) ? 1 : 0;

        for (int i = 0; i < maxSections; ++i)
        {
            state[i].ic1[l] = in.ic1[i];
            state[i].ic2[l] = in.ic2[i];
            state[i].s[l]   = in.s[i];
        }
    }

    /** One sample through every section of the band, on a working state.
        This is the body of every sample loop: processLane uses it, and so
        does the Router for Mid and Side, where the input sample is formed
        from two lanes. */
    double tick (LaneState& st, double x) const noexcept
    {
        double v = x;

        for (int q = 0; q < numSections; ++q)
        {
            const SectionRuntime& sec = sections[q];

            if (sec.hasSvf)
                v = svfTick (sec.svf, st.ic1[q], st.ic2[q], v);

            if (sec.hasOnePole)
                v = onePoleTick (sec.onePole, st.s[q], v);
        }

        return v;
    }

private:
    /** Per section: the two lanes side by side. */
    struct SectionState
    {
        double ic1[2];
        double ic2[2];
        double s[2];
    };

    int numSections = 0;
    SectionRuntime sections[maxSections];
    SectionState state[maxSections] = {};
};

} // namespace eq
