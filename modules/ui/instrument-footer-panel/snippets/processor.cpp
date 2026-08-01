/*
   This file is part of the Ouaricon Audio instrument-footer-panel module.
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
// ===== Footer Keyboard - PluginProcessor additions =====
// Add these methods to enable WebView keyboard → synth communication

// ----- In PluginProcessor.h -----
// Add to public section:

    // Note trigger methods for WebView keyboard
    void triggerNoteOn(int midiNote, float velocity);
    void triggerNoteOff(int midiNote);


// ----- In PluginProcessor.cpp -----
// Add these implementations:

void YourProcessorName::triggerNoteOn(int midiNote, float velocity)
{
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);

    // For synthesisers:
    synthesiser.noteOn(1, midiNote, velocity);

    // Alternative: If you use a different synth architecture, adjust accordingly:
    // mySynth.startNote(midiNote, velocity);
}

void YourProcessorName::triggerNoteOff(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);

    // For synthesisers:
    synthesiser.noteOff(1, midiNote, 0.0f, true);

    // Alternative: If you use a different synth architecture:
    // mySynth.stopNote(midiNote);
}
