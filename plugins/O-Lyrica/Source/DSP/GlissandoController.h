/*
   This file is part of O-Lyrica, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
 * v1.28.0: Glissando sweep direction (auto-detected from pitch relationship)
 */
enum class GlissandoDirection
{
    Ascending = 0,    // End frequency higher than start (finger pad, warmer)
    Descending = 1    // End frequency lower than start (thumb, brighter)
};

/**
 * Glissando modes
 */
enum class GlissandoMode
{
    Off = 0,          // Normal discrete notes (no glissando)
    Free = 1,         // Continuous pitch sweep (smooth ramp)
    ScaleLocked = 2   // Step through scale degrees (discrete steps)
};

/**
 * v1.22.0: Glissando shape curves for scale-locked mode
 * Controls timing distribution of notes across the glissando sweep.
 */
enum class GlissandoShape
{
    Linear = 0,       // Constant spacing (metronomic)
    Accelerate = 1,   // Starts slow, speeds up (hand gaining momentum)
    Decelerate = 2,   // Starts fast, slows into final note (landing effect)
    SCurve = 3        // Slow start, fast middle, slow end (most natural)
};

/** Convert parameter index to GlissandoShape */
inline GlissandoShape glissandoShapeFromIndex(int index)
{
    index = juce::jlimit(0, 3, index);
    return static_cast<GlissandoShape>(index);
}

/** Convert parameter index to GlissandoMode (v1.3.1 simplification) */
inline GlissandoMode glissandoModeFromIndex(int index)
{
    index = juce::jlimit(0, 2, index);
    return static_cast<GlissandoMode>(index);
}

// v1.3.2: Fixed-size array to avoid audio thread allocation
constexpr int MAX_SCALE_SIZE = 64;  // v1.23.0: Increased for up to 48-semitone custom intervals

/**
 * GlissandoController: Implements smooth and scale-locked pitch sweeps
 *
 * - Free mode: Logarithmic interpolation for perceptually uniform pitch sweep
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
     * v1.22.0: Set glissando shape for scale-locked mode
     * @param shape Acceleration curve applied to note spacing
     */
    void setShape(GlissandoShape shape);

    /**
     * v1.24.0: Set humanization amount for scale-locked timing jitter
     * @param amount 0.0 (perfectly metronomic) to 1.0 (maximum jitter)
     */
    void setHumanize(float amount);

    /**
     * v1.27.0: Set velocity profile for scale-locked glissando dynamics
     * @param startVel Velocity at start of sweep (0.0-1.0)
     * @param endVel Velocity at end of sweep (0.0-1.0)
     */
    void setVelocityProfile(float startVel, float endVel);

    /**
     * v1.27.0: Get interpolated velocity for the current scale degree position
     * Returns the velocity based on linear interpolation between start and end.
     * Only meaningful in Scale-Locked mode; returns 1.0 otherwise.
     * Updates per scale-degree step, not per sample.
     * @return Current velocity (0.0-1.0)
     */
    float getNextVelocity() const;

    /**
     * v1.25.0: Set ramp time for Free mode glissando
     * @param seconds Duration in seconds (0.01 = fast snap, 0.5 = slow portamento)
     */
    void setRampTime(float seconds);

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
     * v1.28.0: Get the auto-detected direction of the current glissando
     * @return Ascending if endFreq > startFreq, Descending otherwise
     */
    GlissandoDirection getDirection() const;

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
    GlissandoDirection direction = GlissandoDirection::Ascending; // v1.28.0
    double sampleRate = 44100.0;
    bool active = false;

    // Free mode: Logarithmic frequency sweep (v1.24.1)
    double currentFrequency = 440.0;         // Current output frequency (Hz)
    double targetFrequency = 440.0;          // Target frequency (Hz)
    double startFreqLog = 0.0;               // log2(startFreq) at glissando start
    double endFreqLog = 0.0;                 // log2(endFreq) at glissando start
    double freeProgress = 0.0;               // 0.0 to 1.0 interpolation progress
    double freeProgressIncrement = 0.0;      // Per-sample progress step
    float rampTimeSeconds = 0.05f;           // v1.25.0: Configurable ramp duration (default 50ms)

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

    // v1.22.0: Shape-curved timing for scale-locked glissandos
    GlissandoShape shape = GlissandoShape::SCurve;
    std::array<int, MAX_SCALE_SIZE> stepDurations{};  // Pre-computed per-step sample counts
    int currentStepIndex = 0;            // Which step we're on (indexes into stepDurations)

    // v1.24.0: Timing humanization for scale-locked glissandos
    float humanizeAmount = 0.0f;         // 0.0 = metronomic, 1.0 = max jitter
    juce::Random humanizeRandom;         // Per-step jitter generator

    // v1.27.0: Velocity profiling for scale-locked glissando dynamics
    int initialScaleDegree = 0;          // Starting scale degree (for normalized progress)
    float startVelocity = 0.5f;          // Velocity at start of sweep
    float endVelocity = 0.7f;            // Velocity at end of sweep
    float currentVelocity = 0.5f;        // Interpolated velocity for current step

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlissandoController)
};
