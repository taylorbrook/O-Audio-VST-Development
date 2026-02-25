/*
  ==============================================================================

    PluckExciter.h
    Pluck Excitation Generator - Phase 2.3
    Ouaricon Audio
    Developer: Taylor Brook

    Generates realistic pluck excitation signals with:
    - Filtered noise burst for attack
    - ADSR envelope shaping
    - Position-dependent spectral content (comb filtering)
    - Finger hardness affecting brightness
    - Velocity sensitivity
    - Playing technique variations

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * Playing techniques that modify excitation characteristics
 */
enum class PlayingTechnique
{
    Normal = 0,       // Standard pluck
    Harmonic = 1,     // Touch at node point (filtered harmonic)
    Muted = 2,        // Heavy damping, short decay
    PresDeLaTable = 3 // Close to soundboard (metallic/bright)
};

/** Convert parameter index to PlayingTechnique (v1.3.1 simplification) */
inline PlayingTechnique techniqueFromIndex(int index)
{
    index = juce::jlimit(0, 3, index);
    return static_cast<PlayingTechnique>(index);
}

/**
 * Pluck Excitation Generator
 *
 * Creates the initial excitation signal that drives waveguide string vibration.
 * Models the physical action of plucking a string with variations in:
 * - Position along string (affects harmonic content via comb filtering)
 * - Finger hardness (soft = filtered, hard = bright transient)
 * - Velocity (amplitude and brightness)
 * - Playing technique (normal, harmonic, muted, près de la table)
 */
class PluckExciter
{
public:
    PluckExciter();
    ~PluckExciter();

    /**
     * Prepare for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum expected block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Trigger pluck excitation
     * @param velocity MIDI velocity (0.0 - 1.0)
     * @param position Pluck position (0.0 = nut, 1.0 = bridge)
     * @param hardness Finger hardness (0.0 = soft/filtered, 1.0 = hard/bright)
     * @param frequency Fundamental frequency for position filter tuning
     */
    void trigger(float velocity, float position, float hardness, double frequency);

    /**
     * Set playing technique
     * @param technique Technique to use (Normal, Harmonic, Muted, PresDeLaTable)
     */
    void setTechnique(PlayingTechnique technique);

    /**
     * Set noise amount (Phase 2.5 - Material integration)
     * @param amount Noise content 0.0-1.0 (affects attack transient character)
     */
    void setNoiseAmount(float amount);

    /**
     * Process one sample through the exciter
     * @return Excitation sample (returns 0.0 when envelope complete)
     */
    float process();

    /**
     * Check if exciter is still active
     */
    bool isActive() const;

    /**
     * Reset to silent state
     */
    void reset();

private:
    // Envelope for excitation shaping
    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParams;

    // Noise source for excitation
    juce::Random noiseSource;

    // Position filter (comb filter effect based on pluck position)
    juce::dsp::IIR::Filter<float> positionFilter;

    // Brightness/hardness filter
    juce::dsp::IIR::Filter<float> brightnessFilter;

    // Technique-specific filter
    juce::dsp::IIR::Filter<float> techniqueFilter;

    // Current state
    PlayingTechnique currentTechnique = PlayingTechnique::Normal;
    double currentSampleRate = 44100.0;
    double currentFrequency = 440.0;

    // Excitation parameters
    float pluckVelocity = 0.7f;
    float pluckPosition = 0.5f;
    float fingerHardness = 0.5f;
    float noiseAmount = 0.5f;  // Material-based noise content (Phase 2.5)

    // Noise burst duration control
    int noiseBurstSamples = 0;
    int noiseBurstRemaining = 0;
    float burstAmplitude = 0.0f;

    /**
     * Update all filter coefficients based on current parameters
     */
    void updateFilters();

    /**
     * Configure envelope based on playing technique
     */
    void updateEnvelope();

    /**
     * Calculate position filter coefficients (comb filtering)
     * Creates spectral notches based on pluck position
     */
    void updatePositionFilter();

    /**
     * Calculate brightness filter based on finger hardness
     * Soft = more lowpass filtering, Hard = brighter transient
     */
    void updateBrightnessFilter();

    /**
     * Apply technique-specific filtering
     */
    void updateTechniqueFilter();

    /**
     * Generate excitation sample (v1.20.1: blend between clean impulse and noise)
     */
    float generateExcitation();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluckExciter)
};
