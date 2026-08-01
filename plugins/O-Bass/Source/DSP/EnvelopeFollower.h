/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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

    EnvelopeFollower.h
    O-Bass - Dual-Coefficient IIR Envelope Follower

    Tracks signal amplitude with configurable attack/release times.
    Used for transient detection and dynamic harmonic control.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

/**
 * Dual-coefficient IIR envelope follower.
 *
 * Uses fast attack (signal rising) and slow release (signal falling)
 * to track signal envelope with minimal latency.
 *
 * Time constants are defined as time to fall from 100% to 1%.
 */
class EnvelopeFollower {
public:
    EnvelopeFollower() = default;
    ~EnvelopeFollower() = default;

    /**
     * Prepare the envelope follower for processing.
     * @param sampleRate The sample rate in Hz
     */
    void prepare(double sampleRate);

    /**
     * Reset envelope state to zero.
     */
    void reset();

    /**
     * Set attack time in milliseconds.
     * Default: 0.5ms (fast for transient detection)
     * @param ms Attack time in milliseconds
     */
    void setAttackMs(float ms);

    /**
     * Set release time in milliseconds.
     * Default: 20ms (moderate for smooth tracking)
     * @param ms Release time in milliseconds
     */
    void setReleaseMs(float ms);

    /**
     * Get current attack time in milliseconds.
     */
    float getAttackMs() const { return attackMs; }

    /**
     * Get current release time in milliseconds.
     */
    float getReleaseMs() const { return releaseMs; }

    /**
     * Process a single sample and return envelope value.
     * @param input Input sample
     * @return Current envelope value (always >= 0)
     */
    float process(float input);

    /**
     * Alias for process() - JUCE-style naming convention.
     */
    float processSample(float input) { return process(input); }

    /**
     * Get current envelope value without processing new input.
     */
    float getCurrentEnvelope() const { return envelope; }

private:
    void updateCoefficients();

    double currentSampleRate = 44100.0;
    float attackMs = 0.5f;      // Fast attack for transient detection
    float releaseMs = 20.0f;    // Moderate release for smooth tracking

    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;
    float envelope = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFollower)
};
