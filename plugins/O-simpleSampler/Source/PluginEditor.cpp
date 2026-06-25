/*
  ==============================================================================

    O-simpleSampler - Plugin Editor (implementation)

    Stage 1 (Foundation): minimal placeholder. Paints a centered "Stage 1 shell"
    label. The real WebView UI (interactive waveform editor + signal-path layout)
    lands in Stage 3.

  ==============================================================================
*/

#include "PluginEditor.h"

OSimpleSamplerAudioProcessorEditor::OSimpleSamplerAudioProcessorEditor (OSimpleSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (720, 480);
}

OSimpleSamplerAudioProcessorEditor::~OSimpleSamplerAudioProcessorEditor() = default;

void OSimpleSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1c1a)); // dark naturalist-leaning background

    auto bounds = getLocalBounds();

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawFittedText ("O-simpleSampler",
                      bounds.removeFromTop (bounds.getHeight() / 2),
                      juce::Justification::centredBottom, 1);

    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Stage 1 shell - 21 parameters, silent (audio: Stage 2, UI: Stage 3)",
                      bounds, juce::Justification::centredTop, 2);
}

void OSimpleSamplerAudioProcessorEditor::resized()
{
    // Nothing to lay out yet — placeholder editor (Stage 3 adds the WebView).
}
