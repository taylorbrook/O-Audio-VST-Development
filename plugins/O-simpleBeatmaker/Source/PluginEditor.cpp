/*
  ==============================================================================

    O-simpleBeatmaker - Plugin Editor (implementation)

    Stage 1 (Foundation): host a GenericAudioProcessorEditor sized to fill the
    window. No Timer, no WebView, no binary data — that arrives in Stage 3.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
OSimpleBeatmakerAudioProcessorEditor::OSimpleBeatmakerAudioProcessorEditor (
        OSimpleBeatmakerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      genericEditor (p)
{
    addAndMakeVisible (genericEditor);

    setResizable (true, true);
    setResizeLimits (480, 360, 1200, 1400);
    setSize (700, 900); // tall enough to scroll the 42 generic-view rows comfortably
}

OSimpleBeatmakerAudioProcessorEditor::~OSimpleBeatmakerAudioProcessorEditor() = default;

//==============================================================================
void OSimpleBeatmakerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OSimpleBeatmakerAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
