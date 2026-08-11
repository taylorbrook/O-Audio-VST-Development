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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>

#include <array>

#include "../DSP/Vec.h"

namespace oo
{

/**
    The 42 venue values and every geometric quantity derived from them that does not change per
    audio block.

    Message thread only. The audio thread never sees this object — it reads the POD `VenueSnapshot`
    the processor publishes (ARCHITECTURE §3.6.6).

    The 42 values are 8 speakers × 5 (x, y, z, trimDb, label) plus `rakeFront` and `rakeRear`.
    They live in a ValueTree child of `apvts.state` rather than in the APVTS itself (§4.1 / OQ5):
    a musical preset physically cannot reach them, and they cannot be automated by a stray lane.

    This class has NO processor reference. That is what lets the unit-test target link it without
    PluginProcessor.cpp (PLAN-2.1 P7/D3).
*/
class VenueModel
{
public:
    //==============================================================================
    static constexpr int   kNumSpeakers   = 8;
    static constexpr int   kSchemaVersion = 1;

    /** Below this span the audience plane and the bbox denormalisation are both degenerate. Two
        separate guards use it — see earHeight() and normToMetres(). */
    static constexpr float kMinSpan       = 1.0e-6f;

    //==============================================================================
    // ValueTree schema (§4.1):
    //
    //   VENUE  @name @savedAt @schemaVersion=1 @rakeFront @rakeRear
    //   └── SPEAKER × 8  { @index, @x, @y, @z, @trimDb, @label }
    //
    // Attribute names are the on-disk contract for .venue files and for every session saved from
    // here on. They are as load-bearing as the "OOctagon" state root and must not be renamed.

    static const juce::Identifier venueTag;
    static const juce::Identifier speakerTag;

    static const juce::Identifier propName;
    static const juce::Identifier propSavedAt;
    static const juce::Identifier propSchemaVersion;
    static const juce::Identifier propRakeFront;
    static const juce::Identifier propRakeRear;

    static const juce::Identifier propIndex;
    static const juce::Identifier propX;
    static const juce::Identifier propY;
    static const juce::Identifier propZ;
    static const juce::Identifier propTrimDb;
    static const juce::Identifier propLabel;

    //==============================================================================
    /** Constructs the §OQ4 default venue with all derived quantities valid. */
    VenueModel();

    /** Restores every one of the 42 values to its §OQ4 default and recomputes. */
    void resetToDefaults();

    //==============================================================================
    /** Reads the VENUE child of `parentState`, if present.

        A missing OR PARTIAL node yields the §OQ4 defaults PER ATTRIBUTE — never zeros, never an
        error. Stage 1 sessions carry no VENUE child at all and must restore to the default venue
        silently; a session written by a future build with more attributes must not lose the ones
        it does carry. Missing SPEAKER children are likewise defaulted individually.

        Derived quantities are recomputed before returning.
    */
    void readFromState (const juce::ValueTree& parentState);

    /** Replaces (or creates) the VENUE child of `parentState` with the current venue. */
    void writeToState (juce::ValueTree& parentState, juce::UndoManager* undoManager = nullptr) const;

    /** The VENUE subtree on its own — the payload of a `.venue` file. File I/O itself is Stage 3. */
    juce::ValueTree toValueTree() const;

    //==============================================================================
    // Mutators. Message thread only; each recomputes the derived quantities. Out-of-range speaker
    // indices are ignored rather than asserted — a UI edit is not a programming error.

    void setSpeakerPosition (int speakerIndex, Vec3 positionMetres);
    void setSpeakerTrimDb   (int speakerIndex, float trimDecibels);
    void setSpeakerLabel    (int speakerIndex, const juce::String& abbreviation);
    void setRake            (float front, float rear);
    void setName            (const juce::String& newName);

    //==============================================================================
    // Stored values.

    Vec3         speaker (int speakerIndex) const noexcept;

    /** All eight positions at once — what ConvexHull2D::build() and the snapshot both want. */
    const std::array<Vec3, kNumSpeakers>& speakerPositions() const noexcept { return speakers; }

    float        trimDb  (int speakerIndex) const noexcept;

    /** trimDb converted to a linear factor for the snapshot. STORED here, but APPLIED nowhere in
        Phase 2.1 — trim application is FUNC-07 at Phase 2.3. */
    float        trimLin (int speakerIndex) const noexcept;

    juce::String labelAbbreviation (int speakerIndex) const;

    /** The label resolved to a ChannelType via AudioChannelSet::getChannelTypeFromAbbreviation.

        An unrecognised label resolves to `unknown`, which is a member of no accepted channel set,
        so getChannelIndexForType() returns −1 and the map build rejects it. A purely NUMERIC label
        is a sharper case: JUCE maps it to discreteChannel0 + n − 1 with no range check, producing
        a plausible-looking type that is still in no accepted set — it fails the same way, via the
        permutation check rather than via a parse error (RESEARCH-2.1 G4).
    */
    juce::AudioChannelSet::ChannelType labelType (int speakerIndex) const;

    std::array<juce::AudioChannelSet::ChannelType, kNumSpeakers> labelTypes() const;

    float rakeFront() const noexcept  { return rakeFrontM; }
    float rakeRear()  const noexcept  { return rakeRearM; }

    juce::String getName() const      { return name; }

    //==============================================================================
    // Derived. Recomputed on every edit; never on the audio thread.

    float bbMinX() const noexcept { return boxMinX; }
    float bbMaxX() const noexcept { return boxMaxX; }
    float bbMinY() const noexcept { return boxMinY; }
    float bbMaxY() const noexcept { return boxMaxY; }

    Vec3  centroid() const noexcept { return centroidM; }

    /** RMS speaker radius about the centroid, in metres — the paper's §3.1 normalisation of blur
        against the covariance of speaker distances from rig centre (DSP-08). Scales linearly with
        the room, which is the property the tests assert rather than the value. */
    float rigScale() const noexcept { return rigScaleM; }

    //==============================================================================
    /** Audience-plane ear height at a depth `yMetres`.

        Linear from `rakeFront` at `bbMinY` to `rakeRear` at `bbMaxY`, extrapolated linearly
        outside that range (§2 VenueModel / DSP-04).

        Zero-span guard: if the bbox y-span is below kMinSpan the plane collapses to the constant
        `rakeFront` rather than dividing by zero (QUAL-02).
    */
    float earHeight (float yMetres) const noexcept;

    /** Absolute source height — `earHeight(y) + srcZ`, so `srcZ = 0` rides the rake. The DSP-04
        sink at Phase 2.1. */
    float absoluteHeight (float yMetres, float srcZ) const noexcept;

    /** Normalised srcX/srcY (0..1, the host-facing parameter range) → metres via the speaker
        bounding box.

        Carries its OWN zero-span guard, separately from earHeight's: a venue whose speakers are
        all at one x would otherwise divide by zero here even though the rake is fine. A degenerate
        axis pins to its minimum.
    */
    Vec2 normToMetres (float nx, float ny) const noexcept;

private:
    //==============================================================================
    void recomputeDerived() noexcept;

    static Vec3         defaultSpeakerPosition (int speakerIndex) noexcept;
    static juce::String defaultSpeakerLabel    (int speakerIndex);

    //==============================================================================
    std::array<Vec3, kNumSpeakers>         speakers {};
    std::array<float, kNumSpeakers>        trims {};
    std::array<juce::String, kNumSpeakers> labels {};

    float rakeFrontM { 0.0f };
    float rakeRearM  { 0.0f };

    juce::String name;
    juce::String savedAt;

    // Derived.
    float boxMinX { 0.0f }, boxMaxX { 0.0f }, boxMinY { 0.0f }, boxMaxY { 0.0f };
    Vec3  centroidM {};
    float rigScaleM { 0.0f };
};

} // namespace oo
