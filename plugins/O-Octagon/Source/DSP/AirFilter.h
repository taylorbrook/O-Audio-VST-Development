/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include <cmath>

#include "TapCrossfade.h"

namespace oo
{

/**
    v1.15.0 (B8) — §3.5.2's air filter, with its coefficient INTERPOLATED PER SAMPLE.

    ── WHY THIS REPLACED juce::dsp::FirstOrderTPTFilter ──────────────────────────────────────────
    The JUCE class keeps G private and recomputes it only in setCutoffFrequency(), so the cutoff
    could only move once per 64-sample control block — a staircase. Since v1.13.0's listener-
    distance law the filter runs over most of the plane down to 552 Hz, and each stair perturbs the
    output by ΔG·(x − s): a stepped position, a cutoff jump or a Motion-off teleport clicked at up
    to 207x a matched control (REVIEW-v1.13.1 B8, probe DU).

    ── THE ARITHMETIC IS JUCE'S, STATEMENT FOR STATEMENT ─────────────────────────────────────────
    G = g / (1 + g), g = float (tan (pi_double * fc / fs))  — juce_FirstOrderTPTFilter.cpp update()
    v = G·(x − s); y = v + s; s = y + v; lowpass returns y   — processSample()

    With the ramp settled nextGain() returns the target itself — the very float the JUCE class
    would have computed — so every steady-state render is bit-identical to the JUCE filter's. That is what
    keeps the CU/DC digests, BD's bit-transparency and AU's cutoff table where they were.

    ── THE RAMP: 5 ms, SMOOTHSTEP, ONE CHANGE IN FLIGHT ──────────────────────────────────────────
    setCutoff() (only when the solve runs) moves the TARGET. nextGain(), called once per sample in
    both GainStage arms, walks G toward it through a TapCrossfade<float> — the same machinery the
    decorrelator taps and the alignment delay use — so G lands EXACTLY on the solved value and
    holds it. A target that moves mid-ramp waits for the ramp to land, then starts the next one.

    WHY NOT ONE CONTROL BLOCK, MEASURED. The first cut interpolated G across the 64-sample block,
    linear and then smoothstep. Linear left an air 1 -> 0.05 jump at 10x a no-filter control on a
    200 Hz tone (probe DU); smoothstep at 64 samples still left 4.2x — a 552 Hz -> 15 kHz cutoff
    move in 1.3 ms is a click whatever the curve. 5 ms is the length every other ramp in GainStage
    uses, and the curvature it adds falls as 1/N². Under motion the cutoff now lags the puck by at
    most two ramps (~10 ms) — the gain vector itself rides a 5 ms ramp, so the two stay together.

    S IS SMOOTHSTEP: slope zero at both ends, so a jump has no corner (TapCrossfade.h).

    No JUCE, deliberately — the same <cmath>-only rule Decorrelator.h and SourceShaper.h live under.
*/
class AirFilter
{
public:
    static constexpr double kRampMs = 5.0;

    void prepare (double sampleRateToUse) noexcept
    {
        sampleRate = sampleRateToUse;
        ramp.setLength (static_cast<int> (std::lround (kRampMs * 0.001 * sampleRate)));
        ramp.unprime();
        reset();
    }

    void reset (float value = 0.0f) noexcept { s = value; }

    /// The solve's new target. The ramp walks to it; nothing jumps here.
    void setCutoff (float cutoffHz) noexcept
    {
        const auto g = static_cast<float> (std::tan (3.141592653589793 * static_cast<double> (cutoffHz)
                                                     / sampleRate));
        target = g / (1.0f + g);
    }

    /// No ramp: prepare()'s teleport, and a filter (re)starting from not running.
    void snapToTarget() noexcept { ramp.snap (target); }

    /// G for this sample. Advance EXACTLY once per sample, in both arms (QUAL-03).
    float nextGain() noexcept
    {
        const float w = ramp.next (target);

        return w == 1.0f ? ramp.to : ramp.from + w * (ramp.to - ramp.from);
    }

    float process (float x, float G) noexcept
    {
        const float v = G * (x - s);
        const float y = v + s;
        s = y + v;
        return y;
    }

private:
    double             sampleRate { 48000.0 };
    float              s { 0.0f };
    float              target { 0.0f };
    TapCrossfade<float> ramp;
};

} // namespace oo
