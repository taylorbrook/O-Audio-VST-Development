/*
  ==============================================================================

    TuningEngine.cpp
    scala-tuning-engine module v2.0.0

  ==============================================================================
*/

#include "TuningEngine.h"

// ═══════════════════════════════════════════════════════════════════
// Built-in Temperament Preset Data (cents from C for each note)
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

    // Initialize 12-TET intervals WITH period (13 values: 0-1200)
    scaleIntervals = {0.0, 100.0, 200.0, 300.0, 400.0, 500.0,
                      600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0};
    scaleDegrees = 12;

    // Initialize rotated intervals cache
    rotatedIntervals = scaleIntervals;

    // Publish initial snapshot for lock-free reads
    publishIntervalsSnapshot();

    // Initialize default keyboard mapping
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
// Octave Stretch
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
// Built-in Temperament Presets
// ═══════════════════════════════════════════════════════════════════

// Lookup table for built-in preset data
struct PresetEntry
{
    const double* data;
    size_t size;
    const char* name;
    double period;  // 1200.0 for octave-based, 1902.0 for tritave (Bohlen-Pierce)
};

static const std::array<PresetEntry, 10> PRESET_TABLE = {{
    { PRESET_EQUAL.data(),              PRESET_EQUAL.size(),              "Equal 12-TET",         1200.0 },
    { PRESET_PYTHAGOREAN.data(),        PRESET_PYTHAGOREAN.size(),        "Pythagorean",          1200.0 },
    { PRESET_ZARLINO.data(),            PRESET_ZARLINO.size(),            "Zarlino (Just Major)",  1200.0 },
    { PRESET_MEANTONE_QUARTER.data(),   PRESET_MEANTONE_QUARTER.size(),   "Meantone (1/4 comma)", 1200.0 },
    { PRESET_WERCKMEISTER_III.data(),   PRESET_WERCKMEISTER_III.size(),   "Werckmeister III",     1200.0 },
    { PRESET_KIRNBERGER_III.data(),     PRESET_KIRNBERGER_III.size(),     "Kirnberger III",       1200.0 },
    { PRESET_VALLOTTI.data(),           PRESET_VALLOTTI.size(),           "Vallotti",             1200.0 },
    { PRESET_WELL_TEMPERED.data(),      PRESET_WELL_TEMPERED.size(),      "Well Tempered",        1200.0 },
    { PRESET_JUST_INTONATION.data(),    PRESET_JUST_INTONATION.size(),    "Just Intonation",      1200.0 },
    { PRESET_BOHLEN_PIERCE.data(),      PRESET_BOHLEN_PIERCE.size(),      "Bohlen-Pierce",        1902.0 },
}};

void TuningEngine::setBuiltInPreset(BuiltInPreset preset)
{
    currentPreset = preset;

    if (preset == BuiltInPreset::Custom)
        return;

    auto idx = static_cast<size_t>(preset);
    jassert(idx < PRESET_TABLE.size());
    const auto& entry = PRESET_TABLE[idx];

    std::vector<double> intervals(entry.data, entry.data + entry.size);
    intervals.push_back(entry.period);

    if (preset == BuiltInPreset::Equal12TET)
        currentMode.store(Mode::TwelveTET, std::memory_order_relaxed);
    else
        currentMode.store(Mode::Scala, std::memory_order_relaxed);

    setCustomIntervals(intervals, entry.name);

    DBG("TuningEngine: preset -> " + juce::String(entry.name));
}

// ═══════════════════════════════════════════════════════════════════
// Tuning Mode
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::ensureIntervalsInitialized()
{
    scaleIntervals = {0.0, 100.0, 200.0, 300.0, 400.0, 500.0,
                      600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0};
    scaleDegrees = 12;
    scaleName = "Custom";
    publishIntervalsSnapshot();
}

void TuningEngine::setMode(Mode mode)
{
    Mode oldMode = currentMode.load(std::memory_order_relaxed);
    if (oldMode != mode)
    {
        if (mode == Mode::Scala)
        {
            std::lock_guard<std::mutex> lock(intervalMutex);
            if (scaleIntervals.size() < 2)
                ensureIntervalsInitialized();
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

        // Initialize rotated intervals cache
        rotatedIntervals = scaleIntervals;

        publishIntervalsSnapshot();
    }

    // Recalculate rotated intervals for current tonic
    int currentTonic = tonicOffset.load(std::memory_order_relaxed);
    if (currentTonic != 0)
        rotateIntervalsForTonic(currentTonic);

    rebuildFrequencyTable();
}

void TuningEngine::setSingleInterval(int index, double cents)
{
    {
        std::lock_guard<std::mutex> lock(intervalMutex);

        if (scaleIntervals.size() < 2 || scaleIntervals.size() == 12)
            ensureIntervalsInitialized();

        // Update the interval at the specified index
        if (index >= 0 && index < static_cast<int>(scaleIntervals.size()))
        {
            scaleIntervals[static_cast<size_t>(index)] = cents;
        }

        publishIntervalsSnapshot();
    }

    // Update rotated intervals cache when tonic != 0
    int currentTonic = tonicOffset.load(std::memory_order_relaxed);
    if (currentTonic != 0)
    {
        rotateIntervalsForTonic(currentTonic);
    }
    else
    {
        std::lock_guard<std::mutex> lock(intervalMutex);
        rotatedIntervals = scaleIntervals;
    }

    // Switch to Scala mode and rebuild
    currentMode.store(Mode::Scala, std::memory_order_relaxed);
    rebuildFrequencyTable();
}

std::vector<double> TuningEngine::getIntervals() const
{
    auto snapshot = std::atomic_load(&intervalsSnapshot_);
    if (snapshot)
        return *snapshot;
    return {};
}

juce::String TuningEngine::getActiveTuningName() const
{
    Mode mode = currentMode.load(std::memory_order_relaxed);
    if (mode == Mode::TwelveTET)
        return "12-TET Standard";
    return scaleName;
}

// ═══════════════════════════════════════════════════════════════════
// Tonic (Modal Rotation)
// ═══════════════════════════════════════════════════════════════════

void TuningEngine::rotateIntervalsForTonic(int tonic)
{
    std::lock_guard<std::mutex> lock(intervalMutex);

    if (scaleIntervals.size() < 2 || tonic == 0)
    {
        rotatedIntervals = scaleIntervals;
        return;
    }

    int scaleSize = static_cast<int>(scaleIntervals.size()) - 1;
    double period = scaleIntervals.back();

    // Ensure tonic is in valid range
    tonic = tonic % scaleSize;
    if (tonic < 0) tonic += scaleSize;

    rotatedIntervals.clear();
    rotatedIntervals.reserve(scaleIntervals.size());

    // Start from 0 (the new tonic)
    rotatedIntervals.push_back(0.0);

    // Get the offset to subtract
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
}

void TuningEngine::setTonicNote(int tonicIndex)
{
    int newTonic = juce::jlimit(0, 11, tonicIndex);
    int oldTonic = tonicOffset.load(std::memory_order_relaxed);
    if (oldTonic != newTonic)
    {
        tonicOffset.store(newTonic, std::memory_order_relaxed);
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
        int slashPos = trimmed.indexOf("/");
        double numerator = trimmed.substring(0, slashPos).getDoubleValue();
        double denominator = trimmed.substring(slashPos + 1).getDoubleValue();
        if (denominator <= 0.0 || numerator <= 0.0)
            return -1.0;
        double ratio = numerator / denominator;
        return 1200.0 * std::log2(ratio);
    }
    else if (trimmed.contains("."))
    {
        // It's already in cents
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
        DBG("TuningEngine: .scl not found: " + sclFile.getFullPathName());
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

            if (pitchLineCount >= expectedDegrees)
                break;
        }
    }

    // Validate
    if (newIntervals.size() < 2)
    {
        DBG("TuningEngine: .scl has insufficient pitch values");
        return false;
    }

    // Apply the new tuning
    setCustomIntervals(newIntervals, parsedName.isEmpty() ? sclFile.getFileNameWithoutExtension() : parsedName);

    // Automatically switch to Scala mode
    setMode(Mode::Scala);

    // Mark as custom preset
    currentPreset = BuiltInPreset::Custom;

    DBG("TuningEngine: loaded .scl '" + scaleName + "' (" + juce::String(scaleDegrees) + " degrees)");
    return true;
}

bool TuningEngine::loadKBMFile(const juce::File& kbmFile)
{
    if (!kbmFile.existsAsFile())
    {
        DBG("TuningEngine: .kbm not found: " + kbmFile.getFullPathName());
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
        DBG("TuningEngine: .kbm has insufficient data lines");
        return false;
    }

    // Parse header lines
    int newMapSize = dataLines[0].getIntValue();
    int newFirstNote = dataLines[1].getIntValue();
    int newLastNote = dataLines[2].getIntValue();
    int newMiddleNote = dataLines[3].getIntValue();
    int newReferenceNote = dataLines[4].getIntValue();
    double newRefFreq = dataLines[5].getDoubleValue();
    int newOctaveDegree = dataLines[6].getIntValue();

    // Validate ranges
    newFirstNote = juce::jlimit(0, 127, newFirstNote);
    newLastNote = juce::jlimit(0, 127, newLastNote);
    newMiddleNote = juce::jlimit(0, 127, newMiddleNote);
    newReferenceNote = juce::jlimit(0, 127, newReferenceNote);

    // Parse mapping entries
    std::vector<int> newMapping;
    int mappingCount = (newMapSize > 0) ? newMapSize : 12;

    for (int i = 7; i < dataLines.size() && newMapping.size() < static_cast<size_t>(mappingCount); ++i)
    {
        juce::String entry = dataLines[i].trim().toLowerCase();

        if (entry == "x")
            newMapping.push_back(-1);
        else
            newMapping.push_back(entry.getIntValue());
    }

    // Fill with linear mapping if needed
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

    // Set reference frequency
    if (newRefFreq > 0.0)
    {
        setMasterTune(newRefFreq);
    }

    rebuildFrequencyTable();

    DBG("TuningEngine: loaded .kbm (mapSize=" + juce::String(kbmMapSize) + ")");
    return true;
}

juce::String TuningEngine::generateScalaFileContent() const
{
    juce::String content;

    content += "! " + scaleName + ".scl\n";
    content += "!\n";
    content += scaleName + "\n";

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

    content += "! Keyboard mapping for " + scaleName + "\n";
    content += "! Generated by scala-tuning-engine module\n";

    int mapSize = kbmLoaded ? kbmMapSize : static_cast<int>(kbmMapping.size());
    content += juce::String(mapSize) + "\n";
    content += juce::String(kbmFirstNote) + "\n";
    content += juce::String(kbmLastNote) + "\n";
    content += juce::String(kbmMiddleNote) + "\n";
    content += juce::String(kbmReferenceNote) + "\n";
    content += juce::String(a4Frequency, 6) + "\n";

    int octDegree = kbmLoaded ? kbmOctaveDegree : (static_cast<int>(scaleIntervals.size()) - 1);
    content += juce::String(octDegree) + "\n";

    if (kbmMapping.empty())
    {
        for (int i = 0; i < mapSize; ++i)
            content += juce::String(i) + "\n";
    }
    else
    {
        for (int degree : kbmMapping)
        {
            if (degree < 0)
                content += "x\n";
            else
                content += juce::String(degree) + "\n";
        }
    }

    return content;
}


bool TuningEngine::isNoteMapped(int midiNote) const
{
    midiNote = juce::jlimit(0, 127, midiNote);

    if (!kbmLoaded || kbmMapping.empty())
        return true;

    if (midiNote < kbmFirstNote || midiNote > kbmLastNote)
        return true;

    int mapSize = kbmMapSize > 0 ? kbmMapSize : static_cast<int>(kbmMapping.size());
    int noteOffset = midiNote - kbmMiddleNote;
    int positionInPattern = noteOffset % mapSize;

    if (positionInPattern < 0)
        positionInPattern += mapSize;

    if (positionInPattern >= 0 && positionInPattern < static_cast<int>(kbmMapping.size()))
    {
        return kbmMapping[static_cast<size_t>(positionInPattern)] >= 0;
    }

    return true;
}

void TuningEngine::resetKeyboardMapping()
{
    std::lock_guard<std::mutex> lock(intervalMutex);

    int mapSize = (scaleDegrees > 0) ? scaleDegrees : 12;

    kbmMapSize = mapSize;
    kbmFirstNote = 0;
    kbmLastNote = 127;
    kbmMiddleNote = 60;
    kbmReferenceNote = 69;
    kbmOctaveDegree = mapSize;

    kbmMapping.clear();
    kbmMapping.reserve(static_cast<size_t>(mapSize));
    for (int i = 0; i < mapSize; ++i)
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

    double baseFreq = frequencyTable[static_cast<size_t>(midiNote)].load(std::memory_order_relaxed);

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


// ═══════════════════════════════════════════════════════════════════
// Internal Methods
// ═══════════════════════════════════════════════════════════════════

double TuningEngine::calculate12TETFrequency(int midiNote) const
{
    const double semitonesFromA4 = static_cast<double>(midiNote - 69);
    const double stretchedSemitones = semitonesFromA4 * static_cast<double>(octaveStretch);
    return a4Frequency * std::pow(2.0, stretchedSemitones / 12.0);
}

double TuningEngine::calculateCustomFrequencyUnlocked(int midiNote) const
{
    if (scaleIntervals.size() < 2)
        return calculate12TETFrequency(midiNote);

    // Check if note is in the retune range
    if (kbmLoaded && (midiNote < kbmFirstNote || midiNote > kbmLastNote))
    {
        return calculate12TETFrequency(midiNote);
    }

    // For 12-note scales without KBM: pitch-class-based approach
    // Matches original TuningSystem behavior — 12-TET base * centsToRatio(interval)
    // This produces the plugin's characteristic wider-than-standard intervals
    int tonic = tonicOffset.load(std::memory_order_relaxed);
    if (scaleDegrees == 12 && !kbmLoaded)
    {
        double baseFreq = calculate12TETFrequency(midiNote);
        int pitchClass = midiNote % 12;
        int relativePitch = (pitchClass - tonic + 12) % 12;
        double intervalCents = scaleIntervals[static_cast<size_t>(relativePitch)];
        return baseFreq * std::pow(2.0, intervalCents / 1200.0);
    }

    // Non-12-note scales and KBM: use linear/KBM mapping
    const auto& activeIntervals = (tonic == 0 || rotatedIntervals.empty()) ? scaleIntervals : rotatedIntervals;

    double period = activeIntervals.back();
    int scaleSize = static_cast<int>(activeIntervals.size()) - 1;
    if (scaleSize <= 0) scaleSize = 12;

    int scaleDegree;
    int octaveNumber;

    if (kbmLoaded && !kbmMapping.empty())
    {
        // Full KBM mapping mode
        int mapSize = kbmMapSize > 0 ? kbmMapSize : static_cast<int>(kbmMapping.size());

        int noteOffset = midiNote - kbmMiddleNote;
        int patternOctave = noteOffset >= 0 ? noteOffset / mapSize : (noteOffset - mapSize + 1) / mapSize;
        int positionInPattern = noteOffset - (patternOctave * mapSize);

        if (positionInPattern < 0)
        {
            positionInPattern += mapSize;
            patternOctave--;
        }

        if (positionInPattern >= 0 && positionInPattern < static_cast<int>(kbmMapping.size()))
        {
            int mappedDegree = kbmMapping[static_cast<size_t>(positionInPattern)];

            if (mappedDegree < 0)
            {
                return calculate12TETFrequency(midiNote);
            }

            scaleDegree = mappedDegree;
        }
        else
        {
            scaleDegree = positionInPattern % scaleSize;
        }

        octaveNumber = patternOctave;
        scaleDegree = juce::jlimit(0, scaleSize, scaleDegree);

        double centsOffset = activeIntervals[static_cast<size_t>(scaleDegree)];
        centsOffset += octaveNumber * period;

        double refFreq = a4Frequency;
        int refNote = kbmReferenceNote;

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
        double refCentsFromC0 = activeIntervals[static_cast<size_t>(refDegree)] + refPatternOctave * period;

        double centsFromRef = centsOffset - refCentsFromC0;
        double stretchedCents = centsFromRef * static_cast<double>(octaveStretch);

        return refFreq * std::pow(2.0, stretchedCents / 1200.0);
    }
    else
    {
        // Linear mapping for all scale sizes
        const int anchorNote = 60 + tonic;
        int noteRelativeToAnchor = midiNote - anchorNote;

        int scaleOctave;
        if (noteRelativeToAnchor >= 0)
        {
            scaleOctave = noteRelativeToAnchor / scaleSize;
            scaleDegree = noteRelativeToAnchor % scaleSize;
        }
        else
        {
            scaleOctave = (noteRelativeToAnchor - scaleSize + 1) / scaleSize;
            scaleDegree = noteRelativeToAnchor - (scaleOctave * scaleSize);
        }

        scaleDegree = juce::jlimit(0, scaleSize - 1, scaleDegree);

        double intervalCents = scaleIntervals[static_cast<size_t>(scaleDegree)];
        double scalePeriod = scaleIntervals.back();

        double anchorFreq = calculate12TETFrequency(anchorNote);
        double totalCents = (scaleOctave * scalePeriod) + intervalCents;
        double stretchedCents = totalCents * static_cast<double>(octaveStretch);

        return anchorFreq * std::pow(2.0, stretchedCents / 1200.0);
    }
}

double TuningEngine::applyPitchBend(double baseFreq, float bendAmount) const
{
    const double bendSemitones = static_cast<double>(bendAmount * pitchBendRange);
    return baseFreq * std::pow(2.0, bendSemitones / 12.0);
}

void TuningEngine::publishIntervalsSnapshot()
{
    std::atomic_store(&intervalsSnapshot_,
                      std::make_shared<const std::vector<double>>(scaleIntervals));
}

void TuningEngine::rebuildFrequencyTable()
{
    Mode mode = currentMode.load(std::memory_order_relaxed);

    if (mode == Mode::TwelveTET)
    {
        for (int midiNote = 0; midiNote < 128; ++midiNote)
        {
            double freq = calculate12TETFrequency(midiNote);
            frequencyTable[static_cast<size_t>(midiNote)].store(freq, std::memory_order_relaxed);
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(intervalMutex);
        for (int midiNote = 0; midiNote < 128; ++midiNote)
        {
            double freq = calculateCustomFrequencyUnlocked(midiNote);
            frequencyTable[static_cast<size_t>(midiNote)].store(freq, std::memory_order_relaxed);
        }
    }
}
