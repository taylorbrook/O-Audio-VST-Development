/*
  ==============================================================================

    ChordGenerator.h
    Chord generation algorithm with scale-degree analysis
    Phase 2.2: Generate 2-12 voice chords from single MIDI note

  ==============================================================================
*/

#pragma once
#include <vector>
#include <array>

struct ChordVoice
{
    int midiNote;              // Absolute MIDI note number
    int scaleDegree;           // Scale degree (0-6 for I-vii)
    int semitoneOffset;        // Semitones from root
    int octaveShift;           // Additional octave shifts for voicing
    float complexityThreshold; // Minimum complexity for this voice to be audible (0.0 = triad, always on)
};

class ChordGenerator
{
public:
    ChordGenerator() = default;

    // Generate chord voicing from single MIDI note
    std::vector<ChordVoice> generateChord(int rootMidiNote, int numVoices, float complexity,
                                           int keyRoot, int scaleType);

private:
    // Scale patterns (semitone intervals from root)
    static constexpr std::array<int, 7> majorScale = {0, 2, 4, 5, 7, 9, 11};
    static constexpr std::array<int, 7> minorScale = {0, 2, 3, 5, 7, 8, 10};
    static constexpr std::array<int, 7> dorianScale = {0, 2, 3, 5, 7, 9, 10};
    static constexpr std::array<int, 7> phrygianScale = {0, 1, 3, 5, 7, 8, 10};
    static constexpr std::array<int, 7> lydianScale = {0, 2, 4, 6, 7, 9, 11};
    static constexpr std::array<int, 7> mixolydianScale = {0, 2, 4, 5, 7, 9, 10};
    static constexpr std::array<int, 7> aeolianScale = {0, 2, 3, 5, 7, 8, 10};
    static constexpr std::array<int, 7> locrianScale = {0, 1, 3, 5, 6, 8, 10};
    static constexpr std::array<int, 7> harmonicMinorScale = {0, 2, 3, 5, 7, 8, 11};
    static constexpr std::array<int, 7> melodicMinorScale = {0, 2, 3, 5, 7, 9, 11};

    // Chord types for each scale degree
    enum ChordQuality { Major, Minor, Diminished };

    // Get scale pattern for scale type index
    const std::array<int, 7>& getScalePattern(int scaleType) const;

    // Get chord quality for scale degree in given scale
    ChordQuality getChordQuality(int scaleDegree, int scaleType) const;

    // Map MIDI note to scale degree
    int findScaleDegree(int midiNote, int keyRoot, const std::array<int, 7>& scale) const;

    // Build chord intervals based on quality and complexity
    std::vector<int> buildChordIntervals(ChordQuality quality, float complexity) const;

    // Distribute voices across octaves
    std::vector<ChordVoice> distributeVoices(int rootMidiNote, int scaleDegree,
                                              const std::vector<int>& intervals, int numVoices) const;

    // Map a semitone interval to its complexity threshold
    static float getComplexityThreshold(int semitoneOffset);
};
