/*
  ==============================================================================

    OuariconSimpleReverb - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    v1.1.0 - Type-specific DSP for meaningful reverb differentiation

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class OuariconSimpleReverbAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconSimpleReverbAudioProcessor();
    ~OuariconSimpleReverbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Simple Reverb"; }
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

    // Pre-delay line (max 100ms at 192kHz = 19200 samples)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL { 19200 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR { 19200 };

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSimpleReverbAudioProcessor)
};
