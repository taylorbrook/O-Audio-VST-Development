/*
   This file is part of O-Bassoon, an Ouaricon Audio plugin.
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
#include "Vibrato.h"
#include "NoiseExciter.h"

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

    // Phase 2.3 aggregate setter (called from PluginProcessor each block, throttled).
    void setExpression (float attackMs, float releaseMs,
                        float vibRateHz, float vibDepthCents, float vibOnsetMs,
                        float uiBreath) noexcept;

    // Phase 2.3 voice-construction-time setter for per-voice noise seed.
    void setVoiceIndex (int idx) noexcept { voiceIndex = idx; }

    // Wiring setters (called once per voice from PluginProcessor ctor).
    // Stage 1 stores raw pointers but never dereferences them; Phase 2.1+ consumes.
    void setAPVTS               (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine        (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }

    // Stage 3 / Phase 3.2 — UI feedback accessors (header-inline; no .cpp touched).
    // breathSmoother already composes ui_breath x cc2_normalised per BassoonVoice.cpp:149.
    // vibrato.getEnvelope() returns the onset progress envelope (0..1).
    float getEffectiveBreath  () const noexcept { return breathSmoother.getCurrentValue(); }
    float getVibratoEnvelope  () const noexcept { return vibrato.getEnvelope(); }

private:
    static constexpr float PITCH_BEND_RANGE_SEMITONES = 2.0f;

    juce::AudioProcessorValueTreeState*           parameters          = nullptr;
    TuningEngine*                                 tuningEngine        = nullptr;  // D2: global namespace
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    ModeBank   modeBank;
    Exciter    exciter;       // Phase 2.4 retention (D6-rev-3) — not called from
                              // renderNextBlock at Phase 2.3; member declared
                              // verbatim for attack-character morph re-introduction.
    juce::ADSR adsr;

    int   pitchWheelValue       = 8192;
    float pitchBendSemitones    = 0.0f;
    float currentFrequencyBase  = 0.0f;

    // Phase 2.3 systems
    Vibrato      vibrato;
    NoiseExciter noiseExciter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> breathSmoother { 0.0f };

    // Phase 2.3 frequency-change throttle (vibrato + pitch-bend block-rate compose)
    float lastDispatchedFrequency = 0.0f;

    // Phase 2.3 rev-4 vibratoMult cache: pow(2, c/1200) is hot in the per-sample
    // render loop. At max LFO derivative (5 Hz × 100c = ~3142 c/s) the cents
    // value moves ~0.065 c/sample @ 48 k — recomputing only when |Δc| > 0.5 c
    // gives ~15× fewer pow calls without audible artifacts (sub-cent error).
    float lastVibratoCents       = 1.0e9f;   // sentinel forces first compute
    float cachedVibratoMult      = 1.0f;

    // Phase 2.3 expression dispatch shadows (per-voice; processor-scope shadows separate)
    float lastAppliedAttackMs   = -1.0f;
    float lastAppliedReleaseMs  = -1.0f;
    float lastAppliedVibRate    = -1.0f;
    float lastAppliedVibDepth   = -1.0f;
    float lastAppliedVibOnsetMs = -1.0f;

    // Phase 2.3 CC2-takeover state machine (500 ms idle window — OQ#5-rev-3)
    bool        cc2EverActive       = false;
    juce::int64 lastCC2SampleCount  = 0;
    juce::int64 currentSampleCount  = 0;
    float       lastUiBreath        = 0.7f;   // shadow for multiplicative compose

    // Phase 2.3 voice-index for noise seeding
    int voiceIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassoonVoice)
};
