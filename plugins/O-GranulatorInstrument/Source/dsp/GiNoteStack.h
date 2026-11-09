/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#pragma once

#include <cstdint>

// ============================================================================
// GiNoteStack — the Mono / Legato physical-key stack (PLAN-2.5b PD10,
// RESEARCH-2.5b Summary 3). The O-simpleWavetable MonoStack semantics (push
// moves an existing key to the top, last-note priority, remove by key,
// drop-oldest on overflow, no allocation), keyed on (channel, note) so MPE
// member expression and VST3 note expression follow the sounding key. Each
// entry carries its noteId and its own NE tuning (a D30 fallback lands on the
// held key's tuning). Not a MonoStack fork: no FORK-MANIFEST row.
// ============================================================================
namespace ogi
{
    struct GiNoteEntry
    {
        int    channel = 1;
        int    note    = 0;
        int    noteId  = -1;
        double neSemis = 0.0;
    };

    class GiNoteStack
    {
    public:
        static constexpr int kCapacity = 128;

        void clear() noexcept                 { n = 0; }
        int  size() const noexcept            { return n; }
        bool empty() const noexcept           { return n == 0; }
        const GiNoteEntry& at (int i) const noexcept { return e[i]; }   // 0 = oldest
        GiNoteEntry&       at (int i) noexcept       { return e[i]; }
        const GiNoteEntry* top() const noexcept { return n > 0 ? &e[n - 1] : nullptr; }

        bool isTop (int channel, int note) const noexcept
        {
            return n > 0 && e[n - 1].channel == channel && e[n - 1].note == note;
        }

        /** Push (an existing (channel, note) moves to the top). Full → the
            oldest entry is dropped (counted). */
        void push (const GiNoteEntry& x) noexcept
        {
            remove (x.channel, x.note);
            if (n == kCapacity)
            {
                for (int i = 1; i < n; ++i)
                    e[i - 1] = e[i];
                --n;
                ++dropped;
            }
            e[n++] = x;
        }

        /** Removes (channel, note); true when it was there. */
        bool remove (int channel, int note) noexcept
        {
            for (int i = 0; i < n; ++i)
                if (e[i].channel == channel && e[i].note == note)
                {
                    for (int j = i + 1; j < n; ++j)
                        e[j - 1] = e[j];
                    --n;
                    return true;
                }
            return false;
        }

        std::uint64_t dropped = 0;   // overflow counter (stackDropped)

    private:
        GiNoteEntry e[kCapacity] {};
        int n = 0;
    };
} // namespace ogi
