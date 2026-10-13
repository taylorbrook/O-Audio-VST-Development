/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    ShRotator — third-order real spherical-harmonic rotation (Phase 2.2).

    THE LESSON this class carries: ambisonics is not panning. The scene is
    rotated AFTER encoding, in the SH domain, by a block-diagonal 16 × 16
    matrix R_SH = diag (1, R₁, R₂, R₃). The same matrix would rotate a
    thousand sources at once; here it rotates one, and Phase 2.3's binaural
    decoder consumes the rotated bus.

    PROVENANCE (CONTEXT-2.2 D9). Own transcription of the recursion of
    Ivanic & Ruedenberg, "Rotation Matrices for Real Spherical Harmonics.
    Direct Determination by Recursion", J. Phys. Chem. 100 (1996) 6342, with
    the 1998 erratum (J. Phys. Chem. A 102 (1998) 9099). Written from the
    paper's structure; IEM SceneRotator (GPL-3.0) and Green, "Spherical
    Harmonic Lighting: The Gritty Details" (2003) were READ for the index
    conventions only — nothing is lifted. The transcription is the one
    validated numerically in RESEARCH-2.2 R1: 1.8e-15 (double) / 1.3e-7
    (float32) against re-encoding at the rotated direction over 200 random
    (az, el, ψ, θ, φ) tuples, orthogonality 1.2e-15.

    CONVENTIONS
      • Cartesian frame: x front, y left, z up (AmbiX). Direction of a source
        at (az, el): d = (cos e·cos a, cos e·sin a, sin e).
      • R₃ = Rz(ψ) · Ry′(θ) · Rx(φ) — ARCHITECTURE §SH rotation:
            Rz(ψ)  = [[ c, −s, 0], [ s, c, 0], [ 0, 0, 1]]   +ψ: front → LEFT
            Ry′(θ) = [[ c, 0, −s], [ 0, 1, 0], [ s, 0, c]]   +θ: front RISES
            Rx(φ)  = [[ 1, 0, 0], [ 0, c, −s], [ 0, s, c]]   +φ: left RISES
      • Order-1 block in ACN order (Y = y, Z = z, X = x): the paper's
        m, n ∈ {−1, 0, +1} map to ACN 1, 2, 3, so
            R₁[m][n] = R₃[p(m + 1)][p(n + 1)],  p = (1, 2, 0).
        This is the ACTIVE convention (a source at d moves to R₃·d) and it
        falls out of the recursion directly — there is NO transpose anywhere
        in this file (R1: the transposed matrix misses the oracle by 1.24).
      • Paper indices m, n ∈ [−l, l] ↔ ACN l² + l + m.
      • SN3D and N3D share the same rotation matrices (per-order scaling
        commutes with each block), so the SN3D bus needs no conversion.

    STORAGE (RESEARCH-2.2 R5). The 83 nonzero elements (9 + 25 + 49) of
    R₁, R₂, R₃ live contiguously, row-major per block:
        rotIndex (l, m, n) = blockOffset (l) + (m + l)·(2l + 1) + (n + l),
        blockOffset = { 0, 9, 34 }.
    W (ACN 0) is IMPLICIT 1 — never stored, lerped or multiplied (D2):
    apply() copies in[0] to out[0]. The recursion reads R_{l−1} through
    rotIndex (l − 1, ·, ·) on the SAME array, so build() fills R₁ → R₂ → R₃
    in place with no scratch.

    PRECISION. Everything is float (sinf / cosf / sqrtf; no double anywhere
    in Source/dsp/, B2). The identity is EXACT in float (R3): at
    ψ = θ = φ = 0 every one of the 83 elements is exactly 0 or 1
    (sqrtf (2)·sqrtf (2) == 2.0f in IEEE single), so a zero rotation is
    bit-transparent on the bus and the Phase 2.1 render digest stays the
    regression verdict (D3). The only possible bit difference is a signed
    zero (−0 → +0) on an exactly-zero bus channel.

    COST. u / v / w depend only on (l, m, n): three float[83] tables filled
    once in prepare() (B3) — no sqrt on the audio thread. One build() per
    control step ≈ 700 flops + 3 sinf + 3 cosf. applyLerped() per sample =
    83 lerps + 83 MACs = 332 flops ≈ 16 MFLOPS at 48 kHz (the corrected D1
    number; CONTEXT-2.2 D1's "≈ 4" counted the multiplies only).

    Headless: <algorithm>, <cmath> + ShEncoder.h (kNumSh, kDegToRad). No JUCE.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>

#include "ShEncoder.h"

namespace OSimpleAmbisonics::dsp
{

// Radians → degrees: the one new literal of Phase 2.2 (the frame's azRot /
// elRot are published in degrees). kDegToRad lives in ShEncoder.h.
inline constexpr float kRadToDeg = 57.29577951308232f;

// 9 + 25 + 49 nonzero elements of the block-diagonal R_SH (W implicit).
inline constexpr int kNumRotElems = 83;

// First element of block l (l = 1, 2, 3) in the 83-array.
constexpr int blockOffset (int l) noexcept
{
    return l == 1 ? 0 : l == 2 ? 9 : 34;
}

// Element (m, n) of block l, paper indices m, n ∈ [−l, l]; row-major.
constexpr int rotIndex (int l, int m, int n) noexcept
{
    return blockOffset (l) + (m + l) * (2 * l + 1) + (n + l);
}

static_assert (blockOffset (3) + 49 == kNumRotElems, "83-element layout: 9 + 25 + 49");
static_assert (rotIndex (1, -1, -1) == 0 && rotIndex (1, 1, 1) == 8, "block 1 spans [0, 9)");
static_assert (rotIndex (2, -2, -2) == 9 && rotIndex (2, 2, 2) == 33, "block 2 spans [9, 34)");
static_assert (rotIndex (3, -3, -3) == 34 && rotIndex (3, 3, 3) == 82, "block 3 spans [34, 83)");

class ShRotator
{
public:
    ShRotator() = default;

    // Fills the u / v / w coefficient tables (B3). Called once from
    // SpatialEngine::prepare() before the first build(); never on the audio
    // thread (std::sqrt). Entries for l = 1 are unused (0).
    void prepare() noexcept
    {
        std::fill (std::begin (uTab), std::end (uTab), 0.0f);
        std::fill (std::begin (vTab), std::end (vTab), 0.0f);
        std::fill (std::begin (wTab), std::end (wTab), 0.0f);

        sqrt2 = std::sqrt (2.0f);

        for (int l = 2; l <= 3; ++l)
            for (int m = -l; m <= l; ++m)
                for (int n = -l; n <= l; ++n)
                {
                    const int   am    = m < 0 ? -m : m;
                    const int   delta = m == 0 ? 1 : 0;
                    const float denom = (n == l || n == -l) ? (float) ((2 * l) * (2 * l - 1))
                                                            : (float) ((l + n) * (l - n));

                    const int k = rotIndex (l, m, n);

                    // u = sqrt ((l+m)(l−m) / denom)
                    uTab[k] = std::sqrt ((float) ((l + m) * (l - m)) / denom);

                    // v = ½·sqrt ((1+δ)(l+|m|−1)(l+|m|) / denom)·(1 − 2δ)
                    vTab[k] = 0.5f * std::sqrt ((float) ((1 + delta) * (l + am - 1) * (l + am)) / denom)
                                   * (float) (1 - 2 * delta);

                    // w = −½·sqrt ((l−|m|−1)(l−|m|) / denom)·(1 − δ); 0 at |m| ≥ l − 1
                    const int wNum = (l - am - 1) * (l - am);
                    wTab[k] = wNum <= 0 ? 0.0f
                                        : -0.5f * std::sqrt ((float) wNum / denom) * (float) (1 - delta);
                }
    }

    // R₃ = Rz(ψ) · (Ry′(θ) · Rx(φ)) from degrees. Signs: +ψ front → left;
    // +θ front rises; +φ left rises (ARCHITECTURE §SH rotation).
    static void buildCartesian (float yawDeg, float pitchDeg, float rollDeg, float R3[3][3]) noexcept
    {
        const float cz = std::cos (yawDeg * kDegToRad),   sz = std::sin (yawDeg * kDegToRad);
        const float cy = std::cos (pitchDeg * kDegToRad), sy = std::sin (pitchDeg * kDegToRad);
        const float cx = std::cos (rollDeg * kDegToRad),  sx = std::sin (rollDeg * kDegToRad);

        const float Rz[3][3] = { {  cz, -sz, 0.0f }, {   sz,   cz, 0.0f }, { 0.0f, 0.0f, 1.0f } };
        const float Ry[3][3] = { {  cy, 0.0f,  -sy }, { 0.0f, 1.0f, 0.0f }, {   sy, 0.0f,   cy } };
        const float Rx[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f,   cx,  -sx }, { 0.0f,   sx,   cx } };

        float YX[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                YX[i][j] = Ry[i][0] * Rx[0][j] + Ry[i][1] * Rx[1][j] + Ry[i][2] * Rx[2][j];

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                R3[i][j] = Rz[i][0] * YX[0][j] + Rz[i][1] * YX[1][j] + Rz[i][2] * YX[2][j];
    }

    // Builds the 83 elements of R_SH (and returns R₃) for the three angles.
    // R₁ by the ACN permutation, then R₂ from R₁, then R₃ from R₁ and R₂ —
    // each read back through rotIndex on R itself. One call per control step.
    void build (float yawDeg, float pitchDeg, float rollDeg,
                float R[kNumRotElems], float R3[3][3]) const noexcept
    {
        buildCartesian (yawDeg, pitchDeg, rollDeg, R3);

        // R₁[m][n] = R₃[p(m+1)][p(n+1)], p = (1, 2, 0): ACN (Y, Z, X) = (y, z, x).
        constexpr int p[3] = { 1, 2, 0 };
        for (int m = -1; m <= 1; ++m)
            for (int n = -1; n <= 1; ++n)
                R[rotIndex (1, m, n)] = R3[p[m + 1]][p[n + 1]];

        for (int l = 2; l <= 3; ++l)
            for (int m = -l; m <= l; ++m)
                for (int n = -l; n <= l; ++n)
                {
                    const int   k = rotIndex (l, m, n);
                    const float u = uTab[k], v = vTab[k], w = wTab[k];

                    float acc = 0.0f;

                    // U — only when u ≠ 0: at |m| = l, P would read R_{l−1} at a = ±l (out of the block).
                    if (u != 0.0f)
                        acc += u * P (0, m, n, l, R);

                    // V — never zero for l ≥ 2.
                    float V;
                    if (m == 0)
                        V = P (1, 1, n, l, R) + P (-1, -1, n, l, R);
                    else if (m > 0)
                        V = P (1, m - 1, n, l, R) * (m == 1 ? sqrt2 : 1.0f)
                          - P (-1, -m + 1, n, l, R) * (m == 1 ? 0.0f : 1.0f);
                    else
                        V = P (1, m + 1, n, l, R) * (m == -1 ? 0.0f : 1.0f)
                          + P (-1, -m - 1, n, l, R) * (m == -1 ? sqrt2 : 1.0f);
                    acc += v * V;

                    // W — only when w ≠ 0: w = 0 at m = 0 (never evaluated) and at |m| = l − 1
                    // (P would read a = ±l). Not an optimisation — a guard.
                    if (w != 0.0f)
                    {
                        const float W = m > 0 ? P (1, m + 1, n, l, R) + P (-1, -m - 1, n, l, R)
                                              : P (1, m - 1, n, l, R) - P (-1, -m + 1, n, l, R);
                        acc += w * W;
                    }

                    R[k] = acc;
                }
    }

    // out = R_SH · in with W implicit (out[0] = in[0]). Used once per control
    // step for the meter frame (shGain = R_SH · c_eff). in and out may alias
    // only if the caller does not need `in` afterwards — the engine passes
    // distinct arrays.
    static void apply (const float R[kNumRotElems], const float in[kNumSh], float out[kNumSh]) noexcept
    {
        out[0] = in[0];

        for (int l = 1; l <= 3; ++l)
        {
            const int base = l * l + l;
            for (int m = -l; m <= l; ++m)
            {
                float acc = 0.0f;
                for (int n = -l; n <= l; ++n)
                    acc += R[rotIndex (l, m, n)] * in[base + n];
                out[base + m] = acc;
            }
        }
    }

    // Fused lerp-and-apply (B5): out = (prev + t·(cur − prev)) · in, W implicit.
    // A3 blend form, A5 t. `in` and `out` MUST be distinct arrays (the engine
    // passes bus → tap). At prev == cur == I every element is exactly 1 or 0,
    // so out == in to the bit (up to a −0 → +0 on an exactly-zero channel).
    static void applyLerped (const float prev[kNumRotElems], const float cur[kNumRotElems], float t,
                             const float in[kNumSh], float out[kNumSh]) noexcept
    {
        out[0] = in[0];

        for (int l = 1; l <= 3; ++l)
        {
            const int base = l * l + l;
            for (int m = -l; m <= l; ++m)
            {
                float acc = 0.0f;
                for (int n = -l; n <= l; ++n)
                {
                    const int k = rotIndex (l, m, n);
                    acc += (prev[k] + t * (cur[k] - prev[k])) * in[base + n];
                }
                out[base + m] = acc;
            }
        }
    }

private:
    // P (i, a, b, l) — the paper's three cases, R₁ indexed by i ∈ {−1, 0, +1}
    // and R_{l−1} by the centred (a, b), both read from R through rotIndex.
    static float P (int i, int a, int b, int l, const float* R) noexcept
    {
        const int lp = l - 1;

        if (b < l && b > -l)
            return R[rotIndex (1, i, 0)] * R[rotIndex (lp, a, b)];

        if (b == l)
            return R[rotIndex (1, i, 1)] * R[rotIndex (lp, a, l - 1)]
                 - R[rotIndex (1, i, -1)] * R[rotIndex (lp, a, -l + 1)];

        // b == −l
        return R[rotIndex (1, i, 1)] * R[rotIndex (lp, a, -l + 1)]
             + R[rotIndex (1, i, -1)] * R[rotIndex (lp, a, l - 1)];
    }

    float uTab[kNumRotElems] {};
    float vTab[kNumRotElems] {};
    float wTab[kNumRotElems] {};
    float sqrt2 = 1.4142135f;   // overwritten by prepare() with std::sqrt (2.0f)
};

} // namespace OSimpleAmbisonics::dsp
