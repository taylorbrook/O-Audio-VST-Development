#pragma once

#include <JuceHeader.h>

#include "dsp/CaptureBuffer.h"
#include "dsp/GrainScheduler.h"
#include "dsp/ReverseGrain.h"
#include "dsp/WindowLut.h"

// O-ReverseDelay — granular reverse delay (Stage 2 DSP, Phase 2.2: core reverse
// wet path + damped tanh-stable feedback loop; tempo sync + width land in 2.3).
// APVTS with 10 parameters per research/ARCHITECTURE.md (immutable contract).
// NOTE: this file (and PluginProcessor.cpp) must stay free of editor-only includes —
// the render harness compiles the processor without any editor sources.
class ReverseDelayProcessor : public juce::AudioProcessor
{
public:
    ReverseDelayProcessor();
    ~ReverseDelayProcessor() override;

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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // DSP components (Stage 2). All allocation confined to prepareToPlay().
    CaptureBuffer  capture;             // 3.5 s stereo ring, input + feedback return
    WindowLut      hannLut { 2048 };    // built once at construction, never on audio thread
    GrainPool      grainPool;           // 32 preallocated reverse-grain slots
    GrainScheduler scheduler;           // free-countdown spawn scheduler

    std::array<SpawnRequest, GrainScheduler::kMaxSpawnsPerBlock> spawnRequests {};

    juce::AudioBuffer<float> wetScratch;   // wet accumulation — never aliases the I/O buffer
    juce::AudioBuffer<float> fbScratch;    // feedback return: wet → fbGain → HP → LP → tanh → guard

    // In-loop damping filters (2nd-order Butterworth): lowCut = HP, highCut = LP.
    // Coefficient updates use ArrayCoefficients assigned IN PLACE into the
    // existing Coefficients objects (*filter.coefficients = array) — never
    // Coefficients::makeXXX on the audio thread (heap-allocates), never memcpy
    // raw 6-arrays over the 5-value normalised storage.
    juce::dsp::IIR::Filter<float> hpL, hpR, lpL, lpR;

    // Cached-cutoff guards — gate ONLY the coefficient recompute; no
    // enabled/bypass flag exists (O-MultiBandCompressor v1.6.0 lesson).
    float lastLowCut  = -1.0f;
    float lastHighCut = -1.0f;

    // Smoothed (~20 ms): feedback, mix, lowCut, highCut.
    // NEVER smoothed (latched per grain): delayTime/D, grainSize, density, width.
    juce::SmoothedValue<float> feedbackSmoothed, mixSmoothed, lowCutSmoothed, highCutSmoothed;

    double currentSampleRate = 44100.0;

    // Cached APVTS atomics — read once per block on the audio thread.
    std::atomic<float>* pDelayTime    = nullptr;
    std::atomic<float>* pSyncMode     = nullptr;   // consumed in Phase 2.3
    std::atomic<float>* pNoteDivision = nullptr;   // consumed in Phase 2.3
    std::atomic<float>* pGrainSize    = nullptr;
    std::atomic<float>* pDensity      = nullptr;
    std::atomic<float>* pFeedback     = nullptr;   // consumed in Phase 2.2
    std::atomic<float>* pLowCut       = nullptr;   // consumed in Phase 2.2
    std::atomic<float>* pHighCut      = nullptr;   // consumed in Phase 2.2
    std::atomic<float>* pWidth        = nullptr;   // consumed in Phase 2.3
    std::atomic<float>* pMix          = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseDelayProcessor)
};
