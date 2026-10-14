/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics - factory recipe table (Stage 4, PLAN P1)

    The ONE sound authority for the 9 factory presets (Init + the 8 lessons):
    the lesson buttons (applyFactoryPreset), the browser's factory loads,
    buildFactoryPresetDefs(), the page catalog and the modified check all
    read this table. Disk copies of the factory bank are never read back.

    Values are RAW engineering units (degrees, metres, Hz) or choice indices:
    order 0 / 1 / 2 = orders 1 / 2 / 3; decoder 0 Binaural, 1 Stereo; orbit
    0 / 1. Each is converted with the parameter's OWN convertTo0to1 at use,
    so the distance / orbitRate skews are honoured. Unlisted parameters go
    to their defaults (a lesson is a full reset + its recipe, R-D2).

    User-owned, never listed (R-D3): outputLevel and inputMode.
    Headroom: no row (and no orderStep step, P4) reaches order 3 with a
    distance below 1 m — the Binaural order-3 bank peaks +6.5 dB, 0.2 m
    adds +8.9 dB. Harness G-ORDER3-DIST gates it.

    File names are ASCII and File::createLegalFileName-stable (the lesson
    captions contain "/" and an arrow, which are not); the page shows the
    localized label from its i18n key, never the file name.

    Header-only, C++17, ASCII.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cstring>

namespace ambpresets
{
struct Entry  { const char* paramId; float raw; };
struct Recipe { const char* id; const char* fileName; int count; Entry entries[4]; };

// Table order = walk order = bank order (PLAN factory table).
inline constexpr Recipe kFactory[9] = {
    { "init",               "Init",                   1, { { "azimuth", 0.0f }, {}, {}, {} } },   // == all defaults (one entry: C++ has no zero-length arrays)
    { "pointSource",        "Point Source (FOA)",     4, { { "order", 0.0f }, { "azimuth", 30.0f }, { "elevation", 0.0f }, { "distance", 1.0f } } },
    { "orderStep",          "Order 1-2-3",            2, { { "order", 0.0f }, { "azimuth", 45.0f }, {}, {} } },   // lesson clicks walk the order (P4)
    { "behindYou",          "Behind You",             1, { { "azimuth", 180.0f }, {}, {}, {} } },
    { "overhead",           "Overhead",               2, { { "elevation", 90.0f }, { "order", 1.0f }, {}, {} } },
    { "rotateRoom",         "Rotate the Room",        3, { { "azimuth", 60.0f }, { "yaw", 120.0f }, { "order", 1.0f }, {} } },
    { "farNear",            "Far Away - Up Close",    1, { { "distance", 12.0f }, {}, {}, {} } },
    { "slowOrbit",          "Slow Orbit",             3, { { "orbit", 1.0f }, { "orbitRate", 0.05f }, { "order", 1.0f }, {} } },
    { "speakersHeadphones", "Speakers vs Headphones", 3, { { "decoder", 1.0f }, { "order", 2.0f }, { "azimuth", 45.0f }, {} } },
};

inline constexpr int kNumFactory = (int) (sizeof (kFactory) / sizeof (kFactory[0]));

// The lesson whose repeated clicks walk the order 1 -> 2 -> 3 -> 1 (R-D4).
inline constexpr const char* kOrderStepId = "orderStep";

// Table lookup by stable id (exact, case-sensitive: ids are a code contract).
inline const Recipe* findById (const juce::String& id)
{
    if (id.isEmpty())
        return nullptr;
    for (const auto& r : kFactory)
        if (id == r.id)
            return &r;
    return nullptr;
}

// Table lookup by file name, case-insensitive (a user name that differs only
// in case from a factory name is still a factory name).
inline const Recipe* findByName (const juce::String& name)
{
    if (name.isEmpty())
        return nullptr;
    for (const auto& r : kFactory)
        if (name.equalsIgnoreCase (juce::String (r.fileName)))
            return &r;
    return nullptr;
}
} // namespace ambpresets
