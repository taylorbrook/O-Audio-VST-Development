/*
   This file is part of the Ouaricon Audio scala-tuning-engine module.
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
// ===== scala-tuning-engine - PluginProcessor Header Additions =====
// Add these to your PluginProcessor.h

#pragma once
#include <JuceHeader.h>
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "TuningExporter.h"
#include "EmbeddedTunings.h"

class YourProcessor : public juce::AudioProcessor,
                      public juce::AudioProcessorValueTreeState::Listener
{
public:
    // ... existing declarations ...

    // ═══════════════════════════════════════════════════════════════════
    // TUNING ENGINE
    // ═══════════════════════════════════════════════════════════════════

    TuningEngine tuningEngine;
    ScaleGenerator scaleGenerator;
    TuningExporter tuningExporter;

    // Parameter listener callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // ═══════════════════════════════════════════════════════════════════
    // USAGE IN processBlock
    // ═══════════════════════════════════════════════════════════════════
    //
    // To get frequency for a MIDI note:
    //
    //   double freq = tuningEngine.getFrequency(midiNoteNumber);
    //
    // Example in note-on handling:
    //
    //   if (msg.isNoteOn()) {
    //       int note = msg.getNoteNumber();
    //       float velocity = msg.getVelocity() / 127.0f;
    //       double freq = tuningEngine.getFrequency(note);
    //       myVoice->startNote(note, freq, velocity);
    //   }
    //

private:
    juce::AudioProcessorValueTreeState apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YourProcessor)
};
