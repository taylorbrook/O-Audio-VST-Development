/*
  ==============================================================================

    TuningEngine.cpp
    v1.6.0: Full Tuning Module Implementation

  ==============================================================================
*/

#include "TuningEngine.h"

TuningEngine::TuningEngine()
{
    // Initialize all pitch bends to sentinel value (no bend)
    for (auto& bend : notePitchBends)
        bend.store(NO_BEND, std::memory_order_relaxed);

    // Initialize 12-TET intervals (100 cents per semitone)
    scaleIntervals.reserve(13);
    scaleIntervals.push_back(0.0);    // Unison
    for (int i = 1; i <= 12; ++i)
        scaleIntervals.push_back(i * 100.0);

    // Build initial frequency table
    rebuildFrequencyTable();
}

// ═══════════════════════════════════════════════════════════════════
// Core Settings
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::setMasterTune(double freqHz)
{
    double newFreq = juce::jlimit(400.0, 480.0, freqHz);
    if (std::abs(newFreq - a4Frequency) > 0.01)
    {
        a4Frequency = newFreq;
        rebuildFrequencyTable();
    }
}

void TuningEngine::setPitchBendRange(float semitones)
{
    pitchBendRange = juce::jlimit(1.0f, 48.0f, semitones);
}

// ═══════════════════════════════════════════════════════════════════
// Tuning Mode
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::setMode(Mode mode)
{
    Mode oldMode = currentMode.load(std::memory_order_relaxed);
    if (oldMode != mode)
    {
        currentMode.store(mode, std::memory_order_relaxed);
        rebuildFrequencyTable();
    }
}

// ═══════════════════════════════════════════════════════════════════
// Custom Intervals
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::setCustomIntervals(const std::vector<double>& cents, const juce::String& name)
{
    {
        std::lock_guard<std::mutex> lock(intervalMutex);
        scaleIntervals = cents;

        // Ensure we have at least unison
        if (scaleIntervals.empty() || scaleIntervals[0] != 0.0)
            scaleIntervals.insert(scaleIntervals.begin(), 0.0);

        scaleDegrees = static_cast<int>(scaleIntervals.size()) - 1; // Exclude unison from count
        scaleName = name;
        scalaFileLoaded = true;
    }

    rebuildFrequencyTable();
}

std::vector<double> TuningEngine::getIntervals() const
{
    std::lock_guard<std::mutex> lock(intervalMutex);
    return scaleIntervals;
}

juce::String TuningEngine::getActiveTuningName() const
{
    Mode mode = currentMode.load(std::memory_order_relaxed);
    if (mode == Mode::TwelveTET)
        return "12-TET Standard";
    if (mode == Mode::MTSESP)
        return "MTS-ESP (Not Connected)";  // v1.7.2: Placeholder until MTS-ESP implemented
    return scaleName;
}

// ═══════════════════════════════════════════════════════════════════
// Tonic (Transposition)
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::setTonicNote(int tonicIndex)
{
    int newTonic = juce::jlimit(0, 11, tonicIndex);
    int oldTonic = tonicOffset.load(std::memory_order_relaxed);
    if (oldTonic != newTonic)
    {
        tonicOffset.store(newTonic, std::memory_order_relaxed);
        rebuildFrequencyTable();
    }
}

// ═══════════════════════════════════════════════════════════════════
// Scala File I/O
// ═══════════════════════════════════════════════════════════════════

double TuningEngine::parseScalaPitch(const juce::String& line) const
{
    juce::String trimmed = line.trim();
    if (trimmed.isEmpty())
        return -1.0;

    // Check if it's a ratio (contains /)
    if (trimmed.contains("/"))
    {
        // Parse as ratio n/d
        int slashPos = trimmed.indexOf("/");
        double numerator = trimmed.substring(0, slashPos).getDoubleValue();
        double denominator = trimmed.substring(slashPos + 1).getDoubleValue();
        if (denominator <= 0.0 || numerator <= 0.0)
            return -1.0;
        double ratio = numerator / denominator;
        // Convert ratio to cents: cents = 1200 * log2(ratio)
        return 1200.0 * std::log2(ratio);
    }
    else if (trimmed.contains("."))
    {
        // It's already in cents (contains decimal point)
        return trimmed.getDoubleValue();
    }
    else
    {
        // Integer ratio (e.g., "2" means 2/1)
        double ratio = trimmed.getDoubleValue();
        if (ratio <= 0.0)
            return -1.0;
        return 1200.0 * std::log2(ratio);
    }
}

bool TuningEngine::loadScalaFile(const juce::File& sclFile)
{
    if (!sclFile.existsAsFile())
    {
        DBG("TuningEngine::loadScalaFile() - File not found: " + sclFile.getFullPathName());
        return false;
    }

    juce::StringArray lines;
    sclFile.readLines(lines);

    std::vector<double> newIntervals;
    newIntervals.push_back(0.0); // Unison always first

    juce::String parsedName;
    int expectedDegrees = 0;
    int pitchLineCount = 0;
    bool foundDescription = false;
    bool foundDegreeCount = false;

    for (const auto& line : lines)
    {
        juce::String trimmed = line.trim();

        // Skip empty lines and comments
        if (trimmed.isEmpty())
            continue;
        if (trimmed.startsWith("!"))
            continue;

        // First non-comment line is the description
        if (!foundDescription)
        {
            parsedName = trimmed;
            foundDescription = true;
            continue;
        }

        // Second non-comment line is the number of degrees
        if (!foundDegreeCount)
        {
            expectedDegrees = trimmed.getIntValue();
            foundDegreeCount = true;
            continue;
        }

        // Parse pitch lines
        double cents = parseScalaPitch(trimmed);
        if (cents >= 0.0)
        {
            newIntervals.push_back(cents);
            pitchLineCount++;

            // Stop when we've read expected degrees
            if (pitchLineCount >= expectedDegrees)
                break;
        }
    }

    // Validate
    if (newIntervals.size() < 2)
    {
        DBG("TuningEngine::loadScalaFile() - Not enough pitch values in file");
        return false;
    }

    // Apply the new tuning
    setCustomIntervals(newIntervals, parsedName.isEmpty() ? sclFile.getFileNameWithoutExtension() : parsedName);
    DBG("TuningEngine::loadScalaFile() - Loaded '" + scaleName + "' with " + juce::String(scaleDegrees) + " degrees");
    return true;
}

bool TuningEngine::loadKBMFile(const juce::File& kbmFile)
{
    // Basic KBM support - just extract reference frequency if present
    if (!kbmFile.existsAsFile())
    {
        DBG("TuningEngine::loadKBMFile() - File not found");
        return false;
    }

    juce::StringArray lines;
    kbmFile.readLines(lines);

    int lineNum = 0;
    for (const auto& line : lines)
    {
        juce::String trimmed = line.trim();
        if (trimmed.isEmpty() || trimmed.startsWith("!"))
            continue;

        lineNum++;

        // Line 6 is the reference frequency
        if (lineNum == 6)
        {
            double refFreq = trimmed.getDoubleValue();
            if (refFreq > 0.0)
            {
                setMasterTune(refFreq);
                DBG("TuningEngine::loadKBMFile() - Set reference frequency to " + juce::String(refFreq) + " Hz");
            }
            break;
        }
    }

    return true;
}

juce::String TuningEngine::generateScalaFileContent() const
{
    juce::String content;

    // Comment header
    content += "! " + scaleName + ".scl\n";
    content += "!\n";

    // Description
    content += scaleName + "\n";

    // Number of degrees (excluding unison)
    std::lock_guard<std::mutex> lock(intervalMutex);
    content += juce::String(scaleDegrees) + "\n";

    // Pitch values (skip unison at index 0)
    for (size_t i = 1; i < scaleIntervals.size(); ++i)
    {
        content += juce::String(scaleIntervals[i], 6) + "\n";
    }

    return content;
}

juce::String TuningEngine::generateKBMFileContent() const
{
    juce::String content;

    // Basic 12-note KBM
    content += "! Keyboard mapping\n";
    content += "12\n";           // Size of map
    content += "0\n";            // First MIDI note number to retune
    content += "127\n";          // Last MIDI note number to retune
    content += "60\n";           // Middle note (C4)
    content += "69\n";           // Reference note (A4)
    content += juce::String(a4Frequency, 6) + "\n"; // Reference frequency
    content += "12\n";           // Scale degree for reference note

    // 12-note mapping
    for (int i = 0; i < 12; ++i)
        content += juce::String(i) + "\n";

    return content;
}

// Deprecated compatibility stub
bool TuningEngine::loadScalaFile(const juce::File& scl, const juce::File& kbm)
{
    juce::ignoreUnused(kbm);
    return loadScalaFile(scl);
}

bool TuningEngine::connectMTSClient()
{
    DBG("TuningEngine::connectMTSClient() - Not implemented");
    return false;
}

// ═══════════════════════════════════════════════════════════════════
// Frequency Calculation
// ═══════════════════════════════════════════════════════════════════

double TuningEngine::getFrequency(int midiNote, int midiChannel)
{
    juce::ignoreUnused(midiChannel);
    midiNote = juce::jlimit(0, 127, midiNote);

    // Get pre-computed base frequency from table (lock-free)
    double baseFreq = frequencyTable[static_cast<size_t>(midiNote)].load(std::memory_order_relaxed);

    // Apply per-note pitch bend if present
    float bendAmount = notePitchBends[static_cast<size_t>(midiNote)].load(std::memory_order_relaxed);
    if (bendAmount >= -1.0f && bendAmount <= 1.0f)
    {
        return applyPitchBend(baseFreq, bendAmount);
    }

    return baseFreq;
}

void TuningEngine::setPitchBend(int midiNote, float bendAmount)
{
    midiNote = juce::jlimit(0, 127, midiNote);
    bendAmount = juce::jlimit(-1.0f, 1.0f, bendAmount);
    notePitchBends[static_cast<size_t>(midiNote)].store(bendAmount, std::memory_order_relaxed);
}

void TuningEngine::clearPitchBend(int midiNote)
{
    midiNote = juce::jlimit(0, 127, midiNote);
    notePitchBends[static_cast<size_t>(midiNote)].store(NO_BEND, std::memory_order_relaxed);
}

void TuningEngine::clearAllPitchBends()
{
    for (auto& bend : notePitchBends)
        bend.store(NO_BEND, std::memory_order_relaxed);
}

std::vector<double> TuningEngine::getScaleFrequencies(int rootNote, int numNotes)
{
    std::vector<double> frequencies;
    frequencies.reserve(numNotes);

    for (int i = 0; i < numNotes; ++i)
    {
        int midiNote = rootNote + i;
        if (midiNote >= 0 && midiNote <= 127)
        {
            frequencies.push_back(frequencyTable[static_cast<size_t>(midiNote)].load(std::memory_order_relaxed));
        }
    }

    return frequencies;
}

// ═══════════════════════════════════════════════════════════════════
// Internal Methods
// ═══════════════════════════════════════════════════════════════════

double TuningEngine::calculate12TETFrequency(int midiNote) const
{
    // 12-TET formula: f = A4 * 2^((n - 69) / 12)
    const double semitonesFromA4 = static_cast<double>(midiNote - 69);
    return a4Frequency * std::pow(2.0, semitonesFromA4 / 12.0);
}

double TuningEngine::calculateCustomFrequency(int midiNote) const
{
    std::lock_guard<std::mutex> lock(intervalMutex);

    if (scaleIntervals.size() < 2)
        return calculate12TETFrequency(midiNote);

    // Apply tonic offset
    int tonic = tonicOffset.load(std::memory_order_relaxed);
    int adjustedNote = midiNote - tonic;

    // Get octave period (last interval in scale, typically 1200 cents for octave)
    double period = scaleIntervals.back();

    // Find which octave this note is in (relative to C0 = MIDI 0)
    int degreesPerPeriod = static_cast<int>(scaleIntervals.size()) - 1;
    int octave = adjustedNote / degreesPerPeriod;
    int degree = adjustedNote % degreesPerPeriod;

    // Handle negative notes
    if (adjustedNote < 0)
    {
        octave = (adjustedNote - degreesPerPeriod + 1) / degreesPerPeriod;
        degree = adjustedNote - (octave * degreesPerPeriod);
    }

    // Get cents offset for this degree
    double centsOffset = scaleIntervals[static_cast<size_t>(degree)];

    // Add octave transposition
    centsOffset += octave * period;

    // Calculate frequency relative to C0 (MIDI 0)
    // C0 in 12-TET: A4 (440) / 2^(69/12) = ~8.176 Hz
    double c0Freq = a4Frequency / std::pow(2.0, 69.0 / 12.0);

    // Apply tonic offset in cents (shift the whole scale)
    double tonicCents = tonic * 100.0; // 100 cents per semitone

    // Final frequency
    return c0Freq * std::pow(2.0, (centsOffset + tonicCents) / 1200.0);
}

double TuningEngine::applyPitchBend(double baseFreq, float bendAmount) const
{
    const double bendSemitones = static_cast<double>(bendAmount * pitchBendRange);
    return baseFreq * std::pow(2.0, bendSemitones / 12.0);
}

void TuningEngine::rebuildFrequencyTable()
{
    Mode mode = currentMode.load(std::memory_order_relaxed);

    for (int midiNote = 0; midiNote < 128; ++midiNote)
    {
        double freq;
        if (mode == Mode::TwelveTET)
        {
            freq = calculate12TETFrequency(midiNote);
        }
        else
        {
            freq = calculateCustomFrequency(midiNote);
        }
        frequencyTable[static_cast<size_t>(midiNote)].store(freq, std::memory_order_relaxed);
    }
}
