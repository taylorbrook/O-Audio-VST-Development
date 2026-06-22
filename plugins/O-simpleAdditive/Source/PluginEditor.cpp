/*
  ==============================================================================

    O-simpleAdditive - Editor (implementation)

    Stage 1 (Foundation): hosts a GenericAudioProcessorEditor so the host shows
    all 33 parameters with correct ranges/defaults. The bespoke WebView UI
    replaces this in Stage 3.

  ==============================================================================
*/

#include "PluginEditor.h"

OSimpleAdditiveAudioProcessorEditor::OSimpleAdditiveAudioProcessorEditor (OSimpleAdditiveAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      genericEditor (p)   // GenericAudioProcessorEditor binds to the processor's APVTS params
{
    addAndMakeVisible (genericEditor);
    setSize (420, 640);
}

OSimpleAdditiveAudioProcessorEditor::~OSimpleAdditiveAudioProcessorEditor() = default;

void OSimpleAdditiveAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OSimpleAdditiveAudioProcessorEditor::resized()
{
    // Fill the window with the generic parameter list (placeholder for Stage 3 WebView).
    genericEditor.setBounds (getLocalBounds());
}
