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
#include "TuningEngine.h"
#include "DSP/DelayProcessor.h"
#include "DSP/EQProcessor.h"
#include "DSP/ReverbProcessor.h"
#include "ScaleGenerator.h"
#include "TuningExporter.h"
#include "EmbeddedTunings.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

class OBellsAudioProcessor : public juce::AudioProcessor,
                              public juce::AudioProcessorValueTreeState::Listener
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

    // v2.2.0: GUI keyboard note triggering
    void triggerNoteOn(int midiNote, float velocity);
    void triggerNoteOff(int midiNote);

    // v3.0.0: Tuning engine access
    TuningEngine& getTuningEngine() { return tuningEngine; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // v3.1.0: Get held notes and their actual frequencies for TrueKeys visualization
    void getHeldNotesData(std::vector<int>& notes, std::vector<double>& frequencies);

    // APVTS::Listener override for tuning parameter changes
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Output level metering (peak values, 0.0 to 1.0)
    std::atomic<float> outputLevelLeft { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };

    // v2.7.0: Note event tracking for UI spoke highlighting
    // Bitfield: 128 bits for MIDI notes 0-127 (2 x uint64)
    std::atomic<uint64_t> activeNotesLow { 0 };   // notes 0-63
    std::atomic<uint64_t> activeNotesHigh { 0 };   // notes 64-127

private:
    // DSP Components (BEFORE parameters for initialization order)
    juce::Synthesiser synthesiser;

    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

    // v4.0.0: Effects chain (Chorus -> Delay -> Reverb -> EQ)
    juce::dsp::Chorus<float> chorus;
    DelayProcessor delayProcessor;
    EQProcessor eqProcessor;
    ReverbProcessor reverbFDN;

    // v3.0.0: Tuning engine
    TuningEngine tuningEngine;

    // One-pole lowpass filter state (v2.6.0)
    float lpFilterStateL = 0.0f;
    float lpFilterStateR = 0.0f;
    double currentSampleRate = 44100.0;

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
    std::atomic<float>* velocityCurveParam = nullptr;
    std::atomic<float>* pitchEnvelopeParam = nullptr;
    std::atomic<float>* pitchEnvTimeParam = nullptr;
    // Multi-stage envelope (4 params, always active in v1.2.0)
    std::atomic<float>* strikeTimeParam = nullptr;
    std::atomic<float>* brillianceParam = nullptr;
    std::atomic<float>* bodyTimeParam = nullptr;
    std::atomic<float>* humSustainParam = nullptr;
    // Realism (v2.4.0)
    std::atomic<float>* humanizeParam = nullptr;
    // Lowpass Filter (v2.6.0)
    std::atomic<float>* lpFilterEnabledParam = nullptr;
    std::atomic<float>* lpFilterCutoffParam = nullptr;
    // High Fidelity (v3.1.2)
    std::atomic<float>* highFidelityParam = nullptr;
    // Tuning (v3.0.0)
    std::atomic<float>* tuningMasterTuneParam = nullptr;
    std::atomic<float>* tuningOctaveStretchParam = nullptr;
    std::atomic<float>* tuningPitchBendRangeParam = nullptr;
    std::atomic<float>* tuningTemperamentPresetParam = nullptr;
    // Output
    std::atomic<float>* outputGainParam = nullptr;

    // v4.0.0: Effects chain cached parameter pointers
    struct EffectsParamCache
    {
        std::atomic<float>* chorusBypass = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* delayBypass = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMode = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* eqBypass = nullptr;
        std::atomic<float>* eqLowGain = nullptr;
        std::atomic<float>* eqMidGain = nullptr;
        std::atomic<float>* eqMidFreq = nullptr;
        std::atomic<float>* eqHighGain = nullptr;
        std::atomic<float>* reverbBypass = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamp = nullptr;
        std::atomic<float>* reverbPredelay = nullptr;
        std::atomic<float>* reverbMix = nullptr;
        std::atomic<float>* reverbMod = nullptr;
        std::atomic<float>* reverbShimmer = nullptr;
    };
    EffectsParamCache fxCache;

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
