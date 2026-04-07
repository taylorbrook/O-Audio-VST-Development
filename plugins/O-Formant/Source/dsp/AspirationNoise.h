/*
  ==============================================================================

    AspirationNoise.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice pitch-synchronous aspiration noise with biquad bandpass at ~3kHz.
    Noise amplitude modulated by glottal cycle phase — peaks during open phase,
    dips during closed phase for organic, throaty breathiness.

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

        // Biquad bandpass: center 3kHz, Q=1.0 (BW ~3kHz)
        computeBandpassCoeffs (3000.0f, 1.0f);

        breathSmoothed.reset (sr, 0.020); // 20ms ramp
        bpZ1 = 0.0f;
        bpZ2 = 0.0f;
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

    // Mix glottal sample with pitch-synchronous filtered noise based on breathiness
    inline float process (float glottalSample) noexcept
    {
        // Generate white noise [-1, 1]
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Biquad bandpass at ~3kHz (transposed direct form II)
        float bpOut = bpB0 * noise + bpZ1;
        bpZ1 = bpB1 * noise - bpA1 * bpOut + bpZ2;
        bpZ2 = bpB2 * noise - bpA2 * bpOut;

        // Pitch-synchronous amplitude modulation
        // Peak during open phase (~0.0-0.6), dip during closed phase (~0.6-1.0)
        // Window centered at phase=0.3 with 30% floor
        float phaseArg = juce::MathConstants<float>::twoPi * (glottalPhase - 0.3f);
        float noiseGain = 0.3f + 0.7f * (0.5f + 0.5f * std::cos (phaseArg));
        noiseGain = juce::jlimit (0.3f, 1.0f, noiseGain);

        float modulatedNoise = bpOut * noiseGain;

        // Smooth breathiness and mix
        float breath = breathSmoothed.getNextValue();
        return (1.0f - breath) * glottalSample + breath * modulatedNoise;
    }

    void reset() noexcept
    {
        bpZ1 = 0.0f;
        bpZ2 = 0.0f;
        glottalPhase = 0.0f;
        breathSmoothed.setCurrentAndTargetValue (0.1f);
    }

private:
    void computeBandpassCoeffs (float centerFreq, float Q) noexcept
    {
        float w0 = juce::MathConstants<float>::twoPi * centerFreq / static_cast<float> (sampleRate);
        float alpha = std::sin (w0) / (2.0f * Q);
        float cosW0 = std::cos (w0);

        float a0 = 1.0f + alpha;
        float a0Inv = 1.0f / a0;

        bpB0 = alpha * a0Inv;
        bpB1 = 0.0f;
        bpB2 = -alpha * a0Inv;
        bpA1 = -2.0f * cosW0 * a0Inv;
        bpA2 = (1.0f - alpha) * a0Inv;
    }

    juce::Random random;
    double sampleRate = 44100.0;

    // Biquad bandpass state (transposed direct form II)
    float bpB0 = 0.0f, bpB1 = 0.0f, bpB2 = 0.0f;
    float bpA1 = 0.0f, bpA2 = 0.0f;
    float bpZ1 = 0.0f, bpZ2 = 0.0f;

    float glottalPhase = 0.0f;
    juce::SmoothedValue<float> breathSmoothed { 0.1f };
};
