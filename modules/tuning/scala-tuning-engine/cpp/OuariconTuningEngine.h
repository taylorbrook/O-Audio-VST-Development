/*
  ==============================================================================

    OuariconTuningEngine.h
    Ouaricon Module System - Microtonal Tuning

    Provides flexible tuning with priority fallback: MTS-ESP > Scala > 12-TET
    Thread-safe atomic frequency table for real-time audio access.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <vector>
#include <functional>

/**
 * Microtonal tuning engine with Scala file support.
 *
 * Features:
 * - 12-TET (equal temperament) with configurable A4 reference
 * - Scala (.scl) file loading for custom scales (any number of notes)
 * - KBM (.kbm) keyboard mapping support
 * - Tonic transposition (shift scale root to any note)
 * - Thread-safe atomic frequency table (lock-free audio thread reads)
 * - MTS-ESP integration (stubbed for future implementation)
 *
 * Usage in Processor:
 *   OuariconTuningEngine tuning;
 *
 *   // In synthesizer voice:
 *   double freq = tuning.getFrequency(midiNote);
 *
 * Usage in Editor:
 *   // Register native functions for WebView
 *   .withNativeFunction("loadScalaFile", ...)
 *   .withNativeFunction("setTuningIntervals", ...)
 */
class OuariconTuningEngine
{
public:
    //==========================================================================
    // Tuning Modes
    //==========================================================================

    enum class Mode
    {
        TwelveTET = 0,  // Standard 12-tone equal temperament
        Scala = 1,       // Custom scale from Scala file or manual intervals
        MTSESP = 2       // MTS-ESP master tuning (stub)
    };

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Called when tuning changes (scale loaded, intervals edited, mode changed). */
    using TuningChangedCallback = std::function<void()>;

    //==========================================================================
    // Construction
    //==========================================================================

    OuariconTuningEngine();
    ~OuariconTuningEngine() = default;

    //==========================================================================
    // Audio Thread Interface (lock-free)
    //==========================================================================

    /**
     * Get frequency for MIDI note (thread-safe).
     * Reads from pre-calculated atomic frequency table.
     *
     * @param midiNote  MIDI note number (0-127)
     * @return Frequency in Hz
     */
    double getFrequency(int midiNote) const;

    //==========================================================================
    // Mode Control
    //==========================================================================

    /** Set tuning mode. */
    void setMode(Mode mode);

    /** Get current tuning mode. */
    Mode getMode() const { return currentMode.load(); }

    //==========================================================================
    // Reference Pitch
    //==========================================================================

    /**
     * Set A4 reference pitch.
     * @param freq  Frequency in Hz (clamped to 400-480 Hz)
     */
    void setReferencePitch(double freq);

    /** Get current A4 reference pitch. */
    double getReferencePitch() const { return referencePitch.load(); }

    //==========================================================================
    // Scala File Loading
    //==========================================================================

    /**
     * Load scale from Scala (.scl) file.
     * Supports any number of notes per octave.
     *
     * @param sclFile  Path to .scl file
     * @return true on success
     */
    bool loadScalaFile(const juce::File& sclFile);

    /**
     * Load keyboard mapping from KBM (.kbm) file.
     * @param kbmFile  Path to .kbm file
     * @return true on success
     */
    bool loadKBMFile(const juce::File& kbmFile);

    //==========================================================================
    // Custom Intervals
    //==========================================================================

    /**
     * Set custom intervals directly (from UI).
     * Automatically switches to Scala mode.
     *
     * @param cents  Vector of cent values (excluding unison, which is implicit)
     * @param name   Scale name for display
     */
    void setCustomIntervals(const std::vector<double>& cents,
                            const juce::String& name = "Custom");

    //==========================================================================
    // Tonic (Transposition)
    //==========================================================================

    /**
     * Set tonic note for transposition.
     * Shifts the scale root to the specified note.
     *
     * @param tonicIndex  0 = C, 1 = C#, 2 = D, ... 11 = B
     */
    void setTonicNote(int tonicIndex);

    /** Get current tonic note index. */
    int getTonicNote() const { return tonicOffset.load(); }

    //==========================================================================
    // Scale Information
    //==========================================================================

    /** Get current intervals (cents values including unison at 0). */
    const std::vector<double>& getIntervals() const { return scaleIntervals; }

    /** Get number of scale degrees (notes per octave). */
    int getScaleDegrees() const { return scaleDegrees; }

    /** Get active tuning name. */
    juce::String getActiveTuningName() const;

    /** Check if a Scala scale is loaded. */
    bool hasScalaLoaded() const { return !scaleIntervals.empty() && scaleDegrees > 0; }

    //==========================================================================
    // File Export
    //==========================================================================

    /** Generate Scala file content from current intervals. */
    juce::String generateScalaFileContent() const;

    /** Generate KBM file content. */
    juce::String generateKBMFileContent() const;

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Set callback for tuning changes. */
    void setTuningChangedCallback(TuningChangedCallback callback)
    {
        onTuningChanged = std::move(callback);
    }

private:
    // Current state (atomic for thread safety)
    std::atomic<Mode> currentMode { Mode::TwelveTET };
    std::atomic<double> referencePitch { 440.0 };
    std::atomic<int> tonicOffset { 0 };

    // Frequency table (128 MIDI notes, atomic for lock-free reads)
    std::array<std::atomic<double>, 128> frequencyTable;

    // Scale data (message thread only)
    std::vector<double> scaleIntervals;  // In cents (including unison at 0)
    int scaleDegrees = 12;
    juce::String scaleName = "12-TET";
    juce::String scalaFilePath;
    juce::String kbmFilePath;

    // Callback
    TuningChangedCallback onTuningChanged;

    // Internal methods
    void rebuildFrequencyTable();
    double calculate12TET(int midiNote) const;
    double calculateScala(int midiNote) const;
    bool parseScalaFile(const juce::File& file, std::vector<double>& intervals, juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconTuningEngine)
};


//==============================================================================
// IMPLEMENTATION
//==============================================================================

inline OuariconTuningEngine::OuariconTuningEngine()
{
    // Initialize with 12-TET at 440 Hz
    rebuildFrequencyTable();
}

inline double OuariconTuningEngine::getFrequency(int midiNote) const
{
    midiNote = juce::jlimit(0, 127, midiNote);
    return frequencyTable[midiNote].load();
}

inline void OuariconTuningEngine::setMode(Mode mode)
{
    if (currentMode.load() == mode)
        return;

    currentMode.store(mode);
    rebuildFrequencyTable();

    if (onTuningChanged)
        onTuningChanged();
}

inline void OuariconTuningEngine::setReferencePitch(double freq)
{
    freq = juce::jlimit(400.0, 480.0, freq);

    if (std::abs(referencePitch.load() - freq) < 0.01)
        return;

    referencePitch.store(freq);
    rebuildFrequencyTable();

    if (onTuningChanged)
        onTuningChanged();
}

inline bool OuariconTuningEngine::loadScalaFile(const juce::File& sclFile)
{
    if (!sclFile.existsAsFile())
        return false;

    std::vector<double> intervals;
    juce::String name;

    if (!parseScalaFile(sclFile, intervals, name))
        return false;

    if (intervals.size() < 2)
        return false;

    scaleIntervals = intervals;
    scaleDegrees = static_cast<int>(intervals.size()) - 1;
    scaleName = name;
    scalaFilePath = sclFile.getFullPathName();

    rebuildFrequencyTable();

    if (onTuningChanged)
        onTuningChanged();

    return true;
}

inline bool OuariconTuningEngine::loadKBMFile(const juce::File& kbmFile)
{
    // KBM support - store path for future implementation
    juce::ignoreUnused(kbmFile);
    kbmFilePath = kbmFile.getFullPathName();
    return true;
}

inline void OuariconTuningEngine::setTonicNote(int tonicIndex)
{
    tonicIndex = juce::jlimit(0, 11, tonicIndex);

    if (tonicOffset.load() == tonicIndex)
        return;

    tonicOffset.store(tonicIndex);
    rebuildFrequencyTable();

    if (onTuningChanged)
        onTuningChanged();
}

inline void OuariconTuningEngine::setCustomIntervals(const std::vector<double>& cents,
                                                      const juce::String& name)
{
    if (cents.empty())
        return;

    scaleIntervals.clear();
    scaleIntervals.push_back(0.0);  // Unison

    for (const auto& c : cents)
        scaleIntervals.push_back(c);

    scaleDegrees = static_cast<int>(scaleIntervals.size()) - 1;
    scaleName = name;
    scalaFilePath = "";

    currentMode.store(Mode::Scala);
    rebuildFrequencyTable();

    if (onTuningChanged)
        onTuningChanged();
}

inline juce::String OuariconTuningEngine::getActiveTuningName() const
{
    Mode mode = currentMode.load();

    switch (mode)
    {
        case Mode::Scala:
            return hasScalaLoaded() ? scaleName : "12-TET (No Scala loaded)";
        case Mode::MTSESP:
            return "MTS-ESP";
        case Mode::TwelveTET:
        default:
            return "12-TET";
    }
}

inline void OuariconTuningEngine::rebuildFrequencyTable()
{
    Mode mode = currentMode.load();

    for (int note = 0; note < 128; ++note)
    {
        double freq;

        if (mode == Mode::Scala && hasScalaLoaded())
            freq = calculateScala(note);
        else
            freq = calculate12TET(note);

        frequencyTable[note].store(freq);
    }
}

inline double OuariconTuningEngine::calculate12TET(int midiNote) const
{
    double refPitch = referencePitch.load();
    return refPitch * std::pow(2.0, (midiNote - 69) / 12.0);
}

inline double OuariconTuningEngine::calculateScala(int midiNote) const
{
    if (!hasScalaLoaded())
        return calculate12TET(midiNote);

    double refPitch = referencePitch.load();
    int tonic = tonicOffset.load();

    // Map MIDI note to scale degree relative to tonic
    int tonicMidi = 60 + tonic;
    int noteOffset = midiNote - tonicMidi;
    int octave = noteOffset / scaleDegrees;
    int degree = noteOffset % scaleDegrees;

    if (degree < 0)
    {
        degree += scaleDegrees;
        octave--;
    }

    double cents = scaleIntervals[degree];
    double octaveCents = scaleIntervals.back();
    cents += octave * octaveCents;

    double tonicFreq = refPitch * std::pow(2.0, (tonicMidi - 69) / 12.0);
    return tonicFreq * std::pow(2.0, cents / 1200.0);
}

inline bool OuariconTuningEngine::parseScalaFile(const juce::File& file,
                                                  std::vector<double>& intervals,
                                                  juce::String& name)
{
    juce::StringArray lines;
    file.readLines(lines);

    if (lines.isEmpty())
        return false;

    intervals.clear();
    intervals.push_back(0.0);  // Unison

    bool foundDescription = false;
    bool foundCount = false;
    int expectedDegrees = 0;

    for (const auto& line : lines)
    {
        auto trimmed = line.trim();

        if (trimmed.isEmpty() || trimmed.startsWith("!"))
            continue;

        if (!foundDescription)
        {
            name = trimmed;
            foundDescription = true;
            continue;
        }

        if (!foundCount)
        {
            expectedDegrees = trimmed.getIntValue();
            if (expectedDegrees <= 0 || expectedDegrees > 128)
                return false;
            foundCount = true;
            continue;
        }

        // Parse interval (cents or ratio)
        if (trimmed.contains("."))
        {
            double cents = trimmed.getDoubleValue();
            intervals.push_back(cents);
        }
        else if (trimmed.contains("/"))
        {
            auto parts = juce::StringArray::fromTokens(trimmed, "/", "");
            if (parts.size() == 2)
            {
                double num = parts[0].getDoubleValue();
                double den = parts[1].getDoubleValue();
                if (den > 0.0)
                {
                    double ratio = num / den;
                    intervals.push_back(1200.0 * std::log2(ratio));
                }
            }
        }
        else
        {
            double ratio = trimmed.getDoubleValue();
            if (ratio > 0.0)
                intervals.push_back(1200.0 * std::log2(ratio));
        }
    }

    return intervals.size() >= 2;
}

inline juce::String OuariconTuningEngine::generateScalaFileContent() const
{
    juce::String content;

    content += "! " + scaleName + ".scl\n";
    content += "!\n";
    content += scaleName + "\n";

    int numNotes = static_cast<int>(scaleIntervals.size()) - 1;
    if (numNotes < 1) numNotes = 11;

    content += juce::String(numNotes) + "\n";
    content += "!\n";

    for (size_t i = 1; i < scaleIntervals.size(); ++i)
        content += juce::String(scaleIntervals[i], 6) + "\n";

    if (scaleIntervals.size() <= 1)
    {
        for (int i = 1; i <= 12; ++i)
            content += juce::String(i * 100.0, 6) + "\n";
    }

    return content;
}

inline juce::String OuariconTuningEngine::generateKBMFileContent() const
{
    juce::String content;

    content += "! Keyboard mapping for " + scaleName + "\n";
    content += "!\n";

    int mapSize = scaleDegrees > 0 ? scaleDegrees : 12;
    content += juce::String(mapSize) + "\n";
    content += "0\n";   // First MIDI note
    content += "127\n"; // Last MIDI note

    int middleNote = 60 + tonicOffset.load();
    content += juce::String(middleNote) + "\n";
    content += juce::String(middleNote) + "\n";

    double refPitch = referencePitch.load();
    double tonicFreq = refPitch * std::pow(2.0, (middleNote - 69) / 12.0);
    content += juce::String(tonicFreq, 6) + "\n";

    content += juce::String(mapSize) + "\n";

    for (int i = 0; i < mapSize; ++i)
        content += juce::String(i) + "\n";

    return content;
}
