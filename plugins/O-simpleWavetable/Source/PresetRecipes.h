/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - factory recipe table (Stage 4, D-AD)

    The ONE sound authority for the 9 factory presets: the lesson buttons
    (applyFactoryPreset), the factory preset load, buildFactoryPresetDefs(),
    the page catalog and the modified check all read this table. Disk copies
    of the factory bank are never read back.

    Values are RAW engineering units (seconds, Hz, 0..1, bipolar -1..1), or
    choice indices: bank 0 Sine->Saw, 1 Sine->Square, 2 Pulse Width,
    3 Formant, 4 Drive (Imported is never used); lfo_shape 0 Sine,
    1 Triangle, 2 Saw, 3 Square, 4 S&H; bit_depth 9 = "8", 13 = "4"; bools
    0 / 1. Each is converted with the parameter's own convertTo0to1 at use,
    so the skews are honoured. Unlisted parameters go to their defaults. The
    output level is user-owned: no recipe lists it.

    Fill rule: ARCHITECTURE A9 wins where it names a value; the Stage 3
    lesson recipe fills where A9 is silent; the 4-bit PPG variant is the 9th
    entry. kInit equals the APVTS defaults.

    Header-only, C++17, ASCII.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cstring>

namespace wtpresets
{
struct Entry  { const char* paramId; float raw; };
struct Recipe { const char* id; const char* name; const Entry* entries; int count; };
template <int N> constexpr Recipe make (const char* id, const char* name, const Entry (&e)[N]) { return { id, name, e, N }; }

inline constexpr Entry kInit[]     = { { "bank", 0 } };   // == all defaults (a 1-entry array; C++ has no zero-length arrays)
inline constexpr Entry kStepped[]  = { {"bank",0}, {"position",0.5f}, {"interp",0}, {"lfo_sync",0}, {"lfo_shape",2}, {"lfo_rate",0.25f}, {"lfo_depth",1} };
inline constexpr Entry kSmooth[]   = { {"bank",0}, {"position",0.5f}, {"lfo_sync",0}, {"lfo_shape",2}, {"lfo_rate",0.25f}, {"lfo_depth",1} };      // interp default On
inline constexpr Entry kAlias[]    = { {"bank",4}, {"position",1}, {"bandlimit",0} };
inline constexpr Entry kDrive[]    = { {"bank",4}, {"position",0}, {"env_amount",1}, {"menv_attack",0.01f}, {"menv_decay",1.5f}, {"menv_sustain",0}, {"menv_release",0.8f} };
inline constexpr Entry kVowel[]    = { {"bank",3}, {"position",0.5f}, {"lfo_sync",0}, {"lfo_shape",1}, {"lfo_rate",0.1f}, {"lfo_depth",1}, {"amp_attack",0.6f}, {"amp_release",1.4f} };
inline constexpr Entry kPulse[]    = { {"bank",2}, {"env_amount",0.8f} };
inline constexpr Entry kPpg8[]     = { {"bank",1}, {"position",0.6f}, {"interp",0}, {"bit_depth",9},  {"lfo_sync",0}, {"lfo_shape",4}, {"lfo_rate",4}, {"lfo_depth",0.4f} };
inline constexpr Entry kPpg4[]     = { {"bank",1}, {"position",0.6f}, {"interp",0}, {"bit_depth",13}, {"lfo_sync",0}, {"lfo_shape",4}, {"lfo_rate",4}, {"lfo_depth",0.4f} };

inline constexpr Recipe kFactory[] = {
    make ("init", "Init - Additive Build", kInit),  make ("steppedSmooth", "Stepped Scan", kStepped),
    make ("smoothScan", "Smooth Scan", kSmooth),    make ("aliasDemo", "Alias Demo", kAlias),
    make ("driveSweep", "Drive Sweep", kDrive),     make ("vowelPad", "Vowel Pad", kVowel),
    make ("pulseNarrowing", "Pulse Narrowing", kPulse), make ("ppg8bit", "8-bit PPG", kPpg8),
    make ("ppg4bit", "4-bit PPG", kPpg4) };

inline constexpr int kNumFactory = (int) (sizeof (kFactory) / sizeof (kFactory[0]));

// Table lookup by stable id (exact, case-sensitive: ids are a code contract).
inline const Recipe* findById (const char* id)
{
    if (id == nullptr)
        return nullptr;
    for (const auto& r : kFactory)
        if (std::strcmp (r.id, id) == 0)
            return &r;
    return nullptr;
}

inline const Recipe* findById (const juce::String& id)
{
    return findById (id.toRawUTF8());
}

// Table lookup by file / display name, case-insensitive (a user name that
// differs only in case from a factory name is still a factory name).
inline const Recipe* findByName (const juce::String& name)
{
    if (name.isEmpty())
        return nullptr;
    for (const auto& r : kFactory)
        if (name.equalsIgnoreCase (juce::String (r.name)))
            return &r;
    return nullptr;
}
} // namespace wtpresets
