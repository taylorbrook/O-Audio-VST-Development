/*
  ==============================================================================

    O-Bells - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OBellsAudioProcessorEditor::OBellsAudioProcessorEditor(OBellsAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(400, 300);
}

OBellsAudioProcessorEditor::~OBellsAudioProcessorEditor()
{
}

void OBellsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("O-Bells - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("22 parameters implemented",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OBellsAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 3 (GUI implementation)
}
