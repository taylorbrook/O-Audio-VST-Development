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
#include "ChannelMap.h"

namespace ochan
{

//==============================================================================
bool isPermutationOf0to7 (const std::array<int, kNumSpeakers>& map) noexcept
{
    bool seen[kNumSpeakers] = {};

    for (const int idx : map)
    {
        if (idx < 0 || idx >= kNumSpeakers)
            return false;                       // a −1 from an absent label lands here

        if (seen[idx])
            return false;                       // two speakers claiming one output

        seen[idx] = true;
    }

    return true;
}

//==============================================================================
bool buildSpeakerToBuffer (const juce::AudioChannelSet& outSet,
                           const std::array<juce::AudioChannelSet::ChannelType, kNumSpeakers>& labels,
                           std::array<int, kNumSpeakers>& out,
                           MapDiagnosis* whyNot)
{
    // Cleared up front so the success path leaves `{ none, -1 }` and a caller that reuses one
    // MapDiagnosis across several builds cannot inherit a stale reason from the previous one.
    if (whyNot != nullptr)
        *whyNot = {};

    const auto fail = [whyNot] (MapFailure reason, int speakerIndex)
    {
        if (whyNot != nullptr)
            *whyNot = { reason, speakerIndex };

        return false;
    };

    if (outSet.size() != kNumSpeakers)
        return fail (MapFailure::notEightChannels, -1);

    std::array<int, kNumSpeakers> next {};

    for (int n = 0; n < kNumSpeakers; ++n)
    {
        // A GENUINE LOOKUP, because the label is stored as a ChannelType. Had it been stored as a
        // slot index this call would be the identity function and the map would be unable to detect
        // anything (ARCHITECTURE §3.2.3 note 3).
        const int idx = outSet.getChannelIndexForType (labels[(size_t) n]);

        if (idx < 0)
            return fail (MapFailure::labelNotInSet, n);   // not a member of the negotiated set

        next[(size_t) n] = idx;
    }

    if (! isPermutationOf0to7 (next))
    {
        // isPermutationOf0to7 is NOT changed to carry the row — it is probed by name at Phase 2.1
        // and its contract is a pure predicate. The row is recovered here instead, by the scan that
        // could not have run before `next` was complete. Reporting the SECOND of the two colliding
        // rows is deliberate: under commit-on-blur it is the one the operator just typed.
        for (int n = 1; n < kNumSpeakers; ++n)
            for (int m = 0; m < n; ++m)
                if (next[(size_t) n] == next[(size_t) m])
                    return fail (MapFailure::duplicateLabel, n);

        // Unreachable in practice — every non-permutation of eight in-range indices contains a
        // repeat — but a bare `return false` here would be a reason of `none` on a failure.
        return fail (MapFailure::duplicateLabel, -1);
    }

    // `out` is written ONLY here, on the success path. A partially-written map is worse than no
    // map: the caller's "retain the last valid map" contract would silently inherit half of a
    // rejected assignment.
    out = next;
    return true;
}

//==============================================================================
bool verifyEnumBitOrder (const juce::AudioChannelSet& set, juce::String* whyNot)
{
    const auto fail = [whyNot] (const juce::String& reason)
    {
        if (whyNot != nullptr)
            *whyNot = reason;

        return false;
    };

    // Reconstruct the expected buffer order WITHOUT getChannelTypes() — probing
    // getChannelIndexForType() one bit at a time is a different path through the same bitset, which
    // is what makes this an invariant check rather than a tautology.
    std::array<int, kMaxChannelTypeScan> expected {};
    int expectedCount = 0;

    for (int bit = 0; bit < kMaxChannelTypeScan; ++bit)
        if (set.getChannelIndexForType (static_cast<juce::AudioChannelSet::ChannelType> (bit)) >= 0)
            expected[(size_t) expectedCount++] = bit;

    // ── The assertion that actually closes G2 ────────────────────────────────────────────────
    //
    // Without this, a scan bound that is too small fails SILENTLY: the truncated reconstruction is
    // still strictly increasing and still matches getTypeOfChannel(i) for every i it covers, so
    // every remaining assertion below passes on a prefix. The size comparison is the only thing
    // that can tell a complete reconstruction from a plausible partial one.
    if (expectedCount != set.size())
        return fail ("reconstructed " + juce::String (expectedCount) + " channel type(s) but the set "
                     "reports " + juce::String (set.size()) + " — the "
                     + juce::String (kMaxChannelTypeScan) + "-bit scan is too narrow for this set, "
                     "or getChannelIndexForType() no longer agrees with the bitset");

    for (int i = 1; i < expectedCount; ++i)
        if (expected[(size_t) i] <= expected[(size_t) (i - 1)])
            return fail ("reconstructed channel order is not strictly increasing at index "
                         + juce::String (i));

    for (int i = 0; i < set.size(); ++i)
        if (static_cast<int> (set.getTypeOfChannel (i)) != expected[(size_t) i])
            return fail ("buffer slot " + juce::String (i) + " reports type "
                         + juce::String (static_cast<int> (set.getTypeOfChannel (i)))
                         + " but ascending enum-bit order gives " + juce::String (expected[(size_t) i])
                         + " — JUCE's buffer ordering has CHANGED and every speaker→buffer map in "
                           "this plugin is now wrong");

    return true;
}

} // namespace ochan
