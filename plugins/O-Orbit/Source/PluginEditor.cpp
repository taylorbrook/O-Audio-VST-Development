#include "PluginEditor.h"

OOrbitEditor::OOrbitEditor (OOrbitProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    setSize (900, 600);
}

OOrbitEditor::~OOrbitEditor() {}

void OOrbitEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText (processorRef.getName(), getLocalBounds(), juce::Justification::centred, 1);
}

void OOrbitEditor::resized() {}
