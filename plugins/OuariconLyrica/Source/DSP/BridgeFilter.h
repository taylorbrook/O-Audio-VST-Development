/*
  ==============================================================================

    BridgeFilter.h
    Bridge Reflection Filter - Phase 2.2
    Ouaricon Audio
    Developer: Taylor Brook

    Models frequency-dependent reflection at the bridge endpoint.
    Higher frequencies are damped more than lower frequencies,
    simulating the compliance of the bridge structure.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * Bridge Reflection Filter
 *
 * Simulates the frequency-dependent reflection characteristics of a
 * string-to-bridge coupling. The bridge acts as a soft boundary that
 * reflects lower frequencies more efficiently than higher frequencies.
 *
 * Implementation: One-pole lowpass filter with adjustable cutoff
 */
class BridgeFilter
{
public:
    BridgeFilter();
    ~BridgeFilter();

    /**
     * Prepare for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum expected block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Process one sample through the bridge filter
     * @param input Input sample
     * @return Filtered output
     */
    float processSample(float input);

    /**
     * Reset filter state
     */
    void reset();

    /**
     * Set brightness parameter (controls reflection characteristics)
     * @param brightness 0.0 = dark (low cutoff), 1.0 = bright (high cutoff)
     */
    void setBrightness(float brightness);

    /**
     * Set fundamental frequency (for frequency-dependent tuning)
     * @param frequency Fundamental in Hz
     */
    void setFundamental(double frequency);

private:
    juce::dsp::IIR::Filter<float> filter;
    double currentSampleRate = 44100.0;
    double fundamentalFreq = 440.0;
    float brightnessAmount = 0.5f;

    /**
     * Update filter coefficients based on current parameters
     */
    void updateCoefficients();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BridgeFilter)
};
