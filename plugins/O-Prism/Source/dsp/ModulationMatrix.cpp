/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    ModulationMatrix.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    16-slot modulation routing matrix with per-sample evaluation

  ==============================================================================
*/

#include "ModulationMatrix.h"

void ModulationMatrix::setAPVTS (juce::AudioProcessorValueTreeState* apvts)
{
    if (apvts == nullptr)
        return;

    for (int i = 0; i < kNumSlots; ++i)
    {
        auto prefix = "modSlot" + juce::String (i);
        cachedParams[static_cast<size_t> (i)].src = apvts->getRawParameterValue (prefix + "Src");
        cachedParams[static_cast<size_t> (i)].dst = apvts->getRawParameterValue (prefix + "Dst");
        cachedParams[static_cast<size_t> (i)].amt = apvts->getRawParameterValue (prefix + "Amt");
        cachedParams[static_cast<size_t> (i)].on  = apvts->getRawParameterValue (prefix + "On");
    }
}

void ModulationMatrix::updateFromAPVTS()
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        const auto& cp = cachedParams[static_cast<size_t> (i)];

        if (cp.src == nullptr)
            continue;

        slots[static_cast<size_t> (i)].source  = static_cast<int> (cp.src->load());
        slots[static_cast<size_t> (i)].dest    = static_cast<int> (cp.dst->load());
        slots[static_cast<size_t> (i)].amount  = cp.amt->load();
        slots[static_cast<size_t> (i)].enabled = cp.on->load() > 0.5f;
    }

    // Which destinations any slot actually targets this block (REG-03), and
    // which slots are worth visiting at all (IN-06). The skip condition below
    // is the one evaluate() used to apply inline, verbatim: a destination
    // absent from this set receives no accumulation, so getModOffset returns
    // 0.0f for every sample until the next updateFromAPVTS. Amount is
    // deliberately NOT part of the test — an amount of exactly 0 is a value,
    // and keeping the predicate purely structural is what makes it safe to
    // hoist out of the per-sample loop.
    // Snapshot the previous routing before rebuilding it, so a destination that
    // just LEFT the routed set can be zeroed exactly once, at the transition.
    const auto wasRouted = destRouted;

    destRouted.fill (false);
    numActiveSlots = 0;
    numRoutedDests = 0;

    for (int i = 0; i < kNumSlots; ++i)
    {
        const auto& slot = slots[static_cast<size_t> (i)];

        if (! slot.enabled || slot.source == 0 || slot.dest == 0)
            continue;

        auto srcIdx = static_cast<size_t> (slot.source);
        auto dstIdx = static_cast<size_t> (slot.dest);

        // evaluate() used to bounds-check both indices per sample. Hoisted:
        // a slot that would fail either check can never contribute, so it does
        // not belong in the active list at all.
        if (srcIdx >= sourceValues.size() || dstIdx >= destOffsets.size())
            continue;

        activeSlots[static_cast<size_t> (numActiveSlots++)] = i;

        if (! destRouted[dstIdx])
        {
            destRouted[dstIdx] = true;
            routedDests[static_cast<size_t> (numRoutedDests++)] = slot.dest;
        }
    }

    // THE INVARIANT THAT MAKES IN-06 SAFE. evaluate() clears only routedDests,
    // so a destination that was routed last block and is not routed now would
    // keep the offset it last accumulated — forever, since nothing would ever
    // zero it again. It is zeroed here instead, once, on the transition.
    //
    // This is deliberately a TRANSITION clear and not a blanket
    // `destOffsets.fill (0.0f)`. A blanket wipe here is wrong, and the
    // lfo-subblock-check gate catches it: this function runs once per MIDI
    // SUB-BLOCK, not once per block, and PrismVoice reads ModDest::Pitch at the
    // TOP of the sample loop (PrismVoice.cpp:599) — i.e. one sample late, using
    // the value the previous iteration's evaluate() left behind. Zeroing a
    // still-routed destination at a sub-block boundary therefore changes the
    // rendered pitch on the first sample after every MIDI event, which made the
    // render density-sensitive (max |diff| 0.380141199 against the undivided
    // block). Destinations that stay routed must keep carrying across the
    // boundary exactly as they did before IN-06.
    for (size_t d = 0; d < destRouted.size(); ++d)
        if (wasRouted[d] && ! destRouted[d])
            destOffsets[d] = 0.0f;
}

void ModulationMatrix::setSourceValue (ModSource source, float value)
{
    auto idx = static_cast<size_t> (source);
    if (idx < sourceValues.size())
        sourceValues[idx] = value;
}

void ModulationMatrix::evaluate()
{
    // IN-06: clears and walks only what updateFromAPVTS() found this block,
    // not all kNumDests and all kNumSlots. A destination that left the routed
    // set is zeroed by the transition clear in updateFromAPVTS(), once, rather
    // than by a full wipe here every sample.
    for (int d = 0; d < numRoutedDests; ++d)
        destOffsets[static_cast<size_t> (routedDests[static_cast<size_t> (d)])] = 0.0f;

    for (int a = 0; a < numActiveSlots; ++a)
    {
        const auto& slot = slots[static_cast<size_t> (activeSlots[static_cast<size_t> (a)])];

        // Predicate and bounds already applied in updateFromAPVTS().
        destOffsets[static_cast<size_t> (slot.dest)]
            += sourceValues[static_cast<size_t> (slot.source)] * slot.amount;
    }
}

float ModulationMatrix::getModOffset (ModDest dest) const
{
    auto idx = static_cast<size_t> (dest);
    if (idx < destOffsets.size())
        return destOffsets[idx];
    return 0.0f;
}

bool ModulationMatrix::isDestinationRouted (ModDest dest) const
{
    auto idx = static_cast<size_t> (dest);
    if (idx < destRouted.size())
        return destRouted[idx];
    return false;
}

