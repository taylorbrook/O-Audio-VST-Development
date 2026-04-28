/*
  ==============================================================================

    BassoonVoice.h
    Modal Synthesis Bassoon - Synthesiser Voice (Stage 1 silent stub)
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 surface: full method signatures, no DSP.
    First audio: Phase 2.1.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "TuningEngine.h"          // global namespace (D2)
#include "NoteExpression.h"        // resolved via ouaricon_add_module include path
#include "BassoonSound.h"

class BassoonVoice : public juce::SynthesiserVoice
{
public:
    BassoonVoice() = default;
    ~BassoonVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    // Wiring setters (called once per voice from PluginProcessor ctor).
    // Stage 1 stores raw pointers but never dereferences them; Phase 2.1+ consumes.
    void setAPVTS               (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine        (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }

private:
    juce::AudioProcessorValueTreeState*           parameters          = nullptr;
    TuningEngine*                                 tuningEngine        = nullptr;  // D2: global namespace
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassoonVoice)
};
