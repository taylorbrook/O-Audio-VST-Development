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
#include "TuningEngine.h"

class BowedContrabassVoice;     // Phase 2.3 R29 forward decl — used by getActiveVoice()

class OContrabassAudioProcessor : public juce::AudioProcessor,
                                  public juce::AudioProcessorValueTreeState::Listener,
                                  public juce::AsyncUpdater
{
public:
    OContrabassAudioProcessor();
    ~OContrabassAudioProcessor() override;

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

    // Phase 2.6b R40a — TuningEngine accessor + Scala/TUN file-load entry
    // point (ESCALATION-FPK1: harness-only at v1.0; Stage 3 GUI replaces with
    // FileChooser).
    TuningEngine* getTuningEngine() noexcept { return &tuningEngine; }
    bool loadScalaFile (const juce::File& sclFile);

    // Phase 2.6b R40a — APVTS Listener override (TUNING_SYSTEM Choice → mode
    // dispatch). CR-03: parameterChanged may fire on the audio thread under host
    // automation, so it only stores the choice atomically + triggerAsyncUpdate()
    // (RT-safe, reuses a preallocated message). The mutex-holding setMode runs on
    // the message thread in handleAsyncUpdate(). Replaces the prior audio-thread
    // MessageManager::callAsync (heap alloc + this-capturing teardown UAF).
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // CR-03 / WR-01 — message-thread apply of the staged tuning mode.
    void handleAsyncUpdate() override;

    // NOTE: Do NOT declare getLatencySamples() here — it is non-virtual in JUCE 8.
    // Use setLatencySamples(N) inside prepareToPlay() instead.

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Phase 2.6b R40a — TuningEngine member declared BEFORE synth so C++ ctor
    // init order = declaration order (Risk #32 mitigation). synth.addVoice
    // passes &tuningEngine to BowedContrabassVoice; pointer must be valid at
    // construction time.
    TuningEngine tuningEngine;

    // CR-03 — choice index staged by parameterChanged (any thread) for the
    // message-thread setMode apply in handleAsyncUpdate(). Default 2 = "12-TET".
    std::atomic<int> pendingTuningChoice { 2 };

    // WR-10: 4-voice polyphony (one per EADG string) — enables double-stop drones.
    static constexpr int kNumVoices = 4;
    OContrabassMPESynthesiser synth;

    // Phase 2.6a — master output chain (post-voice-summation): Saturator →
    // Limiter → StereoWidth → OUTPUT_GAIN. OUTPUT_GAIN relocated from voice
    // per ARCHITECTURE §258 (user volume should not affect saturator color).
    MasterSaturator              masterSaturator;
    MasterLimiter                masterLimiter;
    StereoWidth                  stereoWidth;
    juce::SmoothedValue<float>   outputGainSmoothed { 1.0f };

    // WR-02: cache the master-chain parameter atomics (resolved once in the
    // constructor) so processBlock does not re-walk the APVTS std::map 4× per block.
    std::atomic<float>* satAmountParam      = nullptr;
    std::atomic<float>* limiterCeilingParam = nullptr;
    std::atomic<float>* widthParam          = nullptr;
    std::atomic<float>* outputGainParam     = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessor)
};
