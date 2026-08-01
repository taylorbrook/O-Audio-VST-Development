/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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

// Lock-free single-producer single-consumer triple buffer.
// Writer and reader never contend on the same slot.
template <typename T>
class TripleBuffer
{
public:
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
