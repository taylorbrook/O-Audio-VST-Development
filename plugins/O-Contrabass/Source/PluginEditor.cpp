/*
  ==============================================================================

    O-Contrabass — Editor Implementation (Stage 1 placeholder)

  ==============================================================================
*/

#include "PluginEditor.h"

OContrabassAudioProcessorEditor::OContrabassAudioProcessorEditor(OContrabassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(600, 400);
}

void OContrabassAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("O-Contrabass — Stage 1 (Foundation)",
               getLocalBounds(), juce::Justification::centred);
}
