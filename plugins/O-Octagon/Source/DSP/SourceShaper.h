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

#include "../Data/VenueSnapshot.h"
#include "Vec.h"

namespace oo
{

//==============================================================================
/** The two solve positions the gain stage feeds to DBAP, and the effective spread that produced
    them. */
struct SubPoints
{
    Vec3  left  {};
    Vec3  right {};

    /// `width` after the rFade collapse — 0 when the sub-points coincide.
    float wEff { 0.0f };
};

//==============================================================================
/**
    ARCHITECTURE §3.4.1 — puck to two sub-points, steps 1 through 6, IN FULL.

    ── Created at Phase 2.2, not 2.3, and why (PLAN-2.2 P15 / RESEARCH-2.2 H3) ────────────────────
    ROADMAP assigns this file to 2.3, but DBAP needs metres and srcX/srcY are normalised, so 2.2 has
    to implement §5 step 2 and the per-sub-point rake resolution regardless of which file they live
    in. Inlining them into GainStage and extracting at 2.3 would write the two zero-span guards
    twice — which is D1's own argument, one level up.

    ── LIVE SINCE PHASE 2.3 ──────────────────────────────────────────────────────────────────────
    GainStage::updateControl() passes `p[params::width]`. At width = 0 the sub-points coincide, v_L
    is bit-for-bit v_R, and the per-sample sum degenerates to v_i · 0.5·(L+R) with NO BRANCH
    (§3.4.3) — the general path in its degenerate configuration, not a special case. Probe AY
    asserts that bitwise identity and carries a width = 4 negative control; DSP-06 closed at 2.3.

    At 2.2 this file was complete but its caller passed a literal 0.0f, so probe AH drove shape()
    directly as coverage for code shipped inert. AH still runs, and AX is the DSP-06 claim.

    No JUCE: <cmath>, Vec.h, VenueSnapshot.h and VenueGeometry.h only, so the narrow unit-target link
    line survives.
*/
namespace shaper
{
    /// §3.4.1 step 3. Room-relative so the collapse radius is venue-portable (≈ 0.40 m by default).
    ///
    /// v1.3.0: 0.15 → 0.05. At 0.15 the collapse radius was ~1.19 m on the default rig, and the
    /// DEFAULT centre puck sits only ~0.46 m from the centroid — so the shipped position suppressed
    /// width to 39% of its dialled value, exactly where a user first tries the control. 0.05 keeps
    /// the collapse (the 180° axis flip still happens only while the sub-points are coincident at
    /// the exact centroid) but shrinks the guarded zone below the default puck offset. The faster
    /// axis rotation near the centroid is still smoothed by the 5 ms gain ramps.
    inline constexpr float kFadeFraction = 0.05f;

    /// §3.4.1 step 4. Below this bearing length the puck is AT the centroid and the direction of
    /// "outward" is undefined.
    inline constexpr float kBearingEpsilon = 1.0e-6f;

    /** Resolves the puck to metres and splits it into two sub-points.

        @param v            the published room. Only bbox, centroid, rigScale and the two rake
                            values are read.
        @param srcXNorm     srcX as the host sees it, 0..1
        @param srcYNorm     srcY as the host sees it, 0..1
        @param srcZ         height offset in metres, ADDED to the audience plane
        @param widthMetres  the spread. 0 collapses both sub-points onto the puck.
    */
    SubPoints shape (const VenueSnapshot& v, float srcXNorm, float srcYNorm,
                     float srcZ, float widthMetres) noexcept;

    /** v1.8.0 — steps 2 through 6 from a puck ALREADY IN METRES (RESEARCH Q2).

        The motion engine's offset is metric and belongs AFTER step 1's denormalisation (D2), so
        the split sits at exactly that seam: shape() is step 1 followed by this. GainStage takes
        this entry on the motion branch and the verbatim shape() on the off branch — probe DC holds
        the off branch against the v1.7.0 binary's digest.

        @param metres  the puck in venue metres (bbox-denormalised srcX/srcY plus the offset)
    */
    SubPoints shapeAt (const VenueSnapshot& v, Vec2 metres,
                       float srcZ, float widthMetres) noexcept;
}

} // namespace oo
