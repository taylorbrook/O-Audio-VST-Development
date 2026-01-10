/*
  ==============================================================================

    BodyResonance.h
    Phase 2.4: Convolution-based body resonance for marimba tube character
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

class BodyResonance
{
public:
    BodyResonance();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process audio block (in-place, stereo)
    void process(juce::AudioBuffer<float>& buffer);

    // Set dry/wet mix (0.0 = dry only, 1.0 = full wet blend)
    void setMix(float newMix);

private:
    juce::dsp::Convolution convolution;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    double currentSampleRate = 44100.0;
    float mix = 0.5f;

    // Generate synthetic IR
    void generateSyntheticIR();
    juce::AudioBuffer<float> irBuffer;
};
