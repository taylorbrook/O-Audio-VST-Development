/*
  ==============================================================================

    OuariconComp - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class OuariconCompAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconCompAudioProcessor();
    ~OuariconCompAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OuariconComp"; }
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

    juce::AudioProcessorValueTreeState parameters;

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (BEFORE APVTS for initialization order)
    juce::dsp::ProcessSpec spec;

    // Compressor state
    float envelopeDB = -60.0f;  // Current envelope level in dB
    float attackCoeff = 0.0f;   // Attack time coefficient
    float releaseCoeff = 0.0f;  // Release time coefficient

    // Metering (atomic for thread-safe access from UI)
    std::atomic<float> inputLevelDB { -60.0f };
    std::atomic<float> outputLevelDB { -60.0f };
    std::atomic<float> currentGainReductionDB { 0.0f };
    std::atomic<float> currentEnvelopeDB { -60.0f };

public:
    // Meter getters for UI
    float getInputLevelDB() const { return inputLevelDB.load(); }
    float getOutputLevelDB() const { return outputLevelDB.load(); }
    float getGainReductionDB() const { return currentGainReductionDB.load(); }
    float getEnvelopeDB() const { return currentEnvelopeDB.load(); }

private:

    // DSP Helper Methods
    float calculateGainReduction(float inputLevel, float thresholdDB,
                                  float ratio, float kneeDB);
    void updateCoefficients(float attackTimeMs, float releaseTimeMs, double sampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconCompAudioProcessor)
};
