/*
  ==============================================================================

    EQProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EQProcessor
{
public:
    EQProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setLowGain (float dB);
    void setMidGain (float dB);
    void setMidFreq (float hz);
    void setHighGain (float dB);

private:
    void updateLowShelf();
    void updateMidPeak();
    void updateHighShelf();

    using FilterCoeffs = juce::dsp::IIR::Coefficients<float>;
    using Filter = juce::dsp::IIR::Filter<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<Filter, FilterCoeffs>;

    StereoFilter lowShelf;
    StereoFilter midPeak;
    StereoFilter highShelf;

    float currentSampleRate = 44100.0f;
    float lowGainDB = 0.0f;
    float midGainDB = 0.0f;
    float midFreqHz = 1000.0f;
    float highGainDB = 0.0f;
};
