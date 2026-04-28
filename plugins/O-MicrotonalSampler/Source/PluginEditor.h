/*
  ==============================================================================

    O-MicrotonalSampler - Editor (Phase 2.2 placeholder)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.2: minimal editor — a 40-px top strip with a "Load Folder..." button
    plus an embedded GenericAudioProcessorEditor for the 7 APVTS parameters.
    Sufficient to drive Phase 2.2 Gate 2 verification (drag a folder, audition
    the resulting SampleMap). Stage 3 replaces this wholesale with a WebView UI.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OMicrotonalSamplerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OMicrotonalSamplerAudioProcessorEditor (OMicrotonalSamplerAudioProcessor& p);
    ~OMicrotonalSamplerAudioProcessorEditor() override = default;

    void paint   (juce::Graphics&) override;
    void resized() override;

private:
    OMicrotonalSamplerAudioProcessor& processorRef;

    juce::GenericAudioProcessorEditor genericEditor;
    juce::TextButton                  loadFolderButton { "Load Folder..." };
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OMicrotonalSamplerAudioProcessorEditor)
};
