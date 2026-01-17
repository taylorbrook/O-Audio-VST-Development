/*
  ==============================================================================

    TriggerRouter.cpp
    Ouaricon Polystutter - Trigger Router Implementation
    Phase 2.3: Central trigger management for all trigger modes

  ==============================================================================
*/

#include "TriggerRouter.h"

TriggerRouter::TriggerRouter()
{
}

void TriggerRouter::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    // v1.1.6: Calculate cooldown duration (~50ms to prevent rapid-fire triggers)
    cooldownDurationSamples = static_cast<int>(spec.sampleRate * 0.05);
    cooldownSamplesRemaining = 0;

    // Prepare main input envelope followers (1ms attack, 100ms release per architecture spec)
    envelopeFollowerLeft.prepare(spec);
    envelopeFollowerRight.prepare(spec);

    envelopeFollowerLeft.setAttackTime(0.001f);   // 1ms
    envelopeFollowerLeft.setReleaseTime(0.100f);  // 100ms

    envelopeFollowerRight.setAttackTime(0.001f);
    envelopeFollowerRight.setReleaseTime(0.100f);

    // Prepare separate sidechain envelope followers (same settings)
    sidechainEnvelopeLeft.prepare(spec);
    sidechainEnvelopeRight.prepare(spec);

    sidechainEnvelopeLeft.setAttackTime(0.001f);
    sidechainEnvelopeLeft.setReleaseTime(0.100f);

    sidechainEnvelopeRight.setAttackTime(0.001f);
    sidechainEnvelopeRight.setReleaseTime(0.100f);

    reset();
}

void TriggerRouter::reset()
{
    envelopeFollowerLeft.reset();
    envelopeFollowerRight.reset();
    sidechainEnvelopeLeft.reset();
    sidechainEnvelopeRight.reset();

    wasAboveThreshold = false;
    sidechainWasAboveThreshold = false;
    midiTriggeredLane = -1;
    freezeToggleRequested = false;

    // v1.1.6: Reset cooldown timer
    cooldownSamplesRemaining = 0;
}

bool TriggerRouter::processTriggerDetection(const juce::AudioBuffer<float>& mainInput,
                                             const juce::AudioBuffer<float>* sidechainInput,
                                             const juce::MidiBuffer& midiMessages,
                                             int numSamples)
{
    bool triggerDetected = false;

    // Reset MIDI trigger state
    midiTriggeredLane = -1;
    freezeToggleRequested = false;

    // Check envelope follower (main input)
    if (envelopeEnabled)
    {
        triggerDetected |= detectEnvelopeTrigger(mainInput, numSamples);
    }

    // Check sidechain input
    if (sidechainEnabled && sidechainInput != nullptr)
    {
        triggerDetected |= detectSidechainTrigger(*sidechainInput, numSamples);
    }

    // Parse MIDI messages
    if (midiEnabled)
    {
        parseMidiMessages(midiMessages);
        // MIDI triggers are handled per-lane in PluginProcessor
    }

    return triggerDetected;
}

void TriggerRouter::setEnvelopeEnabled(bool enabled)
{
    envelopeEnabled = enabled;
}

void TriggerRouter::setSidechainEnabled(bool enabled)
{
    sidechainEnabled = enabled;
}

void TriggerRouter::setMidiEnabled(bool enabled)
{
    midiEnabled = enabled;
}

void TriggerRouter::setEnvelopeThreshold(float thresholdDB)
{
    // Convert dB to linear amplitude
    // Parameter range: -60dB to 0dB
    envelopeThreshold = juce::Decibels::decibelsToGain(thresholdDB);
}

bool TriggerRouter::detectEnvelopeTrigger(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (numSamples == 0 || buffer.getNumChannels() == 0)
        return false;

    bool triggered = false;

    // Process envelope follower sample-by-sample for accurate trigger detection
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftSample = buffer.getSample(0, sample);
        float rightSample = buffer.getNumChannels() > 1 ? buffer.getSample(1, sample) : leftSample;

        // Process through envelope followers
        float envLeft = envelopeFollowerLeft.processSample(0, std::abs(leftSample));
        float envRight = envelopeFollowerRight.processSample(1, std::abs(rightSample));

        // Take maximum of left and right channels
        float envelopeLevel = juce::jmax(envLeft, envRight);

        // v1.1.6: Decrement cooldown timer (per-sample for accuracy)
        if (cooldownSamplesRemaining > 0)
            cooldownSamplesRemaining--;

        // Detect rising edge (crossing threshold) - only if cooldown has elapsed
        if (envelopeLevel > envelopeThreshold && !wasAboveThreshold && cooldownSamplesRemaining == 0)
        {
            triggered = true;
            // v1.1.6: Start cooldown to prevent rapid-fire triggers
            cooldownSamplesRemaining = cooldownDurationSamples;
        }

        wasAboveThreshold = (envelopeLevel > envelopeThreshold);
    }

    return triggered;
}

bool TriggerRouter::detectSidechainTrigger(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Uses SEPARATE envelope followers and state to avoid interference with main input detection
    if (numSamples == 0 || buffer.getNumChannels() == 0)
        return false;

    bool triggered = false;

    // Process sidechain envelope follower sample-by-sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftSample = buffer.getSample(0, sample);
        float rightSample = buffer.getNumChannels() > 1 ? buffer.getSample(1, sample) : leftSample;

        // Process through SIDECHAIN envelope followers (separate from main)
        float envLeft = sidechainEnvelopeLeft.processSample(0, std::abs(leftSample));
        float envRight = sidechainEnvelopeRight.processSample(1, std::abs(rightSample));

        // Take maximum of left and right channels
        float envelopeLevel = juce::jmax(envLeft, envRight);

        // Detect rising edge using SIDECHAIN threshold state
        if (envelopeLevel > envelopeThreshold && !sidechainWasAboveThreshold)
        {
            triggered = true;
        }

        sidechainWasAboveThreshold = (envelopeLevel > envelopeThreshold);
    }

    return triggered;
}

void TriggerRouter::parseMidiMessages(const juce::MidiBuffer& midiMessages)
{
    // Iterate through MIDI messages
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();

            // MIDI Note Routing (from architecture.md):
            // C3 (note 60) → Lane 1
            // D3 (note 61) → Lane 2
            // E3 (note 62) → Lane 3
            // F3 (note 63) → Lane 4
            // G3 (note 67) → Trigger all lanes (handled in PluginProcessor)
            // A3 (note 69) → Toggle freeze on all lanes

            if (note >= 60 && note <= 63)
            {
                // Map C3-F3 to lanes 0-3
                midiTriggeredLane = note - 60;
            }
            else if (note == 67)
            {
                // G3 triggers all lanes (use special value)
                midiTriggeredLane = 100;  // Special value for "all lanes"
            }
            else if (note == 69)
            {
                // A3 toggles freeze
                freezeToggleRequested = true;
            }
        }
    }
}
