/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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

    BellSound.h
    O-Bells - Physical Modeling Bell Synthesizer
    Simple SynthesiserSound implementation

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class BellSound : public juce::SynthesiserSound
{
public:
    BellSound() {}

    // All MIDI notes trigger bell sounds (full keyboard range)
    bool appliesToNote(int) override { return true; }

    // All MIDI channels trigger bell sounds
    bool appliesToChannel(int) override { return true; }
};
