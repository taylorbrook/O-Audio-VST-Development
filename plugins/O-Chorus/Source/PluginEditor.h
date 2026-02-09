#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

//==============================================================================
/**
 * O-Chorus Plugin Editor with WebView UI
 *
 * Stage 1 (Foundation) - Placeholder UI
 * Parameters: 8 total (rate, depth, voices, spread, width, tone, mix, drive)
 */
class OChorusAudioProcessorEditor : public juce::AudioProcessorEditor
#if OUARICON_LICENSING_ENABLED
                                  , private OuariconLicense::Listener
#endif
{
public:
    OChorusAudioProcessorEditor(OChorusAudioProcessor&);
    ~OChorusAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to processor
    OChorusAudioProcessor& audioProcessor;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order (automatic in C++)
    // ========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> rateRelay;
    std::unique_ptr<juce::WebSliderRelay> depthRelay;
    std::unique_ptr<juce::WebSliderRelay> voicesRelay;
    std::unique_ptr<juce::WebSliderRelay> spreadRelay;
    std::unique_ptr<juce::WebSliderRelay> widthRelay;
    std::unique_ptr<juce::WebSliderRelay> toneRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> driveRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> rateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> depthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> voicesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> toneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;

    // File chooser for preset save/load dialogs (must persist across async callbacks)
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OChorusAudioProcessorEditor)
};
