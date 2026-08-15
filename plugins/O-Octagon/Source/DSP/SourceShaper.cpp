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
#include "SourceShaper.h"

#include "../Data/VenueGeometry.h"

#include <cmath>

namespace oo::shaper
{

SubPoints shape (const VenueSnapshot& v, float srcXNorm, float srcYNorm,
                 float srcZ, float widthMetres) noexcept
{
    // ── Step 1 — puck to metres ───────────────────────────────────────────────────────────────
    // Via oo::plane, NOT a second denormalisation written out here. The zero-span guard is stated
    // once, in one file, and VenueModel delegates to the same function (P14).
    const Vec2 p = plane::normToMetres (v.bbMinX, v.bbMaxX, v.bbMinY, v.bbMaxY, srcXNorm, srcYNorm);

    // ── Step 2 — bearing from the RIG CENTROID ────────────────────────────────────────────────
    // The centroid, not the bounding-box centre: the centroid is what the array physically "is",
    // and it is what the level field is symmetric about (§3.4.1).
    const Vec2  b    = { p.x - v.centroid.x, p.y - v.centroid.y };
    const float bLen = len (b);

    // ── Step 3 — fade the spread to zero near the centroid (§3.4.2) ───────────────────────────
    // Without this, a puck sweeping through the centroid flips b̂ by 180°, n̂ with it, and L and R
    // swap sides INSTANTANEOUSLY — a 6 m jump of both sub-points in one control block at width = 6.
    // Collapsing the spread first means the flip happens while both sub-points are already
    // coincident with the puck, so the gain vectors stay continuous.
    const float rFade = kFadeFraction * v.rigScale;

    // A degenerate rig (rigScale == 0, every speaker coincident) has no meaningful spread axis, and
    // |b| is 0 there too — so the ratio would be 0/0. Collapse rather than divide.
    const float fadeRatio = bLen / rFade;
    const float fade      = rFade > kBearingEpsilon ? (fadeRatio < 1.0f ? fadeRatio : 1.0f) : 0.0f;

    const float wEff = widthMetres * fade;

    // ── Step 4 — unit bearing and its left-hand perpendicular ─────────────────────────────────
    // At |b| < eps the puck IS the centroid and "outward" is undefined. The specified fallback is
    // (0, −1) — toward the stage — which makes the spread axis the room's left-right axis, the only
    // choice a listener would call neutral.
    const Vec2 bHat = bLen < kBearingEpsilon ? Vec2 { 0.0f, -1.0f }
                                             : Vec2 { b.x / bLen, b.y / bLen };

    // HANDEDNESS: x increases to the audience's right. Puck downstage of centre → b̂ = (0,−1) →
    // n̂ = (1,0) = audience right, so P_R = P + (wEff/2)·n̂ really does put R on the right.
    // Asserted by probe AH, not by this comment.
    const Vec2 nHat = { -bHat.y, bHat.x };

    // ── Step 5 — the two sub-points ───────────────────────────────────────────────────────────
    const float half = 0.5f * wEff;

    const Vec2 pL = { p.x - half * nHat.x, p.y - half * nHat.y };
    const Vec2 pR = { p.x + half * nHat.x, p.y + half * nHat.y };

    // ── Step 6 — each sub-point resolves ITS OWN height at ITS OWN y ──────────────────────────
    // The plane slopes in y and the spread has a y component whenever the bearing is not purely
    // fore-aft, so this must NOT be hoisted out of the pair.
    const auto heightAt = [&v, srcZ] (float y) noexcept
    {
        return plane::earHeight (v.rakeFront, v.rakeRear, v.bbMinY, v.bbMaxY, y) + srcZ;
    };

    SubPoints out;
    out.left  = { pL.x, pL.y, heightAt (pL.y) };
    out.right = { pR.x, pR.y, heightAt (pR.y) };
    out.wEff  = wEff;

    return out;
}

} // namespace oo::shaper
