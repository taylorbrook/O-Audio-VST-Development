/*
  ==============================================================================

    Ouaricon Marimba - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TuningEngine.h"
#include "BodyResonance.h"
#include "PresetManager.h"
#include "AnalogEQUnit.h"

// Lock-free FIFO for waveform visualization
class WaveformFifo
{
public:
    static constexpr int kBufferSize = 512;  // Samples for one waveform frame

    void write(const float* data, int numSamples)
    {
        // Simple overwrite strategy - always keep latest samples
        for (int i = 0; i < numSamples && i < kBufferSize; ++i)
        {
            buffer[(writePos + i) % kBufferSize] = data[i];
        }
        writePos = (writePos + numSamples) % kBufferSize;
        newDataAvailable.store(true);
    }

    bool read(float* dest, int numSamples)
    {
        if (!newDataAvailable.exchange(false))
            return false;  // No new data

        // Read from the position that gives us the most recent complete buffer
        int readStart = (writePos - numSamples + kBufferSize) % kBufferSize;
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = buffer[(readStart + i) % kBufferSize];
        }
        return true;
    }

private:
    std::array<std::atomic<float>, kBufferSize> buffer{};
    std::atomic<int> writePos { 0 };
    std::atomic<bool> newDataAvailable { false };
};

// v1.2.6: MIDI event for polyphonic note tracking (note-on and note-off)
struct MidiNoteEvent
{
    int noteNumber;
    float velocity;  // 0.0 = note-off, >0.0 = note-on with velocity
    bool isNoteOn;
};

// Lock-free queue for MIDI events (for UI visualization)
class MidiEventQueue
{
public:
    static constexpr int kMaxEvents = 32;  // Max events per timer callback

    void push(const MidiNoteEvent& event)
    {
        int nextWrite = (writePos.load() + 1) % kMaxEvents;
        if (nextWrite != readPos.load())  // Don't overwrite unread events
        {
            events[writePos.load()] = event;
            writePos.store(nextWrite);
        }
    }

    bool pop(MidiNoteEvent& event)
    {
        int currentRead = readPos.load();
        if (currentRead == writePos.load())
            return false;  // Queue empty

        event = events[currentRead];
        readPos.store((currentRead + 1) % kMaxEvents);
        return true;
    }

private:
    std::array<MidiNoteEvent, kMaxEvents> events{};
    std::atomic<int> writePos { 0 };
    std::atomic<int> readPos { 0 };
};

class OuariconMarimbaAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconMarimbaAudioProcessor();
    ~OuariconMarimbaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Marimba"; }
    bool acceptsMidi() const override { return true; }  // Synth accepts MIDI
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

    // Public access to parameters for editor
    juce::AudioProcessorValueTreeState parameters;

    // Phase 2.3: Public access to tuning engine for UI
    TuningEngine& getTuningEngine() { return tuningEngine; }

    // v1.3.0: Public access to preset manager for UI
    PresetManager& getPresetManager() { return presetManager; }

    // v1.2.3: Public access to waveform FIFO for oscilloscope display
    WaveformFifo& getWaveformFifo() { return waveformFifo; }

    // v1.2.5: VU Meter level (atomic for thread-safe access from editor)
    std::atomic<float> outputLevelDB { -100.0f };

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (Phase 2.1: Basic synthesizer)
    juce::Synthesiser synthesiser;

    // Phase 2.3: Tuning engine
    TuningEngine tuningEngine;

    // v1.3.0: Preset manager (must be after parameters and tuningEngine)
    PresetManager presetManager { parameters, tuningEngine };

    // Phase 2.4: Body resonance (convolution)
    BodyResonance bodyResonance;

    // v1.8.0: Analog EQ Unit (effects tab)
    AnalogEQUnit eqUnit;

    // v1.2.3: Waveform FIFO for oscilloscope display
    WaveformFifo waveformFifo;

    // UI keyboard MIDI injection
    juce::MidiBuffer pendingUiMidi;
    juce::CriticalSection midiLock;

    // v1.2.6: MIDI event queue for polyphonic note visualization
    MidiEventQueue midiEventQueue;

public:
    // Called from UI thread to inject MIDI from WebView keyboard
    void addMidiMessage(const juce::MidiMessage& msg);

    // v1.2.6: Pop next MIDI event from queue (returns false if empty)
    bool popMidiEvent(MidiNoteEvent& event)
    {
        return midiEventQueue.pop(event);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconMarimbaAudioProcessor)
};
