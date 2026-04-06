/*
  ==============================================================================

    FormantFilterBank.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    5 parallel bandpass filters imposing vowel character on excitation signal.
    Coefficient updates at block rate via JUCE ArrayCoefficients::makeBandPass.

  ==============================================================================
*/

#pragma once
#include "FormantBiquad.h"
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

class FormantFilterBank
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

    // Update coefficients from vowel morph output + shift/spread
    void updateCoefficients (const float freq[5], const float bw[5],
                             const float gain[5], float shift, float spread,
                             double sr) noexcept
    {
        // Shift factor (semitone-based)
        float shiftFactor = std::pow (2.0f, shift / 12.0f);

        // Compute shifted frequencies
        float shiftedFreq[5];
        for (int i = 0; i < 5; ++i)
            shiftedFreq[i] = freq[i] * shiftFactor;

        // Spread: scale distance from center-of-mass
        float centerOfMass = 0.0f;
        for (int i = 0; i < 5; ++i)
            centerOfMass += shiftedFreq[i];
        centerOfMass *= 0.2f; // /5

        for (int i = 0; i < 5; ++i)
        {
            float distance = shiftedFreq[i] - centerOfMass;
            float finalFreq = centerOfMass + distance * spread;

            // Clamp frequency to safe range
            float nyquistLimit = static_cast<float> (sr * 0.5) - 100.0f;
            finalFreq = std::max (20.0f, std::min (finalFreq, nyquistLimit));

            // Q = freq / bandwidth, clamped min 0.5
            float Q = finalFreq / std::max (bw[i], 1.0f);
            Q = std::max (Q, 0.5f);

            auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, finalFreq, Q);
            filters[i].setCoefficients (coeffs);
            filters[i].gain = gain[i];
        }
    }

    // Process single sample through all 5 parallel filters, sum outputs
    inline float process (float input) noexcept
    {
        float output = 0.0f;
        for (int i = 0; i < 5; ++i)
            output += filters[i].processSample (input);
        return output;
    }

private:
    FormantBiquad filters[5];
    double sampleRate = 44100.0;
};
