/*
  ==============================================================================

    O-MicrotonalSampler - Editor (Stage 1 placeholder)
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1: GenericAudioProcessorEditor auto-renders all 7 APVTS parameters.
    Replaced wholesale at Stage 3 with WebView UI.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OMicrotonalSamplerAudioProcessorEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit OMicrotonalSamplerAudioProcessorEditor (OMicrotonalSamplerAudioProcessor& p)
        : juce::GenericAudioProcessorEditor (p)
    {
        setSize (500, 360);   // 7 params * ~40px row + breathing room
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OMicrotonalSamplerAudioProcessorEditor)
};
