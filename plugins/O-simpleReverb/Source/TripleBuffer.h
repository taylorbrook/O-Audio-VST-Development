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
#pragma once

#include <array>
#include <atomic>
#include <type_traits>

namespace osr
{

// Lock-free single-producer single-consumer triple buffer.
// Writer and reader never contend on the same slot.
//
// O-simpleReverb v3.0.0: copied VERBATIM from O-simpleAmbisonics
// (Source/dsp/TripleBuffer.h), itself a copy of O-GrainScatter's. Not a
// registered Ouaricon module; this is the fourth consumer. Kept OUT of
// Source/dsp/ so that directory stays the frozen v2.0.0 engines.
// Used for the visualisation tap (TripleBuffer<MeterFrame>): the audio
// thread fills getWriteBuffer() then publish()es once per host block; the
// editor's 30 Hz Timer read()s on the message thread.
//
// Protocol: publish() swaps the write slot with `middle` (acq_rel) and raises
// newData; read() consumes newData then swaps `middle` into the read slot.
// Wait-free; each side only ever touches its own index plus the atomic
// middle, so there is no ABA on the indices.
//
// read() must be called from EXACTLY ONE thread (the consumer); hasNewData()
// may be polled from either side (the writer uses it to gate its reset).
template <typename T>
class TripleBuffer
{
public:
    static_assert (std::is_trivially_copyable_v<T>, "TripleBuffer<T> crosses threads by value");

    T& getWriteBuffer() { return buffers[static_cast<size_t> (writeIdx)]; }

    void publish()
    {
        writeIdx = middle.exchange (writeIdx, std::memory_order_acq_rel);
        newData.store (true, std::memory_order_release);
    }

    const T& read()
    {
        if (newData.exchange (false, std::memory_order_acq_rel))
            readIdx = middle.exchange (readIdx, std::memory_order_acq_rel);
        return buffers[static_cast<size_t> (readIdx)];
    }

    bool hasNewData() const { return newData.load (std::memory_order_acquire); }

private:
    std::array<T, 3> buffers {};
    int writeIdx = 0;
    std::atomic<int> middle { 1 };
    int readIdx = 2;
    std::atomic<bool> newData { false };
};

} // namespace osr
