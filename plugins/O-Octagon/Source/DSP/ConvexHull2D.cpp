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
#include "ConvexHull2D.h"

#include <algorithm>
#include <limits>

namespace oo
{

//==============================================================================
namespace hull
{

/** Squared length below which an edge is treated as a point rather than a segment. */
static constexpr float EPS_LEN2 = 1.0e-12f;

float crossEpsilonFor (float spanX, float spanY) noexcept
{
    return 1.0e-6f * spanX * spanY;
}

//==============================================================================
/** Nearest point on the segment a→b to p. Degenerate (zero-length) segments return a. */
static Vec2 nearestOnSegment (Vec2 a, Vec2 b, Vec2 p) noexcept
{
    const Vec2  ab   = sub (b, a);
    const float ab2  = len2 (ab);

    // std::max, not a divide-and-hope: a zero-length edge would otherwise produce inf or NaN and
    // propagate straight into the gain vector at Phase 2.2.
    const float tRaw = dot (sub (p, a), ab) / std::max (ab2, EPS_LEN2);
    const float t    = std::min (1.0f, std::max (0.0f, tRaw));

    return { a.x + t * ab.x, a.y + t * ab.y };
}

//==============================================================================
bool isInside (const Vec2* pts, int count, Vec2 p, float epsCross) noexcept
{
    if (pts == nullptr || count <= 0)
        return false;

    // ── Degenerate counts, per the ARCHITECTURE §3.1.6 matrix ────────────────────────────────
    // These are routed by CHAIN LENGTH, explicitly, rather than being allowed to fall through the
    // polygon loop — a 2-point "polygon" has two zero-area edges and every point would test as
    // inside.
    if (count == 1)
        return len (sub (p, pts[0])) < ConvexHull2D::EPS_ONEDGE;

    if (count == 2)
        return len (sub (p, nearestOnSegment (pts[0], pts[1], p))) < ConvexHull2D::EPS_ONEDGE;

    // ── The CCW polygon test (ARCHITECTURE §3.1.4) ───────────────────────────────────────────
    // For a counter-clockwise hull, p is inside-or-on iff it is left-of-or-on every directed edge.
    for (int i = 0; i < count; ++i)
    {
        const Vec2 a = pts[i];
        const Vec2 b = pts[(i + 1) % count];

        if (cross (sub (b, a), sub (p, a)) < -epsCross)
            return false;
    }

    return true;
}

//==============================================================================
Projection project (const Vec2* pts, int count, Vec2 p) noexcept
{
    if (pts == nullptr || count <= 0)
        return { p, 0.0f };

    if (count == 1)
        return { pts[0], len (sub (p, pts[0])) };

    // A 2-point hull has exactly ONE segment. Iterating (i, (i+1)%count) would visit it twice —
    // harmless for the minimum, but the explicit case documents the degeneracy rather than relying
    // on the reader to notice it is benign.
    if (count == 2)
    {
        const Vec2 q = nearestOnSegment (pts[0], pts[1], p);
        return { q, len (sub (p, q)) };
    }

    Vec2  best     { pts[0] };
    float bestDist { std::numeric_limits<float>::max() };

    for (int i = 0; i < count; ++i)
    {
        const Vec2  q = nearestOnSegment (pts[i], pts[(i + 1) % count], p);
        const float d = len (sub (p, q));

        if (d < bestDist)
        {
            bestDist = d;
            best     = q;
        }
    }

    return { best, bestDist };
}

} // namespace hull

//==============================================================================
namespace
{
    struct IndexedPoint
    {
        Vec2 p   {};
        int  src { -1 };   ///< originating speaker index, 0-based
    };
}

//==============================================================================
void ConvexHull2D::build (const std::array<Vec3, kNumSpeakers>& speakers) noexcept
{
    hullPts.fill (Vec2 {});
    hullSrc.fill (-1);
    hullCount = 0;

    for (int i = 0; i < kNumSpeakers; ++i)
        sourcePts[(size_t) i] = floorOf (speakers[(size_t) i]);

    // ── EPS_CROSS from the actual footprint ──────────────────────────────────────────────────
    float minX = sourcePts[0].x, maxX = sourcePts[0].x;
    float minY = sourcePts[0].y, maxY = sourcePts[0].y;

    for (const auto& q : sourcePts)
    {
        minX = std::min (minX, q.x);  maxX = std::max (maxX, q.x);
        minY = std::min (minY, q.y);  maxY = std::max (maxY, q.y);
    }

    epsCross = hull::crossEpsilonFor (maxX - minX, maxY - minY);

    // ── STEP 0 — deduplicate ─────────────────────────────────────────────────────────────────
    // Two points within EPS_DEDUP are one point; the LOWEST speaker index is kept as the
    // representative. Speakers collapsed away here are still classified in classify(), by
    // coordinate rather than by identity.
    std::array<IndexedPoint, kNumSpeakers> pts {};
    int n = 0;

    constexpr float dedup2 = EPS_DEDUP * EPS_DEDUP;

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        bool duplicate = false;

        for (int j = 0; j < n && ! duplicate; ++j)
            duplicate = len2 (sub (sourcePts[(size_t) i], pts[(size_t) j].p)) < dedup2;

        if (! duplicate)
            pts[(size_t) n++] = { sourcePts[(size_t) i], i };
    }

    if (n == 1)
    {
        hullPts[0] = pts[0].p;
        hullSrc[0] = pts[0].src;
        hullCount  = 1;
        return;
    }

    // ── STEP 1 — sort lexicographically by (x, then y) ───────────────────────────────────────
    std::sort (pts.begin(), pts.begin() + n,
               [] (const IndexedPoint& a, const IndexedPoint& b)
               {
                   // Written as two strict `<` tests rather than `if (a.x != b.x)`: the repo builds
                   // with -Wfloat-equal (JUCEHelperTargets.cmake:76) and the zero-warning gate is
                   // hard. This form is also the textbook strict weak ordering.
                   if (a.p.x < b.p.x) return true;
                   if (b.p.x < a.p.x) return false;

                   return a.p.y < b.p.y;
               });

    // ── STEP 2 — monotone chain ──────────────────────────────────────────────────────────────
    //
    // The comparison is `<= epsCross`, so COLLINEAR POINTS ARE POPPED. This yields strict vertices
    // only, and it is the entire reason speakers 3 and 8 of the traced layout come out ON_EDGE
    // rather than as vertices. Changing it to `<` would silently reclassify them and quietly alter
    // the hull-attenuation behaviour of the shipped default venue.
    std::array<IndexedPoint, 2 * kNumSpeakers> chain {};
    int k = 0;

    const auto popsAgainst = [&chain, this] (int kk, Vec2 candidate)
    {
        return cross (sub (chain[(size_t) (kk - 1)].p, chain[(size_t) (kk - 2)].p),
                      sub (candidate, chain[(size_t) (kk - 2)].p)) <= epsCross;
    };

    for (int i = 0; i < n; ++i)                      // lower hull
    {
        while (k >= 2 && popsAgainst (k, pts[(size_t) i].p))
            --k;

        chain[(size_t) k++] = pts[(size_t) i];
    }

    const int lowerEnd = k + 1;

    for (int i = n - 2; i >= 0; --i)                 // upper hull
    {
        while (k >= lowerEnd && popsAgainst (k, pts[(size_t) i].p))
            --k;

        chain[(size_t) k++] = pts[(size_t) i];
    }

    // The chain closes on its own first point; drop the duplicate.
    hullCount = std::min (k - 1, kNumSpeakers);

    for (int i = 0; i < hullCount; ++i)
    {
        hullPts[(size_t) i] = chain[(size_t) i].p;
        hullSrc[(size_t) i] = chain[(size_t) i].src;
    }

    // ── Winding ──────────────────────────────────────────────────────────────────────────────
    // Andrew's chain in this orientation produces counter-clockwise output, and the inside test
    // depends on that. Rather than trusting it, measure the signed area and reverse once if it
    // came out clockwise — the check is four multiplies on a venue edit.
    if (hullCount >= 3)
    {
        float signedAreaX2 = 0.0f;

        for (int i = 0; i < hullCount; ++i)
            signedAreaX2 += cross (hullPts[(size_t) i], hullPts[(size_t) ((i + 1) % hullCount)]);

        if (signedAreaX2 < 0.0f)
        {
            std::reverse (hullPts.begin(), hullPts.begin() + hullCount);
            std::reverse (hullSrc.begin(), hullSrc.begin() + hullCount);
        }
    }
}

//==============================================================================
Vec2 ConvexHull2D::getHullPoint (int i) const noexcept
{
    return (i >= 0 && i < hullCount) ? hullPts[(size_t) i] : Vec2 {};
}

int ConvexHull2D::getHullPointSource (int i) const noexcept
{
    return (i >= 0 && i < hullCount) ? hullSrc[(size_t) i] : -1;
}

//==============================================================================
ConvexHull2D::Classification ConvexHull2D::classify (int speakerIndex) const noexcept
{
    if (speakerIndex < 0 || speakerIndex >= kNumSpeakers || hullCount <= 0)
        return Classification::INTERIOR;

    const Vec2 p = sourcePts[(size_t) speakerIndex];

    // VERTEX by identity — the speaker's own index survived the chain.
    for (int i = 0; i < hullCount; ++i)
        if (hullSrc[(size_t) i] == speakerIndex)
            return Classification::VERTEX;

    // VERTEX by coordinate — the speaker was collapsed by STEP 0 onto a point that IS a vertex.
    // ARCHITECTURE §3.1.3 defines VERTEX as "index appears in hullPts", which leaves this case
    // ambiguous; a duplicate sitting exactly on a corner is a corner, and reporting it as ON_EDGE
    // would mislead a user who has just typed two identical coordinates by mistake.
    constexpr float dedup2 = EPS_DEDUP * EPS_DEDUP;

    for (int i = 0; i < hullCount; ++i)
        if (len2 (sub (p, hullPts[(size_t) i])) < dedup2)
            return Classification::VERTEX;

    // ON_EDGE — minimum perpendicular distance to any hull edge segment. project() already routes
    // the count == 1 and count == 2 degeneracies, so no branch here divides by zero.
    if (hull::project (hullPts.data(), hullCount, p).distance < EPS_ONEDGE)
        return Classification::ON_EDGE;

    return Classification::INTERIOR;
}

bool ConvexHull2D::isInside (Vec2 p) const noexcept
{
    return hull::isInside (hullPts.data(), hullCount, p, epsCross);
}

hull::Projection ConvexHull2D::project (Vec2 p) const noexcept
{
    return hull::project (hullPts.data(), hullCount, p);
}

} // namespace oo
