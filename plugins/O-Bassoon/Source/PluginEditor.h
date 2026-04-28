/*
  ==============================================================================

    O-Bassoon - Editor (Stage 1 placeholder)
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1: GenericAudioProcessorEditor auto-renders all 10 APVTS parameters.
    Replaced wholesale at Stage 3 with WebView UI.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OBassoonAudioProcessorEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit OBassoonAudioProcessorEditor (OBassoonAudioProcessor& p)
        : juce::GenericAudioProcessorEditor (p)
    {
        setSize (500, 480);   // ~10 params * ~40px row + label headroom
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OBassoonAudioProcessorEditor)
};
