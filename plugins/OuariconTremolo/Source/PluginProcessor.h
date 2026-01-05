#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class OuariconTremoloAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconTremoloAudioProcessor();
    ~OuariconTremoloAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Tremolo"; }
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

    // Public access to parameters for editor
    juce::AudioProcessorValueTreeState parameters;

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (declared BEFORE parameters for initialization order)
    // LFO state
    float lfoPhase = 0.0f;
    float lfoPhaseIncrement = 0.0f;
    double currentSampleRate = 44100.0;

    // Smoothing filter state (separate for L/R when Pan Sync enabled)
    float smoothedLFO_L = 0.0f;
    float smoothedLFO_R = 0.0f;

    // Random number generator for noise waveform
    juce::Random random;

    // Helper methods
    float generateWaveform(float phase, int waveformType);
    float applySmoothingFilter(float rawLFO, float& prevSmoothed, float coefficient);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconTremoloAudioProcessor)
};
