#pragma once

#include <JuceHeader.h>

#include "dsp/CaptureBuffer.h"
#include "dsp/GrainScheduler.h"
#include "dsp/ReverseGrain.h"
#include "dsp/WindowLut.h"

// O-ReverseDelay — granular reverse delay (Stage 2 DSP, Phase 2.3 complete:
// reverse wet path + damped tanh-stable feedback loop + tempo sync + width).
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

    // Width-spread RNG (RT-safe xorshift32 — never juce::Random::getSystemRandom
    // on the audio thread) + alternating pan sign so consecutive grains ping
    // left/right rather than clumping. Bias amount is a harness-tuned constant
    // (probe K, D5), frozen once green.
    static constexpr float kPanBias = 0.5f;
    juce::uint32 rngState = 0x12345678u;
    float        panSign  = 1.0f;

    float nextRand01() noexcept
    {
        juce::uint32 x = rngState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        rngState = x;
        return static_cast<float>(x >> 8) * (1.0f / 16777216.0f);   // [0, 1)
    }

    // Cached APVTS atomics — read once per block on the audio thread.
    std::atomic<float>* pDelayTime    = nullptr;
    std::atomic<float>* pSyncMode     = nullptr;
    std::atomic<float>* pNoteDivision = nullptr;
    std::atomic<float>* pGrainSize    = nullptr;
    std::atomic<float>* pDensity      = nullptr;
    std::atomic<float>* pFeedback     = nullptr;
    std::atomic<float>* pLowCut       = nullptr;
    std::atomic<float>* pHighCut      = nullptr;
    std::atomic<float>* pWidth        = nullptr;
    std::atomic<float>* pMix          = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseDelayProcessor)
};
