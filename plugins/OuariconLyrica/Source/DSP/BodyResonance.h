/*
  ==============================================================================

    BodyResonance.h
    OuariconLyrica - Body Resonance Module

    Implements harp body resonance using modal synthesis (5 bandpass filters)
    modeling soundboard vibration modes. Provides fallback for convolution IR.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

enum class WoodType
{
    Spruce = 0,
    Maple = 1,
    Exotic = 2,
    Synthetic = 3
};

/**
 * Body Resonance Module
 *
 * Models acoustic character of harp soundboard using modal synthesis.
 * 5 resonant bandpass filters simulate body vibration modes.
 *
 * Modal frequencies (base): 300, 400, 600, 900, 1200 Hz
 * bodySize parameter scales these frequencies (0.5-2.0x range)
 * woodType selects different Q and amplitude profiles
 * bodyResonance controls wet/dry mix
 */
class BodyResonance
{
public:
    BodyResonance();
    ~BodyResonance() = default;

    /**
     * Prepare for playback
     * @param sampleRate Sample rate in Hz
     * @param maxBlockSize Maximum block size in samples
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Set body parameters
     * @param size Body size (0.0-1.0, affects mode frequencies)
     * @param type Wood type (affects Q and mode amplitudes)
     * @param amount Wet/dry mix (0.0-1.0)
     */
    void setBodyParameters(float size, WoodType type, float amount);

    /**
     * Process a single sample through body resonance
     * @param input Input sample
     * @return Output sample with body resonance applied
     */
    float process(float input);

    /**
     * Reset all filters to initial state
     */
    void reset();

private:
    // Modal synthesis: 5 resonant bandpass filters
    static constexpr int NUM_MODES = 5;

    // Base modal frequencies (Hz) - concert harp body modes
    static constexpr std::array<float, NUM_MODES> BASE_FREQUENCIES = {
        300.0f,   // First mode - low fundamental
        400.0f,   // Second mode
        600.0f,   // Third mode
        900.0f,   // Fourth mode
        1200.0f   // Fifth mode - upper resonance
    };

    std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModes;
    std::array<float, NUM_MODES> modeAmplitudes;

    double currentSampleRate = 44100.0;
    float bodyAmount = 0.6f;
    float bodySize = 0.5f;
    WoodType currentWoodType = WoodType::Spruce;

    /**
     * Update filter coefficients based on current parameters
     */
    void updateFilterCoefficients();

    /**
     * Get Q factor for a specific wood type
     * @param type Wood type
     * @return Q factor (resonance sharpness)
     */
    float getQForWoodType(WoodType type) const;

    /**
     * Get resonance gain for a specific wood type
     * @param type Wood type
     * @return Linear gain for peak filter (> 1.0 for boost)
     */
    float getGainForWoodType(WoodType type) const;

    /**
     * Get mode amplitude for a specific mode index and wood type
     * @param modeIndex Mode index (0-4)
     * @param type Wood type
     * @return Amplitude scaling (0.0-1.0)
     */
    float getModeAmplitude(int modeIndex, WoodType type) const;

    /**
     * Scale frequency based on body size
     * @param baseFreq Base frequency in Hz
     * @return Scaled frequency in Hz
     */
    float scaleFrequency(float baseFreq) const;
};
