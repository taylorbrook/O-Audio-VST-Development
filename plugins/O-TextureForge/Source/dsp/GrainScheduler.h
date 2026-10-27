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
#include "GrainVoice.h"
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
    void processDroneMode(int numSamples, const SchedulerParams& params, const SharedCorpus* corpus);
    void triggerGrain(const SharedCorpus* corpus, const SchedulerParams& params, int midiNote, int midiChannel, float velocity);
    void triggerGrainByIndex(const SharedCorpus* corpus, const SchedulerParams& params,
                             uint32_t grainIndex, int midiNote, int midiChannel, float velocity);

public:
    // Trigger a specific grain by index (from UI scatter click)
    void triggerSpecificGrain(const SharedCorpus* corpus, const SchedulerParams& params, uint32_t grainIndex);

private:
    uint32_t queryGrainIndex(
        const SharedCorpus* corpus,
        const SchedulerParams& params,
        juce::Random& rng) const;

    void renderVoices(juce::AudioBuffer<float>& buffer, const SharedCorpus* corpus);

    float sampleFromCorpus(const SharedCorpus* corpus, float readPosition) const;

    GrainPool grainPool;
    juce::Random random;
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

    // Drone mode state
    int droneTimerSamples = 0;
    int droneIntervalSamples = 1000;

    // MIDI CC state for Trigger+Modulate mode (Mode 1)
    float ccEnergy = 0.5f;         // CC1 (mod wheel) → energy bias
    float ccBrightness = 0.5f;     // Channel pressure → brightness bias

    // Drone mode CC overrides (Mode 2), -1 = use APVTS value
    float droneCCEnergy = -1.0f;
    float droneCCBrightness = -1.0f;
    float droneCCTexture = -1.0f;
};
