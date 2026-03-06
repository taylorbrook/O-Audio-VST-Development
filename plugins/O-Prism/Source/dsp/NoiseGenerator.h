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
    void getNextSampleStereo (double& outL, double& outR);

private:
    juce::Random randomL, randomR;
    double currentSampleRate = 44100.0;
    int currentType = 0; // 0=White, 1=Pink, 2=Brown, 3=Digital, 4=Vinyl, 5=Wind

    // Pink noise state (Paul Kellet economy) — per channel
    double b0L = 0.0, b1L = 0.0, b2L = 0.0;
    double b0R = 0.0, b1R = 0.0, b2R = 0.0;

    // Brown noise state — per channel
    double brownStateL = 0.0;
    double brownStateR = 0.0;

    // Digital noise state — per channel
    double digitalHoldValueL = 0.0, digitalHoldValueR = 0.0;
    int digitalCounterL = 0, digitalCounterR = 0;
    int digitalHoldSamples = 8;

    // Vinyl noise state — per channel
    double vinylBP1L = 0.0, vinylBP2L = 0.0;
    double vinylBP1R = 0.0, vinylBP2R = 0.0;
    double crackleDecayL = 0.0, crackleDecayR = 0.0;

    // Wind noise state — shared LFO, per-channel filter
    double windLFOPhase = 0.0;
    double windLPStateL = 0.0, windLPStateR = 0.0;
    double windBrownStateL = 0.0, windBrownStateR = 0.0;
};
