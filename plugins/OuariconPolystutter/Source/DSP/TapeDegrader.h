/*
  ==============================================================================

    TapeDegrader.h
    Ouaricon Polystutter - Tape Degradation Post-Processor
    Phase 2.4: Analog tape simulation effects

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class TapeDegrader
{
public:
    TapeDegrader();
    ~TapeDegrader() = default;

    // Prepare for playback
    void prepare(const juce::dsp::ProcessSpec& spec);

    // Reset state
    void reset();

    // Process a block of audio (in-place)
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);

    // Parameters (0-100%)
    void setSaturation(float saturationPercent);
    void setWow(float wowPercent);
    void setFlutter(float flutterPercent);
    void setHiss(float hissPercent);
    void setRolloff(float rolloffPercent);
    void setDropout(float dropoutPercent);

private:
    // Sample rate
    double sampleRate = 48000.0;

    // Parameters (normalized 0.0-1.0)
    float saturationAmount = 0.0f;
    float wowAmount = 0.0f;
    float flutterAmount = 0.0f;
    float hissAmount = 0.0f;
    float rolloffAmount = 0.0f;
    float dropoutAmount = 0.0f;

    // Saturation waveshaper
    juce::dsp::WaveShaper<float> saturationShaper;

    // Wow/Flutter LFOs (sine oscillators)
    juce::dsp::Oscillator<float> wowLFO;
    juce::dsp::Oscillator<float> flutterLFO;

    // Wow/Flutter modulation delay line (for pitch modulation)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> wowFlutterDelayLeft;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> wowFlutterDelayRight;

    // Hiss noise generator
    juce::Random noiseGenerator;
    juce::dsp::IIR::Filter<float> hissBandpassLeft;
    juce::dsp::IIR::Filter<float> hissBandpassRight;

    // Rolloff lowpass filter
    juce::dsp::IIR::Filter<float> rolloffFilterLeft;
    juce::dsp::IIR::Filter<float> rolloffFilterRight;
    float lastRolloffAmount = -1.0f;  // Cache to avoid per-block coefficient allocation

    // Dropout state
    bool isDroppedOut = false;
    int dropoutSamplesRemaining = 0;
    int dropoutFadeSamples = 0;
    int dropoutFadePosition = 0;

    // Helper functions
    static float tanhSaturation(float input, float drive);
    void updateWowFlutterLFOs();
    void updateRolloffFilter();
    void updateHissBandpass();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeDegrader)
};
