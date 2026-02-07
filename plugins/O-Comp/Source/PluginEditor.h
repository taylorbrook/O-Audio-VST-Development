/*
  ==============================================================================

    O-Comp - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI) - WebView UI Implementation

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

class OCompAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
#if OUARICON_LICENSING_ENABLED
                                , private OuariconLicense::Listener
#endif
{
public:
    explicit OCompAudioProcessorEditor(OCompAudioProcessor&);
    ~OCompAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for metering updates
    void timerCallback() override;

    OCompAudioProcessor& processorRef;

    // File chooser for preset dialogs (must persist during async operation)
    std::unique_ptr<juce::FileChooser> fileChooser;

    //==========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order
    // Attachments call evaluateJavascript() during destruction
    // Therefore attachments MUST be destroyed BEFORE webView
    //==========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> thresholdRelay;
    std::unique_ptr<juce::WebSliderRelay> ratioRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> kneeRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autoGainRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> thresholdAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ratioAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> kneeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autoGainAttachment;

    //==========================================================================
    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OCompAudioProcessorEditor)
};
