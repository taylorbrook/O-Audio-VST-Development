/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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

    FactoryPresets.h
    O-simplePhysicalModelSynth — factory preset library (6 presets).

    The six concept-isolating presets named in the brief (Bright Steel, Muted
    Nylon, Koto/Harp, Struck Bar, Bell, Bowed String) — covering all 3 exciters
    (Pluck/Strike/Bow) and both resonators (String KS / Modal). Raw parameter
    values are converted to APVTS-normalized [0,1] at build time via each
    parameter's NormalisableRange. Materialized to JSON under
    ~/Library/O-simplePhysicalModelSynth/Presets/Factory/ by OuariconPresetManager.

    No "Default" preset: the plugin's power-on sound is already the
    createParameterLayout() defaults, and a Default that set `material` would
    stomp the damping/decay defaults via the macro listener (RESEARCH §2).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the 6 factory presets (normalized values). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
