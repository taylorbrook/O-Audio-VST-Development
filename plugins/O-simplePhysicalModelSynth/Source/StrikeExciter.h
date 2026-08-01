/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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

    O-simplePhysicalModelSynth - StrikeExciter (band-limited raised-cosine mallet)

    A single Hann-windowed impulse of width W samples: smooth on/off so it injects
    no DC step and no alias energy (the click DSP-08 forbids). Excitation Color sets
    mallet hardness — harder = narrower W (brighter) + higher hardness-LPF cutoff;
    velocity raises both. Position comb (shared with Pluck) places the strike point.

    Built new (RESEARCH §2.1 / F3): O-Bells' "strike" is a bare 2-sample impulse
    with vestigial dead LPF fields — not reusable. The raised-cosine shape matches
    O-Bells' bloom window (BellVoice.cpp:1124).

  ==============================================================================
*/

#pragma once
#include "OnePoleLPF.h"
#include "PositionComb.h"
#include <cmath>

class StrikeExciter
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        reset();
    }

    void reset() noexcept
    {
        hardness.reset();
        comb.reset();
        burstRemaining = 0;
    }

    // color01 0–1 → mallet hardness (width + cutoff); colorCutoffHz already
    // velocity-raised; position 0–1; amp 0–1.
    void trigger (float f0, float position01, float color01, float colorCutoffHz, float amp) noexcept
    {
        amplitude = amp;
        // Harder mallet (more color) = narrower window → brighter / more impulsive.
        const float c = juce::jlimit (0.0f, 1.0f, color01);
        burstLength    = juce::jmax (2, static_cast<int> (juce::jmap (c, static_cast<float> (sampleRate * 0.004),
                                                                          static_cast<float> (sampleRate * 0.0004))));
        burstRemaining = burstLength;
        hardness.setCutoff (juce::jlimit (200.0f, 18000.0f, colorCutoffHz), sampleRate);
        comb.setDelay (position01, f0, sampleRate);
    }

    float processSample() noexcept
    {
        float s = 0.0f;
        if (burstRemaining > 0)
        {
            const int   n = burstLength - burstRemaining;
            const float w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi
                                                     * static_cast<float> (n + 1)
                                                     / static_cast<float> (burstLength + 1)));   // Hann bump
            s = amplitude * w;
            --burstRemaining;
        }

        const float positionFiltered = comb.processSample (s);
        return hardness.processSample (positionFiltered);
    }

    bool isBursting() const noexcept { return burstRemaining > 0; }

private:
    OnePoleLPF   hardness;
    PositionComb comb;
    int    burstLength    = 64;
    int    burstRemaining = 0;
    float  amplitude      = 1.0f;
    double sampleRate     = 44100.0;
};
