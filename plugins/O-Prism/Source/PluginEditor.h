/*
  ==============================================================================

    PluginEditor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PrismParamIds.h"

#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

class OPrismAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
#if OUARICON_LICENSING_ENABLED
                                 , private OuariconLicense::Listener
#endif
{
public:
    explicit OPrismAudioProcessorEditor (OPrismAudioProcessor&);
    ~OPrismAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OPrismAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Member declaration order (C++ destroys in reverse)
    // 1. Relays destroyed LAST
    // 2. WebView destroyed SECOND
    // 3. Attachments destroyed FIRST (WebView still alive — safe)
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS (destroyed last)
    std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;
    std::unique_ptr<juce::WebToggleButtonRelay> delaySyncRelay;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> lfoSyncRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> lfoFreeRunRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> bypassRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> modSlotToggleRelays;

    // 2. WEBVIEW (destroyed second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed first — WebView still alive)
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delaySyncAttachment;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> lfoSyncAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> lfoFreeRunAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> bypassAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> modSlotToggleAttachments;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource>
        getResource (const juce::String& url);

    // Native function registration
    juce::WebBrowserComponent::Options addNativeFunctions (
        juce::WebBrowserComponent::Options options);

    // Timer callback to push active notes to WebView for TrueKeys
    void timerCallback() override;
    std::vector<std::pair<int, double>> lastSentNotes;

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OPrismAudioProcessorEditor)
};
