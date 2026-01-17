/*
  ==============================================================================

    WaveguideString.cpp
    Bidirectional Digital Waveguide String Model - Phase 2.2-2.5
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "WaveguideString.h"

WaveguideString::WaveguideString()
{
    // Initialize with default material (Nylon)
    currentMaterial = StringMaterial::fromType(MaterialType::Nylon);
}

WaveguideString::~WaveguideString()
{
}

void WaveguideString::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    // Maximum delay for lowest MIDI note (A0 = 27.5 Hz)
    // Each rail is half the total string length
    // Add safety margin for pitch bend
    int maxDelaySamples = static_cast<int>(sampleRate / 20.0) + 100;

    upperRail.prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});
    lowerRail.prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});
    upperRail.setMaximumDelayInSamples(maxDelaySamples);
    lowerRail.setMaximumDelayInSamples(maxDelaySamples);

    // Prepare filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    bridgeFilter.prepare(spec);
    nutFilter.prepare(spec);
    loopDamping.prepare(spec);
    stiffnessFilter.prepare(sampleRate, maxBlockSize);

    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    stiffnessFilter.reset();

    // Prepare pluck exciter
    exciter.prepare(sampleRate, maxBlockSize);

    updateFilters();
    reset();
}

void WaveguideString::trigger(double frequency, float velocity, float position, float hardness)
{
    currentFrequency = frequency;
    pluckPosition = juce::jlimit(0.05f, 0.95f, position);

    // Calculate delay for each rail (half of full wavelength)
    float railDelay = calculateRailDelay();

    upperRail.setDelay(railDelay);
    lowerRail.setDelay(railDelay);

    // Clear delay lines for clean start
    upperRail.reset();
    lowerRail.reset();

    // Reset filters
    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    stiffnessFilter.reset();

    // Configure stiffness filter for this note
    stiffnessFilter.setParameters(frequency, stiffnessAmount);

    // Trigger pluck exciter
    exciter.trigger(velocity, position, hardness, frequency);

    // Initialize energy tracking
    currentEnergy = velocity;

    updateFilters();
}

float WaveguideString::processSample()
{
    // Generate excitation from PluckExciter
    float excitation = exciter.process();

    // Read from both delay lines
    float upperOut = upperRail.popSample(0);
    float lowerOut = lowerRail.popSample(0);

    // Bridge reflection: Filter upper rail output, reflect back to lower rail
    // The bridge end has frequency-dependent damping
    float bridgeReflection = bridgeFilter.processSample(upperOut);

    // Nut reflection: Invert lower rail output, reflect back to upper rail
    // The nut end inverts the wave (rigid boundary condition)
    float nutReflection = -nutFilter.processSample(lowerOut);

    // Apply loop damping to reduce overall energy
    bridgeReflection = loopDamping.processSample(bridgeReflection);
    nutReflection = loopDamping.processSample(nutReflection);

    // Apply stiffness filter (Phase 2.4: creates inharmonicity/dispersion)
    // This adds frequency-dependent phase shift, making higher harmonics sharp
    bridgeReflection = stiffnessFilter.processSample(bridgeReflection);

    // Inject excitation at pluck position
    // This creates the comb filtering effect based on pluck position
    float excitationToUpper = excitation * pluckPosition;
    float excitationToLower = excitation * (1.0f - pluckPosition);

    // Feed reflected waves back into opposite rails
    upperRail.pushSample(0, nutReflection + excitationToUpper);
    lowerRail.pushSample(0, bridgeReflection + excitationToLower);

    // Output is sum of both traveling waves
    float output = (upperOut + lowerOut) * 0.5f;

    // Update energy estimate (for voice stealing and activity detection)
    currentEnergy = currentEnergy * energyDecayRate + std::abs(output) * (1.0f - energyDecayRate);

    return output;
}

bool WaveguideString::isActive() const
{
    return currentEnergy > ENERGY_THRESHOLD;
}

void WaveguideString::reset()
{
    upperRail.reset();
    lowerRail.reset();
    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    stiffnessFilter.reset();
    exciter.reset();
    currentEnergy = 0.0f;
}

void WaveguideString::setDamping(float damping)
{
    dampingAmount = juce::jlimit(0.0f, 1.0f, damping);
    updateFilters();
}

void WaveguideString::setBrightness(float brightness)
{
    brightnessAmount = juce::jlimit(0.0f, 1.0f, brightness);
    updateFilters();
}

void WaveguideString::setPluckPosition(float position)
{
    pluckPosition = juce::jlimit(0.05f, 0.95f, position);
}

void WaveguideString::setTechnique(PlayingTechnique technique)
{
    exciter.setTechnique(technique);
}

void WaveguideString::setStiffness(float stiffness)
{
    stiffnessAmount = juce::jlimit(0.0f, 1.0f, stiffness);
    // Update stiffness filter with current frequency and new stiffness
    stiffnessFilter.setParameters(currentFrequency, stiffnessAmount);
}

void WaveguideString::setMaterial(const StringMaterial& material)
{
    currentMaterial = material;

    // Apply material properties to waveguide parameters
    // Material's dampingCoeff controls the sustain (inverse relationship)
    dampingAmount = material.dampingCoeff;

    // Material's brightnessCutoff and stiffnessAmount directly map
    // We'll blend these with user parameters in updateFilters()
    stiffnessAmount = material.stiffnessAmount;

    // Update pluck exciter with noise content
    exciter.setNoiseAmount(material.noiseContent);

    // Update all filters with new material properties
    updateFilters();

    // Update stiffness filter
    stiffnessFilter.setParameters(currentFrequency, stiffnessAmount);
}

void WaveguideString::setFrequency(double frequency)
{
    if (frequency > 20.0 && frequency < 20000.0)
    {
        currentFrequency = frequency;

        // Update delay line lengths for new frequency
        float railDelay = calculateRailDelay();
        upperRail.setDelay(railDelay);
        lowerRail.setDelay(railDelay);

        // Update stiffness filter for new frequency
        stiffnessFilter.setParameters(currentFrequency, stiffnessAmount);
    }
}

void WaveguideString::updateFilters()
{
    // Bridge Filter: Frequency-dependent reflection
    // Now uses material's brightnessCutoff as base, modulated by brightness parameter
    float materialBrightness = currentMaterial.brightnessCutoff;
    float bridgeCutoffHz = materialBrightness * (0.5f + brightnessAmount * 0.8f); // ±30% from material value
    bridgeCutoffHz = juce::jlimit(500.0f, 20000.0f, bridgeCutoffHz);

    auto bridgeCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, bridgeCutoffHz);
    *bridgeFilter.coefficients = *bridgeCoeffs;

    // Nut Filter: Simple reflection with slight damping
    // The nut is typically a harder boundary than the bridge
    float nutCutoffHz = materialBrightness * 1.2f * (0.7f + brightnessAmount * 0.5f);
    nutCutoffHz = juce::jlimit(2000.0f, 20000.0f, nutCutoffHz);

    auto nutCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, nutCutoffHz);
    *nutFilter.coefficients = *nutCoeffs;

    // Loop Damping Filter: Material-based energy loss
    // Material damping coefficient directly controls decay rate
    // Lower dampingCoeff (less damping) = more sustain
    // Higher dampingCoeff (more damping) = faster decay
    float dampingCutoffHz = 500.0f + (1.0f - currentMaterial.dampingCoeff) * 10000.0f; // 500Hz - 10.5kHz
    dampingCutoffHz = juce::jlimit(300.0f, 12000.0f, dampingCutoffHz);

    auto dampingCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, dampingCutoffHz);
    *loopDamping.coefficients = *dampingCoeffs;
}

float WaveguideString::calculateRailDelay() const
{
    // Total string delay = sampleRate / frequency
    // Each rail is half the total length
    float totalDelay = static_cast<float>(currentSampleRate / currentFrequency);
    return totalDelay * 0.5f;
}
