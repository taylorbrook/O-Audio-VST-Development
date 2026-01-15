/*
  ==============================================================================

    Ouaricon Polystutter - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "DSP/RepeatLane.h"

class OuariconPolystutterAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconPolystutterAudioProcessor();
    ~OuariconPolystutterAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Polystutter"; }
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

    // Public access to APVTS for UI binding
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

private:
    // DSP Components (BEFORE parameters for initialization order)
    juce::dsp::ProcessSpec spec;

    // Single lane for Phase 2.1
    std::unique_ptr<class RepeatLane> lane1;

    // Beat sync state
    double lastPPQPosition = 0.0;
    double currentBPM = 120.0;
    bool wasPlaying = false;
    int samplesSinceLastBeat = 0;
    double subdivisionSamples = 0.0;

    // Mix state
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> wetBuffer;
    int maxBlockSize = 0;  // Track max block size for buffer safety

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

    // Cached parameter pointers (avoid string lookups in processBlock)
    std::atomic<float>* lane1EnabledParam = nullptr;
    std::atomic<float>* lane1SubdivParam = nullptr;
    std::atomic<float>* lane1RepeatsParam = nullptr;
    std::atomic<float>* lane1DecayParam = nullptr;
    std::atomic<float>* lane1VolumeParam = nullptr;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Helper functions
    void updateBeatSync(const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo);
    double getSubdivisionSamples(int subdivIndex, double bpm, double sampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconPolystutterAudioProcessor)
};
