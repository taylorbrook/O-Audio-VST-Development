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

    BowNoiseGenerator.h
    O-Bowed - Per-Voice Bandpass Bow Noise Generator
    Ouaricon Audio
    Developer: Taylor Brook

    Bandpass-filtered noise (3464 Hz, Q=0.87) modulated by bow pressure
    and speed. Added post-body, NOT in waveguide feedback loop.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>

class BowNoiseGenerator
{
public:
    void prepare (double sampleRate, int voiceIndex) noexcept
    {
        noiseRandom.setSeed (static_cast<juce::int64> (voiceIndex * 31337));

        *bandpassFilter.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (
            sampleRate, 3464.0f, 0.87f);
    }

    float processSample (float bowPressure, float bowSpeed, float noiseAmount) noexcept
    {
        if (noiseAmount < 0.001f)
            return 0.0f;

        // Generate white noise
        float noise = noiseRandom.nextFloat() * 2.0f - 1.0f;

        // Bandpass filter the noise
        float filtered = bandpassFilter.processSample (noise);

        // Amplitude modulated by bowing intensity
        float amplitude = bowPressure * bowSpeed * noiseAmount * 0.03f;

        return filtered * amplitude;
    }

    void reset() noexcept
    {
        bandpassFilter.reset();
    }

private:
    juce::Random noiseRandom;
    juce::dsp::IIR::Filter<float> bandpassFilter;
};
