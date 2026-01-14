/*
  ==============================================================================

    Ouaricon Marimba - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class OuariconMarimbaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit OuariconMarimbaAudioProcessorEditor(OuariconMarimbaAudioProcessor&);
    ~OuariconMarimbaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for polling note-on events from processor
    void timerCallback() override;

private:
    OuariconMarimbaAudioProcessor& processorRef;

    // ⚠️ CRITICAL: MEMBER DECLARATION ORDER (Pattern 11)
    // Members destroyed in REVERSE order of declaration
    // Order: Relays → WebView → Attachments (prevents 90% of release build crashes)

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> malletHardnessRelay;
    std::unique_ptr<juce::WebSliderRelay> barMaterialRelay;
    std::unique_ptr<juce::WebSliderRelay> resonanceRelay;
    // v1.6.0: New timbre parameter relays
    std::unique_ptr<juce::WebSliderRelay> strikePositionRelay;
    std::unique_ptr<juce::WebSliderRelay> overtoneDampingRelay;
    std::unique_ptr<juce::WebSliderRelay> toneRelay;
    // Tuning/system relays
    std::unique_ptr<juce::WebSliderRelay> tuningModeRelay;
    std::unique_ptr<juce::WebSliderRelay> referencePitchRelay;
    std::unique_ptr<juce::WebSliderRelay> velCurveRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // v1.8.0: Analog EQ Unit relays (effects tab)
    // Slider relays (freq/gain parameters)
    std::unique_ptr<juce::WebSliderRelay> eqLfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLfGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLmfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLmfGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHmfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHmfGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHfGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqOutputGainRelay;
    // Toggle relays (band on/off, analog)
    std::unique_ptr<juce::WebToggleButtonRelay> eqLfOnRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqLmfOnRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqHmfOnRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqHfOnRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqAnalogRelay;
    // ComboBox relays (Q selection)
    std::unique_ptr<juce::WebComboBoxRelay> eqLmfQRelay;
    std::unique_ptr<juce::WebComboBoxRelay> eqHmfQRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> malletHardnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> barMaterialAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> resonanceAttachment;
    // v1.6.0: New timbre parameter attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> strikePositionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> overtoneDampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> toneAttachment;
    // Tuning/system attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> referencePitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> velCurveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    // v1.8.0: Analog EQ Unit attachments (effects tab)
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLfGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLmfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLmfGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHmfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHmfGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHfGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqOutputGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqLfOnAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqLmfOnAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqHmfOnAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqHfOnAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqAnalogAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> eqLmfQAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> eqHmfQAttachment;

    // Helper for resource serving (Pattern 8: Explicit URL mapping)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // Native function for keyboard MIDI playback
    juce::var sendMidiNote(const juce::Array<juce::var>& args);

    // Native function for setting custom tuning intervals
    juce::var setTuningIntervals(const juce::Array<juce::var>& args);

    // Native functions for Scala file loading
    juce::var loadScalaFile(const juce::Array<juce::var>& args);
    juce::var loadKBMFile(const juce::Array<juce::var>& args);
    juce::var getTuningIntervals(const juce::Array<juce::var>& args);

    // v1.4.0: Native functions for Scala file saving
    juce::var saveScalaFile(const juce::Array<juce::var>& args);
    juce::var saveKBMFile(const juce::Array<juce::var>& args);

    // Native function for tonic transposition
    juce::var setTonicNote(const juce::Array<juce::var>& args);

    // v1.2.3: Native function for oscilloscope waveform data
    juce::var getWaveformData(const juce::Array<juce::var>& args);

    // v1.3.0: Native functions for preset system
    juce::var savePreset(const juce::Array<juce::var>& args);
    juce::var loadPreset(const juce::Array<juce::var>& args);
    juce::var loadPresetFromFile(const juce::Array<juce::var>& args);  // v1.3.1: File dialog load
    juce::var getPresetList(const juce::Array<juce::var>& args);
    juce::var getCurrentPreset(const juce::Array<juce::var>& args);
    juce::var selectNextPreset(const juce::Array<juce::var>& args);
    juce::var selectPreviousPreset(const juce::Array<juce::var>& args);
    juce::var deletePreset(const juce::Array<juce::var>& args);

    // File chooser (must be member to stay alive during async operation)
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconMarimbaAudioProcessorEditor)
};
