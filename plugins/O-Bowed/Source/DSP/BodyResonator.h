/*
  ==============================================================================

    BodyResonator.h
    O-Bowed - 8-Section Parallel Peaking EQ Body Resonator
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class BodyResonator
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Called from processBlock with current APVTS values
    void setMaterial (float material);   // 0.0 - 1.0
    void setSize (float size);           // 0.0 - 1.0
    void setBodyAmount (float amount);   // 0.0 - 1.0 (dry/wet blend)

    // Per-sample stereo processing (parallel biquad bank, separate state per channel)
    void processStereo (float& left, float& right);

private:
    static constexpr int NUM_MODES = 8;
    static constexpr int NUM_PRESETS = 4;

    struct ModePreset {
        float freq[NUM_MODES];
        float q[NUM_MODES];
        float gainDb[NUM_MODES];
    };

    static const ModePreset presets[NUM_PRESETS];

    // Stereo filter banks (shared coefficients, separate filter state per channel)
    std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesL;
    std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesR;

    // Current state
    double currentSampleRate = 44100.0;
    float currentMaterial = -1.0f;   // -1 forces initial update
    float currentSize = -1.0f;

    // Normalization
    float normGain = 1.0f;
    float dryMix = 0.4f;
    float wetMix = 0.6f;

    // Precomputed gain sums for normalization
    float presetGainSums[NUM_PRESETS] {};
    float referenceGainSum = 1.0f;

    void updateCoefficients();
    static float computePresetGainSum (const ModePreset& preset);
};
