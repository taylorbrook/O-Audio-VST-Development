/*
  ==============================================================================

    WaveguideString.h
    O-Bowed - Bidirectional Digital Waveguide for Bowed String
    Ouaricon Audio
    Developer: Taylor Brook

    Two DelayLine<float, Thiran> rails split at bow contact point.
    Bridge loss filter (custom one-pole) for frequency-dependent decay.
    Nut reflection is simple sign inversion (hard boundary).
    Filter group delay compensation for pitch accuracy.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>

class HyperbolicFriction;

class WaveguideString
{
public:
    // Junction state for split read/write interface
    struct JunctionState
    {
        float bridgeReflection = 0.0f;
        float nutReflection = 0.0f;
        float v_string_incoming = 0.0f;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void trigger (float frequency);       // set delay lengths for new note
    void reset();                         // clear delay lines + filter state

    // Core per-sample processing: takes bow signals + friction model, returns output sample.
    // Internally runs steps 2-8 of the algorithm.
    // KEPT for DroneStringEngine compatibility.
    float processSample (float v_bow, float F_bow,
                         const HyperbolicFriction& friction);

    // Split junction interface for advanced friction tiers.
    // readJunction: steps 2-4 (pop from delays, compute incoming velocity)
    JunctionState readJunction (float v_bow);

    // writeJunction: steps 6-8 (inject velocity, push to delays, return output)
    float writeJunction (float rho, float v_delta, const JunctionState& state);

    bool isActive() const noexcept;

    // Parameter setters (called once per block from voice)
    void setBowPosition (float beta);          // 0.02 - 0.30
    void setBrightness (float cutoffHz);       // 20 - 20000
    void setInfiniteSustain (float amount);    // 0.0 - 1.0

private:
    void updateDelayLengths();
    void updateBridgeFilterCoeffs();

    double sampleRate = 44100.0;
    float currentFrequency = 440.0f;
    float bowPosition = 0.12f;
    float brightnessHz = 8000.0f;
    float infiniteSustain = 0.0f;

    // Delay lines: Thiran interpolation for fractional delay accuracy
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> bridgeDelay { 4410 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> neckDelay { 4410 };

    // Bridge loss filter (custom one-pole: H(z) = g*(1-p) / (1 - p*z^-1))
    juce::dsp::IIR::Filter<float> bridgeLossFilter;

    // Energy tracking for voice cleanup
    float energyEstimate = 0.0f;

    // Flag for pending filter coefficient update
    bool filterDirty = true;
};
