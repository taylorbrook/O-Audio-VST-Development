/*
  ==============================================================================

    O-Wind - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FluteSynthSound.h"
#include "FluteSynthVoice.h"
#include "DSP/StereoWidth.h"
#include "DSP/InstrumentPresets.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "OuariconPresetManager.h"

class OWindAudioProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    OWindAudioProcessor();
    ~OWindAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Wind"; }
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

    // Public access to APVTS for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Public access to tuning engine
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    // Public access to preset manager for editor
    OuariconPresetManager& getPresetManager() { return presetManager; }

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synthesiser;

    // Tuning engine (processor-level, shared by all voices)
    TuningEngine tuningEngine;

    // Post-voice stereo width processor
    StereoWidthProcessor stereoWidth;

    // Post-width headjoint formant resonance filter (stereo)
    juce::dsp::IIR::Filter<float> formantFilterL, formantFilterR;
    float lastFormantGainDb = 0.0f;
    float lastFormantCenterHz = 2500.0f;
    int lastFormantPresetIndex = -1;

    // Preset manager (OuariconPresetManager)
    OuariconPresetManager presetManager;

    // APVTS parameter change callback
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Factory preset initialization
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OWindAudioProcessor)
};
