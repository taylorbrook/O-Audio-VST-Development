/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#include <climits>
#include <cmath>
#include <utility>

// ============================================================================
// Voice-axis grain budget (CONTEXT-2.1 D12, RESEARCH-2.1 §1, PLAN-2.1 T4, P1).
//
// waterFillN<N> is the parent Quotas.h:53-99 body with kMaxStreams → N:
//   ΣN <= Q : q_k = N_k
//   else    : λ with Σ min (N_k, λ) = Q,  q_k = min (N_k, λ)
// Insertion sort of a fixed int idx[N] by demand, ties by index (stable). The
// result for equal demands does not depend on their order (λ is symmetric), so
// a tie-order perturbation alone CANNOT change q / λ; the Q0 control therefore
// uses WaterFillTest::noSort (skips the sort), which does. waterFillN<4> is
// bitwise equal to the forked waterFill on q / lambda / capped (row Q0).
//
// P1 integer-quota thinning (adopted research flag):
//   m_k = capped ? ⌊q_k⌋ : ∞
//   T_k = max (fs / density, (L_max + 1) / m_k)
// Each grain lives <= L_max samples and the onsets are >= (L_max + 1)/m_k
// apart, so at most m_k grains of voice k are live at any sample and
// Σ live <= Σ ⌊q_k⌋ <= Q exactly (R3 kills become a transitions-only path).
// The output law uses what thinning produces: N_eff = len̄ / T_k.
//
// 2.2 (PLAN-2.2 T5): the engine calls waterFillN<kPairs> (80 (voice, stream)
// demands, index k·4 + s) with this code unchanged. Zero demands sort first
// and subtract 0.0 exactly, so with one stream the nonzero shares are bitwise
// those of waterFillN<20> (R§10.1).
//
// Header-only, allocation-free, no JUCE. Grid thread (= audio thread).
// ============================================================================
namespace ogi
{
    /** m sentinel for "not capped" (m = ∞). */
    inline constexpr int kQuotaUncapped = INT_MAX;

    template <int N>
    struct QuotasN
    {
        double q[N] {};
        int    m[N] {};          // ⌊q⌋ when capped, kQuotaUncapped otherwise
        double lambda = 0.0;     // the water level (ΣN when uncapped)
        bool   capped = false;
    };

    /** Q0 negative-control knobs (test only; the engine always passes none). */
    enum class WaterFillTest
    {
        none,             // the parent algorithm
        tiesDescending,   // equal demands ordered by DESCENDING index (result-invariant: documents that)
        noSort            // skip the sort entirely (changes q whenever demands are unsorted)
    };

    template <int N>
    QuotasN<N> waterFillN (const double (&demand)[N], int capacity, WaterFillTest test = WaterFillTest::none) noexcept
    {
        static_assert (N >= 1, "waterFillN needs at least one demand");
        QuotasN<N> r;
        const double Q = static_cast<double> (capacity);

        double sum = 0.0;
        for (double n : demand)
            sum += n;

        if (sum <= Q)
        {
            for (int k = 0; k < N; ++k)
                r.q[k] = demand[k];
            r.lambda = sum;
        }
        else
        {
            // Insertion sort of N indices by demand (ties by index — deterministic).
            int idx[N];
            for (int k = 0; k < N; ++k)
                idx[k] = k;

            if (test != WaterFillTest::noSort)
            {
                for (int i = 1; i < N; ++i)
                {
                    for (int j = i; j > 0; --j)
                    {
                        const double a = demand[idx[j]];
                        const double b = demand[idx[j - 1]];
                        const bool tieSwap = test == WaterFillTest::tiesDescending
                                          && ! (a < b) && ! (b < a) && idx[j] > idx[j - 1];
                        if (! (a < b) && ! tieSwap)
                            break;
                        std::swap (idx[j], idx[j - 1]);
                    }
                }
            }

            double remaining = Q;
            double lambda    = Q;
            int i = 0;
            for (; i < N; ++i)
            {
                const double share = remaining / static_cast<double> (N - i);
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
            for (; i < N; ++i)
                r.q[idx[i]] = lambda;
            r.lambda = lambda;
            r.capped = true;
        }

        for (int k = 0; k < N; ++k)
            r.m[k] = r.capped ? static_cast<int> (std::floor (r.q[k])) : kQuotaUncapped;
        return r;
    }

    /** P1: T = max (fs / density, (L_max + 1) / m). m = kQuotaUncapped → fs / density.
        m < 1 is treated as 1 (a capped voice always keeps one grain).

        PLAN-2.2 PD7 (jitter J in [0, 1], the latched sN_jitter): onset k sits
        at k·T + J·T·(u_k − ½), so any m + 1 consecutive onsets span at least
        T·(m − J). For m >= 2 the bound becomes T = max (fs / density,
        (L_max + 1) / (m − J)); m <= 1 keeps (L_max + 1) / 1 (no finite T bounds
        a jittered m = 1 pair: R3 kills there are expected, P10 / R5).
        J = 0: m − 0.0 == m exactly, so every 2.1 period is bitwise unchanged. */
    inline double thinnedPeriod (double fs, double density, double lMaxSamples, int m, double jitter = 0.0) noexcept
    {
        const double base = fs / std::max (1.0e-9, density);
        if (m >= kQuotaUncapped)
            return base;
        const double mm = static_cast<double> (std::max (1, m));
        const double den = mm >= 2.0 ? mm - std::clamp (jitter, 0.0, 1.0) : mm;
        return std::max (base, (lMaxSamples + 1.0) / den);
    }

    /** N_eff = len̄ / T (full windows under thinning). */
    inline double effectiveCount (double lenBarSamples, double periodSamples) noexcept
    {
        return periodSamples > 0.0 ? lenBarSamples / periodSamples : 0.0;
    }
} // namespace ogi
