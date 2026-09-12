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

    ChebyshevProjector.cpp
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    // background job only (see the header).

  ==============================================================================
*/

#include "ChebyshevProjector.h"
#include <cmath>

int ChebyshevProjector::nodesFor (float F)
{
    const int m = 8 * static_cast<int> (std::ceil (2.0f * F)) + 16;
    return m < 32 ? 32 : m;
}

void ChebyshevProjector::buildNodes (int M, std::vector<double>& x, std::vector<double>& T)
{
    constexpr double kPi = 3.14159265358979323846;
    x.assign (static_cast<size_t> (M), 0.0);
    T.assign (static_cast<size_t> ((kChebDegree + 1) * M), 0.0);
    for (int i = 0; i < M; ++i)
    {
        const double a = kPi * (static_cast<double> (i) + 0.5) / static_cast<double> (M);
        x[static_cast<size_t> (i)] = std::cos (a);
        for (int n = 0; n <= kChebDegree; ++n)
            T[static_cast<size_t> (n * M + i)] = std::cos (static_cast<double> (n) * a);   // T_n (cos a) = cos (n a)
    }
}

void ChebyshevProjector::projectGrid (const double* f, int M, const double* T, ChebyshevSet& out,
                                      const std::atomic<bool>* shouldExit)
{
    const size_t Mz = static_cast<size_t> (M);
    // G_nj = Σ_i T_n (x_i) f (x_i, x_j)
    std::vector<double> G (static_cast<size_t> (kChebDegree + 1) * Mz, 0.0);
    for (int n = 0; n <= kChebDegree; ++n)
    {
        const double* Tn = T + static_cast<size_t> (n) * Mz;
        double* Gn = G.data() + static_cast<size_t> (n) * Mz;
        for (size_t i = 0; i < Mz; ++i)
        {
            const double t = Tn[i];
            const double* fi = f + i * Mz;
            for (size_t j = 0; j < Mz; ++j)
                Gn[j] += t * fi[j];
        }
    }

    if (shouldExit != nullptr && shouldExit->load())
        return;

    // c_nm = (2/M)² w_n w_m Σ_j G_nj T_m (x_j), w_0 = ½; n + m > 16 discarded
    const double scale = 4.0 / (static_cast<double> (M) * static_cast<double> (M));
    for (int n = 0; n <= kChebDegree; ++n)
    {
        const double* Gn = G.data() + static_cast<size_t> (n) * Mz;
        const double wn = n == 0 ? 0.5 : 1.0;
        for (int m = 0; m + n <= kChebDegree; ++m)
        {
            const double* Tm = T + static_cast<size_t> (m) * Mz;
            const double wm = m == 0 ? 0.5 : 1.0;
            double acc = 0.0;
            for (size_t j = 0; j < Mz; ++j)
                acc += Gn[j] * Tm[j];
            out.c[static_cast<size_t> (chebRowStart (n) + m)] = static_cast<float> (scale * wn * wm * acc);
        }
    }

    // fit = 100 · (1 − ‖f − Pf‖² / ‖f‖²) with Pf from the FLOAT coefficients the
    // oscillator will actually evaluate.
    std::vector<double> x (Mz);
    for (size_t i = 0; i < Mz; ++i) x[i] = T[Mz + i];   // T_1 (x_i) = x_i
    double err = 0.0, pow2 = 0.0;
    for (size_t i = 0; i < Mz; ++i)
        for (size_t j = 0; j < Mz; ++j)
        {
            const double v = f[i * Mz + j];
            const double p = clenshaw2D (out.c.data(), static_cast<float> (x[i]), static_cast<float> (x[j]));
            err += (v - p) * (v - p);
            pow2 += v * v;
        }
    out.fit = pow2 > 0.0 ? static_cast<float> (100.0 * (1.0 - err / pow2)) : 100.0f;
}

void ChebyshevProjector::projectAnalytic (TerrainKind kind, float F, float mx, float my, ChebyshevSet& out,
                                          const std::atomic<bool>* shouldExit)
{
    const int M = nodesFor (F);
    std::vector<double> x, T;
    buildNodes (M, x, T);
    std::vector<double> f (static_cast<size_t> (M) * static_cast<size_t> (M));
    for (int i = 0; i < M; ++i)
        for (int j = 0; j < M; ++j)
            f[static_cast<size_t> (i * M + j)] = terrain (kind, static_cast<float> (x[static_cast<size_t> (i)]),
                                                          static_cast<float> (x[static_cast<size_t> (j)]), F, mx, my);
    if (shouldExit != nullptr && shouldExit->load())
        return;
    projectGrid (f.data(), M, T.data(), out, shouldExit);
}
