/*
  ==============================================================================

    O-Bells - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class OBellsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OBellsAudioProcessorEditor(OBellsAudioProcessor&);
    ~OBellsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OBellsAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBellsAudioProcessorEditor)
};
