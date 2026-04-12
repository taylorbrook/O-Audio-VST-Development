/*
  ==============================================================================

    DelayProcessor.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class DelayProcessor
{
public:
    DelayProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setTime (float seconds);
    void setFeedback (float fb);
    void setMode (int mode);
    void setMix (float mix);

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayL { 192000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayR { 192000 };
    juce::dsp::StateVariableTPTFilter<float> feedbackFilterL;
    juce::dsp::StateVariableTPTFilter<float> feedbackFilterR;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    float delaySamples = 0.0f;
    float feedbackAmount = 0.3f;
    int delayMode = 0; // 0=Normal, 1=PingPong
    float currentSampleRate = 44100.0f;
    float feedbackL = 0.0f, feedbackR = 0.0f;
};
