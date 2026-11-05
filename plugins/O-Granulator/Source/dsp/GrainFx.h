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
#include "Grain.h"

// ============================================================================
// Per-grain FX (Stage 2, Phase 2.3) — ARCHITECTURE §9 order
//     read → SVF → drive → crush → ring → window·gain·pan
// PLAN-2.3 P18–P25, RESEARCH-2.3 R13–R18, CONTEXT-2.3 D6–D8.
//
// Two kinds of function, both free, noexcept and allocation-free:
//   • LATCH functions run once per grain at spawn (Spawner::plan). They are the
//     ONLY place tan / pow / exp2 / cos / sin appear.
//   • STAGE functions run over one segment's gathered samples (≤ kCell, the
//     Engine kernel's L/R arrays) and advance the grain's FX state ONE SAMPLE
//     AT A TIME. Nothing is keyed to the segment (R13: segment boundaries
//     depend on the host block), so every stage is block-size invariant.
// The law gates (D6, D7, D8, R14) are unit rows on these same functions.
// ============================================================================
namespace ogran::fx
{
    //==========================================================================
    // Latch math (spawn time)
    //==========================================================================

    /** base · 2^(rand · 4 · (u − ½)) — ±2 octaves at rand 1 (double). */
    inline double randOct (double base, double rand, double u) noexcept
    {
        return base * std::exp2 (rand * kFxRandOct * (u - 0.5));
    }

    /** Cytomic TPT SVF coefficients. fc is clamped to [20 Hz, 0.45·fs] (the
        clamped value is returned in fcOut, as float, for the census); g, k and
        the a's are formed in double and cast. */
    struct SvfLatch
    {
        float k = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        float fcHz = 0.0f;   // post-clamp (telemetry)
        float make = 1.0f;   // D10 LP/HP make-down 1/√P(k) (2.4)
    };

    inline SvfLatch svfLatch (double fcHz, double res, double fs) noexcept
    {
        const double fc = std::clamp (fcHz, kFxMinHz, kFxNyquist * fs);
        const double g  = std::tan (kFxPi * fc / fs);
        const double k  = kSvfKMax - kSvfKSpan * std::clamp (res, 0.0, 1.0);
        const double a1 = 1.0 / (1.0 + g * (g + k));
        const double a2 = g * a1;
        const double a3 = g * a2;
        SvfLatch s;
        s.k  = static_cast<float> (k);
        s.a1 = static_cast<float> (a1);
        s.a2 = static_cast<float> (a2);
        s.a3 = static_cast<float> (a3);
        s.fcHz = static_cast<float> (fc);
        // D10 (R33): P(k) = 1/(k·√(1 − k²/4)) for k < √2, else 1 — the analog
        // LP/HP peak, which the bilinear map preserves at any fc, so it depends
        // on k only. m = 1/√P in double from the LATCHED float k: exactly 1.0f
        // for res <= 30.83 % (k >= √2), so non-resonant renders stay bitwise.
        const double kl = static_cast<double> (s.k);
        if (kl < 1.41421356237309504880)
        {
            const double p = 1.0 / (kl * std::sqrt (1.0 - 0.25 * kl * kl));
            s.make = static_cast<float> (1.0 / std::sqrt (p));
        }
        return s;
    }

    /** D6 shaper. The normaliser and the per-sample stage BOTH go through this
        function (R15: y (±r) == ±r then holds to rounding for any odd shaper). */
    inline float driveShape (float d, float x) noexcept
    {
        return std::tanh (d * x);
    }

    struct DriveLatch
    {
        bool  active = false;
        float d = 1.0f, norm = 1.0f;   // Grain {} defaults when inactive
    };

    /** From the shaper gain d directly (U2c probes the guard with d itself). */
    inline DriveLatch driveLatchD (float d) noexcept
    {
        DriveLatch l;
        if (! (d >= kDriveGuard))   // R16: below the guard the law is ≡ x to ~1 ulp; drive 0 → bitwise bypass
            return l;
        l.active = true;
        l.d      = d;
        l.norm   = kDriveRef / driveShape (d, kDriveRef);   // float, through the SAME shaper
        return l;
    }

    /** D6: d = 16·drive² (continuous from 0). */
    inline DriveLatch driveLatch (float drive) noexcept
    {
        return driveLatchD (kDriveGain * drive * drive);
    }

    struct CrushLatch
    {
        float inc = 0.0f, scale = 0.0f;
    };

    /** D7 / P24: hold increment (1000/fs)^crush (→ hold rate fs·inc), and the
        quantiser scale 2^(b−1) = 2^(15 − 14·crush). */
    inline CrushLatch crushLatch (float crush, double fs) noexcept
    {
        const double c = static_cast<double> (crush);
        CrushLatch l;
        l.inc   = static_cast<float> (std::pow (kCrushHoldHz / fs, c));
        l.scale = static_cast<float> (std::exp2 (kCrushScaleTop - kCrushScaleSpan * c));
        return l;
    }

    /** D7 mid-tread quantiser (ties to even): silence → ±0, never ½ LSB. */
    inline float crushQ (float x, float scale) noexcept
    {
        return std::nearbyint (x * scale) / scale;
    }

    struct RingLatch
    {
        float rotRe = 1.0f, rotIm = 0.0f;
        float hz = 0.0f;   // post-clamp (telemetry)
    };

    /** Carrier rotation e^{jω}, ω = 2π·min (f, 0.45·fs)/fs, formed in double. */
    inline RingLatch ringLatch (double fHz, double fs) noexcept
    {
        const double f = std::min (fHz, kFxNyquist * fs);
        const double w = 2.0 * kFxPi * f / fs;
        RingLatch l;
        l.rotRe = static_cast<float> (std::cos (w));
        l.rotIm = static_cast<float> (std::sin (w));
        l.hz    = static_cast<float> (f);
        return l;
    }

    //==========================================================================
    // Stages (per sample; l / r hold n ≤ kCell gathered samples)
    //==========================================================================

    /** One Cytomic TPT SVF step — THE statements (2.3 verbatim), shared by the
        per-grain SVF and the feedback chain's HP / LP (P36). Inlining does not
        merge statements, so FP contraction is unchanged. */
    inline void tptStep (float& ic1, float& ic2, float a1, float a2, float a3, float x, float& v1, float& v2) noexcept
    {
        const float v3 = x - ic2;
        v1 = a1 * ic1 + a2 * v3;
        v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
    }

    /** Cytomic TPT SVF, stereo, latched mode (1 LP = m·v2, 2 BP = k·v1 (D8),
        3 HP = m·(x − k·v1 − v2)); m = the D10 make-down (1.0f where P <= 1). */
    inline void svf (Grain& g, float* l, float* r, int n) noexcept
    {
        const float a1 = g.svfA1, a2 = g.svfA2, a3 = g.svfA3, k = g.svfK, m = g.svfMake;
        float* const io[2] { l, r };
        for (int c = 0; c < 2; ++c)
        {
            float* const x = io[c];
            float ic1 = g.svfIc1[c], ic2 = g.svfIc2[c];
            switch (g.svfMode)
            {
                case kSvfLP:
                    for (int i = 0; i < n; ++i)
                    {
                        float v1 = 0.0f, v2 = 0.0f;
                        tptStep (ic1, ic2, a1, a2, a3, x[i], v1, v2);
                        x[i] = m * v2;
                    }
                    break;
                case kSvfBP:
                    for (int i = 0; i < n; ++i)
                    {
                        float v1 = 0.0f, v2 = 0.0f;
                        tptStep (ic1, ic2, a1, a2, a3, x[i], v1, v2);
                        x[i] = k * v1;
                    }
                    break;
                case kSvfHP:
                    for (int i = 0; i < n; ++i)
                    {
                        const float in = x[i];
                        float v1 = 0.0f, v2 = 0.0f;
                        tptStep (ic1, ic2, a1, a2, a3, in, v1, v2);
                        x[i] = m * (in - k * v1 - v2);
                    }
                    break;
                default:
                    break;
            }
            g.svfIc1[c] = ic1;
            g.svfIc2[c] = ic2;
        }
    }

    /** P25 NaN kill detector: false if any integrator is NaN or ±Inf. */
    inline bool svfStateFinite (const Grain& g) noexcept
    {
        return std::isfinite (g.svfIc1[0] + g.svfIc2[0] + g.svfIc1[1] + g.svfIc2[1]);
    }

    /** D6: y = norm · tanh (d·x). */
    inline void drive (const Grain& g, float* l, float* r, int n) noexcept
    {
        const float d = g.driveD, nm = g.driveNorm;
        for (int i = 0; i < n; ++i)
        {
            l[i] = nm * driveShape (d, l[i]);
            r[i] = nm * driveShape (d, r[i]);
        }
    }

    /** D7 + fractional-phase hold. One phase per grain, shared by L/R; the
        Grain default phase 1 captures the first sample ("fresh hold"). */
    inline void crush (Grain& g, float* l, float* r, int n) noexcept
    {
        const float inc = g.crushHoldInc, scale = g.crushScale;
        float ph = g.crushHoldPhase, hl = g.crushHeld[0], hr = g.crushHeld[1];
        for (int i = 0; i < n; ++i)
        {
            if (ph >= 1.0f)
            {
                ph -= 1.0f;
                hl = crushQ (l[i], scale);
                hr = crushQ (r[i], scale);
            }
            l[i] = hl;
            r[i] = hr;
            ph += inc;
        }
        g.crushHoldPhase = ph;
        g.crushHeld[0] = hl;
        g.crushHeld[1] = hr;
    }

    /** Ring: y = x·((1 − mix) + mix·Re c), L/R share the carrier; rotate, then
        one Newton renormalisation step EVERY sample (R14 / P21). */
    inline void ring (Grain& g, float* l, float* r, int n) noexcept
    {
        const float rr = g.ringRotRe, ri = g.ringRotIm, mix = g.ringMix, dry = 1.0f - mix;
        float re = g.ringRe, im = g.ringIm;
        for (int i = 0; i < n; ++i)
        {
            const float m = dry + mix * re;
            l[i] = l[i] * m;
            r[i] = r[i] * m;
            const float nr = re * rr - im * ri;
            const float ni = re * ri + im * rr;
            const float s  = 1.5f - 0.5f * (nr * nr + ni * ni);
            re = nr * s;
            im = ni * s;
        }
        g.ringRe = re;
        g.ringIm = im;
    }
} // namespace ogran::fx
