/*
  ==============================================================================

    O-Detune - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"

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

    // Preset management
    OuariconPresetManager presetManager;

private:
    //==============================================================================
    // DSP Components (Phase 4.1: Core Processing)

    // Processing spec
    juce::dsp::ProcessSpec spec;

    // Wobble Engine (delay-based pitch modulation)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wobbleDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wobbleDelayR;

    // LFO for wobble modulation (Phase 4.1: Simple sine only)
    juce::dsp::Oscillator<float> wobbleLFO;

    // Unison Engine (3 voices for Phase 4.1)
    static constexpr int maxUnisonVoices = 7;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> unisonDelaysL[maxUnisonVoices];
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> unisonDelaysR[maxUnisonVoices];

    // Focus Filter (frequency-selective processing)
    juce::dsp::IIR::Filter<float> focusHighPassL;
    juce::dsp::IIR::Filter<float> focusHighPassR;
    juce::dsp::IIR::Filter<float> focusLowPassL;
    juce::dsp::IIR::Filter<float> focusLowPassR;

    // Dry/Wet Mixer
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Processing buffers (pre-allocated for real-time safety)
    juce::AudioBuffer<float> wobbleBuffer;
    juce::AudioBuffer<float> unisonBuffer;

    // State variables
    double currentSampleRate = 48000.0;
    int latencySamples = 2400; // 50ms @ 48kHz
    static constexpr float centerDelayMs = 50.0f;

    // LFO state (for multi-waveform support)
    float lfoPhase = 0.0f;
    float noiseHeldValue = 0.0f;
    int noiseLastQuarter = -1;
    juce::Random random;

    // Voice randomization (refresh periodically)
    float voiceRandomOffsets[maxUnisonVoices] = {0};
    int randomRefreshCounter = 0;

    // Per-voice LFO phases for chorus-style unison modulation
    float voiceLfoPhases[maxUnisonVoices] = {0};

    // Pre-delay lines
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> preDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> preDelayR;
    float feedbackStateL = 0.0f;
    float feedbackStateR = 0.0f;

    // Parameter smoothing (50ms ramp time)
    juce::SmoothedValue<float> smoothedBlend;
    juce::SmoothedValue<float> smoothedWobbleRate;
    juce::SmoothedValue<float> smoothedWobbleDepth;
    juce::SmoothedValue<float> smoothedUnisonDetune;
    juce::SmoothedValue<float> smoothedWidth;
    juce::SmoothedValue<float> smoothedDelay;
    juce::SmoothedValue<float> smoothedFeedback;
    juce::SmoothedValue<float> smoothedUnisonSpread;
    juce::SmoothedValue<float> smoothedRandomAmt;

    //==============================================================================
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP helper functions
    float generateLFO(float phase, int shapeType, float& noiseHeld, int& lastQuarter, juce::Random& rng);
    void processWidth(float& left, float& right, float widthPercent);
    void processMonoSafe(float& left, float& right);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ODetuneAudioProcessor)
};
