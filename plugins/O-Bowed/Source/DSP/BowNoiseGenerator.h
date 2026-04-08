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
