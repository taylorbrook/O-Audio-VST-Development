/*
  ==============================================================================

    O-Contrabass — Editor (Stage 1: minimal placeholder)
    Full WebView UI lands in Stage 3.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OContrabassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OContrabassAudioProcessorEditor(OContrabassAudioProcessor&);
    ~OContrabassAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override {}

private:
    OContrabassAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessorEditor)
};
