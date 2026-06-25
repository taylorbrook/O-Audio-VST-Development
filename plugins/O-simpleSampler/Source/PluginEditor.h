/*
  ==============================================================================

    O-simpleSampler - Plugin Editor

    Stage 1 (Foundation): MINIMAL placeholder editor — a plain
    juce::AudioProcessorEditor that paints a "Stage 1 shell" label so the
    instrument is visibly alive in a host. NO WebView yet (Stage 3 brings the
    Ouaricon WebView UI: the interactive waveform editor, relays/attachments,
    filter curve, amp-ADSR, tooltips, preset tour).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleSamplerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OSimpleSamplerAudioProcessorEditor (OSimpleSamplerAudioProcessor&);
    ~OSimpleSamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OSimpleSamplerAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSamplerAudioProcessorEditor)
};
