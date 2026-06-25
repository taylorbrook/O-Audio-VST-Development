/*
  ==============================================================================

    O-simpleSubtractive - Plugin Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): a thin wrapper that hosts a GenericAudioProcessorEditor
    child filling the window, so every one of the 20 parameters is visible and
    testable in any host. Stage 3 (GUI) swaps the body for the WebView UI while
    keeping this class's identity/CMake target_sources stable.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class OSimpleSubtractiveAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OSimpleSubtractiveAudioProcessorEditor (OSimpleSubtractiveAudioProcessor&);
    ~OSimpleSubtractiveAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OSimpleSubtractiveAudioProcessor& processorRef;

    // Foundation UI: exposes all params via the host's generic view. Constructed
    // from the processor so it binds straight to the APVTS-backed parameters.
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSubtractiveAudioProcessorEditor)
};
