/*
  ==============================================================================

    WaveguideString.h
    Bidirectional Digital Waveguide String Model - Phase 2.2
    Ouaricon Audio
    Developer: Taylor Brook

    Implements true waveguide synthesis with separate upper/lower rails,
    bridge filter, nut filter, and loop damping for realistic string behavior.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * Bidirectional Digital Waveguide String Model
 *
 * Models physical wave propagation in a string using two delay lines:
 * - Upper Rail: Right-traveling waves (toward bridge)
 * - Lower Rail: Left-traveling waves (toward nut)
 *
 * Filters:
 * - Bridge Filter: Frequency-dependent reflection at bridge
 * - Nut Filter: Inverted reflection at nut (sign inversion)
 * - Loop Damping: Material-based energy loss per cycle
 */
class WaveguideString
{
public:
    WaveguideString();
    ~WaveguideString();

    /**
     * Prepare for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum expected block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Trigger string excitation
     * @param frequency Fundamental frequency in Hz
     * @param velocity Note velocity (0.0 - 1.0)
     */
    void trigger(double frequency, float velocity);

    /**
     * Process one sample through the waveguide
     * @return Output sample
     */
    float processSample();

    /**
     * Check if string is still sounding
     */
    bool isActive() const;

    /**
     * Reset string to silent state
     */
    void reset();

    /**
     * Set material-based damping (affects decay time)
     * @param damping 0.0 = fast decay, 1.0 = long sustain
     */
    void setDamping(float damping);

    /**
     * Set brightness (affects tone color via bridge filter)
     * @param brightness 0.0 = dark/muted, 1.0 = bright
     */
    void setBrightness(float brightness);

    /**
     * Set pluck position along string (0.0 = nut, 1.0 = bridge)
     * @param position Normalized position 0.0-1.0
     */
    void setPluckPosition(float position);

private:
    // Delay lines for bidirectional propagation
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> upperRail;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> lowerRail;

    // Filters
    juce::dsp::IIR::Filter<float> bridgeFilter;   // Frequency-dependent reflection
    juce::dsp::IIR::Filter<float> nutFilter;      // Inverted reflection
    juce::dsp::IIR::Filter<float> loopDamping;    // Material-based damping

    // Excitation state
    juce::Random random;
    int excitationSamplesRemaining = 0;
    float excitationAmplitude = 0.0f;

    // Brightness filter (affects excitation spectrum)
    juce::dsp::IIR::Filter<float> excitationBrightnessFilter;

    // State
    double currentSampleRate = 44100.0;
    double currentFrequency = 440.0;
    float pluckPosition = 0.5f;  // 0.0 = nut, 1.0 = bridge

    // Energy tracking for voice stealing
    float currentEnergy = 0.0f;
    float energyDecayRate = 0.9999f;

    // Parameters
    float dampingAmount = 0.7f;    // Material-based damping
    float brightnessAmount = 0.5f; // Tone brightness

    /**
     * Generate noise burst for excitation
     */
    float generateExcitation();

    /**
     * Update all filter coefficients based on current parameters
     */
    void updateFilters();

    /**
     * Calculate delay length for half the string (one rail)
     */
    float calculateRailDelay() const;

    /**
     * Energy threshold for considering voice inactive
     */
    static constexpr float ENERGY_THRESHOLD = 0.0001f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveguideString)
};
