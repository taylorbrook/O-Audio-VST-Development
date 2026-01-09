/*
  ==============================================================================

    OuariconSaturationModeling - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OuariconSaturationModelingAudioProcessorEditor::OuariconSaturationModelingAudioProcessorEditor(OuariconSaturationModelingAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(500, 500);
}

OuariconSaturationModelingAudioProcessorEditor::~OuariconSaturationModelingAudioProcessorEditor()
{
}

void OuariconSaturationModelingAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Ouaricon Saturation Modeling - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("4 parameters implemented",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OuariconSaturationModelingAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 5 (GUI)
}
