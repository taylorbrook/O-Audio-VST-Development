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

    ShEncoder — AmbiX (ACN ordering, SN3D normalisation, no Condon–Shortley
    phase, sin(|m|·a) for m < 0) third-order spherical-harmonic encoder.

    PROVENANCE: kSqrt3_2 / kSqrt15_2 / kSqrt5_8 / kSqrt3_8, AmbiTrigCache and
    encodeSH16() are lifted VERBATIM from
    plugins/O-GrainScatter/Source/dsp/SpatialEncoder.h (lines 33–103),
    which derives them from research/ambisonics-encoding-deep-dive.md §2.
    Verified against the generic SN3D triple product
    N_n^|m| · P_n^|m|(sin e) · T_m(a) to 4.7e-16 in double and 2.8e-08 in
    float on a 13 × 7 (az × el) grid — RESEARCH-2.1 R4. The harness DSP-01
    gate repeats that comparison against its own independent table
    (tests/render-harness/ReferenceModel.h), which shares nothing with this
    file but <cmath>.

    O-GrainScatter's SmoothedSHCoeffs (per-sample one-pole on the 16
    coefficients) is deliberately NOT lifted: 2.1 smooths the ANGLES at the
    control grid and lerps the resulting vector per sample (A1 / A5).

    Input angles are RADIANS. The engine converts once per control step
    (deg · kDegToRad). azimuth 0 = front, +π/2 = LEFT (AmbiX), elevation
    +π/2 = up.

    Headless: <cmath> only. No JUCE.

  ==============================================================================
*/

#pragma once

#include <cmath>

namespace OSimpleAmbisonics::dsp
{

inline constexpr int kNumSh = 16;

// Degrees → radians, the ONE literal both the engine and the harness use.
inline constexpr float kDegToRad = 0.017453292519943295f;

// ── Lifted verbatim from O-GrainScatter SpatialEncoder.h:33–103 ───────────────

// SN3D normalization constants
static constexpr float kSqrt3_2  = 0.86602540378f;  // sqrt(3)/2
static constexpr float kSqrt15_2 = 1.93649167310f;  // sqrt(15)/2
static constexpr float kSqrt5_8  = 0.79056941504f;  // sqrt(5/8)
static constexpr float kSqrt3_8  = 0.61237243570f;  // sqrt(3/8)

// Precomputed trig values for SH encoding.
// Uses double/triple-angle identities to derive all terms from just
// sin(a), cos(a), sin(e), cos(e) -- only 2 trig call pairs needed.
struct AmbiTrigCache
{
    float sinA, cosA;
    float sinE, cosE;
    float sin2A, cos2A;
    float sin3A, cos3A;
    float cosE2, cosE3;
    float sinE2;
    float sin2E;

    void compute (float azimuth, float elevation)
    {
        sinA = std::sin (azimuth);
        cosA = std::cos (azimuth);
        sinE = std::sin (elevation);
        cosE = std::cos (elevation);

        sin2A = 2.0f * sinA * cosA;
        cos2A = cosA * cosA - sinA * sinA;

        float sinA2 = sinA * sinA;
        float cosA2 = cosA * cosA;
        sin3A = sinA * (3.0f * cosA2 - sinA2);
        cos3A = cosA * (cosA2 - 3.0f * sinA2);

        cosE2 = cosE * cosE;
        cosE3 = cosE2 * cosE;
        sinE2 = sinE * sinE;
        sin2E = 2.0f * sinE * cosE;
    }
};

// Compute all 16 SN3D encoding coefficients for direction (azimuth, elevation).
// azimuth: 0=front, +pi/2=left, -pi/2=right  (radians)
// elevation: 0=horizon, +pi/2=up, -pi/2=down  (radians)
inline void encodeSH16 (float azimuth, float elevation, float* outCoeffs)
{
    AmbiTrigCache c;
    c.compute (azimuth, elevation);

    // Order 0
    outCoeffs[0]  = 1.0f;

    // Order 1
    outCoeffs[1]  = c.sinA * c.cosE;          // Y
    outCoeffs[2]  = c.sinE;                    // Z
    outCoeffs[3]  = c.cosA * c.cosE;          // X

    // Order 2
    outCoeffs[4]  = kSqrt3_2 * c.sin2A * c.cosE2;
    outCoeffs[5]  = kSqrt3_2 * c.sinA * c.sin2E;
    outCoeffs[6]  = 0.5f * (3.0f * c.sinE2 - 1.0f);
    outCoeffs[7]  = kSqrt3_2 * c.cosA * c.sin2E;
    outCoeffs[8]  = kSqrt3_2 * c.cos2A * c.cosE2;

    // Order 3
    outCoeffs[9]  = kSqrt5_8 * c.sin3A * c.cosE3;
    outCoeffs[10] = kSqrt15_2 * c.sin2A * c.sinE * c.cosE2;
    outCoeffs[11] = kSqrt3_8 * c.sinA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    outCoeffs[12] = 0.5f * c.sinE * (5.0f * c.sinE2 - 3.0f);
    outCoeffs[13] = kSqrt3_8 * c.cosA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    outCoeffs[14] = kSqrt15_2 * c.cos2A * c.sinE * c.cosE2;
    outCoeffs[15] = kSqrt5_8 * c.cos3A * c.cosE3;
}

// ── End of the verbatim lift ──────────────────────────────────────────────────

// Order band of an ACN index: 0 → 0; 1–3 → 1; 4–8 → 2; 9–15 → 3.
inline constexpr int shBand (int acn) noexcept
{
    return acn == 0 ? 0 : acn < 4 ? 1 : acn < 9 ? 2 : 3;
}

// c_eff[acn] = Y_acn · orderGain[band(acn)]; orderGain[0] ≡ 1 by contract
// (ARCHITECTURE §SH Encoder, order gating). Bands above `order` fade to
// EXACTLY zero because the gains are LinearRamps (F1).
inline void applyOrderGains (const float* y16, const float orderGain[4], float* out16) noexcept
{
    for (int acn = 0; acn < kNumSh; ++acn)
        out16[acn] = y16[acn] * orderGain[shBand (acn)];
}

} // namespace OSimpleAmbisonics::dsp
