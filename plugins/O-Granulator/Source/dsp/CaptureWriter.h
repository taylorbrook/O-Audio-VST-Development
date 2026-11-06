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
#include "ControlGrid.h"
#include "EngineConstants.h"

// ============================================================================
// I/O adapter + capture writer (ARCHITECTURE §1–3, PLAN-2.1 T5).
//
// Engine input is always stereo: a mono bus feeds L = R (the caller passes the
// same pointer twice; channel counts come from the BUS layout, never from the
// buffer width). Per sample:
//
//     ring.push (xL·g + fb[n − 32].L,  xR·g + fb[n − 32].R)
//     x = in, or 0 when ! (|in·g| <= kAnalysisTapMax)        (R96 host guard)
//
// g = the input-gain ramp lerped inside the cell. R96 (PLAN-2.6 P81): a NaN,
// ±Inf or absurd (> +80 dBFS after the input gain) host sample is replaced by 0
// before it reaches the ring — one such sample used to poison the ring, every
// grain reading it, the feedback loop and the reverb tank for good. The guard
// selects the INPUT operand in its own statement and keeps each push statement's
// arithmetic verbatim (FP contraction is per statement: `x·g + fb` stays one
// fused expression), so every finite host sample within +80 dBFS is bitwise
// unchanged, and a guarded sample pushes 0·g + fb == fb exactly. fb is the z^-32 feedback
// return: slot (n mod 32) still holds what the feedback chain wrote 32 samples
// ago, and is overwritten after this piece's feedback step (2.4).
//
// Write Arbiter (2.4, ARCHITECTURE Algorithm §3, RESEARCH-2.4 R26, PLAN P38):
//   frozen  : NO push (head-stop). The ring is never cleared, the input never
//             gain-ramped.
//   release : a 10 ms raised-cosine CONTENT crossfade indexed from the release
//             tick: push w·(in·g + fb) + (1 − w)·held. held continues the frozen
//             history at W0 with no step AND no slope reversal (2.4 execution
//             deviation from R26's plain mirror, whose slope reversal is a 2·s'
//             kink every grain reading across W0 hears — κ ≈ 7x on a sine):
//                 even (k) = x[W0 − 1 − k]                 (R26 mirror: DC-free)
//                 odd  (k) = 2·x[W0 − 1] − x[W0 − 2 − k]   (point reflection: slope-continuous)
//                 held     = even + β(k)·(odd − even),  β = ½ + ½cos (πk/Lb) over Lb = 2 ms
//             so the continuation starts on the linear extrapolation and settles
//             onto the DC-free mirror. Indices clamp to the captured span.
//   live    : the 2.1 statement, verbatim.
//
// ring.clear() / prepare() happen ONLY in Engine::prepare() on the message
// thread — never here (a 27.6 MB memset on the audio thread).
// ============================================================================
namespace ogran
{
    /** Freeze arbiter state (Engine-owned; edges set at grid ticks only). */
    struct FreezeState
    {
        bool         frozen       = false;
        bool         xfading      = false;
        std::int64_t W0           = 0;      // the stopped head (ring domain)
        std::int64_t heldCaptured = 0;      // W0 − margin(2) − 1, clamped [0, ring]
        std::int64_t heldSpan     = 0;      // min (span, heldCaptured) — the R26 control only
        int          xfadePos     = 0;
        int          xfadeLen     = 480;
        int          slopeLen     = 96;     // β: odd → even mirror blend (2 ms)
        bool         spliceOld    = false;  // test hook releaseSpliceOld: ring[W − heldSpan]
    };

    struct CaptureWriter
    {
        /** Pushes piece samples [0, n) into the ring. cellPos = the piece's first
            sample's position inside its cell (piece ⊂ one cell, so cellPos + n <= kCell,
            and since kFbDelay == kCell the return slot of sample i is cellPos + i).
            Returns the number of host samples (per channel) the R96 guard replaced by
            0; sanitise = false is the Z7hn / Z7hr test control only. */
        static int write (Ring& ring, const float* inL, const float* inR, int n, int cellPos,
                          const ExactRamp& inputGain, const float (&fbReturn)[2][kFbDelay], FreezeState& fz,
                          bool sanitise = true) noexcept
        {
            static_assert (kFbDelay == kCell, "return-slot indexing below assumes kFbDelay == kCell");
            if (fz.frozen)
                return 0;   // head-stop: no push

            int guarded = 0;
            // R96: the input operand of one push, 0 when |x·g| is non-finite or > +80 dBFS
            // (the analysis tap's predicate, Analysis.h). Own statements: the product
            // here never fuses into the push.
            auto operand = [sanitise, &guarded] (float x, float g) noexcept
            {
                const float xg = x * g;
                if (sanitise && ! (std::abs (xg) <= kAnalysisTapMax))
                {
                    ++guarded;
                    return 0.0f;
                }
                return x;
            };

            int i = 0;
            for (; i < n && fz.xfading; ++i)
            {
                const int c = cellPos + i;
                const float g = inputGain.value (c);
                const std::int64_t wNow = ring.getWriteCount();
                const std::int64_t lo = fz.W0 - fz.heldCaptured, hi = fz.W0 - 1;
                const std::int64_t je = std::clamp (2 * fz.W0 - 1 - wNow, lo, hi);   // even mirror (R26)
                const std::int64_t jo = std::clamp (2 * fz.W0 - 2 - wNow, lo, hi);   // odd mirror partner
                const float w  = static_cast<float> (0.5 - 0.5 * std::cos (kFxPi * static_cast<double> (fz.xfadePos)
                                                                           / static_cast<double> (fz.xfadeLen)));
                const float b  = fz.xfadePos < fz.slopeLen
                                     ? static_cast<float> (0.5 + 0.5 * std::cos (kFxPi * static_cast<double> (fz.xfadePos)
                                                                                / static_cast<double> (fz.slopeLen)))
                                     : 0.0f;
                float hl = 0.0f, hr = 0.0f;
                if (fz.spliceOld)
                {
                    hl = ring.levelSample (0, 0, wNow - fz.heldSpan);   // R26 control: the splice
                    hr = ring.levelSample (0, 1, wNow - fz.heldSpan);
                }
                else
                {
                    const float el = ring.levelSample (0, 0, je), er = ring.levelSample (0, 1, je);
                    const float ol = 2.0f * ring.levelSample (0, 0, hi) - ring.levelSample (0, 0, jo);
                    const float orr = 2.0f * ring.levelSample (0, 1, hi) - ring.levelSample (0, 1, jo);
                    hl = el + b * (ol - el);
                    hr = er + b * (orr - er);
                }
                const float xl = operand (inL[i], g);
                const float xr = operand (inR[i], g);
                ring.push (w * (xl * g + fbReturn[0][c]) + (1.0f - w) * hl,
                           w * (xr * g + fbReturn[1][c]) + (1.0f - w) * hr);
                if (++fz.xfadePos >= fz.xfadeLen)
                    fz.xfading = false;
            }
            for (; i < n; ++i)
            {
                const int c = cellPos + i;
                const float g = inputGain.value (c);
                const float xl = operand (inL[i], g);
                const float xr = operand (inR[i], g);
                ring.push (xl * g + fbReturn[0][c], xr * g + fbReturn[1][c]);
            }
            return guarded;
        }
    };
} // namespace ogran
