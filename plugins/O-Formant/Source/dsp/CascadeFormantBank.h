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
        normGainSmoothed.reset (sr, 0.010); // 10ms ramp to avoid clicks
        normGainSmoothed.setCurrentAndTargetValue (1.0f);
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
        normGainSmoothed.setCurrentAndTargetValue (normGainSmoothed.getTargetValue());
    }

    // Set number of cascade stages (5 = full cascade, 3 = hybrid F1-F3 cascade + F4-F5 parallel)
    void setNumCascadeStages (int n) noexcept
    {
        numCascade = juce::jlimit (1, 5, n);
    }

    // Update coefficients — same freq/bw/shift/spread as parallel bank
    // Cascade filters use all-pole resonators (Klatt 1980); hybrid parallel filters use BPF
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

        float maxPeakGain = 1.0f;

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

            // Estimate resonator peak gain for cascade normalization
            if (i < numCascade)
            {
                float r = std::exp (-juce::MathConstants<float>::pi * scaledBW
                                    / static_cast<float> (sr));
                float theta = juce::MathConstants<float>::twoPi * finalFreq
                              / static_cast<float> (sr);
                float sinTheta = std::abs (std::sin (theta));
                float A = 1.0f - 2.0f * r * std::cos (theta) + r * r;
                float peakGain = A / std::max (2.0f * (1.0f - r) * sinTheta, 0.01f);
                maxPeakGain = std::max (maxPeakGain, peakGain);
            }

            if (! smoothedFreq[i].isSmoothing() && ! smoothedBW[i].isSmoothing())
            {
                if (i < numCascade)
                {
                    auto coeffs = makeResonator (sr, finalFreq, scaledBW);
                    filters[i].setCoefficients (coeffs);
                }
                else
                {
                    float Q = finalFreq / std::max (scaledBW, 1.0f);
                    Q = juce::jlimit (0.5f, 25.0f, Q);
                    auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, finalFreq, Q);
                    filters[i].setCoefficients (coeffs);
                }
            }
        }

        // Cascade gain compensation: sqrt normalization allows gentle tanh saturation
        // Full 1/maxPeakGain was too aggressive (-20 to -26 dB), making output near-silent
        normGainSmoothed.setTargetValue (1.0f / std::sqrt (maxPeakGain));
    }

    // Process: first numCascade filters in series (resonators), remaining in parallel (BPF)
    inline float process (float input) noexcept
    {
        // Advance smoothed values and recompute coefficients for transitioning formants
        for (int i = 0; i < 5; ++i)
        {
            if (smoothedFreq[i].isSmoothing() || smoothedBW[i].isSmoothing())
            {
                float f = smoothedFreq[i].getNextValue();
                float bw = smoothedBW[i].getNextValue();

                if (i < numCascade)
                {
                    auto coeffs = makeResonator (sampleRate, f, bw);
                    filters[i].setCoefficients (coeffs);
                }
                else
                {
                    float Q = f / std::max (bw, 1.0f);
                    Q = juce::jlimit (0.5f, 25.0f, Q);
                    auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (
                        sampleRate, f, Q);
                    filters[i].setCoefficients (coeffs);
                }
            }
        }

        // Cascade path: chain all-pole resonators in series
        float cascadeOut = input;
        for (int i = 0; i < numCascade; ++i)
            cascadeOut = filters[i].processSample (cascadeOut);

        // Normalize cascade output by estimated peak gain to prevent saturation
        cascadeOut *= normGainSmoothed.getNextValue();

        // Parallel path: remaining filters sum independently (hybrid mode)
        float parallelOut = 0.0f;
        for (int i = numCascade; i < 5; ++i)
            parallelOut += filters[i].processSample (input);

        return cascadeOut + parallelOut;
    }

private:
    // All-pole resonator for Klatt cascade synthesis (Klatt, 1980).
    // Unlike makeBandPass (which has zeros that kill out-of-band signal),
    // this adds a peak at freq without attenuating other frequencies,
    // allowing cascade of resonators at different formant frequencies.
    static std::array<float, 6> makeResonator (double sr, float freq, float bw) noexcept
    {
        float nyLimit = static_cast<float> (sr * 0.5) - 100.0f;
        freq = juce::jlimit (20.0f, nyLimit, freq);
        bw = std::max (1.0f, bw);

        float theta = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        float r = std::exp (-juce::MathConstants<float>::pi * bw / static_cast<float> (sr));
        r = std::min (r, 0.9999f);

        float cosTheta = std::cos (theta);

        // Unity DC gain: A = 1 - 2r*cos(θ) + r²
        float A = 1.0f - 2.0f * r * cosTheta + r * r;

        // {b0, b1, b2, a0, a1, a2}
        return { A, 0.0f, 0.0f, 1.0f, -2.0f * r * cosTheta, r * r };
    }

    FormantBiquad filters[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFreq[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBW[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> normGainSmoothed { 1.0f };
    int numCascade = 5;
    double sampleRate = 44100.0;
};
