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

class OPrismAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OPrismAudioProcessorEditor (OPrismAudioProcessor&);
    ~OPrismAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OPrismAudioProcessor& processorRef;

    // Parameter IDs for slider relays (67 slider params)
    static constexpr const char* sliderParamIds[] = {
        // Osc A (9 slider params — oscATable is Int but uses slider relay)
        "oscATable", "oscAPos", "oscALevel", "oscAPan", "oscACoarse",
        "oscAFine", "oscAPhase", "oscAUnison", "oscADetune", "oscAWidth",
        // Osc B (10)
        "oscBTable", "oscBPos", "oscBLevel", "oscBPan", "oscBCoarse",
        "oscBFine", "oscBPhase", "oscBUnison", "oscBDetune", "oscBWidth",
        // Sub + Noise (5 — includes Choice params via slider relay)
        "subShape", "subOctave", "subLevel", "noiseType", "noiseLevel",
        // Amp Envelope (4)
        "ampAttack", "ampDecay", "ampSustain", "ampRelease",
        // Filter Envelope (5)
        "filtAttack", "filtDecay", "filtSustain", "filtRelease", "filtEnvDepth",
        // Filter A (5)
        "filtAType", "filtACutoff", "filtARes", "filtADrive", "filtAKeyTrack",
        // Filter B (5)
        "filtBType", "filtBCutoff", "filtBRes", "filtBDrive", "filtBKeyTrack",
        // Filter Routing (1)
        "filtRouting",
        // Tuning (7 — tuningPreset/tonic/glideMode are Choice, use slider relay)
        "tuningPreset", "tonic", "masterTune", "octaveStretch",
        "pitchBendRange", "glideMode", "glideTime",
        // Reverb (4)
        "reverbSize", "reverbDamp", "reverbPredelay", "reverbMix",
        // Delay (4 — delaySync is bool, handled separately)
        "delayTime", "delayFeedback", "delayMode", "delayMix",
        // Chorus (3)
        "chorusRate", "chorusDepth", "chorusMix",
        // Distortion (3)
        "distType", "distDrive", "distMix",
        // EQ (4)
        "eqLowGain", "eqMidGain", "eqMidFreq", "eqHighGain",
        // Global (3)
        "masterVol", "oscMix", "polyphony"
    };

    static constexpr int numSliderParams = 67;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Member declaration order (C++ destroys in reverse)
    // 1. Relays destroyed LAST
    // 2. WebView destroyed SECOND
    // 3. Attachments destroyed FIRST (WebView still alive — safe)
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS (destroyed last)
    std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;
    std::unique_ptr<juce::WebToggleButtonRelay> delaySyncRelay;

    // 2. WEBVIEW (destroyed second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed first — WebView still alive)
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delaySyncAttachment;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource>
        getResource (const juce::String& url);

    // Native function registration
    juce::WebBrowserComponent::Options addNativeFunctions (
        juce::WebBrowserComponent::Options options);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OPrismAudioProcessorEditor)
};
