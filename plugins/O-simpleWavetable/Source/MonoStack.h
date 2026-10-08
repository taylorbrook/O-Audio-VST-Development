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

    O-simpleWavetable - MonoStack (Stage 2.2)
    Verbatim from O-simpleSubtractive/Source/PluginProcessor.h:85-131.

  ==============================================================================
*/

#pragma once

#include <array>
#include <cstddef>

//==============================================================================
// RT-safe held-note stack (last-note priority) for Mono / Legato. Fixed
// capacity, no allocation on the audio thread.
class MonoStack
{
public:
    void clear() noexcept { count = 0; }
    bool empty() const noexcept { return count == 0; }

    // Push a note (most recent = top). If already present, move it to the top.
    // Velocity is not kept: a fallback to a held note keeps the sounding
    // velocity (true legato), so only the note numbers matter.
    void push (int note) noexcept
    {
        remove (note);
        if (count < kCap)
        {
            notes[(size_t) count++] = note;
        }
        else
        {
            // Full: drop the oldest, append newest (extremely rare - 32 held notes).
            for (int i = 1; i < kCap; ++i)
                notes[(size_t) (i - 1)] = notes[(size_t) i];
            notes[(size_t) (kCap - 1)] = note;
        }
    }

    void remove (int note) noexcept
    {
        int w = 0;
        for (int r = 0; r < count; ++r)
            if (notes[(size_t) r] != note)
                notes[(size_t) w++] = notes[(size_t) r];
        count = w;
    }

    int topNote() const noexcept { return count > 0 ? notes[(size_t) (count - 1)] : -1; }

private:
    static constexpr int kCap = 32;
    std::array<int, kCap> notes {};
    int count = 0;
};
