/*
  ==============================================================================

    O-MicrotonalSampler - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): silent shell. APVTS + headless TuningEngine + NE drain
    + sample-map shared_ptr surface + SampleLoader skeleton. First audio: Phase 2.1.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <memory>
#include "MicrotonalSamplerSound.h"
#include "MicrotonalSamplerVoice.h"
#include "SampleMap.h"
#include "SampleLoader.h"
#include "TuningEngine.h"          // global namespace (D-4)
#include "NoteExpression.h"        // modules/tuning/note-expression (via ouaricon_add_module)

class OMicrotonalSamplerAudioProcessor : public juce::AudioProcessor
{
public:
    OMicrotonalSamplerAudioProcessor();
    ~OMicrotonalSamplerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-MicrotonalSampler"; }
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

    // VST3 Note Expression (kTuningTypeID) - Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // Phase 2.2: drag-drop folder load entry point (called from PluginEditor).
    // Spawns the background SampleLoader; on completion (message thread) the
    // new SampleMap is atomic-stored into currentSampleMap and any skipped
    // files are recorded in lastSkippedFiles for Stage-3 UI surfacing.
    void loadSampleFolder (const juce::File& folder);

    // Read-only accessor for Stage-3 UI: list of files the loader skipped
    // (unparseable filenames, unreadable files, etc.). Refreshed on each
    // loadSampleFolder completion; cleared on failure.
    const juce::StringArray& getLastSkippedFiles() const noexcept { return lastSkippedFiles; }

private:
    juce::AudioProcessorValueTreeState        parameters;
    juce::Synthesiser                         synthesiser;
    TuningEngine                              tuningEngine;       // D-4: global namespace
    Ouaricon::NoteExpression::VST3Extensions  vst3Extensions;

    // Sample-map storage (atomic-swap target — Stage 2.2 background loader writes here)
    std::shared_ptr<SampleMap>                currentSampleMap;

    // Background sample loader (owns juce::Thread)
    std::unique_ptr<SampleLoader>             sampleLoader;

    // Output gain smoothing (RESEARCH R7, pitfall #8 — 10 ms ramp prevents
    // zipper noise on parameter changes). Initialized in prepareToPlay.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;

    // Phase 2.2: list of filenames the most recent loader pass could not
    // parse / read. Populated on completion callback (message thread); read
    // by Stage-3 UI. Cleared on failure.
    juce::StringArray lastSkippedFiles;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OMicrotonalSamplerAudioProcessor)
};
