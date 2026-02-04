/*
  ==============================================================================

    O-FreqPulse - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OFreqPulseAudioProcessorEditor::OFreqPulseAudioProcessorEditor(OFreqPulseAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(800, 600);
}

OFreqPulseAudioProcessorEditor::~OFreqPulseAudioProcessorEditor()
{
}

void OFreqPulseAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("O-FreqPulse - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("165 parameters implemented (5 global + 32 band + 128 step)",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OFreqPulseAudioProcessorEditor::resized()
{
    // Layout will be added in Stage 3 (WebView UI)
}
