/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    TuningEngine.h
    scala-tuning-engine module v2.0.0

    Complete microtonal tuning system supporting:
    - 12-TET base tuning with adjustable A4 reference (masterTune)
    - Per-note pitch bend (±pitchBendRange semitones)
    - Scala file parsing (.scl) for custom tunings
    - Complete keyboard mapping file support (.kbm)
    - Built-in temperament presets (Werckmeister III, Pythagorean, etc.)
    - Octave stretch for physical modeling
    - Linear mapping for any scale size (7, 12, 19, 31, etc.)
    - Tonic selection as 12-TET semitone transposition

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <optional>

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
        Scala = 1,      // Custom tuning from Scala file
        MTSESP = 2      // MTS-ESP (placeholder - future implementation)
    };

    /**
     * Built-in temperament presets
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
    // Octave Stretch
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
    // Built-in Presets
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
     * Set a single interval by index
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

    /** What getActiveTuningName() returns in TwelveTET mode — and so what a
        session saved in that mode carries as its "scaleName" (PluginProcessor
        setStateInformation infers the mode from it, review IN-19). */
    static constexpr const char* kTwelveTetModeName = "12-TET Standard";

    // ═══════════════════════════════════════════════════════════════════
    // Tonic Selection
    // ═══════════════════════════════════════════════════════════════════

    /**
     * Set the tonic note (0-11, where 0=C, 1=C#, etc.)
     *
     * The tonic means different things with and without a KBM (IN-18,
     * kept deliberately):
     * - No KBM (linear mapping): tonic shifts the ANCHOR by 12-TET
     *   semitones. Anchor = MIDI 60 + tonic, pitched at its 12-TET
     *   frequency, and degree 0 there is the scale's own degree 0 — the
     *   intervals are NOT rotated. Works for any scale size (7, 12, 19, 31…).
     * - KBM loaded: the mapping owns the anchor, so tonic ROTATES the scale
     *   by scale degrees instead (rotateIntervalsForTonic): degree `tonic`
     *   becomes the new 0 cents, the mode of the same scale.
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
     * Reset keyboard mapping to default (linear mapping) and rebuild the table
     */
    void resetKeyboardMapping();

    /**
     * Load a keyboard mapping from .kbm text (session restore, WR-06)
     */
    bool loadKBMFromString(const juce::String& kbmText);

    /**
     * The loaded .kbm as text, or empty when no KBM is loaded (WR-06)
     */
    juce::String getKBMState() const;

    bool isKBMLoaded() const;

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

    double calculate12TETFrequency(int midiNote) const;
    double calculateCustomFrequency(int midiNote) const;
    double calculateTwelveTETTableFrequency(int midiNote) const;
    double calculateKBMFrequency(int midiNote, const std::vector<double>& intervals) const;
    static const std::vector<double>& twelveTETIntervals() noexcept;
    double applyPitchBend(double baseFreq, float bendAmount) const;
    void rebuildFrequencyTable();
    void rotateIntervalsForTonic(int tonic);
    std::optional<double> parseScalaPitch(const juce::String& line) const;

    // ═══════════════════════════════════════════════════════════════════
    // State
    // ═══════════════════════════════════════════════════════════════════

    // Core tuning parameters
    double a4Frequency = 440.0;
    float pitchBendRange = 2.0f;
    float octaveStretch = 1.0f;

    // Built-in preset tracking
    BuiltInPreset currentPreset = BuiltInPreset::Equal12TET;

    // Mode and scale
    std::atomic<Mode> currentMode { Mode::TwelveTET };
    std::atomic<int> tonicOffset { 0 };
    int scaleDegrees = 12;
    juce::String scaleName = "12-TET Standard";

    // Scale intervals (in cents, starting with 0.0 for unison)
    std::vector<double> scaleIntervals;

    // Rotated intervals cache for modal rotation
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

    int kbmMapSize = 0;
    int kbmFirstNote = 0;
    int kbmLastNote = 127;
    int kbmMiddleNote = 60;
    int kbmReferenceNote = 69;
    int kbmOctaveDegree = 0;
    std::vector<int> kbmMapping;
    // CR-07: the loaded .kbm's own reference frequency. Kept apart from
    // a4Frequency (the user's A4 / masterTune, clamped 400–480 Hz) — a KBM
    // reference is often middle C (261.63 Hz) and must not be clamped into,
    // or overwrite, the A4 range.
    double kbmReferenceFreq = 440.0;
    bool kbmLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningEngine)
};
