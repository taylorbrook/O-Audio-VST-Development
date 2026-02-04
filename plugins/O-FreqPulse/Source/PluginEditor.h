/*
  ==============================================================================

    O-FreqPulse - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

class OFreqPulseAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit OFreqPulseAudioProcessorEditor(OFreqPulseAudioProcessor&);
    ~OFreqPulseAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for playhead updates
    void timerCallback() override;

    OFreqPulseAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL ORDER: Relays → WebView → Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ RELAYS FIRST (no dependencies)
    // Global parameter relays (5 total)
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebComboBoxRelay> stepsRelay;
    std::unique_ptr<juce::WebComboBoxRelay> rateRelay;
    std::unique_ptr<juce::WebSliderRelay> swingRelay;
    std::unique_ptr<juce::WebSliderRelay> smoothingRelay;

    // Per-band parameter relays (32 total: 8 params × 4 bands)
    std::unique_ptr<juce::WebToggleButtonRelay> band0EnableRelay;
    std::unique_ptr<juce::WebSliderRelay> band0LowRelay;
    std::unique_ptr<juce::WebSliderRelay> band0HighRelay;
    std::unique_ptr<juce::WebSliderRelay> band0DepthRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> band0EucOnRelay;
    std::unique_ptr<juce::WebSliderRelay> band0EucStepsRelay;
    std::unique_ptr<juce::WebSliderRelay> band0EucPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> band0EucOffsetRelay;

    std::unique_ptr<juce::WebToggleButtonRelay> band1EnableRelay;
    std::unique_ptr<juce::WebSliderRelay> band1LowRelay;
    std::unique_ptr<juce::WebSliderRelay> band1HighRelay;
    std::unique_ptr<juce::WebSliderRelay> band1DepthRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> band1EucOnRelay;
    std::unique_ptr<juce::WebSliderRelay> band1EucStepsRelay;
    std::unique_ptr<juce::WebSliderRelay> band1EucPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> band1EucOffsetRelay;

    std::unique_ptr<juce::WebToggleButtonRelay> band2EnableRelay;
    std::unique_ptr<juce::WebSliderRelay> band2LowRelay;
    std::unique_ptr<juce::WebSliderRelay> band2HighRelay;
    std::unique_ptr<juce::WebSliderRelay> band2DepthRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> band2EucOnRelay;
    std::unique_ptr<juce::WebSliderRelay> band2EucStepsRelay;
    std::unique_ptr<juce::WebSliderRelay> band2EucPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> band2EucOffsetRelay;

    std::unique_ptr<juce::WebToggleButtonRelay> band3EnableRelay;
    std::unique_ptr<juce::WebSliderRelay> band3LowRelay;
    std::unique_ptr<juce::WebSliderRelay> band3HighRelay;
    std::unique_ptr<juce::WebSliderRelay> band3DepthRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> band3EucOnRelay;
    std::unique_ptr<juce::WebSliderRelay> band3EucStepsRelay;
    std::unique_ptr<juce::WebSliderRelay> band3EucPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> band3EucOffsetRelay;

    // Step grid relays (128 total: 32 steps × 4 bands)
    std::array<std::unique_ptr<juce::WebToggleButtonRelay>, 128> stepRelays;

    // 2️⃣ WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    // Global parameter attachments (5 total)
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> stepsAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> rateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> swingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> smoothingAttachment;

    // Per-band parameter attachments (32 total)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band0EnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band0LowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band0HighAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band0DepthAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band0EucOnAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band0EucStepsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band0EucPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band0EucOffsetAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band1EnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band1LowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band1HighAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band1DepthAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band1EucOnAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band1EucStepsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band1EucPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band1EucOffsetAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band2EnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band2LowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band2HighAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band2DepthAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band2EucOnAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band2EucStepsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band2EucPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band2EucOffsetAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band3EnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band3LowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band3HighAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band3DepthAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> band3EucOnAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band3EucStepsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band3EucPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> band3EucOffsetAttachment;

    // Step grid attachments (128 total)
    std::array<std::unique_ptr<juce::WebToggleButtonParameterAttachment>, 128> stepAttachments;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreqPulseAudioProcessorEditor)
};
