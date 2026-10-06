/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Voice + Sound
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): a SILENT voice. The Synthesiser allocates and
    steals these voices and routes MIDI to them, but the render callback
    writes nothing. Stage 2 fills in the oscillator, envelopes, LFO and bend.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

struct WtSound final : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class WtVoice final : public juce::SynthesiserVoice
{
public:
    // Non-virtual: SynthesiserVoice has no prepare hook, so the processor
    // dispatches here through dynamic_cast<WtVoice*>.
    void prepareToPlay (double sampleRate, int /*maxBlock*/) { sr = sampleRate; }

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<WtSound*> (s) != nullptr; }

    void startNote (int, float, juce::SynthesiserSound*, int currentPitchWheelPosition) override
    {
        pitchWheelPos = currentPitchWheelPosition;   // SEED the wheel at note-on (restrike memory)
    }

    void stopNote (float, bool) override { clearCurrentNote(); }   // no envelope yet -> free at once
    void pitchWheelMoved (int v) override { pitchWheelPos = v; }   // Stage 2: +/-2 st -> mip level
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int, int) override {}   // silent until Stage 2

private:
    double sr = 44100.0;
    int pitchWheelPos = 8192;   // centre
};
