/*
  ==============================================================================

    TriggerRouter.h
    Ouaricon Polystutter - Trigger Router
    v1.3.0: Simplified to MIDI-only trigger management (removed ENV and SC)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class TriggerRouter
{
public:
    TriggerRouter();
    ~TriggerRouter() = default;

    // Reset state
    void reset();

    // Process MIDI messages for trigger detection
    void processMidiTriggerDetection(const juce::MidiBuffer& midiMessages);

    // MIDI trigger mode enable
    void setMidiEnabled(bool enabled);

    // MIDI note routing (returns lane index 0-3, or -1 for no trigger, 100 for all lanes)
    int getMidiTriggeredLane() const { return midiTriggeredLane; }

    // Check for global freeze toggle (MIDI note A3 = note 69)
    bool shouldToggleFreeze() const { return freezeToggleRequested; }
    void clearFreezeToggle() { freezeToggleRequested = false; }

private:
    // State
    bool midiEnabled = false;

    // MIDI trigger state
    int midiTriggeredLane = -1;  // -1 = no trigger, 0-3 = lane 1-4, 100 = all lanes
    bool freezeToggleRequested = false;

    // Helper functions
    void parseMidiMessages(const juce::MidiBuffer& midiMessages);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerRouter)
};
