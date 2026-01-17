/*
  ==============================================================================

    OuariconLyrica - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OuariconLyricaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconLyricaAudioProcessorEditor(OuariconLyricaAudioProcessor&);
    ~OuariconLyricaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconLyricaAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconLyricaAudioProcessorEditor)
};
