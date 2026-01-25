/*
  ==============================================================================

    OBass - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Temporary generic editor for DSP testing (WebView UI added in Phase 5)

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
OBassAudioProcessorEditor::OBassAudioProcessorEditor(OBassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Create generic editor for parameter testing
    genericEditor = std::make_unique<juce::GenericAudioProcessorEditor>(p);
    addAndMakeVisible(genericEditor.get());

    // Size to fit generic editor with some padding
    setSize(400, 450);
}

OBassAudioProcessorEditor::~OBassAudioProcessorEditor()
{
}

//==============================================================================
void OBassAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));  // Dark gray background

    // Title at top
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f));
    g.drawFittedText("O-Bass", getLocalBounds().removeFromTop(50),
                     juce::Justification::centred, 1);
}

void OBassAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50);  // Space for title

    if (genericEditor)
        genericEditor->setBounds(bounds);
}
