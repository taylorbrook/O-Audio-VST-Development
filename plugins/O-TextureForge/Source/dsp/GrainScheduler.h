/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

    GrainScheduler.h
    Real-time grain scheduling and playback engine

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SharedCorpus.h"
#include "SearchIndex.h"
#include "GrainVoice.h"
#include "CounterRng.h"
#include <array>
#include <cstdint>

struct SchedulerParams
{
    float energy = 0.5f;
    float brightness = 0.5f;
    float texture = 0.5f;
    float position = 0.0f;
    int grainDensity = 8;
    float grainSizeMs = 50.0f;
    float scatterX = 0.5f;
    float scatterY = 0.5f;
    float variation = 0.2f;
    float crossfadePercent = 50.0f;
    float outputGainDb = -6.0f;   // v1.5.1: matches the OUTPUT_GAIN default
    int midiMode = 2;  // 0=Pitch-Mapped, 1=Trigger+Modulate, 2=Generative Drone
    uint64_t rngSeed = 0;  // v1.6.0: grain-selection seed, persisted in plugin state

    // v1.8.0 (research §3.5). All three at their defaults = the v1.7.0 search.
    // searchIndex: a tree built with steered descriptor weights, used only while
    // it belongs to the corpus being played; nullptr = the corpus's own tree.
    const SearchIndex* searchIndex = nullptr;
    float pickSpread = 0.0f;    // 0..1: 0 = nearest candidate; > 0 = softmax(-D^2/tau) over a cached k = 16
    float repeatAvoid = 0.0f;   // 0..1: penalty on the last kRecentLen picks
};

class GrainScheduler
{
public:
    GrainScheduler();

    // initialOutputGainDb seeds the output-gain smoother (v1.5.1).
    void prepare(double sampleRate, float initialOutputGainDb = -6.0f);
    void reset();

    void processBlock(
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midiMessages,
        const SharedCorpus* corpus,
        const SchedulerParams& params);

    // Get active grain info for visualization
    struct ActiveGrainInfo
    {
        uint32_t grainIndex;
        float envelope;
        float readPositionNorm;
    };

    void getActiveGrains(ActiveGrainInfo* outGrains, int& outCount) const;

private:
    void handleMidiMessage(const juce::MidiMessage& msg, const SchedulerParams& params, const SharedCorpus* corpus);
    void triggerGrain(const SharedCorpus* corpus, const SchedulerParams& params, int midiNote, int midiChannel, float velocity);
    void triggerGrainByIndex(const SharedCorpus* corpus, const SchedulerParams& params,
                             uint32_t grainIndex, int midiNote, int midiChannel, float velocity);

public:
    // Trigger a specific grain by index (from UI scatter click)
    void triggerSpecificGrain(const SharedCorpus* corpus, const SchedulerParams& params, uint32_t grainIndex);

private:
    // v1.8.0: non-const — it refreshes the candidate cache and reads the ring.
    uint32_t queryGrainIndex(
        const SharedCorpus* corpus,
        const SchedulerParams& params,
        uint64_t grainCounter);

    // v1.8.0: candidate cache (research §3.5: "cache k = 8-16 candidates, then
    // pick by softmax(-D^2/tau)"). Holds the kCacheK nearest grains to the
    // UNJITTERED macro target and their tree-space coordinates. It is refreshed
    // only when its key changes (tree, corpus or index generation, or the target),
    // never per grain at a steady target, and never because a block ended: the
    // picks are a function of the grain sequence alone, so they do not depend on
    // the host's block size. Per grain, the Variation jitter moves the query and
    // the kCacheK distances are recomputed exactly (16 x 19 MACs). Preallocated.
    static constexpr int kLegacyK = 8;
    static constexpr int kCacheK = 16;
    struct CandidateCache
    {
        bool valid = false;
        const KDTreeSearch* tree = nullptr;
        uint64_t corpusGeneration = 0;
        uint64_t indexGeneration = 0;
        std::array<float, 3> target {};       // ENERGY, BRIGHTNESS, TEXTURE query coords
        int count = 0;
        std::array<uint32_t, kCacheK> index {};
        std::array<std::array<float, DESCRIPTOR_DIMS>, kCacheK> point {};
    };
    CandidateCache cache;

    // v1.8.0: recently-used ring — the last kRecentLen search picks, newest at
    // recentHead - 1. Repeat Avoid adds a penalty that fades with age. Cleared on
    // reset() and when the corpus changes. Fixed size, no allocation.
    static constexpr int kRecentLen = 8;
    std::array<uint32_t, kRecentLen> recent {};
    int recentCount = 0;
    int recentHead = 0;
    const SharedCorpus* recentCorpus = nullptr;
    uint64_t recentCorpusGeneration = 0;

    int recentAge(uint32_t grainIndex) const noexcept;   // 0 = newest, -1 = not in the ring
    void pushRecent(uint32_t grainIndex) noexcept;
    void applyRepeatPenalty(const uint32_t* indices, float* scores, int n, float avoid) const noexcept;

    // v1.6.0: renders [startSample, endSample) so spawns land on their own sample.
    void renderVoices(juce::AudioBuffer<float>& buffer, const SharedCorpus* corpus,
                      int startSample, int endSample);

    // v1.6.1: absolute position in double. A float position quantised the
    // interpolation fraction to 1/16 sample ~12 s into a 44.1 kHz corpus, lost it
    // past 2^23 (~3.2 min) and skipped samples past 2^24, so pitched grains got
    // grittier the deeper into the file they started.
    float sampleFromCorpus(const SharedCorpus* corpus, double readPosition) const;

    GrainPool grainPool;

    // v1.6.0: counter-based RNG (was a time-seeded juce::Random). Each randomised
    // grain draws from hash(seed, grainCounter, slot); the counter restarts on
    // reset() and when the seed changes, so a seeded render reproduces at any
    // block size.
    uint64_t rngSeed = 0;
    uint64_t grainCounter = 0;
    double currentSampleRate = 44100.0;

    // v1.5.1: output gain is ramped (Linear, ~20 ms, on the LINEAR gain) instead
    // of jumping per block — it is the last gain stage; only the processor's
    // safety clip follows it.
    juce::SmoothedValue<float> outputGainSmoothed { 1.0f };

    // v1.5.4: drone-mode density normalisation 1/√max(1, N·μ₂/μ₂_Hann), N =
    // grain size / spawn interval capped at the pool. Ramped (~100 ms) so a
    // Density move doesn't step the level; exactly 1 outside drone mode and for
    // sparse drones, where the multiply is skipped (bit-identical to v1.5.3).
    juce::SmoothedValue<float> densityNormSmoothed { 1.0f };
    float normCrossfadeCache = -1.0f;
    float normPowerRatio = 1.0f;
    float computeDensityNorm(const SchedulerParams& params);
    static float envelopeMeanSquare(float crossfadeFactor);

    // Drone mode state. v1.6.0: droneTimerSamples is the time since the last
    // spawn, measured at the current block's start; a grain spawns on the sample
    // where it reaches droneIntervalSamples (was: the next block start).
    int droneTimerSamples = 0;
    bool droneStartPending = true;   // v1.6.0: first grain after reset() on sample 0
    int droneIntervalSamples = 1000;

    // MIDI CC state for Trigger+Modulate mode (Mode 1)
    float ccEnergy = 0.5f;         // CC1 (mod wheel) → energy bias
    float ccBrightness = 0.5f;     // Channel pressure → brightness bias

    // Drone mode CC overrides (Mode 2), -1 = use APVTS value
    float droneCCEnergy = -1.0f;
    float droneCCBrightness = -1.0f;
    float droneCCTexture = -1.0f;
};
