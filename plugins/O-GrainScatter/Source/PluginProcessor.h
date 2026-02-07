#pragma once

#include <JuceHeader.h>
#include "dsp/DelayBuffer.h"
#include "dsp/GrainPool.h"
#include "dsp/GrainScheduler.h"
#include "dsp/TempoTracker.h"
#include "dsp/ScaleQuantizer.h"
#include "dsp/EuclideanGenerator.h"
#include "dsp/FreezeManager.h"

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
    };
    std::array<Voice, 64> voices {};
    int activeCount = 0;
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

    // Visualization getters (lock-free, called from GUI thread)
    const GrainVizSnapshot& getVizSnapshot() const
    {
        return vizSnapshots[static_cast<size_t> (1 - vizWriteIndex.load (std::memory_order_acquire))];
    }

    const std::array<bool, 16>& getEuclideanPattern() const { return euclideanPattern; }
    int getEuclideanStep() const { return currentEuclideanStep.load (std::memory_order_relaxed); }
    int getEuclideanSteps() const { return cachedEuclideanSteps; }

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
    std::atomic<float>* stutterGateParam = nullptr;

    // DSP components
    DelayBuffer delayBuffer;
    GrainPool grainPool;
    GrainScheduler scheduler;
    TempoTracker tempoTracker;
    ScaleQuantizer scaleQuantizer;
    FreezeManager freezeManager;

    // SmoothedValues
    juce::SmoothedValue<float> dryWetSmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;

    // Euclidean pattern cache
    std::array<bool, 16> euclideanPattern {};
    int cachedEuclideanSteps = 0;
    int cachedEuclideanPulses = 0;

    // Visualization double-buffer (lock-free audio→GUI)
    std::array<GrainVizSnapshot, 2> vizSnapshots {};
    std::atomic<int> vizWriteIndex { 0 };
    std::atomic<int> currentEuclideanStep { 0 };

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
