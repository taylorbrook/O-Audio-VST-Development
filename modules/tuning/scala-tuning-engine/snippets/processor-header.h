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
