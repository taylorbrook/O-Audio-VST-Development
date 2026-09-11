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

    Orbits.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    The 11 closed orbits (ARCHITECTURE Core 2, FUNC-03). Clean-room formulas —
    mathematics, not code. Every base curve is evaluated raw here and scaled by
    OrbitScratch::normFactor (1 / max radius over θ at the current Orbit Mod),
    which TerrainOscillator::updateBlockRate refreshes once per block from a
    64-point scan when m moved by more than 1e-3 (plan Decision 15).

    Audio-thread budget (DSP-05): no pow, no std::function, no allocation.
    Superellipse reads r(θ, n) from a 33 × 129 LUT built once under
    std::call_once from prepare() (plan Decision 14); Squarcle uses the Padé
    tanh on a ±3-clamped argument with 1/tanh k cached per block (RESEARCH §2.3).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <mutex>

// Order = the osc?Orbit choice list (asserted against the parameter in
// PluginProcessor.cpp createOscParameters).
enum class OrbitKind
{
    Ellipse = 0,
    Superellipse,
    Limacon,
    Epitrochoid3,
    Epitrochoid5,
    Epitrochoid7,
    Hypocycloid3,
    Hypocycloid5,
    Hypocycloid7,
    Butterfly,
    Squarcle
};

static constexpr int kNumOrbitKinds = 11;

struct OrbitPoint
{
    float x, y;
};

/** Per-oscillator block constants filled by TerrainOscillator::updateBlockRate. */
struct OrbitScratch
{
    float normFactor = 1.0f;   // 1 / max radius of the raw curve at the block's Orbit Mod
    float invTanhK   = 1.0f;   // Squarcle 1 / tanh k
};

/** Trigonometric-polynomial degree K of each orbit (0 = not a trig polynomial;
    Round B's Bandlimited D_max uses it). */
constexpr int orbitK (OrbitKind k)
{
    switch (k)
    {
        case OrbitKind::Ellipse:      return 1;
        case OrbitKind::Superellipse: return 0;
        case OrbitKind::Limacon:      return 2;
        case OrbitKind::Epitrochoid3: return 4;
        case OrbitKind::Epitrochoid5: return 6;
        case OrbitKind::Epitrochoid7: return 8;
        case OrbitKind::Hypocycloid3: return 2;
        case OrbitKind::Hypocycloid5: return 4;
        case OrbitKind::Hypocycloid7: return 6;
        case OrbitKind::Butterfly:    return 0;
        case OrbitKind::Squarcle:     return 0;
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
// Superellipse LUT: r(θ, n) = (|cos θ|ⁿ + |sin θ|ⁿ)^(−1/n), n ∈ [0.5, 6],
// θ over one quadrant [0, π/2] (the curve has 4-fold reflection symmetry).
// ═══════════════════════════════════════════════════════════════════

struct SuperellipseLUT
{
    static constexpr int kN     = 33;    // n rows over [0.5, 6]
    static constexpr int kTheta = 129;   // θ columns over [0, π/2]
    static constexpr float kNMin = 0.5f, kNMax = 6.0f;

    /** Shared table, built on first call (prepare-time only, never on the audio thread). */
    static const float* get()
    {
        static std::array<float, kN * kTheta> table;
        static std::once_flag once;
        std::call_once (once, [] { build (table.data()); });
        return table.data();
    }

    /** Bilinear read; θ in radians (any value), n ∈ [0.5, 6]. */
    static inline float lookup (const float* table, float theta, float n) noexcept
    {
        constexpr float kQuadrant = 1.5707963267948966f;
        // Reduce θ into [0, π/2): the function is symmetric under θ → −θ and θ → θ + π/2.
        float t = theta - kQuadrant * std::floor (theta / kQuadrant);
        if (t < 0.0f) t = 0.0f;
        const float tf = t * (float (kTheta - 1) / kQuadrant);
        int ti = int (tf);
        if (ti > kTheta - 2) ti = kTheta - 2;
        const float tfr = tf - float (ti);

        const float nf = juce::jlimit (0.0f, float (kN - 1), (n - kNMin) * (float (kN - 1) / (kNMax - kNMin)));
        int ni = int (nf);
        if (ni > kN - 2) ni = kN - 2;
        const float nfr = nf - float (ni);

        const float* r0 = table + ni * kTheta;
        const float* r1 = r0 + kTheta;
        const float a = r0[ti] + tfr * (r0[ti + 1] - r0[ti]);
        const float b = r1[ti] + tfr * (r1[ti + 1] - r1[ti]);
        return a + nfr * (b - a);
    }

private:
    // prepare-time only — std::pow is allowed here and nowhere else in this file.
    static void build (float* table)
    {
        for (int ni = 0; ni < kN; ++ni)
        {
            const double n = kNMin + (kNMax - kNMin) * double (ni) / double (kN - 1);
            for (int ti = 0; ti < kTheta; ++ti)
            {
                const double theta = 1.5707963267948966 * double (ti) / double (kTheta - 1);
                const double c = std::abs (std::cos (theta)), s = std::abs (std::sin (theta));
                const double sum = std::pow (c, n) + std::pow (s, n);   // prepare-time only
                table[ni * kTheta + ti] = float (std::pow (sum, -1.0 / n)); // prepare-time only
            }
        }
    }
};

// ═══════════════════════════════════════════════════════════════════
// Base curves (raw, before normalisation / affine)
// ═══════════════════════════════════════════════════════════════════

namespace OrbitDetail
{
    inline OrbitPoint polar (float r, float c, float s) noexcept { return { r * c, r * s }; }

    /** Raw curve at θ for Orbit Mod m. Used by the norm scan and by baseOrbit. */
    inline OrbitPoint rawOrbit (OrbitKind kind, float theta, float m,
                                const OrbitScratch& scratch, const float* superLUT) noexcept
    {
        const float c = std::cos (theta);
        const float s = std::sin (theta);

        switch (kind)
        {
            case OrbitKind::Ellipse:
                return { c, s };

            case OrbitKind::Superellipse:
                return polar (SuperellipseLUT::lookup (superLUT, theta, 0.5f + 5.5f * m), c, s);

            case OrbitKind::Limacon:
                return polar (1.0f + 2.0f * m * c, c, s);

            case OrbitKind::Epitrochoid3:
            case OrbitKind::Epitrochoid5:
            case OrbitKind::Epitrochoid7:
            {
                const float p1 = kind == OrbitKind::Epitrochoid3 ? 4.0f
                               : kind == OrbitKind::Epitrochoid5 ? 6.0f : 8.0f;   // p + 1
                const float d = 0.1f + 2.9f * m;
                return { p1 * c - d * std::cos (p1 * theta),
                         p1 * s - d * std::sin (p1 * theta) };
            }

            case OrbitKind::Hypocycloid3:
            case OrbitKind::Hypocycloid5:
            case OrbitKind::Hypocycloid7:
            {
                const float pm = kind == OrbitKind::Hypocycloid3 ? 2.0f
                               : kind == OrbitKind::Hypocycloid5 ? 4.0f : 6.0f;   // p − 1
                const float d = 0.1f + 1.9f * m;
                return { pm * c + d * std::cos (pm * theta),
                         pm * s - d * std::sin (pm * theta) };
            }

            case OrbitKind::Butterfly:
            {
                const float s2 = s * s;
                const float s5 = s2 * s2 * s;
                const float r = std::exp (c) - 2.0f * std::cos (4.0f * theta) + 2.0f * m * s5;
                return polar (r, c, s);
            }

            case OrbitKind::Squarcle:
            {
                const float k = 0.3f + 6.0f * m;
                const float ax = juce::jlimit (-3.0f, 3.0f, k * c);
                const float ay = juce::jlimit (-3.0f, 3.0f, k * s);
                return { juce::dsp::FastMathApproximations::tanh (ax) * scratch.invTanhK,
                         juce::dsp::FastMathApproximations::tanh (ay) * scratch.invTanhK };
            }
        }
        return { c, s };
    }

    /** Max radius of the raw curve over 64 θ points (block-rate; plan Decision 15). */
    inline float maxRadius (OrbitKind kind, float m, const OrbitScratch& scratch, const float* superLUT) noexcept
    {
        if (kind == OrbitKind::Ellipse)
            return 1.0f;
        float maxR2 = 1.0e-12f;
        for (int i = 0; i < 64; ++i)
        {
            const float theta = 6.283185307179586f * float (i) / 64.0f;
            const auto p = rawOrbit (kind, theta, m, scratch, superLUT);
            maxR2 = juce::jmax (maxR2, p.x * p.x + p.y * p.y);
        }
        return std::sqrt (maxR2);
    }
}

/** Normalised base orbit (max radius 1 at the block's Orbit Mod). */
inline OrbitPoint baseOrbit (OrbitKind kind, float theta, float m,
                             const OrbitScratch& scratch, const float* superLUT) noexcept
{
    auto p = OrbitDetail::rawOrbit (kind, theta, m, scratch, superLUT);
    p.x *= scratch.normFactor;
    p.y *= scratch.normFactor;
    return p;
}
