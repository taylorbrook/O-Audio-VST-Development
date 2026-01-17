/*
  ==============================================================================

    BridgeFilter.cpp
    Bridge Reflection Filter - Phase 2.2
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BridgeFilter.h"

BridgeFilter::BridgeFilter()
{
}

BridgeFilter::~BridgeFilter()
{
}

void BridgeFilter::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    filter.prepare(spec);
    filter.reset();

    updateCoefficients();
}

float BridgeFilter::processSample(float input)
{
    return filter.processSample(input);
}

void BridgeFilter::reset()
{
    filter.reset();
}

void BridgeFilter::setBrightness(float brightness)
{
    brightnessAmount = juce::jlimit(0.0f, 1.0f, brightness);
    updateCoefficients();
}

void BridgeFilter::setFundamental(double frequency)
{
    fundamentalFreq = frequency;
    updateCoefficients();
}

void BridgeFilter::updateCoefficients()
{
    // Bridge reflection characteristics:
    // - Low frequencies: Efficient reflection (minimal damping)
    // - High frequencies: Less efficient reflection (more damping)
    //
    // Cutoff frequency is scaled relative to fundamental:
    // - Bright setting: cutoff at 4-8x fundamental
    // - Dark setting: cutoff at 2-4x fundamental
    //
    // This creates natural harmonic rolloff that's musically appropriate

    float cutoffMultiplier = 2.0f + brightnessAmount * 6.0f; // 2x to 8x fundamental
    float cutoffHz = static_cast<float>(fundamentalFreq * cutoffMultiplier);

    // Clamp to reasonable range
    cutoffHz = juce::jlimit(200.0f, 12000.0f, cutoffHz);

    // Design one-pole lowpass filter
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, cutoffHz);

    *filter.coefficients = *coeffs;
}
