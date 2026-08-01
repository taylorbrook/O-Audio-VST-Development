/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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

    O-simpleBeatmaker - Shared identifiers (voice roster, GM map, APVTS IDs)

    Single source of truth for the voice roster, the General-MIDI drum map, and
    the 42-parameter APVTS IDs. Pulled out of PluginProcessor.h so the Stage-2
    DSP spine headers (DrumVoiceEngine / UnifiedTriggerRouter / TimingFeelEngine)
    can share them without a circular include. IDs/ranges/defaults track
    parameter-spec.md (which mirrors ARCHITECTURE.md -> Parameter Mapping).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>

namespace OSimpleBeatmaker
{
    // Voice roster — row order is the grid's row order AND the GM-map order.
    inline constexpr int kNumVoices = 6;   // Kick Snare Clap ClosedHat OpenHat Tom
    inline constexpr int kMaxSteps  = 32;  // max columns; patternLength picks 8/16/32

    enum Voice { Kick = 0, Snare, Clap, ClosedHat, OpenHat, Tom };

    // General-MIDI drum note per voice (consumed by the Stage-2 trigger router).
    inline constexpr std::array<int, (size_t) kNumVoices> kGmNotes { 36, 38, 39, 42, 46, 45 };

    // lowerCamel prefixes used to compose the 36 per-voice parameter IDs.
    inline constexpr std::array<const char*, (size_t) kNumVoices> kVoicePrefix
        { "kick", "snare", "clap", "closedHat", "openHat", "tom" };

    // Human-readable names (generic editor / Stage-3 labels).
    inline constexpr std::array<const char*, (size_t) kNumVoices> kVoiceName
        { "Kick", "Snare", "Clap", "Closed Hat", "Open Hat", "Tom" };

    //==========================================================================
    // APVTS identifiers — single source of truth.
    namespace ParamIDs
    {
        // Sequencer / timing-feel (5)
        inline constexpr auto swing            = "swing";            // 0-1 (display 0-75%)
        inline constexpr auto humanize         = "humanize";         // 0-1 (display 0-100%)
        inline constexpr auto quantizeStrength = "quantizeStrength"; // 0-1 (display 0-100%)
        inline constexpr auto patternLength    = "patternLength";    // choice 8/16/32
        inline constexpr auto tempo            = "tempo";            // 40-240 BPM (free-run)

        // Master (1)
        inline constexpr auto outputLevel      = "outputLevel";      // -60..0 dB

        // Per-voice suffixes — combine with kVoicePrefix for the 36 voice IDs.
        inline constexpr auto sufTune  = "Tune";   // -12..+12 st
        inline constexpr auto sufDecay = "Decay";  // 0-1 (per-voice ms mapped in DSP)
        inline constexpr auto sufTone  = "Tone";   // 0-1 snap/body-noise/brightness
        inline constexpr auto sufLevel = "Level";  // -60..0 dB
        inline constexpr auto sufMute  = "Mute";   // bool
        inline constexpr auto sufSolo  = "Solo";   // bool
    }

    // Compose a per-voice parameter ID, e.g. voiceParamID (Kick, "Tune") -> "kickTune".
    inline juce::String voiceParamID (int voice, const char* suffix)
    {
        return juce::String (kVoicePrefix[(size_t) voice]) + suffix;
    }
}
