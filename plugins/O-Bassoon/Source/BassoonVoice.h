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
#include "ModeBank.h"
#include "Exciter.h"

class BassoonVoice : public juce::SynthesiserVoice
{
public:
    BassoonVoice() = default;
    ~BassoonVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    // Phase 2.1: non-virtual custom prepare hook. juce::SynthesiserVoice has no
    // virtual prepareToPlay in JUCE 8 — only setCurrentPlaybackSampleRate. The
    // PluginProcessor iterates voices and dispatches via dynamic_cast.
    // (Mirrors O-Wind FluteSynthVoice / O-Lyrica HarpSynthVoice precedent.)
    void prepareToPlay (double sampleRate, int maxBlockSize);

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    // Phase 2.2: tone dispatch from processor (throttled at the processor side).
    // Forwards to modeBank.setTone + applyToneChange. No APVTS read in voice.
    void setTone (float tone01) noexcept;

    // Wiring setters (called once per voice from PluginProcessor ctor).
    // Stage 1 stores raw pointers but never dereferences them; Phase 2.1+ consumes.
    void setAPVTS               (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine        (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }

private:
    static constexpr float PITCH_BEND_RANGE_SEMITONES = 2.0f;

    juce::AudioProcessorValueTreeState*           parameters          = nullptr;
    TuningEngine*                                 tuningEngine        = nullptr;  // D2: global namespace
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    ModeBank   modeBank;
    Exciter    exciter;
    juce::ADSR adsr;

    int   pitchWheelValue       = 8192;
    float pitchBendSemitones    = 0.0f;
    float currentFrequencyBase  = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassoonVoice)
};
