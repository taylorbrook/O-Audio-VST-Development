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
    // If stiffness is zero, bypass by setting all coefficients to 0
    if (currentStiffness < 0.001f)
    {
        for (auto& stage : allpassStages)
        {
            stage.coefficient = 0.0f;
        }
        return;
    }

    // Calculate frequency-dependent scaling (bass strings are stiffer)
    float freqScaling = calculateFrequencyScaling(currentFrequency);

    // Base coefficient scales with stiffness and frequency
    // Higher stiffness = stronger phase shift = more inharmonicity
    float baseCoefficient = currentStiffness * freqScaling;

    // Set coefficients for each stage with progressive scaling
    // Each stage has slightly different coefficient for smooth dispersion curve
    // This creates the characteristic "stretched" harmonic series
    for (int i = 0; i < NUM_STAGES; ++i)
    {
        // Progressive scaling: earlier stages have stronger effect
        float stageScaling = 1.0f - (static_cast<float>(i) / NUM_STAGES) * 0.5f;

        // Map to allpass coefficient range (-1 to 1)
        // We use positive coefficients for high-frequency phase lead (sharp partials)
        float coefficient = baseCoefficient * stageScaling * 0.8f;

        allpassStages[i].coefficient = juce::jlimit(-0.9f, 0.9f, coefficient);
    }
}

float StiffnessFilter::calculateFrequencyScaling(double frequency) const
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
