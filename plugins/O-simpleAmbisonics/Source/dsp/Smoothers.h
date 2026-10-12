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

    Smoothers — the control-rate smoothing primitives (Phase 2.1).

    THE τ = 20 ms CONTRACT. Every control quantity in the engine moves with the
    same time constant (ARCHITECTURE.md §Angle Smoothing System):

      one-poles  (angles, elevation, distance gain g, air-LP coefficient G,
                  air-LP wet mix m):      cur += k·(target − cur)
      ramps      (order gains, decoder
                  mix):                   cur moves by stepSize toward target,
                                          arriving EXACTLY (F1)

    k and stepSize are computed ONCE by the engine from the 64-sample CONTROL
    GRID (PLAN-2.1 A1, the D5 amendment):

      kOnePole  = 1 − exp(−kControlGrid / (τ·fs))     (0.0645 at 48 kHz)
      kRampStep =          kControlGrid / (τ·fs)      (1/15   at 48 kHz)

    — NEVER from the slice length. A smoother keyed to the host block makes
    the render a function of the host's slicing whenever a parameter moves
    (RESEARCH-2.1 R3); on the grid the render is bit-identical across any
    slicing (harness P2b / P3b).

    Snap rule (A2): a one-pole snaps to its target when
    |target − cur| ≤ 1e-6·max(1, |target|), so a static render after a move is
    bit-identical to a fresh render at the same setting. Ramps arrive exactly
    by construction.

    Headless: <cmath>, <algorithm> only. No JUCE.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>

namespace OSimpleAmbisonics::dsp
{

// The control grid: one control step every kControlGrid samples, on a grid
// that persists across process() calls. Equal to kPartition (SpatialEngine.h
// static_asserts it) so the 2.3 UPOLS step lands on the same grid.
inline constexpr int kControlGrid = 64;

// τ for every smoother, seconds.
inline constexpr double kSmoothingTauSeconds = 0.02;

// Wrap an angle in degrees to [−180, 180). NOTE the half-open interval (A6 /
// RESEARCH-2.1 F4): +180 maps to −180. Harmless for the smoother (only the
// delta matters); the published azEff for a source parked at exactly +180°
// reads −180°, and Stage 3 may format −180 as 180 if the UI wants it.
inline float wrapTo180 (float x) noexcept
{
    return x - 360.0f * std::floor ((x + 180.0f) / 360.0f);
}

// Plain one-pole with the A2 snap. step() returns the new current value.
struct OnePoleSmoother
{
    float cur    = 0.0f;
    float target = 0.0f;

    void snapTo (float v) noexcept { cur = target = v; }

    float step (float t, float k) noexcept
    {
        target = t;
        cur += k * (t - cur);

        if (std::abs (t - cur) <= 1.0e-6f * std::max (1.0f, std::abs (t)))
            cur = t;

        return cur;
    }
};

// One-pole on the WRAPPED delta: a +170° → −170° target moves +20° through
// 180°, never −340° through 0° (DSP-06). Result re-wrapped to [−180, 180).
struct WrapSmoother
{
    float cur    = 0.0f;
    float target = 0.0f;

    void snapTo (float v) noexcept { cur = target = wrapTo180 (v); }

    float step (float t, float k) noexcept
    {
        target = t;
        const float d = wrapTo180 (t - cur);
        cur = wrapTo180 (cur + k * d);

        if (std::abs (wrapTo180 (t - cur)) <= 1.0e-6f * std::max (1.0f, std::abs (t)))
            cur = wrapTo180 (t);

        return cur;
    }
};

// Linear ramp with EXACT arrival: the final step assigns the target when the
// remaining distance is within one step (F1). Used for the order gains and the
// decoder mix, whose "exactly 0 / exactly 1" criteria a one-pole can never
// meet (1.75 s to flush-to-zero at k = 0.0645).
struct LinearRamp
{
    float cur = 0.0f;

    void snapTo (float v) noexcept { cur = v; }

    float step (float target, float stepSize) noexcept
    {
        const float d = target - cur;
        cur = std::abs (d) <= stepSize ? target : cur + std::copysign (stepSize, d);
        return cur;
    }
};

} // namespace OSimpleAmbisonics::dsp
