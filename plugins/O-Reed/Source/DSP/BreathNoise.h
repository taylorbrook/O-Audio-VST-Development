/*
   This file is part of O-Reed, an Ouaricon Audio plugin.
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

    BreathNoise.h
    O-Reed - Physical Modeling Reed Wind Synthesizer
    Flow-dependent breath noise injection (bandpass-filtered white noise)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

class BreathNoise
{
public:
    explicit BreathNoise(int seed = 0)
        : noiseRng(seed)
    {
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1;

        noiseFilter.prepare(spec);
        noiseFilter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        noiseFilter.setCutoffFrequency(1700.0f);
        noiseFilter.setResonance(0.707f);

        reset();
    }

    // airNoise: 0-1 from APVTS
    // u_reed_prev: previous sample's reed flow (m^3/s)
    // Z_c: characteristic impedance (Pa*s/m^3)
    // p_mouth: current mouth pressure (Pa)
    // Returns noise pressure to add to p_mouth
    float processSample(float airNoise, float u_reed_prev, float Z_c, float p_mouth)
    {
        if (airNoise < 1e-6f)
            return 0.0f;

        // White noise [-1, 1]
        float white = noiseRng.nextFloat() * 2.0f - 1.0f;

        // Bandpass filter the noise (centered ~1700 Hz, breath turbulence band)
        float filtered = noiseFilter.processSample(0, white);

        // Flow-dependent amplitude: noise scales with reed flow
        // Normalize flow by reference level (Z_c * u at max pressure ~ 12000 Pa)
        float flowNorm = std::min(std::abs(u_reed_prev) * Z_c / 12000.0f, 1.0f);

        // Noise amplitude = airNoise * flowModulation * breathPressure * scaling
        // 0.05 scaling factor keeps noise subordinate to tonal signal
        float noisePressure = airNoise * flowNorm * std::max(p_mouth, 0.0f) * 0.05f * filtered;

        return noisePressure;
    }

    void reset()
    {
        noiseFilter.reset();
    }

private:
    juce::Random noiseRng;
    juce::dsp::StateVariableTPTFilter<float> noiseFilter;
};
