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

#include <array>

/**
    The speaker→buffer channel map — R1, THE HIGHEST-RISK COMPONENT IN THIS PLUGIN.

    A wrong map is SILENT. It passes the build, passes auval, passes pluginval at strictness 10,
    passes every automated gate in the repo, and is audible only in the hall, where the sound comes
    out of the wrong speakers. That is why this lands before a single gain is computed.

    ── Why these are free functions ──────────────────────────────────────────────────────────────
    Namespace `ochan`, no processor reference (PLAN-2.1 P7). The unit-test target links this TU
    WITHOUT PluginProcessor.cpp, so it needs no JucePlugin_* block, compiles in seconds, and cannot
    be broken by the Stage-3 WebView editor swap.

    "One construction site" is preserved by discipline about the CALLER, not by hiding the logic on
    the processor: OOctagonProcessor::rebuildChannelMap() is the only caller of
    buildSpeakerToBuffer() in the plugin, the only writer of `mapInvalid`, and the only publisher of
    the map into the snapshot.

    ── The ground truth this rests on ────────────────────────────────────────────────────────────
    AudioChannelSet stores channels as a BITSET over ChannelType, and buffer order is ascending
    enum-bit order (juce_AudioChannelSet.cpp:514-527). The map is therefore keyed on ChannelType and
    resolved through getChannelIndexForType(), which is a GENUINE LOOKUP.

    DO NOT "simplify" the stored label to a slot index. That would make the lookup an identity
    function and the entire safety property would evaporate (ARCHITECTURE §3.2.3 note 3, §14 note 1).
*/
namespace ochan
{

//==============================================================================
// `inline constexpr`, not `static constexpr`: a namespace-scope static in a header gives every TU
// its own copy and can trip -Wunused-const-variable in a TU that only uses one of them. The
// zero-warning gate is hard.
inline constexpr int kNumSpeakers = 8;

/** Upper bound of the ChannelType bit scan in verifyEnumBitOrder().

    NOT 64, which is what ARCHITECTURE §3.2.5's Layer 1 snippet used. Named ChannelType values run
    to 99 (ambisonicACN63) and discreteChannel0 is 128; `AudioChannelSet::channels` is a BigInteger,
    not a 64-bit word (RESEARCH-2.1 G2).

    The bound alone does NOT close the hole — a truncated reconstruction is still strictly
    increasing, so it fails silently in exactly the direction the check exists to catch. The size
    assertion inside verifyEnumBitOrder() is the part that actually closes it.
*/
inline constexpr int kMaxChannelTypeScan = 256;

//==============================================================================
/** True iff `map` is a permutation of 0..7.

    THIS IS THE FUNC-03 DUPLICATE/MISSING DETECTOR, not a defensive afterthought:
      - two speakers assigned the same label produce a repeated target index;
      - a speaker whose label is absent from the negotiated set produces a −1.
    Both fail here, and a failing map is never applied.
*/
bool isPermutationOf0to7 (const std::array<int, kNumSpeakers>& map) noexcept;

/** Builds the speaker→buffer index map for a negotiated output set and a label assignment.

    Returns false — and LEAVES `out` COMPLETELY UNTOUCHED — if the set is not 8 channels, if any
    label is absent from the set, or if the result is not a permutation of 0..7. The caller's
    contract is to retain its last valid map on false and raise the FUNC-03 warning; it must never
    fall back to slot identity, which is silent misrouting by another name.

    @param outSet  the NEGOTIATED output channel set (getBusesLayout().getMainOutputChannelSet())
    @param labels  speaker 0..7 → the ChannelType that speaker is assigned to
    @param out     written only on success
*/
bool buildSpeakerToBuffer (const juce::AudioChannelSet& outSet,
                           const std::array<juce::AudioChannelSet::ChannelType, kNumSpeakers>& labels,
                           std::array<int, kNumSpeakers>& out);

/** Layer 1 of the three-layer channel-map test strategy — a RUNTIME INVARIANT.

    Reconstructs the expected buffer order without calling getChannelTypes(), then checks it against
    getTypeOfChannel(i). Two assertions, and the second is the load-bearing one:

      1. the reconstructed bit list is strictly increasing and matches getTypeOfChannel(i);
      2. `expected.size() == set.size()` — WITHOUT THIS THE CHECK FAILS SILENTLY, because a scan
         that stops short still produces a strictly-increasing prefix that matches position for
         position (RESEARCH-2.1 G2).

    Shares the bitset implementation with what it checks, so it is an invariant, not an independent
    oracle — that is Layer 2's job. It is kept because it is free and because it fires in Debug on
    the first prepareToPlay() for a developer who never builds the test target.

    @param whyNot  optional; receives a human-readable reason on failure
*/
bool verifyEnumBitOrder (const juce::AudioChannelSet& set, juce::String* whyNot = nullptr);

} // namespace ochan
