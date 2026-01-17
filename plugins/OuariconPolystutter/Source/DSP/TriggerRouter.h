/*
  ==============================================================================

    TriggerRouter.h
    Ouaricon Polystutter - Trigger Router
    Phase 2.3: Central trigger management for all trigger modes

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class TriggerRouter
{
public:
    TriggerRouter();
    ~TriggerRouter() = default;

    // Prepare for playback
    void prepare(const juce::dsp::ProcessSpec& spec);

    // Reset state
    void reset();

    // Process a block to detect triggers
    // Returns true if trigger detected this block
    bool processTriggerDetection(const juce::AudioBuffer<float>& mainInput,
                                  const juce::AudioBuffer<float>* sidechainInput,
                                  const juce::MidiBuffer& midiMessages,
                                  int numSamples);

    // Trigger mode enables
    void setEnvelopeEnabled(bool enabled);
    void setSidechainEnabled(bool enabled);
    void setMidiEnabled(bool enabled);

    // Envelope follower parameters
    void setEnvelopeThreshold(float thresholdDB);  // -60 to 0 dB

    // MIDI note routing (returns lane index 0-3, or -1 for no trigger)
    int getMidiTriggeredLane() const { return midiTriggeredLane; }

    // Check for global freeze toggle (MIDI note A3 = note 69)
    bool shouldToggleFreeze() const { return freezeToggleRequested; }
    void clearFreezeToggle() { freezeToggleRequested = false; }

private:
    // DSP components - Main input envelope followers
    juce::dsp::BallisticsFilter<float> envelopeFollowerLeft;
    juce::dsp::BallisticsFilter<float> envelopeFollowerRight;

    // DSP components - Separate sidechain envelope followers (avoid state corruption)
    juce::dsp::BallisticsFilter<float> sidechainEnvelopeLeft;
    juce::dsp::BallisticsFilter<float> sidechainEnvelopeRight;

    // State
    double currentSampleRate = 48000.0;
    bool envelopeEnabled = false;
    bool sidechainEnabled = false;
    bool midiEnabled = false;

    // Main envelope follower state
    float envelopeThreshold = 0.1f;  // Linear amplitude threshold
    bool wasAboveThreshold = false;

    // v1.1.6: Cooldown timer to prevent rapid-fire triggers
    // Prevents distortion from triggers firing too rapidly on transient-heavy material
    int cooldownSamplesRemaining = 0;
    int cooldownDurationSamples = 0;  // Set in prepare() based on sample rate (~50ms)

    // Sidechain envelope follower state (separate to avoid interference)
    bool sidechainWasAboveThreshold = false;

    // v1.1.9: Separate cooldown for sidechain (same logic as ENV)
    int sidechainCooldownSamplesRemaining = 0;

    // MIDI trigger state
    int midiTriggeredLane = -1;  // -1 = no trigger, 0-3 = lane 1-4
    bool freezeToggleRequested = false;

    // Helper functions
    bool detectEnvelopeTrigger(const juce::AudioBuffer<float>& buffer, int numSamples);
    bool detectSidechainTrigger(const juce::AudioBuffer<float>& buffer, int numSamples);
    void parseMidiMessages(const juce::MidiBuffer& midiMessages);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerRouter)
};
