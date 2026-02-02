/*
  ==============================================================================

    O-Detune - Editor
    Ouaricon Development
    Developer: Taylor Brook
    Version: 1.1.0

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class ODetuneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ODetuneAudioProcessorEditor(ODetuneAudioProcessor&);
    ~ODetuneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    ODetuneAudioProcessor& processorRef;

    // ============================================================================
    // CRITICAL: Member declaration order (Pattern #11)
    // Members are destroyed in REVERSE order of declaration
    // MUST be: Relays → WebView → Attachments
    // ============================================================================

    // 1. RELAYS FIRST (no dependencies)
    // Float parameters (sliders/knobs)
    std::unique_ptr<juce::WebSliderRelay> blendRelay;
    std::unique_ptr<juce::WebSliderRelay> wobbleRateRelay;
    std::unique_ptr<juce::WebSliderRelay> wobbleDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> unisonDetuneRelay;
    std::unique_ptr<juce::WebSliderRelay> unisonSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> driveRelay;
    std::unique_ptr<juce::WebSliderRelay> colorRelay;
    std::unique_ptr<juce::WebSliderRelay> ageRelay;
    std::unique_ptr<juce::WebSliderRelay> widthRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> focusLowRelay;
    std::unique_ptr<juce::WebSliderRelay> focusHighRelay;
    std::unique_ptr<juce::WebSliderRelay> delayRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> randomAmtRelay;

    // Choice parameters (dropdowns)
    std::unique_ptr<juce::WebComboBoxRelay> wobbleEraRelay;
    std::unique_ptr<juce::WebComboBoxRelay> wobbleShapeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> unisonVoicesRelay;
    std::unique_ptr<juce::WebComboBoxRelay> unisonDistRelay;

    // Bool parameters (toggles)
    std::unique_ptr<juce::WebToggleButtonRelay> wobbleSyncRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> monoSafeRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state (per-instance, not static)
    bool hasNavigated = false;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)
    // Float parameter attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> blendAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wobbleRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wobbleDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> unisonDetuneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> unisonSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> colorAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ageAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> focusLowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> focusHighAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> randomAmtAttachment;

    // Choice parameter attachments
    std::unique_ptr<juce::WebComboBoxParameterAttachment> wobbleEraAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> wobbleShapeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> unisonVoicesAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> unisonDistAttachment;

    // Bool parameter attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> wobbleSyncAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> monoSafeAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ODetuneAudioProcessorEditor)
};
