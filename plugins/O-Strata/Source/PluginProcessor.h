/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Forked from O-Prism v1.24.0 (2026-09-07); see CHANGELOG.md.
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
#include "StrataSound.h"
#include "StrataVoice.h"
#include "dsp/TerrainOscillator.h"
#include "dsp/Retirable.h"
#include "dsp/ChebyshevSet.h"
#include "dsp/TerrainScheduler.h"
#include "dsp/DistortionProcessor.h"
#include "dsp/DelayProcessor.h"
#include "dsp/ReverbProcessor.h"
#include "dsp/EQProcessor.h"
#include "dsp/EnsembleChorus.h"
#include "OuariconPresetManager.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)
#include <thread>

struct TerrainImage;   // dsp/TerrainImage.h (Phase 2.5)

class OStrataAudioProcessor : public juce::AudioProcessor,
                             private juce::Timer,
                             private juce::AsyncUpdater
{
public:
    OStrataAudioProcessor();
    ~OStrataAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Strata"; }
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

    /** The codec, three branches as of v1.24.0. languageIndex() maps anything
        the branches below do not name to 0, so a hand-edited session or an
        unexpected argument from the page degrades to English rather than being
        stored unvalidated.

        THE STORED FORM IS THE BCP-47 TAG, NOT THE INDEX, and the Simplified
        Chinese tag is spelled here exactly as the page's <option value> and
        i18n.js LANGUAGES spell it — one spelling crosses the whole boundary, so
        nothing has to translate between two of them. The two literals below are
        the ONLY occurrences of that tag in this file; the gate counts them,
        which is why this sentence does not spell it a third time.

        PURE ASCII, AND THAT IS THE CONTRACT. Not one Han character exists
        anywhere under Source/ — every Chinese string lives in
        ui/public/js/i18n.js, and the one in the markup is written as numeric
        character references. This header names the language by its tag, which
        is Latin. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
    ScaleGenerator* getScaleGenerator() { return &scaleGenerator; }
    TuningExporter* getTuningExporter() { return &tuningExporter; }

    // ─── Harness-only switches (ARCH "Harness design"); never a parameter, never persisted. ───
    // Read by the voices at block start (relaxed loads). The offline render harness
    // (tests/render-harness) is the only writer; production leaves every default.
    std::atomic<uint32_t> harnessPhaseSeed { 0 };          // 0 = production address-hash phases (plan Decision 8)
    std::atomic<bool>  harnessFeedbackPathEnabled { true };  // H3 negative control
    std::atomic<bool>  harnessTerrainKernelBypass { false }; // H7 baseline (scan returns 0)
    std::atomic<bool>  harnessSingleSampleFeedback { false };// Nyquist-hunting control
    std::atomic<bool>  harnessPreFilterTap { false };        // H1 / H6 signal tap (plan Decision 3)
    std::atomic<bool>  harnessSaturationBypass { false };    // DSP-07 memcmp control
    std::atomic<bool>  harnessDcBlockerBypass { false };     // H3 |y| <= 1 row reads the pre-blocker scan bound
    std::atomic<bool>  harnessChebyshevBypass { false };     // Bandlimited = the analytic 1x path (H6 "1x" column, H1; plan Decision 30)
    std::atomic<float> harnessRampSeconds { 0.005f };        // H5 negative control (0 s ⇒ stepped)
    std::atomic<int>   harnessTerrainOverride[2] { -1, -1 }; // TerrainKind forced per oscillator (−1 = parameter)
    std::atomic<int>   harnessEdgeOverride[2] { -1, -1 };    // EdgeMode forced per oscillator (100 = HarnessWrap, H11 control)

    void setHarnessPhaseSeed (uint32_t s) { harnessPhaseSeed.store (s, std::memory_order_relaxed); }
    uint32_t getHarnessPhaseSeed() const { return harnessPhaseSeed.load (std::memory_order_relaxed); }

    // ─── Round B: published terrain objects + readout atomics (plan Decisions 33, 34) ───
    // Written by TerrainScheduler::publish* on the message thread; load-acquired once
    // per block by updateOscillatorAssignments() and handed to every voice. Old objects
    // go through retire(). Data only — nothing pushes them until Stage 3.
    std::atomic<const ChebyshevSet*> chebPtr[2]  { nullptr, nullptr };
    std::atomic<const TerrainImage*> imagePtr[2] { nullptr, nullptr };
    std::atomic<int>   chebGeneration[2]   { 0, 0 };
    std::atomic<float> chebFit[2]          { 0.0f, 0.0f };   // 0–100 %
    std::atomic<int>   chebPartialsAtC4[2] { 0, 0 };         // min (16, D_max (C4, fs)) · K_nominal
    std::atomic<bool>  chebApproximate[2]  { false, false }; // orbitK == 0 || Feedback > 0 || Feedback routed
    std::atomic<int>   imageGeneration[2]  { 0, 0 };
    std::atomic<float> imageFit[2]         { 0.0f, 0.0f };
    std::atomic<int>   publishCount { 0 };                    // prepareToPlay / setStateInformation publish nothing (harness)

    TerrainScheduler& getTerrainScheduler() { return terrainScheduler; }

    /** True while processBlock is running AND the caller is the thread running it
        (plan Decision 39: jobs assert the negation). */
    bool isInsideProcessBlockOnThisThread() const
    {
        return insideProcessBlock.load (std::memory_order_acquire)
            && audioThreadHash.load (std::memory_order_relaxed) == std::hash<std::thread::id>{} (std::this_thread::get_id());
    }

    /** Message thread: park an object the audio thread may still read (see dsp/Retirable.h). */
    void retire (std::unique_ptr<Retirable> object);
    int getRetiredCount() const { return static_cast<int> (retired.size()); }   // harness (message thread)
    uint64_t getBlockGeneration() const { return blockGeneration.load (std::memory_order_acquire); }

    /** Core 10 rings (0 = A, 1 = B); written by the display voice, read by Stage 3. */
    const CycleCapture& getCycleCapture (int oscIndex) const { return cycleCapture[juce::jlimit (0, 1, oscIndex)]; }

    // ─── Shared base-value ramp rows (plan Decision 5; RESEARCH §2.4) ───
    // 24 SmoothedValues (22 modulated base params + ModWheel + Aftertouch) filled
    // once per processBlock into rampBuffers (24 rows × samplesPerBlock) BEFORE the
    // voices render; voices index a row with the loop's ABSOLUTE sample position, so
    // juce::Synthesiser's sub-block split at MIDI events costs nothing. Row order =
    // StrataVoice's (0 Pos … 10 TerSat for A, 11–21 for B, 22 ModWheel, 23 Aftertouch);
    // rows 6 / 17 hold log2 (Terrain Freq). A host that exceeds samplesPerBlock reads
    // the last ramp sample (index clamped; never setSize on the audio thread).
    static constexpr int kNumRampRows = 24;
    const float* getRampRow (int row) const { return rampBuffers.getReadPointer (juce::jlimit (0, kNumRampRows - 1, row)); }
    int getRampCapacity() const { return rampCapacity; }

    /** MIDI note of the most recently started voice — selects the Core 10 display voice. */
    int getLastPlayedNote() const { return lastPlayedNote.load (std::memory_order_relaxed); }
    void setLastPlayedNote (int n) { lastPlayedNote.store (n, std::memory_order_relaxed); }

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

    CycleCapture cycleCapture[2];

    std::array<juce::SmoothedValue<float>, kNumRampRows> baseRamps;
    juce::AudioBuffer<float> rampBuffers;
    int rampCapacity = 0;
    std::atomic<float>* pRampParam[kNumRampRows - 2] = {};   // the 22 base parameters in row order
    void fillRampRows (int numSamples);

    int lastTuningPreset = -1;
    int lastTonic = -1;

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
    std::atomic<int> lastPlayedNote { -1 };

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

    /** Publishes chebPtr[] / imagePtr[] into the voices once per block (the renamed
        wavetable-assignment step; plan Decision 33). */
    void updateOscillatorAssignments();
    void releasePublishedImages();   // destructor helper (Phase 2.5)

    // ─── Retired-object reaper (ARCH Decision 6: type-erased) ───
    // A ChebyshevSet or a TerrainImage the audio thread may still be reading is
    // never freed in place: it is parked here (message thread only) stamped with
    // the current block generation, and freed by the timer only after the
    // generation has advanced ≥ 2 — guaranteeing a full processBlock has started
    // and finished since the pointer was unpublished, so no voice still references
    // it (the full argument lives in dsp/Retirable.h). Same class of fix as
    // O-MicrotonalSampler v1.23.2.
    std::atomic<uint64_t> blockGeneration { 0 };
    struct Retired
    {
        std::unique_ptr<Retirable> object;
        uint64_t retiredAt = 0;
    };
    std::vector<Retired> retired;   // message thread only
    void timerCallback() override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // processBlock bookkeeping for isInsideProcessBlockOnThisThread() (plan Decision 39)
    std::atomic<bool> insideProcessBlock { false };
    std::atomic<size_t> audioThreadHash { 0 };

    // LAST data member on purpose: destroyed first, and the destructor calls
    // terrainScheduler.shutdown() before anything else (plan Decision 32).
    TerrainScheduler terrainScheduler { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OStrataAudioProcessor)
};
