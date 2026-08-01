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

    O-simplePhysicalModelSynth - PluckExciter

    Plectrum / fingertip model: a short windowed white-noise burst (~10 ms) shaped
    by the feed-forward position comb (spectral nulls at the pluck point) and a
    brightness one-pole LPF (fingertip hardness, from Excitation Color, raised by
    velocity). The burst is fed additively into the KS loop and decays to zero —
    the string's feedback then rings and decays on its own.

    Position comb shared with Strike via PositionComb.h (O-Lyrica port). Hann burst
    window authored here so the on/off ramps inject no DC step / click.

  ==============================================================================
*/

#pragma once
#include "OnePoleLPF.h"
#include "PositionComb.h"
#include <cmath>

class PluckExciter
{
public:
    void prepare (double sr, int voiceIndex) noexcept
    {
        sampleRate = sr;
        rng.setSeed (static_cast<juce::int64> (voiceIndex * 2747 + 1));   // deterministic per voice
        reset();
    }

    void reset() noexcept
    {
        brightness.reset();
        comb.reset();
        burstRemaining = 0;
    }

    // f0 → comb delay; position 0–1; colorCutoffHz already velocity-raised; amp 0–1.
    void trigger (float f0, float position01, float colorCutoffHz, float amp) noexcept
    {
        amplitude      = amp;
        burstLength    = juce::jmax (1, static_cast<int> (sampleRate * 0.010));  // 10 ms
        burstRemaining = burstLength;
        brightness.setCutoff (juce::jlimit (200.0f, 18000.0f, colorCutoffHz), sampleRate);
        comb.setDelay (position01, f0, sampleRate);
    }

    float processSample() noexcept
    {
        float noiseSample = 0.0f;
        if (burstRemaining > 0)
        {
            const int   n = burstLength - burstRemaining;
            const float w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi
                                                     * static_cast<float> (n + 1)
                                                     / static_cast<float> (burstLength + 1)));   // Hann
            noiseSample = (rng.nextFloat() * 2.0f - 1.0f) * amplitude * w;
            --burstRemaining;
        }

        const float positionFiltered = comb.processSample (noiseSample);
        return brightness.processSample (positionFiltered);
    }

    bool isBursting() const noexcept { return burstRemaining > 0; }

private:
    juce::Random rng;
    OnePoleLPF   brightness;
    PositionComb comb;
    int    burstLength    = 0;
    int    burstRemaining = 0;
    float  amplitude      = 0.5f;
    double sampleRate     = 44100.0;
};
