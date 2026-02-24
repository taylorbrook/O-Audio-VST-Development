/*
  ==============================================================================

    PluginProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "PrismSound.h"
#include "PrismVoice.h"
#include "dsp/WavetableData.h"
#include "dsp/WavetableGenerator.h"
#include "dsp/WavetableFactory.h"
#include "dsp/DistortionProcessor.h"
#include "dsp/DelayProcessor.h"
#include "dsp/ReverbProcessor.h"
#include "dsp/EQProcessor.h"

class OPrismAudioProcessor : public juce::AudioProcessor
{
public:
    OPrismAudioProcessor();
    ~OPrismAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Prism"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }
    TuningEngine* getTuningEngine() { return &tuningEngine; }
    ScaleGenerator* getScaleGenerator() { return &scaleGenerator; }
    TuningExporter* getTuningExporter() { return &tuningExporter; }

    const WavetableData* getFactoryTable (int index) const
    {
        if (index >= 0 && index < static_cast<int> (factoryTables.size()))
            return factoryTables[static_cast<size_t> (index)].get();
        return nullptr;
    }

    int getNumFactoryTables() const { return static_cast<int> (factoryTables.size()); }

    juce::String getTableName (int index) const
    {
        if (index >= 0 && index < static_cast<int> (tableInfoList.size()))
            return tableInfoList[static_cast<size_t> (index)].name;
        return {};
    }

    juce::String getTableCategory (int index) const
    {
        if (index >= 0 && index < static_cast<int> (tableInfoList.size()))
            return tableInfoList[static_cast<size_t> (index)].category;
        return {};
    }

    /** Get current mod wheel value (0..1) for modulation matrix */
    float getModWheelValue() const { return modWheelValue.load (std::memory_order_relaxed); }

    /** Get current aftertouch value (0..1) for modulation matrix */
    float getAftertouchValue() const { return aftertouchValue.load (std::memory_order_relaxed); }

    /** Get currently active MIDI notes and their microtonal frequencies */
    std::vector<std::pair<int, double>> getActiveNotes()
    {
        std::vector<std::pair<int, double>> result;
        for (int i = 0; i < 128; ++i)
        {
            if (noteStates[static_cast<size_t> (i)].load (std::memory_order_relaxed))
                result.push_back ({ i, tuningEngine.getFrequency (i) });
        }
        return result;
    }

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synthesiser;
    TuningEngine tuningEngine;
    ScaleGenerator scaleGenerator;
    TuningExporter tuningExporter;

    // Factory wavetable library (28 tables across 5 categories)
    std::vector<std::unique_ptr<WavetableData>> factoryTables;
    std::vector<TableInfo> tableInfoList;
    int lastOscATable = -1;
    int lastOscBTable = -1;
    int lastTuningPreset = -1;
    int lastTonic = -1;

    // Effects chain (float precision)
    DistortionProcessor distortion;
    juce::dsp::Chorus<float> chorus;
    DelayProcessor delay;
    EQProcessor eq;
    ReverbProcessor reverbProcessor;

    // Master volume (smoothed)
    juce::SmoothedValue<float> masterVolSmoothed { 0.8f };

    // Active MIDI note tracking for TrueKeys visualization
    struct ActiveNote { int midiNote; double frequency; };
    std::array<std::atomic<bool>, 128> noteStates {};
    mutable std::mutex activeNotesMutex;

    // MIDI CC state for modulation matrix (global sources)
    std::atomic<float> modWheelValue { 0.0f };
    std::atomic<float> aftertouchValue { 0.0f };

    void updateWavetableAssignments();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OPrismAudioProcessor)
};
