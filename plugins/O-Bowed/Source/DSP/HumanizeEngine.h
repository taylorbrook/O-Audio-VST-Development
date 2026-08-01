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

    HumanizeEngine.h
    O-Bowed - Random Walk Humanization
    Ouaricon Audio

    Lowpass-filtered uniform noise driving a bounded per-parameter offset.
    Updated once per audio block at processor level, shared across all voices
    so the instrument "breathes" coherently. Walk state is persistent — never
    resets on note-on, matching the natural drift of a live player.

    Per parameter:
      * rate  (0..1) -> corner freq 0.15..8 Hz of the 1-pole smoother
      * range (0..1) -> peak target swing (scaled downstream by each bow
                        parameter's natural musical deviation)

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>
#include <juce_core/juce_core.h>

class HumanizeEngine
{
public:
    enum Index { Speed = 0, Pressure = 1, Position = 2, Rosin = 3, NumParams = 4 };

    HumanizeEngine() { rng.setSeedRandomly(); }

    void prepare (double sr, int /*maxBlockSize*/) noexcept
    {
        sampleRate = (sr > 0.0) ? sr : 44100.0;
    }

    // Called once per processBlock. Both arrays are normalised APVTS values.
    // numSamples is the ACTUAL block size for this callback (IN-07): deriving the
    // update rate from the max block size (as prepare() used to) makes the walk drift
    // faster than its labeled 0.15-8 Hz whenever the host uses a smaller buffer.
    void update (const std::array<float, NumParams>& ranges,
                 const std::array<float, NumParams>& rates,
                 int numSamples) noexcept
    {
        constexpr float twoPi = 6.28318530718f;

        const float updatesPerSecond = (numSamples > 0)
            ? static_cast<float> (sampleRate / numSamples)
            : 200.0f;

        for (int i = 0; i < NumParams; ++i)
        {
            const float rangeN = juce::jlimit (0.0f, 1.0f, ranges[(size_t) i]);
            const float rateN  = juce::jlimit (0.0f, 1.0f, rates[(size_t) i]);

            if (rangeN < 1.0e-5f)
            {
                offsets[(size_t) i] *= 0.95f; // glide to zero when disabled
                continue;
            }

            const float fcHz  = 0.15f + 7.85f * rateN;
            const float alpha = 1.0f - std::exp (-twoPi * fcHz / updatesPerSecond);

            // Gaussian-ish noise via sum of three uniforms (CLT approximation),
            // centered and scaled to roughly unit peak.
            const float n = (rng.nextFloat() + rng.nextFloat() + rng.nextFloat() - 1.5f) * (2.0f / 3.0f);

            const float target = n * rangeN;
            offsets[(size_t) i] += alpha * (target - offsets[(size_t) i]);
        }
    }

    // Normalised offset in [-1..+1] scale (before each bow parameter's own deviation gain).
    float getOffset (int paramIndex) const noexcept
    {
        return (paramIndex >= 0 && paramIndex < NumParams)
                   ? offsets[(size_t) paramIndex]
                   : 0.0f;
    }

private:
    std::array<float, NumParams> offsets { 0.0f, 0.0f, 0.0f, 0.0f };
    juce::Random rng;
    double sampleRate = 44100.0;
};
