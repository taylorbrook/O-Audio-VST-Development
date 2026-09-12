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

    ChebyshevProjector.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Projects a terrain onto the degree-16 Chebyshev triangle (ARCHITECTURE
    Algorithm "Bandlimited mode — Chebyshev projection"; RESEARCH §2.10): sample
    f on the M × M Chebyshev–Gauss grid x_i = cos (π (i + ½) / M), contract
    separably — G_nj = Σ_i T_n(x_i) f(x_i, x_j), c_nm = (2/M)² w_n w_m Σ_j G_nj T_m(x_j),
    w_0 = ½ — discard n + m > 16, and score the fit as 100 · (1 − ‖f − Pf‖² / ‖f‖²)
    with Pf re-evaluated by clenshaw2D on the same nodes.

    // background job only — every function here allocates and runs on the
    // TerrainScheduler's pool (or inline on the message thread for the harness's
    // runOnceSynchronously); never on the audio thread (plan Decision 39).

  ==============================================================================
*/

#pragma once
#include "ChebyshevSet.h"
#include "Terrains.h"
#include <vector>

struct ChebyshevProjector
{
    /** Grid size for an analytic terrain at spatial frequency F (plan Task 3):
        max (32, 8·⌈2F⌉ + 16) — 32 at F <= 1, 48 at F = 2. */
    static int nodesFor (float F);

    /** x_i = cos (π (i + ½) / M) and T_n (x_i) for n <= 16, laid out T[n * M + i]. */
    static void buildNodes (int M, std::vector<double>& x, std::vector<double>& T);

    /** The shared separable contraction. f[i * M + j] = f (x_i, x_j). Writes
        out.c and out.fit; leaves out.key to the caller. `shouldExit` (nullable)
        is polled between the two contraction passes. */
    static void projectGrid (const double* f, int M, const double* T, ChebyshevSet& out,
                             const std::atomic<bool>* shouldExit = nullptr);

    /** One analytic terrain (kind 0–5) at F, mx, my — M = nodesFor (F). */
    static void projectAnalytic (TerrainKind kind, float F, float mx, float my, ChebyshevSet& out,
                                 const std::atomic<bool>* shouldExit = nullptr);
};
