/*
  ==============================================================================

    OBass - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Minimal placeholder editor (WebView UI added in Phase 5)

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
OBassAudioProcessorEditor::OBassAudioProcessorEditor(OBassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Placeholder size - WebView UI will define final dimensions in Phase 5
    setSize(400, 300);
}

OBassAudioProcessorEditor::~OBassAudioProcessorEditor()
{
}

//==============================================================================
void OBassAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Placeholder UI - WebView replaces this in Phase 5
    g.fillAll(juce::Colour(0xff2d2d2d));  // Dark gray background

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(28.0f));
    g.drawFittedText("O-Bass", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(juce::FontOptions(12.0f));
    g.setColour(juce::Colours::grey);
    g.drawFittedText("Bass Enhancement Plugin",
                     getLocalBounds().removeFromBottom(40),
                     juce::Justification::centred, 1);
}

void OBassAudioProcessorEditor::resized()
{
    // WebView layout will be implemented in Phase 5
}
