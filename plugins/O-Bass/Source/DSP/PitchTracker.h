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

    PitchTracker.h
    O-Bass - YIN-based Monophonic Pitch Detection

    Detects fundamental frequency of mono bass signal (30-200Hz range).
    Uses YIN algorithm with bass-optimized window size.

    STATUS: DISABLED - Not currently used in processing pipeline.
    The YIN algorithm has O(n²) complexity (~2M iterations per call at 48kHz)
    which overruns the audio thread, causing DAW crashes. Kept for potential
    future optimization (e.g., running on background thread with smoothed
    output). See CODE_REVIEW.md Priority 4.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

/**
 * YIN-based monophonic pitch detector optimized for bass frequencies.
 *
 * Reference: de Cheveigne & Kawahara 2002 - "YIN, a fundamental frequency
 * estimator for speech and music"
 *
 * Window size calculated for 2 periods minimum at 30Hz (lowest expected).
 * Uses parabolic interpolation for sub-sample accuracy.
 *
 * NOTE: Currently disabled due to real-time performance constraints.
 */
class PitchTracker {
public:
    PitchTracker() = default;
    ~PitchTracker() = default;

    /**
     * Prepare the pitch tracker for processing.
     * Allocates buffers - call only from prepare(), not processBlock().
     * @param sampleRate The sample rate in Hz
     * @param maxBlockSize Maximum expected block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Reset pitch tracker state.
     */
    void reset();

    /**
     * Detect fundamental frequency from input samples.
     * WARNING: O(n²) complexity - not suitable for audio thread!
     * @param input Input sample buffer (mono)
     * @param numSamples Number of samples in buffer
     * @return Detected frequency in Hz, or 0.0f if no pitch detected
     */
    float detectPitch(const float* input, int numSamples);

    /**
     * Get the last detected pitch without processing new samples.
     */
    float getLastPitch() const { return lastPitch; }

    /**
     * Get the analysis window size in samples.
     */
    int getWindowSize() const { return windowSize; }

    /**
     * Set YIN threshold (lower = stricter detection, fewer false positives).
     * Default: 0.1 (standard YIN threshold)
     * @param threshold Value between 0.0 and 1.0
     */
    void setThreshold(float threshold);

    /**
     * Get current YIN threshold.
     */
    float getThreshold() const { return yinThreshold; }

private:
    /**
     * Parabolic interpolation for sub-sample accuracy.
     * @param tau Estimated period in samples
     * @return Refined period estimate
     */
    float parabolicInterpolation(int tau) const;

    double currentSampleRate = 44100.0;
    int windowSize = 2048;
    int writeIndex = 0;
    float yinThreshold = 0.1f;
    float lastPitch = 0.0f;

    // Analysis buffers (allocated in prepare only)
    std::vector<float> buffer;     // Ring buffer for input accumulation
    std::vector<float> yinBuffer;  // Cumulative mean normalized difference

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchTracker)
};
