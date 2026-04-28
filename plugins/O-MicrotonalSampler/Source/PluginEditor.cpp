/*
  ==============================================================================

    O-MicrotonalSampler - Editor Implementation (Phase 2.2 placeholder)
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 will replace this entirely with a WebView-based UI. For Phase 2.2
    the editor exists solely to drive the loader: a single button opens an
    async FileChooser; on selection we hand the folder to the processor.

  ==============================================================================
*/

#include "PluginEditor.h"

OMicrotonalSamplerAudioProcessorEditor::OMicrotonalSamplerAudioProcessorEditor (
    OMicrotonalSamplerAudioProcessor& p)
    : juce::AudioProcessorEditor (p),
      processorRef (p),
      genericEditor (p)
{
    addAndMakeVisible (genericEditor);
    addAndMakeVisible (loadFolderButton);

    loadFolderButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Choose sample folder", juce::File{}, juce::String{});

        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode
                | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& fc)
            {
                const auto folder = fc.getResult();
                if (folder.isDirectory())
                    processorRef.loadSampleFolder (folder);
            });
    };

    setSize (500, 400);
}

void OMicrotonalSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OMicrotonalSamplerAudioProcessorEditor::resized()
{
    constexpr int topStripHeight = 40;
    auto area = getLocalBounds();
    loadFolderButton.setBounds (area.removeFromTop (topStripHeight).reduced (8, 6));
    genericEditor.setBounds   (area);
}
