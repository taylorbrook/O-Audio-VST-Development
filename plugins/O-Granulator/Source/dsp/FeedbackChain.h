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
#include <cstdint>
#include <limits>
#include "ControlGrid.h"
#include "EngineConstants.h"
#include "GrainFx.h"

// ============================================================================
// Feedback chain (Stage 2, Phase 2.4) — ARCHITECTURE §12 order, RESEARCH-2.4
// §8 (R31, R32, R36), CONTEXT-2.4 D12, PLAN-2.4 P37.
//
// Per sample, per channel, float:
//     x   = tap · g_knob                 g_knob = 0.95 · feedback (engine ramp)
//     x   = HP_tpt (x)                   k = kFbK (Butterworth, next float > √2)
//     x   = LP_tpt (x)                   fc ≤ 0.45 · fs
//     x   = (1 − m)·x + m·AP4 (x)        m = fb_diffusion (AP4 always runs; m 0 → bitwise x)
//     x   = tanh (d·x) / d               d = 1 + 7 · sat (D12)
//     env = stereo-linked peak follower  1 ms attack / 400 ms release
//     x  *= min (1, 0.7 / env)           env 0 → 0.7/0 = +inf → gain 1
//     non-finite (xL + xR) → reset every state (diffuser lines included), x = 0
//     store x + 0.0f                     (R36: canonical +0)
//
// The HP / LP coefficients are formed at grid ticks only (tan in tptCoeffs);
// inside a cell they are constant. Every per-sample control (g_knob, m, d) is
// an engine ExactRamp read at the cell position, so the chain is block-size
// invariant. No allocation anywhere: the diffuser lines are fixed arrays sized
// for 192 kHz (kDiffuserMaxLen) and prepare() only sets lengths and clears.
// ============================================================================
namespace ogran
{
    /** TPT SVF coefficients for an explicit damping k (the fb filters' latch).
        fc is clamped to [20 Hz, 0.45·fs]. tan only here (grid ticks). */
    struct TptCoeffs { float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f; };

    inline TptCoeffs tptCoeffs (double fcHz, double k, double fs) noexcept
    {
        const double fc = std::clamp (fcHz, kFxMinHz, kFbNyquist * fs);
        const double g  = std::tan (kFxPi * fc / fs);
        const double a1 = 1.0 / (1.0 + g * (g + k));
        const double a2 = g * a1;
        const double a3 = g * a2;
        return { static_cast<float> (a1), static_cast<float> (a2), static_cast<float> (a3) };
    }

    /** D12 loop saturation: slope 1 at 0, |y| ≤ 1/d. */
    inline float fbSat (float x, float d) noexcept
    {
        return std::tanh (d * x) / d;
    }

    /** AGC one-pole coefficients a = exp (−1/(τ·fs)). */
    struct AgcCoeffs { float att = 0.0f, rel = 0.0f; };

    inline AgcCoeffs agcCoeffs (double fs) noexcept
    {
        return { static_cast<float> (std::exp (-1.0 / (kAgcAttackS * fs))),
                 static_cast<float> (std::exp (-1.0 / (kAgcReleaseS * fs))) };
    }

    /** Diffuser section lengths, lround (ms·fs/1000) from kDiffuserMs (the one
        source every row reads): 226/398/667/1042 @ 48 k, 207/366/613/957 @ 44.1 k. */
    struct DiffuserLengths { int n[4] {}; };

    inline DiffuserLengths diffuserLengths (double fs) noexcept
    {
        DiffuserLengths d;
        for (int s = 0; s < 4; ++s)
            d.n[s] = std::clamp (static_cast<int> (std::lround (kDiffuserMs[s] * fs / 1000.0)), 1, kDiffuserMaxLen);
        return d;
    }

    /** One Schroeder allpass section (−g + z^−M)/(1 − g·z^−M), in place on a
        circular line: w = x + g·w[n−M], y = w[n−M] − g·w. |H| = 1. */
    inline float allpassStep (float* line, int pos, float x) noexcept
    {
        const float dd = line[pos];
        const float w  = x + kDiffuserG * dd;
        line[pos] = w;
        return dd - kDiffuserG * w;
    }

    struct FeedbackChain
    {
        /** Message thread (Engine::prepare): lengths, AGC coefficients, state cleared. */
        void prepare (double fs) noexcept
        {
            sampleRate = fs;
            lengths    = diffuserLengths (fs);
            agc        = agcCoeffs (fs);
            scrubReset();
        }

        /** Grid tick: coefficients from the ramped cutoffs (tan here only). */
        void setCutoffs (double lowCutHz, double highCutHz) noexcept
        {
            hp = tptCoeffs (lowCutHz, static_cast<double> (kFbK), sampleRate);
            lp = tptCoeffs (highCutHz, static_cast<double> (kFbK), sampleRate);
        }

        /** Fault path (and prepare): every state to zero, the diffuser lines
            included (writes, no allocation). */
        void scrubReset() noexcept
        {
            for (int c = 0; c < 2; ++c)
            {
                hpIc1[c] = hpIc2[c] = lpIc1[c] = lpIc2[c] = 0.0f;
                for (int s = 0; s < 4; ++s)
                    std::fill (line[c][s], line[c][s] + kDiffuserMaxLen, 0.0f);
            }
            for (int s = 0; s < 4; ++s)
                pos[s] = 0;
            env = 0.0f;
        }

        // ── stages (public: the U7 unit rows drive them one at a time) ──────
        /** One TPT HP / LP sample at k = kFbK (shared statements: fx::tptStep). */
        static inline float hpTick (float& ic1, float& ic2, const TptCoeffs& c, float in) noexcept
        {
            float v1 = 0.0f, v2 = 0.0f;
            fx::tptStep (ic1, ic2, c.a1, c.a2, c.a3, in, v1, v2);
            return in - kFbK * v1 - v2;
        }
        static inline float lpTick (float& ic1, float& ic2, const TptCoeffs& c, float in) noexcept
        {
            float v1 = 0.0f, v2 = 0.0f;
            fx::tptStep (ic1, ic2, c.a1, c.a2, c.a3, in, v1, v2);
            return v2;
        }

        /** HP then LP. */
        inline void stepFilters (float& xl, float& xr) noexcept
        {
            xl = lpTick (lpIc1[0], lpIc2[0], lp, hpTick (hpIc1[0], hpIc2[0], hp, xl));
            xr = lpTick (lpIc1[1], lpIc2[1], lp, hpTick (hpIc1[1], hpIc2[1], hp, xr));
        }

        /** AP4 always runs (lines stay warm); the blend applies only for m > 0,
            so m == 0 leaves x bitwise (signed zeros included). */
        inline void stepDiffuser (float& xl, float& xr, float m) noexcept
        {
            float apl = xl, apr = xr;
            for (int s = 0; s < 4; ++s)
            {
                apl = allpassStep (line[0][s], pos[s], apl);
                apr = allpassStep (line[1][s], pos[s], apr);
                if (++pos[s] >= lengths.n[s])
                    pos[s] = 0;
            }
            if (m > 0.0f)
            {
                xl = (1.0f - m) * xl + m * apl;
                xr = (1.0f - m) * xr + m * apr;
            }
        }

        /** Stereo-linked peak AGC. */
        inline void stepAgc (float& xl, float& xr) noexcept
        {
            const float peak = std::max (std::abs (xl), std::abs (xr));
            env = peak > env ? agc.att * env + (1.0f - agc.att) * peak
                             : agc.rel * env + (1.0f - agc.rel) * peak;
            const float g = std::min (1.0f, kAgcCeil / env);
            xl *= g;
            xr *= g;
        }

        /** NaN / Inf scrub. True when it fired (state reset, x = 0). */
        inline bool stepScrub (float& xl, float& xr) noexcept
        {
            if (std::isfinite (xl + xr))
                return false;
            scrubReset();
            xl = xr = 0.0f;
            ++scrubs;
            return true;
        }

        /** Step 7 of the pass: l / r hold the tap on entry and the canonical
            return (R36) on exit. cellPos = first sample's position in its cell. */
        void process (float* l, float* r, int n, int cellPos,
                      const ExactRamp& knob, const ExactRamp& diff, const ExactRamp& satD) noexcept
        {
            for (int i = 0; i < n; ++i)
            {
                const int c = cellPos + i;
                const float g = knob.value (c);
                float xl = l[i] * g, xr = r[i] * g;
               #if OGRAN_TEST_HOOKS
                if (i == testInjectAt && testInjectNan)
                    xl = xr = std::numeric_limits<float>::quiet_NaN();
               #endif
                stepFilters (xl, xr);
                stepDiffuser (xl, xr, diff.value (c));
                const float d = satD.value (c);
                xl = fbSat (xl, d);
                xr = fbSat (xr, d);
                stepAgc (xl, xr);
                stepScrub (xl, xr);
               #if OGRAN_TEST_HOOKS
                if (i == testInjectAt && ! testInjectNan)
                {
                    scrubReset();   // the Z4 reference: the scrub's state effect, without a NaN
                    xl = xr = 0.0f;
                }
               #endif
                l[i] = xl + 0.0f;
                r[i] = xr + 0.0f;
            }
        }

        // ── state ───────────────────────────────────────────────────────────
        double sampleRate = 48000.0;
        DiffuserLengths lengths;
        AgcCoeffs agc;
        TptCoeffs hp, lp;
        float hpIc1[2] {}, hpIc2[2] {}, lpIc1[2] {}, lpIc2[2] {};
        float env = 0.0f;
        int   pos[4] {};
        float line[2][4][kDiffuserMaxLen] {};
        std::int64_t scrubs = 0;
       #if OGRAN_TEST_HOOKS
        int  testInjectAt  = -1;      // sample index inside the next process() call (−1 off)
        bool testInjectNan = true;    // true: NaN at the input; false: reset after the sample (reference)
       #endif
    };
} // namespace ogran
