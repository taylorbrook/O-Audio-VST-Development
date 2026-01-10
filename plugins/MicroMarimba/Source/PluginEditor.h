/*
  ==============================================================================

    Ouaricon Marimba - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class MicroMarimbaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MicroMarimbaAudioProcessorEditor(MicroMarimbaAudioProcessor&);
    ~MicroMarimbaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MicroMarimbaAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MicroMarimbaAudioProcessorEditor)
};
