/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

#include <algorithm>
#include <cmath>
#include "EngineConstants.h"

// ============================================================================
// Output law — incoherent, cap-aware (RESEARCH-2.1 §4, PLAN-2.1 T3).
//
//   N     = density · len̄ / fs            len̄ = clamp (size, 5, 2000) ms · fs
//   q     = the stream's quota (one stream in 2.1: quota = grain_limit)
//   M(x)  = (1/x) ∫₀ˣ w²(φ) dφ             mean square over the first x of the window
//   P     = N <= q ? N·μ₂ : q·M(q/N)       capped: q grains alive, each truncated at q/N
//   g_out = 1 / sqrt (max (1, P / μ₂))     μ₂ = μ₂_Hann = 3/8
//
// Single normalisation. ARCHITECTURE §10 divides by μ₂_Hann twice (N_eff =
// q·M/μ₂_Hann, then ·μ₂(m)/μ₂_Hann); that is harmless for Hann (ratio 1) but
// 2.2/2.3 must use THIS form with M_m(x) from the morph bank's cumulative-w²
// table. Computed in double at the grid from LATCHED parameters only — never
// from pool occupancy / liveWeight() (block-size leak).
//
// size_rand is ignored: its (2u − 1)·0.5 spread is symmetric, so the mean
// length is the clamped size (the clamp truncation is not modelled).
// ============================================================================
namespace ogran
{
    inline constexpr double kHannMu2 = 0.375;   // ∫₀¹ w² for w = ½(1 − cos 2πφ)

    /** ∫₀ˣ w²(φ) dφ for the Hann window, closed form, x in [0, 1].
        w² = ¼(3/2 − 2 cos 2πφ + ½ cos 4πφ). x = 1 → 0.375 (to ~1e-17). */
    inline double hannCumW2 (double x) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        return 0.375 * x - std::sin (2.0 * pi * x) / (4.0 * pi) + std::sin (4.0 * pi * x) / (32.0 * pi);
    }

    /** M(x) = hannCumW2 (x) / x, the mean square over the first fraction x. M(1) = μ₂. */
    inline double hannMeanSquare (double x) noexcept
    {
        return x > 0.0 ? hannCumW2 (x) / x : 0.0;
    }

    /** P / μ₂ (the effective incoherent overlap). Exposed for the harness unit rows. */
    inline double effectiveOverlap (double density, double sizeMs, double fs, int quota) noexcept
    {
        const double lenBar = std::clamp (sizeMs, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
        const double n      = density * lenBar / fs;
        const double q      = static_cast<double> (quota);
        const double p      = (n <= q) ? n * kHannMu2 : q * hannMeanSquare (q / n);
        return p / kHannMu2;
    }

    /** g_out for one stream. Hann's window ratio μ₂(m)/μ₂_Hann is exactly 1 in 2.1. */
    inline float streamOutputGain (double density, double sizeMs, double fs, int quota) noexcept
    {
        return static_cast<float> (1.0 / std::sqrt (std::max (1.0, effectiveOverlap (density, sizeMs, fs, quota))));
    }

    /** Hann window at phase φ in [0, 1): 0.5 − 0.5·cos (2πφ), float from the
        double phase. (2.2 replaces this with the window morph bank.) */
    inline float hannWindow (double phase) noexcept
    {
        constexpr float twoPi = 6.28318530717958647692f;
        return 0.5f - 0.5f * std::cos (twoPi * static_cast<float> (phase));
    }
} // namespace ogran
