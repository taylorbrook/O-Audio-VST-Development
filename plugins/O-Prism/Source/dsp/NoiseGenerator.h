/*
  ==============================================================================

    NoiseGenerator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class NoiseGenerator
{
public:
    NoiseGenerator() = default;

    void prepare (double sampleRate);
    void reset();
    void setType (int type);
    double getNextSample();

private:
    juce::Random random;
    double currentSampleRate = 44100.0;
    int currentType = 0; // 0=White, 1=Pink, 2=Brown, 3=Digital, 4=Vinyl, 5=Wind

    // Pink noise state (Paul Kellet economy)
    double b0 = 0.0, b1 = 0.0, b2 = 0.0;

    // Brown noise state
    double brownState = 0.0;

    // Digital noise state
    double digitalHoldValue = 0.0;
    int digitalCounter = 0;
    int digitalHoldSamples = 8;

    // Vinyl noise state
    double vinylBP1 = 0.0, vinylBP2 = 0.0;
    double crackleDecay = 0.0;

    // Wind noise state
    double windLFOPhase = 0.0;
    double windLPState = 0.0;
    double windBrownState = 0.0;
};
