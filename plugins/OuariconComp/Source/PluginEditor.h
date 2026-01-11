/*
  ==============================================================================

    OuariconComp - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OuariconCompAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconCompAudioProcessorEditor(OuariconCompAudioProcessor&);
    ~OuariconCompAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconCompAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconCompAudioProcessorEditor)
};
