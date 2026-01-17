/*
  ==============================================================================

    WaveguideString.h
    Bidirectional Digital Waveguide String Model - Phase 2.2-2.5
    Ouaricon Audio
    Developer: Taylor Brook

    Implements true waveguide synthesis with separate upper/lower rails,
    bridge filter, nut filter, loop damping, stiffness filter, and
    material system for realistic string behavior with piano-like
    inharmonicity and material-specific timbres.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluckExciter.h"
#include "StiffnessFilter.h"
#include "StringMaterial.h"

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
 * - Stiffness Filter: Allpass cascade for inharmonicity/dispersion (Phase 2.4)
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
     * @param position Pluck position (0.0 = nut, 1.0 = bridge)
     * @param hardness Finger hardness (0.0 = soft, 1.0 = hard)
     */
    void trigger(double frequency, float velocity, float position, float hardness);

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

    /**
     * Set playing technique
     * @param technique Technique to apply (Normal, Harmonic, Muted, PresDeLaTable)
     */
    void setTechnique(PlayingTechnique technique);

    /**
     * Set string stiffness (affects inharmonicity/dispersion)
     * @param stiffness 0.0 = flexible/harmonic, 1.0 = stiff/piano-like
     */
    void setStiffness(float stiffness);

    /**
     * Set string material (Phase 2.5)
     * @param material StringMaterial with physical properties
     */
    void setMaterial(const StringMaterial& material);

    /**
     * Update frequency in real-time (for pitch bend)
     * @param frequency New frequency in Hz
     */
    void setFrequency(double frequency);

    /**
     * Get current material settings
     * @return Current StringMaterial
     */
    const StringMaterial& getMaterial() const { return currentMaterial; }

private:
    // Delay lines for bidirectional propagation
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> upperRail;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> lowerRail;

    // Filters
    juce::dsp::IIR::Filter<float> bridgeFilter;   // Frequency-dependent reflection
    juce::dsp::IIR::Filter<float> nutFilter;      // Inverted reflection
    juce::dsp::IIR::Filter<float> loopDamping;    // Material-based damping
    StiffnessFilter stiffnessFilter;              // Inharmonicity/dispersion (Phase 2.4)

    // Pluck excitation generator (Phase 2.3)
    PluckExciter exciter;

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
    float stiffnessAmount = 0.2f;  // Final computed stiffness (Phase 2.4)

    // Material-aware stiffness system (v1.0.3 fix)
    float materialStiffness = 0.2f;      // Base stiffness from material
    float userStiffnessModifier = 0.5f;  // User slider value (0-1, 0.5 = neutral)

    // Material system (Phase 2.5)
    StringMaterial currentMaterial;

    /**
     * Calculate final stiffness from material base and user modifier
     * User modifier at 0.5 = material value unchanged
     * User modifier at 0.0 = half material value
     * User modifier at 1.0 = 1.5x material value
     */
    float calculateFinalStiffness() const
    {
        return materialStiffness * (0.5f + userStiffnessModifier * 1.0f);
    }

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
