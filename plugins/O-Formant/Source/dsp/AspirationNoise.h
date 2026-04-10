/*
  ==============================================================================

    AspirationNoise.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice pitch-synchronous aspiration noise (unfiltered white noise).
    Noise amplitude modulated by glottal cycle phase — peaks during open phase,
    dips during closed phase for organic, throaty breathiness.
    Spectral shaping is handled by the downstream formant filter bank.

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
        breathSmoothed.reset (sr, 0.020); // 20ms ramp
        glottalPhase = 0.0f;
    }

    void setBreathiness (float breath) noexcept
    {
        breathSmoothed.setTargetValue (breath);
    }

    void setGlottalPhase (float phase01) noexcept
    {
        glottalPhase = phase01;
    }

    // Mix glottal sample with pitch-synchronous noise based on breathiness
    // (no internal filtering — formant bank downstream handles spectral shaping)
    inline float process (float glottalSample) noexcept
    {
        // Generate white noise [-1, 1]
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Pitch-synchronous amplitude modulation
        // Peak during open phase (~0.0-0.6), dip during closed phase (~0.6-1.0)
        // Window centered at phase=0.3 with 30% floor
        float phaseArg = juce::MathConstants<float>::twoPi * (glottalPhase - 0.3f);
        float noiseGain = 0.3f + 0.7f * (0.5f + 0.5f * std::cos (phaseArg));
        noiseGain = juce::jlimit (0.3f, 1.0f, noiseGain);

        float modulatedNoise = noise * noiseGain;

        // Smooth breathiness and mix
        float breath = breathSmoothed.getNextValue();
        return (1.0f - breath) * glottalSample + breath * modulatedNoise;
    }

    void reset() noexcept
    {
        glottalPhase = 0.0f;
        breathSmoothed.setCurrentAndTargetValue (0.1f);
    }

private:
    juce::Random random;
    double sampleRate = 44100.0;

    float glottalPhase = 0.0f;
    juce::SmoothedValue<float> breathSmoothed { 0.1f };
};
