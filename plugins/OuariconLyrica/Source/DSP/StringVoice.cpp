/*
  ==============================================================================

    StringVoice.cpp
    Basic Karplus-Strong string model for Phase 2.1
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "StringVoice.h"

StringVoice::StringVoice()
{
}

StringVoice::~StringVoice()
{
}

void StringVoice::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    // Maximum delay for lowest MIDI note (A0 = 27.5 Hz)
    // Add safety margin for pitch bend
    int maxDelaySamples = static_cast<int>(sampleRate / 20.0) + 100;

    delayLine.prepare(sampleRate, maxDelaySamples);

    // Prepare brightness filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    brightnessFilter.prepare(spec);
    brightnessFilter.reset();

    updateFilters();
    reset();
}

void StringVoice::trigger(double frequency, float velocity)
{
    currentFrequency = frequency;

    // Calculate delay line length for this frequency
    float delayInSamples = static_cast<float>(currentSampleRate / frequency);
    delayLine.setDelay(delayInSamples);

    // Clear delay line for clean start
    delayLine.clear();

    // Set excitation parameters
    excitationAmplitude = velocity;
    excitationSamplesRemaining = static_cast<int>(delayInSamples * 0.5f); // Half period burst

    // Reset filters
    filterState = 0.0f;
    brightnessFilter.reset();

    // Initialize energy tracking
    currentEnergy = velocity;

    updateFilters();
}

float StringVoice::processSample()
{
    float output = 0.0f;

    // Generate excitation (noise burst during initial attack)
    float excitation = generateExcitation();

    // Read delayed sample from delay line
    float delayedSample = delayLine.popSample();

    // Karplus-Strong: Average current and previous sample (one-pole lowpass)
    // This provides the damping that creates the plucked string decay
    filterState = filterCoefficient * delayedSample + (1.0f - filterCoefficient) * filterState;

    // Combine excitation with feedback
    float inputSample = excitation + filterState;

    // Push back into delay line
    delayLine.pushSample(inputSample);

    // Output is the delayed sample
    output = delayedSample;

    // Update energy estimate (for voice stealing and activity detection)
    currentEnergy = currentEnergy * energyDecayRate + std::abs(output) * (1.0f - energyDecayRate);

    return output;
}

bool StringVoice::isActive() const
{
    return currentEnergy > ENERGY_THRESHOLD;
}

void StringVoice::reset()
{
    delayLine.clear();
    filterState = 0.0f;
    brightnessFilter.reset();
    excitationSamplesRemaining = 0;
    excitationAmplitude = 0.0f;
    currentEnergy = 0.0f;
}

void StringVoice::setDamping(float damping)
{
    dampingAmount = juce::jlimit(0.0f, 1.0f, damping);
    updateFilters();
}

void StringVoice::setBrightness(float brightness)
{
    brightnessAmount = juce::jlimit(0.0f, 1.0f, brightness);
    updateFilters();
}

float StringVoice::generateExcitation()
{
    if (excitationSamplesRemaining <= 0)
        return 0.0f;

    excitationSamplesRemaining--;

    // Generate random noise
    float noise = random.nextFloat() * 2.0f - 1.0f; // -1.0 to +1.0

    // Scale by velocity
    noise *= excitationAmplitude;

    // Apply brightness filter to noise
    float filteredNoise = brightnessFilter.processSample(noise);

    // Envelope: taper off at end of burst
    float envelope = static_cast<float>(excitationSamplesRemaining) /
                     static_cast<float>(std::max(1, excitationSamplesRemaining + 1));

    return filteredNoise * envelope;
}

void StringVoice::updateFilters()
{
    // Update loop damping filter coefficient
    // Higher damping = less feedback = faster decay
    // Lower damping = more feedback = longer sustain
    filterCoefficient = 0.5f + dampingAmount * 0.49f; // Range: 0.5 to 0.99

    // Update brightness filter (affects excitation spectrum)
    // Brightness controls lowpass cutoff frequency
    float cutoffHz = 500.0f + brightnessAmount * 7500.0f; // 500Hz - 8kHz range

    // Design one-pole lowpass filter
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, cutoffHz);

    *brightnessFilter.coefficients = *coeffs;
}
