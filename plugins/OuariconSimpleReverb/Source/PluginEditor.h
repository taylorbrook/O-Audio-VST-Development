/*
  ==============================================================================

    OuariconSimpleReverb - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OuariconSimpleReverbAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconSimpleReverbAudioProcessorEditor(OuariconSimpleReverbAudioProcessor&);
    ~OuariconSimpleReverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconSimpleReverbAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSimpleReverbAudioProcessorEditor)
};
