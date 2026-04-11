/*
  ==============================================================================

    NasalPoleZero.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Klatt-style nasal pole-zero filter for nasal consonant (/m/ /n/ /ŋ/)
    support. Adds a fixed nasal-cavity resonator at 270 Hz plus two
    anti-formant notches whose center frequencies interpolate across the
    bilabial/alveolar/velar place axis.

    The pole-zero chain runs into a wet/dry mix controlled by nasalCoupling
    so the component is perfectly transparent when coupling = 0 (existing
    patches are unaffected).

  ==============================================================================
*/

#pragma once
#include "FormantBiquad.h"
#include <JuceHeader.h>
#include <array>
#include <cmath>

class NasalPoleZero
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;

        smoothedCoupling.reset (sr, 0.020); // 20 ms wet/dry ramp
        smoothedCoupling.setCurrentAndTargetValue (0.0f);

        smoothedAntiFreq1.reset (sr, 0.050); // 50 ms place sweep
        smoothedAntiFreq2.reset (sr, 0.050);
        smoothedAntiFreq1.setCurrentAndTargetValue (antiFreqForPlace1 (0.5f));
        smoothedAntiFreq2.setCurrentAndTargetValue (antiFreqForPlace2 (0.5f));

        // Nasal pole stays fixed at 270 Hz / BW 90 Hz (shared across all nasals)
        auto poleCoeffs = makeResonator (sr, kNasalPoleFreq, kNasalPoleBW);
        nasalPole.setCoefficients (poleCoeffs);

        updateNotchCoefficients (smoothedAntiFreq1.getCurrentValue(),
                                 smoothedAntiFreq2.getCurrentValue());

        reset();
    }

    void reset() noexcept
    {
        nasalPole.reset();
        antiFormant1.reset();
        antiFormant2.reset();
    }

    // Snap SmoothedValues to targets (use on note onset for click-free start)
    void snapToTargets() noexcept
    {
        smoothedCoupling.setCurrentAndTargetValue (smoothedCoupling.getTargetValue());
        smoothedAntiFreq1.setCurrentAndTargetValue (smoothedAntiFreq1.getTargetValue());
        smoothedAntiFreq2.setCurrentAndTargetValue (smoothedAntiFreq2.getTargetValue());
        updateNotchCoefficients (smoothedAntiFreq1.getCurrentValue(),
                                 smoothedAntiFreq2.getCurrentValue());
    }

    // Block-rate: set new coupling + place targets
    //   coupling: 0-1 velum opening (0 = transparent, 1 = full nasal murmur)
    //   place:    0 = bilabial /m/, 0.5 = alveolar /n/, 1.0 = velar /ŋ/
    void updateCoefficients (float coupling, float place, double /*sr*/) noexcept
    {
        smoothedCoupling.setTargetValue (juce::jlimit (0.0f, 1.0f, coupling));

        float clampedPlace = juce::jlimit (0.0f, 1.0f, place);
        smoothedAntiFreq1.setTargetValue (antiFreqForPlace1 (clampedPlace));
        smoothedAntiFreq2.setTargetValue (antiFreqForPlace2 (clampedPlace));
    }

    // Per-sample processing. Transparent when coupling = 0.
    inline float process (float input) noexcept
    {
        // Recompute notch coefficients when place is sweeping
        if (smoothedAntiFreq1.isSmoothing() || smoothedAntiFreq2.isSmoothing())
        {
            float af1 = smoothedAntiFreq1.getNextValue();
            float af2 = smoothedAntiFreq2.getNextValue();
            updateNotchCoefficients (af1, af2);
        }

        float wet = nasalPole.processSample (input);
        wet = antiFormant1.processSample (wet);
        wet = antiFormant2.processSample (wet);

        float mix = smoothedCoupling.getNextValue();
        return input + mix * (wet - input);
    }

    // Query (used by FormantVoice for block-rate gain/bw scaling)
    float getCurrentCoupling() const noexcept { return smoothedCoupling.getCurrentValue(); }

private:
    static constexpr float kNasalPoleFreq = 270.0f;
    static constexpr float kNasalPoleBW   = 90.0f;
    static constexpr float kNotchBW1      = 100.0f; // primary anti-formant notch BW (Hz)
    static constexpr float kNotchBW2      = 150.0f; // secondary anti-formant notch BW (Hz)

    // Anti-formant 1 frequency: bilabial 800 -> alveolar 1700 -> velar 3200
    static float antiFreqForPlace1 (float place) noexcept
    {
        if (place <= 0.5f)
            return juce::jmap (place, 0.0f, 0.5f, 800.0f, 1700.0f);
        return juce::jmap (place, 0.5f, 1.0f, 1700.0f, 3200.0f);
    }

    // Anti-formant 2 frequency: bilabial 2700 -> alveolar 3500 -> velar 4800
    static float antiFreqForPlace2 (float place) noexcept
    {
        if (place <= 0.5f)
            return juce::jmap (place, 0.0f, 0.5f, 2700.0f, 3500.0f);
        return juce::jmap (place, 0.5f, 1.0f, 3500.0f, 4800.0f);
    }

    void updateNotchCoefficients (float af1, float af2) noexcept
    {
        float nyquistLimit = static_cast<float> (sampleRate * 0.5) - 100.0f;
        af1 = juce::jlimit (20.0f, nyquistLimit, af1);
        af2 = juce::jlimit (20.0f, nyquistLimit, af2);

        float q1 = juce::jlimit (0.5f, 25.0f, af1 / kNotchBW1);
        float q2 = juce::jlimit (0.5f, 25.0f, af2 / kNotchBW2);

        auto c1 = juce::dsp::IIR::ArrayCoefficients<float>::makeNotch (sampleRate, af1, q1);
        auto c2 = juce::dsp::IIR::ArrayCoefficients<float>::makeNotch (sampleRate, af2, q2);

        antiFormant1.setCoefficients (c1);
        antiFormant2.setCoefficients (c2);
    }

    // Same all-pole formula as CascadeFormantBank::makeResonator (unity DC gain)
    static std::array<float, 6> makeResonator (double sr, float freq, float bw) noexcept
    {
        float nyLimit = static_cast<float> (sr * 0.5) - 100.0f;
        freq = juce::jlimit (20.0f, nyLimit, freq);
        bw = std::max (1.0f, bw);

        float theta = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        float r = std::exp (-juce::MathConstants<float>::pi * bw / static_cast<float> (sr));
        r = std::min (r, 0.9999f);

        float cosTheta = std::cos (theta);
        float A = 1.0f - 2.0f * r * cosTheta + r * r;

        return { A, 0.0f, 0.0f, 1.0f, -2.0f * r * cosTheta, r * r };
    }

    FormantBiquad nasalPole;
    FormantBiquad antiFormant1;
    FormantBiquad antiFormant2;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCoupling;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAntiFreq1;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAntiFreq2;

    double sampleRate = 44100.0;
};
