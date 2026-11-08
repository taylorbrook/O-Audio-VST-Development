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
#include <vector>
#include "WindowLut.h"      // grain-engine 1.1.0 (Expo-Decay shape, Tilt warp)
#include "EngineConstants.h"

// ============================================================================
// WindowMorphBank (gap G1, plugin-local; ARCHITECTURE §8 segments, RESEARCH-2.2
// R6, PLAN-2.2 T3).
//
// 65 tables × 2048 points, step k ↔ m = k / 64:
//   A  m ∈ [0, ⅓]   Tukey α 1 → 0.25 (α = 1 is Hann)
//   B  m ∈ [⅓, ⅔]   lerp Tukey 0.25 → Trapezoid (12.5 % linear ramps)
//   C  m ∈ [⅔, 1]   lerp Trapezoid → WindowLut Expo-Decay (2 % raised-cosine
//                   attack, e^(−5u)) — COPIED from WindowLut's own table, not
//                   re-derived (the window that is heard is WindowLut's).
// Read like WindowLut: linear interpolation at phase · 2047 after the per-grain
// Tilt warp (peak at 0.5 + 0.45·skew; {0.5, 1, 1} is the bitwise identity).
//
// Cumulatives (the R5 capped law needs ∫w and ∫w², not just the moments):
//   C1[k][i] = ∫₀^{i/2048} w,  C2[k][i] = ∫₀^{i/2048} w²   (2049 entries)
// integrated in DOUBLE over the piecewise-linear function the renderer reads
// (each 1/2047 segment integrated exactly — the trapezoid rule for w, the exact
// quadratic for w²), stored as float. C(1) = μ.
//
// Warped cumulative (R6, exact): q = warp (x), Ch = C(0.5)
//   ∫₀ˣ w_t = x <= t ? 2t·C(q) : 2t·Ch + 2(1−t)·(C(q) − Ch)
//
// Memory ≈ 1.6 MB, heap. Built ONCE in the constructor (message thread);
// never rebuilt in prepare(). The audio thread only reads.
// ============================================================================
namespace ogi
{
    class WindowMorphBank
    {
    public:
        using Tilt = ouaricon::grain::WindowLut::Tilt;

        static constexpr int kSteps  = kWindowSteps;
        static constexpr int kPoints = kWindowPoints;
        static constexpr int kCum    = kWindowPoints + 1;

        WindowMorphBank()
            : tables (static_cast<size_t> (kSteps * kPoints), 0.0f),
              c1 (static_cast<size_t> (kSteps * kCum), 0.0f),
              c2 (static_cast<size_t> (kSteps * kCum), 0.0f)
        {
            build();
        }

        //======================================================================
        // Audio thread (renderer).
        const float* table (int step) const noexcept
        {
            return tables.data() + static_cast<size_t> (std::clamp (step, 0, kSteps - 1)) * kPoints;
        }

        /** One warp + clamp + lerp (the WindowLut::readAt convention). */
        static float read (const float* tab, const Tilt& tilt, double phase) noexcept
        {
            const float p    = std::clamp (tilt.warp (static_cast<float> (phase)), 0.0f, 1.0f);
            const float fpos = p * static_cast<float> (kPoints - 1);
            const int   i0   = static_cast<int> (fpos);
            const int   i1   = std::min (i0 + 1, kPoints - 1);
            const float frac = fpos - static_cast<float> (i0);
            return tab[i0] + frac * (tab[i1] - tab[i0]);
        }

        //======================================================================
        // Grid / message thread (law, gates). All double.

        /** Unwarped cumulative C_k (x), k = 1 (∫w) or 2 (∫w²), x in [0, 1]. */
        double cumUnwarped (int k, int step, double x) const noexcept
        {
            const float* c = (k == 1 ? c1 : c2).data() + static_cast<size_t> (std::clamp (step, 0, kSteps - 1)) * kCum;
            const double u  = std::clamp (x, 0.0, 1.0) * static_cast<double> (kPoints);
            const int    i0 = std::min (static_cast<int> (u), kPoints - 1);
            const double f  = u - static_cast<double> (i0);
            return static_cast<double> (c[i0]) + f * (static_cast<double> (c[i0 + 1]) - static_cast<double> (c[i0]));
        }

        /** Warped cumulative ∫₀ˣ w_t^k (R6 two-piece form). */
        double cum (int k, int step, const Tilt& tilt, double x) const noexcept
        {
            x = std::clamp (x, 0.0, 1.0);
            const double t  = static_cast<double> (tilt.t);
            const double q  = std::min (x, t) * (0.5 / t) + std::max (x - t, 0.0) * (0.5 / (1.0 - t));
            const double cq = cumUnwarped (k, step, q);
            if (x <= t)
                return 2.0 * t * cq;
            const double ch = cumUnwarped (k, step, 0.5);
            return 2.0 * t * ch + 2.0 * (1.0 - t) * (cq - ch);
        }

        /** Unwarped whole-window moments μ₁ = C1(1), μ₂ = C2(1). */
        double mu1 (int step) const noexcept { return cumUnwarped (1, step, 1.0); }
        double mu2 (int step) const noexcept { return cumUnwarped (2, step, 1.0); }

        /** The step a window value 0..1 latches (P13 / T7). */
        static int stepFor (float window) noexcept
        {
            return std::clamp (static_cast<int> (std::lround (static_cast<double> (window) * (kSteps - 1))), 0, kSteps - 1);
        }

        /** Window shape value at phase φ for morph m (double, the build formula).
            expo = WindowLut's Expo-Decay table value at the same point. */
        static double shapeAt (double m, double phi, double expo) noexcept
        {
            constexpr double third = 1.0 / 3.0;
            if (m <= third)
            {
                const double alpha = 1.0 - 0.75 * (m / third);
                return tukey (alpha, phi);
            }
            if (m <= 2.0 * third)
            {
                const double s = (m - third) / third;
                return (1.0 - s) * tukey (0.25, phi) + s * trapezoid (phi);
            }
            const double s = std::min (1.0, (m - 2.0 * third) / third);
            return (1.0 - s) * trapezoid (phi) + s * expo;
        }

    private:
        static double tukey (double alpha, double phi) noexcept
        {
            constexpr double pi = 3.14159265358979323846;
            const double te = 0.5 * alpha;   // taper length at each end
            const double u  = std::min (phi, 1.0 - phi);
            return u < te ? 0.5 * (1.0 - std::cos (pi * u / te)) : 1.0;
        }

        static double trapezoid (double phi) noexcept
        {
            constexpr double ramp = 0.125;
            const double u = std::min (phi, 1.0 - phi);
            return u < ramp ? u / ramp : 1.0;
        }

        void build()
        {
            const ouaricon::grain::WindowLut lut (kPoints);   // message thread: allocates
            const float* expo = lut.getTable (ouaricon::grain::WindowLut::expoDecay);

            const double h = 1.0 / static_cast<double> (kPoints - 1);
            std::vector<double> p1 (static_cast<size_t> (kPoints)), p2 (static_cast<size_t> (kPoints));

            for (int k = 0; k < kSteps; ++k)
            {
                const double m = static_cast<double> (k) / static_cast<double> (kSteps - 1);
                float* const w = tables.data() + static_cast<size_t> (k) * kPoints;
                for (int i = 0; i < kPoints; ++i)
                {
                    const double phi = static_cast<double> (i) / static_cast<double> (kPoints - 1);
                    w[i] = static_cast<float> (std::clamp (shapeAt (m, phi, static_cast<double> (expo[i])), 0.0, 1.0));
                }

                // Exact integrals of the lerped float table, knot prefix sums.
                p1[0] = p2[0] = 0.0;
                for (int j = 0; j + 1 < kPoints; ++j)
                {
                    const double a = w[j], b = w[j + 1];
                    p1[(size_t) j + 1] = p1[(size_t) j] + h * 0.5 * (a + b);
                    p2[(size_t) j + 1] = p2[(size_t) j] + h * (a * a + a * b + b * b) / 3.0;
                }

                float* const o1 = c1.data() + static_cast<size_t> (k) * kCum;
                float* const o2 = c2.data() + static_cast<size_t> (k) * kCum;
                for (int i = 0; i < kCum; ++i)
                {
                    const double x  = static_cast<double> (i) / static_cast<double> (kPoints);
                    const double u  = x * static_cast<double> (kPoints - 1);
                    const int    j  = std::min (static_cast<int> (u), kPoints - 2);
                    const double f  = u - static_cast<double> (j);
                    const double a  = w[j], d = static_cast<double> (w[j + 1]) - a;
                    const double s1 = p1[(size_t) j] + h * (a * f + 0.5 * d * f * f);
                    const double s2 = p2[(size_t) j] + h * (a * a * f + a * d * f * f + d * d * f * f * f / 3.0);
                    o1[i] = static_cast<float> (s1);
                    o2[i] = static_cast<float> (s2);
                }
            }
        }

        std::vector<float> tables, c1, c2;
    };
} // namespace ogi
