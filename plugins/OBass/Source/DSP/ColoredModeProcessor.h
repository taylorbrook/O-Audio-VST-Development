/*
  ==============================================================================

    ColoredModeProcessor.h
    OBass - Colored Mode Asymmetric Saturation

    Warm, analog-style harmonic generation using asymmetric tanh saturation.
    Unlike Clean Mode's symmetric saturation (odd harmonics), Colored Mode
    uses DC bias to generate even harmonics (2nd, 4th) characteristic of
    tube/tape saturation.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>

/**
 * Colored Mode enhancement processor with asymmetric saturation.
 *
 * Character:
 * - Asymmetric tanh saturation generates even harmonics (2nd dominant)
 * - DC bias creates tube/tape-like warmth
 * - Softer, rounder tone compared to Clean Mode's clarity
 *
 * Modes:
 * - LowLatency: Direct processing, minimal delay
 * - HighFidelity: (Future) May add oversampling for cleaner saturation
 */
class ColoredModeProcessor {
public:
    enum class Mode { LowLatency, HighFidelity };

    ColoredModeProcessor();
    ~ColoredModeProcessor() = default;

    // Lifecycle
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Configuration
    void setMode(Mode mode);
    Mode getMode() const;
    void setEnhanceAmount(float amount);  // 0.0 - 1.0

    // Processing
    void process(juce::AudioBuffer<float>& monoBuffer);

    // Latency
    int getLatencyInSamples() const;

private:
    // Asymmetric saturation core
    float asymmetricTanh(float x, float drive, float bias);

    // State
    Mode currentMode = Mode::LowLatency;
    float enhanceAmount = 0.5f;
    double sampleRate = 44100.0;

    // Saturation parameters
    float drive = 4.0f;         // 2.0 - 6.0 range based on enhance
    static constexpr float bias = 0.3f;  // DC bias for even harmonics (stronger)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColoredModeProcessor)
};
