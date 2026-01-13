/*
  ==============================================================================

    OuariconSimpleReverb - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OuariconSimpleReverbAudioProcessorEditor::OuariconSimpleReverbAudioProcessorEditor(OuariconSimpleReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(600, 400);
}

OuariconSimpleReverbAudioProcessorEditor::~OuariconSimpleReverbAudioProcessorEditor()
{
}

void OuariconSimpleReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Ouaricon Simple Reverb - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("6 parameters implemented",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OuariconSimpleReverbAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 2 (GUI)
}
