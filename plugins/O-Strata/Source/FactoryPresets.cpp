/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    FactoryPresets.cpp
    O-Strata - Factory preset library.
    Stage 1: a single "Init" preset at the 205-parameter defaults so the preset
    manager has a non-empty bank. The real bank is authored in Phase 4.1 from
    the live oscillator. No O-Prism preset content survives (Stage 1 CONTEXT D2).

  ==============================================================================
*/

#include "FactoryPresets.h"

std::vector<OuariconPresetManager::FactoryPresetDef>
FactoryPresets::build (juce::AudioProcessorValueTreeState& apvts)
{
    OuariconPresetManager::FactoryPresetDef init;
    init.category = "Init";
    init.name     = "Init";

    // getDefaultValue() is already normalised [0,1] — store it directly
    // (memory critical_apvts_denormalised_vs_preset_normalised). The preset
    // manager drops excludedParameterIds (7 tuning IDs) at write time → 198 keys.
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            init.parameters[rp->getParameterID()] = rp->getDefaultValue();

    return { init };
}
