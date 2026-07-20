#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class AutoClipAudioProcessor : public juce::AudioProcessor
{
public:
    AutoClipAudioProcessor();
    ~AutoClipAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AutoClip"; }
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

    // Public APVTS for editor binding
    juce::AudioProcessorValueTreeState parameters;

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (Phase 4.1: Core Processing)
    juce::dsp::ProcessSpec spec;
    juce::dsp::DelayLine<float> lookaheadDelayL { 48000 };  // Max 1 second at 48kHz
    juce::dsp::DelayLine<float> lookaheadDelayR { 48000 };
    int lookaheadSamples = 0;

    // Phase 4.2: Automatic Gain Matching
    juce::SmoothedValue<float> smoothedGain;
    float inputPeak = 0.0f;
    float outputPeak = 0.0f;

    // Phase 4.3: Clip Solo (Delta Monitoring)
    juce::AudioBuffer<float> originalBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoClipAudioProcessor)
};
