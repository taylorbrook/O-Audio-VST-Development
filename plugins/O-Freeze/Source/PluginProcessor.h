/*
  ==============================================================================

    O-Freeze - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class OFreezeAudioProcessor : public juce::AudioProcessor
{
public:
    OFreezeAudioProcessor();
    ~OFreezeAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Freeze"; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

private:
    // Gate state machine
    enum class GateState { Idle, Frozen };

    // Grain structure for granular synthesis
    struct Grain
    {
        int startSample = 0;      // Position within grain (0 to grainSize)
        int position = 0;         // Position in freeze buffer
        bool active = false;      // Grain active flag
    };

    // DSP Components (declared BEFORE parameters for initialization order)
    juce::AudioBuffer<float> freezeBuffer;
    int writePosition = 0;
    int readPosition = 0;
    bool bufferFrozen = false;
    double currentSampleRate = 44100.0;
    juce::LinearSmoothedValue<float> freezeGain;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Threshold gate components
    GateState gateState = GateState::Idle;
    float rmsLevel = 0.0f;
    std::vector<float> rmsBuffer;
    int rmsWriteIndex = 0;
    int rmsSamplesPerWindow = 0;

    // Granular synthesis components
    static constexpr int NUM_GRAINS = 8;  // 8 grains for 87.5% overlap (COLA compliant)
    std::array<Grain, NUM_GRAINS> grains;
    std::vector<float> hannWindow;
    int grainSize = 0;
    int grainTriggerInterval = 0;
    int grainTriggerCounter = 0;
    int nextGrainIndex = 0;
    juce::Random random;
    bool stopTriggeringNewGrains = false;  // Soft release: let active grains complete

    // Smoothed drift (prevents clicking from discontinuous grain positions)
    float currentDriftOffset = 0.0f;   // Current smoothed offset (0 to 1)
    float targetDriftOffset = 0.0f;    // Target offset to drift toward
    int driftUpdateCounter = 0;        // Counter for picking new targets
    int driftUpdateInterval = 0;       // How often to pick new target (samples)

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreezeAudioProcessor)
};
