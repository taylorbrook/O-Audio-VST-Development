/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    Terrains.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    The 6 analytic terrains (ARCHITECTURE Core 3, FUNC-02) under the πF
    convention (F = 1 ⇒ one full sine cycle across [−1, 1]). Clean-room
    formulas; Mitsuhashi's polynomial is from the 1982 paper via Mills &
    de Souza 1999. Every branch returns a value clamped to [−1, 1].

    Audio-thread budget (DSP-05): no pow (Cosine Wells' fractional power is a
    lerp between integer powers by repeated multiplication), no allocation.
    Trig via std::sinf / cosf (RESEARCH §2.3: Apple libm is fast enough).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

// 0–6 = the osc?Terrain choice list (asserted in PluginProcessor.cpp);
// HarnessIdentityX is harness-only and never reachable from the parameter —
// the voice maps choice index → kind through jlimit (0, 6, …).
enum class TerrainKind
{
    SineProduct = 0,
    RadialRings,
    Saddle,
    RidgedCosines,
    Mitsuhashi,
    CosineWells,
    Imported,
    HarnessIdentityX = 100
};

static constexpr int kNumAnalyticTerrains = 6;

namespace TerrainDetail
{
    constexpr float kPi    = 3.14159265358979f;
    constexpr float kTwoPi = 6.28318530717959f;

    /** Triangle wrap of u into [−1, 1] with period 2 (tri (0) = 0, tri (0.5) = 1, tri (1) = 0, tri (1.5) = −1). */
    inline float tri (float u) noexcept
    {
        // Map to a saw in [0, 1) with period 2 then fold.
        float t = 0.5f * u - 0.25f;            // shift so tri (0) = 0 rising
        t -= std::floor (t);                    // [0, 1)
        return 4.0f * std::abs (t - 0.5f) - 1.0f; // [−1, 1]
    }

    /** hᵖ for p ∈ [1, 7] as a lerp between integer powers (no pow). */
    inline float fracPow (float h, float p) noexcept
    {
        const int   pi = juce::jlimit (1, 6, int (p));
        const float f  = p - float (pi);
        float lo = h;
        for (int i = 1; i < pi; ++i) lo *= h;
        const float hi = lo * h;
        return lo + f * (hi - lo);
    }
}

/** f(x, y) ∈ [−1, 1] over [−1, 1]² at effective spatial frequency F with shape inputs mx, my ∈ [0, 1]. */
inline float terrain (TerrainKind kind, float x, float y, float F, float mx, float my) noexcept
{
    using namespace TerrainDetail;
    float v;

    switch (kind)
    {
        case TerrainKind::SineProduct:
            v = std::sin (kPi * F * x + kPi * (mx - 0.5f)) * std::sin (kPi * F * (0.5f + my) * y);
            break;

        case TerrainKind::RadialRings:
        {
            const float rho = std::sqrt (x * x + y * y);
            v = std::cos (kTwoPi * F * (0.5f + mx) * rho + kTwoPi * (my - 0.5f));
            break;
        }

        case TerrainKind::Saddle:
            v = std::sin (kPi * F * (x * x - y * y + 2.0f * (mx - 0.5f) * x * y) + kPi * (my - 0.5f));
            break;

        case TerrainKind::RidgedCosines:
            v = 1.0f - std::abs (std::cos (kPi * F * x + kPi * (mx - 0.5f)))
                     - std::abs (std::cos (kPi * F * (0.5f + my) * y));
            break;

        case TerrainKind::Mitsuhashi:
        {
            const float u = tri (F * x);
            const float w = tri (F * (0.5f + my) * y);
            const float alpha = mx * 1.5707963267948966f;
            const float rot = u * std::cos (alpha) - w * std::sin (alpha);
            v = 1.747f * rot * (u * u - 1.0f) * (w * w - 1.0f);
            break;
        }

        case TerrainKind::CosineWells:
        {
            // πF, not ARCH Core 3's 2πF: the 2πF form puts TWO cycles across [−1, 1] at
            // F = 1, contradicting the stated convention (F = 1 ⇒ one full cycle) and
            // failing the DSP-06 gate on 10 of 11 orbits at the locked defaults; under
            // πF h1 is the strongest partial on all 11 (Round A SUMMARY, deviation 1).
            const float h = 0.5f * (1.0f + std::cos (kPi * F * x) * std::cos (kPi * F * (0.5f + mx) * y));
            const float p = 1.0f + 6.0f * my;
            v = 1.0f - 2.0f * fracPow (h, p);
            break;
        }

        case TerrainKind::Imported:
            v = 0.0f;   // Round B (Phase 2.5) lands the PNG path; the choice is legal, the terrain is silent
            break;

        case TerrainKind::HarnessIdentityX:
            v = x;      // harness-only: θ-parity reference (H1)
            break;

        default:
            v = 0.0f;
            break;
    }

    return juce::jlimit (-1.0f, 1.0f, v);
}
