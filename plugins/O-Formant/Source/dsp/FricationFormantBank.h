/*
  ==============================================================================

    FricationFormantBank.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Klatt-style parallel frication formant bank with bypass path.

    3 parallel bandpass filters at fixed Klatt frequencies:
      F3F = 2500 Hz, Q=2.0  -> /S Z tS dZ/ (post-alveolar peak)
      F4F = 3500 Hz, Q=1.8  -> /t d/ burst peak
      F6F = 6000 Hz, Q=2.5  -> /s z/ sibilant peak (wide BW ~1 kHz)

    Plus a flat bypass path for weak fricatives /f v T D/ (Klatt AB topology).

    Alternating-sign summation (eSpeak trick) creates inter-formant notches
    without explicit anti-resonators -- cheaper and more stable than pole-zero
    pairs.

    Piecewise-linear place-to-amplitude mapping routes continuous place axis
    through the bank:
       place=0.00 (Labial)       -> bypass only  (flat -> /f T/)
       place=0.25 (Dental)       -> bypass + mild F4F, F6F
       place=0.50 (Alveolar)     -> F6F dominant (-> /s/)
       place=0.75 (Post-alveolar)-> F3F dominant (-> /S/)
       place=1.00 (Velar)        -> F3F + F4F mid (-> /k/)

  ==============================================================================
*/

#pragma once
#include "FormantBiquad.h"
#include <JuceHeader.h>

class FricationFormantBank
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        a3fSmoothed.reset (sr, 0.010);
        a4fSmoothed.reset (sr, 0.010);
        a6fSmoothed.reset (sr, 0.010);
        bypassSmoothed.reset (sr, 0.010);

        // Default: bypass-only (labial/weak fricative)
        a3fSmoothed.setCurrentAndTargetValue (0.0f);
        a4fSmoothed.setCurrentAndTargetValue (0.0f);
        a6fSmoothed.setCurrentAndTargetValue (0.0f);
        bypassSmoothed.setCurrentAndTargetValue (1.0f);

        setFilterCoefficients (sr);
        reset();
    }

    void reset() noexcept
    {
        f3f.reset();
        f4f.reset();
        f6f.reset();
    }

    // Block-rate: update amplitudes from continuous place axis [0, 1]
    void setPlace (float place) noexcept
    {
        float a3, a4, a6, bypass;
        computeAmplitudes (place, a3, a4, a6, bypass);
        a3fSmoothed.setTargetValue (a3);
        a4fSmoothed.setTargetValue (a4);
        a6fSmoothed.setTargetValue (a6);
        bypassSmoothed.setTargetValue (bypass);
    }

    // Snap smoothed amplitudes to current targets (use on note onset)
    void snapToTargets() noexcept
    {
        a3fSmoothed.setCurrentAndTargetValue (a3fSmoothed.getTargetValue());
        a4fSmoothed.setCurrentAndTargetValue (a4fSmoothed.getTargetValue());
        a6fSmoothed.setCurrentAndTargetValue (a6fSmoothed.getTargetValue());
        bypassSmoothed.setCurrentAndTargetValue (bypassSmoothed.getTargetValue());
    }

    // Per-sample: process one noise sample through the bank
    // Uses eSpeak alternating-sign summation for inter-formant notches.
    inline float process (float noise) noexcept
    {
        float a3 = a3fSmoothed.getNextValue();
        float a4 = a4fSmoothed.getNextValue();
        float a6 = a6fSmoothed.getNextValue();
        float bypass = bypassSmoothed.getNextValue();

        // Alternating-sign summation (eSpeak NG klatt.c pattern):
        // creates notches between formants without explicit zeros
        float out = f6f.processSample (noise) * a6;
        out = f4f.processSample (noise) * a4 - out;
        out = f3f.processSample (noise) * a3 - out;
        out = bypass * noise - out;
        return out;
    }

private:
    void setFilterCoefficients (double sr) noexcept
    {
        auto c3 = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, 2500.0f, 2.0f);
        auto c4 = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, 3500.0f, 1.8f);
        auto c6 = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (sr, 6000.0f, 2.5f);
        f3f.setCoefficients (c3);
        f4f.setCoefficients (c4);
        f6f.setCoefficients (c6);
    }

    // 5-anchor piecewise-linear place -> amplitudes (Klatt Table III, condensed)
    static void computeAmplitudes (float place,
                                   float& a3, float& a4, float& a6, float& bypass) noexcept
    {
        // Anchor points at place = 0.0, 0.25, 0.5, 0.75, 1.0
        static constexpr float A3[5]     = { 0.0f, 0.0f, 0.0f, 1.0f, 0.5f };
        static constexpr float A4[5]     = { 0.0f, 0.3f, 0.5f, 0.6f, 0.4f };
        static constexpr float A6[5]     = { 0.0f, 0.2f, 1.0f, 0.3f, 0.0f };
        static constexpr float Bypass[5] = { 1.0f, 0.7f, 0.0f, 0.0f, 0.0f };

        place = juce::jlimit (0.0f, 1.0f, place);
        float segF = place * 4.0f;
        int idx = juce::jmin (3, static_cast<int> (segF));
        float t = segF - static_cast<float> (idx);

        a3     = A3[idx]     + t * (A3[idx + 1]     - A3[idx]);
        a4     = A4[idx]     + t * (A4[idx + 1]     - A4[idx]);
        a6     = A6[idx]     + t * (A6[idx + 1]     - A6[idx]);
        bypass = Bypass[idx] + t * (Bypass[idx + 1] - Bypass[idx]);
    }

    FormantBiquad f3f, f4f, f6f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> a3fSmoothed    { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> a4fSmoothed    { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> a6fSmoothed    { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassSmoothed { 1.0f };

    double sampleRate = 44100.0;
};
