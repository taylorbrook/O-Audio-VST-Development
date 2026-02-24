/*
  ==============================================================================

    ReverbProcessor.h
    O-IntonationPad - Wavetable Pad Synthesizer
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

    // Thread-safe setters (store to atomics; applied in process())
    void setSize (float size);
    void setDamping (float damp);
    void setPredelay (float ms);
    void setMix (float mix);

private:
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL { 19200 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR { 19200 };
    juce::dsp::DryWetMixer<float> dryWetMixer;

    float currentSampleRate = 44100.0f;

    // Atomic targets (written from any thread, read on audio thread in process())
    std::atomic<float> targetSize { 0.5f };
    std::atomic<float> targetDamping { 0.5f };
    std::atomic<float> targetPredelayMs { 0.0f };
    std::atomic<float> targetMix { 0.0f };
};
