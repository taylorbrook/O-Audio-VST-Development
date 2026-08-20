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
    The measured CoreAudio device order for the negotiated 7.1 set — v1.1.0's "which PHYSICAL
    output does this label reach" table.

    ── What this is, and what it is not ──────────────────────────────────────────────────────────
    The plugin writes a JUCE buffer in enum-bit order; the HOST decides which physical output each
    channel ROLE lands on, and the plugin cannot see that decision. For create7point1() under
    CoreAudio (Logic Pro, the Standalone) the device order was MEASURED on this plugin at Stage 4
    Gate 16 as `kAudioChannelLayoutTag_Emagic_Default_7_1`:

        physical output:  1  2   3    4    5  6    7    8
        channel role:     L  R  Lrs  Rrs   C  Lfe  Lss  Rss

    (CHANGELOG v1.0.0, "Host validation". The BOUNCE path orders differently — that table is not
    this table, and neither is the JUCE buffer order.)

    So "wire speaker n to physical output k" is a LABEL assignment, not an index: give speaker n
    the ChannelType this table holds at k, and the existing buildSpeakerToBuffer() /
    getChannelIndexForType() path does the rest. Nothing here touches a buffer index, which is the
    R1 discipline unchanged (critical_audiochannelset_is_a_bitset_not_an_order).

    ── The honest caveat, stated once ────────────────────────────────────────────────────────────
    This table is CoreAudio's. A non-CoreAudio VST3 host may map roles to hardware differently, in
    which case the output numbers this file names are that host's numbers only after its own
    routing is accounted for. The verify ping remains the 60-second ground truth in any host, and
    the UI copy says so.
*/
namespace oo::outorder
{

inline constexpr int kNumOutputs = 8;

/** kDeviceOrder71[k] = the ChannelType CoreAudio's Emagic_Default_7_1 device order places at
    1-based physical output k+1. Measured, not assumed — see the header comment. */
inline constexpr std::array<juce::AudioChannelSet::ChannelType, kNumOutputs> kDeviceOrder71 {
    juce::AudioChannelSet::left,
    juce::AudioChannelSet::right,
    juce::AudioChannelSet::leftSurroundRear,
    juce::AudioChannelSet::rightSurroundRear,
    juce::AudioChannelSet::centre,
    juce::AudioChannelSet::LFE,
    juce::AudioChannelSet::leftSurroundSide,
    juce::AudioChannelSet::rightSurroundSide,
};

/** The label abbreviation for 1-based physical output k — JUCE's own table via
    getAbbreviatedChannelTypeName(), never a transcribed string (the abbreviations are JUCE's
    public contract and a local copy would drift silently on a bump, RESEARCH-2.1 G4). */
inline juce::String abbreviationForOutput (int output1based)
{
    if (output1based < 1 || output1based > kNumOutputs)
        return {};

    return juce::AudioChannelSet::getAbbreviatedChannelTypeName (
        kDeviceOrder71[static_cast<std::size_t> (output1based - 1)]);
}

/** 1-based physical output a ChannelType reaches under the measured device order, or 0 when the
    type is not in the 7.1 set (a custom or unresolvable label — the UI renders that as "?"). */
inline int outputNumberForType (juce::AudioChannelSet::ChannelType t) noexcept
{
    for (int k = 0; k < kNumOutputs; ++k)
        if (kDeviceOrder71[static_cast<std::size_t> (k)] == t)
            return k + 1;

    return 0;
}

/** outputNumberForType over a stored label abbreviation, through JUCE's own parser — the same
    parser VenueModel::labelType() uses, so the two cannot disagree about what a label means. */
inline int outputNumberForLabel (const juce::String& abbreviation)
{
    return outputNumberForType (juce::AudioChannelSet::getChannelTypeFromAbbreviation (abbreviation));
}

} // namespace oo::outorder
