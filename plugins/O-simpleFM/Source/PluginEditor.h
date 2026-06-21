/*
  ==============================================================================

    O-simpleFM - Plugin Editor

    Stage 1 (Foundation): thin host around a GenericAudioProcessorEditor so all
    17 parameters are visible and testable in a DAW. Stage 3 replaces the body
    with the WebView UI (spectrum analyzer + oscilloscope + controls).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleFMAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit OSimpleFMAudioProcessorEditor (OSimpleFMAudioProcessor&);
    ~OSimpleFMAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Message-thread viz pump (ARCHITECTURE.md: FFT on the editor Timer, 30 Hz).
    // Stage 3 will emit analyzer output to the WebView; Stage 2 just runs the path.
    void timerCallback() override;

    OSimpleFMAudioProcessor& processorRef;

    FmVizAnalyzer vizAnalyzer;

    // Temporary generic parameter view (replaced by WebView in Stage 3).
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleFMAudioProcessorEditor)
};
