/*
  ==============================================================================

    TuningEngine.cpp
    Phase 2.3: Microtonal tuning implementation

  ==============================================================================
*/

#include "TuningEngine.h"
#include <cmath>

TuningEngine::TuningEngine()
{
    // Initialize frequency table with 12-TET at 440 Hz
    rebuildFrequencyTable();
}

TuningEngine::~TuningEngine()
{
}

double TuningEngine::getFrequency(int midiNote) const
{
    // Apply tonic transposition: shift MIDI note by tonic offset
    // When tonic = D (2), pressing C (60) should play D (62) frequency
    int tonic = tonicOffset.load();
    int transposedNote = midiNote + tonic;

    // Clamp transposed note to valid range
    transposedNote = juce::jlimit(0, 127, transposedNote);

    Mode mode = currentMode.load();

    // MTS-ESP priority (stub for Phase 2.3 - full integration later)
    if (mode == Mode::MTSESP)
    {
        // TODO: Full MTS-ESP integration in future improvement
        // For now, fall back to frequency table (Scala or 12-TET)
        return frequencyTable[transposedNote].load();
    }

    // Return pre-calculated frequency from table
    return frequencyTable[transposedNote].load();
}

void TuningEngine::setMode(Mode mode)
{
    if (currentMode.load() == mode)
        return;

    currentMode.store(mode);

    // Rebuild frequency table when mode changes
    rebuildFrequencyTable();
}

void TuningEngine::setReferencePitch(double freq)
{
    // Clamp to reasonable range (400-480 Hz)
    freq = juce::jlimit(400.0, 480.0, freq);

    if (std::abs(referencePitch.load() - freq) < 0.01)
        return;

    referencePitch.store(freq);

    // Rebuild frequency table when reference pitch changes
    rebuildFrequencyTable();
}

bool TuningEngine::loadScalaFile(const juce::File& sclFile)
{
    if (!sclFile.existsAsFile())
        return false;

    std::vector<double> intervals;
    juce::String name;

    // Parse file
    if (!parseScalaFile(sclFile, intervals, name))
        return false;

    // Validation: Need at least unison (0.0) and octave
    if (intervals.size() < 2)
        return false;

    // Store scale data
    scaleIntervals = intervals;
    scaleDegrees = static_cast<int>(intervals.size()) - 1;  // Exclude unison
    scaleName = name;
    scalaFilePath = sclFile.getFullPathName();

    // Rebuild frequency table with new scale
    rebuildFrequencyTable();

    return true;
}

bool TuningEngine::loadKBMFile(const juce::File& kbmFile)
{
    // KBM (keyboard mapping) support deferred to future improvement
    // For Phase 2.3, we use default linear mapping (MIDI note = scale degree)
    juce::ignoreUnused(kbmFile);

    kbmFilePath = kbmFile.getFullPathName();
    return true;
}

void TuningEngine::setTonicNote(int tonicIndex)
{
    // Clamp to valid range (0-11, representing C through B)
    tonicIndex = juce::jlimit(0, 11, tonicIndex);
    tonicOffset.store(tonicIndex);

    // No need to rebuild frequency table - transposition is applied in getFrequency()
}

void TuningEngine::setCustomIntervals(const std::vector<double>& cents, const juce::String& name)
{
    // Validate: need at least some intervals
    if (cents.empty())
        return;

    // Store scale data
    scaleIntervals.clear();
    scaleIntervals.push_back(0.0);  // Always start with unison

    for (size_t i = 0; i < cents.size(); ++i)
    {
        scaleIntervals.push_back(cents[i]);
    }

    scaleDegrees = static_cast<int>(scaleIntervals.size()) - 1;
    scaleName = name;
    scalaFilePath = "";  // Not from file

    // Switch to Scala mode to use custom intervals
    currentMode.store(Mode::Scala);

    // Rebuild frequency table with new scale
    rebuildFrequencyTable();
}

juce::String TuningEngine::getActiveTuningName() const
{
    Mode mode = currentMode.load();

    switch (mode)
    {
        case Mode::Scala:
            return hasScalaLoaded() ? scaleName : "12-TET (No Scala loaded)";
        case Mode::MTSESP:
            return "MTS-ESP (Stub - falls back to table)";
        case Mode::TwelveTET:
        default:
            return "12-TET";
    }
}

void TuningEngine::rebuildFrequencyTable()
{
    Mode mode = currentMode.load();
    double refPitch = referencePitch.load();

    for (int note = 0; note < 128; ++note)
    {
        double freq;

        // Use Scala if in Scala mode and scale is loaded
        if (mode == Mode::Scala && hasScalaLoaded())
        {
            freq = calculateScala(note);
        }
        else
        {
            // Fall back to 12-TET (for TwelveTET mode or when Scala not loaded)
            freq = calculate12TET(note);
        }

        // Store in atomic table
        frequencyTable[note].store(freq);
    }
}

double TuningEngine::calculate12TET(int midiNote) const
{
    double refPitch = referencePitch.load();

    // Standard 12-TET formula: f = 440 * 2^((n-69)/12)
    // Adjusted for reference pitch: multiply by (refPitch / 440)
    return refPitch * std::pow(2.0, (midiNote - 69) / 12.0);
}

double TuningEngine::calculateScala(int midiNote) const
{
    if (!hasScalaLoaded())
        return calculate12TET(midiNote);

    double refPitch = referencePitch.load();

    // Map MIDI note to scale degree
    // Reference point: MIDI 60 (Middle C) maps to scale degree 0
    int noteOffset = midiNote - 60;
    int octave = noteOffset / scaleDegrees;
    int degree = noteOffset % scaleDegrees;

    // Handle negative modulo correctly
    if (degree < 0)
    {
        degree += scaleDegrees;
        octave--;
    }

    // Get cents offset for this degree
    double cents = scaleIntervals[degree];

    // Add octaves (using the last interval as octave size)
    double octaveCents = scaleIntervals.back();
    cents += octave * octaveCents;

    // Calculate frequency from middle C
    // Middle C (60) in 12-TET at reference pitch:
    double middleCFreq = refPitch * std::pow(2.0, -9.0 / 12.0);  // C4 from A4

    // Apply cents offset: f = f0 * 2^(cents/1200)
    return middleCFreq * std::pow(2.0, cents / 1200.0);
}

bool TuningEngine::parseScalaFile(const juce::File& file, std::vector<double>& intervals, juce::String& name)
{
    // Read file lines
    juce::StringArray lines;
    file.readLines(lines);

    if (lines.isEmpty())
        return false;

    intervals.clear();
    intervals.push_back(0.0);  // Unison (0 cents)

    bool foundDescription = false;
    bool foundCount = false;
    int expectedDegrees = 0;

    for (const auto& line : lines)
    {
        auto trimmed = line.trim();

        // Skip empty lines and comments
        if (trimmed.isEmpty() || trimmed.startsWith("!"))
            continue;

        // First non-comment line is description
        if (!foundDescription)
        {
            name = trimmed;
            foundDescription = true;
            continue;
        }

        // Second non-comment line is note count
        if (!foundCount)
        {
            expectedDegrees = trimmed.getIntValue();
            if (expectedDegrees <= 0 || expectedDegrees > 128)
                return false;  // Invalid count

            foundCount = true;
            continue;
        }

        // Parse interval (cents or ratio)
        if (trimmed.contains("."))
        {
            // Cents value (e.g., "193.15")
            double cents = trimmed.getDoubleValue();
            intervals.push_back(cents);
        }
        else if (trimmed.contains("/"))
        {
            // Ratio (e.g., "3/2" for perfect fifth)
            auto parts = juce::StringArray::fromTokens(trimmed, "/", "");
            if (parts.size() == 2)
            {
                double numerator = parts[0].getDoubleValue();
                double denominator = parts[1].getDoubleValue();

                if (denominator > 0.0)
                {
                    double ratio = numerator / denominator;
                    double cents = 1200.0 * std::log2(ratio);
                    intervals.push_back(cents);
                }
            }
        }
        else
        {
            // Integer ratio (e.g., "2" for octave)
            double ratio = trimmed.getDoubleValue();
            if (ratio > 0.0)
            {
                double cents = 1200.0 * std::log2(ratio);
                intervals.push_back(cents);
            }
        }
    }

    // Validation: Must have correct number of intervals (count + 1 including unison)
    // Note: Some .scl files include the octave, some don't
    // We accept if we have at least 2 intervals (unison + at least one more)
    if (intervals.size() < 2)
        return false;

    return true;
}
