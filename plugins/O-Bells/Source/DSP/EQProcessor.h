/*
  ==============================================================================

    EQProcessor.h
    O-Bells - Physical Modeling Bell Synthesizer
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

    // Thread-safe setters (store to atomics; coefficients updated in process())
    void setLowGain (float dB);
    void setMidGain (float dB);
    void setMidFreq (float hz);
    void setHighGain (float dB);

private:
    using FilterCoeffs = juce::dsp::IIR::Coefficients<float>;
    using Filter = juce::dsp::IIR::Filter<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<Filter, FilterCoeffs>;

    StereoFilter lowShelf;
    StereoFilter midPeak;
    StereoFilter highShelf;

    float currentSampleRate = 44100.0f;

    // Atomic targets (written from any thread, read on audio thread in process())
    std::atomic<float> targetLowGainDB { 0.0f };
    std::atomic<float> targetMidGainDB { 0.0f };
    std::atomic<float> targetMidFreqHz { 1000.0f };
    std::atomic<float> targetHighGainDB { 0.0f };

    // Dirty-flag: previous values for coefficient caching
    float prevLowGainDB = -999.0f;
    float prevMidGainDB = -999.0f;
    float prevMidFreqHz = -999.0f;
    float prevHighGainDB = -999.0f;
};
