/*
  ==============================================================================

    O-IntonationPad - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "DSP/ChordGenerator.h"
#include "DSP/TuningSystem.h"

class OIntonationPadAudioProcessor : public juce::AudioProcessor
{
public:
    OIntonationPadAudioProcessor();
    ~OIntonationPadAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-IntonationPad"; }
    bool acceptsMidi() const override { return true; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

private:
    // DSP Components (declare BEFORE parameters for initialization order)
    juce::Synthesiser synthesiser;
    ChordGenerator chordGenerator;
    TuningSystem tuningSystem;

    // Global LFO
    double lfoPhase = 0.0;
    double lfoPhaseIncrement = 0.0;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::ProcessSpec filterSpec;

    // Randomization
    juce::Random randomGenerator;

    // Parameters (APVTS comes after DSP components)
    juce::AudioProcessorValueTreeState parameters;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OIntonationPadAudioProcessor)
};
