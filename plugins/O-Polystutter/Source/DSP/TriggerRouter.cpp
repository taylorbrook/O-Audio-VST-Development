/*
  ==============================================================================

    TriggerRouter.cpp
    Ouaricon Polystutter - Trigger Router Implementation
    v1.3.0: Simplified to MIDI-only trigger management (removed ENV and SC)

  ==============================================================================
*/

#include "TriggerRouter.h"

TriggerRouter::TriggerRouter()
{
}

void TriggerRouter::reset()
{
    midiTriggeredLane = -1;
}

void TriggerRouter::processMidiTriggerDetection(const juce::MidiBuffer& midiMessages)
{
    // Reset MIDI trigger state each block
    midiTriggeredLane = -1;

    // Parse MIDI messages if MIDI trigger mode is enabled
    if (midiEnabled)
    {
        parseMidiMessages(midiMessages);
    }
}

void TriggerRouter::setMidiEnabled(bool enabled)
{
    midiEnabled = enabled;
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
        }
    }
}
