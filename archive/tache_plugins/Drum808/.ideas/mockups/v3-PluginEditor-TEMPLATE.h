#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * WebView-based Plugin Editor for Drum808
 *
 * CRITICAL: Member declaration order prevents release build crashes.
 *
 * Order: Relays → WebView → Attachments
 *
 * Destruction happens in REVERSE order:
 * 1. Attachments destroyed FIRST (stop calling evaluateJavascript)
 * 2. WebView destroyed SECOND (safe, attachments gone)
 * 3. Relays destroyed LAST (safe, nothing using them)
 */

class Drum808AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    Drum808AudioProcessorEditor(Drum808AudioProcessor& p);
    ~Drum808AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    /**
     * Resource provider for embedded HTML/JS files
     * Maps URLs to binary data from juce_add_binary_data
     */
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // Reference to audio processor
    Drum808AudioProcessor& audioProcessor;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    //
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order
    // ========================================================================

    // ------------------------------------------------------------------------
    // 1️⃣ RELAYS FIRST (24 parameters)
    // ------------------------------------------------------------------------

    // Kick drum (4 parameters)
    std::unique_ptr<juce::WebSliderRelay> kickLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> kickToneRelay;
    std::unique_ptr<juce::WebSliderRelay> kickDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> kickTuningRelay;

    // Low tom (4 parameters)
    std::unique_ptr<juce::WebSliderRelay> lowtomLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> lowtomToneRelay;
    std::unique_ptr<juce::WebSliderRelay> lowtomDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lowtomTuningRelay;

    // Mid tom (4 parameters)
    std::unique_ptr<juce::WebSliderRelay> midtomLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> midtomToneRelay;
    std::unique_ptr<juce::WebSliderRelay> midtomDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> midtomTuningRelay;

    // Clap (4 parameters)
    std::unique_ptr<juce::WebSliderRelay> clapLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> clapToneRelay;
    std::unique_ptr<juce::WebSliderRelay> clapSnapRelay;
    std::unique_ptr<juce::WebSliderRelay> clapTuningRelay;

    // Closed hat (4 parameters)
    std::unique_ptr<juce::WebSliderRelay> closedhatLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> closedhatToneRelay;
    std::unique_ptr<juce::WebSliderRelay> closedhatDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> closedhatTuningRelay;

    // Open hat (4 parameters)
    std::unique_ptr<juce::WebSliderRelay> openhatLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> openhatToneRelay;
    std::unique_ptr<juce::WebSliderRelay> openhatDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> openhatTuningRelay;

    // Preset browser (3 buttons)
    std::unique_ptr<juce::WebToggleButtonRelay> presetPreviousRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> presetNextRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> presetSaveRelay;

    // ------------------------------------------------------------------------
    // 2️⃣ WEBVIEW SECOND
    // ------------------------------------------------------------------------

    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ------------------------------------------------------------------------
    // 3️⃣ PARAMETER ATTACHMENTS LAST (24 parameters)
    // ------------------------------------------------------------------------

    // Kick drum (4 attachments)
    std::unique_ptr<juce::WebSliderParameterAttachment> kickLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> kickToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> kickDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> kickTuningAttachment;

    // Low tom (4 attachments)
    std::unique_ptr<juce::WebSliderParameterAttachment> lowtomLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lowtomToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lowtomDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lowtomTuningAttachment;

    // Mid tom (4 attachments)
    std::unique_ptr<juce::WebSliderParameterAttachment> midtomLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> midtomToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> midtomDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> midtomTuningAttachment;

    // Clap (4 attachments)
    std::unique_ptr<juce::WebSliderParameterAttachment> clapLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> clapToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> clapSnapAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> clapTuningAttachment;

    // Closed hat (4 attachments)
    std::unique_ptr<juce::WebSliderParameterAttachment> closedhatLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> closedhatToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> closedhatDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> closedhatTuningAttachment;

    // Open hat (4 attachments)
    std::unique_ptr<juce::WebSliderParameterAttachment> openhatLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> openhatToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> openhatDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> openhatTuningAttachment;

    // Preset browser (3 button attachments)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> presetPreviousAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> presetNextAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> presetSaveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Drum808AudioProcessorEditor)
};
