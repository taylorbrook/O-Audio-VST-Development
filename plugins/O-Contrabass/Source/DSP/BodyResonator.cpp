/*
  ==============================================================================

    BodyResonator.cpp
    O-Contrabass - 8-Mode Parallel Bandpass Body Resonator (Phase 2.5)
    Ouaricon Audio
    Developer: Taylor Brook

    Implementation per PLAN.md R37a + RESEARCH §21.2.6.
    Mono single-bank parallel bandpass using
    juce::dsp::IIR::Coefficients<float>::makeBandPass.
    Per-block coefficient recompute (CONTEXT line 152 + ARCHITECTURE §152).
    Drops O-Bowed's normGain cross-preset normalization (single-preset bass
    design has no morph drift to correct) and preset table machinery
    (single hard-coded bass mode set).

  ==============================================================================
*/

#include "BodyResonator.h"
#include <cmath>

// Out-of-class constexpr static array definitions are not required in C++17
// (inline constexpr static); JUCE 8 builds use C++17 by default.

void BodyResonator::prepare (double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (maxBlockSize),
        1u
    };

    for (size_t i = 0; i < kNumModes; ++i)
    {
        modes[i].prepare (spec);
        modes[i].reset();
    }

    // 35 Hz HP one-pole pole — exp(-2π·35/sr)
    hp35_a = std::exp (-2.0f * juce::MathConstants<float>::pi * 35.0f
                       / static_cast<float> (sampleRate));
    hp35_x1 = 0.0f;
    hp35_y1 = 0.0f;

    recomputeCoefficients();
}

void BodyResonator::reset()
{
    for (size_t i = 0; i < kNumModes; ++i)
        modes[i].reset();

    hp35_x1 = 0.0f;
    hp35_y1 = 0.0f;
}

void BodyResonator::setSize (float v) noexcept
{
    currentSize = juce::jlimit (0.0f, 1.0f, v);
}

void BodyResonator::setDamping (float v) noexcept
{
    currentDamping = juce::jlimit (0.0f, 1.0f, v);
}

void BodyResonator::setMix (float v) noexcept
{
    currentMix = juce::jlimit (0.0f, 1.0f, v);
}

void BodyResonator::recomputeCoefficients() noexcept
{
    // ARCHITECTURE §509 size scalar (frequency shift): 0.85 .. 1.15
    const float sizeScalar = 0.85f + 0.30f * currentSize;
    // ARCHITECTURE §150 Q scalar (damping): more damping → lower Q
    const float qScalar    = juce::jmax (0.15f, 1.0f - 0.85f * currentDamping);

    for (size_t i = 0; i < kNumModes; ++i)
    {
        const float fc = juce::jlimit (
            20.0f,
            static_cast<float> (currentSampleRate * 0.45),
            kDefaultFreq[i] / sizeScalar);
        const float qEff = juce::jmax (0.10f, kDefaultQ[i] * qScalar);
        // ARCHITECTURE §511 size-driven gain offset (1.5 dB per unit deviation
        // from default 0.75)
        const float gDb  = kDefaultGainDb[i] + 1.5f * (currentSize - 0.75f);
        gainLinear[i]    = juce::Decibels::decibelsToGain (gDb);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (
            currentSampleRate, fc, qEff);
        *modes[i].coefficients = *coeffs;
    }
}

void BodyResonator::processBlock (float* mono, int numSamples) noexcept
{
    // Per-block recompute (CONTEXT line 152 + ARCHITECTURE §152).
    recomputeCoefficients();

    for (int n = 0; n < numSamples; ++n)
    {
        const float x = mono[n];

        // 35 Hz HP one-pole (dry path)
        const float dry = hp35_a * (hp35_y1 + x - hp35_x1);
        hp35_x1 = x;
        hp35_y1 = dry;

        // 8-mode parallel bandpass on input (NOT on dry — wet is independent
        // bank output of raw input, mixed against HP-filtered dry per
        // ARCHITECTURE §"Body Resonator" wet/dry blend spec).
        float wet = 0.0f;
        for (size_t i = 0; i < kNumModes; ++i)
            wet += gainLinear[i] * modes[i].processSample (x);

        mono[n] = (1.0f - currentMix) * dry + currentMix * wet;
    }
}
