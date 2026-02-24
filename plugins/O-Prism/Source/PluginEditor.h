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

class OPrismAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit OPrismAudioProcessorEditor (OPrismAudioProcessor&);
    ~OPrismAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OPrismAudioProcessor& processorRef;

    // Parameter IDs for slider relays
    static constexpr const char* sliderParamIds[] = {
        // Osc A (10 — oscATable is Int but uses slider relay)
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
        // LFO (4 — rate + shape only, routing via mod matrix)
        "lfo1Rate", "lfo1Shape",
        "lfo2Rate", "lfo2Shape",
        // Mod Matrix (48 — source + dest + amount per slot, toggles handled separately)
        "modSlot0Src", "modSlot0Dst", "modSlot0Amt",
        "modSlot1Src", "modSlot1Dst", "modSlot1Amt",
        "modSlot2Src", "modSlot2Dst", "modSlot2Amt",
        "modSlot3Src", "modSlot3Dst", "modSlot3Amt",
        "modSlot4Src", "modSlot4Dst", "modSlot4Amt",
        "modSlot5Src", "modSlot5Dst", "modSlot5Amt",
        "modSlot6Src", "modSlot6Dst", "modSlot6Amt",
        "modSlot7Src", "modSlot7Dst", "modSlot7Amt",
        "modSlot8Src", "modSlot8Dst", "modSlot8Amt",
        "modSlot9Src", "modSlot9Dst", "modSlot9Amt",
        "modSlot10Src", "modSlot10Dst", "modSlot10Amt",
        "modSlot11Src", "modSlot11Dst", "modSlot11Amt",
        "modSlot12Src", "modSlot12Dst", "modSlot12Amt",
        "modSlot13Src", "modSlot13Dst", "modSlot13Amt",
        "modSlot14Src", "modSlot14Dst", "modSlot14Amt",
        "modSlot15Src", "modSlot15Dst", "modSlot15Amt",
        // Global (3)
        "masterVol", "oscMix", "polyphony"
    };

    static constexpr int numSliderParams = 120;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Member declaration order (C++ destroys in reverse)
    // 1. Relays destroyed LAST
    // 2. WebView destroyed SECOND
    // 3. Attachments destroyed FIRST (WebView still alive — safe)
    // ═══════════════════════════════════════════════════════════════════

    // Bypass toggle param IDs
    static constexpr const char* bypassParamIds[] = {
        "reverbBypass", "delayBypass", "chorusBypass", "distBypass", "eqBypass"
    };
    static constexpr int numBypassParams = 5;

    // Mod matrix slot toggle param IDs
    static constexpr const char* modSlotToggleIds[] = {
        "modSlot0On", "modSlot1On", "modSlot2On", "modSlot3On",
        "modSlot4On", "modSlot5On", "modSlot6On", "modSlot7On",
        "modSlot8On", "modSlot9On", "modSlot10On", "modSlot11On",
        "modSlot12On", "modSlot13On", "modSlot14On", "modSlot15On"
    };
    static constexpr int numModSlotToggles = 16;

    // 1. RELAYS (destroyed last)
    std::vector<std::unique_ptr<juce::WebSliderRelay>> sliderRelays;
    std::unique_ptr<juce::WebToggleButtonRelay> delaySyncRelay;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> bypassRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> modSlotToggleRelays;

    // 2. WEBVIEW (destroyed second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed first — WebView still alive)
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delaySyncAttachment;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OPrismAudioProcessorEditor)
};
