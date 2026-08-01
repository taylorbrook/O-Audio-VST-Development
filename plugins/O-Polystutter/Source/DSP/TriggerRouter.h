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

private:
    // State
    bool midiEnabled = false;

    // MIDI trigger state
    int midiTriggeredLane = -1;  // -1 = no trigger, 0-3 = lane 1-4, 100 = all lanes

    // Helper functions
    void parseMidiMessages(const juce::MidiBuffer& midiMessages);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerRouter)
};
