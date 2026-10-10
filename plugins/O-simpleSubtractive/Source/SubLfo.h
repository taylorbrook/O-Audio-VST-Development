/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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

    O-simpleSubtractive - the one global LFO (v1.7.0)

    A single free-running low-frequency oscillator shared by every voice, the
    way a classic monosynth or a Juno-class poly has ONE LFO. It is STATELESS
    on the voice side: the processor owns the running phase and advances it
    once per block; a voice evaluates the shape at

        p = phase0 + (sampleIndexInBlock) * inc

    so sixteen voices rendering the same block read the same curve at the same
    sample without a shared buffer (and so there is no per-block array to size
    against the host's block length). Sample & Hold needs a held value per
    cycle: it is derived from the absolute CYCLE INDEX through an integer hash,
    which makes it a pure function of (phase, cycle) too.

    Shapes (match the APVTS `lfoShape` choice order):
      Triangle : 0 → +1 → −1 → 0 over one cycle (sine-like; starts neutral).
      Square   : +1 for the first half, −1 for the second.
      Saw      : falls +1 → −1 then snaps back (the classic "decay" LFO).
      S&H      : one random level per cycle, in [−1, +1].

  ==============================================================================
*/

#pragma once
#include <cstdint>
#include <cmath>

namespace SubLfo
{
    enum Shape { Triangle = 0, Square = 1, Saw = 2, SampleHold = 3 };

    // Integer hash → uniform in [−1, +1). lowbias32 (Chris Wellons), chosen for
    // its avalanche on sequential inputs — consecutive cycle indices give
    // uncorrelated levels.
    inline float hashUnit (uint32_t x) noexcept
    {
        x ^= x >> 16;  x *= 0x7FEB352Du;
        x ^= x >> 15;  x *= 0x846CA68Bu;
        x ^= x >> 16;
        return (float) x * (1.0f / 2147483648.0f) - 1.0f;
    }

    // phase01 in [0,1); cycle = absolute cycle index (for S&H only).
    inline float value (int shape, double phase01, uint32_t cycle) noexcept
    {
        const float p = (float) phase01;
        switch (shape)
        {
            case Triangle:
                // 0 at p=0, +1 at 0.25, −1 at 0.75, back to 0 at 1.
                return (p < 0.25f) ? 4.0f * p
                     : (p < 0.75f) ? 2.0f - 4.0f * p
                                   : 4.0f * p - 4.0f;
            case Square:     return p < 0.5f ? 1.0f : -1.0f;
            case Saw:        return 1.0f - 2.0f * p;
            case SampleHold: return hashUnit (cycle);
            default:         return 0.0f;
        }
    }

    // The block-level state a voice needs to evaluate the LFO at any sample of
    // the current block. Filled by the processor once per block.
    struct BlockState
    {
        int      shape  = Triangle;
        double   phase0 = 0.0;   // phase at sample 0 of this block, [0,1)
        double   inc    = 0.0;   // phase increment per sample (rate / fs)
        uint32_t cycle0 = 0;     // absolute cycle index at sample 0

        // Value at block sample `n`.
        float at (int n) const noexcept
        {
            const double p = phase0 + (double) n * inc;
            const double c = std::floor (p);
            return value (shape, p - c, cycle0 + (uint32_t) c);
        }
    };
}
