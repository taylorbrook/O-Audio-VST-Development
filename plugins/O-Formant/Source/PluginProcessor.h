/*
  ==============================================================================

    PluginProcessor.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "dsp/GlottalWavetable.h"
#include "dsp/LyricsEngine.h"
#include "DSP/DelayProcessor.h"
#include "DSP/EQProcessor.h"
#include "DSP/ReverbProcessor.h"
#include "OuariconPresetManager.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "TuningExporter.h"
#include "EmbeddedTunings.h"

class OFormantAudioProcessor : public juce::AudioProcessor
{
public:
    OFormantAudioProcessor();
    ~OFormantAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Formant"; }
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

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }
    OuariconPresetManager& getPresetManager() { return presetManager; }

    TuningEngine tuningEngine;
    ScaleGenerator scaleGenerator;
    TuningExporter tuningExporter;
    LyricsEngine lyricsEngine;

    LyricsEngine& getLyricsEngine() { return lyricsEngine; }

private:
    // DSP: Shared wavetable (generated once, read-only across all voices)
    GlottalWavetable glottalWavetable;
    double lastSampleRate = 0.0;

    // Post-synth output gain (dB -> linear, smoothed)
    juce::SmoothedValue<float> outputGainSmoothed { 1.0f };

    // Effects chain (Chorus -> Delay -> Reverb -> EQ)
    juce::dsp::Chorus<float> chorus;
    DelayProcessor delayProcessor;
    ReverbProcessor reverbProcessor;
    EQProcessor eqProcessor;

    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;
    juce::MPESynthesiser synthesiser;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OFormantAudioProcessor)
};
