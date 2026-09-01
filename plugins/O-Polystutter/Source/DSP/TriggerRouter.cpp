/*
   This file is part of O-Polystutter, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

            // MIDI Note Routing — four CONSECUTIVE semitones, not a diatonic
            // run. Note names use middle C = C3 = note 60 — the JUCE keyboard
            // component's octaveNumForMiddleC default and architecture.md's
            // convention:
            // C3  (note 60) → Lane 1
            // C#3 (note 61) → Lane 2
            // D3  (note 62) → Lane 3
            // D#3 (note 63) → Lane 4
            // G3  (note 67) → Trigger all lanes (handled in PluginProcessor)
            // Every other note is ignored. The `midi` tooltip in
            // ui/public/js/i18n.js names these same notes — keep them in step.

            if (note >= 60 && note <= 63)
            {
                // Map C3-D#3 (60-63) to lanes 0-3
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
