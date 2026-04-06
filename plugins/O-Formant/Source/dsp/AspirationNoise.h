/*
  ==============================================================================

    AspirationNoise.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice aspiration noise mixer with single-pole IIR lowpass at ~4kHz
    and SmoothedValue for breathiness parameter.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

class AspirationNoise
{
public:
    explicit AspirationNoise (int seed = 0)
        : random (seed)
    {
    }

    void prepare (double sr) noexcept
    {
        sampleRate = sr;

        // Single-pole IIR: y[n] = (1-a)*x[n] + a*y[n-1]
        // a = exp(-2*pi*cutoff/sampleRate)
        float cutoff = 4000.0f;
        lpCoeff = std::exp (-juce::MathConstants<float>::twoPi * cutoff / static_cast<float> (sr));

        breathSmoothed.reset (sr, 0.020); // 20ms ramp
        prevFilterOutput = 0.0f;
    }

    void setBreathiness (float breath) noexcept
    {
        breathSmoothed.setTargetValue (breath);
    }

    // Mix glottal sample with filtered noise based on breathiness
    inline float process (float glottalSample) noexcept
    {
        // Generate white noise [-1, 1]
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Single-pole lowpass at ~4kHz
        float filteredNoise = (1.0f - lpCoeff) * noise + lpCoeff * prevFilterOutput;
        prevFilterOutput = filteredNoise;

        // Smooth breathiness and mix
        float breath = breathSmoothed.getNextValue();
        return (1.0f - breath) * glottalSample + breath * filteredNoise;
    }

    void reset() noexcept
    {
        prevFilterOutput = 0.0f;
        breathSmoothed.setCurrentAndTargetValue (0.1f);
    }

private:
    juce::Random random;
    double sampleRate = 44100.0;
    float lpCoeff = 0.5654f;          // Default for 44100Hz
    float prevFilterOutput = 0.0f;
    juce::SmoothedValue<float> breathSmoothed { 0.1f };
};
