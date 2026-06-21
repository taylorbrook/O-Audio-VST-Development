/*
  ==============================================================================

    O-simpleFM - Plugin Editor (implementation)

    Stage 1 (Foundation): generic parameter view. Replaced by WebView in Stage 3.

  ==============================================================================
*/

#include "PluginEditor.h"

OSimpleFMAudioProcessorEditor::OSimpleFMAudioProcessorEditor (OSimpleFMAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      genericEditor (p)
{
    addAndMakeVisible (genericEditor);
    setSize (520, 640);
}

OSimpleFMAudioProcessorEditor::~OSimpleFMAudioProcessorEditor() = default;

void OSimpleFMAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OSimpleFMAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
