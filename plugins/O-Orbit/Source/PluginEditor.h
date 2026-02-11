#pragma once

#include "PluginProcessor.h"

class OOrbitEditor : public juce::AudioProcessorEditor
{
public:
    explicit OOrbitEditor (OOrbitProcessor&);
    ~OOrbitEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OOrbitProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOrbitEditor)
};
