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
    float outputGainDb = 0.0f;
    int midiMode = 2;  // 0=Pitch-Mapped, 1=Trigger+Modulate, 2=Generative Drone
};

class GrainScheduler
{
public:
    GrainScheduler();

    void prepare(double sampleRate);
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
    void triggerGrain(const SharedCorpus* corpus, const SchedulerParams& params, int midiNote, float velocity);
    void triggerGrainByIndex(const SharedCorpus* corpus, const SchedulerParams& params,
                             uint32_t grainIndex, int midiNote, float velocity);

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

    // Drone mode state
    int droneTimerSamples = 0;
    int droneIntervalSamples = 1000;

    // MIDI CC state for Trigger+Modulate mode
    float ccScatterX = 0.5f;
    float ccScatterY = 0.5f;
    float aftertouchValue = 0.0f;
};
