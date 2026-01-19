/*
  ==============================================================================

    TuningEngine.h
    v1.6.0: Full Tuning Module with 12-TET and Scala Support

    Implements:
    - 12-TET base tuning with adjustable A4 reference (masterTune)
    - Per-note pitch bend (±pitchBendRange semitones)
    - Scala file parsing (.scl) for custom tunings
    - Keyboard mapping file support (.kbm)
    - Scale frequency retrieval for glissando
    - Tonic transposition

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>

/**
 * TuningEngine: Converts MIDI notes to frequencies with tuning flexibility
 *
 * - Supports 12-TET and Custom (Scala) tuning modes
 * - Thread-safe frequency table for lock-free audio access
 * - Per-note pitch bend for expression
 */
class TuningEngine
{
public:
    /**
     * Tuning modes supported by the engine
     */
    enum class Mode
    {
        TwelveTET = 0,  // Standard 12-tone equal temperament
        Scala = 1       // Custom tuning from Scala file
    };

    TuningEngine();
    ~TuningEngine() = default;

    // ═══════════════════════════════════════════════════════════════════
    // Core Settings
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set the reference frequency for A4 (MIDI note 69)
     * @param freqHz Frequency in Hz (typically 440.0)
     */
    void setMasterTune(double freqHz);

    /**
     * Get current A4 reference frequency
     */
    double getMasterTune() const { return a4Frequency; }

    /**
     * Set the pitch bend range in semitones
     * @param semitones Range ±semitones (typically 2.0 for ±2 semitones)
     */
    void setPitchBendRange(float semitones);

    // ═══════════════════════════════════════════════════════════════════
    // Tuning Mode
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set the tuning mode
     * @param mode TwelveTET or Scala
     */
    void setMode(Mode mode);

    /**
     * Get current tuning mode
     */
    Mode getMode() const { return currentMode.load(std::memory_order_relaxed); }

    // ═══════════════════════════════════════════════════════════════════
    // Custom Intervals (Scala)
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set custom scale intervals (in cents)
     * @param cents Vector of intervals in cents (should start with 0.0 for unison)
     * @param name Display name for the scale
     */
    void setCustomIntervals(const std::vector<double>& cents, const juce::String& name);

    /**
     * Get current scale intervals (in cents)
     */
    std::vector<double> getIntervals() const;

    /**
     * Get number of degrees in the current scale
     */
    int getScaleDegrees() const { return scaleDegrees; }

    /**
     * Get the name of the active tuning
     */
    juce::String getActiveTuningName() const;

    // ═══════════════════════════════════════════════════════════════════
    // Tonic (Transposition)
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set the tonic note (0-11, where 0=C, 1=C#, etc.)
     * This transposes the scale so the root aligns with the selected tonic
     */
    void setTonicNote(int tonicIndex);

    /**
     * Get current tonic note
     */
    int getTonicNote() const { return tonicOffset.load(std::memory_order_relaxed); }

    // ═══════════════════════════════════════════════════════════════════
    // Scala File I/O
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Load Scala scale file (.scl)
     * @param sclFile The .scl file to load
     * @return true if loaded successfully
     */
    bool loadScalaFile(const juce::File& sclFile);

    /**
     * Load Scala keyboard mapping file (.kbm)
     * @param kbmFile The .kbm file to load
     * @return true if loaded successfully
     */
    bool loadKBMFile(const juce::File& kbmFile);

    /**
     * Generate Scala file content from current intervals
     */
    juce::String generateScalaFileContent() const;

    /**
     * Generate KBM file content from current mapping
     */
    juce::String generateKBMFileContent() const;

    // ═══════════════════════════════════════════════════════════════════
    // Frequency Retrieval (Audio Thread Safe)
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Get frequency for a MIDI note with optional pitch bend
     * @param midiNote MIDI note number (0-127)
     * @param midiChannel MIDI channel (0-15) - reserved for future use
     * @return Frequency in Hz
     */
    double getFrequency(int midiNote, int midiChannel = 0);

    /**
     * Set pitch bend for a specific MIDI note
     * @param midiNote MIDI note number (0-127)
     * @param bendAmount Normalized bend amount (-1.0 to +1.0)
     */
    void setPitchBend(int midiNote, float bendAmount);

    /**
     * Clear pitch bend for a specific note
     */
    void clearPitchBend(int midiNote);

    /**
     * Clear all pitch bend data (e.g., on all-notes-off)
     */
    void clearAllPitchBends();

    /**
     * Get scale frequencies for glissando
     * @param rootNote MIDI note number for root of scale
     * @param numNotes Number of scale degrees to return
     * @return Vector of frequencies in Hz
     */
    std::vector<double> getScaleFrequencies(int rootNote, int numNotes);

    // Deprecated compatibility stubs
    bool loadScalaFile(const juce::File& scl, const juce::File& kbm);
    bool connectMTSClient();

private:
    // ═══════════════════════════════════════════════════════════════════
    // Internal Methods
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Calculate 12-TET frequency for MIDI note
     */
    double calculate12TETFrequency(int midiNote) const;

    /**
     * Calculate custom-tuned frequency for MIDI note
     */
    double calculateCustomFrequency(int midiNote) const;

    /**
     * Apply pitch bend to base frequency
     */
    double applyPitchBend(double baseFreq, float bendAmount) const;

    /**
     * Rebuild the frequency lookup table (call on tuning changes)
     */
    void rebuildFrequencyTable();

    /**
     * Parse a single Scala pitch line (cents or ratio)
     * @return Interval in cents, or -1.0 on parse error
     */
    double parseScalaPitch(const juce::String& line) const;

    // ═══════════════════════════════════════════════════════════════════
    // State
    // ═══════════════════════════════════════════════════════════════════

    // Core tuning parameters
    double a4Frequency = 440.0;
    float pitchBendRange = 2.0f;

    // Mode and scale
    std::atomic<Mode> currentMode { Mode::TwelveTET };
    std::atomic<int> tonicOffset { 0 };
    int scaleDegrees = 12;
    juce::String scaleName = "12-TET Standard";

    // Scale intervals (in cents, starting with 0.0 for unison)
    std::vector<double> scaleIntervals;
    mutable std::mutex intervalMutex;

    // Pre-computed frequency table for lock-free audio access
    std::array<std::atomic<double>, 128> frequencyTable;

    // Per-note pitch bend storage
    static constexpr float NO_BEND = 2.0f;
    std::array<std::atomic<float>, 128> notePitchBends;

    // Future expansion
    bool mtsSynthClientConnected = false;
    bool scalaFileLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningEngine)
};
