/*
  ==============================================================================

    Ouaricon Digital Delay - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

class OuariconDigitalDelayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
#if OUARICON_LICENSING_ENABLED
                                                , private OuariconLicense::Listener
#endif
{
public:
    explicit OuariconDigitalDelayAudioProcessorEditor(OuariconDigitalDelayAudioProcessor&);
    ~OuariconDigitalDelayAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void parentHierarchyChanged() override;

private:
    OuariconDigitalDelayAudioProcessor& processorRef;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER - prevents release build crashes
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order (attachments before webView)
    // ========================================================================

    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> timeRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> spreadRelay;
    std::unique_ptr<juce::WebSliderRelay> modRelay;
    std::unique_ptr<juce::WebSliderRelay> wetRelay;
    std::unique_ptr<juce::WebSliderRelay> dryRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> syncRelay;
    std::unique_ptr<juce::WebComboBoxRelay> divisionRelay;

    // 2. WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> timeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> syncAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> divisionAttachment;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset dialogs (kept alive for async operations)
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Navigation flag (prevents re-navigation on window reopen)
    bool hasNavigated = false;

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconDigitalDelayAudioProcessorEditor)
};
