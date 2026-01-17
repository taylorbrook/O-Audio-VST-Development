/*
  ==============================================================================

    OuariconLyrica - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OuariconLyricaAudioProcessorEditor::OuariconLyricaAudioProcessorEditor(OuariconLyricaAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(600, 400);
}

OuariconLyricaAudioProcessorEditor::~OuariconLyricaAudioProcessorEditor()
{
}

void OuariconLyricaAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("OuariconLyrica - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("19 parameters implemented",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OuariconLyricaAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 2 (GUI)
}
