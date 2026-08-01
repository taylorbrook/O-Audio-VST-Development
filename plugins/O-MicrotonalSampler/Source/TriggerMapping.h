/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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

    TriggerMapping.h
    O-MicrotonalSampler — v1.15.0 CC + PC trigger mapping tables.

    Pure-data layer for the three-trigger routing introduced in v1.15.0:

      * Keyswitch (KS) — semitone-offset note-on inside [ks_low..ks_high]
        directly maps to a technique slot. Implemented entirely in
        PluginProcessor::processBlock; no table needed.
      * MIDI CC (Continuous Controller) — a configurable controller number
        (cc_number) routes its 0..127 byte through an 8-slot table whose
        entries each cover an inclusive [rangeLow..rangeHigh] sub-band and
        emit a target technique index.
      * Program Change (PC) — an 8-slot table maps PC numbers (0..127)
        to target technique indices.

    All structures here are plain data + free helpers so the audio thread,
    the message thread, and the unit-test executable can share identical
    resolution logic without dragging in JuceHeader.h or the audio
    processor. The audio thread accesses live tables via the COW
    shared_ptr pattern already used for `currentSampleMap`.

    Precedence at note-on (per the v1.15.0 plan):

        KS > CC > PC > history > 0

    "history" = the prior pendingTechniqueIndex value (e.g. set by an
    earlier block's trigger or by a UI click). Within a single audio
    block, only the highest-precedence trigger that fired is committed
    to the atomic — see resolveTriggerPrecedence below.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace OMtsTrigger
{
    static constexpr int kMaxSlots = 8;
    static constexpr int kMaxTech  = 8;

    // CC sub-band → technique slot. rangeLow / rangeHigh are inclusive,
    // 0..127. technique is 0..7.
    struct CcSlot
    {
        int rangeLow  = 0;
        int rangeHigh = 127;
        int technique = 0;
    };

    // PC number → technique slot.
    struct PcSlot
    {
        int pc        = 0;
        int technique = 0;
    };

    using CcMapping = std::array<CcSlot, (size_t) kMaxSlots>;
    using PcMapping = std::array<PcSlot, (size_t) kMaxSlots>;

    // Default CC mapping: equally split [0..127] into `count` consecutive
    // sub-bands, each routed to its own technique. Slots beyond `count`
    // collapse onto the last band so the table is fully populated (those
    // tail slots produce duplicate matches but are harmless — the user
    // controls `count` from the UI and only the first match wins anyway,
    // see resolveCcTechnique).
    inline CcMapping defaultCcMapping (int count) noexcept
    {
        CcMapping m{};
        const int n = juce::jlimit (1, kMaxTech, count);
        for (int i = 0; i < kMaxSlots; ++i)
        {
            const int slotN = juce::jmin (i, n - 1);
            const int start = (slotN * 128) / n;
            const int end   = ((slotN + 1) * 128) / n - 1;
            m[(size_t) i] = CcSlot { start, end, slotN };
        }
        return m;
    }

    // Default PC mapping: pc[i] = i, technique[i] = i. PC 0 → tech 0,
    // PC 1 → tech 1, ..., PC 7 → tech 7.
    inline PcMapping defaultPcMapping() noexcept
    {
        PcMapping m{};
        for (int i = 0; i < kMaxSlots; ++i)
            m[(size_t) i] = PcSlot { i, i };
        return m;
    }

    // Resolve a CC value (0..127) to a technique index. First slot whose
    // [rangeLow..rangeHigh] contains `value` wins (slot 0 is searched
    // first — the user can deliberately stack overlapping bands and
    // expect the lower slot to take priority). Returns -1 on out-of-range
    // input or no match.
    inline int resolveCcTechnique (int value, const CcMapping& slots) noexcept
    {
        if (value < 0 || value > 127) return -1;
        for (const auto& s : slots)
        {
            if (value >= s.rangeLow && value <= s.rangeHigh)
                return juce::jlimit (0, kMaxTech - 1, s.technique);
        }
        return -1;
    }

    // Resolve a PC number (0..127) to a technique index. First slot
    // matching `pcNumber` wins. Returns -1 on no match.
    inline int resolvePcTechnique (int pcNumber, const PcMapping& slots) noexcept
    {
        if (pcNumber < 0 || pcNumber > 127) return -1;
        for (const auto& s : slots)
        {
            if (pcNumber == s.pc)
                return juce::jlimit (0, kMaxTech - 1, s.technique);
        }
        return -1;
    }

    // Precedence resolver: KS > CC > PC > history. Each ks/cc/pc input
    // is the candidate technique from the corresponding scan, or -1 if
    // no event of that kind fired in the current block. `historyTech`
    // is the prior pendingTechniqueIndex value (always 0..7). Returns
    // the technique index that should be committed to the atomic this
    // block.
    inline int resolveTriggerPrecedence (int ksTech, int ccTech, int pcTech,
                                         int historyTech) noexcept
    {
        if (ksTech >= 0) return juce::jlimit (0, kMaxTech - 1, ksTech);
        if (ccTech >= 0) return juce::jlimit (0, kMaxTech - 1, ccTech);
        if (pcTech >= 0) return juce::jlimit (0, kMaxTech - 1, pcTech);
        return juce::jlimit (0, kMaxTech - 1, historyTech);
    }
}
