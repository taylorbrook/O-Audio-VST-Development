/*
  ==============================================================================

    Ouaricon Analog EQ - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OuariconAnalogEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconAnalogEQAudioProcessorEditor(OuariconAnalogEQAudioProcessor&);
    ~OuariconAnalogEQAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconAnalogEQAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconAnalogEQAudioProcessorEditor)
};
