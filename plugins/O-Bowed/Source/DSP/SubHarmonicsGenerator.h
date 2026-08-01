/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    SubHarmonicsGenerator.h
    O-Bowed - Sub-Harmonics via Asymmetric Waveshaping
    Ouaricon Audio
    Developer: Taylor Brook

    Asymmetric tanh waveshaper that generates sub-octave content
    by applying different saturation to positive and negative peaks.
    No state -- purely combinatorial.

  ==============================================================================
*/

#pragma once
#include <cmath>

class SubHarmonicsGenerator
{
public:
    // Process a single sample with sub-harmonics amount (0-1).
    // No state -- const noexcept.
    float process (float input, float amount) const noexcept
    {
        if (amount < 0.001f)
            return input;

        float depth = amount * 3.0f;

        // Asymmetric waveshaping: different saturation for +/- peaks
        float shaped;
        if (input >= 0.0f)
            shaped = std::tanh (input * (1.0f + depth));        // Harder clip on positive
        else
            shaped = std::tanh (input * (1.0f + depth * 0.3f)); // Softer clip on negative

        // Wet/dry blend
        return input + amount * (shaped - input);
    }
};
