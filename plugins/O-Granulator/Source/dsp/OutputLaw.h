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
#include "WindowMorphBank.h"

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
        double phase. P14: still THE window at step 0 / skew 0 (the bank serves
        every other step). */
    inline float hannWindow (double phase) noexcept
    {
        constexpr float twoPi = 6.28318530717958647692f;
        return 0.5f - 0.5f * std::cos (twoPi * static_cast<float> (phase));
    }

    // ========================================================================
    // Phase 2.2 — R5 partial-coherence law (RESEARCH-2.2 §1.2, PLAN-2.2 P10/T4).
    //
    //   S1 = N·μ₁ (N <= q) | q·(C1w(x)/x) (N > q, x = q/N)     mean Σw
    //   S2 = N·μ₂          | q·(C2w(x)/x)                       mean Σw²
    //   S1 *= E[g_spread] = (4/(π·s))·sin(π·s/4)                 exactly 1 at s = 0
    //   P  = max (c·S2, (c·S1)²) + (1 − c)·S2
    //   g_out = 1 / sqrt (max (1, P / μ₂_Hann))
    //
    // c = 0 reduces BITWISE to the 2.1 streamOutputGain (max (0, 0) + 1·S2 = S2,
    // and S2 is formed with the 2.1 expressions: n·μ₂, q·(C/x)). Step 0 / skew 0
    // uses the Hann closed forms (P14); every other step the bank cumulatives.
    // The 2.1 functions above stay: E0 and the bitwise reduction row use them.
    // ========================================================================

    /** ∫₀ˣ w for Hann, closed form: x/2 − sin (2πx)/(4π). x = 1 → 0.5. */
    inline double hannCumW (double x) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        return 0.5 * x - std::sin (2.0 * pi * x) / (4.0 * pi);
    }

    /** Mean grain count N = density · len̄ / fs — THE 2.1 expression, shared by the
        water-fill demand (T5) and the law, so N <= q compares like with like. */
    inline double streamDemand (double density, double sizeMs, double fs) noexcept
    {
        const double lenBar = std::clamp (sizeMs, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;
        return density * lenBar / fs;
    }

    /** E[g_spread] for the coherent subset (R10): (4/(π·s))·sin(π·s/4); 1 at s <= 0. */
    inline double spreadAmplitudeFactor (double spread) noexcept
    {
        if (! (spread > 0.0))
            return 1.0;
        constexpr double pi = 3.14159265358979323846;
        const double a = 0.25 * pi * spread;
        return std::sin (a) / a;
    }

    /** The window a grain latches: step + tilt (P13). hann() ⇔ the P14 fast path. */
    struct WindowSel
    {
        int   step = 0;
        WindowMorphBank::Tilt tilt {};
        bool hann() const noexcept { return step == 0 && juce::exactlyEqual (tilt.t, 0.5f); }
    };

    /** R5 stream power P (in Σw² units). n = streamDemand, q = the water-filled quota. */
    inline double streamPower (double n, double q, const WindowSel& win, double c, double spread,
                               const WindowMorphBank& bank) noexcept
    {
        double s1 = 0.0, s2 = 0.0;
        if (win.hann())
        {
            if (n <= q) { s1 = n * 0.5;  s2 = n * kHannMu2; }
            else
            {
                const double x = q / n;
                s1 = q * (hannCumW (x) / x);
                s2 = q * (hannCumW2 (x) / x);
            }
        }
        else
        {
            if (n <= q)
            {
                s1 = n * bank.cum (1, win.step, win.tilt, 1.0);
                s2 = n * bank.cum (2, win.step, win.tilt, 1.0);
            }
            else
            {
                const double x = q / n;
                s1 = q * (bank.cum (1, win.step, win.tilt, x) / x);
                s2 = q * (bank.cum (2, win.step, win.tilt, x) / x);
            }
        }
        s1 *= spreadAmplitudeFactor (spread);
        const double cs1 = c * s1;
        return std::max (c * s2, cs1 * cs1) + (1.0 - c) * s2;
    }

    /** g_out from P: 1 / sqrt (max (1, P / μ₂_Hann)) — the 2.1 tail, verbatim. */
    inline float outputGainFromPower (double p) noexcept
    {
        return static_cast<float> (1.0 / std::sqrt (std::max (1.0, p / kHannMu2)));
    }

    /** Coherence c in [0, 1] from latched parameters (RESEARCH-2.2 §1.2):
          c       = (1 − reverse_prob) · c_spray · c_pitch
          c_spray = (1 − clamp (spray·span / (W₀·fs)))²
          c_pitch = (1 − clamp (|pitch|/p₀))² · { scale Off:            (1 − clamp (pitch_rand/r₀))²
                                                  scale on, rand > 0,
                                                    nInWindow > 0:      unison ? 1/nInWindow : 0
                                                  otherwise (empty set,
                                                    no degree, rand 0): 1 } */
    struct Coherence
    {
        static double compute (double reverseProb, double spray, double spanSamples, double fs,
                               double pitchSt, double pitchRandSt, bool scaleOn,
                               int nInWindow, bool unisonActive) noexcept
        {
            auto sq  = [] (double v) noexcept { return v * v; };
            auto c01 = [] (double v) noexcept { return std::clamp (v, 0.0, 1.0); };

            const double cRev   = 1.0 - c01 (reverseProb);
            const double cSpray = sq (1.0 - c01 (spray * spanSamples / (kCoherW0Ms * 0.001 * fs)));
            double cPitch = sq (1.0 - c01 (std::abs (pitchSt) / kCoherP0St));
            if (! scaleOn)
                cPitch *= sq (1.0 - c01 (pitchRandSt / kCoherR0St));
            else if (pitchRandSt > 0.0 && nInWindow > 0)
                cPitch *= unisonActive ? 1.0 / static_cast<double> (nInWindow) : 0.0;
            return cRev * cSpray * cPitch;
        }
    };
} // namespace ogran
