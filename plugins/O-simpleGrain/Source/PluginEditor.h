/*
  ==============================================================================

    O-simpleGrain - Plugin Editor

    Stage 1 (Foundation): MINIMAL placeholder editor — a plain
    juce::AudioProcessorEditor that paints a "Stage 1 shell" label so the
    instrument is visibly alive in a host. NO WebView yet (Stage 3 brings the
    Ouaricon-Naturalist WebView UI, relays/attachments, visualizations).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleGrainAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OSimpleGrainAudioProcessorEditor (OSimpleGrainAudioProcessor&);
    ~OSimpleGrainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OSimpleGrainAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleGrainAudioProcessorEditor)
};
