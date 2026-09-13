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

    FactoryPresets.h
    O-Strata - Factory preset library (Stage 4 Round A: the 18-preset bank of
    stages/4-polish/RESEARCH.md §5.1, stamped by content so the on-disk bank is
    swept and regenerated whenever the authored content or the version moves).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the factory preset vector: `Init` (every non-excluded parameter at its
        default) + 17 presets authored in ENGINEERING units and converted once here
        through each parameter's convertTo0to1 (the exact-log Terrain Freq range, the
        skewed envelope / cutoff / rate ranges, choice indices). Values in the returned
        maps are normalised [0,1]; the seven tuning IDs are never written. A RawMap key
        that does not resolve to a parameter is jassert'ed, DBG'ed and skipped. */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);

    /** The bank stamp written to Factory/.factory-version: JucePlugin_VersionString + "+" +
        the first 12 hex of SHA-256 over a canonical serialisation of `defs` (per def in
        order: category, name, then every (id, float) in map order as the id's UTF-8 bytes
        + the 4 IEEE bytes — no text formatting, no drift). The constructor deletes and
        regenerates the on-disk bank when the stamp differs (Decision 19). */
    juce::String stamp (const std::vector<OuariconPresetManager::FactoryPresetDef>& defs);
}
