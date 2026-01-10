/*
  ==============================================================================

    Ouaricon Marimba - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class MicroMarimbaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MicroMarimbaAudioProcessorEditor(MicroMarimbaAudioProcessor&);
    ~MicroMarimbaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MicroMarimbaAudioProcessor& processorRef;

    // ⚠️ CRITICAL: MEMBER DECLARATION ORDER (Pattern 11)
    // Members destroyed in REVERSE order of declaration
    // Order: Relays → WebView → Attachments (prevents 90% of release build crashes)

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> malletHardnessRelay;
    std::unique_ptr<juce::WebSliderRelay> barMaterialRelay;
    std::unique_ptr<juce::WebSliderRelay> resonanceRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningModeRelay;
    std::unique_ptr<juce::WebSliderRelay> referencePitchRelay;
    std::unique_ptr<juce::WebSliderRelay> velCurveRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> malletHardnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> barMaterialAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> resonanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> referencePitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> velCurveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    // Helper for resource serving (Pattern 8: Explicit URL mapping)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MicroMarimbaAudioProcessorEditor)
};
