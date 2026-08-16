/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop - WindowLut (Hann-only)

    ~40-line reduction of O-ReverseDelay's WindowLut.h (Stage-2 PLAN Task 3 /
    RESEARCH §2.2): the Hann table build + readAt (clamp, lookup, lerp) and
    nothing else. The shapes / tilt / Tukey-taper / normalisation machinery in
    the parent file serves overlapping GRAIN populations — none of it applies
    to a deterministic 2-voice crossfade.

    Crossfade gains per ARCHITECTURE (consumed from Phase 2.2's ResyncXfade):

        fadeOut = hann(0.5 + phi/2)     // monotonic falling half
        fadeIn  = hann(phi/2)           // monotonic rising half

    Equal-power by construction (sin^2 + cos^2 = 1). No normalisation
    constants: exactly two voices, deterministic gains, no population
    statistics.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <cmath>

class WindowLut
{
public:
    static constexpr int kSize = 2048;

    WindowLut() noexcept
    {
        // Bit-for-bit the O-ReverseDelay v1.0.0 Hann expression.
        constexpr float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < kSize; ++i)
        {
            const float phi = static_cast<float> (i) / static_cast<float> (kSize - 1);
            table[(size_t) i] = 0.5f * (1.0f - std::cos (twoPi * phi));
        }
    }

    /** One clamp + one table lookup + one lerp. No transcendental. */
    float readAt (float phase) const noexcept
    {
        const float p    = juce::jlimit (0.0f, 1.0f, phase);
        const float fpos = p * static_cast<float> (kSize - 1);
        const int   i0   = static_cast<int> (fpos);
        const int   i1   = juce::jmin (i0 + 1, kSize - 1);
        const float frac = fpos - static_cast<float> (i0);

        return table[(size_t) i0] + frac * (table[(size_t) i1] - table[(size_t) i0]);
    }

private:
    std::array<float, kSize> table {};
};
