/*
  ==============================================================================

    O-MultiBandCompressor - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OMultiBandCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OMultiBandCompressorAudioProcessorEditor(OMultiBandCompressorAudioProcessor&);
    ~OMultiBandCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OMultiBandCompressorAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OMultiBandCompressorAudioProcessorEditor)
};
