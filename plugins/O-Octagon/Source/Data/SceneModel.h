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

#include <juce_data_structures/juce_data_structures.h>

#include <array>

#include "../DSP/ConvexHull2D.h"
#include "../DSP/Vec.h"

namespace oo
{

//==============================================================================
/**
    ARCHITECTURE §6.3 — the six NAMED weight scenes, and the four USER slots.

    ── ONE IMPLEMENTATION, TWO CONSUMERS (PLAN-3.3 P79 / CONTEXT-3.3 D19) ────────────────────────
    The predicate lives here as a PURE FUNCTION so the fast unit target can test it (probes CF, CG,
    CH) and the editor's `applyScene` can consult the same function. There is deliberately no second
    implementation anywhere — and specifically NONE IN JAVASCRIPT. A JS re-derivation would be a
    mirrored fixture (`pattern_test_fixture_mirrors_drift_silently`) over R1, the highest-risk
    component in the project, and it is exactly what makes FUNC-06 criterion 2's PERMUTATION probe
    meaningful: rotate the eight indices against the same eight physical positions and `front` must
    return the indices that NOW hold `y < cy`. A fixed-index implementation returns {1,2,3,8} and
    FAILS. `ui_frontend_check.js` §32 asserts the JS absence statically.

    ── NO PROCESSOR REFERENCE, AND NO VenueModel ─────────────────────────────────────────────────
    Raw positions plus a built hull, following DbapSolver's P19 precedent one layer up: it is what
    lets this TU join the unit target whose link line is juce_audio_basics + juce_core +
    juce_data_structures, and what lets the permutation fixture be eight coordinates rather than a
    constructed venue.
*/
namespace scenes
{
    inline constexpr int kNumSpeakers = ConvexHull2D::kNumSpeakers;

    /** The four USER slots. Four, not eight: they are a concert-desk convenience, and the six named
        scenes already cover every axis split the geometry admits. */
    inline constexpr int kNumUserSlots = 4;

    //==========================================================================
    /** The six named scenes, in the order the UI lays them out. */
    enum class Named
    {
        all = 0,
        front,
        rear,
        left,
        right,
        sides,
        kCount
    };

    inline constexpr int kNumNamed = static_cast<int> (Named::kCount);

    /** The wire id — "ALL", "FRONT", "REAR", "LEFT", "RIGHT", "SIDES".

        PURE ASCII, deliberately: it crosses the bridge through juce::String, whose `const char*`
        constructor converts via CharPointer_ASCII and mangles every byte above 127 with no compiler
        warning at all (`critical_juce_string_char_ctor_is_ascii_only`). Any human-readable copy is
        built in JS, where a non-ASCII character costs nothing.
    */
    const char* name (Named which) noexcept;

    /** Wire id → Named. False (and `out` untouched) for anything else, INCLUDING the "slot1".."slot4"
        ids, which are not named scenes and are resolved from the store instead. */
    bool parse (const juce::String& id, Named& out) noexcept;

    //==========================================================================
    /** Which speakers a scene resolves to, on a specific room. */
    struct Membership
    {
        std::array<bool, kNumSpeakers> in {};
        int count { 0 };

        /** D20's predicate. An empty set is DSP-05's silence path, so it is legible and not
            writable — the UI disables the control and `applyScene` refuses it, and both exist
            because the affordance is not the guarantee. */
        bool isEmpty() const noexcept { return count == 0; }
    };

    //==========================================================================
    /** D16's predicate, evaluated on the measured geometry.

        `classify(i) != INTERIOR  ∧  |x−cx|/hx > |y−cy|/hy`, with `(cx,cy)` the SPEAKER CENTROID and
        `(hx,hy)` the BBOX HALF-SPANS. The half-span normalisation is what makes "off the sides"
        mean anything on a rectangular hall — RESEARCH-3.1 N3 proved the unqualified phrasing admits
        several readings that disagree on real rigs.

        On the §OQ4 default venue:

            ALL   {1,2,3,4,5,6,7,8}      LEFT  {1,6,7,8}
            FRONT {1,2,3,8}              RIGHT {2,3,4,5}
            REAR  {4,5,6,7}              SIDES {3,4,7,8}

        Speakers 1 and 2 MISS SIDES BY 6.2 % (1.0617 against 1.0000). That is a property of this
        hall rather than a defect, and FUNC-06 criterion 3's show-before-commit is what makes a 6 %
        margin VISIBLE INSTEAD OF SILENT.

        `!= INTERIOR` AND NOT `== VERTEX`: speakers 3 and 8 are ON_EDGE on the default venue —
        three collinear speakers at x = 12.50 and three at x = 0.50 make the hull a hexagon — and
        both belong to SIDES.

        @param which  the scene
        @param spk    the eight positions, metres
        @param hull   a hull BUILT FROM `spk`. Passing one built from different positions would
                      classify the wrong speakers; the two always travel together at both call
                      sites (the processor owns them side by side, the probes build them together).
    */
    Membership resolve (Named which,
                        const std::array<Vec3, kNumSpeakers>& spk,
                        const ConvexHull2D& hull) noexcept;

    /** A membership as the eight weights it writes: 1.0 for a member, EXACTLY 0.0f otherwise.

        The zeros are exact because DSP-05/1 requires `w_i == 0` to produce a bit-exact zero gain at
        speaker i, and a 1e-7 residue there is an inaudible-but-real leak into a speaker the
        operator has just told the plugin to silence.
    */
    std::array<float, kNumSpeakers> weightsFor (const Membership& m) noexcept;
}

//==============================================================================
/**
    The four USER slots, as a `SCENES` child of `apvts.state`.

    ── SIBLING OF `VENUE`, NEVER A CHILD (CONTEXT-3.3 D17 / PLAN-3.3 P80) ────────────────────────
    `getStateInformation` is `apvts.copyState()` → XML, so a `SCENES` child of `apvts.state` is
    persisted and restored WITH NO NEW CODE — FUNC-06 criterion 4's session round-trip is
    STRUCTURAL rather than disciplined. And because `OuariconPresetManager::applyPresetJson`
    iterates `processor.getParameters()` only and can never walk `apvts.state`'s children, a musical
    preset physically cannot reach these slots either.

    ── WHAT IS *NOT* FREE, AND HAS A `VENUE` PRECEDENT TO COPY (RESEARCH-3.3 N13) ────────────────
    THE NORMALISATION. `setStateInformation` calls `venue.writeToState (apvts.state)` after
    restoring, so a missing or partial node is written back complete and an older session is
    upgraded EXACTLY ONCE. `SCENES` needs the identical treatment at the identical two points, or
    every session written before Phase 3.3 restores with no `SCENES` node at all and the four slots
    read as ABSENT rather than EMPTY. Probe CK drives a pre-3.3 session explicitly.

    Only the four user slots persist. The six named scenes are DERIVED ON DEMAND from the live
    geometry, so there is nothing here to go stale when the venue moves.
*/
class SceneStore
{
public:
    static constexpr int kNumSlots     = scenes::kNumUserSlots;
    static constexpr int kNumSpeakers  = scenes::kNumSpeakers;
    static constexpr int kSchemaVersion = 1;

    //==========================================================================
    // ValueTree schema, a sibling of VENUE under the "OOctagon" state root:
    //
    //   SCENES  @schemaVersion=1
    //   └── SLOT × 4  { @index, @occupied, @w1 .. @w8 }
    //
    // Attribute names are the on-disk contract for every session saved from here on and are as
    // load-bearing as VENUE's. They must not be renamed.

    static const juce::Identifier scenesTag;
    static const juce::Identifier slotTag;

    static const juce::Identifier propSchemaVersion;
    static const juce::Identifier propIndex;
    static const juce::Identifier propOccupied;

    /** `@w1`.. `@w8`, built from one table so the writer and the reader cannot disagree. */
    static const juce::Identifier& propWeight (int speakerIndex);

    //==========================================================================
    SceneStore() = default;

    /** Reads the SCENES child of `parentState`, if present. A missing OR PARTIAL node yields EMPTY
        slots — never an error, and never a slot that reads occupied with garbage weights. */
    void readFromState (const juce::ValueTree& parentState);

    /** Replaces (or creates) the SCENES child of `parentState`. THE NORMALISATION HALF of N13. */
    void writeToState (juce::ValueTree& parentState, juce::UndoManager* undoManager = nullptr) const;

    //==========================================================================
    bool isOccupied (int slot) const noexcept;

    /** The eight stored weights. All zeros for an unoccupied slot, which is why `isOccupied` is
        asked first at every call site rather than inferred from the vector. */
    std::array<float, kNumSpeakers> weights (int slot) const noexcept;

    /** D22's capture. Out-of-range slots are ignored rather than asserted — a UI click is not a
        programming error. Non-finite weights are dropped to 0.0f at ingestion, for the same reason
        `publishSnapshot()` sanitises the venue: a NaN here would reach `setValueNotifyingHost` and
        latch a SmoothedValue into permanent silence. */
    void capture (int slot, const std::array<float, kNumSpeakers>& w) noexcept;

private:
    struct Slot
    {
        bool occupied { false };
        std::array<float, kNumSpeakers> w {};
    };

    std::array<Slot, kNumSlots> slots {};
};

} // namespace oo
