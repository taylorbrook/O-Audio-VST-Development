/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    DistanceModel — "distance = level + air", applied to the mono source BEFORE
    encoding so it is one filter, not sixteen (ARCHITECTURE §Distance Model,
    DSP-05).

      gain   g(d)  = 1.25 / (d + 0.25)              soft-knee 1/d, +8.87 dB at 0.2 m,
                                                     0 dB at 1 m, −24.19 dB at 20 m
      cutoff f_c(d) = 20000 · min(1, d^−0.5) Hz,     clamped to 0.45·fs BEFORE tan
                                                     (pattern_sc_filter_needs_nyquist_clamp)
      air LP        = own TPT one-pole (D4):          v = (u − z)·G; lp = v + z; z = lp + v
                      G = t/(1 + t), t = tan(π f_c/fs)
      wet mix m(d)  = clamp(d − 1, 0, 1)   (D7):     y = u + m·(lp − u)

    D7 exists for EXACTNESS, not for tone: at d ≤ 1 m the output is bit-equal
    to g·x (the harness P1 anchor and the DSP-05 "exactly 0 dB" clause assert
    it). The bilinear (TPT) pole parked at the 0.45·fs clamp is only
    −0.06 dB at 10 kHz (A15 — RESEARCH-2.1 R5; the −0.85 dB figure quoted in
    CONTEXT-2.1 D7 is the analog prototype, not this filter).

    Control grid: g, G and m are one-pole smoothed ONCE PER CONTROL STEP (k from
    the 64-sample grid, Smoothers.h) and lerped per sample in the A3 form
    prev + t·(cur − prev). The coefficient G is smoothed, not the cutoff, so
    there is exactly one tan() per control step and none per sample.

    clampedCutoffHz() exposes the clamp TARGET for the DSP-05 clamp gate
    (pattern_downstream_guard_masks_defect_gate_the_clamp_target).

    Headless: <cmath>, <algorithm> only. No JUCE.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>

#include "Smoothers.h"

namespace OSimpleAmbisonics::dsp
{

struct DistanceModel
{
    static constexpr double kNyquistFraction = 0.45;
    static constexpr double kPi              = 3.14159265358979323846;

    static float gainFor (float d) noexcept
    {
        return 1.25f / (d + 0.25f);
    }

    // Clamped to kNyquistFraction·fs — the clamp happens here, before tan().
    static float cutoffFor (float d, double sampleRate) noexcept
    {
        const float open = 20000.0f * std::min (1.0f, 1.0f / std::sqrt (d));
        return std::min (open, (float) (kNyquistFraction * sampleRate));
    }

    static float tptCoefficient (float cutoffHz, double sampleRate) noexcept
    {
        const float t = std::tan ((float) (kPi * (double) cutoffHz / sampleRate));
        return t / (1.0f + t);
    }

    static float mixFor (float d) noexcept
    {
        return std::clamp (d - 1.0f, 0.0f, 1.0f);
    }

    void prepare (double sampleRate) noexcept
    {
        sampleRateHz = sampleRate;
        z = 0.0f;
    }

    // Snap every smoother to the setting (A7) and clear the filter state.
    void snapTo (float d) noexcept
    {
        clampedFc = cutoffFor (d, sampleRateHz);
        g.snapTo (gainFor (d));
        G.snapTo (tptCoefficient (clampedFc, sampleRateHz));
        m.snapTo (mixFor (d));
        prevG = curG = g.cur;
        prevCoef = curCoef = G.cur;
        prevMix = curMix = m.cur;
        z = 0.0f;
    }

    // One control step: previous endpoints ← current, then advance.
    void controlStep (float d, float k) noexcept
    {
        prevG    = curG;
        prevCoef = curCoef;
        prevMix  = curMix;

        clampedFc = cutoffFor (d, sampleRateHz);
        curG      = g.step (gainFor (d), k);
        curCoef   = G.step (tptCoefficient (clampedFc, sampleRateHz), k);
        curMix    = m.step (mixFor (d), k);
    }

    // t = (j + 1)/64 within the control grid (A5).
    float processSample (float x, float t) noexcept
    {
        const float gS = prevG + t * (curG - prevG);
        const float GS = prevCoef + t * (curCoef - prevCoef);
        const float mS = prevMix + t * (curMix - prevMix);

        const float u  = x * gS;
        const float v  = (u - z) * GS;
        const float lp = v + z;
        z = lp + v;

        return u + mS * (lp - u);
    }

    // Clears the filter state only; the engine re-snaps the smoothers.
    void reset() noexcept { z = 0.0f; }

    float clampedCutoffHz() const noexcept { return clampedFc; }

    OnePoleSmoother g, G, m;

private:
    double sampleRateHz = 48000.0;
    float  clampedFc    = 20000.0f;
    float  prevG = 1.0f, curG = 1.0f;
    float  prevCoef = 0.0f, curCoef = 0.0f;
    float  prevMix = 0.0f, curMix = 0.0f;
    float  z = 0.0f;
};

} // namespace OSimpleAmbisonics::dsp
