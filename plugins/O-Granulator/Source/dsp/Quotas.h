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

#include <cmath>
#include <utility>
#include "EngineConstants.h"

// ============================================================================
// Stream quotas — closed-form water-fill over the 4 demands (RESEARCH-2.2 §3,
// PLAN-2.2 T5).
//
//   N_s = streamDemand (density_s, size_s, fs) for enabled streams, 0 otherwise
//         (mute / solo do NOT change demand — CONTEXT-2.2, P15)
//   ΣN <= Q : q_s = N_s
//   else    : λ with Σ min (N_s, λ) = Q,  q_s = min (N_s, λ)
//   qCeil_s = ceil (q_s)   (R8: the steal flag is live_s > qCeil_s)
//
// Grid rate, latched parameters only — never pool occupancy. λ is continuous
// and piecewise linear in the demands, so quotas (and g_out) cannot step.
//
// Single-stream capped case: the three zero demands come first in the sorted
// order and subtract 0.0 from the remaining capacity (exact), so the last
// stream's λ is Q / 1 == Q EXACTLY — the R12 2.1-equivalence null depends on it.
// ============================================================================
namespace ogran
{
    struct Quotas
    {
        double q[kMaxStreams]     {};
        int    qCeil[kMaxStreams] {};
        double lambda = 0.0;   // the water level (== Q when capped by one stream); +inf-free: ΣN when uncapped
        bool   capped = false;
    };

    inline Quotas waterFill (const double (&demand)[kMaxStreams], int capacity) noexcept
    {
        Quotas r;
        const double Q = static_cast<double> (capacity);

        double sum = 0.0;
        for (double n : demand)
            sum += n;

        if (sum <= Q)
        {
            for (int s = 0; s < kMaxStreams; ++s)
                r.q[s] = demand[s];
            r.lambda = sum;
        }
        else
        {
            // Insertion sort of ≤ 4 indices by demand (ties by index — deterministic).
            int idx[kMaxStreams];
            for (int s = 0; s < kMaxStreams; ++s)
                idx[s] = s;
            for (int i = 1; i < kMaxStreams; ++i)
                for (int j = i; j > 0 && demand[idx[j]] < demand[idx[j - 1]]; --j)
                    std::swap (idx[j], idx[j - 1]);

            double remaining = Q;
            double lambda    = Q;
            int i = 0;
            for (; i < kMaxStreams; ++i)
            {
                const double share = remaining / static_cast<double> (kMaxStreams - i);
                const double n     = demand[idx[i]];
                if (n <= share)
                {
                    r.q[idx[i]] = n;
                    remaining  -= n;
                }
                else
                {
                    lambda = share;
                    break;
                }
            }
            for (; i < kMaxStreams; ++i)
                r.q[idx[i]] = lambda;
            r.lambda = lambda;
            r.capped = true;
        }

        for (int s = 0; s < kMaxStreams; ++s)
            r.qCeil[s] = static_cast<int> (std::ceil (r.q[s]));
        return r;
    }
} // namespace ogran
