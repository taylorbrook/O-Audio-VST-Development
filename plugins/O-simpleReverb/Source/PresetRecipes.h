/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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

    O-simpleReverb - the lesson table (v3.0.0, family-retrofit)

    The ONE source for Init and the ten lessons: the lesson buttons
    (applyFactoryPreset), the browser's lesson entries, the page catalog and
    the modified check all read this table. Lessons are NEVER written to disk:
    the Factory folder holds the 48 utility presets only (render-check
    section 9 counts 8 per type).

    Values are the parameters' own units (TYPE index, %, x, Hz, 0/1), each
    converted through the parameter's OWN convertTo0to1 at use, so DECAY's
    skew is honoured. A lesson is a full reset to defaults, then its recipe;
    unlisted parameters are at their defaults.

    Row 1, the spaces: booth .. ambient (one per TYPE).
    Row 2, the ideas:  init, sizeVsDecay, character, lowCut, insertSend.

    File names are ASCII and File::createLegalFileName-stable; none collides
    with a factory-bank name. The page shows the localized label for the id,
    never the file name.

    Header-only, C++17, ASCII.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace srpresets
{
struct Entry  { const char* paramId; float raw; };
struct Recipe { const char* id; const char* fileName; int count; Entry entries[6]; };

// Catalog order: Init, the six spaces in TYPE order, then the four ideas.
inline constexpr Recipe kLessons[11] = {
    { "init",        "Init",                    0, {} },   // == all defaults
    { "booth",       "Lesson - Booth",          2, { { "TYPE", 0.0f }, { "WET", 30.0f } } },
    { "room",        "Lesson - Room",           2, { { "TYPE", 1.0f }, { "WET", 30.0f } } },
    { "hall",        "Lesson - Hall",           3, { { "TYPE", 2.0f }, { "SIZE", 70.0f }, { "WET", 30.0f } } },
    { "spring",      "Lesson - Spring",         2, { { "TYPE", 3.0f }, { "WET", 35.0f } } },
    { "plate",       "Lesson - Plate",          3, { { "TYPE", 4.0f }, { "CHARACTER", 20.0f }, { "WET", 30.0f } } },
    { "ambient",     "Lesson - Ambient",        4, { { "TYPE", 5.0f }, { "SIZE", 75.0f }, { "WET", 45.0f }, { "DRY", 80.0f } } },
    // Room at its smallest with twice the decay: the taps pack into 11.5 ms
    // while the slope stays long (2.2 s). Size moves the walls, not the time.
    { "sizeVsDecay", "Lesson - Size vs Decay",  4, { { "TYPE", 1.0f }, { "SIZE", 0.0f }, { "DECAY", 2.0f }, { "WET", 35.0f } } },
    // Hall darkened: the Warm low-pass at about 5.6 kHz. The tail length does not move.
    { "character",   "Lesson - Character",      3, { { "TYPE", 2.0f }, { "CHARACTER", -80.0f }, { "WET", 35.0f } } },
    { "lowCut",      "Lesson - Low Cut",        5, { { "TYPE", 2.0f }, { "DECAY", 1.3f }, { "WET", 40.0f }, { "LPON", 1.0f }, { "LPFREQ", 250.0f } } },
    // The values of the bank's "Plate - Send": an aux-bus setting, exempt from
    // the insert level ceiling.
    { "insertSend",  "Lesson - Insert vs Send", 5, { { "TYPE", 4.0f }, { "WET", 100.0f }, { "DRY", 0.0f }, { "LPON", 1.0f }, { "LPFREQ", 150.0f } } },
};

inline constexpr int kNumLessons = (int) (sizeof (kLessons) / sizeof (kLessons[0]));

// The lessons that sit on a send bus rather than an insert (no level ceiling).
inline bool isSendLesson (const Recipe& r) { return juce::String (r.id) == "insertSend"; }

// Table lookup by stable id (exact, case-sensitive: ids are a code contract).
inline const Recipe* findById (const juce::String& id)
{
    if (id.isEmpty())
        return nullptr;
    for (const auto& r : kLessons)
        if (id == r.id)
            return &r;
    return nullptr;
}

// Table lookup by file name, case-insensitive (a user name that differs only
// in case from a lesson name is still a lesson name).
inline const Recipe* findByName (const juce::String& name)
{
    if (name.isEmpty())
        return nullptr;
    for (const auto& r : kLessons)
        if (name.equalsIgnoreCase (juce::String (r.fileName)))
            return &r;
    return nullptr;
}
} // namespace srpresets
