/*
  ==============================================================================

    O-SimpleReverb - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    v1.5.0 - Renamed from OuariconSimpleReverb

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"

class OSimpleReverbAudioProcessor : public juce::AudioProcessor
{
public:
    // ==================== DSP Constants ====================
    // These named constants replace magic numbers for clarity and tuneability

    // Delay line sizing (samples at 192kHz)
    static constexpr int kMaxPreDelaySamples = 19200;       // ~100ms at 192kHz
    static constexpr int kMaxEarlyReflectionSamples = 9600; // ~50ms at 192kHz
    static constexpr int kMaxAllPassSamples = 960;          // ~5ms at 192kHz

    // Stereo offsets for spatial separation
    static constexpr float kEarlyReflectionStereoOffset = 1.07f;  // 7% L/R offset
    static constexpr float kAllPassStereoOffset = 1.05f;          // 5% L/R offset

    // Modulation amounts
    static constexpr float kLfoAmplitudeModulation = 0.03f;  // ±3% amplitude modulation
    static constexpr float kShimmerMixAmount = 0.08f;        // 8% shimmer mix
    static constexpr float kDefaultShimmerFreq = 1500.0f;    // ~1.5kHz ring modulation

    // Early reflection base times (ms) - prime numbers for natural sound
    static constexpr std::array<float, 4> kBaseEarlyDelaysMs = { 7.0f, 11.0f, 17.0f, 23.0f };

    // VU meter floor level
    static constexpr float kVuMeterFloorDB = -100.0f;


    OSimpleReverbAudioProcessor();
    ~OSimpleReverbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-SimpleReverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;

    // VU Meter - output level for WebView (thread-safe)
    std::atomic<float> outputLevelDB { kVuMeterFloorDB };

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components
    juce::dsp::ProcessSpec spec;
    juce::dsp::Reverb reverb;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> characterFilter;

    // Character filter mode tracking
    enum class CharacterMode { Warm, Neutral, Bright };
    CharacterMode previousMode = CharacterMode::Neutral;

    // === Type-Specific DSP Components (v1.1.0) ===

    // Pre-delay line
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL { kMaxPreDelaySamples };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR { kMaxPreDelaySamples };

    // Early reflection comb filters (4 per channel for density)
    static constexpr int numEarlyReflections = 4;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numEarlyReflections> earlyReflectionsL;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numEarlyReflections> earlyReflectionsR;
    std::array<float, numEarlyReflections> earlyReflectionGains = { 0.7f, 0.5f, 0.35f, 0.25f };

    // All-pass diffusers for Spring dispersion (creates metallic chirp)
    static constexpr int numAllPassFilters = 3;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numAllPassFilters> allPassL;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numAllPassFilters> allPassR;
    std::array<float, numAllPassFilters> allPassDelayMs = { 1.5f, 2.3f, 3.7f };  // Prime-ish ratios
    float allPassCoeff = 0.6f;  // Feedback coefficient
    std::array<float, numAllPassFilters> allPassStateL = { 0.0f, 0.0f, 0.0f };
    std::array<float, numAllPassFilters> allPassStateR = { 0.0f, 0.0f, 0.0f };

    // Modulation LFO for Spring flutter and Ambient movement
    float lfoPhase = 0.0f;
    float lfoIncrement = 0.0f;

    // Type-specific EQ filters
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> typeEqFilter;

    // Lowpass filter for wet signal (user-controlled, 20-400Hz)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lpFilter;
    float previousLPFreq = -1.0f;

    // Shimmer for Plate (octave-up pitch shift approximation via ring modulation)
    float shimmerPhase = 0.0f;
    float shimmerFreq = 0.0f;

    // State tracking
    int previousType = -1;
    double currentSampleRate = 44100.0;

    // Type preset structure (expanded)
    struct TypePreset {
        float baseRoomSize;
        float baseDamping;
        float width;
        float preDelayMs;           // Pre-delay in milliseconds
        float earlyReflectionScale; // Scale factor for early reflection times
        float earlyReflectionMix;   // How much early reflections to mix in
        float modRate;              // LFO rate in Hz (0 = no modulation)
        float modDepth;             // Modulation depth in ms
        bool useAllPass;            // Enable all-pass dispersion (Spring)
        bool useShimmer;            // Enable shimmer effect (Plate)
        float eqFreq;               // Type-specific EQ frequency
        float eqGain;               // Type-specific EQ gain (dB)
        float eqQ;                  // Type-specific EQ Q
        enum class EqType { None, LowShelf, HighShelf, Peak, HighPass } eqType;
    };

    static const TypePreset typePresets[6];

    // Helper methods
    void updateTypeSpecificDSP(int typeIndex);
    float processAllPassChain(float input, bool isLeft);
    void initializeFactoryPresets();

    // Template helpers to reduce initialization code duplication
    template<typename DelayContainer>
    void prepareDelayContainer(DelayContainer& delays, const juce::dsp::ProcessSpec& spec) {
        for (auto& delay : delays) {
            delay.prepare(spec);
            delay.reset();
        }
    }

    template<typename FilterType>
    void prepareFilterAsAllPass(FilterType& filter, const juce::dsp::ProcessSpec& spec, double sampleRate) {
        filter.prepare(spec);
        filter.reset();
        *filter.state = *juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, 1000.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSimpleReverbAudioProcessor)
};
