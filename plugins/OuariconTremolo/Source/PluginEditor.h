#pragma once
#include "PluginProcessor.h"

class OuariconTremoloAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconTremoloAudioProcessorEditor(OuariconTremoloAudioProcessor&);
    ~OuariconTremoloAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconTremoloAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconTremoloAudioProcessorEditor)
};
