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
#include "SceneModel.h"

#include <cmath>

// NOTHING WITH A PROCESSOR IN IT MAY BE INCLUDED HERE. SceneModel.h pulls
// juce_data_structures, ConvexHull2D.h and Vec.h, and that is the whole dependency set — which is
// what lets this TU join the FAST unit target (Gate 11 re-verifies the link line rather than
// trusting this comment). Two TUs join that target at 3.3, which is exactly the state in which a
// narrow link line quietly widens.

namespace oo
{

//==============================================================================
namespace scenes
{

const char* name (Named which) noexcept
{
    switch (which)
    {
        case Named::all:    return "ALL";
        case Named::front:  return "FRONT";
        case Named::rear:   return "REAR";
        case Named::left:   return "LEFT";
        case Named::right:  return "RIGHT";
        case Named::sides:  return "SIDES";
        case Named::kCount: break;
    }

    return "ALL";
}

bool parse (const juce::String& id, Named& out) noexcept
{
    // Compared against name() rather than against a second literal table: a transcribed list here
    // would be free to drift from the one above, and the drift would present as a scene button
    // that silently does nothing (pattern_test_fixture_mirrors_drift_silently).
    for (int i = 0; i < kNumNamed; ++i)
    {
        const auto candidate = static_cast<Named> (i);

        if (id == name (candidate))
        {
            out = candidate;
            return true;
        }
    }

    return false;
}

//==============================================================================
Membership resolve (Named which,
                    const std::array<Vec3, kNumSpeakers>& spk,
                    const ConvexHull2D& hull) noexcept
{
    Membership m {};

    // ── The centroid and the bbox half-spans, from the positions and nothing else ─────────────
    float cx = 0.0f, cy = 0.0f;

    for (const auto& p : spk)
    {
        cx += p.x;
        cy += p.y;
    }

    cx /= static_cast<float> (kNumSpeakers);
    cy /= static_cast<float> (kNumSpeakers);

    float minX = spk[0].x, maxX = spk[0].x;
    float minY = spk[0].y, maxY = spk[0].y;

    for (const auto& p : spk)
    {
        minX = juce::jmin (minX, p.x);
        maxX = juce::jmax (maxX, p.x);
        minY = juce::jmin (minY, p.y);
        maxY = juce::jmax (maxY, p.y);
    }

    const float hx = (maxX - minX) * 0.5f;
    const float hy = (maxY - minY) * 0.5f;

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const auto& p = spk[static_cast<std::size_t> (i)];
        bool member = false;

        switch (which)
        {
            case Named::all:
                member = true;
                break;

            // STRICT comparisons on all four axis splits. A speaker exactly ON the centroid axis
            // belongs to neither half, which is the honest answer: it is not in front of the
            // centre and it is not behind it. `<=` would put it in one arbitrary half and make
            // FRONT and REAR overlap, so ALL would no longer be their disjoint union.
            case Named::front: member = p.y < cy; break;
            case Named::rear:  member = p.y > cy; break;
            case Named::left:  member = p.x < cx; break;
            case Named::right: member = p.x > cx; break;

            case Named::sides:
            {
                // != INTERIOR, NOT == VERTEX. Speakers 3 and 8 of the §OQ4 venue are ON_EDGE —
                // three collinear speakers on each side wall make the hull a hexagon — and both
                // belong to SIDES. Reading this as `== VERTEX` silently drops them and the scene
                // still LOOKS plausible, which is why the predicate is stated rather than
                // paraphrased (D16).
                if (hull.classify (i) == ConvexHull2D::Classification::INTERIOR)
                    break;

                // The HALF-SPAN NORMALISATION is what makes "off the sides" mean anything on a
                // rectangular hall: without it, a 12 x 15 m room calls almost everything sideward
                // because x deviations are compared with y deviations in raw metres. A degenerate
                // axis contributes 0 rather than dividing by zero — the same guard VenueModel
                // applies to the audience plane and the bbox denormalisation, arriving a third
                // time (QUAL-02).
                const float nx = hx > 0.0f ? std::abs (p.x - cx) / hx : 0.0f;
                const float ny = hy > 0.0f ? std::abs (p.y - cy) / hy : 0.0f;

                member = nx > ny;
                break;
            }

            case Named::kCount:
                break;
        }

        m.in[static_cast<std::size_t> (i)] = member;

        if (member)
            ++m.count;
    }

    return m;
}

std::array<float, kNumSpeakers> weightsFor (const Membership& m) noexcept
{
    std::array<float, kNumSpeakers> w {};

    for (int i = 0; i < kNumSpeakers; ++i)
        w[static_cast<std::size_t> (i)] = m.in[static_cast<std::size_t> (i)] ? 1.0f : 0.0f;

    return w;
}

} // namespace scenes

//==============================================================================
const juce::Identifier SceneStore::scenesTag         { "SCENES" };
const juce::Identifier SceneStore::slotTag           { "SLOT" };
const juce::Identifier SceneStore::propSchemaVersion { "schemaVersion" };
const juce::Identifier SceneStore::propIndex         { "index" };
const juce::Identifier SceneStore::propOccupied      { "occupied" };

const juce::Identifier& SceneStore::propWeight (int speakerIndex)
{
    // ONE table, built once. The writer and the reader both come through here, so a renamed
    // attribute cannot be renamed on only one side — the same discipline VenueModel's twelve
    // Identifiers follow, expressed as a function because these eight are indexed.
    static const std::array<juce::Identifier, kNumSpeakers> ids {
        juce::Identifier ("w1"), juce::Identifier ("w2"),
        juce::Identifier ("w3"), juce::Identifier ("w4"),
        juce::Identifier ("w5"), juce::Identifier ("w6"),
        juce::Identifier ("w7"), juce::Identifier ("w8"),
    };

    const int clamped = juce::jlimit (0, kNumSpeakers - 1, speakerIndex);
    return ids[static_cast<std::size_t> (clamped)];
}

//==============================================================================
void SceneStore::readFromState (const juce::ValueTree& parentState)
{
    // Start from EMPTY, not from whatever this object already held. That is the opposite of
    // VenueModel::readFromState's per-attribute fallback, and deliberately so: a venue has §OQ4
    // defaults that are the right answer for a missing value, whereas a missing scene slot means
    // "the operator has not stored one" and inventing weights for it would put an unmeasured gain
    // vector one click away from the PA.
    slots = {};

    const auto node = parentState.getChildWithName (scenesTag);

    if (! node.isValid())
        return;

    for (int i = 0; i < node.getNumChildren(); ++i)
    {
        const auto child = node.getChild (i);

        if (! child.hasType (slotTag))
            continue;

        const int index = static_cast<int> (child.getProperty (propIndex, -1));

        if (index < 0 || index >= kNumSlots)
            continue;

        auto& slot = slots[static_cast<std::size_t> (index)];

        slot.occupied = static_cast<bool> (child.getProperty (propOccupied, false));

        for (int s = 0; s < kNumSpeakers; ++s)
        {
            const float raw = static_cast<float> (
                static_cast<double> (child.getProperty (propWeight (s), 0.0)));

            // Sanitised AT INGESTION, at the same site and for the same reason publishSnapshot()
            // sanitises the venue: a NaN reaching setValueNotifyingHost sets a SmoothedValue's
            // step to NaN and latches PERMANENT SILENCE with no self-healing path
            // (RESEARCH-2.2 H2, arriving through a new door).
            slot.w[static_cast<std::size_t> (s)] =
                std::isfinite (raw) ? juce::jlimit (0.0f, 1.0f, raw) : 0.0f;
        }
    }
}

void SceneStore::writeToState (juce::ValueTree& parentState, juce::UndoManager* undoManager) const
{
    auto node = parentState.getOrCreateChildWithName (scenesTag, undoManager);

    node.setProperty (propSchemaVersion, kSchemaVersion, undoManager);
    node.removeAllChildren (undoManager);

    for (int i = 0; i < kNumSlots; ++i)
    {
        const auto& slot = slots[static_cast<std::size_t> (i)];

        juce::ValueTree child { slotTag };
        child.setProperty (propIndex,    i,             undoManager);
        child.setProperty (propOccupied, slot.occupied, undoManager);

        for (int s = 0; s < kNumSpeakers; ++s)
            child.setProperty (propWeight (s), slot.w[static_cast<std::size_t> (s)], undoManager);

        node.appendChild (child, undoManager);
    }
}

//==============================================================================
bool SceneStore::isOccupied (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots)
        return false;

    return slots[static_cast<std::size_t> (slot)].occupied;
}

std::array<float, SceneStore::kNumSpeakers> SceneStore::weights (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots)
        return {};

    return slots[static_cast<std::size_t> (slot)].w;
}

void SceneStore::capture (int slot, const std::array<float, kNumSpeakers>& w) noexcept
{
    if (slot < 0 || slot >= kNumSlots)
        return;

    auto& target = slots[static_cast<std::size_t> (slot)];

    for (int s = 0; s < kNumSpeakers; ++s)
    {
        const float raw = w[static_cast<std::size_t> (s)];
        target.w[static_cast<std::size_t> (s)] =
            std::isfinite (raw) ? juce::jlimit (0.0f, 1.0f, raw) : 0.0f;
    }

    target.occupied = true;
}

} // namespace oo
