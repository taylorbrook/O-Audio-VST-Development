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

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <climits>
#include <cstdint>

// ============================================================================
// MIDI cursor (RESEARCH-2.1 §4, PLAN-2.1 P5 / T8).
//
// The host MidiBuffer is iterated IN PLACE: no copy, no fixed event list, no
// 512 cap, no re-sort. MidiBuffer keeps events sorted by time with same-time
// events in insertion (host) order (juce_MidiBuffer.cpp findEventAfter), and
// that order is applied as is — forcing "off before on" would turn a
// zero-length note (on, off, same sample) into a stuck note.
//
// Each 32-grid piece [a, b) pulls the events whose CLAMPED offset
// clamp (pos, 0, n − 1) is < b. Only the raw bytes are read
// (MidiMessageMetadata::data); MidiMessage is never constructed (a long sysex
// MidiMessage would allocate). Allocation-free, audio thread.
// ============================================================================
namespace ogi
{
    struct MidiEvent
    {
        int           pos    = 0;   // clamped sample offset in the host block
        std::uint8_t  status = 0;
        std::uint8_t  data1  = 0;
        std::uint8_t  data2  = 0;
        int           numBytes = 0;
    };

    class MidiCursor
    {
    public:
        MidiCursor (const juce::MidiBuffer& buffer, int numSamples) noexcept
            : it (buffer.cbegin()), end (buffer.cend()), lastSample (std::max (0, numSamples - 1)) {}

        /** Clamped offset of the next event, INT_MAX when none is left. */
        int nextPos() const noexcept
        {
            if (it == end)
                return INT_MAX;
            return std::clamp ((*it).samplePosition, 0, lastSample);
        }

        /** The next event's bytes; advances. Call only when nextPos() != INT_MAX. */
        MidiEvent pop() noexcept
        {
            MidiEvent e;
            const auto meta = *it;
            e.pos = std::clamp (meta.samplePosition, 0, lastSample);
            e.numBytes = meta.numBytes;
            if (meta.numBytes > 0 && meta.data != nullptr)
            {
                e.status = meta.data[0];
                if (meta.numBytes > 1) e.data1 = meta.data[1];
                if (meta.numBytes > 2) e.data2 = meta.data[2];
            }
            ++it;
            return e;
        }

    private:
        juce::MidiBufferIterator it, end;
        int lastSample = 0;
    };
} // namespace ogi
