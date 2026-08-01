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
    FactoryPresets.h
    O-Bassoon - 4 factory preset definitions for v1.0.0
  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the 4-preset factory vector for O-Bassoon v1.0.0.
        Values are stored as normalized [0,1] per OuariconPresetManager
        convention. Tuning state is not preset-persisted at v1.0
        (custom-state callback unused). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
