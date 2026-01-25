/*
  ==============================================================================

    GlissandoController.h
    Phase 2.9: Glissando Controller - Smooth and Scale-Locked Pitch Sweeps

    Implements:
    - Off mode: Normal discrete notes (pass-through)
    - Free mode: Continuous pitch sweep between notes
    - Scale-Locked mode: Steps through scale degrees from tuning system

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Glissando modes
 */
enum class GlissandoMode
{
    Off = 0,          // Normal discrete notes (no glissando)
    Free = 1,         // Continuous pitch sweep (smooth ramp)
    ScaleLocked = 2   // Step through scale degrees (discrete steps)
};

/** Convert parameter index to GlissandoMode (v1.3.1 simplification) */
inline GlissandoMode glissandoModeFromIndex(int index)
{
    index = juce::jlimit(0, 2, index);
    return static_cast<GlissandoMode>(index);
}

// v1.3.2: Fixed-size array to avoid audio thread allocation
constexpr int MAX_SCALE_SIZE = 48;  // 4 octaves of chromatic scale (generous headroom)

/**
 * GlissandoController: Implements smooth and scale-locked pitch sweeps
 *
 * - Free mode: Uses juce::SmoothedValue for continuous frequency ramp
 * - Scale-Locked mode: Steps through scale degrees at controlled speed
 * - Integrates with TuningEngine for scale data
 */
class GlissandoController
{
public:
    GlissandoController();
    ~GlissandoController() = default;

    /**
     * Prepare for playback
     * @param sampleRate Sample rate in Hz
     */
    void prepare(double sampleRate);

    /**
     * Set glissando mode
     * @param mode Off, Free, or ScaleLocked
     */
    void setMode(GlissandoMode mode);

    /**
     * Set scale frequencies for scale-locked mode
     * @param scaleFrequencies Vector of frequencies in Hz (from TuningEngine)
     */
    void setScale(const std::vector<double>& scaleFrequencies);

    /**
     * Set glissando speed for scale-locked mode
     * @param speed Notes per second (e.g., 10.0 = 10 notes per second)
     */
    void setSpeed(float speed);

    /**
     * Start a glissando from current frequency to target
     * @param startFreq Starting frequency in Hz
     * @param endFreq Target frequency in Hz
     */
    void startGlissando(double startFreq, double endFreq);

    /**
     * Get the next frequency value (per-sample query)
     * Call this once per sample to get smoothly changing frequency
     * @return Current frequency in Hz
     */
    double getNextFrequency();

    /**
     * Check if glissando is currently active
     * @return true if glissando is in progress
     */
    bool isActive() const;

    /**
     * Reset glissando state
     */
    void reset();

private:
    /**
     * Find closest scale degree to a given frequency
     * @param freq Frequency in Hz
     * @return Index of closest scale degree
     */
    int findClosestScaleDegree(double freq) const;

    /**
     * Update scale-locked glissando state (called per-sample)
     */
    void updateScaleLocked();

    // Glissando state
    GlissandoMode mode = GlissandoMode::Off;
    double sampleRate = 44100.0;
    bool active = false;

    // Free mode: Continuous sweep
    juce::SmoothedValue<double> frequencyRamp;
    double targetFrequency = 440.0;

    // Scale-Locked mode: Discrete steps through scale
    // v1.3.2: Fixed-size array replaces std::vector to avoid audio thread allocation
    std::array<double, MAX_SCALE_SIZE> scale{};  // Scale frequencies from TuningEngine
    int scaleSize = 0;                   // Actual number of valid entries in scale
    int currentScaleDegree = 0;          // Current position in scale
    int targetScaleDegree = 0;           // Target position in scale
    float speed = 10.0f;                 // Notes per second
    int samplesPerStep = 4410;           // Samples between scale degree changes
    int sampleCounter = 0;               // Counter for timing steps
    double currentScaleFrequency = 440.0; // Current frequency (for scale-locked mode)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlissandoController)
};
