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
#include "VenueModel.h"

#include <algorithm>
#include <cmath>

namespace oo
{

//==============================================================================
// Schema identifiers. These strings are the on-disk contract for every session and every .venue
// file written from v1.0.0 onwards — renaming one orphans user data silently.

const juce::Identifier VenueModel::venueTag           { "VENUE" };
const juce::Identifier VenueModel::speakerTag         { "SPEAKER" };

const juce::Identifier VenueModel::propName           { "name" };
const juce::Identifier VenueModel::propSavedAt        { "savedAt" };
const juce::Identifier VenueModel::propSchemaVersion  { "schemaVersion" };
const juce::Identifier VenueModel::propRakeFront      { "rakeFront" };
const juce::Identifier VenueModel::propRakeRear       { "rakeRear" };

const juce::Identifier VenueModel::propIndex          { "index" };
const juce::Identifier VenueModel::propX              { "x" };
const juce::Identifier VenueModel::propY              { "y" };
const juce::Identifier VenueModel::propZ              { "z" };
const juce::Identifier VenueModel::propTrimDb         { "trimDb" };
const juce::Identifier VenueModel::propLabel          { "label" };

//==============================================================================
namespace
{
    // ── §OQ4 default venue ────────────────────────────────────────────────────────────────────
    //
    // The traced layout re-based on its own bounding box and scaled to a plausible span: 12.0 m
    // between the side-wall lines, 15.0 m front-to-rear, in a 13.0 × 22.0 m hall envelope.
    //
    // THE GRADED HEIGHTS 4.50 → 5.40 ARE LOAD-BEARING AND MUST NOT BE FLATTENED. A uniform default
    // z makes every (z_i − z_s) difference identical across speakers, which would hide a dropped z
    // term in Phase 2.2's DSP-01 acceptance test. A graded default exercises the 3D path from the
    // first build. (The physical argument is also true — wall mounts sit at a constant height above
    // a raked local floor — but the test argument is the one that makes them non-negotiable.)
    //
    // These are PLAUSIBLE PLACEHOLDERS, not measurements. The Venue screen must say so, and the
    // measurement session remains a prerequisite for concert use.
    constexpr float kDefaultSpeakerXYZ[VenueModel::kNumSpeakers][3] =
    {
        {  0.50f,  4.50f, 4.50f },   // 1  front-left
        { 12.50f,  4.50f, 4.50f },   // 2  front-right
        { 12.50f,  9.85f, 4.70f },   // 3  right-2nd
        { 12.50f, 16.00f, 5.10f },   // 4  right-3rd
        {  9.80f, 19.50f, 5.40f },   // 5  back-right
        {  3.20f, 19.50f, 5.40f },   // 6  back-left
        {  0.50f, 16.00f, 5.10f },   // 7  left-3rd
        {  0.50f,  9.85f, 4.70f },   // 8  left-2nd
    };

    constexpr float kDefaultRakeFront = 1.10f;   // seated ear height at the front row
    constexpr float kDefaultRakeRear  = 3.20f;   // 2.1 m of rise over ~13 rows

    // ── Shipped default label map (§3.2.4) ────────────────────────────────────────────────────
    //
    // The identity map, chosen so an interleaved Logic surround bounce yields channel N = speaker N
    // and drops straight into QLab/Reaper without a remap.
    //
    // Speaker 4 → LFE is intentional and safe: Logic applies no automatic bass management or LFE
    // low-pass, so that slot carries a full-range feed to an ordinary speaker.
    //
    // NOTE for tests: because this default is the identity under all three accepted 8-channel
    // containers, a channel-map test driven by it alone is VACUOUS — a hardcoded 0..7 map would
    // pass it (RESEARCH-2.1 C1/G5). Every map probe must drive a NON-identity assignment.
    constexpr juce::AudioChannelSet::ChannelType kDefaultLabelTypes[VenueModel::kNumSpeakers] =
    {
        juce::AudioChannelSet::left,                // 1 → "L"
        juce::AudioChannelSet::right,               // 2 → "R"
        juce::AudioChannelSet::centre,              // 3 → "C"
        juce::AudioChannelSet::LFE,                 // 4 → "Lfe"
        juce::AudioChannelSet::leftSurroundSide,    // 5 → "Lss"
        juce::AudioChannelSet::rightSurroundSide,   // 6 → "Rss"
        juce::AudioChannelSet::leftSurroundRear,    // 7 → "Lrs"
        juce::AudioChannelSet::rightSurroundRear,   // 8 → "Rrs"
    };

    bool isValidSpeaker (int i) noexcept
    {
        return i >= 0 && i < VenueModel::kNumSpeakers;
    }

    /** Reads one float attribute, falling back to `fallback` when the property is absent.

        This is where "partial node yields defaults PER ATTRIBUTE" is actually implemented: the
        fallback is applied one property at a time, so a VENUE node carrying only @rakeRear keeps
        that value and defaults everything else, rather than being discarded wholesale or read as
        zeros.
    */
    float readFloat (const juce::ValueTree& tree, const juce::Identifier& prop, float fallback)
    {
        if (! tree.isValid() || ! tree.hasProperty (prop))
            return fallback;

        return static_cast<float> (static_cast<double> (tree.getProperty (prop)));
    }

    juce::String readString (const juce::ValueTree& tree,
                             const juce::Identifier& prop,
                             const juce::String& fallback)
    {
        if (! tree.isValid() || ! tree.hasProperty (prop))
            return fallback;

        return tree.getProperty (prop).toString();
    }
}

//==============================================================================
Vec3 VenueModel::defaultSpeakerPosition (int speakerIndex) noexcept
{
    if (! isValidSpeaker (speakerIndex))
        return {};

    return { kDefaultSpeakerXYZ[speakerIndex][0],
             kDefaultSpeakerXYZ[speakerIndex][1],
             kDefaultSpeakerXYZ[speakerIndex][2] };
}

juce::String VenueModel::defaultSpeakerLabel (int speakerIndex)
{
    if (! isValidSpeaker (speakerIndex))
        return {};

    // Derived from the ChannelType via JUCE's own table rather than written out as string
    // literals — the abbreviation is JUCE's public contract and mirroring it here would be a
    // fixture that drifts silently on a JUCE bump.
    return juce::AudioChannelSet::getAbbreviatedChannelTypeName (kDefaultLabelTypes[speakerIndex]);
}

//==============================================================================
VenueModel::VenueModel()
{
    resetToDefaults();
}

void VenueModel::resetToDefaults()
{
    for (int i = 0; i < kNumSpeakers; ++i)
    {
        speakers[(size_t) i] = defaultSpeakerPosition (i);
        trims[(size_t) i]    = 0.0f;
        labels[(size_t) i]   = defaultSpeakerLabel (i);
    }

    rakeFrontM = kDefaultRakeFront;
    rakeRearM  = kDefaultRakeRear;

    // juce::String (const char*) converts through CharPointer_ASCII (juce_String.cpp:307-308) and
    // MANGLES every byte above 127 — and `name = "..."` builds exactly that temporary, because the
    // only assignment overloads take a String. The em-dash here is U+2014, three UTF-8 bytes, so
    // the plain form shipped three garbage characters into apvts.state's VENUE @name, into every
    // .venue file written from Phase 3.2 on, and (from Phase 3.1, which is the first thing to
    // RENDER this string) onto the Room screen. There is no compiler warning; the damage is visible
    // only in output, which is why it survived Stage 2 unnoticed
    // (critical_juce_string_char_ctor_is_ascii_only).
    //
    // CharPointer_UTF8 is the explicit form. `+`, `+=` and `<<` are also safe — they take the UTF-8
    // path (juce_String.cpp:773-777) — which is why the two diagnostic strings built with `+` in
    // ChannelMap.cpp were never affected and need no change.
    name    = juce::String (juce::CharPointer_UTF8 ("Default (placeholder — NOT measured)"));
    savedAt = {};

    recomputeDerived();
}

//==============================================================================
void VenueModel::readFromState (const juce::ValueTree& parentState)
{
    // Start from defaults so that every attribute the tree does not carry is already correct.
    // A missing VENUE child then costs exactly one early return, which is the Stage-1-session
    // path: every session saved before Phase 2.1 has 17 parameters and nothing else.
    resetToDefaults();

    if (! parentState.isValid())
        return;

    const auto venue = parentState.getChildWithName (venueTag);

    if (! venue.isValid())
        return;

    name       = readString (venue, propName,    name);
    savedAt    = readString (venue, propSavedAt, savedAt);
    rakeFrontM = readFloat  (venue, propRakeFront, rakeFrontM);
    rakeRearM  = readFloat  (venue, propRakeRear,  rakeRearM);

    // @schemaVersion is read but not branched on: version 1 is the only version that exists. It is
    // written so a future migration has something to key off, and a future build reading a NEWER
    // version still gets a usable venue through the per-attribute fallbacks above.

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        // Located by the @index attribute (1-based, matching "Speaker 1..8" in the UI and w1..w8),
        // not by child position — a hand-edited .venue file with reordered children must still
        // load correctly. Falls back to positional lookup only if no child claims the index.
        auto child = venue.getChildWithProperty (propIndex, i + 1);

        if (! child.isValid() && i < venue.getNumChildren())
        {
            auto positional = venue.getChild (i);

            if (positional.hasType (speakerTag) && ! positional.hasProperty (propIndex))
                child = positional;
        }

        if (! child.isValid())
            continue;                       // this speaker keeps its defaults

        const auto p = speakers[(size_t) i];

        speakers[(size_t) i] = { readFloat (child, propX, p.x),
                                 readFloat (child, propY, p.y),
                                 readFloat (child, propZ, p.z) };

        trims[(size_t) i]  = readFloat  (child, propTrimDb, trims[(size_t) i]);
        labels[(size_t) i] = readString (child, propLabel,  labels[(size_t) i]);
    }

    recomputeDerived();
}

juce::ValueTree VenueModel::toValueTree() const
{
    juce::ValueTree venue { venueTag };

    venue.setProperty (propName,          name,       nullptr);
    venue.setProperty (propSavedAt,       savedAt,    nullptr);
    venue.setProperty (propSchemaVersion, kSchemaVersion, nullptr);
    venue.setProperty (propRakeFront,     rakeFrontM, nullptr);
    venue.setProperty (propRakeRear,      rakeRearM,  nullptr);

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        juce::ValueTree spk { speakerTag };

        spk.setProperty (propIndex,  i + 1,                 nullptr);
        spk.setProperty (propX,      speakers[(size_t) i].x, nullptr);
        spk.setProperty (propY,      speakers[(size_t) i].y, nullptr);
        spk.setProperty (propZ,      speakers[(size_t) i].z, nullptr);
        spk.setProperty (propTrimDb, trims[(size_t) i],      nullptr);
        spk.setProperty (propLabel,  labels[(size_t) i],     nullptr);

        venue.appendChild (spk, nullptr);
    }

    return venue;
}

void VenueModel::writeToState (juce::ValueTree& parentState, juce::UndoManager* undoManager) const
{
    if (! parentState.isValid())
        return;

    auto existing = parentState.getChildWithName (venueTag);

    if (existing.isValid())
        parentState.removeChild (existing, undoManager);

    parentState.appendChild (toValueTree(), undoManager);
}

//==============================================================================
void VenueModel::setSpeakerPosition (int speakerIndex, Vec3 positionMetres)
{
    if (! isValidSpeaker (speakerIndex))
        return;

    speakers[(size_t) speakerIndex] = positionMetres;
    recomputeDerived();
}

void VenueModel::setSpeakerTrimDb (int speakerIndex, float trimDecibels)
{
    if (! isValidSpeaker (speakerIndex))
        return;

    trims[(size_t) speakerIndex] = trimDecibels;
    // No recompute needed — trim is not a geometric quantity — but keeping the call uniform means
    // a future derived value that DOES depend on trim cannot be missed here.
    recomputeDerived();
}

void VenueModel::setSpeakerLabel (int speakerIndex, const juce::String& abbreviation)
{
    if (! isValidSpeaker (speakerIndex))
        return;

    labels[(size_t) speakerIndex] = abbreviation;
}

void VenueModel::setRake (float front, float rear)
{
    rakeFrontM = front;
    rakeRearM  = rear;
    recomputeDerived();
}

void VenueModel::setName (const juce::String& newName)
{
    name = newName;
}

//==============================================================================
Vec3 VenueModel::speaker (int speakerIndex) const noexcept
{
    return isValidSpeaker (speakerIndex) ? speakers[(size_t) speakerIndex] : Vec3 {};
}

float VenueModel::trimDb (int speakerIndex) const noexcept
{
    return isValidSpeaker (speakerIndex) ? trims[(size_t) speakerIndex] : 0.0f;
}

float VenueModel::trimLin (int speakerIndex) const noexcept
{
    return juce::Decibels::decibelsToGain (trimDb (speakerIndex));
}

juce::String VenueModel::labelAbbreviation (int speakerIndex) const
{
    return isValidSpeaker (speakerIndex) ? labels[(size_t) speakerIndex] : juce::String {};
}

juce::AudioChannelSet::ChannelType VenueModel::labelType (int speakerIndex) const
{
    if (! isValidSpeaker (speakerIndex))
        return juce::AudioChannelSet::unknown;

    // JUCE's own parser (juce_AudioChannelSet.h:553), NOT a hand-rolled name→type table: the
    // abbreviations are JUCE's public contract and a local copy would drift silently on a bump
    // (RESEARCH-2.1 G4).
    //
    // Two failure shapes, both safe and both tested:
    //   - an unrecognised string  → `unknown` (0), a member of no accepted set;
    //   - a NUMERIC string        → discreteChannel0 + n − 1 with NO range check, a plausible-
    //                               looking discrete type that is still in no accepted set.
    // Either way getChannelIndexForType() returns −1 and the map build rejects the assignment.
    return juce::AudioChannelSet::getChannelTypeFromAbbreviation (labels[(size_t) speakerIndex]);
}

std::array<juce::AudioChannelSet::ChannelType, VenueModel::kNumSpeakers> VenueModel::labelTypes() const
{
    std::array<juce::AudioChannelSet::ChannelType, kNumSpeakers> out {};

    for (int i = 0; i < kNumSpeakers; ++i)
        out[(size_t) i] = labelType (i);

    return out;
}

//==============================================================================
void VenueModel::recomputeDerived() noexcept
{
    boxMinX = boxMaxX = speakers[0].x;
    boxMinY = boxMaxY = speakers[0].y;

    Vec3 sum {};

    for (const auto& p : speakers)
    {
        boxMinX = std::min (boxMinX, p.x);
        boxMaxX = std::max (boxMaxX, p.x);
        boxMinY = std::min (boxMinY, p.y);
        boxMaxY = std::max (boxMaxY, p.y);

        sum.x += p.x;
        sum.y += p.y;
        sum.z += p.z;
    }

    constexpr float inv = 1.0f / static_cast<float> (kNumSpeakers);
    centroidM = { sum.x * inv, sum.y * inv, sum.z * inv };

    float sumSq = 0.0f;

    for (const auto& p : speakers)
        sumSq += len2 (sub (p, centroidM));

    rigScaleM = std::sqrt (sumSq * inv);
}

//==============================================================================
// ── One-line delegates to oo::plane (PLAN-2.2 P14) ────────────────────────────────────────────
//
// The arithmetic — including BOTH independent zero-span guards — lives in Source/Data/VenueGeometry.h
// so the audio thread can run it against a VenueSnapshot. These members exist because the message
// thread and the editor already hold a VenueModel and should not have to unpack four scalars to ask
// it a question.
//
// Probes V and W assert member == free function over a swept set on a NON-DEFAULT venue, so
// "there is no second implementation to drift" is a test rather than a claim.

float VenueModel::earHeight (float yMetres) const noexcept
{
    return plane::earHeight (rakeFrontM, rakeRearM, boxMinY, boxMaxY, yMetres);
}

float VenueModel::absoluteHeight (float yMetres, float srcZ) const noexcept
{
    return earHeight (yMetres) + srcZ;
}

Vec2 VenueModel::normToMetres (float nx, float ny) const noexcept
{
    return plane::normToMetres (boxMinX, boxMaxX, boxMinY, boxMaxY, nx, ny);
}

} // namespace oo
