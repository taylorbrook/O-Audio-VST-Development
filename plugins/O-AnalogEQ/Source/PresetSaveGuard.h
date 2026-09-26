/*
   This file is part of O-AnalogEQ, an Ouaricon Audio plugin.
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
    PresetSaveGuard.h — the factory-preset protection that savePresetToFile()
    does not carry.

    WHY THIS IS A SEPARATE HEADER AND NOT A LAMBDA IN PluginEditor.cpp.

    The decision belongs to the Save-dialog callback, but the editor TU is
    deliberately not compiled into the render harness (JUCE_WEB_BROWSER=0 —
    pattern_render_harness_breaks_on_webview_editor), so a predicate written
    inline there is unreachable by any gate. Living here it is a pure function
    of (chosen path, preset manager) that both the editor and the harness
    include, so the guard is gated on the value the editor actually branches
    on rather than on a downstream proxy for it.
*/

#pragma once

#include <JuceHeader.h>
#include "OuariconPresetManager.h"

namespace oaeq
{

/** The reason a chosen Save-Preset target must be refused, or an empty string
    to allow the write.

    Background: preset-manager's two save APIs are not interchangeable.
    `savePreset(name)` early-returns false on `isFactoryPreset()`;
    `savePresetToFile(file)` has no guard at all and writes wherever it is
    pointed. O-AnalogEQ v1.5.1 swapped the first for the second to fix WR-07
    (the dialog discarded the user's chosen directory) and lost the protection
    with it. Two failures, both reachable from a dialog that opens in
    `getUserPresetsDirectory()`, and both silent:

      1. Typing a factory name writes `User/<name>.json` and returns true, so
         the UI reports success — but `loadPreset()` searches `Factory/` FIRST,
         so the save is written and permanently unreachable.
      2. Navigating up into `Factory/` overwrites the factory JSON outright;
         `initializeFactoryPresets()` early-returns while the `.factory-version`
         sentinel matches, so nothing restores it until the next version bump.

    The test is LOCATION-AWARE rather than name-only. A bare
    `isFactoryPreset(basename)` reject would re-break the arbitrary-path export
    that WR-07 exists to enable: saving a tweaked "Surgical Cut" to ~/Desktop
    shadows nothing, because `loadPreset()` only ever searches the two library
    directories. Only a write into those can collide, so only those are refused.

    @param chosen  the file the user picked, before extension fixup
    @param pm      the preset manager owning the Factory/User directories
*/
inline juce::String presetSaveRefusal (const juce::File& chosen,
                                       const OuariconPresetManager& pm)
{
    // savePresetToFile() applies this same fixup internally; doing it here
    // first makes the guard and the write agree on one path, so a target
    // typed without an extension cannot be judged under a different name
    // than the one that lands on disk.
    const auto target = chosen.hasFileExtension ("json") ? chosen
                                                         : chosen.withFileExtension ("json");

    // isAChildOf, not parent equality: any write anywhere beneath Factory/ is
    // wrong, including one into a subdirectory the user created there.
    if (target.isAChildOf (pm.getFactoryPresetsDirectory()))
        return "Factory presets are read-only. Choose a different folder.";

    // Parent equality here, by contrast: only a file directly in User/ is
    // listed by getPresetList() and therefore only that one can be shadowed.
    // isFactoryPreset() sanitizes the name it is given, which is what
    // loadPreset() will do to the dropdown entry, so the two agree.
    if (target.getParentDirectory() == pm.getUserPresetsDirectory()
        && pm.isFactoryPreset (target.getFileNameWithoutExtension()))
        return "\"" + target.getFileNameWithoutExtension()
             + "\" is a factory preset name. A user preset saved under it would never load, "
               "because factory presets take precedence. Choose a different name.";

    return {};
}

} // namespace oaeq
