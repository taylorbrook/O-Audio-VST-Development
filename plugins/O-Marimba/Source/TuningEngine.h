/*
  ==============================================================================

    TuningEngine.h
    Phase 2.3: Microtonal tuning system

    Provides flexible tuning with priority fallback: MTS-ESP > Scala > 12-TET

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <vector>

class TuningEngine
{
public:
    enum class Mode
    {
        TwelveTET = 0,
        Scala = 1,
        MTSESP = 2
    };

    TuningEngine();
    ~TuningEngine();

    // Get frequency for MIDI note (thread-safe)
    double getFrequency(int midiNote) const;

    // Set tuning mode
    void setMode(Mode mode);
    Mode getMode() const { return currentMode.load(); }

    // Set reference pitch (A4 frequency)
    void setReferencePitch(double freq);
    double getReferencePitch() const { return referencePitch.load(); }

    // WR-02 (v1.12.1): Complete any frequency-table rebuild that was deferred because
    // the message thread held the tuning lock. Poll this once per audio block. RT-safe
    // (non-blocking try-lock); does nothing when the table is already up to date.
    void serviceRebuild();

    // Load Scala file (returns true on success)
    bool loadScalaFile(const juce::File& sclFile);
    bool loadKBMFile(const juce::File& kbmFile);

    // Set custom intervals directly from UI (cents values, 12 entries for chromatic)
    void setCustomIntervals(const std::vector<double>& cents, const juce::String& name = "Custom");

    // Tonic (transposition) - transposes all notes by this many semitones
    void setTonicNote(int tonicIndex);  // 0 = C, 1 = C#, 2 = D, etc.
    int getTonicNote() const { return tonicOffset.load(); }

    // Get current intervals for UI display
    const std::vector<double>& getIntervals() const { return scaleIntervals; }
    int getScaleDegrees() const { return scaleDegrees; }

    // Get active tuning name for UI
    juce::String getActiveTuningName() const;

    // Check if a scale is loaded
    bool hasScalaLoaded() const { return !scaleIntervals.empty() && scaleDegrees > 0; }

    // v1.4.0: Generate Scala file content from current intervals
    juce::String generateScalaFileContent() const;

    // v1.4.0: Generate KBM (keyboard mapping) file content
    juce::String generateKBMFileContent() const;

private:
    std::atomic<Mode> currentMode { Mode::TwelveTET };
    std::atomic<double> referencePitch { 440.0 };
    std::atomic<int> tonicOffset { 0 };  // 0 = C, 1 = C#, 2 = D, etc. (transposition in semitones)

    // Frequency table (128 MIDI notes)
    // Using array of atomics for lock-free reads from audio thread
    std::array<std::atomic<double>, 128> frequencyTable;

    // Scala scale data. WR-02 (v1.12.1): guarded by tuningLock — mutated on the message
    // thread (setCustomIntervals/loadScalaFile/setTonicNote), read on the audio thread
    // only inside rebuildFrequencyTable()/calculateScala() under a try-lock. getFrequency()
    // reads the atomic frequencyTable instead and never touches these directly.
    std::vector<double> scaleIntervals;  // In cents
    int scaleDegrees = 12;
    juce::String scaleName = "12-TET";
    juce::String scalaFilePath;
    juce::String kbmFilePath;

    // WR-02 (v1.12.1): serializes access to scaleIntervals/scaleDegrees between the
    // message thread (mutation) and the audio thread (rebuild). The audio thread only
    // ever try-locks so it can never be blocked by a message-thread edit.
    juce::CriticalSection tuningLock;
    std::atomic<bool> tableDirty { false };

    // Build frequency table from current settings. MUST be called with tuningLock held.
    void rebuildFrequencyTable();

    // Rebuild now if the lock is free, otherwise mark the table dirty for serviceRebuild().
    void requestRebuild();

    // 12-TET calculation
    double calculate12TET(int midiNote) const;

    // Scala calculation
    double calculateScala(int midiNote) const;

    // Parse Scala .scl file
    bool parseScalaFile(const juce::File& file, std::vector<double>& intervals, juce::String& name);
};
