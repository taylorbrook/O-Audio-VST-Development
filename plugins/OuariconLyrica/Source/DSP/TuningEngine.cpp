/*
  ==============================================================================

    TuningEngine.cpp
    v1.8.0: Complete Scala KBM Support Implementation

  ==============================================================================
*/

#include "TuningEngine.h"

// ═══════════════════════════════════════════════════════════════════
// v1.9.0: Built-in Temperament Preset Data (cents from C for each note)
// ═══════════════════════════════════════════════════════════════════

static const std::array<double, 12> PRESET_EQUAL = {
    0.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0
};

static const std::array<double, 12> PRESET_PYTHAGOREAN = {
    0.0, 113.685, 203.91, 294.135, 407.82, 498.045, 611.73, 701.955, 815.64, 905.865, 996.09, 1109.775
};

static const std::array<double, 12> PRESET_ZARLINO = {
    0.0, 111.73, 203.91, 315.64, 386.31, 498.04, 582.51, 701.96, 813.69, 884.36, 1017.60, 1088.27
};

static const std::array<double, 12> PRESET_MEANTONE_QUARTER = {
    0.0, 76.05, 193.16, 310.26, 386.31, 503.42, 579.47, 696.58, 772.63, 889.74, 1006.84, 1082.89
};

static const std::array<double, 12> PRESET_WERCKMEISTER_III = {
    0.0, 90.225, 192.18, 294.135, 390.225, 498.045, 588.27, 696.09, 792.18, 888.27, 996.09, 1092.18
};

static const std::array<double, 12> PRESET_KIRNBERGER_III = {
    0.0, 90.18, 193.16, 294.13, 386.31, 498.04, 590.22, 696.58, 792.18, 889.74, 996.09, 1088.27
};

static const std::array<double, 12> PRESET_VALLOTTI = {
    0.0, 94.13, 196.09, 298.04, 392.18, 501.96, 592.18, 698.04, 796.09, 894.13, 1000.0, 1090.22
};

static const std::array<double, 12> PRESET_WELL_TEMPERED = {
    0.0, 94.135, 196.09, 298.045, 392.18, 500.0, 594.135, 698.045, 796.09, 894.135, 1000.0, 1092.18
};

static const std::array<double, 12> PRESET_JUST_INTONATION = {
    0.0, 111.73, 203.91, 315.64, 386.31, 498.04, 582.51, 701.96, 813.69, 884.36, 996.09, 1088.27
};

// Bohlen-Pierce uses 13-EDO over a 3:1 ratio (tritave), mapped to 12 notes
static const std::array<double, 12> PRESET_BOHLEN_PIERCE = {
    0.0, 146.3, 292.6, 438.9, 585.2, 731.5, 877.8, 1024.1, 1170.4, 1316.7, 1463.0, 1609.3
};

TuningEngine::TuningEngine()
{
    // Initialize all pitch bends to sentinel value (no bend)
    for (auto& bend : notePitchBends)
        bend.store(NO_BEND, std::memory_order_relaxed);

    // v1.12.0: Initialize 12-TET intervals WITH period (13 values: 0-1200)
    // This ensures scaleDegrees = 12 (size - 1), not 11
    scaleIntervals = {0.0, 100.0, 200.0, 300.0, 400.0, 500.0,
                      600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0};
    scaleDegrees = 12;  // Explicit, not calculated from size

    // v1.12.0: Initialize rotated intervals cache (same as scaleIntervals when tonic=0)
    rotatedIntervals = scaleIntervals;

    // Initialize default keyboard mapping (linear 12-note)
    resetKeyboardMapping();

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
// v1.9.0: Octave Stretch
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::setOctaveStretch(float stretch)
{
    float newStretch = juce::jlimit(0.95f, 1.25f, stretch);
    if (std::abs(newStretch - octaveStretch) > 0.001f)
    {
        octaveStretch = newStretch;
        rebuildFrequencyTable();
    }
}

// ═══════════════════════════════════════════════════════════════════
// v1.9.0: Built-in Temperament Presets
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::setBuiltInPreset(BuiltInPreset preset)
{
    currentPreset = preset;

    // Custom preset - don't change intervals, user has loaded a .scl file
    if (preset == BuiltInPreset::Custom)
        return;

    std::vector<double> intervals;
    juce::String name;

    switch (preset)
    {
        case BuiltInPreset::Equal12TET:
            intervals.assign(PRESET_EQUAL.begin(), PRESET_EQUAL.end());
            name = "Equal 12-TET";
            break;
        case BuiltInPreset::Pythagorean:
            intervals.assign(PRESET_PYTHAGOREAN.begin(), PRESET_PYTHAGOREAN.end());
            name = "Pythagorean";
            break;
        case BuiltInPreset::Zarlino:
            intervals.assign(PRESET_ZARLINO.begin(), PRESET_ZARLINO.end());
            name = "Zarlino (Just Major)";
            break;
        case BuiltInPreset::MeantoneQuarter:
            intervals.assign(PRESET_MEANTONE_QUARTER.begin(), PRESET_MEANTONE_QUARTER.end());
            name = "Meantone (1/4 comma)";
            break;
        case BuiltInPreset::WerckmeisterIII:
            intervals.assign(PRESET_WERCKMEISTER_III.begin(), PRESET_WERCKMEISTER_III.end());
            name = "Werckmeister III";
            break;
        case BuiltInPreset::KirnbergerIII:
            intervals.assign(PRESET_KIRNBERGER_III.begin(), PRESET_KIRNBERGER_III.end());
            name = "Kirnberger III";
            break;
        case BuiltInPreset::Vallotti:
            intervals.assign(PRESET_VALLOTTI.begin(), PRESET_VALLOTTI.end());
            name = "Vallotti";
            break;
        case BuiltInPreset::WellTempered:
            intervals.assign(PRESET_WELL_TEMPERED.begin(), PRESET_WELL_TEMPERED.end());
            name = "Well Tempered";
            break;
        case BuiltInPreset::JustIntonation:
            intervals.assign(PRESET_JUST_INTONATION.begin(), PRESET_JUST_INTONATION.end());
            name = "Just Intonation";
            break;
        case BuiltInPreset::BohlenPierce:
            intervals.assign(PRESET_BOHLEN_PIERCE.begin(), PRESET_BOHLEN_PIERCE.end());
            name = "Bohlen-Pierce";
            break;
        case BuiltInPreset::Custom:
            // Already handled above
            return;
    }

    // Add octave/period at the end (1200 cents for most, 1902 for Bohlen-Pierce)
    if (preset == BuiltInPreset::BohlenPierce)
        intervals.push_back(1902.0); // Tritave
    else
        intervals.push_back(1200.0); // Octave

    setCustomIntervals(intervals, name);

    // v1.12.0: Set appropriate mode AFTER setCustomIntervals
    // (setCustomIntervals no longer forces Scala mode)
    if (preset == BuiltInPreset::Equal12TET)
        currentMode.store(Mode::TwelveTET, std::memory_order_relaxed);
    else
        currentMode.store(Mode::Scala, std::memory_order_relaxed);

    DBG("TuningEngine::setBuiltInPreset() - Set to: " + name);
}

juce::String TuningEngine::getPresetName() const
{
    switch (currentPreset)
    {
        case BuiltInPreset::Equal12TET:     return "Equal 12-TET";
        case BuiltInPreset::Pythagorean:    return "Pythagorean";
        case BuiltInPreset::Zarlino:        return "Zarlino";
        case BuiltInPreset::MeantoneQuarter: return "Meantone (1/4)";
        case BuiltInPreset::WerckmeisterIII: return "Werckmeister III";
        case BuiltInPreset::KirnbergerIII:  return "Kirnberger III";
        case BuiltInPreset::Vallotti:       return "Vallotti";
        case BuiltInPreset::WellTempered:   return "Well Tempered";
        case BuiltInPreset::JustIntonation: return "Just Intonation";
        case BuiltInPreset::BohlenPierce:   return "Bohlen-Pierce";
        case BuiltInPreset::Custom:         return scaleName;
    }
    return scaleName;
}

// ═══════════════════════════════════════════════════════════════════
// Tuning Mode
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::setMode(Mode mode)
{
    Mode oldMode = currentMode.load(std::memory_order_relaxed);
    if (oldMode != mode)
    {
        // v1.11.1: If switching to Scala mode with empty intervals, initialize to 12-TET
        // This ensures users can edit intervals immediately after clicking "Custom"
        if (mode == Mode::Scala)
        {
            std::lock_guard<std::mutex> lock(intervalMutex);
            if (scaleIntervals.size() < 2)
            {
                scaleIntervals = {0.0, 100.0, 200.0, 300.0, 400.0, 500.0,
                                  600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0};
                scaleDegrees = 12;
                scaleName = "Custom";
                scalaFileLoaded = true;
            }
        }

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

        scaleDegrees = static_cast<int>(scaleIntervals.size()) - 1; // Exclude period from count
        scaleName = name;
        scalaFileLoaded = true;

        // v1.12.0: Initialize rotated intervals cache
        rotatedIntervals = scaleIntervals;
    }

    // v1.12.0: Recalculate rotated intervals for current tonic
    int currentTonic = tonicOffset.load(std::memory_order_relaxed);
    if (currentTonic != 0)
        rotateIntervalsForTonic(currentTonic);

    // v1.12.0: Don't set mode here - let caller handle mode switching
    // This allows setBuiltInPreset to set TwelveTET mode for Equal12TET preset
    // Always rebuild frequency table when intervals change
    rebuildFrequencyTable();
}

void TuningEngine::setSingleInterval(int index, double cents)
{
    {
        std::lock_guard<std::mutex> lock(intervalMutex);

        // v1.11.1: Initialize to 12-TET if intervals are empty (fresh load → Custom case)
        if (scaleIntervals.size() < 2)
        {
            scaleIntervals = {0.0, 100.0, 200.0, 300.0, 400.0, 500.0,
                              600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0};
            scaleDegrees = 12;
            scaleName = "Custom";
            scalaFileLoaded = true;
        }

        // Update the interval at the specified index
        if (index >= 0 && index < static_cast<int>(scaleIntervals.size()))
        {
            scaleIntervals[static_cast<size_t>(index)] = cents;
        }
    }

    // v1.11.2: Update rotated intervals cache when tonic != 0
    // Without this, edits to scaleIntervals are ignored because
    // calculateCustomFrequency() uses rotatedIntervals when tonic is set
    int currentTonic = tonicOffset.load(std::memory_order_relaxed);
    if (currentTonic != 0)
        rotateIntervalsForTonic(currentTonic);
    else
    {
        // When tonic is 0, keep rotatedIntervals in sync with scaleIntervals
        std::lock_guard<std::mutex> lock(intervalMutex);
        rotatedIntervals = scaleIntervals;
    }

    // Switch to Scala mode and rebuild
    currentMode.store(Mode::Scala, std::memory_order_relaxed);
    rebuildFrequencyTable();

    DBG("TuningEngine::setSingleInterval() - Set index " + juce::String(index) + " to " + juce::String(cents) + " cents");
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
// Tonic (Modal Rotation) - v1.12.0
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::rotateIntervalsForTonic(int tonic)
{
    std::lock_guard<std::mutex> lock(intervalMutex);

    if (scaleIntervals.size() < 2 || tonic == 0)
    {
        // No rotation needed - use original intervals
        rotatedIntervals = scaleIntervals;
        return;
    }

    int scaleSize = static_cast<int>(scaleIntervals.size()) - 1;  // Exclude period
    double period = scaleIntervals.back();  // Usually 1200 cents

    // Ensure tonic is in valid range
    tonic = tonic % scaleSize;
    if (tonic < 0) tonic += scaleSize;

    rotatedIntervals.clear();
    rotatedIntervals.reserve(scaleIntervals.size());

    // Start from 0 (the new tonic)
    rotatedIntervals.push_back(0.0);

    // Get the offset to subtract (cents value at tonic position in original scale)
    double tonicCentsOffset = scaleIntervals[static_cast<size_t>(tonic)];

    // Rotate: take intervals starting from tonic, wrapping around
    for (int i = 1; i < scaleSize; ++i)
    {
        int sourceIdx = (tonic + i) % scaleSize;
        double sourceCents = scaleIntervals[static_cast<size_t>(sourceIdx)];

        // Adjust for wrap-around
        if (sourceIdx < tonic)
            sourceCents += period;

        // Subtract tonic offset to make new tonic = 0
        double rotatedCents = sourceCents - tonicCentsOffset;
        rotatedIntervals.push_back(rotatedCents);
    }

    // Add the period
    rotatedIntervals.push_back(period);

    DBG("TuningEngine::rotateIntervalsForTonic() - Rotated to tonic " + juce::String(tonic)
        + ", first interval: " + juce::String(rotatedIntervals.size() > 1 ? rotatedIntervals[1] : 0.0, 2) + "¢");
}

void TuningEngine::setTonicNote(int tonicIndex)
{
    int newTonic = juce::jlimit(0, 11, tonicIndex);
    int oldTonic = tonicOffset.load(std::memory_order_relaxed);
    if (oldTonic != newTonic)
    {
        tonicOffset.store(newTonic, std::memory_order_relaxed);

        // v1.12.0: Rotate intervals for modal rotation
        rotateIntervalsForTonic(newTonic);

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

    // v1.7.4: Automatically switch to Scala mode when loading a .scl file
    // This ensures rebuildFrequencyTable() uses custom intervals instead of 12-TET
    setMode(Mode::Scala);

    // v1.9.0: Mark as custom preset when loading external file
    currentPreset = BuiltInPreset::Custom;

    DBG("TuningEngine::loadScalaFile() - Loaded '" + scaleName + "' with " + juce::String(scaleDegrees) + " degrees");
    return true;
}

bool TuningEngine::loadKBMFile(const juce::File& kbmFile)
{
    if (!kbmFile.existsAsFile())
    {
        DBG("TuningEngine::loadKBMFile() - File not found: " + kbmFile.getFullPathName());
        return false;
    }

    juce::StringArray lines;
    kbmFile.readLines(lines);

    // Collect non-comment, non-empty lines
    juce::StringArray dataLines;
    for (const auto& line : lines)
    {
        juce::String trimmed = line.trim();
        if (trimmed.isEmpty() || trimmed.startsWith("!"))
            continue;
        dataLines.add(trimmed);
    }

    // KBM requires at least 7 header lines
    if (dataLines.size() < 7)
    {
        DBG("TuningEngine::loadKBMFile() - Not enough data lines (need at least 7)");
        return false;
    }

    // Parse header lines
    int newMapSize = dataLines[0].getIntValue();         // Line 1: Size of map
    int newFirstNote = dataLines[1].getIntValue();       // Line 2: First MIDI note to retune
    int newLastNote = dataLines[2].getIntValue();        // Line 3: Last MIDI note to retune
    int newMiddleNote = dataLines[3].getIntValue();      // Line 4: Middle note (degree 0)
    int newReferenceNote = dataLines[4].getIntValue();   // Line 5: Reference note
    double newRefFreq = dataLines[5].getDoubleValue();   // Line 6: Reference frequency
    int newOctaveDegree = dataLines[6].getIntValue();    // Line 7: Octave degree

    // Validate ranges
    newFirstNote = juce::jlimit(0, 127, newFirstNote);
    newLastNote = juce::jlimit(0, 127, newLastNote);
    newMiddleNote = juce::jlimit(0, 127, newMiddleNote);
    newReferenceNote = juce::jlimit(0, 127, newReferenceNote);

    // Parse mapping entries (lines 8+)
    std::vector<int> newMapping;
    int mappingCount = (newMapSize > 0) ? newMapSize : 12;  // Default to 12 if mapSize is 0

    for (int i = 7; i < dataLines.size() && newMapping.size() < static_cast<size_t>(mappingCount); ++i)
    {
        juce::String entry = dataLines[i].trim().toLowerCase();

        if (entry == "x" || entry == "X")
        {
            // Unmapped key
            newMapping.push_back(-1);
        }
        else
        {
            // Scale degree
            int degree = entry.getIntValue();
            newMapping.push_back(degree);
        }
    }

    // If we didn't get enough mapping entries, fill with linear mapping
    while (newMapping.size() < static_cast<size_t>(mappingCount))
    {
        newMapping.push_back(static_cast<int>(newMapping.size()));
    }

    // Apply the new mapping
    {
        std::lock_guard<std::mutex> lock(intervalMutex);
        kbmMapSize = newMapSize;
        kbmFirstNote = newFirstNote;
        kbmLastNote = newLastNote;
        kbmMiddleNote = newMiddleNote;
        kbmReferenceNote = newReferenceNote;
        kbmOctaveDegree = (newOctaveDegree > 0) ? newOctaveDegree : static_cast<int>(scaleIntervals.size()) - 1;
        kbmMapping = newMapping;
        kbmLoaded = true;
    }

    // Set reference frequency (validated in setMasterTune)
    if (newRefFreq > 0.0)
    {
        setMasterTune(newRefFreq);
    }

    // Rebuild frequency table with new mapping
    rebuildFrequencyTable();

    DBG("TuningEngine::loadKBMFile() - Loaded KBM: mapSize=" + juce::String(kbmMapSize)
        + ", range=" + juce::String(kbmFirstNote) + "-" + juce::String(kbmLastNote)
        + ", middle=" + juce::String(kbmMiddleNote)
        + ", refNote=" + juce::String(kbmReferenceNote)
        + ", refFreq=" + juce::String(a4Frequency, 2) + "Hz"
        + ", octaveDegree=" + juce::String(kbmOctaveDegree)
        + ", mappingSize=" + juce::String(static_cast<int>(kbmMapping.size())));

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
    std::lock_guard<std::mutex> lock(intervalMutex);

    juce::String content;

    // Comment header
    content += "! Keyboard mapping for " + scaleName + "\n";
    content += "! Generated by OuariconLyrica\n";

    // Line 1: Size of map (0 = linear mapping, N = repeating pattern)
    int mapSize = kbmLoaded ? kbmMapSize : static_cast<int>(kbmMapping.size());
    content += juce::String(mapSize) + "\n";

    // Line 2: First MIDI note to retune
    content += juce::String(kbmFirstNote) + "\n";

    // Line 3: Last MIDI note to retune
    content += juce::String(kbmLastNote) + "\n";

    // Line 4: Middle note (MIDI note for scale degree 0)
    content += juce::String(kbmMiddleNote) + "\n";

    // Line 5: Reference note
    content += juce::String(kbmReferenceNote) + "\n";

    // Line 6: Reference frequency
    content += juce::String(a4Frequency, 6) + "\n";

    // Line 7: Octave degree (scale degree = formal octave)
    int octDegree = kbmLoaded ? kbmOctaveDegree : (static_cast<int>(scaleIntervals.size()) - 1);
    content += juce::String(octDegree) + "\n";

    // Mapping entries
    if (kbmMapping.empty())
    {
        // Default linear mapping
        for (int i = 0; i < mapSize; ++i)
            content += juce::String(i) + "\n";
    }
    else
    {
        for (int degree : kbmMapping)
        {
            if (degree < 0)
                content += "x\n";  // Unmapped key
            else
                content += juce::String(degree) + "\n";
        }
    }

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

bool TuningEngine::isNoteMapped(int midiNote) const
{
    midiNote = juce::jlimit(0, 127, midiNote);

    // If no KBM loaded, all notes are mapped
    if (!kbmLoaded || kbmMapping.empty())
        return true;

    // Check retune range
    if (midiNote < kbmFirstNote || midiNote > kbmLastNote)
        return true;  // Outside range uses 12-TET, so considered "mapped"

    // Calculate position in mapping pattern
    int mapSize = kbmMapSize > 0 ? kbmMapSize : static_cast<int>(kbmMapping.size());
    int noteOffset = midiNote - kbmMiddleNote;
    int positionInPattern = noteOffset % mapSize;

    // Handle negative positions
    if (positionInPattern < 0)
        positionInPattern += mapSize;

    // Check if this position is mapped
    if (positionInPattern >= 0 && positionInPattern < static_cast<int>(kbmMapping.size()))
    {
        return kbmMapping[static_cast<size_t>(positionInPattern)] >= 0;
    }

    return true;  // Default to mapped
}

void TuningEngine::resetKeyboardMapping()
{
    std::lock_guard<std::mutex> lock(intervalMutex);

    kbmMapSize = 12;
    kbmFirstNote = 0;
    kbmLastNote = 127;
    kbmMiddleNote = 60;
    kbmReferenceNote = 69;
    kbmOctaveDegree = 12;

    // Default linear 12-note mapping
    kbmMapping.clear();
    kbmMapping.reserve(12);
    for (int i = 0; i < 12; ++i)
        kbmMapping.push_back(i);

    kbmLoaded = false;
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
    // v1.9.0: Apply octave stretch for physical modeling
    // Stretch > 1.0 = wider octaves in upper register (piano-like)
    // Stretch < 1.0 = narrower octaves
    const double semitonesFromA4 = static_cast<double>(midiNote - 69);
    const double stretchedSemitones = semitonesFromA4 * static_cast<double>(octaveStretch);
    return a4Frequency * std::pow(2.0, stretchedSemitones / 12.0);
}

double TuningEngine::calculateCustomFrequency(int midiNote) const
{
    std::lock_guard<std::mutex> lock(intervalMutex);

    if (scaleIntervals.size() < 2)
        return calculate12TETFrequency(midiNote);

    // Check if note is in the retune range (KBM lines 2-3)
    if (kbmLoaded && (midiNote < kbmFirstNote || midiNote > kbmLastNote))
    {
        return calculate12TETFrequency(midiNote);
    }

    // v1.12.0: Use rotated intervals when tonic != 0 for true modal rotation
    int tonic = tonicOffset.load(std::memory_order_relaxed);
    const auto& activeIntervals = (tonic == 0 || rotatedIntervals.empty()) ? scaleIntervals : rotatedIntervals;

    // Get octave period (last interval in scale, typically 1200 cents for octave)
    double period = activeIntervals.back();

    // Get the number of scale degrees (excluding the period)
    int scaleSize = static_cast<int>(activeIntervals.size()) - 1;

    // Determine octave degree for period calculations
    int octaveDegree = kbmLoaded ? kbmOctaveDegree : scaleSize;
    if (octaveDegree <= 0) octaveDegree = scaleSize;

    // Calculate scale degree from MIDI note using keyboard mapping
    int scaleDegree;
    int octaveNumber;

    if (kbmLoaded && !kbmMapping.empty())
    {
        // Full KBM mapping mode
        int mapSize = kbmMapSize > 0 ? kbmMapSize : static_cast<int>(kbmMapping.size());

        // Position relative to middle note
        int noteOffset = midiNote - kbmMiddleNote;

        // Calculate which "octave" of the mapping pattern we're in
        int patternOctave = noteOffset >= 0 ? noteOffset / mapSize : (noteOffset - mapSize + 1) / mapSize;
        int positionInPattern = noteOffset - (patternOctave * mapSize);

        // Handle negative positions
        if (positionInPattern < 0)
        {
            positionInPattern += mapSize;
            patternOctave--;
        }

        // Look up scale degree from mapping
        if (positionInPattern >= 0 && positionInPattern < static_cast<int>(kbmMapping.size()))
        {
            int mappedDegree = kbmMapping[static_cast<size_t>(positionInPattern)];

            // Check for unmapped key
            if (mappedDegree < 0)
            {
                return calculate12TETFrequency(midiNote);
            }

            scaleDegree = mappedDegree;
        }
        else
        {
            // Fallback: linear mapping
            scaleDegree = positionInPattern % scaleSize;
        }

        // Total scale degree including octave transposition
        octaveNumber = patternOctave;
    }
    else
    {
        // v1.12.0: Simple linear mapping (no KBM file)
        // Since intervals are pre-rotated, we map MIDI notes directly relative to tonic
        // MIDI note = tonic becomes scale degree 0
        int adjustedNote = midiNote - tonic;

        octaveNumber = adjustedNote >= 0 ? adjustedNote / scaleSize : (adjustedNote - scaleSize + 1) / scaleSize;
        scaleDegree = adjustedNote - (octaveNumber * scaleSize);

        // Handle negative
        if (scaleDegree < 0)
        {
            scaleDegree += scaleSize;
            octaveNumber--;
        }
    }

    // Ensure scaleDegree is in valid range
    scaleDegree = juce::jlimit(0, scaleSize, scaleDegree);

    // v1.12.0: Get cents offset from rotated/active intervals
    double centsOffset = activeIntervals[static_cast<size_t>(scaleDegree)];

    // Add octave transposition
    centsOffset += octaveNumber * period;

    // Calculate reference frequency
    // KBM: referenceNote at referenceFrequency
    // Default: MIDI 69 (A4) at a4Frequency
    double refFreq = a4Frequency;
    int refNote = kbmLoaded ? kbmReferenceNote : 69;

    // Calculate the cents offset of the reference note in our scale
    double refCentsFromC0;
    if (kbmLoaded && !kbmMapping.empty())
    {
        int mapSize = kbmMapSize > 0 ? kbmMapSize : static_cast<int>(kbmMapping.size());
        int refOffset = refNote - kbmMiddleNote;
        int refPatternOctave = refOffset >= 0 ? refOffset / mapSize : (refOffset - mapSize + 1) / mapSize;
        int refPosInPattern = refOffset - (refPatternOctave * mapSize);
        if (refPosInPattern < 0) {
            refPosInPattern += mapSize;
            refPatternOctave--;
        }

        int refDegree = 0;
        if (refPosInPattern >= 0 && refPosInPattern < static_cast<int>(kbmMapping.size()))
        {
            int mapped = kbmMapping[static_cast<size_t>(refPosInPattern)];
            if (mapped >= 0) refDegree = mapped;
        }
        refDegree = juce::jlimit(0, scaleSize, refDegree);
        refCentsFromC0 = activeIntervals[static_cast<size_t>(refDegree)] + refPatternOctave * period;
    }
    else
    {
        // v1.12.0: Calculate reference note position using rotated intervals
        // Reference note position relative to tonic
        int refAdjusted = refNote - tonic;
        int refOctaveNum = refAdjusted >= 0 ? refAdjusted / scaleSize : (refAdjusted - scaleSize + 1) / scaleSize;
        int refScaleDegree = refAdjusted - (refOctaveNum * scaleSize);
        if (refScaleDegree < 0)
        {
            refScaleDegree += scaleSize;
            refOctaveNum--;
        }
        refScaleDegree = juce::jlimit(0, scaleSize, refScaleDegree);
        refCentsFromC0 = activeIntervals[static_cast<size_t>(refScaleDegree)] + refOctaveNum * period;
    }

    // Current note's cents from tonic origin
    double noteCentsFromTonic = centsOffset;

    // Calculate frequency relative to reference
    double centsFromRef = noteCentsFromTonic - refCentsFromC0;

    // v1.9.0: Apply octave stretch
    // For custom tunings, stretch the cents offset from reference
    double stretchedCents = centsFromRef * static_cast<double>(octaveStretch);

    return refFreq * std::pow(2.0, stretchedCents / 1200.0);
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
