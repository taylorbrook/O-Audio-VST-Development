/*
  ==============================================================================

    Ouaricon Polystutter - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OuariconPolystutterAudioProcessorEditor::OuariconPolystutterAudioProcessorEditor(OuariconPolystutterAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(1000, 750);
}

OuariconPolystutterAudioProcessorEditor::~OuariconPolystutterAudioProcessorEditor()
{
}

void OuariconPolystutterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Ouaricon Polystutter - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("130 parameters implemented (66 main + 64 pattern steps)",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OuariconPolystutterAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 3 (GUI)
}
