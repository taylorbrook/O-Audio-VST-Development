#pragma once

#include <JuceHeader.h>
#include "DSP/MotionEngine.h"
#include "DSP/VBAPRenderer.h"
#include "DSP/DistanceModel.h"
#include "DSP/VBAPDataExchange.h"
#include "DSP/DownmixEngine.h"
#include "Data/SpeakerLayout.h"
#include "Data/SpeakerPresets.h"

class OOrbitProcessor : public juce::AudioProcessor,
                        public juce::AsyncUpdater
{
public:
    OOrbitProcessor();
    ~OOrbitProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void handleAsyncUpdate() override;

    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    static float shortestArc (float from, float to);
    static float wrapAngle (float angle);

    // Cached parameter pointers
    std::atomic<float>* pathParam        = nullptr;
    std::atomic<float>* speedParam       = nullptr;
    std::atomic<float>* widthParam       = nullptr;
    std::atomic<float>* depthParam       = nullptr;
    std::atomic<float>* tiltParam        = nullptr;
    std::atomic<float>* phaseParam       = nullptr;
    std::atomic<float>* elevEnableParam  = nullptr;
    std::atomic<float>* elevRangeParam   = nullptr;
    std::atomic<float>* tempoSyncParam   = nullptr;

    std::atomic<float>* speakerLayoutParam = nullptr;
    std::atomic<float>* distanceParam      = nullptr;
    std::atomic<float>* airAbsorptionParam = nullptr;
    std::atomic<float>* attenCurveParam    = nullptr;
    std::atomic<float>* centerDivergeParam = nullptr;

    std::atomic<float>* sourceModeParam  = nullptr;
    std::atomic<float>* lrOffsetParam    = nullptr;
    std::atomic<float>* mixParam         = nullptr;

    // DSP components
    MotionEngine  motionEngine;
    VBAPRenderer  vbapRenderer;
    DistanceModel distanceModel;
    DistanceModel distanceModelR;  // For L+R Split mode, R channel

    // Thread-safe VBAP gain table exchange
    VBAPDataExchange vbapExchange;
    VBAPComputeThread vbapThread { vbapExchange };

    // Auto-downmix engine
    DownmixEngine downmixEngine;

    // Buffers
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> spatialBuffer;  // Multi-channel intermediate

    // Per-speaker gain smoothing (max 24 speakers)
    std::array<float, 24> previousGains {};
    std::array<float, 24> currentGains {};
    std::array<float, 24> previousGainsR {};  // For L+R Split R source
    std::array<float, 24> currentGainsR {};

    // Layout change detection
    int lastSpeakerLayoutIndex = -1;
    SpeakerLayout currentLayout;
    int layoutNumSpeakers = 2;

    // Smoothed values
    juce::SmoothedValue<float> speedSmoothed;
    juce::SmoothedValue<float> widthSmoothed;
    juce::SmoothedValue<float> depthSmoothed;
    juce::SmoothedValue<float> tiltSmoothed;
    juce::SmoothedValue<float> mixSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOrbitProcessor)
};
