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
#include <cmath>
#include <limits>
#include "BandlimitedReader.h"   // grain-engine 1.1.0: MipRing<L>::margin
#include "GiConstants.h"

// ============================================================================
// Spawner record-head clamp (PLAN-2.4 PD9, RESEARCH-2.4 §2; ARCHITECTURE
// Algorithm §4 with g_a corrected 16 → kCell + 8 = 40).
//
// A grain reads p(t) = p0 + dir·rate·t, t ∈ [0, len]; a running head writes
// h(t) = h0 + t. The relative distance d(t) = p(t) − h(t) sweeps
// [d0 + min (0, Δ), d0 + max (0, Δ)] with Δ = (dir·rate − 1)·len. A read is
// unsafe while d ∈ [−g_b, g_a]:
//   g_b = MipRing<4>::margin (level)  — children behind the head not yet
//         recomputed from the new frames (9 / 80 / 222 / 506 / 1074);
//   g_a = kCell + 8 = 40              — the whole cell is written at the tick
//         (up to 31 frames ahead of h_n) + the 8-tap kernel reach.
// So d0 is forbidden in [min (0, −Δ) − g_b, max (0, −Δ) + g_a]. The head
// wraps hi → lo inside its latched loop, so with `circular` the forbidden set
// is that arc modulo W (copies at −W, 0, +W in absolute tape frames);
// otherwise (the head outside the current region) plain absolute distance.
//
// The forbidden set is subtracted from the region fit [fitLo, fitHi]; the
// feasible p0 nearest the drawn one wins, ties go behind the head (the lower
// candidate). Nothing feasible → the drawn p0, forced = true. Pure function:
// no draws, no engine state.
// ============================================================================
namespace ogi
{
    inline constexpr int kHeadAhead = kCell + 8;   // g_a (PD9)

    struct HeadClampResult
    {
        double p0     = 0.0;
        bool   forced = false;
        bool   moved  = false;
    };

    /** g_b for a latched level (MipRing<4>::margin). */
    inline int headBehind (int level) noexcept
    {
        return ouaricon::grain::MipRing<kTapeLevels>::margin (std::clamp (level, 0, kTapeLevels));
    }

    /** The forbidden d0 interval [a, b] (head-relative frames) for one grain. */
    inline void headForbidden (int dir, double rate, double len, int level, double& a, double& b, int gA = kHeadAhead) noexcept
    {
        const double delta = (static_cast<double> (dir) * rate - 1.0) * len;
        a = std::min (0.0, -delta) - static_cast<double> (headBehind (level));
        b = std::max (0.0, -delta) + static_cast<double> (gA);
    }

    /** p0Drawn: the region-fitted start (fit [fitLo, fitHi]); W = hi − lo of the
        head's loop (used only when circular). gA: the ahead guard (HC3's
        control passes 16, the ARCH value). */
    inline HeadClampResult clampToHead (double p0Drawn, int dir, double rate, double len, int level,
                                        double fitLo, double fitHi, double W, bool circular, double head,
                                        int gA = kHeadAhead) noexcept
    {
        HeadClampResult res;
        res.p0 = p0Drawn;

        double a = 0.0, b = 0.0;
        headForbidden (dir, rate, len, level, a, b, gA);
        if (! (fitHi >= fitLo) || (circular && b - a >= W))
        {
            res.forced = true;
            return res;
        }

        // Forbidden arcs in absolute frames (≤ 3 copies), sorted ascending.
        double lo[3], hi[3];
        int n = 0;
        if (circular)
        {
            for (int k = -1; k <= 1; ++k)
            {
                lo[n] = head + a + static_cast<double> (k) * W;
                hi[n] = head + b + static_cast<double> (k) * W;
                ++n;
            }
        }
        else
        {
            lo[0] = head + a;
            hi[0] = head + b;
            n = 1;
        }

        auto forbidden = [&] (double p) noexcept
        {
            for (int i = 0; i < n; ++i)
                if (p >= lo[i] && p <= hi[i])
                    return true;
            return false;
        };
        auto feasible = [&] (double p) noexcept { return p >= fitLo && p <= fitHi && ! forbidden (p); };

        if (! forbidden (p0Drawn))
            return res;   // the drawn start is safe: unchanged (no move, no draw)

        // Candidates: just outside every arc end, and the fit ends.
        constexpr double inf = std::numeric_limits<double>::infinity();
        double best = 0.0, bestDist = inf;
        bool found = false;
        auto consider = [&] (double p) noexcept
        {
            if (! feasible (p))
                return;
            const double dist = std::abs (p - p0Drawn);
            const bool tie = ! (dist < bestDist) && ! (dist > bestDist);
            if (dist < bestDist || (tie && p < best))   // tie → behind (lower)
            {
                best = p;
                bestDist = dist;
                found = true;
            }
        };
        for (int i = 0; i < n; ++i)
        {
            consider (std::nextafter (lo[i], -inf));
            consider (std::nextafter (hi[i], inf));
        }
        consider (fitLo);
        consider (fitHi);

        if (! found)
        {
            res.forced = true;
            return res;
        }
        res.p0 = best;
        res.moved = true;
        return res;
    }

    /** headReadScan (test): does the linear relative path d0 → d1 touch
        [−g_b, g_a] (mod W when circular)? Exact interval test. */
    inline bool headPathTouches (double d0, double d1, int level, double W, bool circular, int gA = kHeadAhead) noexcept
    {
        const double lo = std::min (d0, d1), hi = std::max (d0, d1);
        const double a = -static_cast<double> (headBehind (level)), b = static_cast<double> (gA);
        if (! circular)
            return hi >= a && lo <= b;
        for (int k = -1; k <= 1; ++k)
        {
            const double s = static_cast<double> (k) * W;
            if (hi >= a + s && lo <= b + s)
                return true;
        }
        return false;
    }
} // namespace ogi
