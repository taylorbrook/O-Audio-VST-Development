/*
  ==============================================================================

    O-Bowed - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "BowedMPESynthesiser.h"
#include "BowedStringVoice.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "DSP/BodyResonator.h"
#include "DSP/StereoWidthProcessor.h"
#include "DSP/SympatheticStringEngine.h"
#include "DSP/HumanizeEngine.h"
#include "OuariconPresetManager.h"

class OBowedAudioProcessor : public juce::AudioProcessor
{
public:
    OBowedAudioProcessor();
    ~OBowedAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Bowed"; }
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

    // Public access to humanize engine (voices read per-block offsets)
    const HumanizeEngine* getHumanizeEngine() const noexcept { return &humanizeEngine; }

    // Public access to preset manager for editor
    OuariconPresetManager& getPresetManager() { return presetManager; }

    // Check if any synthesiser voice is active (for visualization)
    bool isAnyVoiceActive() const
    {
        for (int i = 0; i < synthesiser.getNumVoices(); ++i)
            if (synthesiser.getVoice(i)->isActive())
                return true;
        return false;
    }

private:
    juce::AudioProcessorValueTreeState parameters;
    BowedMPESynthesiser synthesiser;

    // Tuning engine (processor-level, shared by all voices)
    TuningEngine tuningEngine;

    // Body resonator and stereo width (processor-level, post-voice processing)
    BodyResonator bodyResonator;
    StereoWidthProcessor stereoWidthProcessor;

    // Sympathetic string engine (processor-level)
    SympatheticStringEngine sympatheticEngine;

    // Random-walk humanization, shared across voices (processor-level)
    HumanizeEngine humanizeEngine;

    // Preset manager
    OuariconPresetManager presetManager;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Factory preset initialization
    void initializeFactoryPresets();

    // DC blocker state (per-channel first-order highpass)
    float dcBlockX[2] = { 0.0f, 0.0f };
    float dcBlockY[2] = { 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBowedAudioProcessor)
};
