/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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

#include "PerlinNoise.h"
#include "Vec.h"

#include <cmath>

namespace oo::motion
{

/**
    v1.8.0 — THE SINGLE PATH GENERATOR (RESEARCH Q6).

    Three consumers compile this one function: GainStage::updateControl() on the audio thread,
    the editor's getMotionTrace native function on the message thread, and the geometry unit
    target. That is what makes the venue-map trace non-drifting — the page never re-implements a
    path, it translates and projects what this returns (pattern_test_fixture_mirrors_drift_silently).

    ── Coordinates ──────────────────────────────────────────────────────────────────────────────
    Anchor-relative VENUE METRES (D2). The caller adds the result to the puck's metres AFTER
    plane::normToMetres, so "Size = 6 m" is a 6 m figure in any hall. `sizeM` is the path's
    EXTENT (diameter): R = size / 2, so the number on the knob reads as the ring's width on the map.

    ── Phase ────────────────────────────────────────────────────────────────────────────────────
    `cycles` is a pure function of absolute position, supplied by MotionClock.h. NOTHING here
    accumulates: evaluate() is stateless and noexcept, so two calls with equal arguments are
    bit-identical — which is the whole of R4/R5 as far as this file is concerned.

    ── The six equations (RESEARCH §Implementation Approach), t = 2π·frac(cycles) + phase ────────
      Orbit     x = R cos t,        y = R·ratio·sin t
      Figure-8  x = R sin t,        y = R·ratio·sin 2t          (Lissajous 1:2 — closes at t = 2π)
      Sweep     x = R·(2·fold − 1), y = 0                       (fold removes the saw wrap)
      Drift     x = R·fbm(n),       y = R·ratio·fbm(n + 1000), n = cycles (seeded, deterministic)
      Pendulum  x = R sin t,        y = 0
      Spiral    x = R·s·cos t,      y = R·ratio·s·sin t, s = fold(u) (in over the first half, out
                                                                     over the second)
      all       z = height·sin t   (D4; Drift: height·fbm(n + 2000))
    then (x, y) rotated about the anchor by `angleDeg`, so Sweep and Pendulum are not pinned to the
    room's x axis.

    No JUCE. <cmath>, Vec.h and PerlinNoise.h only.
*/
enum Path : int
{
    orbit = 0,
    figure8,
    sweep,
    drift,
    pendulum,
    spiral,
    kNumPaths
};

/// True for every path that closes over one cycle — i.e. everything but Drift. The trace is drawn
/// only for cyclic paths; Drift gets a trailing tail of polled positions instead (plan call).
inline constexpr bool cyclic (int path) noexcept
{
    return path != static_cast<int> (drift);
}

struct MotionParams
{
    int   path     { 0 };
    float sizeM    { 6.0f };   // extent (diameter), metres
    float ratio    { 1.0f };   // minor / major axis, 0..1
    float angleDeg { 0.0f };   // rotation about the anchor
    float heightM  { 0.0f };   // z amplitude, metres
    float phaseDeg { 0.0f };   // phase offset
};

inline constexpr double kTwoPi = 6.283185307179586476925286766559;
inline constexpr float  kDegToRad = 0.017453292519943295769f;

/// The ping-pong fold: u in [0, 1) -> [0, 1] and back, continuous at 0.5 and at the wrap.
inline constexpr float fold (float u) noexcept
{
    return u < 0.5f ? 2.0f * u : 2.0f - 2.0f * u;
}

/** Evaluates the path at `cycles` (whole cycles since phase zero). Pure, allocation-free, noexcept.

    @param perlin  a SEEDED table. Read only; the caller owns seeding (GainStage re-seeds at the
                   grid boundary when motionSeed changes — a bounded table fill, no allocation).
*/
inline Vec3 evaluate (const MotionParams& m, double cycles, const PerlinNoise& perlin) noexcept
{
    const float R = 0.5f * m.sizeM;

    // frac() in double, THEN narrow: a long timeline's cycles would lose cycle precision in float
    // (the same reason O-Orbit derives its PPQ phase in double).
    const double fracD = cycles - std::floor (cycles);
    const float  u     = static_cast<float> (fracD);
    const float  t     = static_cast<float> (kTwoPi * fracD) + m.phaseDeg * kDegToRad;

    float x = 0.0f, y = 0.0f, z = 0.0f;

    switch (m.path)
    {
        case orbit:
            x = R * std::cos (t);
            y = R * m.ratio * std::sin (t);
            z = m.heightM * std::sin (t);
            break;

        case figure8:
            x = R * std::sin (t);
            y = R * m.ratio * std::sin (2.0f * t);
            z = m.heightM * std::sin (t);
            break;

        case sweep:
        {
            // The phase offset shifts the fold's argument the same way it shifts t, so Phase turns
            // the sweep as it turns every other path.
            const float uu = u + m.phaseDeg / 360.0f;
            const float f  = fold (uu - std::floor (uu));
            x = R * (2.0f * f - 1.0f);
            y = 0.0f;
            z = m.heightM * std::sin (t);
            break;
        }

        case drift:
        {
            const float n = static_cast<float> (cycles);
            x = R * perlin.fbm (n);
            y = R * m.ratio * perlin.fbm (n + 1000.0f);
            z = m.heightM * perlin.fbm (n + 2000.0f);
            break;
        }

        case pendulum:
            x = R * std::sin (t);
            y = 0.0f;
            z = m.heightM * std::sin (t);
            break;

        case spiral:
        {
            const float uu = u + m.phaseDeg / 360.0f;
            const float s  = fold (uu - std::floor (uu));
            x = R * s * std::cos (t);
            y = R * m.ratio * s * std::sin (t);
            z = m.heightM * std::sin (t);
            break;
        }

        default:
            break;
    }

    // Rotation about the anchor. Skipped EXACTLY at angle 0 so the unrotated equations above are
    // what a probe compares against (cos 0 = 1 and sin 0 = 0 would round-trip bit-identically
    // anyway, but the branch makes it structural rather than arithmetical).
    if (m.angleDeg != 0.0f)
    {
        const float a  = m.angleDeg * kDegToRad;
        const float ca = std::cos (a);
        const float sa = std::sin (a);
        const float rx = x * ca - y * sa;
        const float ry = x * sa + y * ca;
        x = rx;
        y = ry;
    }

    return { x, y, z };
}

} // namespace oo::motion
