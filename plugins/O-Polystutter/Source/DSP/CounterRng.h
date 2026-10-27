/*
   This file is part of O-Polystutter, an Ouaricon Audio plugin.
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
    Ouaricon Polystutter - Counter-based RNG
    v1.17.0: Every random draw is a pure function of (seed, stream, index, slot),
    so the result can't depend on block size or on the order of other draws
    (a sequential generator shared by two consumers interleaves per block).

  ==============================================================================
*/

#pragma once
#include <cstdint>

namespace CounterRng
{
    // splitmix64 finaliser
    inline uint64_t mix (uint64_t z) noexcept
    {
        z += 0x9E3779B97F4A7C15ull;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    inline uint64_t hash (uint64_t seed, uint64_t stream, uint64_t index, uint64_t slot) noexcept
    {
        return mix (mix (mix (mix (seed) ^ stream) ^ index) ^ slot);
    }

    // Uniform in [0, 1): top 24 bits, exactly representable as float
    inline float uniform (uint64_t seed, uint64_t stream, uint64_t index, uint64_t slot) noexcept
    {
        return static_cast<float> (hash (seed, stream, index, slot) >> 40) * (1.0f / 16777216.0f);
    }

    // Streams: lanes use their index 0-3; tape noise has its own
    enum : uint64_t { kStreamTapeHiss = 0x100, kStreamTapeDropout = 0x101 };

    // RepeatLane slots: 0 = probability gate, 1 + r = pitch offset of repeat r
    enum : uint64_t { kSlotProbability = 0, kSlotPitchBase = 1 };
}
