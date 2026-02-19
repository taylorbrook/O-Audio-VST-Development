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

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synthesiser;
    TuningEngine tuningEngine;
    ScaleGenerator scaleGenerator;
    TuningExporter tuningExporter;

    // Factory wavetables (Saw, Square, Triangle, Sine)
    std::vector<std::unique_ptr<WavetableData>> factoryTables;
    int lastOscATable = -1;
    int lastOscBTable = -1;

    // Effects chain (float precision)
    DistortionProcessor distortion;
    juce::dsp::Chorus<float> chorus;
    DelayProcessor delay;
    EQProcessor eq;
    ReverbProcessor reverbProcessor;

    // Master volume (smoothed)
    juce::SmoothedValue<float> masterVolSmoothed { 0.8f };

    void updateWavetableAssignments();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OPrismAudioProcessor)
};
