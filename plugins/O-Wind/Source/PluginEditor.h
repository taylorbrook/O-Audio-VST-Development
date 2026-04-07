/*
  ==============================================================================

    O-Wind - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

class OWindAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OWindAudioProcessorEditor(OWindAudioProcessor&);
    ~OWindAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OWindAudioProcessor& processorRef;

    // ===================================================================
    // CRITICAL ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ===================================================================

    // 1. RELAYS FIRST (no dependencies) - 14 slider relays + 1 toggle + 1 slider (instrument)
    // Excitation Controls
    std::unique_ptr<juce::WebSliderRelay> breathPressureRelay;
    std::unique_ptr<juce::WebSliderRelay> embouchureRelay;
    std::unique_ptr<juce::WebSliderRelay> breathNoiseRelay;
    // Resonator Controls
    std::unique_ptr<juce::WebSliderRelay> toneColorRelay;
    std::unique_ptr<juce::WebSliderRelay> airColumnRelay;
    std::unique_ptr<juce::WebSliderRelay> jetReflectionRelay;
    std::unique_ptr<juce::WebSliderRelay> endReflectionRelay;
    // Articulation
    std::unique_ptr<juce::WebSliderRelay> flutterTongueRelay;
    std::unique_ptr<juce::WebSliderRelay> flutterRateRelay;
    // Modulation
    std::unique_ptr<juce::WebSliderRelay> vibratoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoDepthRelay;
    // Output
    std::unique_ptr<juce::WebSliderRelay> widthRelay;
    std::unique_ptr<juce::WebSliderRelay> outputLevelRelay;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderRelay> infiniteSustainRelay;
    std::unique_ptr<juce::WebSliderRelay> reversedJetRelay;
    std::unique_ptr<juce::WebSliderRelay> subHarmonicsRelay;
    // Tone Hole Toggle
    std::unique_ptr<juce::WebToggleButtonRelay> toneHoleToggleRelay;
    // Instrument Preset (int param, bound as slider relay)
    std::unique_ptr<juce::WebSliderRelay> instrumentPresetRelay;

    // 2. WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and webView) - 14 slider + 1 toggle + 1 slider
    // Excitation Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> breathPressureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> embouchureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> breathNoiseAttachment;
    // Resonator Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> toneColorAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> airColumnAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> jetReflectionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> endReflectionAttachment;
    // Articulation
    std::unique_ptr<juce::WebSliderParameterAttachment> flutterTongueAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> flutterRateAttachment;
    // Modulation
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDepthAttachment;
    // Output
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputLevelAttachment;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderParameterAttachment> infiniteSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reversedJetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> subHarmonicsAttachment;
    // Tone Hole Toggle
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> toneHoleToggleAttachment;
    // Instrument Preset
    std::unique_ptr<juce::WebSliderParameterAttachment> instrumentPresetAttachment;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset save/load dialogs (must be shared_ptr)
    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OWindAudioProcessorEditor)
};
