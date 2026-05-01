/*
  ==============================================================================

    O-Contrabass — Audio Processor
    Specialized 4-string contrabass physical model
    (Stage 1: Foundation — APVTS shell, no DSP yet)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "OContrabassMPESynthesiser.h"
#include "DSP/MasterSaturator.h"
#include "DSP/MasterLimiter.h"
#include "DSP/StereoWidth.h"

class BowedContrabassVoice;     // Phase 2.3 R29 forward decl — used by getActiveVoice()

class OContrabassAudioProcessor : public juce::AudioProcessor
{
public:
    OContrabassAudioProcessor();
    ~OContrabassAudioProcessor() override = default;

    //==============================================================================
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                            { return true; }

    const juce::String getName() const override                { return JucePlugin_Name; }
    bool acceptsMidi() const override                          { return true; }
    bool producesMidi() const override                         { return false; }
    bool isMidiEffect() const override                         { return false; }
    double getTailLengthSeconds() const override               { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram(int) override                       {}
    const juce::String getProgramName(int) override            { return {}; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    //==============================================================================
    // APVTS — public so editor (and Stage 2 voice) can attach.
    juce::AudioProcessorValueTreeState parameters;

    // Phase 2.3 R29 — accessor for harness clampedDepthMean instrumentation read.
    // Returns the first (currently only) bowed voice; nullptr if synth is empty.
    // Defined in PluginProcessor.cpp where BowedContrabassVoice is fully visible.
    BowedContrabassVoice* getActiveVoice() noexcept;

    // NOTE: Do NOT declare getLatencySamples() here — it is non-virtual in JUCE 8.
    // Use setLatencySamples(N) inside prepareToPlay() instead.

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Phase 2.1a: single E1 voice. Multi-voice / per-string voicing is Phase 2.2.
    OContrabassMPESynthesiser synth;

    // Phase 2.6a — master output chain (post-voice-summation): Saturator →
    // Limiter → StereoWidth → OUTPUT_GAIN. OUTPUT_GAIN relocated from voice
    // per ARCHITECTURE §258 (user volume should not affect saturator color).
    MasterSaturator              masterSaturator;
    MasterLimiter                masterLimiter;
    StereoWidth                  stereoWidth;
    juce::SmoothedValue<float>   outputGainSmoothed { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessor)
};
