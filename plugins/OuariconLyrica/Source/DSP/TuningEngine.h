/*
  ==============================================================================

    TuningEngine.h
    Phase 2.8: Tuning Engine - MIDI to Frequency Conversion

    Implements:
    - 12-TET base tuning with adjustable A4 reference (masterTune)
    - Per-note pitch bend (±pitchBendRange semitones)
    - Scale frequency retrieval for glissando (12-TET scales)

    Optional features (not implemented):
    - MTS-ESP client integration (libMTSClient)
    - Scala file loader (.scl/.kbm)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <atomic>

/**
 * TuningEngine: Converts MIDI notes to frequencies with tuning flexibility
 *
 * - 12-TET baseline with adjustable A4 reference frequency
 * - Per-note pitch bend for expression
 * - Scale generation for glissando controller
 */
class TuningEngine
{
public:
    TuningEngine();
    ~TuningEngine() = default;

    /**
     * Set the reference frequency for A4 (MIDI note 69)
     * @param freqHz Frequency in Hz (typically 440.0)
     */
    void setMasterTune(double freqHz);

    /**
     * Set the pitch bend range in semitones
     * @param semitones Range ±semitones (typically 2.0 for ±2 semitones)
     */
    void setPitchBendRange(float semitones);

    /**
     * Get frequency for a MIDI note with optional pitch bend
     * @param midiNote MIDI note number (0-127)
     * @param midiChannel MIDI channel (0-15) - reserved for future MTS-ESP
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
     * @param midiNote MIDI note number (0-127)
     */
    void clearPitchBend(int midiNote);

    /**
     * Clear all pitch bend data (e.g., on all-notes-off)
     */
    void clearAllPitchBends();

    /**
     * Get scale frequencies for glissando (12-TET scale degrees)
     * @param rootNote MIDI note number for root of scale
     * @param numNotes Number of scale degrees to return
     * @return Vector of frequencies in Hz
     */
    std::vector<double> getScaleFrequencies(int rootNote, int numNotes);

    /**
     * Load Scala tuning file (PLACEHOLDER - not implemented)
     * @param scl Scala scale file (.scl)
     * @param kbm Scala keyboard mapping file (.kbm)
     * @return true if loaded successfully
     */
    bool loadScalaFile(const juce::File& scl, const juce::File& kbm);

    /**
     * Connect to MTS-ESP tuning source (PLACEHOLDER - not implemented)
     * @return true if connected successfully
     */
    bool connectMTSClient();

private:
    /**
     * Calculate 12-TET frequency for MIDI note
     * @param midiNote MIDI note number (0-127)
     * @return Frequency in Hz (without pitch bend)
     */
    double calculate12TETFrequency(int midiNote) const;

    /**
     * Apply pitch bend to base frequency
     * @param baseFreq Base frequency in Hz
     * @param bendAmount Normalized bend amount (-1.0 to +1.0)
     * @return Bent frequency in Hz
     */
    double applyPitchBend(double baseFreq, float bendAmount) const;

    // Tuning state
    double a4Frequency = 440.0;      // Reference frequency for A4 (MIDI 69)
    float pitchBendRange = 2.0f;     // Pitch bend range in semitones (±2)

    // Thread-safe per-note pitch bend storage
    // Uses atomic array for lock-free access from audio thread
    // Sentinel value NO_BEND (2.0f) indicates no bend applied (valid range is -1.0 to 1.0)
    static constexpr float NO_BEND = 2.0f;
    std::array<std::atomic<float>, 128> notePitchBends;

    // Future expansion
    bool mtsSynthClientConnected = false;
    bool scalaFileLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningEngine)
};
