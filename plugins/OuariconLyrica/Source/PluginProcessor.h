/*
  ==============================================================================

    OuariconLyrica - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "HarpSynthSound.h"
#include "HarpSynthVoice.h"
#include "DSP/SympatheticResonance.h"
#include "DSP/TuningEngine.h"
#include "OuariconPresetManager.h"

// v1.7.9: MIDI event for polyphonic note tracking (visual feedback on tuning circle)
struct MidiNoteEvent
{
    int noteNumber;
    float velocity;  // 0.0 = note-off, >0.0 = note-on with velocity
};

// v1.7.9: Lock-free queue for MIDI events (for UI visualization)
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

class OuariconLyricaAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconLyricaAudioProcessor();
    ~OuariconLyricaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OuariconLyrica"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    // v1.3.2: Proper tail length for physical modeling (max decay + sympathetic resonance)
    double getTailLengthSeconds() const override { return 25.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    /**
     * Get pointer to sympathetic resonance engine (for voice access)
     */
    SympatheticResonanceEngine* getSympatheticEngine() { return &sympatheticEngine; }

    /**
     * Get pointer to tuning engine (for voice access)
     */
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    /**
     * Get number of currently active voices (Phase 2.11)
     * Returns the count of voices currently playing notes
     */
    int getActiveVoiceCount() const;

    /**
     * Get preset manager for UI integration (v1.5.0)
     */
    OuariconPresetManager& getPresetManager() { return presetManager; }

    /**
     * v1.7.4: Trigger note from WebView keyboard visualization
     * @param midiNote MIDI note number (0-127)
     * @param velocity Note velocity (0.0-1.0)
     */
    void triggerNoteOn(int midiNote, float velocity);

    /**
     * v1.7.4: Release note from WebView keyboard visualization
     * @param midiNote MIDI note number (0-127)
     */
    void triggerNoteOff(int midiNote);

    /**
     * v1.7.9: Pop MIDI event from queue for UI visualization
     * @param event Output event structure
     * @return true if event was available, false if queue empty
     */
    bool popMidiEvent(MidiNoteEvent& event)
    {
        return midiEventQueue.pop(event);
    }

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synthesiser;

    // Phase 2.7: Sympathetic Resonance Engine (processor-level, shared by all voices)
    SympatheticResonanceEngine sympatheticEngine;

    // Phase 2.8: Tuning Engine (processor-level, shared by all voices)
    TuningEngine tuningEngine;

    // v1.5.0: Preset Manager
    OuariconPresetManager presetManager;

    // v1.7.9: MIDI event queue for UI visualization (note flash on tuning circle)
    MidiEventQueue midiEventQueue;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // v1.5.0: Factory preset initialization
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconLyricaAudioProcessor)
};
