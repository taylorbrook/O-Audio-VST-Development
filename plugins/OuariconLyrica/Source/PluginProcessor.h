/*
  ==============================================================================

    OuariconLyrica - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "HarpSynthSound.h"
#include "HarpSynthVoice.h"
#include "DSP/SympatheticResonance.h"

class OuariconLyricaAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconLyricaAudioProcessor();
    ~OuariconLyricaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OuariconLyrica"; }
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

    /**
     * Get pointer to sympathetic resonance engine (for voice access)
     */
    SympatheticResonanceEngine* getSympatheticEngine() { return &sympatheticEngine; }

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synthesiser;

    // Phase 2.7: Sympathetic Resonance Engine (processor-level, shared by all voices)
    SympatheticResonanceEngine sympatheticEngine;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconLyricaAudioProcessor)
};
