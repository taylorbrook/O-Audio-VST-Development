/*
  ==============================================================================

    O-TextureForge - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TextureForgeEditor : public juce::AudioProcessorEditor,
                            private juce::Timer,
                            private juce::FileDragAndDropTarget
{
public:
    explicit TextureForgeEditor(TextureForgeProcessor&);
    ~TextureForgeEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // Called by processor when corpus is loaded
    void onCorpusLoaded(int grainCount);
    void onUmapProgress(float progress);
    void onUmapComplete(const juce::String& pointsJson);
    void onUmapCancelled();
    void onCorpusLoadFailed(const juce::String& reason);
    void onCorpusMissing(const juce::String& savedPath);

private:
    void timerCallback() override;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    TextureForgeProcessor& processorRef;
    juce::String pendingLargeFilePath;
    juce::String vizJsonBuffer;
    std::unique_ptr<juce::FileChooser> fileChooser;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE declaration order (C++ standard)
    // ========================================================================

    // 1. RELAYS (created before webView, destroyed after)
    std::unique_ptr<juce::WebSliderRelay> energyRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    std::unique_ptr<juce::WebSliderRelay> textureRelay;
    std::unique_ptr<juce::WebSliderRelay> positionRelay;
    std::unique_ptr<juce::WebSliderRelay> grainDensityRelay;
    std::unique_ptr<juce::WebSliderRelay> grainSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> scatterXRelay;
    std::unique_ptr<juce::WebSliderRelay> scatterYRelay;
    std::unique_ptr<juce::WebSliderRelay> variationRelay;
    std::unique_ptr<juce::WebSliderRelay> crossfadeRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    std::unique_ptr<juce::WebComboBoxRelay> midiModeRelay;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (created after webView, destroyed before)
    std::unique_ptr<juce::WebSliderParameterAttachment> energyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> textureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> positionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainDensityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> scatterXAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> scatterYAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> variationAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> crossfadeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> midiModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureForgeEditor)
};
