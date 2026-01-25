/*
  ==============================================================================

    O-Lyrica - Audio Processor
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

// v1.7.10: Thread-safe MPSC queue for MIDI events (for UI visualization)
// Multiple producers (audio thread + UI thread), single consumer (timer callback)
// Uses try-lock pattern - drops events if contention, acceptable for visualization
class MidiEventQueue
{
public:
    static constexpr size_t kMaxEvents = 32;  // Max events per timer callback

    void push(const MidiNoteEvent& event)
    {
        // v1.7.10 FIX: Use try-lock pattern to prevent race conditions
        // For UI visualization, dropping events under contention is acceptable
        bool expected = false;
        if (!pushLock.compare_exchange_strong(expected, true, std::memory_order_acquire))
            return;  // Another thread is pushing, drop this event

        size_t currentWrite = writePos.load(std::memory_order_relaxed);
        size_t nextWrite = (currentWrite + 1) % kMaxEvents;

        // Check if queue is full
        if (nextWrite != readPos.load(std::memory_order_acquire))
        {
            events[currentWrite] = event;
            writePos.store(nextWrite, std::memory_order_release);
        }

        pushLock.store(false, std::memory_order_release);
    }

    bool pop(MidiNoteEvent& event)
    {
        // Single consumer - no lock needed
        size_t currentRead = readPos.load(std::memory_order_relaxed);
        if (currentRead == writePos.load(std::memory_order_acquire))
            return false;  // Queue empty

        event = events[currentRead];
        readPos.store((currentRead + 1) % kMaxEvents, std::memory_order_release);
        return true;
    }

private:
    std::array<MidiNoteEvent, kMaxEvents> events{};
    std::atomic<size_t> writePos { 0 };
    std::atomic<size_t> readPos { 0 };
    std::atomic<bool> pushLock { false };  // v1.7.10: Spinlock for push()
};

class OLyricaAudioProcessor : public juce::AudioProcessor
{
public:
    OLyricaAudioProcessor();
    ~OLyricaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Lyrica"; }
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

    /**
     * v1.10.0: Get held notes and their frequencies for True Keys visualization
     * @param notes Output vector of MIDI note numbers
     * @param frequencies Output vector of corresponding frequencies
     */
    void getHeldNotesData(std::vector<int>& notes, std::vector<double>& frequencies);

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

    // v1.13.3: Flag to prevent processBlock from syncing mode during state restoration
    std::atomic<bool> isRestoringState { false };

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // v1.5.0: Factory preset initialization
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OLyricaAudioProcessor)
};
