/*
  ==============================================================================

    Ouaricon Marimba - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TuningEngine.h"
#include "BodyResonance.h"

class MicroMarimbaAudioProcessor : public juce::AudioProcessor
{
public:
    MicroMarimbaAudioProcessor();
    ~MicroMarimbaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Marimba"; }
    bool acceptsMidi() const override { return true; }  // Synth accepts MIDI
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

    // Phase 2.3: Public access to tuning engine for UI
    TuningEngine& getTuningEngine() { return tuningEngine; }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (Phase 2.1: Basic synthesizer)
    juce::Synthesiser synthesiser;

    // Phase 2.3: Tuning engine
    TuningEngine tuningEngine;

    // Phase 2.4: Body resonance (convolution)
    BodyResonance bodyResonance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MicroMarimbaAudioProcessor)
};
