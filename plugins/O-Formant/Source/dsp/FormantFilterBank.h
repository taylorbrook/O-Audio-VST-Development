/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

    // Configure per-formant transition ramp times
    // normTime 0-1: 0 = instant (backward compatible), 1 = max transition
    // Per-formant max times mimic articulatory dynamics:
    //   F1: 50ms (jaw — fastest), F2-F3: 80ms (tongue body), F4-F5: 120ms (slowest)
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

    // Snap all SmoothedValues to current targets (use on note onset for click-free start)
    void snapToTargets() noexcept
    {
        for (int i = 0; i < 5; ++i)
        {
            smoothedFreq[i].setCurrentAndTargetValue (smoothedFreq[i].getTargetValue());
            smoothedBW[i].setCurrentAndTargetValue (smoothedBW[i].getTargetValue());
        }
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

            // Scale bandwidth proportionally with shift to maintain consistent Q
            float scaledBW = bw[i] * shiftFactor;

            // Set SmoothedValue targets — per-sample interpolation in process()
            smoothedFreq[i].setTargetValue (finalFreq);
            smoothedBW[i].setTargetValue (scaledBW);
            filters[i].gain = gain[i];

            // When not smoothing, apply coefficients directly (zero overhead steady state)
            if (! smoothedFreq[i].isSmoothing() && ! smoothedBW[i].isSmoothing())
            {
                float Q = finalFreq / std::max (scaledBW, 1.0f);
                Q = juce::jlimit (0.5f, 25.0f, Q);
                auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, finalFreq, Q);
                filters[i].setCoefficients (coeffs);
            }
        }
    }

    // Process single sample through all 5 parallel filters, sum outputs
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

        float output = 0.0f;
        for (int i = 0; i < 5; ++i)
            output += filters[i].processSample (input);
        return output;
    }

private:
    FormantBiquad filters[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFreq[5];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBW[5];
    double sampleRate = 44100.0;
};
