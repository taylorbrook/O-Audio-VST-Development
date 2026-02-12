#pragma once

#include "PluginProcessor.h"

class OOrbitEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit OOrbitEditor (OOrbitProcessor&);
    ~OOrbitEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void parentHierarchyChanged() override;

private:
    OOrbitProcessor& processorRef;

    // ⚠️ MEMBER DECLARATION ORDER IS CRITICAL ⚠️
    // Destroyed in REVERSE order: Attachments → WebView → Relays
    // Relays MUST be destroyed last

    // 1️⃣ RELAYS FIRST (no dependencies) - all unique_ptr
    std::unique_ptr<juce::WebSliderRelay> speedRelay;
    std::unique_ptr<juce::WebSliderRelay> widthRelay;
    std::unique_ptr<juce::WebSliderRelay> depthRelay;
    std::unique_ptr<juce::WebSliderRelay> tiltRelay;
    std::unique_ptr<juce::WebSliderRelay> phaseRelay;
    std::unique_ptr<juce::WebSliderRelay> elevationRangeRelay;
    std::unique_ptr<juce::WebSliderRelay> distanceRelay;
    std::unique_ptr<juce::WebSliderRelay> airAbsorptionRelay;
    std::unique_ptr<juce::WebSliderRelay> centerDivergeRelay;
    std::unique_ptr<juce::WebSliderRelay> lrOffsetRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;

    std::unique_ptr<juce::WebComboBoxRelay> pathRelay;
    std::unique_ptr<juce::WebComboBoxRelay> tempoSyncRelay;
    std::unique_ptr<juce::WebComboBoxRelay> speakerLayoutRelay;
    std::unique_ptr<juce::WebComboBoxRelay> attenuationCurveRelay;
    std::unique_ptr<juce::WebComboBoxRelay> sourceModeRelay;

    std::unique_ptr<juce::WebToggleButtonRelay> elevationEnableRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> speedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> depthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tiltAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> phaseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> elevationRangeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> distanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> airAbsorptionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> centerDivergeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lrOffsetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> pathAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tempoSyncAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> speakerLayoutAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> attenuationCurveAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> sourceModeAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> elevationEnableAttachment;

    // Lazy navigation flag
    bool hasNavigated = false;

    // Async file chooser (must be kept alive during async dialog)
    std::shared_ptr<juce::FileChooser> fileChooser;

    // Resource provider
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOrbitEditor)
};
