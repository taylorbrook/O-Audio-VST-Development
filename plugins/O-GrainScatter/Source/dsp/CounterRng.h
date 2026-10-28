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

#include <cstdint>

// v3.1.0: counter-based RNG. Every draw is a pure function of (seed, stream, event
// counter, slot), so the value a grain or a trigger gets can't drift with block size,
// and a conditional draw (Size Random at 0, spatial off, not frozen) can't shift the
// values any other draw gets. The seed is saved with the session.
namespace CounterRng
{
    // splitmix64 finaliser
    inline uint64_t mix (uint64_t x) noexcept
    {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    inline uint64_t hash (uint64_t seed, uint64_t counter, uint32_t slot) noexcept
    {
        return mix (mix (seed ^ mix (counter)) + slot);
    }

    // Uniform in [0, 1), 24-bit resolution (exact in float).
    inline float uniform (uint64_t seed, uint64_t counter, uint32_t slot) noexcept
    {
        return static_cast<float> (hash (seed, counter, slot) >> 40) * (1.0f / 16777216.0f);
    }

    // Uniform integer in [0, n); n > 0.
    inline int uniformInt (uint64_t seed, uint64_t counter, uint32_t slot, int n) noexcept
    {
        return static_cast<int> (((hash (seed, counter, slot) >> 32) * static_cast<uint64_t> (n)) >> 32);
    }

    // Streams keep the scheduler's and the grains' draws independent of each other.
    enum Stream : uint64_t
    {
        SchedulerFree = 0x5C4ED0000001ull,
        SchedulerSync = 0x5C4ED0000002ull,
        Grain         = 0x6EA1A0000003ull
    };

    // Fixed per-quantity slots for a grain's draws.
    enum GrainSlot : uint32_t
    {
        SlotSize = 1, SlotAmp, SlotSpread, SlotPitch, SlotPan, SlotReverse,
        SlotAzimuth, SlotElevation, SlotDetune,
        SlotSkip    // v3.4.0: drawn only while Grain Skip > 0 (appended: no other slot moves)
    };

    // One event's draws: a fixed slot per quantity.
    struct Draw
    {
        uint64_t key = 0;   // seed ^ stream, pre-mixed
        uint64_t counter = 0;

        Draw (uint64_t seed, uint64_t stream, uint64_t eventCounter) noexcept
            : key (mix (seed ^ stream)), counter (eventCounter) {}

        float nextFloat (uint32_t slot) const noexcept        { return uniform (key, counter, slot); }
        int   nextInt (uint32_t slot, int n) const noexcept   { return uniformInt (key, counter, slot, n); }
    };
}
