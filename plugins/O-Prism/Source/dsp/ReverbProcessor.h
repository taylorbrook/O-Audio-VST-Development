/*
  ==============================================================================

    ReverbProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ReverbProcessor
{
public:
    ReverbProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setSize (float size);
    void setDamping (float damp);
    void setPredelay (float ms);
    void setMix (float mix);

private:
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL { 19200 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR { 19200 };
    juce::dsp::DryWetMixer<float> dryWetMixer;

    float preDelaySamples = 0.0f;
    float currentSampleRate = 44100.0f;
    juce::Reverb::Parameters reverbParams;
};
