/*
  ==============================================================================

    Ouaricon Marimba - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

MicroMarimbaAudioProcessorEditor::MicroMarimbaAudioProcessorEditor(MicroMarimbaAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(600, 400);
}

MicroMarimbaAudioProcessorEditor::~MicroMarimbaAudioProcessorEditor()
{
}

void MicroMarimbaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Ouaricon Marimba - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("7 parameters implemented",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void MicroMarimbaAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 3 (GUI)
}
