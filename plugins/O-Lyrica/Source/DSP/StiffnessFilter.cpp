/*
   This file is part of O-Lyrica, an Ouaricon Audio plugin.
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

    StiffnessFilter.cpp
    Stiffness & Dispersion Filter - Phase 2.4
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "StiffnessFilter.h"

StiffnessFilter::StiffnessFilter()
{
}

StiffnessFilter::~StiffnessFilter()
{
}

void StiffnessFilter::prepare(double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    currentSampleRate = sampleRate;
    reset();
    updateCoefficients();
}

void StiffnessFilter::setParameters(double frequency, float stiffness)
{
    currentFrequency = frequency;
    currentStiffness = juce::jlimit(0.0f, 1.0f, stiffness);
    updateCoefficients();
}

float StiffnessFilter::processSample(float input)
{
    // OPTIMIZED: Unrolled 4-stage allpass cascade (no loop overhead)
    // Each stage: output = coeff * input + z1; z1 = input - coeff * output
    float output = allpassStages[0].process(input);
    output = allpassStages[1].process(output);
    output = allpassStages[2].process(output);
    output = allpassStages[3].process(output);

    // v2.1.7: Denormal flush on allpass z1 state variables. During long quiet
    // passages the recursive z1 feedback accumulates subnormal floats, which
    // the FPU handles in microcode and causes large per-sample CPU spikes.
    // Mirrors the 1e-15f threshold flush WaveguideString::processSample already
    // uses on its own output.
    for (auto& stage : allpassStages)
    {
        if (std::abs(stage.z1) < 1e-15f)
            stage.z1 = 0.0f;
    }

    if (std::abs(output) < 1e-15f)
        output = 0.0f;

    return output;
}

void StiffnessFilter::reset()
{
    for (auto& stage : allpassStages)
    {
        stage.reset();
    }
}

void StiffnessFilter::updateCoefficients()
{
    // IN-10: delegate to the shared static recipe so the group-delay estimate
    // (calculateGroupDelaySamples) can never drift from the live coefficients.
    for (int i = 0; i < NUM_STAGES; ++i)
        allpassStages[i].coefficient = calculateStageCoefficient(currentFrequency, currentStiffness, i);
}

float StiffnessFilter::calculateStageCoefficient(double frequency, float stiffness, int stageIndex)
{
    // If stiffness is zero, bypass (coefficient 0 = identity allpass)
    if (stiffness < 0.001f)
        return 0.0f;

    // Calculate frequency-dependent scaling (bass strings are stiffer)
    float freqScaling = calculateFrequencyScaling(frequency);

    // Base coefficient scales with stiffness and frequency
    // Higher stiffness = stronger phase shift = more inharmonicity
    float baseCoefficient = stiffness * freqScaling;

    // Progressive per-stage scaling: earlier stages have stronger effect,
    // creating the characteristic "stretched" harmonic series
    float stageScaling = 1.0f - (static_cast<float>(stageIndex) / NUM_STAGES) * 0.5f;

    // Map to allpass coefficient range; positive coefficients give
    // high-frequency phase lead (sharp partials)
    return juce::jlimit(-0.9f, 0.9f, baseCoefficient * stageScaling * 0.8f);
}

float StiffnessFilter::calculateGroupDelaySamples(double frequency, float stiffness)
{
    float total = 0.0f;

    for (int i = 0; i < NUM_STAGES; ++i)
    {
        float coefficient = calculateStageCoefficient(frequency, stiffness, i);

        // Group delay at DC for a first-order allpass: (1 - a) / (1 + a) samples
        if (std::abs(coefficient) > 0.001f)
            total += (1.0f - coefficient) / (1.0f + coefficient);
    }

    return total;
}

float StiffnessFilter::calculateFrequencyScaling(double frequency)
{
    // Bass strings (low frequencies) exhibit more stiffness in real instruments
    // This creates the characteristic "stretched octaves" in pianos

    // Guard against zero or negative frequency (would cause div-by-zero / NaN)
    double safeFreq = juce::jmax(20.0, frequency);

    // Reference frequency (A4 = 440 Hz)
    constexpr double referenceFreq = 440.0;

    // Calculate frequency ratio (log scale)
    double freqRatio = safeFreq / referenceFreq;

    // Scaling curve: lower frequencies have higher scaling
    // Bass notes (110 Hz / A2): scaling ≈ 1.5
    // Middle notes (440 Hz / A4): scaling ≈ 1.0
    // Treble notes (1760 Hz / A6): scaling ≈ 0.7
    float scaling = 1.0f / std::pow(static_cast<float>(freqRatio), 0.3f);

    // Clamp to reasonable range
    return juce::jlimit(0.5f, 2.0f, scaling);
}
