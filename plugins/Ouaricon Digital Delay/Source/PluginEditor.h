/*
  ==============================================================================

    Ouaricon Digital Delay - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OuariconDigitalDelayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconDigitalDelayAudioProcessorEditor(OuariconDigitalDelayAudioProcessor&);
    ~OuariconDigitalDelayAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconDigitalDelayAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconDigitalDelayAudioProcessorEditor)
};
