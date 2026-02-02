/*
  ==============================================================================

    O-Detune - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook
    Version: 1.1.0

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class ODetuneAudioProcessor : public juce::AudioProcessor
{
public:
    ODetuneAudioProcessor();
    ~ODetuneAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Detune"; }
    bool acceptsMidi() const override { return false; }
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

    // Latency reporting - 50ms delay line (2400 samples @ 48kHz)
    int getLatencySamples() const { return latencySamples; }

    // APVTS (public for editor access)
    juce::AudioProcessorValueTreeState parameters;

private:
    //==============================================================================
    // DSP Components

    juce::dsp::ProcessSpec spec;

    // Wobble Engine (delay-based pitch modulation)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wobbleDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wobbleDelayR;

    // Unison Engine (up to 7 voices)
    static constexpr int maxUnisonVoices = 7;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> unisonDelaysL[maxUnisonVoices];
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> unisonDelaysR[maxUnisonVoices];

    // Pre-delay for advanced section
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR;

    // Focus Filter (frequency-selective processing)
    juce::dsp::IIR::Filter<float> focusHighPassL;
    juce::dsp::IIR::Filter<float> focusHighPassR;
    juce::dsp::IIR::Filter<float> focusLowPassL;
    juce::dsp::IIR::Filter<float> focusLowPassR;

    // Era filters (for 60s/80s character)
    juce::dsp::IIR::Filter<float> eraFilterL;
    juce::dsp::IIR::Filter<float> eraFilterR;

    // Color filters
    juce::dsp::IIR::Filter<float> colorFilterL;
    juce::dsp::IIR::Filter<float> colorFilterR;

    // Cached filter coefficients (avoid allocation in processBlock)
    float lastFocusLow = 20.0f;
    float lastFocusHigh = 20000.0f;
    int lastEra = 1;  // 0=60s, 1=70s, 2=80s
    float lastColor = 0.0f;

    // Dry/Wet Mixer
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Processing buffers (pre-allocated for real-time safety)
    juce::AudioBuffer<float> wobbleBuffer;
    juce::AudioBuffer<float> unisonBuffer;
    juce::AudioBuffer<float> feedbackBuffer;

    // State variables
    double currentSampleRate = 48000.0;
    int latencySamples = 2400; // 50ms @ 48kHz
    static constexpr float centerDelayMs = 50.0f;

    // LFO state
    float lfoPhase = 0.0f;

    // Random LFO state (for true randomness)
    float randomCurrentValue = 0.0f;
    float randomTargetValue = 0.0f;
    int randomHoldCounter = 0;
    int randomHoldSamples = 0;
    juce::Random randomGenerator;

    // Per-voice random offsets for unison randomization
    float voiceRandomOffsets[maxUnisonVoices] = {0.0f};

    // Feedback state
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    //==============================================================================
    // Helper functions
    float generateLFOSample(int shape, float rate);
    void applyEraCharacter(int era, float& rate, float& depth);
    float applySaturation(float input, float driveAmount);

    //==============================================================================
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ODetuneAudioProcessor)
};
