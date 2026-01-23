/*
  ==============================================================================

    OBass - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Minimal placeholder editor (WebView UI added in Phase 5)

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"

class OBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OBassAudioProcessorEditor(OBassAudioProcessor&);
    ~OBassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OBassAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassAudioProcessorEditor)
};
