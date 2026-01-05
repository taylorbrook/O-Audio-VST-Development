#include "PluginEditor.h"

OuariconTremoloAudioProcessorEditor::OuariconTremoloAudioProcessorEditor(OuariconTremoloAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(600, 400);
}

OuariconTremoloAudioProcessorEditor::~OuariconTremoloAudioProcessorEditor()
{
}

void OuariconTremoloAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Ouaricon Tremolo - Stage 1", getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(14.0f);
    g.drawFittedText("6 parameters implemented - UI in Stage 3",
                     getLocalBounds().reduced(20).removeFromBottom(30),
                     juce::Justification::centred, 1);
}

void OuariconTremoloAudioProcessorEditor::resized()
{
    // WebView layout will be added in Stage 3
}
