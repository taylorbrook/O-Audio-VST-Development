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

    ChebyshevSet.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Bandlimited mode's terrain representation (ARCHITECTURE Core 6; plan
    Decision 25): a degree-16 Chebyshev TRIANGLE — the 153 coefficients c_nm with
    n + m <= 16, row-major by n (row n holds m = 0 … 16 − n) — evaluated per sample
    by 2-D Clenshaw: an inner Clenshaw over m for each row gives g_n(y), an outer
    Clenshaw over n at x. On an orbit of trigonometric degree K, the diagonal
    d = n + m contributes harmonics up to d·K·f_note, so truncating diagonals per
    voice (the taper in TerrainOscillator::updateBlockRate) IS the mip.

    Audio-thread budget (DSP-05): the audio-thread evaluator is chebEvalPadded —
    noexcept, branch-free on data, allocation-free, fixed trip counts over the
    padded 17 x kChebPadRow copy the oscillator builds at block rate (Stage 4
    Round B, Decision 28). clenshaw2D remains the reference form and the
    off-thread evaluator for the unpadded 153-float triangle (ChebyshevProjector,
    TerrainScheduler's readout, TerrainViewFeed). The set itself is built off the
    audio thread (ChebyshevProjector), published by atomic pointer and retired
    through the type-erased reaper (Retirable).

  ==============================================================================
*/

#pragma once
#include "Retirable.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

constexpr int kChebDegree = 16;
constexpr int kChebCoeffs = 153;   // (16 + 1)(16 + 2) / 2

/** Start index of row n in the row-major triangle: Σ_{k<n} (17 − k). */
constexpr int chebRowStart (int n) noexcept
{
    return 17 * n - (n * (n - 1)) / 2;
}

static_assert (chebRowStart (kChebDegree + 1) == kChebCoeffs, "triangle size");

/** What a set was projected FROM. The scheduler quantises F / modX / modY to
    1/1024 before building a key (plan Decision 31), so equality is exact and
    float noise never re-projects. `terrain` is the TerrainKind index the set
    stands for (6 = Imported: the image's embedded set, plan Decision 35). */
struct ChebKey
{
    int terrain = 0;
    float F = 1.0f, modX = 0.5f, modY = 0.5f;
    int imageRevision = 0;

    uint32_t hash() const noexcept
    {
        auto mix = [] (uint32_t h, uint32_t v) { h ^= v + 0x9E3779B9u + (h << 6) + (h >> 2); return h; };
        uint32_t h = 0x811C9DC5u;
        h = mix (h, static_cast<uint32_t> (terrain));
        h = mix (h, static_cast<uint32_t> (std::lround (F * 1024.0f)));
        h = mix (h, static_cast<uint32_t> (std::lround (modX * 1024.0f)));
        h = mix (h, static_cast<uint32_t> (std::lround (modY * 1024.0f)));
        h = mix (h, static_cast<uint32_t> (imageRevision));
        return h;
    }
    bool operator== (const ChebKey& o) const noexcept
    {
        // exact compare on purpose: every value is quantised to 1/1024 before it becomes a key
        return terrain == o.terrain && imageRevision == o.imageRevision
            && std::lround (F * 1024.0f) == std::lround (o.F * 1024.0f)
            && std::lround (modX * 1024.0f) == std::lround (o.modX * 1024.0f)
            && std::lround (modY * 1024.0f) == std::lround (o.modY * 1024.0f);
    }
    bool operator!= (const ChebKey& o) const noexcept { return ! (*this == o); }
};

/** One published coefficient set. Immutable once published; the oscillator never
    dereferences the pointer inside its sample loop (it copies the tapered
    coefficients at block start, plan Decision 28). `liveCount` is the Round B
    leak verdict (LSan is unavailable on Apple Silicon — RESEARCH §2.5). */
struct ChebyshevSet final : Retirable
{
    ChebKey key;
    std::array<float, kChebCoeffs> c {};
    float fit = 0.0f;   // 100 · (1 − ‖f − Pf‖² / ‖f‖²) on the projection nodes

    ChebyshevSet()  { liveCount.fetch_add (1, std::memory_order_relaxed); }
    ~ChebyshevSet() override { liveCount.fetch_sub (1, std::memory_order_relaxed); }
    ChebyshevSet (const ChebyshevSet&) = delete;
    ChebyshevSet& operator= (const ChebyshevSet&) = delete;

    inline static std::atomic<int> liveCount { 0 };
};

// ─── Truncation law (plan Decision 27) — shared by the oscillator (taper), the
// scheduler (partialsAtC4 readout) and the harness (H6 Bandlimited gate) so no
// fixture mirrors it. D_c = max (1, fs / (2 K f) − 1) is the real diagonal cut-off:
// (D_c + 1) · K · f <= fs / 2 keeps one harmonic of margin (± 1 semitone of pitch
// modulation). w (d) = 1 for d <= 1 (h1 never mutes), a raised cosine from D_c − 2
// down to 0 at D_c, 0 above. D_max = clamp (⌊D_c⌋, 1, 16) is the integer readout.

inline double chebDiagonalCutoff (double fs, int K, double f) noexcept
{
    if (f <= 0.0 || K <= 0 || fs <= 0.0) return 16.0;
    const double dc = 0.5 * fs / (static_cast<double> (K) * f) - 1.0;
    return dc < 1.0 ? 1.0 : dc;
}

inline int chebDMax (double dc) noexcept
{
    const int d = static_cast<int> (std::floor (dc));
    return d < 1 ? 1 : (d > kChebDegree ? kChebDegree : d);
}

inline float chebTaperWeight (int d, double dc) noexcept
{
    if (d <= 1) return 1.0f;
    double u = static_cast<double> (d) - dc + 2.0;
    u = u < 0.0 ? 0.0 : (u > 2.0 ? 2.0 : u);
    return static_cast<float> (0.5 * (1.0 + std::cos (3.14159265358979323846 * u * 0.5)));
}

/** f(x, y) = Σ_{n+m<=16} c_nm T_n(x) T_m(y) by 2-D Clenshaw. `c` is any 153-float
    triangle (the set's own or the oscillator's tapered copy). Audio-thread read
    path: no allocation, no pow, no data-dependent branch. */
inline float clenshaw2D (const float* c, float x, float y) noexcept
{
    const float x2 = 2.0f * x, y2 = 2.0f * y;
    float g[kChebDegree + 1];

    // inner: g_n(y) = Σ_m c_nm T_m(y), row n has 17 − n coefficients
    for (int n = 0; n <= kChebDegree; ++n)
    {
        const float* row = c + chebRowStart (n);
        const int last = kChebDegree - n;
        float b1 = 0.0f, b2 = 0.0f;
        for (int m = last; m >= 1; --m)
        {
            const float b0 = row[m] + y2 * b1 - b2;
            b2 = b1; b1 = b0;
        }
        g[n] = row[0] + y * b1 - b2;
    }

    // outer: Σ_n g_n T_n(x)
    float b1 = 0.0f, b2 = 0.0f;
    for (int n = kChebDegree; n >= 1; --n)
    {
        const float b0 = g[n] + x2 * b1 - b2;
        b2 = b1; b1 = b0;
    }
    return g[0] + x * b1 - b2;
}


/** Audio-thread layout (Stage 4 Round B, Decision 28): each of the 17 rows padded to
    kChebPadRow floats, cp[n * kChebPadRow + m], pad lanes zero. buildChebWeights
    writes it by loop index; the evaluator has fixed trip counts and no data branch. */
constexpr int kChebPadRow = 20;
constexpr int kChebPadded = (kChebDegree + 1) * kChebPadRow;   // 340
static_assert (kChebPadRow >= kChebDegree + 1 && kChebPadRow % 4 == 0, "pad row");

/** T_0..T_16 (x) by the three-term recurrence. */
inline void chebBasis17 (float x, float* T) noexcept
{
    T[0] = 1.0f; T[1] = x;
    const float x2 = 2.0f * x;
    for (int k = 2; k <= kChebDegree; ++k) T[k] = x2 * T[k - 1] - T[k - 2];
}

/** f(x, y) = Σ_{n+m<=16} c_nm T_n(x) T_m(y) on a padded copy (17 x kChebPadRow).
    Per-row 4-lane dot products, even / odd rows into separate accumulators;
    max |Δ| vs clenshaw2D <= 2e-5 on a uniform ±0.5 set, <= 1e-6 on a projected set
    (harness --gate clenshaw). Portable plain C — MSVC x64 and clang auto-vectorise it. */
inline float chebEvalPadded (const float* cp, float x, float y) noexcept
{
    float T[kChebDegree + 1]; chebBasis17 (x, T);
    alignas (16) float U[kChebPadRow] = {}; chebBasis17 (y, U);   // U[17..19] stay 0
    float accE[4] = {}, accO[4] = {};
    for (int n = 0; n <= kChebDegree; n += 2)
    {
        const float* row = cp + n * kChebPadRow;
        float g[4] = {};
        for (int j = 0; j < kChebPadRow; j += 4)
            for (int l = 0; l < 4; ++l) g[l] += row[j + l] * U[j + l];
        for (int l = 0; l < 4; ++l) accE[l] += g[l] * T[n];
        if (n + 1 <= kChebDegree)
        {
            const float* r1 = row + kChebPadRow; float h[4] = {};
            for (int j = 0; j < kChebPadRow; j += 4)
                for (int l = 0; l < 4; ++l) h[l] += r1[j + l] * U[j + l];
            for (int l = 0; l < 4; ++l) accO[l] += h[l] * T[n + 1];
        }
    }
    return ((accE[0] + accO[0]) + (accE[1] + accO[1])) + ((accE[2] + accO[2]) + (accE[3] + accO[3]));
}
