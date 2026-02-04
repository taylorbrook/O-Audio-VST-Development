/*
  ==============================================================================

    O-FreqPulse - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OFreqPulseAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OFreqPulseAudioProcessorEditor(OFreqPulseAudioProcessor&);
    ~OFreqPulseAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OFreqPulseAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreqPulseAudioProcessorEditor)
};
