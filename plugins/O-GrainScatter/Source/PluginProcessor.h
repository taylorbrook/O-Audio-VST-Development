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
#include "dsp/ScaleQuantizer.h"
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
    std::atomic<float>* dryWetParam = nullptr;
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
    ScaleQuantizer scaleQuantizer;
    FreezeManager freezeManager;
    BinauralDecoder binauralDecoder;

    // HOA3 internal accumulation bus (16 channels)
    juce::AudioBuffer<float> hoaBus;

    // Heap-allocated binaural decode buffers (sized in prepareToPlay)
    std::vector<float> binauralL, binauralR;

    // Distance LPF (1-pole per channel for air absorption approximation)
    float distanceLpfState[2] = { 0.0f, 0.0f };

    // SmoothedValues
    juce::SmoothedValue<float> dryWetSmoothed;
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

    // Scale/pitch mode change detection
    int cachedScaleIndex = -1;
    int cachedPitchMode = -1;

    // Feedback state
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    // Spawn request buffer (pre-allocated)
    std::vector<SpawnRequest> spawnRequests;

    // RNG for grain parameters
    juce::Random grainRng;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainScatterProcessor)
};
