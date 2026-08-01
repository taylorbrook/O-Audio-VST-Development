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

    FactoryPresets.h
    O-Prism - Factory preset library (96 presets across 9 categories)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the full factory preset vector.
        Values in each preset are stored as normalized [0,1] after conversion
        from raw parameter ranges via APVTS param ranges.
        Tuning parameters are never included (they're excluded in PresetManager). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
