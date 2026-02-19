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
#include "DSP/ScaleGenerator.h"
#include "DSP/TuningExporter.h"
#include "DSP/EmbeddedTunings.h"
#include "DSP/WavetableVoice.h"

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
    ~OIntonationPadAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-IntonationPad"; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Tuning engine access
    TuningEngine& getTuningEngine() { return tuningEngine; }

    // APVTS listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // UI data access: collect active sub-voice data from all synthesiser voices
    std::vector<ActiveNoteInfo> getActiveNotes() const;

    // v1.5.0: Enabled interval management
    std::vector<bool> getEnabledIntervals() const;
    void setIntervalEnabled(int index, bool enabled);
    void resetEnabledIntervals();  // Reset all to enabled (called on scale change)
    int getScaleDegreeCount() const;
    std::vector<int> getEnabledDegreeOffsets() const;  // Returns sorted list of enabled degree indices

private:
    // DSP Components (declare BEFORE parameters for initialization order)
    juce::Synthesiser synthesiser;
    ChordGenerator chordGenerator;
    TuningEngine tuningEngine;
    ScaleGenerator scaleGenerator;
    TuningExporter tuningExporter;

    // Global LFO
    double lfoPhase = 0.0;
    double lfoPhaseIncrement = 0.0;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::ProcessSpec filterSpec;

    // Randomization
    juce::Random randomGenerator;

    // Parameters (APVTS comes after DSP components)
    juce::AudioProcessorValueTreeState parameters;

    // v1.5.0: Enabled intervals (which scale degrees participate in chord generation)
    std::vector<bool> enabledIntervals;
    int lastKnownScaleSize = 0;  // Track scale changes to auto-reset
    mutable std::mutex enabledIntervalsMutex;

    // Audio-thread-safe cached copy (rebuilt when dirty flag is set)
    std::vector<int> cachedEnabledDegrees;
    int cachedScaleDegreeCount = 12;
    std::atomic<bool> enabledIntervalsDirty { true };

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OIntonationPadAudioProcessor)
};
