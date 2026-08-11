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

#include <array>

#include "Vec.h"

namespace oo
{

//==============================================================================
/**
    Free functions over raw hull storage — `(const Vec2* pts, int count)`.

    These, not the ConvexHull2D members, are what the audio thread calls at Phase 2.2: the RT path
    holds a VenueSnapshot, which stores the hull as a plain `hullPts[8]` + `hullCount` pair. Writing
    the tests here and the members as thin delegates means the message-thread diagnostic and the
    audio-thread solve provably run the SAME geometry — there is no second implementation to drift.

    None of them allocate, lock, or branch on anything but the point count.
*/
namespace hull
{
    /** Signed-area (cross-product) tolerance, scaled to the room.

        EPS_CROSS is an AREA, not a length: 1e-6 · spanX · spanY. A fixed length tolerance would be
        meaningless in a 40 m hall and hypersensitive in a 3 m studio (ARCHITECTURE §3.1.2).
        A degenerate span yields 0, which correctly makes the collinearity test exact.
    */
    float crossEpsilonFor (float spanX, float spanY) noexcept;

    /** Inside-or-on test. 8 cross products worst case; no allocation, no data-dependent branching.

        Handles the degenerate counts explicitly (ARCHITECTURE §3.1.6):
          count >= 3 → the CCW polygon test
          count == 2 → on the single segment within EPS_ONEDGE
          count == 1 → within EPS_ONEDGE of the single point
          count <= 0 → false
    */
    bool isInside (const Vec2* pts, int count, Vec2 p, float epsCross) noexcept;

    struct Projection
    {
        Vec2  point    {};      ///< nearest point on the hull boundary
        float distance { 0.0f };///< d_hull, always >= 0 and always finite
    };

    /** Nearest point on the hull boundary, and its distance.

        At Phase 2.2 this is called ONLY from inside the `if (! isInside(...))` branch — PERF-01
        acceptance criterion 3 is satisfied structurally by that placement, not by a comment.
    */
    Projection project (const Vec2* pts, int count, Vec2 p) noexcept;
}

//==============================================================================
/**
    Andrew's monotone chain hull over the (x, y) floor projection of the 8 speakers, plus speaker
    classification.

    Message thread only — rebuilt on any venue edit. O(n log n) with n = 8, i.e. microseconds.

    THE DIMENSIONAL SPLIT IS INTENTIONAL (ARCHITECTURE §3.1.1): the hull is 2D on the floor while
    DBAP distances are 3D. A source at srcZ = +8 m above room centre is therefore INSIDE the hull
    and receives no hull attenuation and no air filtering. That is the designed behaviour — srcZ is
    a musical control, not an error condition — and must not be "fixed".
*/
class ConvexHull2D
{
public:
    static constexpr int kNumSpeakers = 8;

    /** Two points closer than this are one point. A duplicate coordinate is a legitimate
        venue-entry error, not a crash case. */
    static constexpr float EPS_DEDUP  = 1.0e-4f;

    /** Perpendicular distance below which a speaker counts as lying on a hull edge. */
    static constexpr float EPS_ONEDGE = 1.0e-3f;

    enum class Classification
    {
        VERTEX,     ///< a strict corner of the hull
        ON_EDGE,    ///< on the boundary but not a corner — collinear with its wall neighbours
        INTERIOR    ///< strictly inside
    };

    ConvexHull2D() = default;

    /** Rebuilds from the 3D speaker positions, using their (x, y) floor projection.

        Takes Vec3 rather than Vec2 deliberately: the caller cannot accidentally project on some
        other pair of axes, and the dimensional split stays stated in one place.
    */
    void build (const std::array<Vec3, kNumSpeakers>& speakers) noexcept;

    int  getNumHullPoints() const noexcept                        { return hullCount; }
    Vec2 getHullPoint (int i) const noexcept;
    const std::array<Vec2, kNumSpeakers>& getHullPoints() const noexcept { return hullPts; }

    /** Which original speaker each hull vertex came from, or −1 past `hullCount`. */
    int getHullPointSource (int i) const noexcept;

    float getCrossEpsilon() const noexcept                        { return epsCross; }

    /** Classification of an original speaker (0-based) against the built hull.

        A FIRST-CLASS RETURN VALUE, not a processor accessor and not a test-only hook (PLAN-2.1
        P11): the Phase 3.2 Venue screen reads this same call, so the diagnostic the user sees and
        the one the tests assert cannot diverge.

        This is not decoration. It tells a user who has just typed measured coordinates that (say)
        speaker 3 has moved from on-edge to interior, which changes hull behaviour.
    */
    Classification classify (int speakerIndex) const noexcept;

    bool isInside (Vec2 p) const noexcept;

    hull::Projection project (Vec2 p) const noexcept;

private:
    std::array<Vec2, kNumSpeakers> hullPts {};
    std::array<int,  kNumSpeakers> hullSrc { -1, -1, -1, -1, -1, -1, -1, -1 };
    std::array<Vec2, kNumSpeakers> sourcePts {};

    int   hullCount { 0 };
    float epsCross  { 0.0f };
};

} // namespace oo
