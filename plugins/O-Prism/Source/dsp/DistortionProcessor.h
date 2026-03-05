/*
  ==============================================================================

    DistortionProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class DistortionProcessor
{
public:
    DistortionProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setType (int type);
    void setDrive (float drive);
    void setMix (float mix);

    float getLatencyInSamples() const { return static_cast<float> (oversampling.getLatencyInSamples()); }

private:
    void applyDistortion (juce::dsp::AudioBlock<float>& block);

    juce::dsp::Oversampling<float> oversampling;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    int distType = 0;
    float driveAmount = 0.0f;
};
