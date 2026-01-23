/*
  ==============================================================================

    OBass - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Bass enhancement plugin with crossover filtering and harmonic generation.

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class OBassAudioProcessor : public juce::AudioProcessor
{
public:
    OBassAudioProcessor();
    ~OBassAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OBass"; }
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

    // DSP spec for processor configuration
    juce::dsp::ProcessSpec spec;

    //==========================================================================
    // Phase 1 Placeholders (to be implemented in subsequent plans)
    //==========================================================================
    // Crossover filter (Plan 01-02)
    // Mono summer (Plan 01-03)
    // Intermediate buffers for low/high split (Plan 01-02/03)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassAudioProcessor)
};
