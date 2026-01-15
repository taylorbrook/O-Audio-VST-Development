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
#include "DSP/TriggerRouter.h"
#include "DSP/TapeDegrader.h"

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

    // Four independent lanes for Phase 2.2
    std::unique_ptr<class RepeatLane> lane1;
    std::unique_ptr<class RepeatLane> lane2;
    std::unique_ptr<class RepeatLane> lane3;
    std::unique_ptr<class RepeatLane> lane4;

    // Phase 2.3: Trigger router for advanced trigger modes
    std::unique_ptr<class TriggerRouter> triggerRouter;

    // Phase 2.4: Tape degradation post-processor
    std::unique_ptr<class TapeDegrader> tapeDegrader;

    // Beat sync state
    double lastPPQPosition = 0.0;
    double currentBPM = 120.0;
    bool wasPlaying = false;
    int samplesSinceLastBeat = 0;
    double subdivisionSamples = 0.0;

    // Phase 2.3: Manual trigger edge detection
    bool previousManualTrigger = false;

    // Mix state (per-lane buffers for parallel processing)
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> lane1Buffer;
    juce::AudioBuffer<float> lane2Buffer;
    juce::AudioBuffer<float> lane3Buffer;
    juce::AudioBuffer<float> lane4Buffer;
    int maxBlockSize = 0;  // Track max block size for buffer safety

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

    // Cached parameter pointers for lane 1 (avoid string lookups in processBlock)
    std::atomic<float>* lane1EnabledParam = nullptr;
    std::atomic<float>* lane1SubdivParam = nullptr;
    std::atomic<float>* lane1RepeatsParam = nullptr;
    std::atomic<float>* lane1DecayParam = nullptr;
    std::atomic<float>* lane1VolumeParam = nullptr;
    std::atomic<float>* lane1PanParam = nullptr;
    std::atomic<float>* lane1ProbabilityParam = nullptr;
    std::atomic<float>* lane1SwingParam = nullptr;

    // Cached parameter pointers for lane 2
    std::atomic<float>* lane2EnabledParam = nullptr;
    std::atomic<float>* lane2SubdivParam = nullptr;
    std::atomic<float>* lane2RepeatsParam = nullptr;
    std::atomic<float>* lane2DecayParam = nullptr;
    std::atomic<float>* lane2VolumeParam = nullptr;
    std::atomic<float>* lane2PanParam = nullptr;
    std::atomic<float>* lane2ProbabilityParam = nullptr;
    std::atomic<float>* lane2SwingParam = nullptr;

    // Cached parameter pointers for lane 3
    std::atomic<float>* lane3EnabledParam = nullptr;
    std::atomic<float>* lane3SubdivParam = nullptr;
    std::atomic<float>* lane3RepeatsParam = nullptr;
    std::atomic<float>* lane3DecayParam = nullptr;
    std::atomic<float>* lane3VolumeParam = nullptr;
    std::atomic<float>* lane3PanParam = nullptr;
    std::atomic<float>* lane3ProbabilityParam = nullptr;
    std::atomic<float>* lane3SwingParam = nullptr;

    // Cached parameter pointers for lane 4
    std::atomic<float>* lane4EnabledParam = nullptr;
    std::atomic<float>* lane4SubdivParam = nullptr;
    std::atomic<float>* lane4RepeatsParam = nullptr;
    std::atomic<float>* lane4DecayParam = nullptr;
    std::atomic<float>* lane4VolumeParam = nullptr;
    std::atomic<float>* lane4PanParam = nullptr;
    std::atomic<float>* lane4ProbabilityParam = nullptr;
    std::atomic<float>* lane4SwingParam = nullptr;

    // Pattern step pointers (cached for performance)
    std::atomic<float>* lane1PatternSteps[16];
    std::atomic<float>* lane2PatternSteps[16];
    std::atomic<float>* lane3PatternSteps[16];
    std::atomic<float>* lane4PatternSteps[16];

    // Phase 2.3: Cached parameter pointers for trigger modes
    std::atomic<float>* envelopeEnabledParam = nullptr;
    std::atomic<float>* sidechainEnabledParam = nullptr;
    std::atomic<float>* midiEnabledParam = nullptr;
    std::atomic<float>* manualTriggerParam = nullptr;

    // Phase 2.3: Cached parameter pointers for lane advanced modes
    std::atomic<float>* lane1PingPongParam = nullptr;
    std::atomic<float>* lane1ReverseParam = nullptr;
    std::atomic<float>* lane1FreezeParam = nullptr;
    std::atomic<float>* lane1ManualTimeParam = nullptr;

    std::atomic<float>* lane2PingPongParam = nullptr;
    std::atomic<float>* lane2ReverseParam = nullptr;
    std::atomic<float>* lane2FreezeParam = nullptr;
    std::atomic<float>* lane2ManualTimeParam = nullptr;

    std::atomic<float>* lane3PingPongParam = nullptr;
    std::atomic<float>* lane3ReverseParam = nullptr;
    std::atomic<float>* lane3FreezeParam = nullptr;
    std::atomic<float>* lane3ManualTimeParam = nullptr;

    std::atomic<float>* lane4PingPongParam = nullptr;
    std::atomic<float>* lane4ReverseParam = nullptr;
    std::atomic<float>* lane4FreezeParam = nullptr;
    std::atomic<float>* lane4ManualTimeParam = nullptr;

    // Phase 2.4: Cached parameter pointers for pitch
    std::atomic<float>* lane1PitchParam = nullptr;
    std::atomic<float>* lane2PitchParam = nullptr;
    std::atomic<float>* lane3PitchParam = nullptr;
    std::atomic<float>* lane4PitchParam = nullptr;

    // Phase 2.4: Cached parameter pointers for tape degradation
    std::atomic<float>* tapeSaturationParam = nullptr;
    std::atomic<float>* tapeWowParam = nullptr;
    std::atomic<float>* tapeFlutterParam = nullptr;
    std::atomic<float>* tapeHissParam = nullptr;
    std::atomic<float>* tapeRolloffParam = nullptr;
    std::atomic<float>* tapeDropoutParam = nullptr;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Helper functions
    void updateBeatSync(const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo);
    double getSubdivisionSamples(int subdivIndex, double bpm, double sampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconPolystutterAudioProcessor)
};
