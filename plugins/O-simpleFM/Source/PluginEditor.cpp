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
    startTimerHz (30);
}

OSimpleFMAudioProcessorEditor::~OSimpleFMAudioProcessorEditor()
{
    stopTimer();
}

void OSimpleFMAudioProcessorEditor::timerCallback()
{
    // Pull the latest audio window, run the FFT + scope downsample. In Stage 3
    // this is followed by webView->emitEventIfBrowserIsVisible(...) for both
    // "spectrumUpdate" and "scopeUpdate".
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());
}

void OSimpleFMAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OSimpleFMAudioProcessorEditor::resized()
{
    genericEditor.setBounds (getLocalBounds());
}
