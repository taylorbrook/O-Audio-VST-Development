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
        for (int i = 0; i < 5; ++i)
        {
            smoothedFreq[i].reset (sr, 0.0);
            smoothedBW[i].reset (sr, 0.0);
        }
        reset();
    }

    void reset() noexcept
    {
        for (int i = 0; i < 5; ++i)
            filters[i].reset();
    }

    // Configure per-formant transition ramp times (same schedule as parallel bank)
    void setTransitionTime (float normTime) noexcept
    {
        static constexpr float maxTimesMs[5] = { 50.0f, 80.0f, 80.0f, 120.0f, 120.0f };
        for (int i = 0; i < 5; ++i)
        {
            double timeSec = static_cast<double> (normTime * maxTimesMs[i]) * 0.001;
            smoothedFreq[i].reset (sampleRate, timeSec);
            smoothedBW[i].reset (sampleRate, timeSec);
        }
    }

    // Snap all SmoothedValues to current targets (use on note onset)
    void snapToTargets() noexcept
    {
        for (int i = 0; i < 5; ++i)
        {
            smoothedFreq[i].setCurrentAndTargetValue (smoothedFreq[i].getTargetValue());
            smoothedBW[i].setCurrentAndTargetValue (smoothedBW[i].getTargetValue());
        }
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

            smoothedFreq[i].setTargetValue (finalFreq);
            smoothedBW[i].setTargetValue (scaledBW);
            filters[i].gain = 1.0f;

            if (! smoothedFreq[i].isSmoothing() && ! smoothedBW[i].isSmoothing())
            {
                float Q = finalFreq / std::max (scaledBW, 1.0f);
                Q = juce::jlimit (0.5f, 25.0f, Q);
                auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, finalFreq, Q);
                filters[i].setCoefficients (coeffs);
            }
        }
    }

    // Process: first numCascade filters in series, remaining in parallel
    inline float process (float input) noexcept
    {
        // Advance smoothed values and recompute coefficients for transitioning formants
        for (int i = 0; i < 5; ++i)
        {
            if (smoothedFreq[i].isSmoothing() || smoothedBW[i].isSmoothing())
            {
                float f = smoothedFreq[i].getNextValue();
                float bw = smoothedBW[i].getNextValue();
                float Q = f / std::max (bw, 1.0f);
                Q = juce::jlimit (0.5f, 25.0f, Q);
                auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (
                    sampleRate, f, Q);
                filters[i].setCoefficients (coeffs);
            }
        }

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
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFreq[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBW[5];
    int numCascade = 5;
    float compensationGain = 4.0f; // ~12 dB for 5 stages
    double sampleRate = 44100.0;
};
