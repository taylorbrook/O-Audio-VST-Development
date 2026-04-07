/*
  ==============================================================================

    CascadeFormantBank.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Klatt-style cascade (series) formant filter topology.
    Chains biquad bandpass filters in series — output of F1 feeds F2, etc.
    Cascade topology automatically produces correct relative formant amplitudes
    without requiring per-formant gain control (Klatt, 1980).

    Supports hybrid mode: first N filters in cascade, remaining in parallel.

  ==============================================================================
*/

#pragma once
#include "FormantBiquad.h"
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

class CascadeFormantBank
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        reset();
    }

    void reset() noexcept
    {
        for (int i = 0; i < 5; ++i)
            filters[i].reset();
    }

    // Set number of cascade stages (5 = full cascade, 3 = hybrid F1-F3 cascade + F4-F5 parallel)
    void setNumCascadeStages (int n) noexcept
    {
        numCascade = juce::jlimit (1, 5, n);
        // Gain compensation: series filters attenuate more than parallel
        // ~2.4 dB per cascade stage empirically tuned
        compensationGain = std::pow (2.0f, numCascade * 0.4f);
    }

    // Update coefficients — same freq/bw/shift/spread as parallel bank
    // No per-filter gain: cascade auto-produces correct relative amplitudes
    void updateCoefficients (const float freq[5], const float bw[5],
                             float shift, float spread, double sr) noexcept
    {
        float shiftFactor = std::pow (2.0f, shift / 12.0f);

        float shiftedFreq[5];
        for (int i = 0; i < 5; ++i)
            shiftedFreq[i] = freq[i] * shiftFactor;

        float centerOfMass = 0.0f;
        for (int i = 0; i < 5; ++i)
            centerOfMass += shiftedFreq[i];
        centerOfMass *= 0.2f;

        for (int i = 0; i < 5; ++i)
        {
            float distance = shiftedFreq[i] - centerOfMass;
            float finalFreq = centerOfMass + distance * spread;

            float nyquistLimit = static_cast<float> (sr * 0.5) - 100.0f;
            finalFreq = std::max (20.0f, std::min (finalFreq, nyquistLimit));

            float scaledBW = bw[i] * shiftFactor;
            float Q = finalFreq / std::max (scaledBW, 1.0f);
            Q = juce::jlimit (0.5f, 25.0f, Q);

            auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, finalFreq, Q);
            filters[i].setCoefficients (coeffs);
            filters[i].gain = 1.0f; // Unity gain per stage — cascade handles amplitude naturally
        }
    }

    // Process: first numCascade filters in series, remaining in parallel
    inline float process (float input) noexcept
    {
        // Cascade path: chain filters in series
        float cascadeOut = input;
        for (int i = 0; i < numCascade; ++i)
            cascadeOut = filters[i].processSample (cascadeOut);

        // Parallel path: remaining filters sum independently (hybrid mode)
        float parallelOut = 0.0f;
        for (int i = numCascade; i < 5; ++i)
            parallelOut += filters[i].processSample (input);

        return cascadeOut * compensationGain + parallelOut;
    }

private:
    FormantBiquad filters[5];
    int numCascade = 5;
    float compensationGain = 4.0f; // ~12 dB for 5 stages
    double sampleRate = 44100.0;
};
