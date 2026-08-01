/*
   This file is part of O-Bassoon, an Ouaricon Audio plugin.
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
    FactoryPresets.cpp - O-Bassoon factory preset definitions
  ==============================================================================
*/
#include "FactoryPresets.h"

std::vector<OuariconPresetManager::FactoryPresetDef>
FactoryPresets::build (juce::AudioProcessorValueTreeState& /*apvts*/)
{
    std::vector<OuariconPresetManager::FactoryPresetDef> presets = {
        {
            "Long Drone",
            {{"vibrato_rate", 0.45f}, {"vibrato_depth", 0.12f}, {"vibrato_onset", 0.40f},
             {"breath", 0.50f}, {"tone", 0.40f}, {"attack_character", 0.00f},
             {"attack_time", 0.60f}, {"release_time", 0.60f},
             {"voice_count", 0.20f}, {"output_gain", 0.70f}},
            juce::var()
        },
        {
            "Microtonal Pad",
            {{"vibrato_rate", 0.40f}, {"vibrato_depth", 0.25f}, {"vibrato_onset", 0.75f},
             {"breath", 0.65f}, {"tone", 0.50f}, {"attack_character", 0.00f},
             {"attack_time", 0.75f}, {"release_time", 0.7333f},
             {"voice_count", 0.4667f}, {"output_gain", 0.7333f}},
            juce::var()
        },
        {
            "Tongued Long Tone",
            {{"vibrato_rate", 0.55f}, {"vibrato_depth", 0.18f}, {"vibrato_onset", 0.15f},
             {"breath", 0.75f}, {"tone", 0.55f}, {"attack_character", 1.00f},
             {"attack_time", 0.04f}, {"release_time", 0.40f},
             {"voice_count", 0.20f}, {"output_gain", 0.7667f}},
            juce::var()
        },
        {
            "Bright Bassoon",
            {{"vibrato_rate", 0.60f}, {"vibrato_depth", 0.10f}, {"vibrato_onset", 0.20f},
             {"breath", 0.70f}, {"tone", 1.00f}, {"attack_character", 0.20f},
             {"attack_time", 0.10f}, {"release_time", 0.2667f},
             {"voice_count", 0.20f}, {"output_gain", 0.80f}},
            juce::var()
        }
    };
    return presets;
}
