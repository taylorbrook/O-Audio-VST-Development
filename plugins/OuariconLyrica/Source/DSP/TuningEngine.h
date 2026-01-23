/*
  ==============================================================================

    TuningEngine.h
    v1.12.3: Tonic transposition now affects sounding pitches in 12-TET mode

    Implements:
    - 12-TET base tuning with adjustable A4 reference (masterTune)
    - Per-note pitch bend (±pitchBendRange semitones)
    - Scala file parsing (.scl) for custom tunings
    - Complete keyboard mapping file support (.kbm):
      * Map size, MIDI range, middle/reference notes
      * Unmapped key support ('x' entries)
      * Octave degree for non-octave repeating scales
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
     * v1.7.2: Added MTSESP mode placeholder
     */
    enum class Mode
    {
        TwelveTET = 0,  // Standard 12-tone equal temperament
        Scala = 1,      // Custom tuning from Scala file
        MTSESP = 2      // MTS-ESP (placeholder - future implementation)
    };

    /**
     * Built-in temperament presets (v1.9.0)
     * These are additive to existing Scala file loading
     */
    enum class BuiltInPreset
    {
        Equal12TET = 0,
        Pythagorean,
        Zarlino,
        MeantoneQuarter,
        WerckmeisterIII,
        KirnbergerIII,
        Vallotti,
        WellTempered,
        JustIntonation,
        BohlenPierce,
        Custom  // Set when user loads .scl file
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
    // Octave Stretch (v1.9.0)
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set octave stretch for physical modeling
     * @param stretch Stretch factor (0.95-1.25, default 1.0)
     *        > 1.0: wider octaves in upper register (piano-like)
     *        < 1.0: narrower octaves
     */
    void setOctaveStretch(float stretch);

    /**
     * Get current octave stretch factor
     */
    float getOctaveStretch() const { return octaveStretch; }

    // ═══════════════════════════════════════════════════════════════════
    // Built-in Presets (v1.9.0)
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set a built-in temperament preset
     * @param preset The preset to apply
     */
    void setBuiltInPreset(BuiltInPreset preset);

    /**
     * Get current built-in preset
     */
    BuiltInPreset getBuiltInPreset() const { return currentPreset; }

    /**
     * Get display name for current preset
     */
    juce::String getPresetName() const;

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
     * Set a single interval by index (v1.11.1)
     * Initializes to 12-TET if intervals are empty, then updates the specified index
     * @param index Scale degree index (0 = unison, 1 = first interval, etc.)
     * @param cents Interval value in cents
     */
    void setSingleInterval(int index, double cents);

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
    // Tonic (Modal Rotation) - v1.11.0
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set the tonic note (0-11, where 0=C, 1=C#, etc.)
     * v1.11.0: Performs modal rotation - the interval pattern rotates so that
     * the selected tonic becomes 0 cents. Notes below the tonic wrap around
     * (last interval minus 1200 cents).
     *
     * Example with intervals [0, 150, 200, ...]:
     *   Tonic C:  C=0¢, C#=150¢, D=200¢
     *   Tonic C#: C#=0¢, D=150¢, D#=200¢, C=-50¢ (wraps from 1150-1200)
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
     * Check if a MIDI note is mapped in the current keyboard mapping
     * @param midiNote MIDI note number (0-127)
     * @return true if mapped, false if unmapped ('x' in KBM)
     */
    bool isNoteMapped(int midiNote) const;

    /**
     * Reset keyboard mapping to default (linear 12-note mapping)
     */
    void resetKeyboardMapping();

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
     * v1.12.0: Rotate interval pattern for modal rotation
     * When tonic changes, create a rotated copy of intervals where the
     * tonic becomes 0 cents and subsequent notes follow the rotated pattern.
     * @param tonic The tonic offset (0-11)
     */
    void rotateIntervalsForTonic(int tonic);

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
    float octaveStretch = 1.0f;  // v1.9.0: Octave stretch (0.95-1.25)

    // v1.9.0: Built-in preset tracking
    BuiltInPreset currentPreset = BuiltInPreset::Equal12TET;

    // Mode and scale
    std::atomic<Mode> currentMode { Mode::TwelveTET };
    std::atomic<int> tonicOffset { 0 };
    int scaleDegrees = 12;
    juce::String scaleName = "12-TET Standard";

    // Scale intervals (in cents, starting with 0.0 for unison)
    std::vector<double> scaleIntervals;

    // v1.12.0: Rotated intervals cache for modal rotation
    // Computed when tonic changes - used by calculateCustomFrequency()
    std::vector<double> rotatedIntervals;

    mutable std::mutex intervalMutex;

    // Pre-computed frequency table for lock-free audio access
    std::array<std::atomic<double>, 128> frequencyTable;

    // Per-note pitch bend storage
    static constexpr float NO_BEND = 2.0f;
    std::array<std::atomic<float>, 128> notePitchBends;

    // ═══════════════════════════════════════════════════════════════════
    // Keyboard Mapping (KBM) State
    // ═══════════════════════════════════════════════════════════════════

    /**
     * KBM parameters (complete Scala keyboard mapping support)
     */
    int kbmMapSize = 12;          // Pattern repeats every N keys (0 = linear)
    int kbmFirstNote = 0;         // First MIDI note to retune
    int kbmLastNote = 127;        // Last MIDI note to retune
    int kbmMiddleNote = 60;       // MIDI note for scale degree 0
    int kbmReferenceNote = 69;    // MIDI note for reference frequency
    int kbmOctaveDegree = 12;     // Scale degree considered formal octave

    /**
     * Keyboard mapping entries
     * -1 = unmapped ('x' in KBM file)
     * >= 0 = scale degree for this position in the pattern
     */
    std::vector<int> kbmMapping;
    bool kbmLoaded = false;

    // Future expansion
    bool mtsSynthClientConnected = false;
    bool scalaFileLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningEngine)
};
