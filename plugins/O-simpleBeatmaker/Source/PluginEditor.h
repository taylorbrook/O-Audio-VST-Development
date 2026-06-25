/*
  ==============================================================================

    O-simpleBeatmaker - Plugin Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): a thin wrapper hosting a GenericAudioProcessorEditor
    that fills the window, so all 42 parameters are visible and testable in any
    host. Stage 3 (GUI) swaps the body for the WebView step-grid UI while keeping
    this class's identity and the CMake target_sources stable.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class OSimpleBeatmakerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OSimpleBeatmakerAudioProcessorEditor (OSimpleBeatmakerAudioProcessor&);
    ~OSimpleBeatmakerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OSimpleBeatmakerAudioProcessor& processorRef;

    // Foundation UI: exposes all params via the host's generic view, bound
    // straight to the APVTS-backed parameters.
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleBeatmakerAudioProcessorEditor)
};
