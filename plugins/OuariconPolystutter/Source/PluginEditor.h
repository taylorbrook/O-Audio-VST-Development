/*
  ==============================================================================

    Ouaricon Polystutter - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"

class OuariconPolystutterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconPolystutterAudioProcessorEditor(OuariconPolystutterAudioProcessor&);
    ~OuariconPolystutterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconPolystutterAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconPolystutterAudioProcessorEditor)
};
