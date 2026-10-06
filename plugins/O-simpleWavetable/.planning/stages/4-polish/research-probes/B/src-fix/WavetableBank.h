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

    O-simpleWavetable - WavetableBank (Stage 2.1)
    Ouaricon Audio
    Developer: Taylor Brook

    A bank = numFrames single cycles x 11 strict mip levels. Flat layout:
        data[((level * numFrames) + frame) * kStride + i],  i = 0..2048
    Sample 2048 is the guard (== sample 0) so the linear read of i0 + 1 is
    always in bounds for i0 <= 2047.

    IMMUTABLE after build: the audio thread holds raw const pointers into it
    and never writes. Built only on the message / worker thread.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstddef>
#include <vector>

// Test hooks compile ONLY on the console test targets (CMake defines
// OSIW_TEST_HOOKS=1 there). The shipped plugin never contains them.
#ifndef OSIW_TEST_HOOKS
 #define OSIW_TEST_HOOKS 0
#endif

struct WavetableBank
{
    static constexpr int kTableSize = 2048;
    static constexpr int kStride    = kTableSize + 1;   // guard sample = s[0]
    static constexpr int kLevels    = 11;
    static constexpr int kMaxFrames = 256;
    static constexpr int kThumbSize = 128;              // Stage 3 bankUpdate seam
    static constexpr int kThumbStep = kTableSize / kThumbSize;

    // Highest harmonic kept at level L: 1023, 512, 256, ..., 2, 1.
    static constexpr int kmax (int level) noexcept { return level <= 0 ? 1023 : (1024 >> level); }

    WavetableBank()
    {
       #if OSIW_TEST_HOOKS
        testLiveCount.fetch_add (1);
       #endif
    }

    ~WavetableBank()
    {
       #if OSIW_TEST_HOOKS
        testLiveCount.fetch_sub (1);
       #endif
    }

    WavetableBank (const WavetableBank&) = delete;
    WavetableBank& operator= (const WavetableBank&) = delete;

    juce::String name;               // ASCII ("Sine->Saw"); the UI uses the param choice text
    int numFrames = 0;
    std::vector<float> data;         // [level][frame][kStride]; immutable after build
    std::vector<float> thumbs;       // [frame][kThumbSize], decimated from level 0

    // Message / worker thread only (allocates).
    void allocate (int frames)
    {
        numFrames = frames < 0 ? 0 : (frames > kMaxFrames ? kMaxFrames : frames);
        data.assign ((size_t) kLevels * (size_t) numFrames * (size_t) kStride, 0.0f);
        thumbs.assign ((size_t) numFrames * (size_t) kThumbSize, 0.0f);
    }

    const float* frame (int level, int f) const noexcept
    {
        return data.data() + ((size_t) level * (size_t) numFrames + (size_t) f) * (size_t) kStride;
    }

    float* frame (int level, int f) noexcept
    {
        return data.data() + ((size_t) level * (size_t) numFrames + (size_t) f) * (size_t) kStride;
    }

    const float* thumb (int f) const noexcept
    {
        return thumbs.data() + (size_t) f * (size_t) kThumbSize;
    }

    float* thumb (int f) noexcept
    {
        return thumbs.data() + (size_t) f * (size_t) kThumbSize;
    }

   #if OSIW_TEST_HOOKS
    mutable std::atomic<bool> testReaped { false };      // graveyard flag (2.4 reaper gate)
    static inline std::atomic<int> testLiveCount { 0 };  // ctor ++ / dtor -- (leak gate)

    // D-C proof without ASan: in graveyard mode the reaper parks banks and
    // sets testReaped instead of freeing them. Every audio-thread dereference
    // site (block resolve, frozen-cycle capture) reports here; a reaped bank
    // being touched is the use-after-free the amendment prevents.
    static inline std::atomic<int> testDerefAfterReap { 0 };
    static void testNoteDeref (const WavetableBank* b) noexcept
    {
        if (b != nullptr && b->testReaped.load (std::memory_order_relaxed))
            testDerefAfterReap.fetch_add (1);
    }
   #endif
};
