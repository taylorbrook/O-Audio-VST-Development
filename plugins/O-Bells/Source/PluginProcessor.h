/*
  ==============================================================================

    O-Bells - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "BellSound.h"
#include "BellVoice.h"
#include "OuariconPresetManager.h"

// Reverb spec for spacious bell sound
struct BellReverbSpec
{
    static constexpr float roomSize = 0.7f;      // Large room
    static constexpr float damping = 0.4f;       // Moderate high-freq damping
    static constexpr float width = 1.0f;         // Full stereo width
    static constexpr float freezeMode = 0.0f;    // No freeze
};

class OBellsAudioProcessor : public juce::AudioProcessor
{
public:
    OBellsAudioProcessor();
    ~OBellsAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Bells"; }
    bool acceptsMidi() const override { return true; }  // Synthesizer - MIDI input
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

    // Public access to preset manager
    OuariconPresetManager& getPresetManager() { return presetManager; }

    // Output level metering (peak values, 0.0 to 1.0)
    std::atomic<float> outputLevelLeft { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };

private:
    // DSP Components (BEFORE parameters for initialization order)
    juce::Synthesiser synthesiser;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    // Cached parameter pointers (atomic reads, real-time safe)
    // Main Panel (8 params in v1.2.0)
    std::atomic<float>* strikePositionParam = nullptr;
    std::atomic<float>* malletHardnessParam = nullptr;
    std::atomic<float>* dampingParam = nullptr;
    std::atomic<float>* overtoneBrightnessParam = nullptr;  // v2.0.0: renamed from brightnessParam
    std::atomic<float>* acousticBrightnessParam = nullptr;  // v2.0.0: new - controls HF decay rate
    std::atomic<float>* airAbsorptionParam = nullptr;       // v2.1.0: time-varying lowpass filter
    std::atomic<float>* airAbsorptionTimeParam = nullptr;  // v2.2.0: independent time control
    std::atomic<float>* materialParam = nullptr;
    std::atomic<float>* inharmonicityParam = nullptr;
    std::atomic<float>* bloomSpeedParam = nullptr;   // v1.4.0: Split bloom
    std::atomic<float>* bloomAmountParam = nullptr;
    // v1.5.0: Bloom fine controls (per-band)
    std::atomic<float>* bloomFineEnabledParam = nullptr;
    std::atomic<float>* bloomSpeedLowParam = nullptr;
    std::atomic<float>* bloomSpeedMidParam = nullptr;
    std::atomic<float>* bloomSpeedHighParam = nullptr;
    std::atomic<float>* bloomAmountLowParam = nullptr;
    std::atomic<float>* bloomAmountMidParam = nullptr;
    std::atomic<float>* bloomAmountHighParam = nullptr;
    std::atomic<float>* shimmerParam = nullptr;
    // Ensemble (5 params)
    std::atomic<float>* unisonCountParam = nullptr;
    std::atomic<float>* unisonDetuneParam = nullptr;
    std::atomic<float>* octaveBlendSubParam = nullptr;
    std::atomic<float>* octaveBlendOctParam = nullptr;
    std::atomic<float>* stereoSpreadParam = nullptr;
    // Advanced (8 params in v1.3.0)
    std::atomic<float>* partialTuningParam = nullptr;
    std::atomic<float>* nonlinearEffectsParam = nullptr;
    std::atomic<float>* strikeNoiseCharParam = nullptr;
    std::atomic<float>* attackLevelParam = nullptr;
    // decayShapeParam removed in v1.2.0 - always multi-stage
    std::atomic<float>* velocityCurveParam = nullptr;
    std::atomic<float>* pitchEnvelopeParam = nullptr;
    std::atomic<float>* pitchEnvTimeParam = nullptr;
    // Multi-stage envelope (4 params, always active in v1.2.0)
    std::atomic<float>* strikeTimeParam = nullptr;
    std::atomic<float>* brillianceParam = nullptr;
    std::atomic<float>* bodyTimeParam = nullptr;
    std::atomic<float>* humSustainParam = nullptr;
    // Output
    std::atomic<float>* reverbMixParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;

    // APVTS (AFTER DSP components)
    juce::AudioProcessorValueTreeState parameters;

    // Preset manager (AFTER APVTS)
    OuariconPresetManager presetManager;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Factory preset initialization (called once on first run)
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBellsAudioProcessor)
};
