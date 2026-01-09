/*
  ==============================================================================

    OuariconSaturationModeling - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OuariconSaturationModelingAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconSaturationModelingAudioProcessorEditor(OuariconSaturationModelingAudioProcessor&);
    ~OuariconSaturationModelingAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconSaturationModelingAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSaturationModelingAudioProcessorEditor)
};
