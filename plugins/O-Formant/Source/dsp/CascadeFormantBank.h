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
        lastTransitionTime = -1.0f;
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
        // CR-03: SmoothedValue::reset() snaps current = target, so calling it
        // every block cut every glide at the block boundary (buffer-size
        // dependent). Re-arm only when the time actually changes, and carry
        // the in-flight position so a knob move mid-glide doesn't jump.
        if (normTime == lastTransitionTime)
            return;
        lastTransitionTime = normTime;

        static constexpr float maxTimesMs[5] = { 50.0f, 80.0f, 80.0f, 120.0f, 120.0f };
        for (int i = 0; i < 5; ++i)
        {
            double timeSec = static_cast<double> (normTime * maxTimesMs[i]) * 0.001;
            rearm (smoothedFreq[i], timeSec);
            rearm (smoothedBW[i], timeSec);
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

    // Update coefficients — same freq/bw/gain/shift/spread as parallel bank
    // Cascade filters use all-pole resonators (Klatt 1980); hybrid parallel filters use BPF.
    // gain[] only reaches the hybrid parallel stages: in series, the cascade's
    // relative formant levels come from the resonators themselves.
    void updateCoefficients (const float freq[5], const float bw[5], const float gain[5],
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

            // WR-17: clamp to 0.45·sr, not Nyquist − 100 Hz. A resonator
            // parked just under Nyquist (R/L vowel, Shift +24, Spread 2 → F5
            // ≈ 21.95 kHz) is a +55 dB whistle.
            finalFreq = juce::jlimit (20.0f, maxFormantFreq (sr), finalFreq);

            float scaledBW = bw[i] * shiftFactor;

            smoothedFreq[i].setTargetValue (finalFreq);
            smoothedBW[i].setTargetValue (scaledBW);

            // WR-17: the hybrid's parallel F4/F5 were hard-wired to 0 dB, so
            // every vowel got full-level upper formants (/o/ F5 should be
            // −40 dB) and the Singer's Formant boost never reached them.
            filters[i].gain = i < numCascade ? 1.0f : gain[i];

            // Resonator peak gain for cascade normalization — WR-17: exact
            // |H(e^{jθ})| at the centre frequency. The old 2(1−r)sinθ floor
            // underestimated the peak near Nyquist.
            if (i < numCascade)
                maxPeakGain = std::max (maxPeakGain, resonatorPeakGain (sr, finalFreq, scaledBW));

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

        // Cascade gain compensation: drive tanh into moderate saturation for warmth
        // 1/maxPeakGain was -20 to -26 dB (silent), 1/sqrt was still too cold
        normGainSmoothed.setTargetValue (2.0f / std::sqrt (maxPeakGain));
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
    static float maxFormantFreq (double sr) noexcept
    {
        return static_cast<float> (sr * 0.45);
    }

    static float resonatorRadius (double sr, float bw) noexcept
    {
        float r = std::exp (-juce::MathConstants<float>::pi * std::max (1.0f, bw) / static_cast<float> (sr));
        return std::min (r, 0.9999f);
    }

    // |H(e^{jθ})| of makeResonator() at its own centre frequency
    static float resonatorPeakGain (double sr, float freq, float bw) noexcept
    {
        freq = juce::jlimit (20.0f, maxFormantFreq (sr), freq);
        const float theta = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        const float r = resonatorRadius (sr, bw);
        const float cosTheta = std::cos (theta);
        const float A = 1.0f - 2.0f * r * cosTheta + r * r;

        // Denominator 1 − 2r cosθ e^{−jθ} + r² e^{−j2θ}
        const float re = 1.0f - 2.0f * r * cosTheta * cosTheta + r * r * std::cos (2.0f * theta);
        const float im = 2.0f * r * cosTheta * std::sin (theta) - r * r * std::sin (2.0f * theta);
        return A / std::max (std::sqrt (re * re + im * im), 1.0e-6f);
    }

    static std::array<float, 6> makeResonator (double sr, float freq, float bw) noexcept
    {
        freq = juce::jlimit (20.0f, maxFormantFreq (sr), freq);

        float theta = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        float r = resonatorRadius (sr, bw);

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
    float lastTransitionTime = -1.0f;

    void rearm (juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>& sv, double timeSec) noexcept
    {
        const float current = sv.getCurrentValue();
        const float target  = sv.getTargetValue();
        sv.reset (sampleRate, timeSec);
        sv.setCurrentAndTargetValue (current);
        sv.setTargetValue (target);
    }
};
