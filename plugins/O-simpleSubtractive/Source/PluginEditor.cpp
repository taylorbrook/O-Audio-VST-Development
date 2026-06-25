/*
  ==============================================================================

    O-simpleSubtractive - Plugin Editor (implementation)

    Stage 1 (Foundation): host a GenericAudioProcessorEditor sized to fill the
    window. No Timer, no WebView, no binary data — that arrives in Stage 3.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
OSimpleSubtractiveAudioProcessorEditor::OSimpleSubtractiveAudioProcessorEditor (
        OSimpleSubtractiveAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      genericEditor (p)
{
    addAndMakeVisible (genericEditor);

    setResizable (true, true);
    setResizeLimits (480, 360, 1200, 1100);
    setSize (640, 760); // tall enough to show all 20 generic-view rows
}

OSimpleSubtractiveAudioProcessorEditor::~OSimpleSubtractiveAudioProcessorEditor() = default;

//==============================================================================
void OSimpleSubtractiveAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OSimpleSubtractiveAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
