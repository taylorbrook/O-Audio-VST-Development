/*
  ==============================================================================

    CleanModeProcessor.h
    O-Bass - Psychoacoustic Bass Enhancement Processor

    Coordinates harmonic generation for bass enhancement. Uses Chebyshev
    waveshaping to generate musically useful harmonics.

    Note: Advanced features (pitch tracking, transient ducking, lookahead)
    are disabled due to real-time performance constraints. See CODE_REVIEW.md
    Priority 4 for investigation status.

  ==============================================================================
*/

#pragma once
#include "HarmonicGenerator.h"
#include <juce_dsp/juce_dsp.h>

/**
 * Bass enhancement processor using harmonic generation.
 *
 * Signal flow:
 * 1. Harmonic generation (Chebyshev waveshaping)
 * 2. Dry/wet mixing with enhance amount
 * 3. Soft limiting for clean output
 */
class CleanModeProcessor {
public:
    enum class Mode { LowLatency, HighFidelity };

    // DSP Constants
    // Default fundamental frequency for harmonic tuning (Hz)
    // Set to deep bass range for optimal psychoacoustic effect
    static constexpr float kDefaultFundamental = 60.0f;

    CleanModeProcessor() = default;
    ~CleanModeProcessor() = default;

    // Lifecycle
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Configuration
    void setMode(Mode mode);
    Mode getMode() const;
    void setEnhanceAmount(float amount);  // 0.0 - 1.0
    void setHighBandEnergy(float energy); // Reserved for future spectral-aware blending
    void setIntensityScale(float scale);  // 1.0 - 2.0, for frequency-dependent boost

    // Processing
    void process(juce::AudioBuffer<float>& monoBuffer);

    // Latency (currently returns 0 - no lookahead)
    int getLatencyInSamples() const;

private:
    // Components
    HarmonicGenerator harmonicGenerator;

    // State
    Mode currentMode = Mode::LowLatency;
    float enhanceAmount = 0.5f;
    float highBandEnergy = 0.0f;
    float intensityScale = 1.0f;
    double sampleRate = 44100.0;
    int maxBlockSize = 512;

    // Pre-allocated buffer for dry signal copy (avoid allocation in process)
    juce::AudioBuffer<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CleanModeProcessor)
};
