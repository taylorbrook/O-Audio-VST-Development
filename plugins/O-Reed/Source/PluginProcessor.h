/*
  ==============================================================================

    O-Reed - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "ReedWindVoice.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "OuariconPresetManager.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

class OReedAudioProcessor : public juce::AudioProcessor
{
public:
    OReedAudioProcessor();
    ~OReedAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Reed"; }
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

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Public access to APVTS for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Public access to tuning engine
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    // Public access to preset manager
    OuariconPresetManager& getPresetManager() { return presetManager; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

private:
    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;
    juce::MPESynthesiser synthesiser;

    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

    // Tuning engine (processor-level, shared by all voices)
    TuningEngine tuningEngine;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Factory preset initialization
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OReedAudioProcessor)
};
