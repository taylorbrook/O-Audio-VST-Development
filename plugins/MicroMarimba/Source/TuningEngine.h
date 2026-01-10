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

    // Load Scala file (returns true on success)
    bool loadScalaFile(const juce::File& sclFile);
    bool loadKBMFile(const juce::File& kbmFile);

    // Get active tuning name for UI
    juce::String getActiveTuningName() const;

    // Check if a scale is loaded
    bool hasScalaLoaded() const { return !scaleIntervals.empty() && scaleDegrees > 0; }

private:
    std::atomic<Mode> currentMode { Mode::TwelveTET };
    std::atomic<double> referencePitch { 440.0 };

    // Frequency table (128 MIDI notes)
    // Using array of atomics for lock-free reads from audio thread
    std::array<std::atomic<double>, 128> frequencyTable;

    // Scala scale data (protected by mutex for loading from message thread)
    std::vector<double> scaleIntervals;  // In cents
    int scaleDegrees = 12;
    juce::String scaleName = "12-TET";
    juce::String scalaFilePath;
    juce::String kbmFilePath;

    // Build frequency table from current settings
    void rebuildFrequencyTable();

    // 12-TET calculation
    double calculate12TET(int midiNote) const;

    // Scala calculation
    double calculateScala(int midiNote) const;

    // Parse Scala .scl file
    bool parseScalaFile(const juce::File& file, std::vector<double>& intervals, juce::String& name);
};
