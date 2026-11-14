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

    O-ParametricEQ - band runtime, one slot (Stage 2, Phases 2.2 and 2.3)

    One of the 24 bands: the runtime sections of its current design, the
    filter state of two lanes, and (since 2.3) its control ramp and the
    per-sample interpolation between two control ticks.

    Static engine (2.2, unchanged in meaning):

      setDesign   converts a BandDesign to runtime coefficients; state kept
      reset       zeroes the filter state of both lanes, and nothing else
      processLane every section of the band inside one sample loop
                  (sample-major, PLAN-2.2 P5), in place, for one state lane

    Continuous modulation (2.3; PLAN-2.3 P3 - P5):

      setControlGrid  K and the ramp length, once per prepare
      snapTo          design from the given settings now; ramp snapped,
                      interpolation cancelled, settled coefficients; state kept
      snapToTarget    the same toward the band's own latest target, and only
                      if the band is moving: for every path that clears the
                      state (bypass, non-finite input, a band going inactive)
      setTarget       a continuous change: taken up at the next control tick
      controlTick     once per grid tick, see below
      isMoving        derived from live state on every call, never stored
      processPair     both lanes of a Stereo band in one sample loop

    The control tick, in order:
      1. if the last interval interpolated, every section parameter is set to
         that interval's end design exactly (the stored design, not the
         accumulated sum) and the increments are zeroed;
      2. a pending target restarts the ramp from its current value;
      3. moving = the ramp counter is non-zero, or the dynamic offset differs
         from the value read at the last tick. Derived here, from those two
         things, every tick;
      4. moving: the ramp advances; the design settings are the ramp's
         (ControlRamp.h) with the offset summed AFTER the ramp (offsetGainDb);
         one design; it becomes the interval's end, and the next K samples
         interpolate toward it;
      5. not moving, and step 1 ran: toRuntime once from the arrived design.
         The settled path runs from this sample on.
    A band that neither interpolated nor moves does nothing in the tick: no
    design, no conversion. On arrival the design is made from the target's own
    BandSettings, so the settled sections are bit-identical to a static design
    of them.

    Ramping path. While an interval interpolates, stepCoefficients() moves
    the section parameters by one sample and rewrites the runtime
    coefficients; tick() then runs the settled recurrences on them. One step
    per SAMPLE, never one per lane: a band that runs two lanes on the ramping
    path must go through processPair (the Router does), and a loop that forms
    its own input (Mid, Side) calls stepCoefficients() itself before each
    tick(). The position inside the interval lives in the band, so an
    interval may be processed in any number of calls and every sample's
    coefficients depend only on its position in the interval. Once K samples
    have been stepped the coefficients hold until the next control tick.

    The section count and each section's half flags cannot change inside a
    continuous move. controlTick asserts it; without assertions a mismatch
    applies the tick's design at once, with no interpolation for that
    interval and the state kept, and is counted.

    The dynamic offset (dynOffsetDb) is read by the band at snapTo and at each
    control tick. It is ignored for the four gainless types. A change of it
    alone moves the band for one interval, with no 20 ms ramp: a detector's
    output is not smoothed a second time.

    What later phases add and this file does not have: the second slot,
    crossfades and the neutral sleep (2.4). Until then a structural change is
    a snapTo plus reset (a click); the caller decides which change is which.

    State layout: per section, the two lanes sit side by side
    (ic1[2], ic2[2], s[2]) under one shared coefficient set
    (ARCHITECTURE "SIMD: deferred").

    Block-size invariance: what is carried from one call to the next is the
    filter state and, on the ramping path, the interpolated parameters and
    the position in the interval; every sample runs the same statements, so n
    samples in one call and the same n split over several calls give the
    same bits.

    A lane index names a STATE lane. The Router decides which audio goes
    through which state lane (Mid and Side use lane 0).

    The designer is a template parameter of snapTo, snapToTarget and
    controlTick: anything with design (const BandSettings&, BandDesign&).
    The processor passes eq::Designer; a harness may pass a counting wrapper.

    Callers: Router.h, the processor, the designer harness.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "BandParams.h"
#include "ControlRamp.h"
#include "EqTypes.h"
#include "SectionEngine.h"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace eq
{

class Band
{
public:
    /** DSP-07, engine half: the dynamic-EQ gain offset of this band in dB.
        The band reads it at snapTo and at every control tick and sums it into
        the design gain after the control ramp (offsetGainDb). Held at 0 in
        v1; a later detector only has to write it. Nothing in this phase
        writes it. A non-finite value is read as 0. */
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
    // Static engine (2.2)
    //==========================================================================

    /** Takes a design: converts every section to runtime coefficients. The
        filter state is kept. A section count outside 0 ... maxSections is
        clamped. An interpolation in flight is cancelled; the control ramp is
        not touched (use snapTo to move the ramp as well). */
    void setDesign (const BandDesign& design) noexcept
    {
        int count = design.numSections;

        if (count < 0)
            count = 0;

        if (count > maxSections)
            count = maxSections;

        numSections = count;

        for (int i = 0; i < count; ++i)
        {
            current[i] = design.params[static_cast<std::size_t> (i)];
            sections[i] = toRuntime (current[i]);
        }

        for (int i = count; i < maxSections; ++i)
        {
            current[i] = SectionParams();
            sections[i] = SectionRuntime();
        }

        cancelInterpolation();
    }

    /** Zeroes the filter state of both lanes. The design, the ramp and any
        interpolation in flight are kept. */
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
    // Continuous modulation (2.3)
    //==========================================================================

    /** K (samples per control interval, at least 1) and the ramp length in
        ticks: controlGridSize (fs) and rampLengthTicks (fs). Call once per
        prepare, before the band's first snapTo. An interpolation in flight is
        cancelled where it stands. */
    void setControlGrid (int gridSize, int rampTicks) noexcept
    {
        grid = (gridSize < 1) ? 1 : gridSize;
        perSample = 1.0 / static_cast<double> (grid);
        ramp.setLength (rampTicks);
        cancelInterpolation();
    }

    int getControlGrid() const noexcept                         { return grid; }

    /** Designs from `settings` now (plus the band's dynamic offset on a gain
        type) and settles on the result: the ramp is snapped, a pending target
        is dropped, interpolation is cancelled. The filter state is kept; pair
        with reset() where the change is structural. */
    template <typename DesignerType>
    void snapTo (const BandSettings& settings, DesignerType& designer) noexcept
    {
        const BandSettings target = settings;

        ramp.snap (target);
        hasPending = false;

        const double offset = effectiveOffsetDb (target.type);
        lastOffsetDb = offset;

        BandDesign design;
        designer.design (withOffset (target, offset), design);
        setDesign (design);
    }

    /** snapTo the band's own latest target (the pending one, else the
        ramp's), and only if the band is moving. A settled band is left as it
        is: no design. For the paths that clear the state without a fresh
        target. */
    template <typename DesignerType>
    void snapToTarget (DesignerType& designer) noexcept
    {
        if (! isMoving())
            return;

        const BandSettings target = hasPending ? pendingTarget : ramp.getTarget();
        snapTo (target, designer);
    }

    /** A continuous change: frequency, effective gain, Q. Nothing is
        designed here; the next control tick restarts the ramp toward
        `settings`. The last call before a tick wins. Settings equal to the
        ramp's target are not a change: nothing is started, and a pending
        target is dropped. Type and slope index must be those of the last
        snapTo; a change of either is structural (snapTo). */
    void setTarget (const BandSettings& settings) noexcept
    {
        if (sameSettings (ramp.getTarget(), settings))
        {
            hasPending = false;
            return;
        }

        pendingTarget = settings;
        hasPending = true;
    }

    /** Once per grid tick, before the samples of the interval are processed.
        See the file comment for the five steps. */
    template <typename DesignerType>
    void controlTick (DesignerType& designer) noexcept
    {
        // 1: land exactly on the end design of the interval that just ran
        const bool wasInterpolating = interpolating;

        if (wasInterpolating)
        {
            for (int i = 0; i < numSections; ++i)
                current[i] = intervalEnd[i];

            cancelInterpolation();
        }

        // 2
        if (hasPending)
        {
            ramp.retarget (pendingTarget);
            hasPending = false;
        }

        // 3: derived here, from the counter and the offset, at every tick
        const double offset = effectiveOffsetDb (ramp.getTarget().type);
        const bool moving = ramp.isRunning() || ! sameValue (offset, lastOffsetDb);

        // 4
        if (moving)
        {
            ramp.advance();
            lastOffsetDb = offset;

            BandDesign design;
            designer.design (withOffset (ramp.getDesignSettings(), offset), design);

            const bool structureKept = hasStructureOf (design);
            assert (structureKept);

            if (! structureKept)
            {
                // No interpolation between two structures: the design applies at once, state kept.
                ++structureMismatches;
                setDesign (design);
                return;
            }

            for (int i = 0; i < numSections; ++i)
            {
                intervalEnd[i] = design.params[static_cast<std::size_t> (i)];
                increments[i] = sectionIncrements (current[i], intervalEnd[i], perSample);
            }

            interpolating = true;
            samplesToStep = grid;
            return;
        }

        // 5: arrived
        if (wasInterpolating)
            for (int i = 0; i < numSections; ++i)
                sections[i] = toRuntime (current[i]);
    }

    /** True while anything is still to happen without a further edit: an
        interval is interpolating, a target is pending, the ramp is running,
        or the dynamic offset differs from the value read at the last tick.
        Computed from those four on every call. When false, the sections are
        toRuntime of a static design of the target's own settings (plus the
        offset), and a control tick does no design and no conversion. */
    bool isMoving() const noexcept
    {
        return interpolating
            || hasPending
            || ramp.isRunning()
            || ! sameValue (effectiveOffsetDb (ramp.getTarget().type), lastOffsetDb);
    }

    /** True while the current control interval runs the ramping path. */
    bool isInterpolating() const noexcept                       { return interpolating; }

    /** The band's control ramp, read only. */
    const ControlRamp& getRamp() const noexcept                 { return ramp; }

    /** How many control ticks met a design whose section count or half flags
        differed from the band's (0 in every measured run). */
    unsigned long getStructureMismatchCount() const noexcept    { return structureMismatches; }

    /** The parameter half of the ramping tick for every section: one sample
        of motion, then the runtime coefficients. Call once per sample, before
        tick() for every lane of that sample, while isInterpolating().
        processLane and processPair do; a caller with its own sample loop
        (the Router's Mid and Side) does it itself. Does nothing when the band
        is not interpolating or the interval's K samples have been stepped. */
    void stepCoefficients() noexcept
    {
        if (samplesToStep <= 0)
            return;

        --samplesToStep;

        for (int q = 0; q < numSections; ++q)
            rampSection (current[q], increments[q], sections[q]);
    }

    //==========================================================================
    int getNumSections() const noexcept                         { return numSections; }

    /** Runtime coefficients of section `index` (0 ... maxSections - 1; an
        index outside that range reads section 0). On the ramping path these
        are the coefficients of the sample last stepped. */
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
        (0 or 1). Sample-major: every section inside one sample loop. On the
        ramping path the coefficients advance one step per sample here, so
        this is for a band that runs ONE lane; two lanes go through
        processPair. */
    void processLane (int lane, double* samples, int numSamples) noexcept
    {
        if (numSamples <= 0 || numSections <= 0)
            return;

        LaneState st;
        loadLane (lane, st);

        if (! interpolating)
        {
            for (int i = 0; i < numSamples; ++i)
                samples[i] = tick (st, samples[i]);
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
            {
                stepCoefficients();
                samples[i] = tick (st, samples[i]);
            }
        }

        storeLane (lane, st);
    }

    /** Both lanes of a Stereo band: `samples0` on state lane 0, `samples1`
        on state lane 1, in place. Settled: exactly processLane (0) followed
        by processLane (1). Ramping: one sample loop, one coefficient step per
        sample, both lanes on the same coefficients (PLAN-2.3 P4). */
    void processPair (double* samples0, double* samples1, int numSamples) noexcept
    {
        if (! interpolating)
        {
            processLane (0, samples0, numSamples);
            processLane (1, samples1, numSamples);
            return;
        }

        if (numSamples <= 0 || numSections <= 0)
            return;

        LaneState st0;
        LaneState st1;
        loadLane (0, st0);
        loadLane (1, st1);

        for (int i = 0; i < numSamples; ++i)
        {
            stepCoefficients();
            samples0[i] = tick (st0, samples0[i]);
            samples1[i] = tick (st1, samples1[i]);
        }

        storeLane (0, st0);
        storeLane (1, st1);
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

    /** One sample through every section of the band, on a working state and
        on the band's current runtime coefficients. This is the body of every
        sample loop: processLane and processPair use it, and so does the
        Router for Mid and Side, where the input sample is formed from two
        lanes. It never moves a coefficient: on the ramping path the caller
        steps them first (stepCoefficients). */
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

    /** The offset as the band uses it: 0 for a gainless type and for a
        non-finite value. */
    double effectiveOffsetDb (Type type) const noexcept
    {
        if (! isGainType (type))
            return 0.0;

        return std::isfinite (dynOffsetDb) ? dynOffsetDb : 0.0;
    }

    /** The settings with the offset summed into the gain. With the offset
        exactly 0 the settings are returned untouched. */
    static BandSettings withOffset (const BandSettings& settings, double offset) noexcept
    {
        BandSettings s = settings;

        if (! sameValue (offset, 0.0))
            s.gainDbEff = offsetGainDb (s.gainDbEff, offset);

        return s;
    }

    static bool sameSettings (const BandSettings& a, const BandSettings& b) noexcept
    {
        return a.type == b.type
            && a.slopeIndex == b.slopeIndex
            && sameValue (a.freqHz, b.freqHz)
            && sameValue (a.gainDbEff, b.gainDbEff)
            && sameValue (a.q, b.q);
    }

    /** True when `design` has the band's section count and half flags. */
    bool hasStructureOf (const BandDesign& design) const noexcept
    {
        if (design.numSections != numSections)
            return false;

        for (int i = 0; i < numSections; ++i)
        {
            const SectionParams& p = design.params[static_cast<std::size_t> (i)];

            if (p.hasSvf != current[i].hasSvf || p.hasOnePole != current[i].hasOnePole)
                return false;
        }

        return true;
    }

    /** Ends the ramping path where it stands: the coefficients hold. */
    void cancelInterpolation() noexcept
    {
        interpolating = false;
        samplesToStep = 0;

        for (int i = 0; i < maxSections; ++i)
            increments[i] = SectionIncrements();
    }

    int numSections = 0;
    SectionRuntime sections[maxSections];
    SectionState state[maxSections] = {};

    // 2.3: the control ramp, and the interpolation between two control ticks
    ControlRamp ramp;
    BandSettings pendingTarget { Type::bell, 1000.0, 0.0, 1.0, 0 };
    bool hasPending = false;
    double lastOffsetDb = 0.0;                      // the offset read at the last tick or snapTo

    SectionParams current[maxSections] = {};        // the parameters the sections run on
    SectionParams intervalEnd[maxSections] = {};    // the design of the tick that started this interval
    SectionIncrements increments[maxSections];
    bool interpolating = false;
    int samplesToStep = 0;                          // samples of this interval not yet stepped

    int grid = 16;
    double perSample = 1.0 / 16.0;
    unsigned long structureMismatches = 0;
};

} // namespace eq
