/*
  ==============================================================================

    DelayLine.h
    Efficient delay line with Lagrange interpolation for pitch-accurate waveguide
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * High-performance delay line for waveguide synthesis
 * Uses power-of-2 buffer sizing and Lagrange interpolation for accurate pitch
 */
class DelayLine
{
public:
    DelayLine();
    ~DelayLine();

    /**
     * Prepare delay line for playback
     * @param sampleRate Current sample rate
     * @param maxDelayInSamples Maximum delay needed (will round up to power of 2)
     */
    void prepare(double sampleRate, int maxDelayInSamples);

    /**
     * Set delay time in samples (can be fractional for accurate pitch)
     * @param delayInSamples Delay time (supports fractional values via interpolation)
     */
    void setDelay(float delayInSamples);

    /**
     * Push a sample into the delay line
     * @param sample Input sample
     */
    void pushSample(float sample);

    /**
     * Read delayed sample with Lagrange interpolation
     * @return Interpolated delayed sample
     */
    float popSample();

    /**
     * Clear delay line buffer
     */
    void clear();

    /**
     * Get maximum delay capacity in samples
     */
    int getMaxDelay() const { return bufferSize - 1; }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    int writePosition = 0;
    float delayInSamples = 0.0f;
    double currentSampleRate = 44100.0;

    /**
     * Lagrange 3rd order interpolation for accurate fractional delay
     * @param buffer Circular buffer
     * @param readPosition Fractional read position
     * @return Interpolated sample
     */
    float lagrangeInterpolation(const float* bufferData, float readPosition);

    /**
     * Round up to next power of 2
     */
    static int nextPowerOfTwo(int value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayLine)
};
