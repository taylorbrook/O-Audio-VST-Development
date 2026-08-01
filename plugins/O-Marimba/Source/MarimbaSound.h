/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
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

    MarimbaSound.h
    Simple SynthesiserSound subclass for O-Marimba
    Indicates that all notes can be played by MarimbaVoice

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class MarimbaSound : public juce::SynthesiserSound
{
public:
    MarimbaSound() {}

    // All MIDI notes can trigger this sound
    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }

    // All channels can play this sound (omni mode)
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};
