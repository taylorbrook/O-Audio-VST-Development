/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include "dsp/DelayBuffer.h"
#include "dsp/GrainPool.h"
#include "dsp/GrainScheduler.h"
#include "dsp/TempoTracker.h"
#include "dsp/PitchEngine.h"
#include "dsp/EuclideanGenerator.h"
#include "dsp/FreezeManager.h"
#include "dsp/BinauralDecoder.h"
#include "dsp/GrainTrajectory.h"
#include "dsp/TripleBuffer.h"

struct GrainVizSnapshot
{
    struct Voice
    {
        bool active = false;
        float positionNorm = 0.0f;
        float pitchSemitones = 0.0f;
        float pan = 0.5f;
        float envelope = 0.0f;
        bool reverse = false;
        bool frozen = false;
        // Spatial visualization
        float azimuth = 0.0f;
        float elevation = 0.0f;
        float distance = 0.5f;
    };
    std::array<Voice, 64> voices {};
    int activeCount = 0;

    // Euclidean visualization (copied from audio thread, safe to read from GUI)
    std::array<bool, 16> euclideanPattern {};
    int euclideanSteps = 0;
    int euclideanStep = 0;
    int euclideanRotation = 0;
};

class GrainScatterProcessor : public juce::AudioProcessor
{
public:
    GrainScatterProcessor();
    ~GrainScatterProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    // ------------------------------------------------------------------------
    // v2.5.0 - the UI language. 0 = en, 1 = fr.
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile (juce::String is not trivially copyable), so the audio-safe form
    // is an index behind the two-function codec below while the PERSISTED form
    // stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    // v3.1.0: RNG seed (message thread; the audio thread picks it up next block).
    uint64_t getRngSeed() const { return rngSeed.load (std::memory_order_relaxed); }
    void setRngSeed (uint64_t s) { rngSeed.store (s, std::memory_order_relaxed); }

    /** The codec. languageIndex() maps anything it does not recognise to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated.

        Three languages since v2.8.0. What crosses this line is a language CODE,
        which is an ASCII identifier in every language — the Simplified Chinese
        copy lives in the UI table (Source/ui/public/js/i18n.js) and the one Han
        string in the markup is written as numeric character references, so no
        Chinese character exists anywhere under Source/. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    // ------------------------------------------------------------------------
    // v3.0.0 — pitch collections: per-collection degree masks and the custom
    // table. Session state, NOT parameters (a 64-bit mask has no automation
    // meaning). Message-thread API; the audio thread reads masks atomically
    // and copies the custom table under a try-lock when its version moves.
    // ------------------------------------------------------------------------
    /** Mask as a '0'/'1' string, one char per degree (JS cannot hold a uint64). */
    juce::String getPitchMaskString (int collection) const;
    void setPitchMaskString (int collection, const juce::String& bits);

    /** Replaces the custom collection. Returns an error KEY (empty = ok). */
    juce::String setCustomPitchesFromText (const juce::String& text, const juce::String& name);
    juce::String setCustomPitchesFromScala (const juce::String& sclBody, const juce::String& fileName);

    PitchCollections::Table getCustomTable() const;
    juce::String getCustomText() const;
    juce::String getCustomName() const;

    /** Bumped on every mask/custom change and state load; the editor polls it
        to refresh the pitch chips after a session or preset load. */
    std::atomic<uint32_t> pitchStateRevision { 1 };

    // Visualization getter (lock-free triple buffer, called from GUI thread)
    const GrainVizSnapshot& getVizSnapshot() { return vizBuffer.read(); }

    void reset() override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached parameter pointers (real-time safe access)
    std::atomic<float>* grainSizeParam = nullptr;
    std::atomic<float>* densityParam = nullptr;
    std::atomic<float>* pitchRandomParam = nullptr;
    std::atomic<float>* panRandomParam = nullptr;
    std::atomic<float>* scaleParam = nullptr;
    std::atomic<float>* rootNoteParam = nullptr;
    std::atomic<float>* reverseParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* dryLevelParam = nullptr;    // v3.0.0: replaces dry_wet
    std::atomic<float>* wetLevelParam = nullptr;
    std::atomic<float>* pitchRangeParam = nullptr;  // v3.0.0
    std::atomic<float>* pitchTransposeParam = nullptr;
    std::atomic<float>* pitchFineParam = nullptr;
    std::atomic<float>* pitchDirectionParam = nullptr;
    std::atomic<float>* syncModeParam = nullptr;
    std::atomic<float>* probabilityParam = nullptr;
    std::atomic<float>* repeatsParam = nullptr;
    std::atomic<float>* spreadParam = nullptr;
    std::atomic<float>* pitchModeParam = nullptr;
    std::atomic<float>* freezeParam = nullptr;
    std::atomic<float>* euclideanPulsesParam = nullptr;
    std::atomic<float>* euclideanStepsParam = nullptr;
    std::atomic<float>* euclideanRotationParam = nullptr;
    std::atomic<float>* euclideanSwingParam = nullptr;
    std::atomic<float>* stutterGateParam = nullptr;
    std::atomic<float>* sizeRandomParam = nullptr;
    std::atomic<float>* ampRandomParam = nullptr;
    std::atomic<float>* scanPositionParam = nullptr;

    // Grain envelope shape
    std::atomic<float>* grainShapeParam = nullptr;

    // Spatial parameters (8 new)
    std::atomic<float>* spatialModeParam = nullptr;
    std::atomic<float>* azimuthParam = nullptr;
    std::atomic<float>* elevationParam = nullptr;
    std::atomic<float>* azSpreadParam = nullptr;
    std::atomic<float>* elSpreadParam = nullptr;
    std::atomic<float>* distanceParam = nullptr;
    std::atomic<float>* spatialWidthParam = nullptr;
    std::atomic<float>* trajectoryParam = nullptr;
    std::atomic<float>* trajSpeedParam = nullptr;
    std::atomic<float>* distLpfParam = nullptr;
    std::atomic<float>* dopplerParam = nullptr;
    std::atomic<float>* spatialSmoothParam = nullptr;

    // DSP components
    DelayBuffer delayBuffer;
    GrainPool grainPool;
    GrainScheduler scheduler;
    TempoTracker tempoTracker;
    PitchEngine pitchEngine;
    FreezeManager freezeManager;
    BinauralDecoder binauralDecoder;

    // HOA3 internal accumulation bus (16 channels)
    juce::AudioBuffer<float> hoaBus;

    // Heap-allocated binaural decode buffers (sized in prepareToPlay)
    std::vector<float> binauralL, binauralR;

    // Distance LPF (1-pole per channel for air absorption approximation)
    float distanceLpfState[2] = { 0.0f, 0.0f };

    // v3.0.0 pitch-collection state (see the public API above)
    std::array<std::atomic<uint64_t>, PitchCollections::kCount> pitchMasks;
    mutable juce::SpinLock customLock;              // guards customTable + strings
    PitchCollections::Table customTable;            // message-thread master
    juce::String customText, customName;
    std::atomic<uint32_t> customVersion { 1 };
    PitchCollections::Table audioCustomTable;       // audio-thread copy
    uint32_t audioCustomVersion = 0;

    void resetPitchCollectionState();               // all masks full, custom = 12-EDO
    static void migrateLegacyState (juce::ValueTree& tree,
                                    const juce::Array<juce::AudioProcessorParameter*>& params);

    // SmoothedValues
    juce::SmoothedValue<float> dryGainSmoothed;     // v3.0.0: linear gains
    juce::SmoothedValue<float> wetGainSmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
    juce::SmoothedValue<float> lpfCoeffSmoothed;   // WR-08: de-zipper the distance LPF cutoff

    // Euclidean pattern cache (audio thread only writes; atomics for cross-thread reads)
    std::array<bool, 16> euclideanPattern {};
    std::atomic<int> cachedEuclideanSteps { 0 };
    std::atomic<int> cachedEuclideanPulses { 0 };

    // Visualization triple-buffer (lock-free audio→GUI)
    TripleBuffer<GrainVizSnapshot> vizBuffer;

    // Freeze state tracking
    bool wasFrozen = false;


    // Feedback state
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    // v3.2.0 density compensation (replaces the wet tanh): a one-pole estimate of the
    // wet power P over ~100 ms, applied as 1 / sqrt(max(1, P)).
    float densityPower = 0.0f;
    float densityCoeff = 0.0f;
    static constexpr double kDensitySmoothSeconds = 0.1;
    static constexpr float  kCoherentSprayLimit = 0.25f;   // Spread (fraction of a grain) where rho reaches 0

    float nextDensityGain (float live, float mu2, float mu1Sq, float rho) noexcept
    {
        const float p = live * mu2 + rho * live * juce::jmax (0.0f, live - 1.0f) * mu1Sq;
        densityPower += densityCoeff * (p - densityPower);
        return 1.0f / std::sqrt (juce::jmax (1.0f, densityPower));
    }

    // Spawn request buffer (pre-allocated)
    std::vector<SpawnRequest> spawnRequests;

    // v3.1.0: counter-based RNG (dsp/CounterRng.h). The seed is per instance, saved
    // with the session as "rngSeed"; draws are keyed by event counters that reset in
    // reset()/prepareToPlay(), so a render is a pure function of seed + input + params.
    std::atomic<uint64_t> rngSeed { 0 };
    uint64_t grainCounter = 0;            // grains spawned since reset (grain RNG event index)
    juce::int64 samplesProcessed = 0;     // absolute sample clock since reset
    juce::int64 stutterGateEndAbs = -1;   // v3.1.0: the stutter gate spans blocks
    std::vector<float> dryGateMask;       // per-sample dry gate (1 = pass) for the spatial post loop

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainScatterProcessor)
};
