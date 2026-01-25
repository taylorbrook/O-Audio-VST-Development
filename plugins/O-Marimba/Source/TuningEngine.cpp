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
    // Clamp MIDI note to valid range
    midiNote = juce::jlimit(0, 127, midiNote);

    Mode mode = currentMode.load();

    // MTS-ESP priority (stub for Phase 2.3 - full integration later)
    if (mode == Mode::MTSESP)
    {
        // TODO: Full MTS-ESP integration in future improvement
        // For now, fall back to frequency table (Scala or 12-TET)
        return frequencyTable[midiNote].load();
    }

    // Return pre-calculated frequency from table
    // Tonic offset is applied during table rebuild (in calculateScala)
    return frequencyTable[midiNote].load();
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

    if (tonicOffset.load() == tonicIndex)
        return;

    tonicOffset.store(tonicIndex);

    // Rebuild frequency table - tonic affects interval calculations in Scala mode
    rebuildFrequencyTable();
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
    int tonic = tonicOffset.load();

    // Map MIDI note to scale degree relative to tonic
    // Tonic note (e.g., D when tonic=2) maps to scale degree 0
    int tonicMidi = 60 + tonic;  // MIDI note of the tonic in octave 4
    int noteOffset = midiNote - tonicMidi;
    int octave = noteOffset / scaleDegrees;
    int degree = noteOffset % scaleDegrees;

    // Handle negative modulo correctly
    if (degree < 0)
    {
        degree += scaleDegrees;
        octave--;
    }

    // Get cents offset for this degree (relative to tonic)
    double cents = scaleIntervals[degree];

    // Add octaves (using the last interval as octave size)
    double octaveCents = scaleIntervals.back();
    cents += octave * octaveCents;

    // Calculate tonic frequency in 12-TET (as the baseline reference)
    // This ensures the tonic note plays at its standard pitch
    double tonicFreq = refPitch * std::pow(2.0, (tonicMidi - 69) / 12.0);

    // Apply cents offset from tonic: f = tonicFreq * 2^(cents/1200)
    return tonicFreq * std::pow(2.0, cents / 1200.0);
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

// v1.4.0: Generate Scala file content from current intervals
juce::String TuningEngine::generateScalaFileContent() const
{
    juce::String content;

    // Comment line
    content += "! " + scaleName + ".scl\n";
    content += "!\n";

    // Scale name/description
    content += scaleName + "\n";

    // Number of notes (excluding unison, which is implicit)
    int numNotes = static_cast<int>(scaleIntervals.size()) - 1;
    if (numNotes < 1)
        numNotes = 11;  // Default to 12-TET if no intervals

    content += juce::String(numNotes) + "\n";
    content += "!\n";

    // Output intervals (skip unison at index 0)
    for (size_t i = 1; i < scaleIntervals.size(); ++i)
    {
        // Format cents with 6 decimal places for precision
        content += juce::String(scaleIntervals[i], 6) + "\n";
    }

    // If we don't have enough intervals, fill with 12-TET
    if (scaleIntervals.size() <= 1)
    {
        for (int i = 1; i <= 12; ++i)
        {
            content += juce::String(i * 100.0, 6) + "\n";
        }
    }

    return content;
}

// v1.4.0: Generate KBM (keyboard mapping) file content
juce::String TuningEngine::generateKBMFileContent() const
{
    juce::String content;

    // Comment line
    content += "! Keyboard mapping for " + scaleName + "\n";
    content += "!\n";

    // Size of map (number of entries in mapping, 0 = use scale size)
    int mapSize = scaleDegrees > 0 ? scaleDegrees : 12;
    content += juce::String(mapSize) + "\n";

    // First MIDI note to retune (0 = retune all)
    content += "0\n";

    // Last MIDI note to retune (127 = retune all)
    content += "127\n";

    // Middle note (usually 60 = C4)
    int middleNote = 60 + tonicOffset.load();
    content += juce::String(middleNote) + "\n";

    // Reference note for 1/1 (same as middle for our use)
    content += juce::String(middleNote) + "\n";

    // Reference frequency (use our A4 reference to calculate tonic frequency)
    double refPitch = referencePitch.load();
    // Calculate frequency of the tonic note (middle note)
    double tonicFreq = refPitch * std::pow(2.0, (middleNote - 69) / 12.0);
    content += juce::String(tonicFreq, 6) + "\n";

    // Octave degree (scale degree that represents the octave, typically last)
    content += juce::String(mapSize) + "\n";

    // Linear mapping: each scale degree maps to itself
    // For a 12-note scale: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
    for (int i = 0; i < mapSize; ++i)
    {
        content += juce::String(i) + "\n";
    }

    return content;
}
