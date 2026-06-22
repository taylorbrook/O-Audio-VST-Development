/*
  ==============================================================================

    O-simpleAdditive - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): a thin wrapper that hosts a
    juce::GenericAudioProcessorEditor so all 33 parameters are visible/editable
    in the host. Placeholder — replaced by the WebView UI in Stage 3.

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OSimpleAdditiveAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OSimpleAdditiveAudioProcessorEditor (OSimpleAdditiveAudioProcessor&);
    ~OSimpleAdditiveAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OSimpleAdditiveAudioProcessor& processorRef;

    // Generic host editor listing all 33 APVTS parameters (Stage 1 placeholder).
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAdditiveAudioProcessorEditor)
};
