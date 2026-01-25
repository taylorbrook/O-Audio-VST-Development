/*
  ==============================================================================

    CleanModeProcessor.h
    OBass - Psychoacoustic Bass Enhancement Orchestrator

    Master class for Clean Mode that coordinates pitch tracking, harmonic
    generation, transient ducking, and spectral-aware blending. Integrates
    EnvelopeFollower, PitchTracker, and HarmonicGenerator into a cohesive
    enhancement system.

  ==============================================================================
*/

#pragma once
#include "EnvelopeFollower.h"
#include "PitchTracker.h"
#include "HarmonicGenerator.h"
#include <juce_dsp/juce_dsp.h>

/**
 * Clean Mode enhancement pipeline orchestrator.
 *
 * Signal flow:
 * 1. Pitch detection (adaptive harmonics)
 * 2. Harmonic generation (Chebyshev waveshaping + oversampling)
 * 3. Transient ducking (preserves attack character)
 * 4. Spectral-aware blending (prevents harmonic buildup)
 *
 * Modes:
 * - LowLatency: Minimal delay, IIR oversampling
 * - HighFidelity: Lookahead for cleaner transients, FIR oversampling
 */
class CleanModeProcessor {
public:
    enum class Mode { LowLatency, HighFidelity };

    CleanModeProcessor();
    ~CleanModeProcessor() = default;

    // Lifecycle
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    // Configuration
    void setMode(Mode mode);
    Mode getMode() const;
    void setEnhanceAmount(float amount);  // 0.0 - 1.0
    void setHighBandEnergy(float energy); // For spectral-aware blending
    void setIntensityScale(float scale);  // 1.0 - 2.0, for frequency-dependent boost

    // Processing
    void process(juce::AudioBuffer<float>& monoBuffer);

    // Latency
    int getLatencyInSamples() const;

private:
    // Components
    PitchTracker pitchTracker;
    HarmonicGenerator harmonicGenerator;
    EnvelopeFollower fastEnvelope;   // Transient detection (fast attack)
    EnvelopeFollower slowEnvelope;   // Average level tracking (slow attack)

    // Lookahead for High Fidelity mode
    juce::AudioBuffer<float> lookaheadBuffer;
    int lookaheadSamples = 0;
    int lookaheadWritePos = 0;

    // State
    Mode currentMode = Mode::LowLatency;
    float enhanceAmount = 0.5f;
    float highBandEnergy = 0.0f;
    float intensityScale = 1.0f;  // Frequency-dependent intensity multiplier
    double sampleRate = 44100.0;
    int maxBlockSize = 512;

    // Internal processing
    float processLookahead(float input);
    float calculateTransientDuckGain(float fastEnv, float slowEnv);
    float calculateSpectralBlend();
    float getCompressedEnhance(float rawEnhance);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CleanModeProcessor)
};
