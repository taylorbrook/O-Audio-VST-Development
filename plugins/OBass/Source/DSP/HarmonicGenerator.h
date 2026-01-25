/*
  ==============================================================================

    HarmonicGenerator.h
    OBass - Chebyshev Waveshaper with 4x Oversampling

    Generates controlled harmonics (2nd-5th) using Chebyshev polynomials with
    dual oversamplers for latency mode support. Output is bandpassed to
    psychoacoustically useful range (60-400Hz).

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>

class HarmonicGenerator {
public:
    enum class Mode { LowLatency, HighFidelity };

    HarmonicGenerator();
    ~HarmonicGenerator() = default;

    // Lifecycle
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Configuration
    void setMode(Mode newMode);
    Mode getMode() const { return activeMode.load(std::memory_order_acquire); }

    void setHarmonicWeights(float h2, float h3, float h4, float h5);

    // Adaptive harmonic count based on detected fundamental
    void setAdaptiveHarmonics(float fundamentalHz);
    int getActiveHarmonicCount() const { return activeHarmonicCount; }

    // Processing - in-place mono buffer processing
    void process(juce::AudioBuffer<float>& monoBuffer);

    // Latency reporting
    int getLatencyInSamples() const;

private:
    std::atomic<Mode> activeMode { Mode::LowLatency };
    double sampleRate = 44100.0;
    int blockSize = 512;

    // Harmonic weights (default from psychoacoustic research)
    std::array<float, 4> harmonicWeights {{ 0.7f, 0.5f, 0.3f, 0.15f }};  // H2, H3, H4, H5

    // Adaptive harmonic count (adjusted based on fundamental frequency)
    int activeHarmonicCount = 5;  // Default: use all harmonics

    // Dual oversamplers for RT-safe mode switching (both always prepared)
    // Factor 2 = 4x oversampling, mono channel
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerIIR;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerFIR;

    // Output bandpass filter (60-400Hz) to limit harmonics to useful range
    juce::dsp::IIR::Filter<float> outputBandpassLow;
    juce::dsp::IIR::Filter<float> outputBandpassHigh;

    // Internal processing
    void processOversampled(float* data, int numSamples);
    juce::dsp::Oversampling<float>* getActiveOversampler() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicGenerator)
};
