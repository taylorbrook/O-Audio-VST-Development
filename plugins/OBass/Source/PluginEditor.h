/*
  ==============================================================================

    OBass - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Temporary generic editor for DSP testing (WebView UI added in Phase 5)

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

    // Generic editor for parameter testing (temporary until Phase 5 WebView)
    std::unique_ptr<juce::GenericAudioProcessorEditor> genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassAudioProcessorEditor)
};
