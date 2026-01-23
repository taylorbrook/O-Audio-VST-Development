/*
  ==============================================================================

    CrossoverFilter.h
    OBass - Dual-Mode Crossover Filter

    Splits stereo audio into low and high frequency bands with two modes:
    - LowLatency: IIR Linkwitz-Riley 24dB/oct (zero latency)
    - HighFidelity: FIR linear-phase convolution (significant latency)

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

class CrossoverFilter {
public:
    enum class Mode { LowLatency, HighFidelity };

    // FIR coefficient bank constants (Approach 3: Deferred Update)
    static constexpr float kMinFreq = 40.0f;
    static constexpr float kMaxFreq = 200.0f;
    static constexpr float kFreqStep = 5.0f;
    static constexpr int kNumPrecomputedFilters = 33;  // (200-40)/5 + 1

    CrossoverFilter();
    ~CrossoverFilter() = default;

    // Configuration
    void setMode(Mode newMode);
    void setCutoffFrequency(float freqHz);
    Mode getMode() const { return currentMode; }
    float getCutoffFrequency() const { return targetCutoffHz; }

    // Lifecycle
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Processing - splits input into low and high bands
    // lowBand and highBand must be pre-allocated to same size as input
    void process(const juce::AudioBuffer<float>& input,
                 juce::AudioBuffer<float>& lowBand,
                 juce::AudioBuffer<float>& highBand);

    // Latency reporting
    int getLatencyInSamples() const;

private:
    Mode currentMode = Mode::LowLatency;
    float targetCutoffHz = 80.0f;
    double sampleRate = 44100.0;
    int blockSize = 512;

    // IIR mode (low-latency) - Linkwitz-Riley 24dB/oct
    juce::dsp::LinkwitzRileyFilter<float> iirLowpass;
    juce::dsp::LinkwitzRileyFilter<float> iirHighpass;

    // FIR mode (linear-phase) - single Convolution, loaded at prepare() time
    juce::dsp::Convolution firLowpass;
    int firTapCount = 0;

    // Pre-computed FIR coefficient bank (allocated once in prepare)
    std::vector<std::vector<float>> firCoefficientBank;
    int currentFirIndex { 8 };   // Currently loaded index (80Hz default)
    int pendingFirIndex { 8 };   // Desired index for next prepare/mode-switch

    // Smooth frequency changes for IIR mode (avoid clicks)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedCutoff;

    // Internal helpers
    void updateIIRFilters();
    void precomputeFIRBank();
    int frequencyToIndex(float freqHz) const;
    void loadFilterAtIndex(int index);
    std::vector<float> generateWindowedSincLowpass(float cutoffHz, int numTaps);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossoverFilter)
};
