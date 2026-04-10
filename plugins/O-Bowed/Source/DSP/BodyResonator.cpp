/*
  ==============================================================================

    BodyResonator.cpp
    O-Bowed - 8-Section Parallel Peaking EQ Body Resonator
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BodyResonator.h"

//==============================================================================
// Static preset data: Membrane, Wood, Metal, Glass
const BodyResonator::ModePreset BodyResonator::presets[NUM_PRESETS] = {
    // Membrane (erhu-like)
    { { 2000.0f, 6000.0f, 10000.0f, 14000.0f, 600.0f, 1200.0f, 3500.0f, 8000.0f },
      { 8.0f, 6.0f, 4.0f, 3.0f, 5.0f, 6.0f, 4.0f, 3.0f },
      { 12.0f, 8.0f, 5.0f, 3.0f, 6.0f, 4.0f, 3.0f, 2.0f } },
    // Wood (violin)
    { { 272.0f, 462.0f, 551.0f, 2500.0f, 1200.0f, 3200.0f, 6000.0f, 800.0f },
      { 12.0f, 10.0f, 10.0f, 3.0f, 5.0f, 4.0f, 2.0f, 8.0f },
      { 10.0f, 14.0f, 12.0f, 8.0f, 4.0f, 3.0f, 2.0f, 6.0f } },
    // Metal
    { { 440.0f, 1123.0f, 1872.0f, 3100.0f, 680.0f, 1560.0f, 2400.0f, 5200.0f },
      { 25.0f, 20.0f, 15.0f, 12.0f, 18.0f, 14.0f, 10.0f, 8.0f },
      { 8.0f, 6.0f, 5.0f, 4.0f, 7.0f, 5.0f, 3.0f, 2.0f } },
    // Glass
    { { 800.0f, 2400.0f, 4800.0f, 7200.0f, 1200.0f, 3600.0f, 6000.0f, 9600.0f },
      { 30.0f, 25.0f, 20.0f, 15.0f, 28.0f, 22.0f, 18.0f, 12.0f },
      { 10.0f, 8.0f, 6.0f, 4.0f, 9.0f, 7.0f, 5.0f, 3.0f } }
};

//==============================================================================
float BodyResonator::computePresetGainSum (const ModePreset& preset)
{
    float sum = 0.0f;
    for (int i = 0; i < NUM_MODES; ++i)
        sum += juce::Decibels::decibelsToGain (preset.gainDb[i]);
    return sum;
}

//==============================================================================
void BodyResonator::prepare (double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (maxBlockSize),
        1
    };

    for (auto& filter : bodyModesL)
    {
        filter.prepare (spec);
        filter.reset();
    }
    for (auto& filter : bodyModesR)
    {
        filter.prepare (spec);
        filter.reset();
    }

    // Precompute gain sums for each preset
    for (int i = 0; i < NUM_PRESETS; ++i)
        presetGainSums[i] = computePresetGainSum (presets[i]);

    // Wood preset as the reference level
    referenceGainSum = presetGainSums[1];

    // Force coefficient update on next setMaterial/setSize call
    currentMaterial = -1.0f;
    currentSize = -1.0f;
}

//==============================================================================
void BodyResonator::reset()
{
    for (auto& filter : bodyModesL)
        filter.reset();
    for (auto& filter : bodyModesR)
        filter.reset();
}

//==============================================================================
void BodyResonator::setMaterial (float material)
{
    if (std::abs (material - currentMaterial) < 0.001f)
        return;

    currentMaterial = material;
    updateCoefficients();
}

//==============================================================================
void BodyResonator::setBodyAmount (float amount)
{
    // amount 0.0 = fully dry (no body), 1.0 = fully wet (all body resonance)
    wetMix = amount;
    dryMix = 1.0f - amount;
}

//==============================================================================
void BodyResonator::setSize (float size)
{
    if (std::abs (size - currentSize) < 0.001f)
        return;

    currentSize = size;
    updateCoefficients();
}

//==============================================================================
void BodyResonator::updateCoefficients()
{
    if (currentMaterial < 0.0f || currentSize < 0.0f)
        return;

    // Determine two adjacent presets for interpolation
    float scaled = currentMaterial * static_cast<float> (NUM_PRESETS - 1);
    int idxA = juce::jlimit (0, NUM_PRESETS - 2, static_cast<int> (scaled));
    int idxB = idxA + 1;
    float t = scaled - static_cast<float> (idxA);

    const auto& presetA = presets[idxA];
    const auto& presetB = presets[idxB];

    for (int i = 0; i < NUM_MODES; ++i)
    {
        // Log-domain frequency interpolation (perceptually linear)
        float freq = std::exp (std::log (presetA.freq[i]) * (1.0f - t)
                             + std::log (presetB.freq[i]) * t);

        // Linear Q interpolation
        float q = presetA.q[i] * (1.0f - t) + presetB.q[i] * t;

        // Linear gain interpolation (in dB domain)
        float gainDb = presetA.gainDb[i] * (1.0f - t) + presetB.gainDb[i] * t;

        // Body size scaling: size=0.5 is neutral, range is +/- 1.5 octaves
        freq *= std::pow (2.0f, (0.5f - currentSize) * 3.0f);

        // Nyquist clamp (AFTER size scaling)
        freq = juce::jlimit (20.0f, static_cast<float> (currentSampleRate * 0.45), freq);

        // Q safety clamp
        q = std::max (0.1f, q);

        // Convert gain from dB to linear for makePeakFilter
        float gainLinear = juce::Decibels::decibelsToGain (gainDb);

        // Apply shared coefficients to both L and R filter banks
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, freq, q, gainLinear);
        bodyModesL[i].coefficients = coeffs;
        bodyModesR[i].coefficients = coeffs;
    }

    // Normalization: keep output level consistent across material morphs
    float morphedSum = presetGainSums[idxA] * (1.0f - t)
                     + presetGainSums[idxB] * t;
    normGain = referenceGainSum / std::max (0.01f, morphedSum);
}

//==============================================================================
void BodyResonator::processStereo (float& left, float& right)
{
    float resonantL = 0.0f;
    float resonantR = 0.0f;

    for (int i = 0; i < NUM_MODES; ++i)
    {
        resonantL += bodyModesL[i].processSample (left);
        resonantR += bodyModesR[i].processSample (right);
    }

    // Average parallel filter outputs instead of summing them.
    // Each peaking filter outputs ~unity at non-peak frequencies,
    // so the raw sum inflates the baseline by NUM_MODES (~+18dB).
    constexpr float invModes = 1.0f / static_cast<float> (NUM_MODES);
    resonantL *= normGain * invModes;
    resonantR *= normGain * invModes;

    left  = left  * dryMix + resonantL * wetMix;
    right = right * dryMix + resonantR * wetMix;
}
