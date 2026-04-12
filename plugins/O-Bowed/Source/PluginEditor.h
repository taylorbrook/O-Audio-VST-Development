/*
  ==============================================================================

    O-Bowed - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>

class OBowedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OBowedAudioProcessorEditor(OBowedAudioProcessor&);
    ~OBowedAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OBowedAudioProcessor& processorRef;

    // ===================================================================
    // CRITICAL ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ===================================================================

    // 1. RELAYS FIRST (no dependencies)
    // Bow Controls
    std::unique_ptr<juce::WebSliderRelay> bowSpeedRelay;
    std::unique_ptr<juce::WebSliderRelay> bowPressureRelay;
    std::unique_ptr<juce::WebSliderRelay> bowPositionRelay;
    std::unique_ptr<juce::WebSliderRelay> rosinRelay;
    std::unique_ptr<juce::WebSliderRelay> bowNoiseRelay;
    // Friction model
    std::unique_ptr<juce::WebComboBoxRelay> frictionTierRelay;
    // Body Controls
    std::unique_ptr<juce::WebSliderRelay> bodyMaterialRelay;
    std::unique_ptr<juce::WebSliderRelay> bodySizeRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    // String Configuration
    std::unique_ptr<juce::WebSliderRelay> sympatheticAmountRelay;
    std::unique_ptr<juce::WebSliderRelay> sympatheticCountRelay;
    // Output
    std::unique_ptr<juce::WebSliderRelay> widthRelay;
    std::unique_ptr<juce::WebSliderRelay> outputLevelRelay;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderRelay> infiniteSustainRelay;
    std::unique_ptr<juce::WebSliderRelay> reversedFrictionRelay;
    std::unique_ptr<juce::WebSliderRelay> subHarmonicsRelay;
    // Tuning
    std::unique_ptr<juce::WebSliderRelay> referencePitchRelay;
    std::unique_ptr<juce::WebComboBoxRelay> tuningSystemRelay;

    // 2. WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)
    // Bow Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> bowSpeedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowPressureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowPositionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rosinAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bowNoiseAttachment;
    // Friction model
    std::unique_ptr<juce::WebComboBoxParameterAttachment> frictionTierAttachment;
    // Body Controls
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyMaterialAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodySizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    // String Configuration
    std::unique_ptr<juce::WebSliderParameterAttachment> sympatheticAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sympatheticCountAttachment;
    // Output
    std::unique_ptr<juce::WebSliderParameterAttachment> widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputLevelAttachment;
    // Impossible Physics
    std::unique_ptr<juce::WebSliderParameterAttachment> infiniteSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reversedFrictionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> subHarmonicsAttachment;
    // Tuning
    std::unique_ptr<juce::WebSliderParameterAttachment> referencePitchAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningSystemAttachment;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset save/load dialogs (must be shared_ptr)
    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBowedAudioProcessorEditor)
};
