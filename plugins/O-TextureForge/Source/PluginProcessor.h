/*
  ==============================================================================

    O-TextureForge - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "dsp/SharedCorpus.h"
#include "dsp/CorpusLoader.h"
#include "dsp/GrainScheduler.h"
#include <memory>
#include <atomic>

struct VizSnapshot
{
    struct ActiveGrain
    {
        uint32_t grainIndex;
        float envelope;
        float readPositionNorm;
    };

    int activeCount = 0;
    std::array<ActiveGrain, 64> activeGrains;
    float cursorX = 0.5f;
    float cursorY = 0.5f;
};

class TextureForgeProcessor : public juce::AudioProcessor
{
public:
    TextureForgeProcessor();
    ~TextureForgeProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-TextureForge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    const VizSnapshot& getVizSnapshot() const
    {
        return vizSnapshots[static_cast<size_t>(1 - vizWriteIndex.load(std::memory_order_acquire))];
    }

    // Corpus loading
    void loadCorpusFile(const juce::File& file);

    // UI interaction
    std::shared_ptr<SharedCorpus> getCurrentCorpus() const { return currentCorpus; }
    void selectGrainFromUI(int grainIndex);

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached parameter pointers (real-time safe)
    std::atomic<float>* energyParam = nullptr;
    std::atomic<float>* brightnessParam = nullptr;
    std::atomic<float>* textureParam = nullptr;
    std::atomic<float>* positionParam = nullptr;
    std::atomic<float>* grainDensityParam = nullptr;
    std::atomic<float>* grainSizeParam = nullptr;
    std::atomic<float>* scatterXParam = nullptr;
    std::atomic<float>* scatterYParam = nullptr;
    std::atomic<float>* variationParam = nullptr;
    std::atomic<float>* crossfadeParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* midiModeParam = nullptr;

    // Visualization double-buffer (lock-free audio->GUI)
    std::array<VizSnapshot, 2> vizSnapshots {};
    std::atomic<int> vizWriteIndex { 0 };

    double currentSampleRate = 44100.0;

    // DSP components
    GrainScheduler grainScheduler;
    CorpusLoader corpusLoader;

    // Thread-safe corpus sharing
    std::shared_ptr<SharedCorpus> currentCorpus;  // Message thread (owns lifetime)
    std::atomic<SharedCorpus*> corpusForAudio { nullptr };  // Audio thread (read-only)

    // UI-selected grain for triggering
    std::atomic<int> pendingGrainSelect { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureForgeProcessor)
};
