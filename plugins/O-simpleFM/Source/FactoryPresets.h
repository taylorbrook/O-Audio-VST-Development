/*
   This file is part of O-simpleFM, an Ouaricon Audio plugin.
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
    O-simpleFM — factory preset library (6 presets).

    Default + the five pedagogical "lesson" sounds (E-Piano, Tubular Bell, Brass,
    Clarinet, Clang Bell). Raw parameter values are converted to APVTS-normalized
    [0,1] at build time via each parameter's NormalisableRange. Materialized to
    JSON under ~/Library/O-simpleFM/Presets/Factory/ by OuariconPresetManager.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the factory preset vector (normalized values). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
