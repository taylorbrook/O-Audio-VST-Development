/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    PluginProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "PrismSound.h"
#include "PrismVoice.h"
#include "dsp/WavetableData.h"
#include "dsp/WavetableGenerator.h"
#include "dsp/WavetableFactory.h"
#include "dsp/DistortionProcessor.h"
#include "dsp/DelayProcessor.h"
#include "dsp/ReverbProcessor.h"
#include "dsp/EQProcessor.h"
#include "dsp/EnsembleChorus.h"
#include "dsp/UserWavetableManager.h"
#include "dsp/WavetableEditor.h"
#include "OuariconPresetManager.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

class OPrismAudioProcessor : public juce::AudioProcessor,
                             private juce::Timer,
                             private juce::AsyncUpdater
{
public:
    OPrismAudioProcessor();
    ~OPrismAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Prism"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    // ── UI LANGUAGE (v1.21.0) ──────────────────────────────────────────────
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their interface in. It rides the APVTS state tree as a
    // non-parameter property, which is this plugin's FIRST such property —
    // getStateInformation/setStateInformation were the only handlers before.
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
    ScaleGenerator* getScaleGenerator() { return &scaleGenerator; }
    TuningExporter* getTuningExporter() { return &tuningExporter; }

    const WavetableData* getFactoryTable (int index) const
    {
        if (index >= 0 && index < static_cast<int> (factoryTables.size()))
            return factoryTables[static_cast<size_t> (index)].get();
        return nullptr;
    }

    int getNumFactoryTables() const { return static_cast<int> (factoryTables.size()); }

    juce::String getTableName (int index) const
    {
        if (index >= 0 && index < static_cast<int> (tableInfoList.size()))
            return tableInfoList[static_cast<size_t> (index)].name;
        return {};
    }

    juce::String getTableCategory (int index) const
    {
        if (index >= 0 && index < static_cast<int> (tableInfoList.size()))
            return tableInfoList[static_cast<size_t> (index)].category;
        return {};
    }

    // ─── User Wavetable API ───

    UserWavetableManager& getUserWavetableManager() { return userWavetableManager; }

    /** Select a user wavetable override for an oscillator (0=A, 1=B). */
    void selectUserWavetable (int oscIndex, const juce::String& name);

    /** Clear user wavetable override, revert to factory. */
    void clearUserWavetableOverride (int oscIndex);

    /** Get the active wavetable for an oscillator (factory or user). */
    const WavetableData* getActiveOscTable (int oscIndex) const;

    /** Get the name of the active user table (empty if factory). */
    juce::String getActiveUserTableName (int oscIndex) const;

    /** Check if a user table is active for an oscillator. */
    bool isUserTableActive (int oscIndex) const;

    /** Delete a user wavetable: clears any osc override bound to it, removes
        it from the manager, and retires the buffer RT-safely. */
    bool deleteUserWavetable (const juce::String& name);

    /** Save the editor's working table under `name`: writes the .wav,
        registers the new table, re-publishes osc pointers, and retires any
        replaced table RT-safely. */
    bool saveEditedWavetable (const juce::String& name);

    // ─── Wavetable Editor API ───

    WavetableEditor& getWavetableEditor() { return wavetableEditor; }

    /** Start editing: clone the active table into working copy, point oscillator at it. */
    void startEditing (int oscIndex);

    /** Stop editing: revert oscillator to its original table source. */
    void stopEditing (int oscIndex);

    /** Get which oscillator is being edited (-1 = none). */
    int getEditingOscIndex() const { return editingOscIndex; }

    /** Get current mod wheel value (0..1) for modulation matrix */
    float getModWheelValue() const { return modWheelValue.load (std::memory_order_relaxed); }

    /** Get current aftertouch value (0..1) for modulation matrix */
    float getAftertouchValue() const { return aftertouchValue.load (std::memory_order_relaxed); }

    /** Get current BPM from host transport (default 120 when not playing) */
    double getCurrentBPM() const { return currentBPM.load (std::memory_order_relaxed); }

    /** Get shared global LFO phase [0, 1) for free-running mode. idx is 0..3. */
    double getGlobalLfoPhase (int idx) const
    {
        return (idx >= 0 && idx < 4) ? globalLfoPhase[static_cast<size_t> (idx)] : 0.0;
    }

    /** Frequency of the most recently started note (0.0 if none yet).
        Seeds glide "Always" mode on fresh voices (WR-06). */
    double getLastPlayedFrequency() const { return lastPlayedFrequency.load (std::memory_order_relaxed); }
    void setLastPlayedFrequency (double freq) { lastPlayedFrequency.store (freq, std::memory_order_relaxed); }

    /** True if any MIDI note other than excludeNote is currently held.
        Gates glide "Legato" mode (WR-06). */
    bool isAnyNoteHeldExcept (int excludeNote) const
    {
        for (int i = 0; i < 128; ++i)
            if (i != excludeNote && noteStates[static_cast<size_t> (i)].load (std::memory_order_relaxed))
                return true;
        return false;
    }

    // ─── Preset Manager (factory + user presets) ───
    OuariconPresetManager& getPresetManager() { return presetManager; }

    /** Get currently active MIDI notes and their microtonal frequencies */
    std::vector<std::pair<int, double>> getActiveNotes()
    {
        std::vector<std::pair<int, double>> result;
        for (int i = 0; i < 128; ++i)
        {
            if (noteStates[static_cast<size_t> (i)].load (std::memory_order_relaxed))
                result.push_back ({ i, tuningEngine.getFrequency (i) });
        }
        return result;
    }

private:
    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;
    juce::Synthesiser synthesiser;
    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    TuningEngine tuningEngine;
    ScaleGenerator scaleGenerator;
    TuningExporter tuningExporter;

    // Factory wavetable library (28 tables across 5 categories)
    std::vector<std::unique_ptr<WavetableData>> factoryTables;
    std::vector<TableInfo> tableInfoList;
    int lastTuningPreset = -1;
    int lastTonic = -1;

    // User wavetable system
    UserWavetableManager userWavetableManager;
    WavetableEditor wavetableEditor;
    int editingOscIndex = -1;
    juce::String userTableNameA;
    juce::String userTableNameB;
    std::atomic<const WavetableData*> userTablePtrA { nullptr };
    std::atomic<const WavetableData*> userTablePtrB { nullptr };
    const WavetableData* lastAssignedTableA = nullptr;
    const WavetableData* lastAssignedTableB = nullptr;

    // Effects chain (float precision)
    DistortionProcessor distortion;
    EnsembleChorus chorus;
    DelayProcessor delay;
    EQProcessor eq;
    ReverbProcessor reverbProcessor;

    // Cached APVTS atomic pointers for the per-block FX configure step.
    // Initialized once in the constructor; APVTS does not reallocate
    // parameter memory after construction so these stay valid for the
    // lifetime of the processor (also used to avoid 30+ hash-map probes
    // per audio block).
    std::atomic<float>* pDistBypass     = nullptr;
    std::atomic<float>* pDistType       = nullptr;
    std::atomic<float>* pDistDrive      = nullptr;
    std::atomic<float>* pDistMix        = nullptr;
    std::atomic<float>* pChorusBypass   = nullptr;
    std::atomic<float>* pChorusRate     = nullptr;
    std::atomic<float>* pChorusDepth    = nullptr;
    std::atomic<float>* pChorusMix      = nullptr;
    std::atomic<float>* pDelayBypass    = nullptr;
    std::atomic<float>* pDelayTime      = nullptr;
    std::atomic<float>* pDelayFeedback  = nullptr;
    std::atomic<float>* pDelayMode      = nullptr;
    std::atomic<float>* pDelayMix       = nullptr;
    std::atomic<float>* pReverbBypass   = nullptr;
    std::atomic<float>* pReverbSize     = nullptr;
    std::atomic<float>* pReverbDamp     = nullptr;
    std::atomic<float>* pReverbPredelay = nullptr;
    std::atomic<float>* pReverbMix      = nullptr;
    std::atomic<float>* pReverbModDepth = nullptr;
    std::atomic<float>* pReverbModRate  = nullptr;
    std::atomic<float>* pEqBypass       = nullptr;
    std::atomic<float>* pEqLowGain      = nullptr;
    std::atomic<float>* pEqMidGain      = nullptr;
    std::atomic<float>* pEqMidFreq      = nullptr;
    std::atomic<float>* pEqHighGain     = nullptr;
    std::atomic<float>* pDelaySync      = nullptr;
    std::atomic<float>* pDelayDivision  = nullptr;

    // Cached global-LFO param pointers (CR-06: advanceGlobalLfoPhases was
    // building ~28 juce::Strings per block for these lookups)
    std::atomic<float>* pLfoSync[4]  = {};
    std::atomic<float>* pLfoRate[4]  = {};
    std::atomic<float>* pLfoDiv[4]   = {};
    std::atomic<float>* pLfoShape[4] = {};

    // Cached tuning/global param pointers (CR-05 / IN-01)
    std::atomic<float>* pMasterTune     = nullptr;
    std::atomic<float>* pOctaveStretch  = nullptr;
    std::atomic<float>* pPitchBendRange = nullptr;
    std::atomic<float>* pTuningPreset   = nullptr;
    std::atomic<float>* pTonic          = nullptr;
    std::atomic<float>* pStereoWidth    = nullptr;
    std::atomic<float>* pMasterVol      = nullptr;
    std::atomic<float>* pOscATable      = nullptr;
    std::atomic<float>* pOscBTable      = nullptr;

    // ─── Deferred TuningEngine sync (CR-05) ───
    // processBlock only detects parameter changes; the engine itself (mutex,
    // heap-allocating Strings/vectors, 128-note table rebuild) is mutated on
    // the message thread via handleAsyncUpdate.
    float lastMasterTune = -1.0e9f;
    float lastOctaveStretch = -1.0e9f;
    float lastPitchBendRange = -1.0e9f;
    std::atomic<bool> pendingTuningPresetChange { false };
    void handleAsyncUpdate() override;

    // ─── FX activity gates (WR-07) ───
    // When an effect stops processing (bypass or mix -> 0) its buffers are
    // reset once, so stale audio can't replay when it re-engages.
    bool distWasActive = false;
    bool chorusWasActive = false;
    bool delayWasActive = false;
    bool reverbWasActive = false;
    bool eqWasActive = false;

    // ─── Processor-level mod matrix for global FX destinations (WR-02) ───
    // Sources: global LFOs 1-4, ModWheel, Aftertouch (per-voice sources are 0).
    // Destinations consumed here: Reverb/Delay/Chorus/Dist Mix, Master Vol.
    ModulationMatrix fxModMatrix;
    LFO fxLfo[4];

    // Last started note frequency for glide "Always" seeding (WR-06)
    std::atomic<double> lastPlayedFrequency { 0.0 };

    // Master volume and stereo width (smoothed)
    juce::SmoothedValue<float> masterVolSmoothed { 0.8f };
    juce::SmoothedValue<float> stereoWidthSmoothed { 1.0f };

    // Active MIDI note tracking for TrueKeys visualization
    std::array<std::atomic<bool>, 128> noteStates {};

    // MIDI CC state for modulation matrix (global sources)
    std::atomic<float> modWheelValue { 0.0f };
    std::atomic<float> aftertouchValue { 0.0f };

    // Host transport BPM for tempo-synced LFOs
    std::atomic<double> currentBPM { 120.0 };

    // Shared global LFO phase accumulators for free-running mode.
    // Advanced at block rate after renderNextBlock; voices copy from here when lfoNFreeRun=true.
    std::array<double, 4> globalLfoPhase {};
    void advanceGlobalLfoPhases (int numSamples, double sampleRate);

    void updateWavetableAssignments();

    // ─── Retired-table reaper ───
    // A WavetableData the audio thread may still be reading is never freed
    // in place: it is parked here (message thread only) stamped with the
    // current block generation, and freed by the timer only after the
    // generation has advanced ≥ 2 — guaranteeing a full processBlock has
    // started and finished since the pointers were unpublished, so no voice
    // still references it. Same class of fix as O-MicrotonalSampler v1.23.2.
    std::atomic<uint64_t> blockGeneration { 0 };
    struct RetiredTable
    {
        std::unique_ptr<WavetableData> table;
        uint64_t retiredAt = 0;
    };
    std::vector<RetiredTable> retiredTables;   // message thread only
    void retireTable (std::unique_ptr<WavetableData> table);
    void timerCallback() override;

    // Single source of truth for "user pointer takes priority over factory index" lookup.
    // Used by updateWavetableAssignments (audio-thread voice-assignment) and getActiveOscTable.
    const WavetableData* resolveActiveTable (int oscIndex) const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OPrismAudioProcessor)
};
