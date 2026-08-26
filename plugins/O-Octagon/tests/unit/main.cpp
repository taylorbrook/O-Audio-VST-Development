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

    O-Octagon geometry unit target — Phase 2.1 (Geometry Core), 2.2 (DBAP Solve),
    2.3 (Source Shaping and Outside-Hull Processing).

    A-P are Phase 2.1's sixteen probes over the three components that decide where sound goes before
    any gain exists to confuse a diagnosis. V-AH are Phase 2.2's, over the solver itself. AU-AX are
    Phase 2.3's, over the two outside-hull arithmetic laws and the now-live sub-point geometry.

    ── Phase 4.1 (CO) ───────────────────────────────────────────────────────────────────────────

      CO COMPAT-04/3 — oo::rig::isRealRig as a FORM: the three real rigs true, and 7.1.4 and
         OCTAGONAL false. Those two rows are the whole probe. They are the only sets on which the
         shipped complement spelling and the rejected "== mono || == stereo" spelling disagree,
         and probe BM (render harness) structurally cannot reach them — it arrives through
         prepareToPlay(), which only ever sees sets isBusesLayoutSupported() already admitted.
         BM proves the wiring; CO proves the form; NC1 proves neither alone is enough.

    ── Phase 2.3 (AU-AX) ────────────────────────────────────────────────────────────────────────

      AU DSP-07/4 — the §3.5.2 cutoff table, AND H4's Nyquist-safe ceiling at four sample rates
      AV DSP-07/1,2,3 — the dB/m trim law, its −24 dB floor, and hullAtten = 0 as BIT-EXACT unity
      AW DSP-08 — room-scale invariance at blur = 0.25 with a POSITIVE CONTROL at the 8 m clamp
      AX DSP-06/2,3,5 — handedness, per-sub-point z on the raked plane, the |b| → 0 guard

    ── Phase 2.2 (V-AH) ─────────────────────────────────────────────────────────────────────────

      V  VenueModel::earHeight()    == oo::plane::earHeight()    on a NON-DEFAULT venue
      W  VenueModel::normToMetres() == oo::plane::normToMetres(), incl. its own zero-span guard
      X  a = R/(20·log10 2) at R = 3, 4, 6 vs hand-computed
      Y  Gains vs the COMMITTED Python fixture, every case — the DSP-01 independence gate
      Z  Changing ONLY srcZ changes the gain vector — the (z_i − z_s)² term
      AA Σ v_i² = 1 inside / outside / at vertices / on speakers, over rolloff × blur
      AB w_i = 0 → exactly 0.0f; a 2-speaker subset still Σ v² = 1
      AC All-zero weights → all 8 exactly 0.0f. Not NaN, not full scale
      AD Finite on speakers at blur = 0, at both rolloff ends, on the §3.1.6 degenerate venues
      AE powCalls == 16 per solve pair — the exact figure, not just the ≤ 32 bound
      AF MIRROR SYMMETRY of the §OQ4 rig — a correctness property a distinctness check cannot give
      AG r_s mapping, the §3.3.2 table, and the SCALING INVARIANT
      AH SourceShaper driven directly at width > 0 — written at 2.2 as coverage for code that
         shipped inert; the DSP-06 claim itself is AX, at 2.3

    ── Phase 2.1 (A-P) ──────────────────────────────────────────────────────────────────────────

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
#include <utility>
#include <vector>

#include "Data/RigPolicy.h"
#include "Data/SceneModel.h"
#include "Data/VenueFile.h"
#include "Data/VenueGeometry.h"
#include "Data/VenueModel.h"
#include "Data/VenueSnapshot.h"
#include "DSP/ChannelMap.h"
#include "DSP/ConvexHull2D.h"
#include "DSP/DbapSolver.h"
#include "DSP/FieldSampler.h"
#include "DSP/HullProcessor.h"
#include "DSP/SourceShaper.h"

#include "JuceChannelOrderGolden.h"
#include "DbapReferenceFixture.h"

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

//==============================================================================
// ── Phase 2.2 helpers ─────────────────────────────────────────────────────────────────────────

/** Builds the POD the audio thread actually sees, exactly as OOctagonProcessor::publishSnapshot()
    does. Probes that call SourceShaper need one and there is no processor in this target. */
oo::VenueSnapshot snapshotOf (const oo::VenueModel& v)
{
    oo::ConvexHull2D h;
    h.build (v.speakerPositions());

    oo::VenueSnapshot s;
    s.spk = v.speakerPositions();

    for (int i = 0; i < 8; ++i)
    {
        s.trimLin[(size_t) i] = v.trimLin (i);
        s.hullPts[(size_t) i] = h.getHullPoint (i);
    }

    s.hullCount    = h.getNumHullPoints();
    s.hullEpsCross = h.getCrossEpsilon();
    s.centroid     = v.centroid();
    s.rigScale     = v.rigScale();
    s.bbMinX       = v.bbMinX();
    s.bbMaxX       = v.bbMaxX();
    s.bbMinY       = v.bbMinY();
    s.bbMaxY       = v.bbMaxY();
    s.rakeFront    = v.rakeFront();
    s.rakeRear     = v.rakeRear();

    return s;
}

/** solve() against the §OQ4 default rig, with unit weights unless overridden. */
struct SolveRig
{
    std::array<oo::Vec3, 8> spk {};
    float                   rigScale { 0.0f };

    explicit SolveRig (const oo::VenueModel& v)
        : spk (v.speakerPositions()), rigScale (v.rigScale()) {}

    std::array<float, 8> solve (oo::Vec3 src, float rolloff, float blur,
                                const std::array<float, 8>& w) const
    {
        std::array<float, 8> v {};

        oo::dbap::solve (spk.data(), w.data(), src,
                         oo::dbap::rolloffToAlpha (rolloff),
                         oo::dbap::blurToRadius (blur, rigScale),
                         v.data());

        return v;
    }
};

const std::array<float, 8> kUnitWeights { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

double sumOfSquares (const std::array<float, 8>& v)
{
    double s = 0.0;

    for (float x : v)
        s += static_cast<double> (x) * static_cast<double> (x);

    return s;
}

bool allFiniteGains (const std::array<float, 8>& v)
{
    for (float x : v)
        if (! std::isfinite (x))
            return false;

    return true;
}

/** Exactly 0.0f, checked through the object representation so -Wfloat-equal stays satisfied.
    Signed zero matters here: DSP-05 says "exactly zero", and k·0·t is +0.0f. */
bool isExactlyZero (float x) noexcept
{
    return bitExact (x, 0.0f);
}

} // namespace

//==============================================================================
int main()
{
    std::printf ("\nO-Octagon geometry unit target — Phases 2.1 (Geometry Core) + 2.2 (DBAP Solve)\n");
    std::printf ("==============================================================================\n\n");

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
    // P2 — PRESENT BUT NOT A NUMBER (CODE_REVIEW WR-03).
    //
    // P above covers ABSENT attributes. This covers the other half, which until v1.3.2 had no
    // guard at all: readFloat() tested presence only, then ran an unchecked var -> double -> float.
    //
    // Two things got through, and this is the NORMAL conversion path rather than an exotic one —
    // getStateInformation() serialises via createXml(), so on restore EVERY property in the tree is
    // a STRING var and juce::String::getDoubleValue() is always what runs:
    //
    //   garbage text  ->  0.0, because getDoubleValue() returns 0.0 with no leading number. So
    //                     rakeFront="tall" loaded a FLAT audience plane instead of the §OQ4 1.10,
    //                     which is precisely the "never zeros" the header contract promises.
    //   "nan" / "inf" ->  a REAL NaN or infinity, because JUCE's parser recognises those literal
    //                     words (juce_CharacterFunctions.h).
    //
    // The audio thread was never at risk — publishSnapshot() sanitises everything it copies — but
    // the message-thread model feeds the UI and toValueTree() writes the value straight back out,
    // and juce::String(double) renders a NaN as "nan", which readFloat() then re-read as a NaN.
    // The corruption loop closed across saves, which is why the last clause below is a ROUND TRIP
    // rather than a single read.
    //
    // The negative half matters as much as the positive: the guard must not start rejecting the
    // numeric spellings a real .venue file carries. Whitespace and exponent notation are both
    // written by juce::String(double) or by a hand-editing operator.
    {
        const oo::VenueModel reference;                     // §OQ4 defaults

        bool ok = true;
        juce::String detail;

        auto treeWith = [] (const juce::var& rakeFrontValue,
                            const juce::var& xValue,
                            const juce::var& yValue,
                            const juce::var& trimValue)
        {
            juce::ValueTree root { "OOctagon" };
            juce::ValueTree venue { oo::VenueModel::venueTag };
            venue.setProperty (oo::VenueModel::propRakeFront, rakeFrontValue, nullptr);

            juce::ValueTree spk { oo::VenueModel::speakerTag };
            spk.setProperty (oo::VenueModel::propIndex,  1,          nullptr);
            spk.setProperty (oo::VenueModel::propX,      xValue,     nullptr);
            spk.setProperty (oo::VenueModel::propY,      yValue,     nullptr);
            spk.setProperty (oo::VenueModel::propTrimDb, trimValue,  nullptr);

            venue.appendChild (spk, nullptr);
            root.appendChild (venue, nullptr);
            return root;
        };

        // 1. Garbage, "nan", "inf" and trailing garbage ALL fall back per attribute.
        {
            oo::VenueModel v;
            v.readFromState (treeWith ("tall", "nan", "-inf", "-3.5dB"));

            const auto s1  = v.speaker (0);
            const auto ref = reference.speaker (0);

            const bool rake  = near (v.rakeFront(), reference.rakeFront(), 1.0e-6f);
            const bool notZero = ! near (v.rakeFront(), 0.0f, 1.0e-6f);
            const bool x     = bitExact (s1.x, ref.x);
            const bool y     = bitExact (s1.y, ref.y);
            const bool trim  = near (v.trimDb (0), reference.trimDb (0), 1.0e-6f);

            const bool good = rake && notZero && x && y && trim && allFinite (v);
            ok = ok && good;
            detail << "garbage/nan/inf " << (good ? "-> §OQ4 defaults, all finite; " : "BAD; ");
        }

        // 2. [negative control] The spellings a REAL file carries must still be READ. If this
        //    fails the guard has stopped being a validator and started being a data loss.
        {
            oo::VenueModel v;
            v.readFromState (treeWith ("  1.75  ", "1.5e1", "-2.5E-1", "+3"));

            const auto s1 = v.speaker (0);

            const bool rake = near (v.rakeFront(),  1.75f,  1.0e-6f);
            const bool x    = near (s1.x,          15.0f,   1.0e-6f);
            const bool y    = near (s1.y,          -0.25f,  1.0e-6f);
            const bool trim = near (v.trimDb (0), 3.0f, 1.0e-6f);

            const bool good = rake && x && y && trim;
            ok = ok && good;
            detail << "[neg-control] whitespace/exponent/sign READ "
                   << (good ? "ok; " : "BAD — the guard is eating valid values; ");
        }

        // 3. THE ROUND TRIP. A model that took a NaN once would write "nan" back out and read it
        //    in again forever. Read the poisoned tree, write the model out, read it back: the
        //    second read must land on the same finite defaults as the first.
        {
            oo::VenueModel first;
            first.readFromState (treeWith ("tall", "nan", "inf", "nan"));

            juce::ValueTree round { "OOctagon" };
            first.writeToState (round);

            oo::VenueModel second;
            second.readFromState (round);

            bool same = near (second.rakeFront(), first.rakeFront(), 1.0e-6f)
                     && allFinite (second);

            for (int i = 0; i < 8; ++i)
            {
                const auto a = second.speaker (i);
                const auto b = first.speaker (i);
                same = same && bitExact (a.x, b.x) && bitExact (a.y, b.y) && bitExact (a.z, b.z);
            }

            ok = ok && same;
            detail << "save/reload round trip " << (same ? "stable" : "BAD — corruption is sticky");
        }

        check ("P2 venue-nonnumeric-attrs", ok, detail);
    }

    //==========================================================================
    //
    //  ═══ PHASE 2.2 — DBAP SOLVE AND GAIN APPLICATION ═══════════════════════════════════════
    //
    //==========================================================================

    std::printf ("\n  ── Phase 2.2 (DBAP Solve) ───────────────────────────────────────────\n");

    //==========================================================================
    // V — VenueModel::earHeight() IS oo::plane::earHeight().
    //
    // Driven on a NON-DEFAULT venue: the default rake happens to be a plain 1.10 -> 3.20 ramp, and a
    // delegate that ignored its arguments and returned a constant would need a venue where the two
    // ends genuinely differ to be caught. The zero-span venue is swept too — that guard is the whole
    // reason there are two of them.
    {
        oo::VenueModel v;
        v.setRake (0.375f, 5.875f);

        for (int i = 0; i < 8; ++i)
            v.setSpeakerPosition (i, { 1.5f + static_cast<float> (i) * 1.375f,
                                       2.25f + static_cast<float> (i) * 2.125f,
                                       3.875f });

        double worst = 0.0;

        for (int i = -40; i <= 80; ++i)
        {
            const float y = 0.5f * static_cast<float> (i);

            const float member = v.earHeight (y);
            const float free   = oo::plane::earHeight (v.rakeFront(), v.rakeRear(),
                                                       v.bbMinY(), v.bbMaxY(), y);

            if (! bitExact (member, free))
                worst = std::max (worst, std::abs (static_cast<double> (member - free)));
        }

        // Zero y-span: every speaker at one depth. Both must collapse to rakeFront.
        oo::VenueModel flat;
        flat.setRake (2.125f, 6.5f);

        for (int i = 0; i < 8; ++i)
            flat.setSpeakerPosition (i, { static_cast<float> (i), 7.0f, 4.0f });

        bool guardAgrees = true;

        for (int i = 0; i <= 20; ++i)
        {
            const float y = -50.0f + 10.0f * static_cast<float> (i);

            guardAgrees = guardAgrees
                       && bitExact (flat.earHeight (y),
                                    oo::plane::earHeight (flat.rakeFront(), flat.rakeRear(),
                                                          flat.bbMinY(), flat.bbMaxY(), y))
                       && bitExact (flat.earHeight (y), flat.rakeFront());
        }

        const bool ok = worst <= 0.0 && guardAgrees;

        check ("V earheight-member-is-free-fn", ok,
               juce::String ("121 swept y on a non-default rake: ")
                   + (worst <= 0.0 ? "bit-identical" : "DIVERGED")
                   + ", zero-span guard "
                   + (guardAgrees ? "agrees and collapses to rakeFront" : "DISAGREES"));
    }

    //==========================================================================
    // W — VenueModel::normToMetres() IS oo::plane::normToMetres().
    //
    // The degenerate case is the interesting one and Q2 originally missed it: a rig with all eight
    // speakers at ONE X has a perfectly good rake and a zero-WIDTH bbox. Collapsing the two
    // zero-span guards into one would leave this path dividing by zero, so the probe drives exactly
    // that venue — zero x-span, live y-span, live rake — and asserts the axes guard independently.
    {
        oo::VenueModel v;
        v.setRake (0.625f, 4.125f);

        for (int i = 0; i < 8; ++i)
            v.setSpeakerPosition (i, { 2.5f + static_cast<float> (i) * 1.75f,
                                       3.5f + static_cast<float> (i) * 1.25f, 4.0f });

        bool identical = true;

        for (int ix = -2; ix <= 12; ++ix)
        {
            for (int iy = -2; iy <= 12; ++iy)
            {
                const float nx = 0.1f * static_cast<float> (ix);
                const float ny = 0.1f * static_cast<float> (iy);

                const auto member = v.normToMetres (nx, ny);
                const auto free   = oo::plane::normToMetres (v.bbMinX(), v.bbMaxX(),
                                                             v.bbMinY(), v.bbMaxY(), nx, ny);

                identical = identical && bitExact (member.x, free.x) && bitExact (member.y, free.y);
            }
        }

        // Zero x-span, LIVE y-span. x must pin to bbMinX while y still denormalises normally.
        oo::VenueModel narrow;
        narrow.setRake (1.0f, 4.0f);

        for (int i = 0; i < 8; ++i)
            narrow.setSpeakerPosition (i, { 6.0f, 2.0f + static_cast<float> (i) * 1.5f, 4.0f });

        const auto mid = narrow.normToMetres (0.75f, 0.25f);
        const auto midFree = oo::plane::normToMetres (narrow.bbMinX(), narrow.bbMaxX(),
                                                      narrow.bbMinY(), narrow.bbMaxY(), 0.75f, 0.25f);

        const float expectedY = narrow.bbMinY() + 0.25f * (narrow.bbMaxY() - narrow.bbMinY());

        const bool xPinned   = bitExact (mid.x, narrow.bbMinX()) && std::isfinite (mid.x);
        const bool yLive     = near (mid.y, expectedY, 1.0e-5f);
        const bool sameAsFree = bitExact (mid.x, midFree.x) && bitExact (mid.y, midFree.y);

        const bool ok = identical && xPinned && yLive && sameAsFree;

        check ("W normtometres-member-is-free-fn", ok,
               juce::String ("225 swept (nx,ny) ") + (identical ? "bit-identical" : "DIVERGED")
                   + "; zero-x-span venue: x "
                   + (xPinned ? "pinned to bbMinX" : "NOT PINNED (divide by zero?)")
                   + ", y "
                   + (yLive ? "still denormalises" : "ALSO COLLAPSED — the guards are not independent"));
    }

    //==========================================================================
    // X — the rolloff exponent. DSP-01 criterion 1.
    {
        struct Case { float rolloff; double expected; };

        // Hand-computed: 20·log10(2) = 6.020599913279624.
        const std::array<Case, 3> cases
            { { { 3.0f, 3.0 / 6.020599913279624 },
                { 4.0f, 4.0 / 6.020599913279624 },
                { 6.0f, 6.0 / 6.020599913279624 } } };

        bool ok = true;
        juce::String detail;

        for (const auto& c : cases)
        {
            const double got = static_cast<double> (oo::dbap::rolloffToAlpha (c.rolloff));
            const double err = std::abs (got - c.expected);

            ok = ok && err <= 1.0e-6;

            detail << "R=" << juce::String (c.rolloff, 0) << " a="
                   << juce::String (got, 7) << " (err " << juce::String (err, 10) << ")  ";
        }

        check ("X rolloff-to-alpha", ok, detail);
    }

    //==========================================================================
    // Y — THE DSP-01 INDEPENDENCE GATE.
    //
    // Every case of the committed Python fixture. The oracle does not transcribe the C++: it forms
    // d^(-a) as exp(-a·log d) rather than pow(), and normalises by explicitly building Σv² rather
    // than via the k = 1/sqrt(denom) shortcut. An oracle that re-ran the implementation's own
    // expression would reproduce its errors and pass forever (the SUMMARY-2.1 F2 argument).
    //
    // THE FIXTURE IS SELF-CONTAINED: its own speaker array and rigScale are used, so nothing here
    // touches VenueModel's defaults and no mirrored coordinate table exists to drift.
    //
    // The HARD GATE IS 1e-5 and the measured worst deviation is PRINTED. DSP-01 names 1e-6; the
    // solver is single-precision and Clang defaults to -ffp-contract=on, so a hard gate pinned at
    // 1e-6 would invite a "fix" that is a tolerance edit. Whether 1e-6 was met is recorded in the
    // SUMMARY from the printed number.
    {
        std::array<oo::Vec3, 8> spk {};

        for (int i = 0; i < 8; ++i)
            spk[(size_t) i] = { static_cast<float> (dbap_fixture::kSpeakers[i][0]),
                                static_cast<float> (dbap_fixture::kSpeakers[i][1]),
                                static_cast<float> (dbap_fixture::kSpeakers[i][2]) };

        const float rigScale = static_cast<float> (dbap_fixture::kRigScale);

        double      worst = 0.0;
        const char* worstCase = "";
        int         nonFinite = 0;

        for (int c = 0; c < dbap_fixture::kNumCases; ++c)
        {
            const auto& fc = dbap_fixture::kCases[c];

            std::array<float, 8> w {};

            for (int i = 0; i < 8; ++i)
                w[(size_t) i] = static_cast<float> (fc.w[i]);

            const oo::Vec3 src { static_cast<float> (fc.src[0]),
                                 static_cast<float> (fc.src[1]),
                                 static_cast<float> (fc.src[2]) };

            std::array<float, 8> v {};

            oo::dbap::solve (spk.data(), w.data(), src,
                             oo::dbap::rolloffToAlpha (static_cast<float> (fc.rolloff)),
                             oo::dbap::blurToRadius (static_cast<float> (fc.blur), rigScale),
                             v.data());

            for (int i = 0; i < 8; ++i)
            {
                if (! std::isfinite (v[(size_t) i]))
                    ++nonFinite;

                const double deviation = std::abs (static_cast<double> (v[(size_t) i]) - fc.v[i]);

                if (deviation > worst)
                {
                    worst     = deviation;
                    worstCase = fc.name;
                }
            }
        }

        const bool ok = worst <= 1.0e-5 && nonFinite == 0 && dbap_fixture::kNumCases > 0;

        check ("Y dbap-vs-python-oracle", ok,
               juce::String (dbap_fixture::kNumCases) + " cases, max |impl - oracle| = "
                   + juce::String (worst, 11) + " (worst: " + worstCase + ")"
                   + (worst <= 1.0e-6 ? "  [DSP-01's 1e-6 MET]" : "  [above 1e-6 — see SUMMARY]")
                   + (nonFinite > 0 ? juce::String (", ") + juce::String (nonFinite)
                                          + " NON-FINITE" : ""));
    }

    //==========================================================================
    // Z — the (z_i − z_s)² term. DSP-01 criterion 2.
    //
    // NON-VACUOUS ONLY BECAUSE §OQ4's default heights are GRADED 4.50 -> 5.40 m. With a uniform z
    // every (z_i − z_s) difference would be identical across speakers, srcZ would scale all eight
    // distances the same way, and normalisation would cancel it exactly — this probe would pass on
    // an implementation that dropped the z term entirely. DO NOT FLATTEN THE DEFAULT HEIGHTS.
    {
        const oo::VenueModel v;
        const SolveRig rig (v);

        const oo::Vec3 base { 4.75f, 8.5f, 1.10f };
        const oo::Vec3 high { 4.75f, 8.5f, 7.10f };

        const auto a = rig.solve (base, 4.0f, 0.10f, kUnitWeights);
        const auto b = rig.solve (high, 4.0f, 0.10f, kUnitWeights);

        float biggest = 0.0f;
        int   moved   = 0;

        for (int i = 0; i < 8; ++i)
        {
            const float d = std::abs (a[(size_t) i] - b[(size_t) i]);

            if (d > 1.0e-6f)
                ++moved;

            biggest = std::max (biggest, d);
        }

        // Guard against the vacuity above: if the rig were flat this would be the assertion that
        // silently stopped meaning anything, so assert the rig is NOT flat too.
        float minZ = v.speaker (0).z, maxZ = v.speaker (0).z;

        for (int i = 1; i < 8; ++i)
        {
            minZ = std::min (minZ, v.speaker (i).z);
            maxZ = std::max (maxZ, v.speaker (i).z);
        }

        const bool graded = (maxZ - minZ) > 0.1f;
        const bool ok     = moved >= 6 && biggest > 1.0e-3f && graded;

        check ("Z srcz-changes-gain-vector", ok,
               juce::String (moved) + "/8 lanes moved, max delta "
                   + juce::String (biggest, 6) + ", rig z-spread "
                   + juce::String (maxZ - minZ, 3) + " m"
                   + (graded ? "" : " — RIG IS FLAT, THIS PROBE IS VACUOUS"));
    }

    //==========================================================================
    // AA — Σ v_i² = 1. DSP-02, all three criteria.
    //
    // Measured AT THE SOLVER OUTPUT, per ROADMAP, over the full rolloff (3-6) × blur (0-1) product,
    // at positions inside the hull, outside it, on hull vertices and on exact speaker coordinates.
    {
        const oo::VenueModel v;
        const SolveRig rig (v);

        oo::ConvexHull2D h;
        h.build (v.speakerPositions());

        std::vector<oo::Vec3> positions;

        // Inside + outside: a grid that deliberately overruns the room in both axes.
        for (int gx = 0; gx <= 12; ++gx)
            for (int gy = 0; gy <= 12; ++gy)
                positions.push_back ({ -3.0f + 19.0f * static_cast<float> (gx) / 12.0f,
                                       -3.0f + 26.0f * static_cast<float> (gy) / 12.0f,
                                       1.10f });

        // Hull vertices.
        for (int i = 0; i < h.getNumHullPoints(); ++i)
        {
            const auto p = h.getHullPoint (i);
            positions.push_back ({ p.x, p.y, 1.10f });
        }

        // EXACT speaker coordinates — the case the kMinDistance floor exists for.
        for (int i = 0; i < 8; ++i)
            positions.push_back (v.speaker (i));

        double worst = 0.0;
        int    tested = 0, inside = 0, outside = 0, nonFinite = 0;

        for (int ri = 0; ri <= 6; ++ri)
        {
            const float rolloff = 3.0f + 0.5f * static_cast<float> (ri);   // 3.0 .. 6.0

            for (int bi = 0; bi <= 5; ++bi)
            {
                const float blur = 0.2f * static_cast<float> (bi);         // 0.0 .. 1.0

                for (const auto& p : positions)
                {
                    const auto gains = rig.solve (p, rolloff, blur, kUnitWeights);

                    if (! allFiniteGains (gains))
                        ++nonFinite;

                    worst = std::max (worst, std::abs (sumOfSquares (gains) - 1.0));
                    ++tested;

                    if (ri == 0 && bi == 0)
                        (h.isInside ({ p.x, p.y }) ? inside : outside)++;
                }
            }
        }

        const bool ok = worst <= 1.0e-6 && nonFinite == 0 && inside > 20 && outside > 20;

        check ("AA sum-of-squares-is-one", ok,
               juce::String (tested) + " solves over rolloff×blur, max |Σv² − 1| = "
                   + juce::String (worst, 11) + " (" + juce::String (inside) + " inside / "
                   + juce::String (outside) + " outside per param pair)"
                   + (nonFinite > 0 ? ", NON-FINITE" : ""));
    }

    //==========================================================================
    // AB — zero weights. DSP-05 criteria 1 and 2.
    {
        const oo::VenueModel v;
        const SolveRig rig (v);

        const oo::Vec3 src { 5.5f, 11.0f, 1.20f };

        bool   zeroExact = true;
        double worstSubsetSum = 0.0;

        for (int muted = 0; muted < 8; ++muted)
        {
            auto w = kUnitWeights;
            w[(size_t) muted] = 0.0f;

            const auto gains = rig.solve (src, 4.0f, 0.10f, w);

            // EXACTLY zero, bitwise — not "small". k·0·t is +0.0f and nothing else is acceptable.
            zeroExact = zeroExact && isExactlyZero (gains[(size_t) muted]);
            worstSubsetSum = std::max (worstSubsetSum, std::abs (sumOfSquares (gains) - 1.0));
        }

        // A 2-speaker subset must still normalise to 1 — the ROADMAP case.
        std::array<float, 8> pair {};
        pair[2] = 1.0f;
        pair[6] = 1.0f;

        const auto pairGains = rig.solve (src, 4.0f, 0.10f, pair);

        bool onlyPairLive = true;

        for (int i = 0; i < 8; ++i)
            if (i != 2 && i != 6 && ! isExactlyZero (pairGains[(size_t) i]))
                onlyPairLive = false;

        const double pairSum = sumOfSquares (pairGains);

        const bool ok = zeroExact && worstSubsetSum <= 1.0e-6 && onlyPairLive
                     && std::abs (pairSum - 1.0) <= 1.0e-6;

        check ("AB zero-weight-and-subset", ok,
               juce::String (zeroExact ? "each muted lane exactly 0.0f" : "A MUTED LANE IS NON-ZERO")
                   + ", 7-live max |Σv²−1| " + juce::String (worstSubsetSum, 10)
                   + ", 2-speaker subset Σv² = " + juce::String (pairSum, 10)
                   + (onlyPairLive ? "" : ", A MUTED LANE LEAKED"));
    }

    //==========================================================================
    // AC — ALL-ZERO WEIGHTS. DSP-05 criterion 3 and QUAL-02.
    //
    // Without §3.3.4's guard: denom = 0 -> k = inf -> v = inf·0 = NaN, which propagates into the
    // SmoothedValue targets and latches PERMANENTLY. Silence must be written explicitly.
    {
        const oo::VenueModel v;
        const SolveRig rig (v);

        const std::array<float, 8> none { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        bool ok = true;
        juce::String detail;

        struct Case { const char* name; oo::Vec3 src; float rolloff; float blur; };

        const std::array<Case, 4> cases
            { { { "room centre",    { 6.5f, 12.0f, 1.10f }, 4.0f, 0.10f },
                { "on a speaker",   { 12.5f, 9.85f, 4.70f }, 6.0f, 0.0f },
                { "far outside",    { -40.0f, 60.0f, -2.0f }, 3.0f, 1.0f },
                { "above the rig",  { 6.5f, 12.0f, 8.0f }, 5.0f, 0.5f } } };

        for (const auto& c : cases)
        {
            const auto gains = rig.solve (c.src, c.rolloff, c.blur, none);

            bool allZero = true;

            for (int i = 0; i < 8; ++i)
                allZero = allZero && std::isfinite (gains[(size_t) i])
                                  && isExactlyZero (gains[(size_t) i]);

            ok = ok && allZero;

            if (! allZero)
                detail << c.name << ": NOT SILENT (" << juce::String (gains[0], 6) << "...); ";
        }

        // `<<`, not `=`. juce::String's const char* CONSTRUCTOR is documented ASCII-only
        // (juce_String.h:88-94) and mangles the em-dash below, while operator+= / operator<< append
        // through CharPointer_UTF8 (juce_String.cpp:773). Every other non-ASCII detail string in
        // this file is built by concatenation, which is why they render and a direct assignment
        // here did not.
        if (ok)
            detail << "4 configurations: all 8 gains exactly 0.0f — not NaN, not full scale";

        check ("AC all-zero-weights-is-silence", ok, detail);
    }

    //==========================================================================
    // AD — finiteness everywhere QUAL-02 names, INCLUDING the §3.1.6 degenerate venues.
    //
    // A degenerate rig reaches the solver just as easily as a good one — the hull may collapse but
    // DBAP still runs, and rigScale = 0 makes the blur mapping produce r_s = 0.
    {
        bool ok = true;
        juce::String detail;

        // 1. Every exact speaker coordinate at blur = 0, at both rolloff ends. Only kMinDistance
        //    keeps d off zero here.
        {
            const oo::VenueModel v;
            const SolveRig rig (v);

            bool finite = true;

            for (float rolloff : { 3.0f, 6.0f })
                for (int i = 0; i < 8; ++i)
                    finite = finite && allFiniteGains (rig.solve (v.speaker (i), rolloff, 0.0f,
                                                                  kUnitWeights));

            ok = ok && finite;
            detail << "on-speaker/blur0 " << (finite ? "finite; " : "NON-FINITE; ");
        }

        // 2. All eight coincident: rigScale = 0, so r_s = 0 on every path, and every d_i is the
        //    same. Normalisation must still produce 1/sqrt(8) each rather than NaN.
        {
            oo::VenueModel v;

            for (int i = 0; i < 8; ++i)
                v.setSpeakerPosition (i, { 3.0f, 4.0f, 5.0f });

            const SolveRig rig (v);
            const auto gains = rig.solve ({ 3.0f, 4.0f, 5.0f }, 4.0f, 1.0f, kUnitWeights);

            const bool good = allFiniteGains (gains) && std::abs (sumOfSquares (gains) - 1.0) <= 1.0e-6;

            ok = ok && good;
            detail << "coincident-rig " << (good ? "finite, Σv²=1; " : "BAD; ");
        }

        // 3. All eight collinear.
        {
            oo::VenueModel v;

            for (int i = 0; i < 8; ++i)
                v.setSpeakerPosition (i, { 0.0f, static_cast<float> (i) * 2.0f, 4.0f });

            const SolveRig rig (v);
            bool finite = true;

            for (float blur : { 0.0f, 1.0f })
                finite = finite && allFiniteGains (rig.solve ({ 5.0f, 7.0f, 1.1f }, 5.0f, blur,
                                                              kUnitWeights));

            ok = ok && finite;
            detail << "collinear-rig " << (finite ? "finite" : "NON-FINITE");
        }

        check ("AD finite-on-degenerate-venues", ok, detail);
    }

    //==========================================================================
    // AE — the pow budget. PERF-02 criterion 3.
    //
    // == 16 EXACTLY, not merely <= 32. The bound alone still passes if the t = pow(d,−a) reuse is
    // dropped and d^(2a) is computed with a second call, which is precisely the regression this
    // number exists to catch.
    {
        const oo::VenueModel v;
        const SolveRig rig (v);

        oo::instr::resetCounters();

        // One SOLVE PAIR — what one control block performs.
        (void) rig.solve ({ 4.0f, 9.0f, 1.10f }, 4.0f, 0.10f, kUnitWeights);
        (void) rig.solve ({ 4.0f, 9.0f, 1.10f }, 4.0f, 0.10f, kUnitWeights);

        const auto calls = oo::instr::get (oo::instr::powCalls);

        const bool withinBudget = calls <= 32;
        const bool exactlySixteen = calls == 16;

        check ("AE pow-budget-per-solve-pair", withinBudget && exactlySixteen,
               juce::String (static_cast<int> (calls)) + " pow calls per solve pair"
                   + (exactlySixteen ? " (== 16, the t = pow(d,-a) reuse is intact)"
                                     : " — EXPECTED 16; the d^(2a) reuse has been dropped")
                   + (withinBudget ? "" : ", AND OVER THE 32 BUDGET"));
    }

    //==========================================================================
    // AF — MIRROR SYMMETRY (RESEARCH-2.2 H4).
    //
    // §OQ4's rig is mirror-symmetric about x = 6.5 m, so with the puck on that axis speaker pairs
    // (1,2), (3,8), (4,7) and (5,6) MUST receive equal gains. This is a correctness property in its
    // own right, and unlike a "the 8 lanes all differ" check it cannot be satisfied by noise: a
    // dropped z term, a transposed index or a sign error in the perpendicular all break it.
    //
    // It is also why a naive FUNC-01/3 independence probe must use an OFF-CENTRE source — on the
    // axis, four pairs are equal on CORRECT code.
    //
    // ── MEASURED, NOT ASSUMED: only three pairs are GUARANTEED bit-identical ──────────────────
    // 0.5 and 12.5 are exactly representable in float32 and sit exactly ±6.0 either side of the
    // puck, so pairs (1,2), (3,8) and (4,7) are bit-for-bit equal by construction.
    //
    // 9.8f and 3.2f are NOT: they land at 9.80000019 and 3.20000005, i.e. 3.30000019 and 3.29999995
    // from x = 6.5. The rig is therefore very slightly asymmetric about the axis, and the
    // double-precision oracle in tests/fixtures/DbapReferenceFixture.h shows it — the gains for
    // speakers 5 and 6 differ there in the 9th significant figure. In float32 that asymmetry sits
    // BELOW one ULP at these gain magnitudes, so the solver currently rounds both to the same float
    // and the measured pair delta is 0. That is a rounding accident, not a guarantee, so `exact` is
    // false for (5,6) and the pair is held to the tolerance instead. Do not "tighten" it to
    // bit-identity because today's build happens to satisfy that.
    {
        const oo::VenueModel v;
        const SolveRig rig (v);

        // On the mirror axis. x = bbMinX + 0.5·span = 0.5 + 6.0 = 6.5 exactly.
        const oo::Vec3 onAxis { 6.5f, 10.0f, 1.30f };
        const auto centred = rig.solve (onAxis, 4.0f, 0.10f, kUnitWeights);

        struct Pair { int a; int b; bool exact; };

        // 0-based. `exact` marks the pairs whose x coordinates are exactly mirrored in float32.
        const std::array<Pair, 4> pairs { { { 0, 1, true }, { 2, 7, true },
                                            { 3, 6, true }, { 4, 5, false } } };

        bool   bitIdenticalWhereExpected = true;
        double worstPairDelta = 0.0;

        for (const auto& p : pairs)
        {
            const float ga = centred[(size_t) p.a];
            const float gb = centred[(size_t) p.b];

            worstPairDelta = std::max (worstPairDelta,
                                       std::abs (static_cast<double> (ga) - static_cast<double> (gb)));

            if (p.exact && ! bitExact (ga, gb))
                bitIdenticalWhereExpected = false;
        }

        // Off the axis the same pairs must SEPARATE — otherwise "symmetric" would be indistinguishable
        // from "the solver ignores x".
        const auto offAxis = rig.solve ({ 2.25f, 7.5f, 1.30f }, 4.0f, 0.10f, kUnitWeights);

        int separated = 0;

        for (const auto& p : pairs)
            if (std::abs (offAxis[(size_t) p.a] - offAxis[(size_t) p.b]) > 1.0e-3f)
                ++separated;

        const bool ok = bitIdenticalWhereExpected && worstPairDelta <= 1.0e-6 && separated == 4;

        check ("AF mirror-symmetry", ok,
               juce::String ("on-axis worst pair delta ") + juce::String (worstPairDelta, 12)
                   + (bitIdenticalWhereExpected ? ", 3 exact pairs bit-identical"
                                                : ", AN EXACT PAIR DIVERGED")
                   + "; off-axis " + juce::String (separated) + "/4 pairs separated");
    }

    //==========================================================================
    // AG — the blur mapping. DSP-08 supporting evidence (DSP-08 itself CLOSES AT 2.3).
    //
    // THE SCALING INVARIANT IS THE REAL ASSERTION. A bare "1.98 m at blur 0.50" is a mirrored
    // fixture — it would be re-blessed to whatever the code produced the day it broke, which is
    // exactly how §OQ4's rigScale carried a wrong 7.95 for two phases. Doubling every coordinate
    // must double r_s, and that property is independent of the venue.
    {
        const oo::VenueModel v;
        const float rigScale = v.rigScale();

        oo::VenueModel doubled;

        for (int i = 0; i < 8; ++i)
        {
            const auto p = v.speaker (i);
            doubled.setSpeakerPosition (i, { p.x * 2.0f, p.y * 2.0f, p.z * 2.0f });
        }

        bool invariant = true;

        for (int i = 1; i <= 5; ++i)
        {
            const float blur = 0.1f * static_cast<float> (i);   // stay well under the 8 m cap

            const float base   = oo::dbap::blurToRadius (blur, rigScale);
            const float scaled = oo::dbap::blurToRadius (blur, doubled.rigScale());

            invariant = invariant && near (scaled, 2.0f * base, 1.0e-4f);
        }

        // The §3.3.2 table at the v1.3.0 scale (kBlurScale 1.5 — the audibility rescale; the
        // pre-1.3 values were exactly a third of these).
        const bool table = near (oo::dbap::blurToRadius (0.0f,  rigScale), 0.0f,   1.0e-6f)
                        && near (oo::dbap::blurToRadius (0.10f, rigScale), 1.19f,  0.01f)
                        && near (oo::dbap::blurToRadius (0.50f, rigScale), 5.95f,  0.01f)
                        && near (oo::dbap::blurToRadius (1.0f,  rigScale), 11.90f, 0.01f);

        // The absolute backstop: a mistyped 1000 m coordinate must not produce an r_s that swamps
        // the array.
        const bool capped = near (oo::dbap::blurToRadius (1.0f, 4000.0f),
                                  oo::dbap::kMaxBlurMetres, 1.0e-6f);

        const bool ok = invariant && table && capped;

        check ("AG blur-to-radius", ok,
               juce::String ("blur 0.50 -> ") + juce::String (oo::dbap::blurToRadius (0.50f, rigScale), 4)
                   + " m (v1.3.0 scale says 5.95), "
                   + (invariant ? "scaling invariant holds" : "INVARIANT VIOLATED")
                   + ", " + (capped ? "24 m cap enforced" : "CAP NOT ENFORCED"));
    }

    //==========================================================================
    // AH — SourceShaper driven DIRECTLY at width > 0.
    //
    // WRITTEN AT 2.2 AS COVERAGE FOR CODE THAT SHIPPED INERT — GainStage then passed a literal
    // 0.0f and never read the width parameter, so this was deliberately NOT a DSP-06 claim. What it
    // bought was that the 2.3 diff became one line rather than one line plus an untested §3.4.1.
    //
    // width is LIVE as of 2.3 and DSP-06 closes on probes AX and AY. This one is kept because it
    // exercises shape() over inputs the caller cannot reach — a degenerate rig in particular.
    {
        const oo::VenueModel v;
        const auto snapshot = snapshotOf (v);

        bool ok = true;
        juce::String detail;

        // 1. HANDEDNESS. Puck downstage of the centroid -> bearing ≈ (0,−1) -> n̂ ≈ (1,0), so P_R
        //    must land at LARGER x, i.e. on the audience's right.
        {
            const float nyDownstage = (6.0f - v.bbMinY()) / (v.bbMaxY() - v.bbMinY());
            const auto sp = oo::shaper::shape (snapshot, 0.5f, nyDownstage, 0.0f, 4.0f);

            const bool rightIsRight = sp.right.x > sp.left.x + 1.0f;
            const bool spanIsWidth  = near (sp.right.x - sp.left.x, 4.0f, 1.0e-3f);

            ok = ok && rightIsRight && spanIsWidth;
            detail << "handedness " << (rightIsRight ? "R on audience right" : "R IS ON THE LEFT")
                   << " (span " << juce::String (sp.right.x - sp.left.x, 3) << " m"
                   << (spanIsWidth ? "" : " — NOT width") << "); ";
        }

        // 2. THE rFade COLLAPSE (§3.4.2). At the centroid the spread must fall to zero, so the
        //    180° bearing flip happens while both sub-points are already coincident and the gain
        //    vectors stay continuous.
        {
            const float nx = (v.centroid().x - v.bbMinX()) / (v.bbMaxX() - v.bbMinX());
            const float ny = (v.centroid().y - v.bbMinY()) / (v.bbMaxY() - v.bbMinY());

            const auto atCentre = oo::shaper::shape (snapshot, nx, ny, 0.0f, 6.0f);

            const bool collapsed = std::abs (atCentre.wEff) < 1.0e-3f
                                && std::abs (atCentre.right.x - atCentre.left.x) < 1.0e-3f
                                && std::abs (atCentre.right.y - atCentre.left.y) < 1.0e-3f;

            // ... and it must RECOVER: outside rFade the full width is delivered.
            const auto farOut = oo::shaper::shape (snapshot, 0.05f, 0.95f, 0.0f, 6.0f);
            const bool recovers = near (farOut.wEff, 6.0f, 1.0e-3f);

            ok = ok && collapsed && recovers;
            detail << "rFade " << (collapsed ? "collapses at centroid" : "DID NOT COLLAPSE")
                   << "/" << (recovers ? "recovers outside" : "DOES NOT RECOVER") << "; ";
        }

        // 3. THE (0,−1) FALLBACK at |b| < 1e-6. Finite, and both sub-points sit on the puck.
        {
            auto degenerate = snapshot;
            degenerate.rigScale = 0.0f;                 // forces the fade guard too

            const auto sp = oo::shaper::shape (degenerate, 0.5f, 0.5f, 0.0f, 6.0f);

            const bool finite = std::isfinite (sp.left.x) && std::isfinite (sp.left.y)
                             && std::isfinite (sp.left.z) && std::isfinite (sp.right.x)
                             && std::isfinite (sp.right.y) && std::isfinite (sp.right.z)
                             && std::isfinite (sp.wEff);

            const bool coincident = near (sp.left.x, sp.right.x, 1.0e-6f)
                                 && near (sp.left.y, sp.right.y, 1.0e-6f);

            ok = ok && finite && coincident;
            detail << "degenerate-rig " << (finite ? "finite" : "NON-FINITE")
                   << (coincident ? "/coincident; " : "/SPLIT; ");
        }

        // 4. PER-SUB-POINT earHeight. A diagonal bearing gives n̂ a y component, so the two
        //    sub-points sit at different depths on a sloped plane and must resolve DIFFERENT
        //    heights. Hoisting earHeight out of the pair would make these equal.
        {
            oo::VenueModel steep = v;
            steep.setRake (1.10f, 8.0f);               // exaggerated so the difference is visible

            const auto steepSnap = snapshotOf (steep);
            const auto sp = oo::shaper::shape (steepSnap, 0.15f, 0.15f, 0.0f, 6.0f);

            const bool differentY = std::abs (sp.left.y - sp.right.y) > 0.5f;
            const bool differentZ = std::abs (sp.left.z - sp.right.z) > 1.0e-3f;

            ok = ok && differentY && differentZ;
            detail << "per-sub-point z: dy " << juce::String (std::abs (sp.left.y - sp.right.y), 3)
                   << " m -> dz " << juce::String (std::abs (sp.left.z - sp.right.z), 4) << " m"
                   << (differentZ ? "" : " — HOISTED OUT OF THE PAIR");
        }

        check ("AH source-shaper-width-live", ok, detail);
    }

    //==========================================================================
    // AU — DSP-07 criterion 4, plus RESEARCH-2.3 H4's Nyquist bounds.
    //
    // H4: setCutoffFrequency asserts isPositiveAndBelow (fc, fs * 0.5), and past Nyquist
    // tan (pi*fc/fs) goes NEGATIVE so G = g/(1+g) is negative or singular and the one-pole is not a
    // lowpass at all. A literal 20 000 therefore crashes in Debug and produces nonsense in Release
    // at 22.05 and 32 kHz. THE ASSERTION JUCE WOULD HAVE MADE IS MADE HERE INSTEAD, where it also
    // holds in Release.
    {
        bool         ok = true;
        juce::String detail;

        // 1. §3.5.2's four-row table at 48 kHz. The arithmetic is exact; 1 Hz is ample.
        {
            struct Row { float air; float d; float fc; };

            const std::array<Row, 4> table
                { { { 0.35f,  5.0f, 13348.4f },
                    { 0.35f, 15.0f,  5946.0f },
                    { 1.00f,  5.0f,  6299.6f },
                    { 1.00f, 15.0f,   625.0f } } };

            bool tableOk = true;

            for (const auto& r : table)
                tableOk = tableOk && near (oo::hullproc::airCutoffHz (r.air, r.d, 48000.0), r.fc, 1.0f);

            ok = ok && tableOk;
            detail << "§3.5.2 table @48k " << (tableOk ? "exact to 1 Hz" : "MISMATCH") << "; ";
        }

        // 2. The H4 ceiling at four rates — reached at d_hull = 0, i.e. inside the hull, which is
        //    also the value a re-entering filter carries.
        {
            struct Rate { double fs; float ceiling; };

            const std::array<Rate, 4> rates
                { { { 22050.0,  9922.5f }, { 32000.0, 14400.0f },
                    { 44100.0, 19845.0f }, { 48000.0, 20000.0f } } };

            bool ceilingOk = true;

            for (const auto& r : rates)
                ceilingOk = ceilingOk
                         && near (oo::hullproc::airCutoffHz (0.35f, 0.0f, r.fs), r.ceiling, 0.01f);

            ok = ok && ceilingOk;
            detail << "ceilings " << (ceilingOk ? "9922.5/14400/19845/20000" : "WRONG") << "; ";
        }

        // 3. fc < fs/2 EVERYWHERE in range — the whole point of H4. Swept over the exposed
        //    airAmount range and well past any realistic hall depth.
        {
            const std::array<double, 5> rates { { 22050.0, 32000.0, 44100.0, 48000.0, 96000.0 } };

            bool belowNyquist = true;
            bool floorNeverInverts = true;

            for (double fs : rates)
            {
                const float nyquist = 0.5f * static_cast<float> (fs);

                for (int ai = 0; ai <= 20; ++ai)
                    for (int di = 0; di <= 50; ++di)
                    {
                        const float air = 0.05f * static_cast<float> (ai);
                        const float d   = 1.0f  * static_cast<float> (di);
                        const float fc  = oo::hullproc::airCutoffHz (air, d, fs);

                        belowNyquist = belowNyquist && fc < nyquist && fc > 0.0f
                                    && std::isfinite (fc);

                        // The floor takes the ceiling as its own upper bound, so the clamp cannot
                        // invert: no returned value may exceed the d_hull = 0 ceiling at that rate.
                        floorNeverInverts = floorNeverInverts
                                         && fc <= oo::hullproc::airCutoffHz (0.0f, 0.0f, fs) + 1.0e-3f;
                    }
            }

            ok = ok && belowNyquist && floorNeverInverts;
            detail << (belowNyquist ? "fc<Nyquist at 5 rates x 21 x 51" : "PAST NYQUIST")
                   << "/" << (floorNeverInverts ? "clamp never inverts" : "CLAMP INVERTED") << "; ";
        }

        // 4. The floor binds only at extremes — at airAmount = 1.0 it is reached at d_hull ≈
        //    15.97 m, off the far edge of any realistic hall. Asserted from BOTH sides so a floor
        //    that had swallowed the whole curve would fail.
        {
            const float justInside  = oo::hullproc::airCutoffHz (1.0f, 15.0f, 48000.0);
            const float wellBeyond  = oo::hullproc::airCutoffHz (1.0f, 25.0f, 48000.0);

            const bool floorHolds = wellBeyond > oo::hullproc::kAirFloorHz - 1.0e-3f
                                 && wellBeyond < oo::hullproc::kAirFloorHz + 1.0e-3f
                                 && justInside > oo::hullproc::kAirFloorHz;

            ok = ok && floorHolds;
            detail << "floor: " << juce::String (justInside, 1) << " Hz @15 m -> "
                   << juce::String (wellBeyond, 1) << " Hz @25 m"
                   << (floorHolds ? "" : " — FLOOR WRONG");
        }

        check ("AU air-cutoff-curve-and-nyquist", ok, detail);
    }

    //==========================================================================
    // AV — DSP-07 criteria 1, 2 and 3: the trim law, and the FIRST HALF of P33's structural
    // bit-transparency proof.
    //
    // There is no "trim absent" build to render against, and building one would mean a second
    // arithmetic path behind a compile flag — the thing §3.4.3 forbids. So the identity is proven
    // where it actually lives: hullTrimGain returns EXACTLY 1.0f, by memcmp, and `v * 1.0f == v`
    // follows for every finite v. Probe BD supplies the non-vacuity half (the trim DOES change the
    // render when enabled).
    {
        bool         ok = true;
        juce::String detail;

        // 1. Linear in dB per metre, over the exposed hullAtten range and a realistic d_hull span.
        {
            bool linear = true;

            for (int ai = 1; ai <= 6; ++ai)
                for (int di = 1; di <= 20; ++di)
                {
                    const float atten = 0.5f * static_cast<float> (ai);     // 0.5 .. 3.0 dB/m
                    const float d     = 1.0f * static_cast<float> (di);
                    const float wantDb = std::max (-atten * d, oo::hullproc::kTrimFloorDb);

                    const float gotDb = juce::Decibels::gainToDecibels (
                                            oo::hullproc::hullTrimGain (atten, d));

                    linear = linear && near (gotDb, wantDb, 1.0e-3f);
                }

            ok = ok && linear;
            detail << "dB/m law " << (linear ? "linear over 6x20" : "NOT LINEAR") << "; ";
        }

        // 2. The floor is EXACTLY −24 dB, not approximately.
        {
            const float deep  = oo::hullproc::hullTrimGain (3.0f, 40.0f);   // would be −120 dB
            const float atEnd = juce::Decibels::gainToDecibels (deep);

            const bool floored = near (atEnd, oo::hullproc::kTrimFloorDb, 1.0e-3f);

            ok = ok && floored;
            detail << "floor " << juce::String (atEnd, 4) << " dB"
                   << (floored ? "" : " — NOT −24") << "; ";
        }

        // 3. P33 FIRST HALF — hullAtten = 0 is bit-exactly unity over the whole d_hull sweep.
        //    memcmp, not near(): the claim is bit-transparency, and a tolerance would prove a
        //    weaker statement than the requirement makes.
        {
            bool unity = true;

            for (int di = 0; di <= 200; ++di)
                unity = unity && bitExact (oo::hullproc::hullTrimGain (0.0f,
                                               0.25f * static_cast<float> (di)), 1.0f);

            ok = ok && unity;
            detail << "hullAtten=0 " << (unity ? "bit-exact 1.0f over 201 d" : "NOT BIT-EXACT") << "; ";
        }

        // 4. DSP-07/3 — no-op INSIDE the hull regardless of setting. d_hull is exactly 0.0f there
        //    (solveSubPoint returns it explicitly), so the argument is −0.0f and pow(10, −0.0f) is
        //    exactly 1.0f by C99 Annex F.
        {
            bool insideUnity = true;

            for (int ai = 0; ai <= 12; ++ai)
                insideUnity = insideUnity && bitExact (oo::hullproc::hullTrimGain (
                                                 0.25f * static_cast<float> (ai), 0.0f), 1.0f);

            ok = ok && insideUnity;
            detail << "d_hull=0 " << (insideUnity ? "bit-exact 1.0f over 13 atten"
                                                  : "NOT BIT-EXACT");
        }

        check ("AV hull-trim-law-and-unity", ok, detail);
    }

    //==========================================================================
    // AW — DSP-08, closing the requirement probe AG only supported.
    //
    // ── WHY λ = 2 AT blur = 0.25 AND NOT blur = 1 (RESEARCH-2.3 H8) ──────────────────────────
    // The invariance is EXACT — v_i is homogeneous of degree 0 in λ — and the only scale-breaking
    // terms are the two clamps. At blur = 1, λ = 2 the wanted r_s is 7.932 m against a
    // kMaxBlurMetres of 8.0: a 0.9% margin. That probe passes today and fails the moment rigScale
    // moves, and rigScale has ALREADY been corrected twice (7.95 -> 7.93165). blur = 0.25 leaves a
    // 4x margin.
    //
    // ── AND THE SOURCE IS SCALED TOO ─────────────────────────────────────────────────────────
    // Scaling only the speakers changes the geometry rather than its scale, and the probe would
    // then fail against a CORRECT implementation. Everything scales about the centroid, which is
    // itself invariant under that map.
    {
        const oo::VenueModel base;
        const auto c = base.centroid();

        const auto scaledRig = [&base, c] (float lambda)
        {
            oo::VenueModel v = base;

            for (int i = 0; i < 8; ++i)
            {
                const auto p = base.speaker (i);

                v.setSpeakerPosition (i, { c.x + lambda * (p.x - c.x),
                                           c.y + lambda * (p.y - c.y),
                                           c.z + lambda * (p.z - c.z) });
            }

            return v;
        };

        const auto scaledPoint = [c] (oo::Vec3 p, float lambda)
        {
            return oo::Vec3 { c.x + lambda * (p.x - c.x),
                              c.y + lambda * (p.y - c.y),
                              c.z + lambda * (p.z - c.z) };
        };

        // An off-centre, off-axis source, so no symmetry can mask a scale error.
        const oo::Vec3 src { 4.2f, 7.1f, 1.6f };

        const auto worstDelta = [&] (float lambda, float blur)
        {
            const SolveRig baseRig (base);
            const auto     scaled = scaledRig (lambda);
            const SolveRig scaledRigSolver (scaled);

            const auto a = baseRig.solve (src, 4.5f, blur, kUnitWeights);
            const auto b = scaledRigSolver.solve (scaledPoint (src, lambda), 4.5f, blur,
                                                  kUnitWeights);

            float worst = 0.0f;

            for (int i = 0; i < 8; ++i)
                worst = std::max (worst, std::abs (a[(size_t) i] - b[(size_t) i]));

            return worst;
        };

        // 1. THE INVARIANCE, at both a shrink and a stretch.
        constexpr float kInvarianceTol = 1.0e-4f;

        const float half   = worstDelta (0.5f, 0.25f);
        const float twice  = worstDelta (2.0f, 0.25f);

        const bool invariant = half <= kInvarianceTol && twice <= kInvarianceTol;

        // 2. POSITIVE CONTROL — at blur = 1, λ = 2.1 the wanted r_s is 8.328 m and kMaxBlurMetres
        //    clamps it to 8.0, so the invariance MUST break. A probe that cannot see the clamp
        //    cannot claim the invariance is a property of the code rather than of the numbers.
        const float clamped = worstDelta (2.1f, 1.0f);
        const bool  controlFires = clamped > 1.0e-3f;

        // 3. DSP-08/4 — blur is ADDITIONAL to the physical floor from flown speaker height. At
        //    blur = 0 with the source on the floor directly beneath speaker 1, the 3-D model still
        //    yields a finite, non-degenerate vector.
        bool floorFinite = false;
        {
            const auto s1 = base.speaker (0);
            const SolveRig rig (base);
            const auto v = rig.solve ({ s1.x, s1.y, 0.0f }, 4.5f, 0.0f, kUnitWeights);

            floorFinite = allFiniteGains (v) && std::abs (sumOfSquares (v) - 1.0) < 1.0e-4;
        }

        // 4. DSP-08/3 — both caps are static_asserted in DbapSolver.h since Phase 2.2. Referenced
        //    rather than duplicated: a second copy of an assertion is a second thing to drift.
        const bool capsPresent = oo::dbap::kMaxBlurMetres > 0.0f && oo::dbap::kBlurScale > 0.0f;

        const bool ok = invariant && controlFires && floorFinite && capsPresent;

        // Built with << rather than juce::String(const char*): that constructor is ASCII-only and
        // mangles the UTF-8 in "λ"/"Δ" (critical_juce_string_char_ctor_is_ascii_only). No compiler
        // warning; visible only in the output.
        juce::String awDetail;

        awDetail << "blur 0.25: λ=0.5 Δ" << juce::String (half, 9)
                 << ", λ=2.0 Δ" << juce::String (twice, 9)
                 << (invariant ? " (invariant)" : " — INVARIANCE VIOLATED")
                 << "; positive control blur=1 λ=2.1 Δ" << juce::String (clamped, 6)
                 << (controlFires ? " (clamp visible, so the probe CAN fail)"
                                  : " — CLAMP INVISIBLE, THIS PROBE CANNOT FAIL")
                 << "; blur=0 at a speaker's floor point "
                 << (floorFinite ? "finite, Σv²=1" : "DEGENERATE");

        check ("AW blur-invariance-under-room-scale", ok, awDetail);
    }

    //==========================================================================
    // AX — DSP-06 criteria 2, 3 and 5, with width LIVE in the caller.
    //
    // AH covered the same function at 2.2 as coverage for code shipped inert. This is the DSP-06
    // claim: the caller now passes p[params::width], so these are properties of the plugin rather
    // than of a function nothing called with a non-zero argument.
    {
        const oo::VenueModel v;
        const auto snapshot = snapshotOf (v);

        bool         ok = true;
        juce::String detail;

        const auto normOf = [&v] (float x, float y)
        {
            return std::pair<float, float> { (x - v.bbMinX()) / (v.bbMaxX() - v.bbMinX()),
                                             (y - v.bbMinY()) / (v.bbMaxY() - v.bbMinY()) };
        };

        // 1. DSP-06/2 — HANDEDNESS, ASSERTED RATHER THAN REASONED. Puck downstage of the CENTROID
        //    (not the bbox centre) -> bearing ≈ (0,−1) -> n̂ ≈ (1,0), so R lands at larger x.
        {
            const auto n = normOf (v.centroid().x, 6.0f);
            const auto sp = oo::shaper::shape (snapshot, n.first, n.second, 0.0f, 4.0f);

            const bool rightIsRight = sp.right.x > sp.left.x + 1.0f;
            const bool spanIsWidth  = near (sp.right.x - sp.left.x, 4.0f, 1.0e-3f);
            const bool perpToBearing = std::abs (sp.right.y - sp.left.y) < 1.0e-3f;

            ok = ok && rightIsRight && spanIsWidth && perpToBearing;
            detail << "handedness " << (rightIsRight ? "R on audience right" : "R IS ON THE LEFT")
                   << ", span " << juce::String (sp.right.x - sp.left.x, 3) << " m"
                   << (spanIsWidth ? "" : " — NOT width")
                   << (perpToBearing ? ", ⟂ bearing" : " — NOT PERPENDICULAR") << "; ";
        }

        // 2. DSP-06/3 — each sub-point resolves its z at ITS OWN y. BOTH HALVES, or the probe
        //    passes on an implementation that ignores y entirely.
        {
            // Sloped: a diagonal bearing gives n̂ a y component, so the two sub-points sit at
            // different depths on a raked plane.
            oo::VenueModel steep = v;
            steep.setRake (1.10f, 8.0f);

            const auto steepSnap = snapshotOf (steep);
            const auto diag = oo::shaper::shape (steepSnap, 0.15f, 0.15f, 0.0f, 6.0f);

            const bool dy = std::abs (diag.left.y - diag.right.y) > 0.5f;
            const bool dz = std::abs (diag.left.z - diag.right.z) > 1.0e-3f;

            // Flat: the SAME geometry with no rake must give EQUAL heights. Without this half, an
            // implementation that returned an arbitrary y-dependent value would still pass.
            oo::VenueModel flat = v;
            flat.setRake (2.0f, 2.0f);

            const auto flatSnap = snapshotOf (flat);
            const auto flatDiag = oo::shaper::shape (flatSnap, 0.15f, 0.15f, 0.0f, 6.0f);

            const bool flatEqual = near (flatDiag.left.z, flatDiag.right.z, 1.0e-6f);

            ok = ok && dy && dz && flatEqual;
            detail << "per-sub-point z: dy " << juce::String (std::abs (diag.left.y - diag.right.y), 2)
                   << " m -> dz " << juce::String (std::abs (diag.left.z - diag.right.z), 4) << " m"
                   << (dz ? "" : " — HOISTED OUT OF THE PAIR")
                   << ", flat rake " << (flatEqual ? "equal z" : "UNEQUAL z ON A FLAT PLANE") << "; ";
        }

        // 3. DSP-06/5 — the |b| < 1e-6 fallback.
        //
        //    THE FALLBACK'S DIRECTION IS UNOBSERVABLE THROUGH shape(), BY CONSTRUCTION, and saying
        //    so is more useful than pretending otherwise: rFade collapses wEff to 0 everywhere
        //    inside 0.05·rigScale ≈ 0.40 m (v1.3.0), and 1e-6 m is deep inside that. What the fallback
        //    EXISTS for is the 0/0 — without it b̂ = (0/0, 0/0) = NaN and both sub-points are NaN.
        //    So: finite, and both land BIT-EXACTLY on the puck. That fails loudly if the guard goes.
        {
            const auto n = normOf (v.centroid().x, v.centroid().y);
            const auto sp = oo::shaper::shape (snapshot, n.first, n.second, 0.0f, 6.0f);

            const bool finite = std::isfinite (sp.left.x)  && std::isfinite (sp.left.y)
                             && std::isfinite (sp.left.z)  && std::isfinite (sp.right.x)
                             && std::isfinite (sp.right.y) && std::isfinite (sp.right.z)
                             && std::isfinite (sp.wEff);

            const bool onThePuck = bitExact (sp.left.x, sp.right.x)
                                && bitExact (sp.left.y, sp.right.y);

            ok = ok && finite && onThePuck;
            detail << "|b|→0 " << (finite ? "finite" : "NaN — THE 0/0 GUARD IS GONE")
                   << "/" << (onThePuck ? "coincident on the puck; " : "SPLIT; ");
        }

        // 4. The rFade collapse AS ARITHMETIC (the continuity half is probe BA, in the harness).
        //    Monotone in |b|, and exactly `width` at and beyond rFade.
        {
            const float rFade = oo::shaper::kFadeFraction * v.rigScale();

            bool  monotone = true;
            float previous = -1.0f;

            for (int k = 0; k <= 40; ++k)
            {
                const float bLen = rFade * (0.05f * static_cast<float> (k));   // 0 .. 2·rFade
                const auto  n = normOf (v.centroid().x + bLen, v.centroid().y);
                const auto  sp = oo::shaper::shape (snapshot, n.first, n.second, 0.0f, 6.0f);

                monotone = monotone && sp.wEff >= previous - 1.0e-5f;
                previous = sp.wEff;
            }

            const auto atFade = normOf (v.centroid().x + rFade * 1.001f, v.centroid().y);
            const auto beyond = normOf (v.centroid().x + rFade * 3.0f,   v.centroid().y);

            const bool full = near (oo::shaper::shape (snapshot, atFade.first, atFade.second,
                                                       0.0f, 6.0f).wEff, 6.0f, 1.0e-3f)
                           && near (oo::shaper::shape (snapshot, beyond.first, beyond.second,
                                                       0.0f, 6.0f).wEff, 6.0f, 1.0e-3f);

            ok = ok && monotone && full;
            detail << "rFade " << juce::String (rFade, 3) << " m: "
                   << (monotone ? "monotone" : "NOT MONOTONE")
                   << "/" << (full ? "full width at and beyond" : "WIDTH NOT REACHED");
        }

        check ("AX sub-point-geometry-live", ok, detail);
    }

    // ══════════════════════════════════════════════════════════════════════════════════════════
    // PHASE 3.2 — BN, BO, BV.
    //
    // All three live in THIS target rather than the harness because none of them needs a
    // processor, and VenueFile.cpp compiles against juce_core + juce_data_structures only. That is
    // what puts UI-01 criterion 3(a) in the seconds-to-build target instead of behind a plugin.
    // ══════════════════════════════════════════════════════════════════════════════════════════

    // A venue with all 42 values DISTINCT and none of them the §OQ4 default, so a round trip that
    // silently fell back to defaults would fail on every field rather than on none.
    const auto makeMeasuredVenue = []
    {
        oo::VenueModel v;

        const std::array<const char*, 8> labels { "Rrs", "Lrs", "Rss", "Lss", "Lfe", "C", "R", "L" };

        for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
        {
            const float f = static_cast<float> (i);

            v.setSpeakerPosition (i, { 1.125f + f * 1.375f,
                                       2.250f + f * 2.125f,
                                       3.375f + f * 0.125f });
            v.setSpeakerTrimDb (i, -5.5f + f * 1.25f);
            v.setSpeakerLabel  (i, labels[static_cast<std::size_t> (i)]);
        }

        v.setRake (0.875f, 2.625f);
        v.setName ("Measured hall");
        return v;
    };

    /** The 42-value bit-compare, as ONE predicate so BN and BO cannot drift apart in what they
        mean by "identical". 32 coordinates + 8 trims is 40 floats compared through their object
        representation (no `==`, so no -Wfloat-equal), plus 8 labels and 2 rake heights. */
    const auto sameFortyTwo = [] (const oo::VenueModel& a, const oo::VenueModel& b, int& firstBad)
    {
        firstBad = -1;

        for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
        {
            const auto pa = a.speaker (i);
            const auto pb = b.speaker (i);

            if (! (bitExact (pa.x, pb.x) && bitExact (pa.y, pb.y) && bitExact (pa.z, pb.z)
                   && bitExact (a.trimDb (i), b.trimDb (i))
                   && a.labelAbbreviation (i) == b.labelAbbreviation (i)))
            {
                firstBad = i;
                return false;
            }
        }

        return bitExact (a.rakeFront(), b.rakeFront()) && bitExact (a.rakeRear(), b.rakeRear());
    };

    const auto scratchDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("OOctagon-venuefile-probes");
    scratchDir.createDirectory();

    //==========================================================================
    // BN — FUNC-02/2 and UI-01/3(a): the .venue round trip is EXACT, through the SAME two
    //      functions the chooser completions call.
    //
    //      (a) of P57's three parts. It is non-vacuous only because (b) — section 29 of
    //      ui_frontend_check.js — separately asserts that those completions call exactly
    //      oo::venuefile::save / ::load and that no parallel serialisation path exists anywhere
    //      outside VenueFile.cpp. Two correct halves wired to different code is the failure that
    //      class of gate exists for.
    {
        const auto source = makeMeasuredVenue();
        const auto file   = scratchDir.getChildFile ("roundtrip.venue");

        file.deleteFile();

        const bool saved = oo::venuefile::save (source, file);

        // FRESH, default-constructed. The contract's first rule, and the one that makes a partial
        // file legible: a missing attribute falls back to a KNOWN §OQ4 default rather than to a
        // neighbouring venue's value.
        oo::VenueModel loaded;
        int fileVersion = -1;

        const auto result = oo::venuefile::load (file, loaded, &fileVersion);

        int firstBad = -1;
        const bool identical = sameFortyTwo (source, loaded, firstBad);

        // The load must not merely agree with the source — it must have MOVED the fresh model off
        // its defaults, or a save that wrote nothing would pass against a source that happened to
        // be default.
        oo::VenueModel untouched;
        int ignored = -1;
        const bool moved = ! sameFortyTwo (untouched, loaded, ignored);

        const bool ok = saved && result == oo::venuefile::LoadResult::ok && identical && moved
                        && fileVersion == oo::VenueModel::kSchemaVersion;

        juce::String detail;
        detail << "save " << (saved ? "ok" : "FAILED")
               << ", load " << static_cast<int> (result)
               << " (0=ok), @schemaVersion " << fileVersion
               << ", all 42 bit-identical: " << (identical ? "yes" : "NO")
               << (firstBad >= 0 ? " (first mismatch speaker " + juce::String (firstBad + 1) + ")" : "")
               << ", differs from defaults: " << (moved ? "yes" : "NO — THE FILE MAY BE EMPTY")
               << ", " << file.getSize() << " bytes";

        check ("BN venue-file-roundtrip-42", ok, detail);
        file.deleteFile();
    }

    //==========================================================================
    // BO — Q6 / P56: a forward version is SURFACED, and a malformed root is REJECTED WITHOUT
    //      TOUCHING `out`.
    //
    //      readFromState() falls back PER ATTRIBUTE to whatever the model already held and never
    //      branches on @schemaVersion. That is right for session state and wrong for a file: a
    //      .venue from a future build would load as a mixture of the file and the live room, with
    //      nothing on screen distinguishing it from a correct load. In a hall that is
    //      unrecoverable, because the operator cannot tell which numbers are theirs.
    {
        bool         ok = true;
        juce::String detail;

        // 1. A FORWARD version still loads, and says so.
        {
            const auto file = scratchDir.getChildFile ("forward.venue");
            file.deleteFile();

            auto tree = makeMeasuredVenue().toValueTree();
            tree.setProperty (oo::VenueModel::propSchemaVersion,
                              oo::VenueModel::kSchemaVersion + 7, nullptr);

            if (auto xml = tree.createXml())
                xml->writeTo (file);

            oo::VenueModel loaded;
            int version = -1;
            const auto result = oo::venuefile::load (file, loaded, &version);

            int firstBad = -1;
            const bool usable = sameFortyTwo (makeMeasuredVenue(), loaded, firstBad);
            const bool surfaced = result == oo::venuefile::LoadResult::forwardVersion
                                  && version == oo::VenueModel::kSchemaVersion + 7;

            ok = ok && surfaced && usable;
            detail << "forward: " << (surfaced ? "SURFACED" : "SILENT")
                   << "/@" << version << ", values " << (usable ? "usable" : "LOST") << "; ";

            file.deleteFile();
        }

        // 2. A NON-VENUE root is rejected and `out` is untouched.
        {
            const auto file = scratchDir.getChildFile ("wrongroot.venue");
            file.deleteFile();

            juce::ValueTree bogus { juce::Identifier ("SCENES") };
            bogus.setProperty (juce::Identifier ("name"), "not a venue", nullptr);

            if (auto xml = bogus.createXml())
                xml->writeTo (file);

            // Deliberately NOT default-constructed: this is the "loaded into the live model"
            // shape, and the assertion is that it comes back UNCHANGED.
            oo::VenueModel live = makeMeasuredVenue();
            const auto result = oo::venuefile::load (file, live, nullptr);

            int firstBad = -1;
            const bool untouched = sameFortyTwo (makeMeasuredVenue(), live, firstBad);
            const bool rejected  = result == oo::venuefile::LoadResult::malformedRoot;

            ok = ok && rejected && untouched;
            detail << "non-VENUE root: " << (rejected ? "rejected" : "ACCEPTED")
                   << "/" << (untouched ? "out untouched" : "OUT WAS MODIFIED") << "; ";

            file.deleteFile();
        }

        // 3. FEWER THAN 8 SPEAKER children — the shape that would otherwise half-apply, leaving a
        //    room that is partly measured and partly placeholder.
        {
            const auto file = scratchDir.getChildFile ("short.venue");
            file.deleteFile();

            auto tree = makeMeasuredVenue().toValueTree();

            while (tree.getNumChildren() > 3)
                tree.removeChild (tree.getNumChildren() - 1, nullptr);

            if (auto xml = tree.createXml())
                xml->writeTo (file);

            oo::VenueModel live = makeMeasuredVenue();
            const auto result = oo::venuefile::load (file, live, nullptr);

            int firstBad = -1;
            const bool untouched = sameFortyTwo (makeMeasuredVenue(), live, firstBad);
            const bool rejected  = result == oo::venuefile::LoadResult::malformedRoot;

            ok = ok && rejected && untouched;
            detail << "3-speaker file: " << (rejected ? "rejected" : "HALF-APPLIED")
                   << "/" << (untouched ? "out untouched" : "OUT WAS MODIFIED") << "; ";

            file.deleteFile();
        }

        // 4. A missing file is `unreadable`, not a crash and not a silent default venue.
        {
            oo::VenueModel live = makeMeasuredVenue();
            const auto result = oo::venuefile::load (scratchDir.getChildFile ("nothing-here.venue"),
                                                    live, nullptr);
            int firstBad = -1;
            const bool untouched = sameFortyTwo (makeMeasuredVenue(), live, firstBad);

            ok = ok && result == oo::venuefile::LoadResult::unreadable && untouched;
            detail << "missing file: " << static_cast<int> (result) << "/"
                   << (untouched ? "out untouched" : "OUT WAS MODIFIED");
        }

        check ("BO venue-file-version-and-root", ok, detail);
    }

    //==========================================================================
    // BV — N7 / P54: the three MapFailure reasons are DISTINGUISHED and carry the right row.
    //
    //      buildSpeakerToBuffer has always separated these three cases and then thrown the
    //      distinction away in a bool. After N8 the banner built from this is the only thing
    //      telling an operator why seven speakers just went mono, and in a hall WHICH ROW is the
    //      actionable half.
    {
        bool         ok = true;
        juce::String detail;

        const auto set8 = juce::AudioChannelSet::create7point1();

        std::array<int, ochan::kNumSpeakers> out { 0, 1, 2, 3, 4, 5, 6, 7 };
        const auto pristine = out;

        // 0. The SUCCESS path leaves { none, -1 }. Without this the three failures below could all
        //    be passing against a diagnosis that is simply never cleared.
        {
            oo::VenueModel v;
            ochan::MapDiagnosis d { ochan::MapFailure::duplicateLabel, 5 };   // deliberately dirty

            const bool built = ochan::buildSpeakerToBuffer (set8, v.labelTypes(), out, &d);
            const bool clean = built && d.reason == ochan::MapFailure::none && d.speakerIndex == -1;

            ok = ok && clean;
            detail << "success: " << (clean ? "cleared" : "STALE REASON SURVIVED") << "; ";
        }

        // 1. notEightChannels — the set is wrong, so no row is named.
        {
            ochan::MapDiagnosis d {};
            oo::VenueModel v;

            const bool built = ochan::buildSpeakerToBuffer (juce::AudioChannelSet::stereo(),
                                                            v.labelTypes(), out, &d);
            const bool right = ! built && d.reason == ochan::MapFailure::notEightChannels
                               && d.speakerIndex == -1;

            ok = ok && right;
            detail << "notEightChannels: " << (right ? "named, row -1" : "WRONG") << "; ";
        }

        // 2. labelNotInSet — the row IS named, and it is the one carrying the foreign label.
        {
            ochan::MapDiagnosis d {};
            oo::VenueModel v;
            v.setSpeakerLabel (4, "Ambisonic W");     // resolves to a type no 7.1 set contains

            const bool built = ochan::buildSpeakerToBuffer (set8, v.labelTypes(), out, &d);
            const bool right = ! built && d.reason == ochan::MapFailure::labelNotInSet
                               && d.speakerIndex == 4;

            ok = ok && right;
            detail << "labelNotInSet: " << (right ? "row 5" : "WRONG (" + juce::String (d.speakerIndex) + ")")
                   << "; ";
        }

        // 3. duplicateLabel — the SECOND of the two colliding rows, which under commit-on-blur is
        //    the one the operator just typed.
        {
            ochan::MapDiagnosis d {};
            oo::VenueModel v;
            v.setSpeakerLabel (6, v.labelAbbreviation (1));   // row 7 duplicates row 2

            const bool built = ochan::buildSpeakerToBuffer (set8, v.labelTypes(), out, &d);
            const bool right = ! built && d.reason == ochan::MapFailure::duplicateLabel
                               && d.speakerIndex == 6;

            ok = ok && right;
            detail << "duplicateLabel: " << (right ? "row 7" : "WRONG (" + juce::String (d.speakerIndex) + ")")
                   << "; ";
        }

        // 4. `out` was never written on ANY failure path — the retention contract, unchanged by
        //    the diagnosis being added.
        {
            const bool retained = out == pristine;
            ok = ok && retained;
            detail << "out " << (retained ? "retained" : "PARTIALLY WRITTEN");
        }

        check ("BV map-failure-reasons", ok, detail);
    }

    //==========================================================================
    //== PHASE 3.3 — CA-CH. The field sampler (UI-04) and the scene predicate (FUNC-06). ========
    //==========================================================================
    //
    // CG IS ONE OF THE TWO PROBES THAT CARRY THE PHASE. It is the ONLY assertion in 3.3 that a
    // fixed-index scene implementation fails; every other scene probe below passes under that
    // defect, which is what makes CG the non-vacuity guard rather than additional coverage.
    // (The other is CM, in the render harness: the only probe a v_i meter fails.)

    // A published room, built the way the processor builds one. Local to this block because it is
    // the only thing here that needs a VenueSnapshot, and because a helper at file scope would
    // invite the 2.1/2.2 probes to start using it.
    const auto makeSnapshot = [] (const oo::VenueModel& v, const oo::ConvexHull2D& h)
    {
        oo::VenueSnapshot s;

        s.spk = v.speakerPositions();

        for (int i = 0; i < 8; ++i)
            s.hullPts[(size_t) i] = h.getHullPoint (i);

        s.hullCount    = h.getNumHullPoints();
        s.hullEpsCross = h.getCrossEpsilon();
        s.centroid     = v.centroid();
        s.rigScale     = v.rigScale();
        s.bbMinX       = v.bbMinX();
        s.bbMaxX       = v.bbMaxX();
        s.bbMinY       = v.bbMinY();
        s.bbMaxY       = v.bbMaxY();
        s.rakeFront    = v.rakeFront();
        s.rakeRear     = v.rakeRear();

        return s;
    };

    //==========================================================================
    // CA — UI-04/1: the out-param is √denom, AND THE EIGHT GAINS ARE UNTOUCHED.
    //
    //      Both halves matter. The first is the feature; the second is the promise that P69's
    //      defaulted out-param changed nothing on the audio path. Bit-identity rather than a
    //      tolerance, because "1/k = √denom" is an identity and the eight gains are k·w·t either
    //      way — an epsilon here would hide exactly the refactor this probe exists to catch.
    {
        const oo::VenueModel v;
        const SolveRig rig (v);

        bool         ok = true;
        juce::String detail;

        const std::array<oo::Vec3, 4> points {{
            { 6.5f, 12.0f, 0.0f }, { 2.0f,  6.0f, 1.2f },
            { 11.0f, 17.0f, 0.4f }, { 6.5f, 12.4625f, 0.0f },
        }};

        float worstInvK = 0.0f;
        int   gainsDiffer = 0;

        for (const auto p : points)
        {
            const float a  = oo::dbap::rolloffToAlpha (4.0f);
            const float rs = oo::dbap::blurToRadius (0.10f, v.rigScale());

            std::array<float, 8> withPtr {};
            std::array<float, 8> withoutPtr {};
            float invK = -1.0f;

            oo::dbap::solve (rig.spk.data(), kUnitWeights.data(), p, a, rs, withPtr.data(), &invK);
            oo::dbap::solve (rig.spk.data(), kUnitWeights.data(), p, a, rs, withoutPtr.data());

            for (int i = 0; i < 8; ++i)
                if (! bitExact (withPtr[(size_t) i], withoutPtr[(size_t) i]))
                    ++gainsDiffer;

            // The INDEPENDENT half: denom from the §3.3 definition, in double, with its own pow.
            // Recomputing it through dbap::solve would be a comparison of a value with itself.
            double denom = 0.0;

            for (int i = 0; i < 8; ++i)
            {
                const auto s  = rig.spk[(size_t) i];
                const double dx = (double) s.x - (double) p.x;
                const double dy = (double) s.y - (double) p.y;
                const double dz = (double) s.z - (double) p.z;

                double d = std::sqrt (dx * dx + dy * dy + dz * dz + (double) rs * (double) rs);
                d = d < (double) oo::dbap::kMinDistance ? (double) oo::dbap::kMinDistance : d;

                const double t = std::pow (d, -(double) a);
                denom += t * t;   // w_i == 1 here
            }

            const float expected = (float) std::sqrt (denom);
            worstInvK = juce::jmax (worstInvK, std::abs (invK - expected));
        }

        ok = gainsDiffer == 0 && worstInvK < 1.0e-4f;

        detail << "outInvK vs an independent √denom: worst " << juce::String (worstInvK, 8)
               << "; gains vs the nullptr path: "
               << (gainsDiffer == 0 ? "BIT-IDENTICAL" : juce::String (gainsDiffer) + " DIFFER");

        // The all-zero-weight path writes 0.0f, not a sentinel and not uninitialised memory.
        {
            const std::array<float, 8> zeros {};
            std::array<float, 8> g {};
            float invK = -7.0f;

            oo::dbap::solve (rig.spk.data(), zeros.data(), { 6.5f, 12.0f, 0.0f },
                             oo::dbap::rolloffToAlpha (4.0f), 0.5f, g.data(), &invK);

            const bool zeroField = isExactlyZero (invK);
            ok = ok && zeroField;
            detail << "; all-zero weights -> invK " << (zeroField ? "EXACTLY 0" : "NOT ZERO");
        }

        check ("CA outinvk-is-sqrt-denom", ok, detail);
    }

    //==========================================================================
    // CB — UI-04/1: TWENTY GRID POINTS vs A DIRECT SOLVE, TO 1e-3.
    //
    //      THE ORACLE IS INDEPENDENT, following probe K's precedent: the distances, the pow loop
    //      and the trim law are written out here in double precision rather than routed back
    //      through dbap::solve. An oracle that reran the implementation's own formula would
    //      reproduce its errors and pass forever (pattern_test_fixture_mirrors_drift_silently) —
    //      and it would turn "compared against a direct solve" into a comparison of two copies of
    //      the same mistake.
    //
    //      ASSERTED ON THE FLOAT FIELD, strictly upstream of the 8-bit transport quantisation
    //      (P73). The bytes the page receives are a rendering of this; the criterion is about
    //      this.
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());

        const auto snap = makeSnapshot (v, h);

        // A non-uniform weight set and a non-zero hullAtten, so BOTH ends of the chain are live:
        // uniform weights would hide a mis-indexed w_i, and hullAtten = 0 makes hullTrimGain
        // bit-exact unity and the outside-hull half of the chain untested (RESEARCH-2.3 Q5).
        const std::array<float, 8> w { 1.0f, 0.7f, 0.0f, 0.35f, 1.0f, 0.15f, 0.9f, 0.5f };
        const float rolloff   = 4.5f;
        const float blur      = 0.22f;
        const float hullAtten = 1.4f;

        const float a  = oo::dbap::rolloffToAlpha (rolloff);
        const float rs = oo::dbap::blurToRadius (blur, v.rigScale());

        oo::FieldSampler sampler;
        oo::instr::resetCounters();                 // P74 qualification 1 — see CE.
        const auto field = sampler.sample (snap, w.data(), a, rs, hullAtten);

        double worst = 0.0;
        int    worstCell = -1;
        int    outsideHullPoints = 0;

        // ── CHOOSING THE TWENTY, AND WHY IT IS NOT A BARE STRIDE ──────────────────────────────
        //
        // The first version of this probe took 20 cells on a stride coprime with the row length.
        // IT FAILED — not on the arithmetic, which agreed to 4e-8, but on its own non-vacuity
        // guard: NOT ONE of the twenty landed outside the hull, so `hullTrimGain` was multiplying
        // by bit-exact unity at every sampled point and half the chain was untested.
        //
        // The §OQ4 hull is a HEXAGON inside a rectangular bbox — speakers 3 and 8 are ON_EDGE, so
        // the only outside-hull region is the pair of small triangles cut off the REAR corners by
        // edges 4->5 and 6->7. A uniform stride simply misses them.
        //
        // So the sample set is built in two parts: fourteen spread across the whole grid, and six
        // drawn evenly from the cells that are actually outside. That is not a weaker probe than a
        // bare stride — it is the same twenty points covering BOTH arms of the chain, which is what
        // the guard was asking for.
        std::vector<int> outsideCells;

        for (int idx = 0; idx < oo::FieldSampler::kNumCells; ++idx)
        {
            const int r = idx / oo::FieldSampler::kCols;
            const int c = idx % oo::FieldSampler::kCols;
            const float px = v.bbMinX() + (((float) c + 0.5f) / (float) oo::FieldSampler::kCols)
                                              * (v.bbMaxX() - v.bbMinX());
            const float py = v.bbMinY() + (((float) r + 0.5f) / (float) oo::FieldSampler::kRows)
                                              * (v.bbMaxY() - v.bbMinY());

            if (! oo::hull::isInside (snap.hullPts.data(), snap.hullCount, { px, py }, snap.hullEpsCross))
                outsideCells.push_back (idx);
        }

        std::vector<int> sampleCells;

        for (int n = 0; n < 14; ++n)
            sampleCells.push_back ((n * 61) % oo::FieldSampler::kNumCells);

        for (int n = 0; n < 6 && ! outsideCells.empty(); ++n)
            sampleCells.push_back (outsideCells[(std::size_t) ((n * (int) outsideCells.size()) / 6)]);

        for (const int idx : sampleCells)
        {
            const int row = idx / oo::FieldSampler::kCols;
            const int col = idx % oo::FieldSampler::kCols;

            const float tx = ((float) col + 0.5f) / (float) oo::FieldSampler::kCols;
            const float ty = ((float) row + 0.5f) / (float) oo::FieldSampler::kRows;

            const float x = v.bbMinX() + tx * (v.bbMaxX() - v.bbMinX());
            const float y = v.bbMinY() + ty * (v.bbMaxY() - v.bbMinY());
            const float z = v.earHeight (y);

            // The hull step. hull::project is reused rather than re-derived — probe K already
            // validates it against a ternary-search oracle, so a second oracle here would test
            // nothing new and would be the third implementation of one thing.
            oo::Vec3 solveAt { x, y, z };
            double   dHull = 0.0;

            if (! oo::hull::isInside (snap.hullPts.data(), snap.hullCount, { x, y }, snap.hullEpsCross))
            {
                const auto proj = oo::hull::project (snap.hullPts.data(), snap.hullCount, { x, y });
                solveAt = { proj.point.x, proj.point.y, z };
                dHull   = (double) proj.distance;
                ++outsideHullPoints;
            }

            // §3.3, from the definition, in double.
            double denom = 0.0;

            for (int i = 0; i < 8; ++i)
            {
                const auto s = snap.spk[(size_t) i];
                const double dx = (double) s.x - (double) solveAt.x;
                const double dy = (double) s.y - (double) solveAt.y;
                const double dz = (double) s.z - (double) solveAt.z;

                double d = std::sqrt (dx * dx + dy * dy + dz * dz + (double) rs * (double) rs);
                d = d < (double) oo::dbap::kMinDistance ? (double) oo::dbap::kMinDistance : d;

                const double wt = (double) w[(size_t) i] * std::pow (d, -(double) a);
                denom += wt * wt;
            }

            // §3.5.1, from the definition: dB per metre, floored at −24 dB.
            const double attenDb  = juce::jmax (-(double) hullAtten * dHull, -24.0);
            const double expected = std::sqrt (denom) * std::pow (10.0, attenDb * 0.05);

            const double got  = (double) field.cell[(size_t) idx];
            const double diff = std::abs (got - expected);

            if (diff > worst)
            {
                worst = diff;
                worstCell = idx;
            }
        }

        // NON-VACUITY: if every sampled point were inside the hull, the trim would be unity
        // everywhere and half the chain would be untested.
        const bool exercisedTrim = outsideHullPoints > 0;
        const bool withinTol     = worst < 1.0e-3;

        juce::String detail;
        detail << sampleCells.size() << " of " << oo::FieldSampler::kNumCells
               << " cells vs an independent solve: worst |Δ| "
               << juce::String (worst, 9) << " at cell " << worstCell
               << " (tol 1e-3); " << outsideHullPoints << " of them outside the hull"
               << " (" << (int) outsideCells.size() << " outside cells in the grid)"
               << (exercisedTrim ? "" : " — TRIM PATH NOT EXERCISED, THIS PROBE IS HALF VACUOUS")
               << "; span " << juce::String (field.spanDb, 2) << " dB";

        check ("CB field-vs-direct-solve-1e-3", withinTol && exercisedTrim, detail);
    }

    //==========================================================================
    // CC — UI-04/2: THE FIELD IS BITWISE UNCHANGED ACROSS srcX / srcY / srcZ / width.
    //
    //      The sampler's signature admits no source at all, so this is structurally true — and
    //      that is the assertion. It is written as a MEASUREMENT rather than left as a remark
    //      because the structural property is the thing that keeps UI-04/2's puck clause honest,
    //      and a future "improvement" that fed the puck in would break it silently.
    //
    //      THE NON-VACUITY HALF IS THE SUB-POINTS. shaper::shape() IS driven across the same sweep
    //      and its output MUST differ, or this probe would be asserting that two identical inputs
    //      produce identical outputs.
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());

        const auto snap = makeSnapshot (v, h);

        const std::array<float, 8> w { 1.0f, 0.7f, 0.2f, 0.35f, 1.0f, 0.15f, 0.9f, 0.5f };
        const float a  = oo::dbap::rolloffToAlpha (4.0f);
        const float rs = oo::dbap::blurToRadius (0.15f, v.rigScale());

        oo::FieldSampler sampler;
        const auto reference = sampler.sample (snap, w.data(), a, rs, 1.0f);

        struct SourceState { float nx, ny, z, width; };

        const std::array<SourceState, 5> sweep {{
            { 0.50f, 0.50f,  0.0f, 0.0f },
            { 0.05f, 0.95f,  3.5f, 0.0f },
            { 0.95f, 0.05f, -1.5f, 4.0f },
            { 0.25f, 0.75f,  8.0f, 6.0f },
            { 0.80f, 0.30f, -2.0f, 2.5f },
        }};

        int    cellsDiffer = 0;
        int    distinctSubPoints = 0;
        oo::Vec3 lastLeft {};

        for (std::size_t k = 0; k < sweep.size(); ++k)
        {
            const auto& s = sweep[k];

            // The four "inputs" that are not inputs, driven through the function that DOES
            // consume them, so the sweep is demonstrably non-trivial.
            const auto sub = oo::shaper::shape (snap, s.nx, s.ny, s.z, s.width);

            if (k > 0 && (! bitExact (sub.left.x, lastLeft.x) || ! bitExact (sub.left.y, lastLeft.y)
                          || ! bitExact (sub.left.z, lastLeft.z)))
                ++distinctSubPoints;

            lastLeft = sub.left;

            const auto again = sampler.sample (snap, w.data(), a, rs, 1.0f);

            for (int i = 0; i < oo::FieldSampler::kNumCells; ++i)
                if (! bitExact (again.cell[(size_t) i], reference.cell[(size_t) i]))
                    ++cellsDiffer;
        }

        const bool sweepIsReal = distinctSubPoints == (int) sweep.size() - 1;
        const bool unchanged   = cellsDiffer == 0;

        juce::String detail;
        detail << "5 source states x " << oo::FieldSampler::kNumCells << " cells: "
               << (unchanged ? "BITWISE IDENTICAL" : juce::String (cellsDiffer) + " CELLS DIFFER")
               << "; the sweep moved the sub-points " << distinctSubPoints << "/4 times"
               << (sweepIsReal ? "" : " — SWEEP IS INERT, THIS PROBE IS VACUOUS");

        check ("CC field-ignores-source", unchanged && sweepIsReal, detail);
    }

    //==========================================================================
    // CD — UI-04/2: THE RECOMPUTE COUNTER, AND THAT ALL FIVE INPUTS ARE REAL INPUTS.
    //
    //      Two claims, and they are different. The counter half is C++-side because D19's rule —
    //      assert against what C++ returned — applies here as much as to scene membership. The
    //      five-inputs half is what makes the CALLER's coalescing correct: a page that refreshed
    //      on geometry and weights only, as UI-04/2 literally reads, would leave the picture stale
    //      through a rolloff, blur or hullAtten move (N12). Each is driven individually, so a
    //      single one going inert is named rather than absorbed.
    //
    //      The other half of UI-04/2 — that a PUCK DRAG does not move the count — is layout §27,
    //      which counts getFieldGrid INVOCATIONS across N frames of a real drag.
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());
        const auto snapA = makeSnapshot (v, h);

        const std::array<float, 8> wBase { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        const float aBase  = oo::dbap::rolloffToAlpha (4.0f);
        const float rsBase = oo::dbap::blurToRadius (0.10f, v.rigScale());

        oo::FieldSampler sampler;

        const bool startsAtZero = sampler.recomputeCount() == 0;

        const auto base = sampler.sample (snapA, wBase.data(), aBase, rsBase, 1.0f);
        const bool oneAfterOne = sampler.recomputeCount() == 1;

        const auto differsFrom = [&base] (const oo::FieldSampler::Field& f)
        {
            for (int i = 0; i < oo::FieldSampler::kNumCells; ++i)
                if (! bitExact (f.cell[(size_t) i], base.cell[(size_t) i]))
                    return true;

            return false;
        };

        // 1. Speaker positions.
        oo::VenueModel moved;
        const auto p2 = moved.speaker (2);
        moved.setSpeakerPosition (2, { p2.x - 3.0f, p2.y + 1.5f, p2.z });
        oo::ConvexHull2D h2;
        h2.build (moved.speakerPositions());
        const bool geomMoves = differsFrom (
            sampler.sample (makeSnapshot (moved, h2), wBase.data(), aBase, rsBase, 1.0f));

        // 2. The eight weights.
        const std::array<float, 8> wAlt { 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
        const bool weightsMove = differsFrom (sampler.sample (snapA, wAlt.data(), aBase, rsBase, 1.0f));

        // 3. rolloff -> a.
        const bool rolloffMoves = differsFrom (
            sampler.sample (snapA, wBase.data(), oo::dbap::rolloffToAlpha (6.0f), rsBase, 1.0f));

        // 4. blur -> r_s.
        const bool blurMoves = differsFrom (
            sampler.sample (snapA, wBase.data(), aBase,
                            oo::dbap::blurToRadius (0.90f, v.rigScale()), 1.0f));

        // 5. hullAtten -> the trim. Only visible OUTSIDE the hull, which the grid reaches because
        //    the §OQ4 hull is a hexagon inside a rectangular bbox — the four clipped corners are
        //    sampled and are where this input lives.
        const bool attenMoves = differsFrom (sampler.sample (snapA, wBase.data(), aBase, rsBase, 3.0f));

        const bool sixCalls = sampler.recomputeCount() == 6;

        const int liveInputs = (geomMoves ? 1 : 0) + (weightsMove ? 1 : 0) + (rolloffMoves ? 1 : 0)
                             + (blurMoves ? 1 : 0) + (attenMoves ? 1 : 0);

        juce::String detail;
        detail << "counter 0 -> 1 -> 6: "
               << (startsAtZero && oneAfterOne && sixCalls ? "exact" : "WRONG")
               << " (" << (int) sampler.recomputeCount() << "); live inputs " << liveInputs << "/5"
               << " [geom " << (geomMoves ? "Y" : "N")
               << " w " << (weightsMove ? "Y" : "N")
               << " rolloff " << (rolloffMoves ? "Y" : "N")
               << " blur " << (blurMoves ? "Y" : "N")
               << " hullAtten " << (attenMoves ? "Y" : "N") << "]";

        check ("CD field-recompute-counter", startsAtZero && oneAfterOne && sixCalls && liveInputs == 5,
               detail);
    }

    //==========================================================================
    // CE — PERF-02: THE resetCounters() DISCIPLINE, ASSERTED RATHER THAN REMEMBERED.
    //
    //      Under OOCTAGON_INSTRUMENT a field sample routes through countedPow 8 times per cell —
    //      10,240 calls for one 32 x 40 grid — and probe AE asserts powCalls == 16 EXACTLY per
    //      solve pair. A probe inserted between a field sample and AE without a reset would
    //      SILENTLY INFLATE AE's figure into a failure whose cause is 900 lines away.
    //
    //      The render harness already calls resetCounters() at eleven sites, so this is a
    //      convention to follow rather than one to invent. THIS PROBE MAKES IT FAIL LOUDLY: it
    //      demonstrates the pollution is real, and then that a reset restores the exact figure.
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());
        const auto snap = makeSnapshot (v, h);

        const SolveRig rig (v);
        oo::FieldSampler sampler;

        // (a) POLLUTION IS REAL. Without this half the probe could pass on a build where the
        //     counter was never incremented at all.
        oo::instr::resetCounters();
        (void) sampler.sample (snap, kUnitWeights.data(), oo::dbap::rolloffToAlpha (4.0f),
                               oo::dbap::blurToRadius (0.10f, v.rigScale()), 1.0f);

        const auto afterField = oo::instr::get (oo::instr::powCalls);

        (void) rig.solve ({ 4.0f, 9.0f, 1.10f }, 4.0f, 0.10f, kUnitWeights);
        (void) rig.solve ({ 4.0f, 9.0f, 1.10f }, 4.0f, 0.10f, kUnitWeights);

        const auto unreset = oo::instr::get (oo::instr::powCalls);

        // (b) AND A RESET RESTORES AE's EXACT FIGURE.
        oo::instr::resetCounters();

        (void) rig.solve ({ 4.0f, 9.0f, 1.10f }, 4.0f, 0.10f, kUnitWeights);
        (void) rig.solve ({ 4.0f, 9.0f, 1.10f }, 4.0f, 0.10f, kUnitWeights);

        const auto afterReset = oo::instr::get (oo::instr::powCalls);

        // solveRuns is NOT at risk and is asserted so: countSolveRun() lives in
        // GainStage::updateControl, not inside dbap::solve, so a field sample cannot touch it.
        const auto solveRuns = oo::instr::get (oo::instr::solveRuns);

        const bool pollutes   = afterField == (std::uint64_t) (8 * oo::FieldSampler::kNumCells);
        const bool wouldBreak = unreset != 16;
        const bool restored   = afterReset == 16;
        const bool runsClean  = solveRuns == 0;

        juce::String detail;
        detail << "one field sample = " << (int) afterField << " pow ("
               << (pollutes ? "8 per cell, as expected" : "UNEXPECTED") << "); AE unreset would read "
               << (int) unreset << (wouldBreak ? " (it WOULD break)" : " — NO POLLUTION, VACUOUS")
               << "; after resetCounters() " << (int) afterReset
               << (restored ? " (== 16)" : " — NOT 16") << "; solveRuns "
               << (int) solveRuns << (runsClean ? " (untouched, as it must be)" : " — POLLUTED");

        check ("CE resetcounters-discipline", pollutes && wouldBreak && restored && runsClean, detail);

        // Leave the counters as this file's later readers expect to find them.
        oo::instr::resetCounters();
    }

    //==========================================================================
    // CF — FUNC-06/2: THE SIX NAMED SETS ON THE DEFAULT VENUE, EXACTLY D16's TABLE.
    //
    //      The table is written out here rather than recomputed, deliberately: it is the
    //      DECISION's own content (CONTEXT-3.3 D16, evaluated at RESEARCH-3.3), and a probe that
    //      recomputed it would agree with any predicate whatsoever.
    //
    //      Note SIDES = {3,4,7,8} and NOT {1,2,3,4,7,8}: speakers 1 and 2 miss by 6.2 %
    //      (1.0617 against 1.0000). That is a property of this hall, not a defect, and FUNC-06/3's
    //      show-before-commit is what makes a 6 % margin visible instead of silent.
    {
        oo::VenueModel v;
        oo::ConvexHull2D h;
        h.build (v.speakerPositions());

        struct Expect { oo::scenes::Named which; const char* set; };

        // 1-BASED, matching every human-facing surface in this plugin.
        const std::array<Expect, 6> table {{
            { oo::scenes::Named::all,   "1,2,3,4,5,6,7,8" },
            { oo::scenes::Named::front, "1,2,3,8" },
            { oo::scenes::Named::rear,  "4,5,6,7" },
            { oo::scenes::Named::left,  "1,6,7,8" },
            { oo::scenes::Named::right, "2,3,4,5" },
            { oo::scenes::Named::sides, "3,4,7,8" },
        }};

        const auto asString = [] (const oo::scenes::Membership& m)
        {
            juce::String s;

            for (int i = 0; i < 8; ++i)
                if (m.in[(size_t) i])
                    s << (s.isEmpty() ? "" : ",") << (i + 1);

            return s;
        };

        bool         ok = true;
        juce::String detail;

        for (const auto& e : table)
        {
            const auto m   = oo::scenes::resolve (e.which, v.speakerPositions(), h);
            const auto got = asString (m);
            const bool hit = got == e.set;

            ok = ok && hit && m.count == got.retainCharacters (",").length() + 1;

            detail << oo::scenes::name (e.which) << " {" << got << "}"
                   << (hit ? "" : " EXPECTED {" + juce::String (e.set) + "}") << "; ";
        }

        // ON_EDGE, NOT VERTEX. Speakers 3 and 8 are on a hull EDGE and both belong to SIDES; a
        // predicate reading `== VERTEX` drops them and still looks plausible.
        const bool threeOnEdge = h.classify (2) == oo::ConvexHull2D::Classification::ON_EDGE;
        const bool eightOnEdge = h.classify (7) == oo::ConvexHull2D::Classification::ON_EDGE;
        const auto sides = oo::scenes::resolve (oo::scenes::Named::sides, v.speakerPositions(), h);

        ok = ok && threeOnEdge && eightOnEdge && sides.in[2] && sides.in[7];

        detail << "3 and 8 are ON_EDGE and IN sides: "
               << (threeOnEdge && eightOnEdge && sides.in[2] && sides.in[7] ? "yes" : "NO");

        check ("CF named-scenes-default-venue", ok, detail);
    }

    //==========================================================================
    // CG — FUNC-06/2: THE PERMUTATION. **THE PROBE THAT CARRIES THIS HALF OF THE PHASE.**
    //
    //      The same eight PHYSICAL positions, with the INDICES rotated by k. The centroid and the
    //      bounding box are unchanged, so the physically-front speakers are the same four points —
    //      but they now sit at different indices, and FRONT must return the indices that NOW hold
    //      y < cy.
    //
    //      A FIXED-INDEX IMPLEMENTATION RETURNS {1,2,3,8} FOR EVERY k AND FAILS HERE. Every other
    //      scene probe in this file — CF's table, CH's empty set — PASSES under that defect, which
    //      is what makes this the guard rather than additional coverage. NC3 fires it.
    {
        const oo::VenueModel base;
        const auto basePositions = base.speakerPositions();

        oo::ConvexHull2D baseHull;
        baseHull.build (basePositions);

        // The physically-front speakers, 0-based, as CF pinned them.
        const std::array<int, 4> frontBase { 0, 1, 2, 7 };

        bool         ok = true;
        juce::String detail;
        int          fixedIndexWouldPass = 0;

        for (int k = 1; k < 8; ++k)
        {
            // rotated[i] = base[(i + k) % 8]. The speaker physically at base index j therefore
            // sits at index (j - k + 8) % 8 after the rotation.
            std::array<oo::Vec3, 8> rotated {};

            for (int i = 0; i < 8; ++i)
                rotated[(size_t) i] = basePositions[(size_t) ((i + k) % 8)];

            oo::ConvexHull2D rotatedHull;
            rotatedHull.build (rotated);

            std::array<bool, 8> expected {};
            for (int j : frontBase)
                expected[(size_t) ((j - k + 8) % 8)] = true;

            const auto got = oo::scenes::resolve (oo::scenes::Named::front, rotated, rotatedHull);

            bool match = true;
            for (int i = 0; i < 8; ++i)
                if (got.in[(size_t) i] != expected[(size_t) i])
                    match = false;

            // Would a FIXED-INDEX implementation have passed this rotation? If it would have for
            // every k, the probe is not the guard it claims to be.
            bool fixedMatches = true;
            for (int i = 0; i < 8; ++i)
            {
                const bool fixed = i == 0 || i == 1 || i == 2 || i == 7;
                if (fixed != expected[(size_t) i])
                    fixedMatches = false;
            }

            if (fixedMatches)
                ++fixedIndexWouldPass;

            ok = ok && match;

            if (! match)
            {
                detail << "k=" << k << " MISMATCH {";
                for (int i = 0; i < 8; ++i)
                    if (got.in[(size_t) i])
                        detail << (i + 1) << " ";
                detail << "}; ";
            }
        }

        // NON-VACUITY, stated as a number: a fixed-index implementation must fail at least one
        // rotation, or this probe cannot be the guard NC3 relies on.
        const bool guards = fixedIndexWouldPass < 7;

        detail << (ok ? "all 7 rotations track the geometry" : "")
               << "; a FIXED-INDEX impl would pass " << fixedIndexWouldPass << "/7"
               << (guards ? " — this probe IS the guard" : " — VACUOUS, IT GUARDS NOTHING");

        check ("CG scene-permutation", ok && guards, detail);
    }

    //==========================================================================
    // CH — FUNC-06/3: THE EMPTY SET, ON A PHYSICALLY PLAUSIBLE RIG.
    //
    //      A PROSCENIUM rig — four corners plus two points on each of the front and rear edges,
    //      and no side fills. Every speaker is NON-INTERIOR and the venue is non-degenerate, yet
    //      SIDES is empty: the corners score 1.000 against 1.000 and the edge points 0.333 against
    //      1.000, and the predicate is strict.
    //
    //      That it is a rig somebody might actually hang is what makes it a fair test of D20
    //      rather than a contrived one. An empty scene is DSP-05's silence one click away, which
    //      is why applyScene refuses it in C++ and not only in a disabled control.
    {
        const std::array<oo::Vec3, 8> proscenium {{
            {  0.0f,  0.0f, 5.0f },   // corners
            { 12.0f,  0.0f, 5.0f },
            { 12.0f, 16.0f, 5.4f },
            {  0.0f, 16.0f, 5.4f },
            {  4.0f,  0.0f, 5.0f },   // front edge fills
            {  8.0f,  0.0f, 5.0f },
            {  4.0f, 16.0f, 5.4f },   // rear edge fills
            {  8.0f, 16.0f, 5.4f },
        }};

        oo::ConvexHull2D h;
        h.build (proscenium);

        const auto sides = oo::scenes::resolve (oo::scenes::Named::sides, proscenium, h);
        const auto front = oo::scenes::resolve (oo::scenes::Named::front, proscenium, h);
        const auto all   = oo::scenes::resolve (oo::scenes::Named::all,   proscenium, h);

        int interior = 0;
        for (int i = 0; i < 8; ++i)
            if (h.classify (i) == oo::ConvexHull2D::Classification::INTERIOR)
                ++interior;

        const bool emptySides   = sides.isEmpty() && sides.count == 0;
        const bool allNonInterior = interior == 0;      // the set is empty for the RIGHT reason
        const bool othersLive   = ! front.isEmpty() && ! all.isEmpty();

        juce::String detail;
        detail << "proscenium rig: SIDES count " << sides.count
               << (emptySides ? " (EMPTY, as designed)" : " — EXPECTED EMPTY")
               << "; INTERIOR speakers " << interior
               << (allNonInterior ? " (so it is empty by the |dx|/hx test, not by classification)"
                                  : " — SOME ARE INTERIOR, THE SET IS EMPTY FOR THE WRONG REASON")
               << "; FRONT " << front.count << ", ALL " << all.count
               << (othersLive ? "" : " — THE WHOLE PREDICATE IS DEAD, THIS PROBE IS VACUOUS");

        check ("CH empty-scene-proscenium", emptySides && allNonInterior && othersLive, detail);
    }

    //==========================================================================
    // ══════════════════════════════════════════════════════════════════════════════════════════
    // PHASE 4.1 — CO.
    // ══════════════════════════════════════════════════════════════════════════════════════════
    //
    // CO — COMPAT-04/3: the SAFE-mode partition, asserted as a FORM rather than as a behaviour.
    //
    //      Probe BM already drives all five layouts through a real prepareToPlay() and asserts
    //      isSafeMode() against each. What BM structurally CANNOT do is present a set that
    //      isBusesLayoutSupported() rejects — it reaches the predicate only through the host
    //      negotiation that filters those sets out. So BM proves the WIRING and can never prove
    //      the FORM, and on today's five sets the shipped complement spelling and the rejected
    //      "== mono || == stereo" spelling are INDISTINGUISHABLE to it.
    //
    //      CO calls oo::rig::isRealRig directly, which is the only way to reach the disagreement.
    //      The two load-bearing rows are 7.1.4 and OCTAGONAL: both are 8-or-more-channel sets that
    //      are NOT one of the three real rigs, so the shipped form reports them SAFE (banner UP —
    //      an unmapped rig folds and says so) while the rejected form would report them REAL
    //      (banner DOWN, silently, on a container nobody has mapped). Octagonal is the sharper of
    //      the two: isBusesLayoutSupported()'s own comment names it as an 8-channel candidate JUCE
    //      offers and Logic ignores.
    //
    //      NEGATIVE CONTROL NC1 (PLAN-4.1 P98) is what makes the pairing a measurement rather than
    //      an assertion: rewrite isRealRig as the rejected spelling and THIS probe fails while BM
    //      still passes.
    {
        struct Case { juce::AudioChannelSet set; bool expectReal; const char* label; bool discriminating; };

        const Case cases[] = {
            // The three real rigs — the only three 8-channel containers Logic exposes.
            { juce::AudioChannelSet::create7point1(),       true,  "7.1",        false },
            { juce::AudioChannelSet::create7point1SDDS(),   true,  "7.1-SDDS",   false },
            { juce::AudioChannelSet::create5point1point2(), true,  "5.1.2",      false },

            // The two rows the whole probe exists for: >= 8 channels, NOT a real rig, so SAFE.
            // Under "== mono || == stereo" both of these would read REAL.
            { juce::AudioChannelSet::create7point1point4(), false, "7.1.4",      true  },
            { juce::AudioChannelSet::octagonal(),           false, "octagonal",  true  },

            // Agreed by both spellings — present so the probe covers the partition, not so it
            // discriminates.
            { juce::AudioChannelSet::quadraphonic(),        false, "quad",       false },
            { juce::AudioChannelSet::mono(),                false, "mono",       false },
            { juce::AudioChannelSet::stereo(),              false, "stereo",     false },
        };

        bool         ok = true;
        bool         discriminatorsLive = false;
        juce::String detail;

        for (const auto& c : cases)
        {
            const bool real = oo::rig::isRealRig (c.set);

            if (real != c.expectReal)
            {
                ok = false;

                if (c.discriminating)
                    detail << "*** " << c.label << " (" << c.set.size() << "ch) READ AS "
                           << (real ? "REAL — BANNER DOWN ON AN UNMAPPED RIG; this is the "
                                      "\"== mono || == stereo\" spelling"
                                    : "SAFE, EXPECTED REAL")
                           << " *** ";
                else
                    detail << c.label << ":" << (real ? "REAL" : "SAFE") << "(WRONG) ";
            }
            else
            {
                if (c.discriminating)
                    discriminatorsLive = true;

                detail << c.label << ":" << (real ? "REAL" : "SAFE")
                       << (c.discriminating ? "*" : "") << " ";
            }
        }

        // The two discriminating rows must have been EXERCISED, not merely present: if JUCE ever
        // stopped offering 7.1.4 or octagonal, this probe would quietly become the same
        // indiscriminate check BM already is.
        const bool eightOrMore = juce::AudioChannelSet::create7point1point4().size() >= 8
                              && juce::AudioChannelSet::octagonal().size() >= 8;

        if (! eightOrMore)
            detail << "— 7.1.4/octagonal ARE NOT >= 8 CHANNELS, THE DISCRIMINATING ROWS ARE VACUOUS";

        detail << "(* = fails under the rejected spelling)";

        check ("CO rig-policy-complement-form", ok && discriminatorsLive && eightOrMore, detail);
    }

    scratchDir.deleteRecursively();

    //==========================================================================
    std::printf ("\n----------------------------------------------------------\n");
    std::printf ("  %d probe(s), %d failure(s)\n\n", probes, failures);

    return failures == 0 ? 0 : 1;
}
