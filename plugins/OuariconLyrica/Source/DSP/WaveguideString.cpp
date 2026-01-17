/*
  ==============================================================================

    WaveguideString.cpp
    Bidirectional Digital Waveguide String Model - Phase 2.2
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "WaveguideString.h"

WaveguideString::WaveguideString()
{
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
    excitationBrightnessFilter.prepare(spec);

    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    excitationBrightnessFilter.reset();

    updateFilters();
    reset();
}

void WaveguideString::trigger(double frequency, float velocity)
{
    currentFrequency = frequency;

    // Calculate delay for each rail (half of full wavelength)
    float railDelay = calculateRailDelay();

    upperRail.setDelay(railDelay);
    lowerRail.setDelay(railDelay);

    // Clear delay lines for clean start
    upperRail.reset();
    lowerRail.reset();

    // Set excitation parameters
    excitationAmplitude = velocity;
    // Excitation burst length: one period of oscillation
    excitationSamplesRemaining = static_cast<int>(currentSampleRate / frequency);

    // Reset filters
    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    excitationBrightnessFilter.reset();

    // Initialize energy tracking
    currentEnergy = velocity;

    updateFilters();
}

float WaveguideString::processSample()
{
    // Generate excitation (noise burst during initial attack)
    float excitation = generateExcitation();

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
    excitationBrightnessFilter.reset();
    excitationSamplesRemaining = 0;
    excitationAmplitude = 0.0f;
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

float WaveguideString::generateExcitation()
{
    if (excitationSamplesRemaining <= 0)
        return 0.0f;

    excitationSamplesRemaining--;

    // Generate random noise
    float noise = random.nextFloat() * 2.0f - 1.0f; // -1.0 to +1.0

    // Scale by velocity
    noise *= excitationAmplitude;

    // Apply brightness filter to noise
    float filteredNoise = excitationBrightnessFilter.processSample(noise);

    // Envelope: taper off at end of burst
    float envelope = static_cast<float>(excitationSamplesRemaining) /
                     static_cast<float>(std::max(1, excitationSamplesRemaining + 1));

    return filteredNoise * envelope;
}

void WaveguideString::updateFilters()
{
    // Bridge Filter: Frequency-dependent reflection
    // Brightness controls the cutoff frequency
    // Higher brightness = higher cutoff = less damping of high frequencies
    float bridgeCutoffHz = 1000.0f + brightnessAmount * 8000.0f; // 1kHz - 9kHz
    auto bridgeCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, bridgeCutoffHz);
    *bridgeFilter.coefficients = *bridgeCoeffs;

    // Nut Filter: Simple reflection with slight damping
    // The nut is typically a harder boundary than the bridge
    float nutCutoffHz = 5000.0f + brightnessAmount * 7000.0f; // 5kHz - 12kHz
    auto nutCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, nutCutoffHz);
    *nutFilter.coefficients = *nutCoeffs;

    // Loop Damping Filter: Material-based energy loss
    // Lower damping = more sustain
    // Higher damping = faster decay
    float dampingCutoffHz = 500.0f + (1.0f - dampingAmount) * 10000.0f; // 500Hz - 10.5kHz
    auto dampingCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, dampingCutoffHz);
    *loopDamping.coefficients = *dampingCoeffs;

    // Excitation Brightness Filter
    float excitationCutoffHz = 1000.0f + brightnessAmount * 9000.0f; // 1kHz - 10kHz
    auto excitationCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, excitationCutoffHz);
    *excitationBrightnessFilter.coefficients = *excitationCoeffs;
}

float WaveguideString::calculateRailDelay() const
{
    // Total string delay = sampleRate / frequency
    // Each rail is half the total length
    float totalDelay = static_cast<float>(currentSampleRate / currentFrequency);
    return totalDelay * 0.5f;
}
