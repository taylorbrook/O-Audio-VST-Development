/*
  ==============================================================================

    SympatheticStringEngine.h
    O-Bowed - Passive Karplus-Strong Sympathetic String Resonators (0-12)
    Ouaricon Audio
    Developer: Taylor Brook

    Passive KS waveguide strings excited by bridge output. Tuned to harmonic
    partials of active voices. Energy-gated for CPU efficiency. Per-string
    stereo panning.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class SympatheticStringEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Configuration
    void setCount (int count);
    void setAmount (float amount);
    void setDecay (float decay);  // 0.0-1.0 mapped to loss coefficient range

    // Recompute sympathetic tunings when active notes change
    void updateTunings (const float* fundamentals, int numFundamentals);

    // Per-sample processing: excitation in, stereo out
    struct StereoSample { float left; float right; };
    StereoSample processSample (float excitation);

private:
    static constexpr int MAX_SYMPATHETICS = 12;
    static constexpr float HALF_PI = 1.5707963f;

    struct SympatheticString {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> delay { 4410 };
        float lossCoeff = 0.9995f;
        float filterState = 0.0f;
        float frequency = 0.0f;
        float energyEstimate = 0.0f;
        float panL = 0.707f;
        float panR = 0.707f;
        bool active = false;
    };

    std::array<SympatheticString, MAX_SYMPATHETICS> strings;
    int activeCount = 0;
    float amountParam = 0.0f;
    float decayParam = 0.5f;  // 0.0-1.0 -> lossCoeff 0.990-0.9999
    double currentSampleRate = 44100.0;

    void tuneString (int index, float frequency);
    void computeHarmonics (const float* fundamentals, int numFundamentals,
                           float* outFreqs, int maxOut, int& numOut);
};
