/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    O-Contrabass — Audio Processor
    Specialized 4-string contrabass physical model
    (Stage 1: Foundation — APVTS shell, no DSP yet)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "OContrabassMPESynthesiser.h"
#include "DSP/MasterSaturator.h"
#include "DSP/MasterLimiter.h"
#include "DSP/StereoWidth.h"
#include "TuningEngine.h"
#include "NoteExpression.h"
#include "OuariconPresetManager.h"

class BowedContrabassVoice;     // Phase 2.3 R29 forward decl — used by getActiveVoice()

class OContrabassAudioProcessor : public juce::AudioProcessor,
                                  public juce::AudioProcessorValueTreeState::Listener,
                                  public juce::AsyncUpdater
{
public:
    OContrabassAudioProcessor();
    ~OContrabassAudioProcessor() override;

    //==============================================================================
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                            { return true; }

    const juce::String getName() const override                { return JucePlugin_Name; }
    bool acceptsMidi() const override                          { return true; }
    bool producesMidi() const override                         { return false; }
    bool isMidiEffect() const override                         { return false; }
    double getTailLengthSeconds() const override               { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram(int) override                       {}
    const juce::String getProgramName(int) override            { return {}; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    //==============================================================================
    // APVTS — public so editor (and Stage 2 voice) can attach.
    juce::AudioProcessorValueTreeState parameters;

    // Phase 2.3 R29 — accessor for harness clampedDepthMean instrumentation read.
    // Returns the first (currently only) bowed voice; nullptr if synth is empty.
    // Defined in PluginProcessor.cpp where BowedContrabassVoice is fully visible.
    BowedContrabassVoice* getActiveVoice() noexcept;

    // Phase 2.6b R40a — TuningEngine accessor + Scala/TUN file-load entry
    // point (ESCALATION-FPK1: harness-only at v1.0; Stage 3 GUI replaces with
    // FileChooser).
    TuningEngine* getTuningEngine() noexcept { return &tuningEngine; }
    bool loadScalaFile (const juce::File& sclFile);

    // Stage 3 Task 8 (D6) — preset persistence (preset-manager v1.0.4, CMake
    // include of the canonical module dir). Custom save/load callbacks round-
    // trip tuning-engine state (intervals, scale name, tonic, octave stretch)
    // — wired in the constructor.
    OuariconPresetManager& getPresetManager() noexcept { return presetManager; }

    // Stage 3 Task 12 — post-limiter/post-gain output RMS (dB) for the UI VU
    // meter. Relaxed-atomic read-only tap stored at the END of processBlock
    // (after the output-gain loop) — never feeds back into the signal path.
    float getOutputRmsDb() const noexcept { return outputRmsDb.load (std::memory_order_relaxed); }

    // Stage 3 Task 13 (D5) — DSP-true bow operating point for the Schelleng
    // wedge dot. Scans per-voice relaxed viz atomics and returns the
    // most-recently-started ACTIVE voice's effective state (fixes the
    // getActiveVoice() voice-0 hardcode for viz purposes; kNumVoices = 4).
    // Defined in PluginProcessor.cpp where BowedContrabassVoice is visible.
    struct BowStateViz { float speed; float pressure; float beta; bool active; };
    BowStateViz getBowStateViz() noexcept;

    // Phase 2.6c R41a — VST3 Note Expression (kTuningTypeID, Dorico microtonal
    // playback). Backed by note-expression module v1.1.0 (D-09 pattern,
    // O-Lyrica PluginProcessor.h:115 precedent). On AU/Standalone the module's
    // dispatch slots stay nullptr and the extension degrades to a no-op —
    // no format branching required (§24.2.3).
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // Phase 2.6c R41a — typed accessor for harness NE injection (ESCALATION-ACC1;
    // mirrors the getTuningEngine() harness-instrumentation precedent). The
    // render harness seeds pending tuning deltas in SEMITONES directly:
    //   proc.getVST3Extensions().getPendingTable()[note].store (semis, release);
    Ouaricon::NoteExpression::VST3Extensions& getVST3Extensions() noexcept { return vst3Extensions; }

    // Phase 2.6b R40a — APVTS Listener override (TUNING_SYSTEM Choice → mode
    // dispatch). CR-03: parameterChanged may fire on the audio thread under host
    // automation, so it only stores the choice atomically + triggerAsyncUpdate()
    // (RT-safe, reuses a preallocated message). The mutex-holding setMode runs on
    // the message thread in handleAsyncUpdate(). Replaces the prior audio-thread
    // MessageManager::callAsync (heap alloc + this-capturing teardown UAF).
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // CR-03 / WR-01 — message-thread apply of the staged tuning mode.
    void handleAsyncUpdate() override;

    // NOTE: Do NOT declare getLatencySamples() here — it is non-virtual in JUCE 8.
    // Use setLatencySamples(N) inside prepareToPlay() instead.

    // v1.7.0 — hover-help ("?" toggle) preference. UI state, not a parameter:
    // no automation, no preset membership. Written by the editor's
    // setTooltipsEnabled native fn (message thread), persisted as an XML
    // attribute in get/setStateInformation so it survives a session reload.
    std::atomic<bool> tooltipsEnabled { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Phase 2.6b R40a — TuningEngine member declared BEFORE synth so C++ ctor
    // init order = declaration order (Risk #32 mitigation). synth.addVoice
    // passes &tuningEngine to BowedContrabassVoice; pointer must be valid at
    // construction time.
    TuningEngine tuningEngine;

    // Phase 2.6c R41a — VST3 Note Expression extension (module-owned 128-slot
    // PendingTuningTable + raw-event buffer). MEMBER-ORDER CONTRACT: declared
    // AFTER tuningEngine and BEFORE synth so the table address handed to each
    // voice via setPendingTuningSource in the ctor addVoice loop is valid at
    // construction time (extends the Risk #32 init-order discipline).
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

    // CR-03 — choice index staged by parameterChanged (any thread) for the
    // message-thread setMode apply in handleAsyncUpdate(). Default 2 = "12-TET".
    std::atomic<int> pendingTuningChoice { 2 };

    // WR-10: 4-voice polyphony (one per EADG string) — enables double-stop drones.
    static constexpr int kNumVoices = 4;
    OContrabassMPESynthesiser synth;

    // Phase 2.6a — master output chain (post-voice-summation): Saturator →
    // Limiter → StereoWidth → OUTPUT_GAIN. OUTPUT_GAIN relocated from voice
    // per ARCHITECTURE §258 (user volume should not affect saturator color).
    MasterSaturator              masterSaturator;
    MasterLimiter                masterLimiter;
    StereoWidth                  stereoWidth;
    juce::SmoothedValue<float>   outputGainSmoothed { 1.0f };

    // WR-02: cache the master-chain parameter atomics (resolved once in the
    // constructor) so processBlock does not re-walk the APVTS std::map 4× per block.
    std::atomic<float>* satAmountParam      = nullptr;
    std::atomic<float>* limiterCeilingParam = nullptr;
    std::atomic<float>* widthParam          = nullptr;
    std::atomic<float>* outputGainParam     = nullptr;

    // Stage 3 Task 8 (D6) — declared AFTER `parameters` (public, above) so the
    // APVTS reference handed to the manager is valid at construction time.
    // Presets live in ~/Library/O-Contrabass/Presets/ (module convention).
    OuariconPresetManager presetManager { parameters, "O-Contrabass" };

    // Stage 3 Task 12 — VU feed (single writer: audio thread; reader: editor
    // timer @30 Hz). -80 dB = meter silence floor.
    std::atomic<float> outputRmsDb { -80.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessor)
};
