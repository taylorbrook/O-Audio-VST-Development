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

    // UI motion snapshot (written by audio thread, read by UI timer)
    std::atomic<float> uiAzimuthL  { 0.0f };
    std::atomic<float> uiElevationL { 0.0f };
    std::atomic<float> uiAzimuthR  { 0.0f };
    std::atomic<float> uiElevationR { 0.0f };
    std::atomic<float> uiDistance   { 1.0f };

    float getUIAzimuthL() const   { return uiAzimuthL.load (std::memory_order_relaxed); }
    float getUIElevationL() const { return uiElevationL.load (std::memory_order_relaxed); }
    float getUIAzimuthR() const   { return uiAzimuthR.load (std::memory_order_relaxed); }
    float getUIElevationR() const { return uiElevationR.load (std::memory_order_relaxed); }
    float getUIDistance() const   { return uiDistance.load (std::memory_order_relaxed); }

    const SpeakerLayout& getCurrentLayout() const { return currentLayout; }
    const DownmixEngine& getDownmixEngine() const { return downmixEngine; }

    // Speaker layout modification (called from message thread via native functions)
    void setCustomSpeakerLayout (const SpeakerLayout& layout);
    void addSpeakerToLayout (float azimuth, float elevation, float distance, const juce::String& label);
    void removeSpeakerFromLayout (int index);
    void moveSpeakerInLayout (int index, float azimuth, float elevation);
    bool isUsingCustomLayout() const { return useCustomLayout; }

    struct FactoryPreset
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values;
    };

    static const std::vector<FactoryPreset>& getFactoryPresets();

private:
    int currentProgramIndex = 0;
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

    // Custom layout support
    bool useCustomLayout = false;
    SpeakerLayout customLayout;
    void applyLayout (const SpeakerLayout& layout);
    void applyLayoutOnAudioThread (const SpeakerLayout& layout);

    // Thread-safe pending layout (message thread writes, audio thread reads)
    juce::SpinLock pendingLayoutLock;
    SpeakerLayout pendingLayout;
    std::atomic<bool> layoutPending { false };

    // Smoothed values
    juce::SmoothedValue<float> speedSmoothed;
    juce::SmoothedValue<float> widthSmoothed;
    juce::SmoothedValue<float> depthSmoothed;
    juce::SmoothedValue<float> tiltSmoothed;
    juce::SmoothedValue<float> mixSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OOrbitProcessor)
};
