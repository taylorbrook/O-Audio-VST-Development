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

    O-simpleWavetable - mid-rise bit quantizer (Stage 2.2, ARCHITECTURE 6)
    Ouaricon Audio
    Developer: Taylor Brook

    bit_depth choice index 0 = Full (early-out, bit-identical bypass);
    index i (1..14) -> 17 - i bits (16..3).

    Mid-rise: M = 2^(bits-1) steps per polarity, outputs +/-(k + 0.5)/M,
    k = 0..M-1. 3 bits -> +/-{1,3,5,7}/8 = 8 values; no zero level.
    sign(0) = +1 (also for -0.0f). NaN / overshoot -> top level, never UB.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

struct BitQuantizer
{
    int   bits = 0;        // 0 = Full
    int   Mi   = 0;
    float M    = 0.0f;
    float invM = 0.0f;

    void setChoiceIndex (int idx) noexcept
    {
        bits = idx <= 0 ? 0 : 17 - juce::jlimit (1, 14, idx);
        if (bits > 0)
        {
            Mi   = 1 << (bits - 1);
            M    = (float) Mi;
            invM = 1.0f / M;
        }
    }

    float apply (float x) const noexcept
    {
        if (bits == 0)
            return x;                                             // Full: bit-identical bypass

        const float y = std::abs (x) * M;
        const int k = (y < (float) (Mi - 1)) ? (int) y : Mi - 1;  // floor; NaN / overshoot -> top level
        const float q = ((float) k + 0.5f) * invM;
        return x < 0.0f ? -q : q;
    }
};
