/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

    O-Emulator — ConsoleCrossfader (Stage 2, Task 14)

    30 ms EQUAL-POWER crossfade state machine for click-safe console
    switching (FUNC-04). Equal-power (sin/cos, g² sum = 1), NOT an
    equal-gain Hann-complement pair — that pair dips audibly on the
    uncorrelated program two different console pipelines produce
    (pattern_hann_pair_is_equal_gain_not_equal_power).

    Pure audio-thread state, advanced at fixed-chunk granularity with
    per-SAMPLE gain evaluation (no zipper at chunk edges: at fade sample 1
    gOld = cos(π/2 · 1/N) ≈ 1, continuous with the pre-fade signal; gNew
    reaches exactly 1.0 at the end). The ENGINE owns which pipelines render
    — this class only owns fade position and the mix; a fade request arriving
    mid-fade is the engine's to queue (only two pipelines ever render
    concurrently).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

namespace oemu
{

class ConsoleCrossfader
{
public:
    void prepare (double hostRate)
    {
        fadeTotal = juce::jmax (1, (int) std::lround (0.030 * hostRate));
        reset();
    }

    void reset() noexcept
    {
        fadeDone = fadeTotal;
        fadingIdx = -1;
    }

    bool isFading() const noexcept { return fadingIdx >= 0; }

    /** The OLD pipeline index (the one fading out); -1 when idle. */
    int fadingIndex() const noexcept { return fadingIdx; }

    void begin (int oldPipelineIndex) noexcept
    {
        fadingIdx = oldPipelineIndex;
        fadeDone = 0;
    }

    /** Equal-power mix of the fading-out chunk into the (in-place) new
        pipeline's chunk, advancing the fade; auto-idles at the end. */
    void mixChunk (float* ioNewL, float* ioNewR,
                   const float* oldL, const float* oldR, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const double t = juce::jmin (1.0, (double) (fadeDone + i + 1)
                                                  / (double) fadeTotal);
            const double th = juce::MathConstants<double>::halfPi * t;
            const float gNew = (float) std::sin (th);
            const float gOld = (float) std::cos (th);

            ioNewL[i] = ioNewL[i] * gNew + oldL[i] * gOld;
            ioNewR[i] = ioNewR[i] * gNew + oldR[i] * gOld;
        }

        fadeDone += n;
        if (fadeDone >= fadeTotal)
            fadingIdx = -1;
    }

private:
    int fadeTotal = 1;
    int fadeDone = 1;
    int fadingIdx = -1;
};

} // namespace oemu
