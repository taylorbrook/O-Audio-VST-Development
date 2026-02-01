/*
  ==============================================================================

    ChordGenerator.cpp
    Implementation of chord generation algorithm

  ==============================================================================
*/

#include "ChordGenerator.h"

std::vector<ChordVoice> ChordGenerator::generateChord(int rootMidiNote, int numVoices, float complexity,
                                                       int keyRoot, int scaleType)
{
    // Get scale pattern
    const auto& scale = getScalePattern(scaleType);

    // Find scale degree of played note
    int scaleDegree = findScaleDegree(rootMidiNote, keyRoot, scale);

    // Determine chord quality for this scale degree
    ChordQuality quality = getChordQuality(scaleDegree, scaleType);

    // Build chord intervals based on quality and complexity
    auto intervals = buildChordIntervals(quality, complexity);

    // Distribute voices across octaves
    return distributeVoices(rootMidiNote, scaleDegree, intervals, numVoices);
}

const std::array<int, 7>& ChordGenerator::getScalePattern(int scaleType) const
{
    switch (scaleType)
    {
        case 0: return majorScale;
        case 1: return minorScale;
        case 2: return dorianScale;
        case 3: return phrygianScale;
        case 4: return lydianScale;
        case 5: return mixolydianScale;
        case 6: return aeolianScale;
        case 7: return locrianScale;
        case 8: return harmonicMinorScale;
        case 9: return melodicMinorScale;
        default: return majorScale;
    }
}

ChordGenerator::ChordQuality ChordGenerator::getChordQuality(int scaleDegree, int scaleType) const
{
    // Chord quality patterns for each scale type
    // Major scale: I, ii, iii, IV, V, vi, vii°
    static const std::array<ChordQuality, 7> majorQualities = {
        Major, Minor, Minor, Major, Major, Minor, Diminished
    };

    // Minor scale (natural minor): i, ii°, III, iv, v, VI, VII
    static const std::array<ChordQuality, 7> minorQualities = {
        Minor, Diminished, Major, Minor, Minor, Major, Major
    };

    // Dorian: i, ii, III, IV, v, vi°, VII
    static const std::array<ChordQuality, 7> dorianQualities = {
        Minor, Minor, Major, Major, Minor, Diminished, Major
    };

    // Use appropriate quality array based on scale type
    switch (scaleType)
    {
        case 0: // Major
        case 4: // Lydian (same chord qualities as major)
        case 5: // Mixolydian (same chord qualities as major)
            return majorQualities[scaleDegree];

        case 1: // Minor
        case 6: // Aeolian (same as natural minor)
        case 8: // Harmonic minor (simplified)
        case 9: // Melodic minor (simplified)
            return minorQualities[scaleDegree];

        case 2: // Dorian
        case 3: // Phrygian (similar to Dorian)
        case 7: // Locrian (similar to Dorian)
            return dorianQualities[scaleDegree];

        default:
            return Major;
    }
}

int ChordGenerator::findScaleDegree(int midiNote, int keyRoot, const std::array<int, 7>& scale) const
{
    // Extract pitch class from MIDI note
    int pitchClass = midiNote % 12;

    // Calculate relative pitch class from key root
    int relativePitch = (pitchClass - keyRoot + 12) % 12;

    // Find closest scale degree
    for (int i = 0; i < 7; ++i)
    {
        if (scale[i] == relativePitch)
            return i;
    }

    // If not in scale, find nearest scale degree (prefer upward)
    for (int i = 0; i < 7; ++i)
    {
        if (scale[i] > relativePitch)
            return i;
    }

    // Default to root
    return 0;
}

std::vector<int> ChordGenerator::buildChordIntervals(ChordQuality quality, float complexity) const
{
    std::vector<int> intervals;

    // Root (always present)
    intervals.push_back(0);

    // Third (always present)
    int third = (quality == Major) ? 4 : 3;  // Major third (4) or minor third (3)
    intervals.push_back(third);

    // Fifth (always present)
    int fifth = (quality == Diminished) ? 6 : 7;  // Diminished fifth (6) or perfect fifth (7)
    intervals.push_back(fifth);

    // Add extensions based on complexity
    if (complexity >= 0.25f)
    {
        // Add 7th
        int seventh = (quality == Major) ? 11 : 10;  // Major 7th (11) or minor 7th (10)
        intervals.push_back(seventh);
    }

    if (complexity >= 0.50f)
    {
        // Add 9th (14 semitones = octave + major second)
        intervals.push_back(14);
    }

    if (complexity >= 0.75f)
    {
        // Add 11th (17 semitones = octave + perfect fourth)
        intervals.push_back(17);
    }

    if (complexity >= 0.85f)
    {
        // Add 13th (21 semitones = octave + major sixth)
        intervals.push_back(21);
    }

    return intervals;
}

std::vector<ChordVoice> ChordGenerator::distributeVoices(int rootMidiNote, int scaleDegree,
                                                          const std::vector<int>& intervals,
                                                          int numVoices) const
{
    std::vector<ChordVoice> voices;

    int availableIntervals = static_cast<int>(intervals.size());

    for (int i = 0; i < numVoices; ++i)
    {
        // Distribute voices evenly across available intervals
        int intervalIndex = (i * availableIntervals) / numVoices;
        int semitoneOffset = intervals[intervalIndex];

        // Spread across octaves if numVoices exceeds available intervals
        int octaveOffset = i / availableIntervals;

        ChordVoice voice;
        voice.midiNote = rootMidiNote + semitoneOffset + (octaveOffset * 12);
        voice.scaleDegree = scaleDegree;
        voice.semitoneOffset = semitoneOffset;
        voice.octaveShift = octaveOffset;

        voices.push_back(voice);
    }

    return voices;
}
