/*
  ==============================================================================

    VibratoLFO.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice sine LFO with onset delay and micro-jitter.
    Returns pitch modulation in cents for application to F0.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

class VibratoLFO
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        phase = 0.0;
        delayCounter = 0;
        delaySamples = 0;
        jitterOffset = 0.0f;
        prevSinPositive = false;
    }

    void noteOn (float delayMs) noexcept
    {
        phase = 0.0;
        delayCounter = 0;
        delaySamples = static_cast<int> (delayMs * 0.001f * static_cast<float> (sampleRate));
        jitterOffset = 0.0f;
        prevSinPositive = false;
    }

    // Returns pitch modulation in cents
    inline float getNextValue (float rateHz, float depthCents) noexcept
    {
        // Onset delay ramp: linear 0->1 over delaySamples
        float delayGain = 1.0f;
        if (delayCounter < delaySamples)
        {
            delayGain = static_cast<float> (delayCounter) / static_cast<float> (juce::jmax (1, delaySamples));
            ++delayCounter;
        }

        // Advance phase accumulator
        double phaseInc = static_cast<double> (rateHz) / sampleRate;
        phase += phaseInc;

        float sinVal = std::sin (static_cast<float> (phase * juce::MathConstants<double>::twoPi));

        // Detect positive-going zero-crossing for micro-jitter
        bool currentPositive = sinVal >= 0.0f;
        if (currentPositive && ! prevSinPositive)
            jitterOffset = (random.nextFloat() - 0.5f) * 0.01f; // +/-0.5%
        prevSinPositive = currentPositive;

        // Wrap phase
        if (phase >= 1.0)
            phase -= 1.0;

        return depthCents * sinVal * delayGain;
    }

    float getJitterOffset() const noexcept { return jitterOffset; }

    void reset() noexcept
    {
        phase = 0.0;
        delayCounter = 0;
        jitterOffset = 0.0f;
        prevSinPositive = false;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    int delayCounter = 0;
    int delaySamples = 0;
    bool prevSinPositive = false;
    float jitterOffset = 0.0f;
    juce::Random random;
};
