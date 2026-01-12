/*
  ==============================================================================

    Ouaricon Analog EQ - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OuariconAnalogEQAudioProcessorEditor::OuariconAnalogEQAudioProcessorEditor(OuariconAnalogEQAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(920, 220);  // Mockup v3 size (compact rack-unit)
}

OuariconAnalogEQAudioProcessorEditor::~OuariconAnalogEQAudioProcessorEditor()
{
}

void OuariconAnalogEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Ouaricon Analog EQ - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("16 parameters implemented",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OuariconAnalogEQAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 3 (GUI)
}
