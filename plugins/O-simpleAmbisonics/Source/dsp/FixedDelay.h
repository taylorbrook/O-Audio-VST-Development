/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    FixedDelay — exact integer delay of N samples, header-only, no JUCE.

    Why not juce::dsp::DelayLine: this delay is a compile-time constant with no
    interpolation and no runtime length, so a std::array ring is exact by
    construction, allocates nothing, needs no prepare(), and has no
    push/pop lockstep trap (critical_delayline_push_without_pop_shifts_delay).

    Stage 1 uses FixedDelay<kPartition> as the pass-through delay so the host
    sees the final 64-sample latency from day one. Stage 2's DecoderMixer
    reuses the same object for the stereo-decoder path (the binaural UPOLS
    path carries the 64 samples intrinsically).

  ==============================================================================
*/

#pragma once

#include <array>
#include <cstddef>

namespace OSimpleAmbisonics::dsp
{

template <int N>
struct FixedDelay
{
    static_assert (N > 0, "FixedDelay length must be positive");

    std::array<float, (size_t) N> buf {};
    int idx = 0;

    void reset() noexcept
    {
        buf.fill (0.0f);
        idx = 0;
    }

    // Returns the sample written N calls ago (0 for the first N calls after reset).
    float process (float x) noexcept
    {
        const float y = buf[(size_t) idx];
        buf[(size_t) idx] = x;
        if (++idx == N)
            idx = 0;
        return y;
    }
};

} // namespace OSimpleAmbisonics::dsp
