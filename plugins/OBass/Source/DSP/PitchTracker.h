/*
  ==============================================================================

    PitchTracker.h
    OBass - YIN-based Monophonic Pitch Detection

    Detects fundamental frequency of mono bass signal (30-200Hz range).
    Uses YIN algorithm with bass-optimized window size.

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
    float yinThreshold = 0.1f;  // YIN threshold (lower = stricter)
    float lastPitch = 0.0f;

    // Analysis buffers (allocated in prepare only)
    std::vector<float> buffer;     // Ring buffer for input accumulation
    std::vector<float> yinBuffer;  // Cumulative mean normalized difference

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchTracker)
};
