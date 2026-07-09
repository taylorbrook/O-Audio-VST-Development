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
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

class OBowedAudioProcessor : public juce::AudioProcessor
{
public:
    OBowedAudioProcessor();
    ~OBowedAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

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

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback (Phase 24).
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

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

    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

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

    // WR-06: APVTS atomic pointers resolved once in prepareToPlay so processBlock reads
    // parameters without per-callback string-keyed map lookups on the audio thread.
    std::atomic<float>* pSympatheticAmount = nullptr;
    std::atomic<float>* pSympatheticCount  = nullptr;
    std::atomic<float>* pBodyMaterial      = nullptr;
    std::atomic<float>* pBodySize          = nullptr;
    std::atomic<float>* pWidth             = nullptr;
    std::atomic<float>* pReferencePitch    = nullptr;
    std::atomic<float>* pSympatheticDecay  = nullptr;
    std::atomic<float>* pBodyAmount        = nullptr;
    std::atomic<float>* pTuningSystem      = nullptr;
    std::atomic<float>* pOutputLevel       = nullptr;
    std::atomic<float>* pHumanizeRange[4]  = { nullptr, nullptr, nullptr, nullptr };
    std::atomic<float>* pHumanizeRate[4]   = { nullptr, nullptr, nullptr, nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBowedAudioProcessor)
};
