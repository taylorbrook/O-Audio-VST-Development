/*
  ==============================================================================

    O-IntonationPad - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "DSP/ChordGenerator.h"
#include "DSP/TuningEngine.h"
#include "DSP/WavetableVoice.h"
#include "DSP/DelayProcessor.h"
#include "DSP/EQProcessor.h"
#include "DSP/ReverbProcessor.h"

// Immutable snapshot published by UI thread, read lock-free by audio thread
struct IntervalSnapshot
{
    std::vector<bool> enabledFlags;      // full toggle state for UI/state queries
    std::vector<int> enabledDegrees;     // sorted enabled degree indices (always has at least 0)
    int scaleDegreeCount;
};

struct ActiveNoteInfo
{
    int midiNote;
    float frequencyHz;
    float gain;  // 0.0-1.0, complexity-based gain for this sub-voice
};

class OIntonationPadAudioProcessor : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener
{
public:
    OIntonationPadAudioProcessor();
    ~OIntonationPadAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-IntonationPad"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Tuning engine access
    TuningEngine& getTuningEngine() { return tuningEngine; }

    // APVTS listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // UI data access: collect active sub-voice data from all synthesiser voices
    std::vector<ActiveNoteInfo> getActiveNotes() const;

    // v2.4.1: LFO-modulated wavetable positions for UI waveform display
    float getModulatedPosA() const { return currentModPosA_.load(std::memory_order_relaxed); }
    float getModulatedPosB() const { return currentModPosB_.load(std::memory_order_relaxed); }

    // v1.5.0: Enabled interval management
    std::vector<bool> getEnabledIntervals() const;
    void setIntervalEnabled(int index, bool enabled);
    void resetEnabledIntervals();  // Reset all to enabled (called on scale change)
    void checkAndResetForScaleChange();  // Call after tuning changes that may alter degree count
    int getScaleDegreeCount() const;
    std::vector<int> getEnabledDegreeOffsets() const;  // Returns sorted list of enabled degree indices

private:
    // DSP Components (declare BEFORE parameters for initialization order)
    juce::Synthesiser synthesiser;
    ChordGenerator chordGenerator;
    TuningEngine tuningEngine;

    // Global LFOs (independent per oscillator)
    double lfoPhaseA = 0.0;
    double lfoPhaseIncrementA = 0.0;
    double lfoPhaseB = 0.0;
    double lfoPhaseIncrementB = 0.0;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;

    // v1.11.0: Effects chain (Chorus -> Delay -> EQ -> Reverb)
    juce::dsp::Chorus<float> chorus;
    DelayProcessor delay;
    EQProcessor eq;
    ReverbProcessor reverbProcessor;

    // Randomization
    // juce::Random is NOT thread-safe, but safe here: only accessed from the audio
    // callback (processBlock -> synthesiser.renderNextBlock -> WavetableVoice::startNote).
    // A pointer is passed to voices, but all access is single-threaded within the callback.
    juce::Random randomGenerator;

    // Parameters (APVTS comes after DSP components)
    juce::AudioProcessorValueTreeState parameters;

    // v1.5.0/v1.15.2/v2.0.3: Lock-free interval double-buffer
    // UI thread writes to inactive slot then flips index; audio thread reads active slot
    IntervalSnapshot intervalSnapshots_[2];
    std::atomic<int> activeSnapshotIndex_ { 0 };
    int lastKnownScaleSize_ = 0;  // Message-thread only: tracks scale changes for auto-reset

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached parameter pointers (stable for APVTS lifetime, eliminates string lookups in processBlock)
    std::atomic<float>* cachedVoiceCount = nullptr;
    std::atomic<float>* cachedComplexity = nullptr;
    std::atomic<float>* cachedKeyRoot = nullptr;
    std::atomic<float>* cachedStereoSpread = nullptr;
    std::atomic<float>* cachedSpacing = nullptr;
    std::atomic<float>* cachedInversion = nullptr;
    std::atomic<float>* cachedTimingRandom = nullptr;
    std::atomic<float>* cachedDetuneRandom = nullptr;
    std::atomic<float>* cachedWavetableBank = nullptr;
    std::atomic<float>* cachedWavetablePos = nullptr;
    std::atomic<float>* cachedWavetableBank2 = nullptr;
    std::atomic<float>* cachedWavetablePos2 = nullptr;
    std::atomic<float>* cachedGainA = nullptr;
    std::atomic<float>* cachedGainB = nullptr;
    std::atomic<float>* cachedAttackTime = nullptr;
    std::atomic<float>* cachedReleaseTime = nullptr;
    std::atomic<float>* cachedDecayTime = nullptr;
    std::atomic<float>* cachedSustainLevel = nullptr;
    std::atomic<float>* cachedLfoRate = nullptr;
    std::atomic<float>* cachedLfoRate2 = nullptr;
    std::atomic<float>* cachedLfoDepth = nullptr;
    std::atomic<float>* cachedLfoDepth2 = nullptr;
    std::atomic<float>* cachedFilterCutoff = nullptr;
    std::atomic<float>* cachedFilterLfoDepth = nullptr;
    std::atomic<float>* cachedVelocityToFilter = nullptr;
    std::atomic<float>* cachedMasterVolume = nullptr;

    // v2.2.0: Most-recent note-on velocity for filter modulation (audio thread only)
    float lastNoteVelocity = 1.0f;

    // v2.4.1: LFO-modulated positions (written by audio thread, read by UI timer)
    std::atomic<float> currentModPosA_ { 0.5f };
    std::atomic<float> currentModPosB_ { 0.5f };
    // Effects
    std::atomic<float>* cachedChorusBypass = nullptr;
    std::atomic<float>* cachedChorusRate = nullptr;
    std::atomic<float>* cachedChorusDepth = nullptr;
    std::atomic<float>* cachedChorusMix = nullptr;
    std::atomic<float>* cachedDelayBypass = nullptr;
    std::atomic<float>* cachedDelayTime = nullptr;
    std::atomic<float>* cachedDelayFeedback = nullptr;
    std::atomic<float>* cachedDelayMode = nullptr;
    std::atomic<float>* cachedDelayMix = nullptr;
    std::atomic<float>* cachedEqBypass = nullptr;
    std::atomic<float>* cachedEqLowGain = nullptr;
    std::atomic<float>* cachedEqMidGain = nullptr;
    std::atomic<float>* cachedEqMidFreq = nullptr;
    std::atomic<float>* cachedEqHighGain = nullptr;
    std::atomic<float>* cachedReverbBypass = nullptr;
    std::atomic<float>* cachedReverbSize = nullptr;
    std::atomic<float>* cachedReverbDamp = nullptr;
    std::atomic<float>* cachedReverbPredelay = nullptr;
    std::atomic<float>* cachedReverbMix = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OIntonationPadAudioProcessor)
};
