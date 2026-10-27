/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    CounterRng.h
    Counter-based RNG (v1.6.0, same scheme as O-Freeze v2.8.0 and
    O-TextureForge v1.6.0): every draw is a pure function of (seed, counter,
    slot), so the values a grain gets can't drift with block size or with the
    order other draws happened in.

  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace CounterRng
{
    // splitmix64 finaliser
    inline uint64_t mix(uint64_t x) noexcept
    {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    inline uint64_t hash(uint64_t seed, uint64_t counter, uint32_t slot) noexcept
    {
        return mix(mix(seed ^ mix(counter)) + slot);
    }

    // Uniform in [0, 1), 24-bit resolution (exact in float).
    inline float uniform(uint64_t seed, uint64_t counter, uint32_t slot) noexcept
    {
        return static_cast<float>(hash(seed, counter, slot) >> 40) * (1.0f / 16777216.0f);
    }

    // Uniform integer in [0, n); n > 0.
    inline int uniformInt(uint64_t seed, uint64_t counter, uint32_t slot, int n) noexcept
    {
        return static_cast<int>(((hash(seed, counter, slot) >> 32) * static_cast<uint64_t>(n)) >> 32);
    }
}
