/*
  ==============================================================================

    Ouaricon Digital Delay - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OuariconDigitalDelayAudioProcessorEditor::OuariconDigitalDelayAudioProcessorEditor(OuariconDigitalDelayAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(700, 196);
}

OuariconDigitalDelayAudioProcessorEditor::~OuariconDigitalDelayAudioProcessorEditor()
{
}

void OuariconDigitalDelayAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Ouaricon Digital Delay - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("8 parameters implemented",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OuariconDigitalDelayAudioProcessorEditor::resized()
{
    // WebView layout will be added in Stage 3 (GUI)
}
