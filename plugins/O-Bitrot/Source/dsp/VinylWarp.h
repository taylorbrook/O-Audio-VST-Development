/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - VinylWarp (v1.7.0, improvement brief item 27b)

    The vinyl family's continuous speed bed: a warped disc. One sinusoid at
    EXACTLY the platter rate — 1/1.8 Hz at 33 1/3, 0.75 Hz at 45, 1.3 Hz at
    78 — because that is what a warp is. A disc pressed off-flat rises and
    falls once per revolution, so the pitch deviation it produces is periodic
    with the revolution and nothing else.

    WHY THIS IS A READ OFFSET AND NOT A RATE MULTIPLIER
    ---------------------------------------------------
    The brief specifies "sinusoidal rate multiplier". A rate multiplier is not
    available in this engine, for the two reasons WowFlutter.h documents at
    length and which apply here verbatim:

      1. ReadHead's lag-overflow clamp is SUPPRESSED while a locked groove owns
         the read rate, and the proof that the suppression is safe is "such a
         loop holds rate at EXACTLY 1.0". A rate multiplier falsifies that
         premise — and a locked groove is precisely the case item 27b is about.
      2. At the engine's lag-0 steady state any rate above 1.0 drives `pos`
         into ReadHead's write-slot pin, which is a zero-order hold: a stutter,
         not a warp.

    Modulating a read OFFSET is the same physics from the other end — pitch
    deviation is the derivative of the delay — so the class specifies its
    excursion by the rate deviation it should produce and inverts the relation:
    a raised cosine of peak-to-peak L at frequency f has peak slope L*pi*f,
    i.e. a rate deviation of L*pi*f/fs. The offset is >= 0 by construction, so
    the head never crosses the write head and every lag budget, loop-wrap test
    and clamp proof in the engine is untouched.

    THE LOCKED-GROOVE IDENTITY
    --------------------------
    The claim worth having is the brief's: a warped locked groove wobbles
    IDENTICALLY every pass. A locked groove jumps back exactly revSamples each
    time the head returns to the loop point, so pos(t + revSamples) == pos(t);
    if the warp's period is also exactly revSamples then offset(t + revSamples)
    == offset(t) and the read position — hence the audio — repeats exactly.
    That is why the phase increment is 2*pi / revolutionSamples() and not
    2*pi*f/fs: the two must be the SAME INTEGER, and VinylGeometry is where
    that integer is defined once for both.

    The accumulator is persistent and free-running. It is deliberately NOT
    reset or re-phased when VINYL_RPM changes, when the depth changes, or when
    a groove locks: a warp is a property of the disc, not of the event, and
    re-phasing it on a parameter touch would be a pitch step.

    At depth exactly 0 the offset is exactly 0.0, so `pos - 0.0` is
    bit-identical to `pos` and CaptureRing's exact-integer fast path still
    fires — the all-off passthrough stays bit-transparent (FUNC-02).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VinylGeometry.h"

class VinylWarp
{
public:
    // Peak read-rate deviation at VINYL_WARP = 100%. The brief's ceiling: a
    // visibly warped LP is a few tenths of a percent, and 0.6% is the "this
    // record is a taco" end of it. At 33 1/3 / 48 kHz that is a 165-sample
    // (3.4 ms) peak-to-peak delay excursion.
    static constexpr double kMaxDeviation = 0.006;

    // Depth fade. Long ON PURPOSE, exactly as in WowFlutter: this ramp
    // multiplies a delay of several milliseconds and its slope IS pitch. A
    // 20 ms fade would be an audible bend every time the knob is touched.
    static constexpr double kDepthRampSeconds = 3.0;

    void prepare (double sampleRate)
    {
        fs        = sampleRate;
        depthStep = 1.0 / juce::jmax (1.0, kDepthRampSeconds * fs);

        setRpm (0);
        reset();
    }

    void reset() noexcept
    {
        depth       = 0.0;
        depthTarget = 0.0;
        phase       = 0.0;
    }

    /** Per-block snapshot. `depth01` arrives already gated by VINYL_ENABLE —
        the caller passes 0 when the family is off, so the warp fades out over
        its own multi-second ramp rather than stepping the delay to zero. */
    void setParams (double depth01, int rpmIndex) noexcept
    {
        depthTarget = juce::jlimit (0.0, 1.0, depth01);
        setRpm (rpmIndex);
    }

    /** True while the warp contributes EXACTLY nothing — the condition under
        which the read is still on CaptureRing's bit-exact integer path. */
    bool isTransparent() const noexcept { return depth == 0.0; }

    /** Per-sample read offset in samples, always >= 0. No RNG: a warp is a
        fixed feature of the disc, and its determinism is what makes the
        locked-groove repeat exact. The oscillator runs unconditionally so the
        bed is phase-continuous across depth and RPM changes; only the final
        scale is gated. */
    double nextOffsetSamples() noexcept
    {
        phase += phaseInc;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;

        // Depth ramp, landing EXACTLY on the target — an epsilon residue here
        // would keep the read fractional forever and silently cost FUNC-02.
        if (depth < depthTarget)      depth = juce::jmin (depthTarget, depth + depthStep);
        else if (depth > depthTarget) depth = juce::jmax (depthTarget, depth - depthStep);

        if (depth == 0.0)
            return 0.0;

        return depth * lagAmp * 0.5 * (1.0 - std::cos (phase));
    }

private:
    // Period and excursion for an RPM index. Recomputed on every block; both
    // are pure functions of (fs, rpmIndex), so a block that does not change
    // the speed writes back the same numbers and the phase carries on.
    void setRpm (int rpmIndex) noexcept
    {
        const int revSamples = VinylGeometry::revolutionSamples (fs, rpmIndex);

        // 2*pi / revSamples, NOT 2*pi*f/fs: the LFO period must be the same
        // INTEGER as the groove-jump distance for the locked-groove repeat to
        // be exact rather than merely close (see the header note).
        phaseInc = juce::MathConstants<double>::twoPi / static_cast<double> (revSamples);

        // deviation -> peak-to-peak lag, inverting rate = L*pi*f/fs with
        // f = fs/revSamples. The fs cancels: L = deviation * revSamples / pi.
        lagAmp = kMaxDeviation * static_cast<double> (revSamples)
                 / juce::MathConstants<double>::pi;
    }

    double fs = 48000.0;

    double depth       = 0.0;
    double depthTarget = 0.0;
    double depthStep   = 1.0;

    double phase    = 0.0;
    double phaseInc = 0.0;
    double lagAmp   = 0.0;
};
