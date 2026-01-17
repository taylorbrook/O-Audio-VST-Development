/*
  ==============================================================================

    StringVoice.h
    Basic Karplus-Strong string model for Phase 2.1
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "DelayLine.h"

/**
 * Basic Karplus-Strong plucked string model
 * Phase 2.1 implementation: delay line + one-pole lowpass + noise exciter
 *
 * Phase 2.2 will upgrade to bidirectional waveguide
 */
class StringVoice
{
public:
    StringVoice();
    ~StringVoice();

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
     * Process one sample through the string model
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
     * Set damping amount (affects decay time)
     * @param damping 0.0 = fast decay, 1.0 = long sustain
     */
    void setDamping(float damping);

    /**
     * Set brightness (affects tone color)
     * @param brightness 0.0 = dark/muted, 1.0 = bright
     */
    void setBrightness(float brightness);

private:
    DelayLine delayLine;
    double currentSampleRate = 44100.0;
    double currentFrequency = 440.0;

    // Excitation state
    juce::Random random;
    int excitationSamplesRemaining = 0;
    float excitationAmplitude = 0.0f;

    // One-pole lowpass filter for damping
    float filterState = 0.0f;
    float filterCoefficient = 0.5f; // Controls damping

    // Brightness filter (affects excitation spectrum)
    juce::dsp::IIR::Filter<float> brightnessFilter;

    // Energy tracking for voice stealing
    float currentEnergy = 0.0f;
    float energyDecayRate = 0.9999f;

    // Parameters
    float dampingAmount = 0.7f;    // 0.0 - 1.0
    float brightnessAmount = 0.5f; // 0.0 - 1.0

    /**
     * Generate noise burst for excitation
     */
    float generateExcitation();

    /**
     * Update filter coefficients based on parameters
     */
    void updateFilters();

    /**
     * Energy threshold for considering voice inactive
     */
    static constexpr float ENERGY_THRESHOLD = 0.0001f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringVoice)
};
