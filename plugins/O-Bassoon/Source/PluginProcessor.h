/*
  ==============================================================================

    O-Bassoon - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): silent shell. APVTS + headless TuningEngine + NE drain.
    First audio: Phase 2.1.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "BassoonSound.h"
#include "BassoonVoice.h"
#include "BassoonSynthesiser.h"    // Phase 2.4: voice manager subclass with active-cap (FUNC-02)
#include "OuariconPresetManager.h" // Stage 4: factory preset persistence
#include "TuningEngine.h"          // global namespace (D2)
#include "NoteExpression.h"        // modules/tuning/note-expression (via ouaricon_add_module)

class OBassoonAudioProcessor : public juce::AudioProcessor
{
public:
    OBassoonAudioProcessor();
    ~OBassoonAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Bassoon"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }  // Stage 1: silent stub

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Public access to APVTS for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Public access to tuning engine (forward-compat for Phase 2.1+)
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    // Stage 4: preset manager (forward-compat for future preset-browser UI).
    OuariconPresetManager& getPresetManager() { return presetManager; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // Stage 3 / Phase 3.2 — push-channel atomics (audio-thread writer, message-thread reader).
    // Editor's 30 Hz Timer reads with std::memory_order_relaxed and emits to JS via
    // emitEventIfBrowserIsVisible (diff-suppressed). Single producer / single consumer.
    std::atomic<int>   currentActiveVoiceCount { 0 };
    std::atomic<float> currentEffectiveBreath  { 0.0f };
    std::atomic<float> currentVibratoEnvelope  { 0.0f };

private:
    juce::AudioProcessorValueTreeState        parameters;
    OuariconPresetManager                     presetManager;    // Stage 4: declared AFTER parameters (member-init order)
    BassoonSynthesiser                        synthesiser;      // Phase 2.4: subclass for voice cap
    TuningEngine                              tuningEngine;     // D2: global namespace
    Ouaricon::NoteExpression::VST3Extensions  vst3Extensions;

    // Phase 2.4 (FUNC-02): voice_count dispatch shadow.
    // Sentinel -1 forces first dispatch on next processBlock (any valid count ∈ [1, 16]
    // differs from -1, integer comparison — no float epsilon needed for AudioParameterInt).
    int lastDispatchedVoiceCount = -1;

    // Phase 2.2: tone smoother + dispatch throttle (CONTEXT-rev-2 Q3/Q4-rev-2).
    // Sentinel -1.0f forces first dispatch on next processBlock (any valid
    // tone ∈ [0, 1] differs from -1 by > 0.001).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoother;
    float                                                          lastDispatchedTone = -1.0f;

    // Phase 2.3: output_gain post-summation smoother (30 ms Linear)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother { 1.0f };

    // Phase 2.3: expression dispatch shadows (processor scope; separate from per-voice shadows)
    float lastDispatchedAttackMs   = -1.0f;
    float lastDispatchedReleaseMs  = -1.0f;
    float lastDispatchedVibRate    = -1.0f;
    float lastDispatchedVibDepth   = -1.0f;
    float lastDispatchedVibOnsetMs = -1.0f;
    float lastDispatchedUiBreath   = -1.0f;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Stage 4: factory preset initialization (idempotent — only writes if Factory dir empty).
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassoonAudioProcessor)
};
