/*
  ==============================================================================

    O-MultiBandCompressor - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"

OMultiBandCompressorAudioProcessorEditor::OMultiBandCompressorAudioProcessorEditor(OMultiBandCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(900, 600);
}

OMultiBandCompressorAudioProcessorEditor::~OMultiBandCompressorAudioProcessorEditor()
{
}

void OMultiBandCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("O-MultiBandCompressor - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("57 parameters implemented (8 global + 48 per-band)",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OMultiBandCompressorAudioProcessorEditor::resized()
{
    // WebView layout will be added in Stage 3 (GUI)
}
