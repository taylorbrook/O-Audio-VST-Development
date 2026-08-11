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
/*
  ==============================================================================

    O-Octagon geometry unit target — Phase 2.1 (Geometry Core) correctness gate.

    Sixteen probes over the three components that decide where sound goes before any gain exists
    to confuse a diagnosis:

      A  Layer 1 — runtime enum-bit-order invariant, all three accepted 8-channel sets
      B  Layer 2 — generated golden vs runtime order, all three sets
      C  Layer 2 — committed SHA vs generated SHA (static_assert, plus a readable runtime print)
      D  NON-IDENTITY map, same container — a hardcoded 0..7 map fails this immediately
      E  Cross-container — 7.1 labels against 7.1-SDDS: 4 of 8 types absent
      F  Duplicate label -> map rejected, last valid map retained
      G  Corrupt numeric label -> a PLAUSIBLE discrete type, rejected by the permutation check
      H  Hull of the §OQ4 default venue: vertices 1,2,4,5,6,7; speakers 3 and 8 ON_EDGE
      I  Near-collinear point set — the EPS_CROSS path probe H does not reach
      J  Rear room corner classifies outside
      K  200-point projection vs an INDEPENDENT oracle, pinned seed
      L  Degeneracy matrix — collinear, coincident, zero rake span, degenerate bbox
      M  earHeight linearity, rakeRear independence, zero-span guard
      N  srcZ = 0 at the rear of a steep rake is never below rear-row ear height
      O  rigScale value AND the scaling invariant
      P  Missing and partial VENUE nodes both yield the §OQ4 defaults, per attribute

    Exit 0 iff every probe passes.

  ==============================================================================
*/
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string_view>
#include <vector>

#include "Data/VenueModel.h"
#include "Data/VenueSnapshot.h"
#include "DSP/ChannelMap.h"
#include "DSP/ConvexHull2D.h"

#include "JuceChannelOrderGolden.h"

//==============================================================================
// ── PROBE C: the Layer-2 build-time gate ──────────────────────────────────────────────────────
//
// ROADMAP:131 requires Layer 2 to FAIL THE BUILD, not merely fail a test run — a non-zero exit
// code is a test failure, and a test target nobody configured is a test that never ran. So the
// comparison is a static_assert.
//
// The SHA covers the canonical serialisation of the PARSED DATA (the referenced name->value pairs
// and the three derived orders), never the emitted file text. Hashing the text would let a comment
// reflow churn the value and train everyone to re-bless it without reading.
//
// IF THIS FIRES: JUCE's ChannelType enum values or the membership of create7point1() /
// create7point1SDDS() / create5point1point2() has CHANGED. Every speaker->buffer map in this
// plugin is derived from that ordering. Re-read the diff, confirm the new order is correct, and
// only then paste the new hash here.
inline constexpr char kCommittedChannelOrderSha256[]
    = "5cd774cb228888a28f1526d127ce4ee1d076386b26a98ef8315b35a4abdd7b00";

static_assert (std::string_view (juce_golden::kGeneratedChannelOrderSha256)
                   == std::string_view (kCommittedChannelOrderSha256),
               "JUCE's ChannelType enum values or 8-channel set membership have CHANGED. The "
               "speaker->buffer channel map is derived from that ordering, and a wrong map is "
               "SILENT — it passes auval, passes pluginval at strictness 10, and is audible only "
               "in the hall. Read the JUCE diff before re-blessing the hash in tests/unit/main.cpp.");

//==============================================================================
namespace
{

using CT = juce::AudioChannelSet::ChannelType;

int  failures = 0;
int  probes   = 0;

void check (const char* name, bool ok, const juce::String& detail)
{
    ++probes;

    if (! ok)
        ++failures;

    std::printf ("  [%s] %-34s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
}

//==============================================================================
/** Tolerant float comparison. Written rather than using `==` because the repo builds with
    -Wfloat-equal and the zero-warning gate is hard. */
bool near (float a, float b, float tol) noexcept
{
    return std::abs (a - b) <= tol;
}

/** Bit-exact float comparison via the object representation — no `==`, no epsilon, no warning. */
bool bitExact (float a, float b) noexcept
{
    return std::memcmp (&a, &b, sizeof (float)) == 0;
}

bool allFinite (const oo::VenueModel& v)
{
    for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
    {
        const auto p = v.speaker (i);

        if (! (std::isfinite (p.x) && std::isfinite (p.y) && std::isfinite (p.z)))
            return false;
    }

    return std::isfinite (v.rigScale()) && std::isfinite (v.centroid().x)
        && std::isfinite (v.centroid().y) && std::isfinite (v.centroid().z)
        && std::isfinite (v.bbMinX()) && std::isfinite (v.bbMaxX())
        && std::isfinite (v.bbMinY()) && std::isfinite (v.bbMaxY());
}

std::array<CT, ochan::kNumSpeakers> labelsOf (const oo::VenueModel& v)
{
    return v.labelTypes();
}

juce::String mapToString (const std::array<int, ochan::kNumSpeakers>& m)
{
    juce::String s = "{";

    for (int i = 0; i < ochan::kNumSpeakers; ++i)
        s << (i > 0 ? "," : "") << m[(size_t) i];

    return s + "}";
}

//==============================================================================
/** An INDEPENDENT oracle for the nearest point on the hull boundary.

    Ternary search for the minimum of a convex 1-D function along each edge, in DOUBLE precision.
    Deliberately NOT the closed-form dot-product projection that ConvexHull2D uses: an oracle that
    reruns the implementation's own formula reproduces its errors and passes forever
    (pattern_test_fixture_mirrors_drift_silently). Distance-to-a-point along a segment is convex in
    the parameter, so ternary search is guaranteed to find the true minimum.
*/
double oracleDistanceToHull (const std::array<oo::Vec2, 8>& pts, int count, oo::Vec2 p)
{
    const double px = static_cast<double> (p.x);
    const double py = static_cast<double> (p.y);

    const auto distAt = [&] (oo::Vec2 a, oo::Vec2 b, double t)
    {
        const double x = static_cast<double> (a.x) + t * (static_cast<double> (b.x) - static_cast<double> (a.x));
        const double y = static_cast<double> (a.y) + t * (static_cast<double> (b.y) - static_cast<double> (a.y));

        return std::sqrt ((px - x) * (px - x) + (py - y) * (py - y));
    };

    if (count <= 0)
        return 0.0;

    if (count == 1)
        return distAt (pts[0], pts[0], 0.0);

    const int numEdges = (count == 2) ? 1 : count;

    double best = std::numeric_limits<double>::max();

    for (int e = 0; e < numEdges; ++e)
    {
        const oo::Vec2 a = pts[(size_t) e];
        const oo::Vec2 b = pts[(size_t) ((e + 1) % count)];

        double lo = 0.0, hi = 1.0;

        for (int it = 0; it < 200; ++it)
        {
            const double m1 = lo + (hi - lo) / 3.0;
            const double m2 = hi - (hi - lo) / 3.0;

            if (distAt (a, b, m1) < distAt (a, b, m2))
                hi = m2;
            else
                lo = m1;
        }

        best = std::min (best, distAt (a, b, 0.5 * (lo + hi)));
    }

    return best;
}

} // namespace

//==============================================================================
int main()
{
    std::printf ("\nO-Octagon geometry unit target — Phase 2.1 (Geometry Core)\n");
    std::printf ("==========================================================\n\n");

    const auto set71     = juce::AudioChannelSet::create7point1();
    const auto set71sdds = juce::AudioChannelSet::create7point1SDDS();
    const auto set512    = juce::AudioChannelSet::create5point1point2();

    //==========================================================================
    // A — Layer 1: the runtime enum-bit-order invariant.
    {
        juce::String detail;
        bool ok = true;

        const std::array<std::pair<const char*, juce::AudioChannelSet>, 3> sets
            { { { "7.1", set71 }, { "7.1-SDDS", set71sdds }, { "5.1.2", set512 } } };

        for (const auto& s : sets)
        {
            juce::String whyNot;

            if (! ochan::verifyEnumBitOrder (s.second, &whyNot))
            {
                ok = false;
                detail << s.first << ": " << whyNot << "  ";
            }
        }

        if (ok)
            detail = "all 3 accepted 8-channel sets, scan bound "
                   + juce::String (ochan::kMaxChannelTypeScan) + " + size assertion";

        check ("A layer1-enum-bit-order", ok, detail);
    }

    //==========================================================================
    // B — Layer 2: the generated golden vs the runtime order.
    //
    // The golden is derived from PARSED JUCE SOURCE at build time. Nothing here is a hand-written
    // constant, which is the whole point: a mirrored fixture drifts with the thing it checks.
    {
        struct Case { const char* name; const juce::AudioChannelSet* set; const int* golden; };

        const std::array<Case, 3> cases
            { { { "7.1",      &set71,     juce_golden::kCreate7point1 },
                { "7.1-SDDS", &set71sdds, juce_golden::kCreate7point1SDDS },
                { "5.1.2",    &set512,    juce_golden::kCreate5point1point2 } } };

        bool ok = true;
        juce::String detail;

        for (const auto& c : cases)
        {
            if (c.set->size() != juce_golden::kNumChannels)
            {
                ok = false;
                detail << c.name << ": runtime size " << c.set->size() << " != golden "
                       << juce_golden::kNumChannels << "  ";
                continue;
            }

            for (int i = 0; i < juce_golden::kNumChannels; ++i)
            {
                const int runtime = static_cast<int> (c.set->getTypeOfChannel (i));

                if (runtime != c.golden[i])
                {
                    ok = false;
                    detail << c.name << ": slot " << i << " runtime " << runtime
                           << " != golden " << c.golden[i] << "  ";
                }
            }
        }

        if (ok)
            detail = "7.1 {1,2,3,4,10,11,20,21} | SDDS {1,2,3,4,5,6,7,8} | 5.1.2 {1,2,3,4,5,6,28,29}";

        check ("B layer2-golden-vs-runtime", ok, detail);
    }

    //==========================================================================
    // C — Layer 2: the checksum. The static_assert above has already fired at build time if this
    // is wrong; the runtime print exists so the diagnostic is READABLE when it does.
    {
        const bool ok = std::string_view (juce_golden::kGeneratedChannelOrderSha256)
                            == std::string_view (kCommittedChannelOrderSha256);

        check ("C layer2-sha256", ok,
               juce::String ("generated ") + juce_golden::kGeneratedChannelOrderSha256
                   + (ok ? "" : juce::String (" != committed ") + kCommittedChannelOrderSha256));
    }

    //==========================================================================
    // D — A NON-IDENTITY map inside one container.
    //
    // THE SHIPPED DEFAULT LABEL MAP IS THE IDENTITY MAP UNDER ALL THREE ACCEPTED CONTAINERS, so a
    // probe driven by it alone is vacuous: a hardcoded 0..7 would pass. Rotating the labels by one
    // is the cheapest assignment that cannot be satisfied by a hardcoded map (RESEARCH-2.1 C1/G5).
    {
        const std::array<CT, 8> rotated
            { juce::AudioChannelSet::right,             // speaker 1 -> buffer 1
              juce::AudioChannelSet::centre,            //           -> 2
              juce::AudioChannelSet::LFE,               //           -> 3
              juce::AudioChannelSet::leftSurroundSide,  //           -> 4
              juce::AudioChannelSet::rightSurroundSide, //           -> 5
              juce::AudioChannelSet::leftSurroundRear,  //           -> 6
              juce::AudioChannelSet::rightSurroundRear, //           -> 7
              juce::AudioChannelSet::left };            // speaker 8 -> 0

        const std::array<int, 8> expected { 1, 2, 3, 4, 5, 6, 7, 0 };

        std::array<int, 8> got { -1, -1, -1, -1, -1, -1, -1, -1 };
        const bool built = ochan::buildSpeakerToBuffer (set71, rotated, got);

        const bool ok = built && got == expected;

        check ("D non-identity-map-same-set", ok,
               juce::String (built ? "built " : "BUILD FAILED ") + mapToString (got)
                   + ", expected " + mapToString (expected));
    }

    //==========================================================================
    // E — Cross-container. 7.1's labels against 7.1-SDDS: leftSurroundSide, rightSurroundSide,
    // leftSurroundRear and rightSurroundRear are absent from SDDS, so 4 of 8 lookups return −1.
    // This is the ROADMAP:131 missing-label test built from real JUCE sets rather than a synthetic
    // bad value.
    {
        oo::VenueModel v;                                  // shipped default = the 7.1 identity map

        const std::array<int, 8> sentinel { 9, 9, 9, 9, 9, 9, 9, 9 };
        std::array<int, 8> got = sentinel;

        const bool built = ochan::buildSpeakerToBuffer (set71sdds, labelsOf (v), got);

        // `got` must be COMPLETELY untouched — a partially-written map is worse than none, because
        // the caller's "retain the last valid map" contract would inherit half a rejected one.
        const bool ok = ! built && got == sentinel;

        check ("E cross-container-missing-label", ok,
               juce::String (built ? "built when it must not have" : "rejected")
                   + ", out " + mapToString (got));
    }

    //==========================================================================
    // F — Duplicate label. Two speakers claiming one output repeats a target index, which fails the
    // permutation check. THIS IS THE FUNC-03 DETECTOR, not a defensive afterthought.
    {
        auto dup = labelsOf (oo::VenueModel {});
        dup[1] = dup[0];                                   // speaker 2 now also claims `left`

        const std::array<int, 8> lastValid { 0, 1, 2, 3, 4, 5, 6, 7 };
        std::array<int, 8> got = lastValid;

        const bool built = ochan::buildSpeakerToBuffer (set71, dup, got);
        const bool ok    = ! built && got == lastValid;

        check ("F duplicate-label-rejected", ok,
               juce::String (built ? "built when it must not have" : "rejected")
                   + ", last valid map retained: " + mapToString (got));
    }

    //==========================================================================
    // G — A corrupt NUMERIC label.
    //
    // getChannelTypeFromAbbreviation() has an unvalidated numeric branch
    // (juce_AudioChannelSet.cpp:283-285): "7" becomes discreteChannel0 + 6, a PLAUSIBLE-LOOKING
    // discrete channel type rather than `unknown`. It fails safe — but via the permutation check,
    // not via a parse error, and that distinction is exactly what this probe pins down.
    {
        oo::VenueModel v;
        v.setSpeakerLabel (2, "7");

        const CT resolved = v.labelType (2);

        const bool plausible = resolved >= juce::AudioChannelSet::discreteChannel0;
        const bool notUnknown = resolved != juce::AudioChannelSet::unknown;

        std::array<int, 8> got { 0, 1, 2, 3, 4, 5, 6, 7 };
        const bool built = ochan::buildSpeakerToBuffer (set71, labelsOf (v), got);

        const bool ok = plausible && notUnknown && ! built;

        check ("G numeric-label-plausible-but-rejected", ok,
               juce::String ("\"7\" -> type ") + juce::String (static_cast<int> (resolved))
                   + (plausible ? " (a discrete channel, NOT unknown)" : " (expected a discrete channel)")
                   + (built ? " — BUILT, must not have" : " — map rejected"));
    }

    //==========================================================================
    // H — The hull of the §OQ4 default venue.
    //
    // Both side walls are dead straight, so speakers 3 and 8 sit exactly between their wall
    // neighbours and are popped by the `<= EPS_CROSS` comparison. Expected hull: speakers
    // 1, 2, 4, 5, 6, 7 (1-based), m = 6.
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());

        std::vector<int> vertices;

        for (int i = 0; i < h.getNumHullPoints(); ++i)
            vertices.push_back (h.getHullPointSource (i) + 1);   // report 1-based

        std::sort (vertices.begin(), vertices.end());

        const std::vector<int> expected { 1, 2, 4, 5, 6, 7 };

        const bool spk3OnEdge = h.classify (2) == oo::ConvexHull2D::Classification::ON_EDGE;
        const bool spk8OnEdge = h.classify (7) == oo::ConvexHull2D::Classification::ON_EDGE;

        const bool ok = h.getNumHullPoints() == 6 && vertices == expected && spk3OnEdge && spk8OnEdge;

        juce::String got = "m=" + juce::String (h.getNumHullPoints()) + " vertices {";

        for (size_t i = 0; i < vertices.size(); ++i)
            got << (i > 0 ? "," : "") << vertices[i];

        got << "}, spk3 " << (spk3OnEdge ? "ON_EDGE" : "NOT on-edge")
            << ", spk8 " << (spk8OnEdge ? "ON_EDGE" : "NOT on-edge");

        check ("H hull-default-venue", ok, got);
    }

    //==========================================================================
    // I — The EPS_CROSS path.
    //
    // PROBE H DOES NOT TEST WHAT IT LOOKS LIKE IT TESTS. The §OQ4 walls are dead straight, so
    // speakers 3 and 8 have EXACTLY zero cross product and are popped for any non-negative
    // epsilon — H would pass with EPS_CROSS = 0. A NEAR-collinear case is the only thing that
    // exercises the tolerance, in both directions.
    //
    // Geometry: a 10 x 10 square plus a mid-bottom point displaced outward by d.
    // EPS_CROSS = 1e-6 · spanX · spanY ≈ 1e-4, and the chain's cross product for that point is
    // ≈ 10·d, so the pop threshold sits at d ≈ 1e-5 m.
    {
        const auto squareWithBump = [] (float d)
        {
            std::array<oo::Vec3, 8> s {};
            s[0] = {  0.0f,  0.0f, 0.0f };
            s[1] = {  5.0f,    -d, 0.0f };      // the point under test
            s[2] = { 10.0f,  0.0f, 0.0f };
            s[3] = { 10.0f,  5.0f, 0.0f };
            s[4] = { 10.0f, 10.0f, 0.0f };
            s[5] = {  5.0f, 10.0f, 0.0f };
            s[6] = {  0.0f, 10.0f, 0.0f };
            s[7] = {  0.0f,  5.0f, 0.0f };
            return s;
        };

        oo::ConvexHull2D belowEps, aboveEps;
        belowEps.build (squareWithBump (1.0e-6f));   // 1 micrometre — must be treated as collinear
        aboveEps.build (squareWithBump (1.0e-3f));   // 1 millimetre — must survive as a vertex

        const auto isVertex = [] (const oo::ConvexHull2D& h, int idx)
        {
            for (int i = 0; i < h.getNumHullPoints(); ++i)
                if (h.getHullPointSource (i) == idx)
                    return true;

            return false;
        };

        const bool poppedWhenTiny = ! isVertex (belowEps, 1);
        const bool keptWhenLarger =   isVertex (aboveEps, 1);

        const bool ok = poppedWhenTiny && keptWhenLarger;

        check ("I near-collinear-epsilon", ok,
               juce::String ("eps=") + juce::String (belowEps.getCrossEpsilon(), 8)
                   + ", d=1e-6 " + (poppedWhenTiny ? "popped" : "KEPT (should pop)")
                   + ", d=1e-3 " + (keptWhenLarger ? "kept" : "POPPED (should keep)"));
    }

    //==========================================================================
    // J — A physical rear room corner is outside the speaker hull.
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());

        const oo::Vec2 rearCorner { 13.0f, 22.0f };            // the hall envelope's rear corner
        const bool inside = h.isInside (rearCorner);
        const auto proj   = h.project (rearCorner);

        const bool ok = ! inside && proj.distance > 0.0f && std::isfinite (proj.distance);

        check ("J rear-corner-outside", ok,
               juce::String (inside ? "INSIDE (should be outside)" : "outside")
                   + ", d_hull=" + juce::String (proj.distance, 4) + " m");
    }

    //==========================================================================
    // K — Projection vs an independent oracle over 200 pinned-seed points.
    //
    // The fixture is GENERATED IN-TEST from a fixed seed and the oracle lives in this same TU, so
    // there is no committed file to drift against. Half the points fall outside the hull by
    // construction (the sampling box is the speaker bbox expanded x2 in each axis).
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());

        juce::Random rng (0x0C7A9042);

        const float cx = 0.5f * (v.bbMinX() + v.bbMaxX());
        const float cy = 0.5f * (v.bbMinY() + v.bbMaxY());
        const float hx = (v.bbMaxX() - v.bbMinX());        // half-width x2 == full width
        const float hy = (v.bbMaxY() - v.bbMinY());

        double worst = 0.0;
        int    outsideCount = 0;
        bool   allFiniteProj = true;

        for (int i = 0; i < 200; ++i)
        {
            const oo::Vec2 p { cx + hx * (2.0f * static_cast<float> (rng.nextDouble()) - 1.0f),
                               cy + hy * (2.0f * static_cast<float> (rng.nextDouble()) - 1.0f) };

            const auto proj = h.project (p);

            if (! (std::isfinite (proj.distance) && std::isfinite (proj.point.x)
                   && std::isfinite (proj.point.y)))
                allFiniteProj = false;

            if (! h.isInside (p))
                ++outsideCount;

            const double oracle = oracleDistanceToHull (h.getHullPoints(), h.getNumHullPoints(), p);

            worst = std::max (worst, std::abs (static_cast<double> (proj.distance) - oracle));
        }

        // DSP-03 criterion 3 names 1e-6 m, and the MEASURED worst case (printed below, ~8.7e-7 m
        // on this toolchain) meets it. The asserted bound is deliberately looser at 1e-4 m: the
        // implementation is single-precision and the sampling box reaches ~27 m, where one float
        // ULP is already ~2e-6 m — so 1e-6 sits close enough to the arithmetic's own resolution
        // that a different optimiser could cross it without anything being wrong. The printed
        // number is the real signal; a genuine regression moves it by orders of magnitude.
        const bool ok = worst <= 1.0e-4 && allFiniteProj && outsideCount > 40;

        check ("K projection-vs-oracle-200pt", ok,
               juce::String ("max |impl - oracle| = ") + juce::String (worst, 9)
                   + " m over 200 pts, " + juce::String (outsideCount) + " outside"
                   + (allFiniteProj ? "" : ", NON-FINITE projection"));
    }

    //==========================================================================
    // L — The §3.1.6 degeneracy matrix. Every branch finite, no NaN, no crash, no divide by zero.
    {
        bool ok = true;
        juce::String detail;

        // 1. All 8 collinear (one wall) -> chain degenerates to a 2-point segment.
        {
            oo::VenueModel v;

            for (int i = 0; i < 8; ++i)
                v.setSpeakerPosition (i, { 0.0f, static_cast<float> (i) * 2.0f, 4.0f });

            oo::ConvexHull2D h;
            h.build (v.speakerPositions());

            const auto proj = h.project ({ 5.0f, 7.0f });
            const bool good = h.getNumHullPoints() == 2 && std::isfinite (proj.distance)
                           && allFinite (v);

            ok = ok && good;
            detail << "collinear m=" << h.getNumHullPoints() << (good ? " ok; " : " BAD; ");
        }

        // 2. All 8 coincident -> a single point.
        {
            oo::VenueModel v;

            for (int i = 0; i < 8; ++i)
                v.setSpeakerPosition (i, { 3.0f, 4.0f, 5.0f });

            oo::ConvexHull2D h;
            h.build (v.speakerPositions());

            const auto proj = h.project ({ 9.0f, 4.0f });
            const bool good = h.getNumHullPoints() == 1 && std::isfinite (proj.distance)
                           && near (proj.distance, 6.0f, 1.0e-4f) && allFinite (v);

            ok = ok && good;
            detail << "coincident m=" << h.getNumHullPoints() << (good ? " ok; " : " BAD; ");
        }

        // 3. Zero rake span — every speaker at one depth. earHeight() must collapse to rakeFront
        //    rather than divide by zero.
        {
            oo::VenueModel v;

            for (int i = 0; i < 8; ++i)
                v.setSpeakerPosition (i, { static_cast<float> (i), 7.0f, 4.0f });

            const float e0 = v.earHeight (0.0f);
            const float e1 = v.earHeight (100.0f);

            const bool good = std::isfinite (e0) && std::isfinite (e1)
                           && near (e0, v.rakeFront(), 1.0e-6f)
                           && near (e1, v.rakeFront(), 1.0e-6f);

            ok = ok && good;
            detail << "zero-rake-span " << (good ? "-> rakeFront ok; " : "BAD; ");
        }

        // 4. Degenerate bbox — normToMetres must not divide by zero on EITHER axis independently.
        {
            oo::VenueModel v;

            for (int i = 0; i < 8; ++i)
                v.setSpeakerPosition (i, { 6.0f, 6.0f, 4.0f });

            const auto m = v.normToMetres (0.5f, 0.5f);
            const bool good = std::isfinite (m.x) && std::isfinite (m.y);

            ok = ok && good;
            detail << "degenerate-bbox " << (good ? "finite ok" : "NON-FINITE");
        }

        check ("L degeneracy-matrix", ok, detail);
    }

    //==========================================================================
    // M — The audience plane (DSP-04 criteria 1 and 2, the parts that CAN close at 2.1).
    //
    // Criterion 3 ("changing rakeRear alone changes the GAIN VECTOR") needs DbapSolver and lands at
    // Phase 2.2. What closes here is the geometric half: the height itself moves.
    {
        oo::VenueModel v;

        const float yFront = v.bbMinY();
        const float yRear  = v.bbMaxY();
        const float yMid   = 0.5f * (yFront + yRear);

        const bool atFront = near (v.earHeight (yFront), v.rakeFront(), 1.0e-5f);
        const bool atRear  = near (v.earHeight (yRear),  v.rakeRear(),  1.0e-5f);
        const bool atMid   = near (v.earHeight (yMid),
                                   0.5f * (v.rakeFront() + v.rakeRear()), 1.0e-5f);

        // Changing rakeRear ALONE must move a rear source's absolute height and leave the front
        // untouched — the two ends of the plane are independently controlled.
        const float rearBefore  = v.absoluteHeight (yRear,  0.0f);
        const float frontBefore = v.absoluteHeight (yFront, 0.0f);

        v.setRake (v.rakeFront(), 6.0f);

        const float rearAfter  = v.absoluteHeight (yRear,  0.0f);
        const float frontAfter = v.absoluteHeight (yFront, 0.0f);

        const bool rearMoved   = std::abs (rearAfter - rearBefore) > 1.0f;
        const bool frontStayed = near (frontAfter, frontBefore, 1.0e-5f);

        const bool ok = atFront && atRear && atMid && rearMoved && frontStayed;

        check ("M ear-height-linearity", ok,
               juce::String ("front ") + juce::String (v.earHeight (yFront), 3)
                   + " mid " + juce::String (v.earHeight (yMid), 3)
                   + " rear " + juce::String (rearAfter, 3)
                   + (rearMoved ? ", rakeRear moves the rear" : ", REAR DID NOT MOVE")
                   + (frontStayed ? ", front unchanged" : ", FRONT ALSO MOVED"));
    }

    //==========================================================================
    // N — srcZ = 0 rides the rake: at the rear of a steep rake the source is never BELOW rear-row
    // ear height. A dropped earHeight() term would put it at 0 m and the check would catch it.
    {
        oo::VenueModel v;
        v.setRake (1.10f, 8.0f);                    // deliberately steeper than any real hall

        bool ok = true;
        float worstDeficit = 0.0f;

        for (int i = 0; i <= 20; ++i)
        {
            const float t = static_cast<float> (i) / 20.0f;
            const float y = v.bbMinY() + t * (v.bbMaxY() - v.bbMinY());
            const float z = v.absoluteHeight (y, 0.0f);

            if (! std::isfinite (z) || z < v.rakeFront() - 1.0e-5f)
                ok = false;

            worstDeficit = std::max (worstDeficit, v.rakeFront() - z);
        }

        const float atRear = v.absoluteHeight (v.bbMaxY(), 0.0f);
        ok = ok && atRear >= v.rakeRear() - 1.0e-5f;

        check ("N srcZ0-rides-the-rake", ok,
               juce::String ("rear height ") + juce::String (atRear, 4) + " m >= rakeRear "
                   + juce::String (v.rakeRear(), 4) + " m, worst deficit "
                   + juce::String (worstDeficit, 6));
    }

    //==========================================================================
    // O — rigScale.
    //
    // THE SCALING INVARIANT IS THE REAL ASSERTION. A bare "≈ 7.95" constant is a mirrored fixture:
    // it would be re-blessed to whatever the code produced the day it broke. Doubling every
    // coordinate must double the RMS radius, and that property is independent of the venue.
    //
    // The value is checked too, but against a WIDE band — see the SUMMARY for the arithmetic:
    // ARCHITECTURE §OQ4's stated 7.95 m is a hand-calculation; the §OQ4 coordinates it lists
    // actually give 7.932 m.
    {
        oo::VenueModel v;
        const float base = v.rigScale();

        oo::VenueModel doubled;

        for (int i = 0; i < 8; ++i)
        {
            const auto p = v.speaker (i);
            doubled.setSpeakerPosition (i, { p.x * 2.0f, p.y * 2.0f, p.z * 2.0f });
        }

        const float scaled = doubled.rigScale();

        const bool invariant = near (scaled, 2.0f * base, 1.0e-3f);
        const bool inBand    = base > 7.0f && base < 9.0f;

        check ("O rig-scale-and-invariant", invariant && inBand,
               juce::String ("rigScale ") + juce::String (base, 4) + " m; doubled -> "
                   + juce::String (scaled, 4) + " m ("
                   + (invariant ? "invariant holds" : "INVARIANT VIOLATED") + ")");
    }

    //==========================================================================
    // P — Missing and PARTIAL VENUE nodes.
    //
    // Every session saved during Stage 1 carries no VENUE child at all, so the missing case is the
    // common one, not an edge case. The partial case is what "defaults PER ATTRIBUTE" means: a node
    // carrying only @rakeRear must keep that value and default everything else — not be discarded
    // wholesale, and never read as zeros.
    {
        const oo::VenueModel reference;                     // §OQ4 defaults

        bool ok = true;
        juce::String detail;

        // Missing: a Stage-1-shaped tree with 17 parameters and nothing else.
        {
            juce::ValueTree stage1 { "OOctagon" };
            stage1.appendChild (juce::ValueTree { "PARAM" }, nullptr);

            oo::VenueModel v;
            v.readFromState (stage1);

            bool same = near (v.rakeFront(), reference.rakeFront(), 1.0e-6f)
                     && near (v.rakeRear(),  reference.rakeRear(),  1.0e-6f);

            for (int i = 0; i < 8; ++i)
            {
                const auto a = v.speaker (i);
                const auto b = reference.speaker (i);

                same = same && bitExact (a.x, b.x) && bitExact (a.y, b.y) && bitExact (a.z, b.z)
                            && v.labelAbbreviation (i) == reference.labelAbbreviation (i);
            }

            ok = ok && same;
            detail << "missing-node " << (same ? "-> §OQ4 defaults ok; " : "BAD; ");
        }

        // Partial: a VENUE node carrying ONE attribute and ONE incomplete speaker.
        {
            juce::ValueTree root { "OOctagon" };
            juce::ValueTree venue { oo::VenueModel::venueTag };
            venue.setProperty (oo::VenueModel::propRakeRear, 5.5, nullptr);

            juce::ValueTree spk { oo::VenueModel::speakerTag };
            spk.setProperty (oo::VenueModel::propIndex, 3, nullptr);
            spk.setProperty (oo::VenueModel::propX, 7.25, nullptr);   // y, z, trim, label absent

            venue.appendChild (spk, nullptr);
            root.appendChild (venue, nullptr);

            oo::VenueModel v;
            v.readFromState (root);

            const auto s3  = v.speaker (2);                  // @index 3 is 1-based -> array slot 2
            const auto ref3 = reference.speaker (2);

            const bool keptRake   = near (v.rakeRear(), 5.5f, 1.0e-6f);
            const bool keptFront  = near (v.rakeFront(), reference.rakeFront(), 1.0e-6f);
            const bool keptX      = near (s3.x, 7.25f, 1.0e-6f);
            const bool defaultedY = bitExact (s3.y, ref3.y);
            const bool defaultedZ = bitExact (s3.z, ref3.z);
            const bool keptLabel  = v.labelAbbreviation (2) == reference.labelAbbreviation (2);

            // A speaker with NO node at all must also default.
            const auto s5 = v.speaker (4);
            const auto ref5 = reference.speaker (4);
            const bool untouchedSpeaker = bitExact (s5.x, ref5.x) && bitExact (s5.y, ref5.y);

            const bool good = keptRake && keptFront && keptX && defaultedY && defaultedZ
                           && keptLabel && untouchedSpeaker;

            ok = ok && good;
            detail << "partial-node " << (good ? "-> per-attribute defaults ok" : "BAD");
        }

        check ("P venue-missing-and-partial", ok, detail);
    }

    //==========================================================================
    std::printf ("\n----------------------------------------------------------\n");
    std::printf ("  %d probe(s), %d failure(s)\n\n", probes, failures);

    return failures == 0 ? 0 : 1;
}
