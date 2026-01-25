/*
  ==============================================================================

    StiffnessFilter.h
    Stiffness & Dispersion Filter - Phase 2.4
    Ouaricon Audio
    Developer: Taylor Brook

    Implements string stiffness via allpass cascade for piano-like inharmonicity.
    Creates frequency-dependent dispersion where higher partials are detuned,
    resulting in a more realistic and complex harmonic spectrum.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * Stiffness Filter using Allpass Cascade
 *
 * Models string stiffness (inharmonicity/dispersion) via a cascade of
 * allpass filters. This creates frequency-dependent phase shift that
 * causes higher harmonics to be slightly sharp (like piano strings).
 *
 * Features:
 * - 4-stage allpass cascade for smooth dispersion
 * - Per-note scaling (bass strings exhibit more stiffness)
 * - Parameter-controlled intensity
 * - CPU-efficient design
 *
 * Algorithm based on Julius O. Smith's work on digital waveguide synthesis:
 * https://ccrma.stanford.edu/~jos/pasp/Dispersion_Filter.html
 */
class StiffnessFilter
{
public:
    StiffnessFilter();
    ~StiffnessFilter();

    /**
     * Prepare for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum expected block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Set stiffness parameters for current note
     * @param frequency Fundamental frequency in Hz (affects dispersion scaling)
     * @param stiffness Stiffness amount 0.0-1.0 (0 = flexible, 1 = stiff/piano-like)
     */
    void setParameters(double frequency, float stiffness);

    /**
     * Process one sample through the stiffness filter
     * @param input Input sample
     * @return Processed sample with dispersive phase shift
     */
    float processSample(float input);

    /**
     * Reset filter state
     */
    void reset();

private:
    /**
     * Single allpass filter stage
     * Transfer function: H(z) = (a + z^-1) / (1 + a*z^-1)
     */
    struct AllpassStage
    {
        float coefficient = 0.0f;  // Allpass coefficient (-1 to 1)
        float z1 = 0.0f;            // State variable (previous sample)

        float process(float input)
        {
            // Allpass formula: output = a*input + z1 - a*z1
            float output = coefficient * input + z1;
            z1 = input - coefficient * output;
            return output;
        }

        void reset()
        {
            z1 = 0.0f;
        }
    };

    // Allpass cascade (4 stages for smooth dispersion curve)
    static constexpr int NUM_STAGES = 4;
    std::array<AllpassStage, NUM_STAGES> allpassStages;

    // State
    double currentSampleRate = 44100.0;
    double currentFrequency = 440.0;
    float currentStiffness = 0.2f;

    /**
     * Update allpass coefficients based on current parameters
     *
     * The coefficient calculation scales with:
     * - Frequency (bass strings have more stiffness effect)
     * - User stiffness parameter
     * - Per-stage tuning for smooth dispersion curve
     */
    void updateCoefficients();

    /**
     * Calculate per-note stiffness scaling
     * Lower frequencies (bass strings) exhibit more stiffness in real instruments
     * @param frequency Fundamental frequency in Hz
     * @return Scaling factor 0.0-1.0
     */
    float calculateFrequencyScaling(double frequency) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StiffnessFilter)
};
