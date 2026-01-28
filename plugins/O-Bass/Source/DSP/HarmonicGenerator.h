/*
  ==============================================================================

    HarmonicGenerator.h
    O-Bass - Chebyshev Waveshaper for Bass Enhancement

    Generates controlled harmonics (2nd, 3rd) using Chebyshev polynomials.
    Output is bandpassed to psychoacoustically useful range (30-500Hz).

    Note: Oversampling is disabled due to JUCE compatibility issues with
    Logic Pro. See CODE_REVIEW.md Priority 4 for investigation status.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class HarmonicGenerator {
public:
    enum class Mode { LowLatency, HighFidelity };

    // DSP Constants - tuned for dramatic psychoacoustic bass enhancement
    // Input drive multiplier for soft clipping (higher = more saturation)
    static constexpr float kInputDrive = 4.0f;
    // 2nd harmonic weight - adds warmth and perceived loudness
    static constexpr float kH2Weight = 0.8f;
    // 3rd harmonic weight - adds presence and definition
    static constexpr float kH3Weight = 0.5f;
    // Overall harmonic mix level before final soft clip
    static constexpr float kHarmonicMix = 1.2f;

    HarmonicGenerator();
    ~HarmonicGenerator() = default;

    // Lifecycle
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Configuration
    void setMode(Mode newMode);
    Mode getMode() const { return activeMode.load(std::memory_order_acquire); }

    // Processing - in-place mono buffer processing
    void process(juce::AudioBuffer<float>& monoBuffer);

    // Latency reporting (currently returns 0 - oversampling disabled)
    int getLatencyInSamples() const;

private:
    std::atomic<Mode> activeMode { Mode::LowLatency };
    double sampleRate = 44100.0;
    int blockSize = 512;

    // Output bandpass filter (30-500Hz) to limit harmonics to useful range
    juce::dsp::IIR::Filter<float> outputBandpassLow;
    juce::dsp::IIR::Filter<float> outputBandpassHigh;

    // Internal processing
    void processOversampled(float* data, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicGenerator)
};
