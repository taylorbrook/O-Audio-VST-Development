/*
   This file is part of the Ouaricon grain-engine module.
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

    grain-engine - CounterRng

    Counter-based RNG: every draw is a pure function of (seed, counter, slot),
    so the values a grain gets can't drift with block size or with the order
    other draws happened in, and a conditional draw (a randomisation at 0)
    can't shift the values any other draw gets.

    Usage: give each event (grain, trigger) a counter — GrainScheduler's
    Onset::index is one — and each randomised quantity a fixed slot.

        const float pan = CounterRng::uniform (seed, onset.index, kSlotPan);

    Origin: the core (mix / hash / uniform / uniformInt) is bit-identical to the
    copies in O-TextureForge 1.6.0, O-Freeze 2.8.0, O-simpleGrain 1.6.0 and
    O-GrainScatter 3.1.0; tests/grain_engine_tests.cpp pins golden values taken
    from those copies. Draw is O-GrainScatter's helper, with its plugin-specific
    stream and slot enums left in the plugin. O-Polystutter 1.17.0 uses a
    different 4-argument hash (seed, stream, index, slot) and gives different
    values — it is NOT interchangeable with this one.

    Header-only, no JUCE dependency.

  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace ouaricon::grain::CounterRng
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

    // One event's draws on one stream: a fixed slot per quantity. Streams keep
    // independent consumers (scheduler vs grains) from sharing values for the
    // same counter. Draw (seed, stream, c).nextFloat (s) is exactly
    // uniform (mix (seed ^ stream), c, s).
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
