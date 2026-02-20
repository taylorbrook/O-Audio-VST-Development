/*
  ==============================================================================

    ChordGenerator.h
    Chord generation from user-selected scale degree intervals
    v1.5.0: Replaced hardcoded 7-note scales with dynamic enabled-interval system

  ==============================================================================
*/

#pragma once
#include <vector>

struct ChordVoice
{
    int midiNote;              // Absolute MIDI note number
    int octaveShift;           // Additional octave shifts for voicing
    float complexityThreshold; // Minimum complexity for this voice to be audible
};

class ChordGenerator
{
public:
    ChordGenerator() = default;

    /**
     * Generate chord voicing from a single MIDI note using enabled scale degrees.
     *
     * @param rootMidiNote  The played MIDI note
     * @param numVoices     Max voices to generate (2-12)
     * @param complexity    0.0-1.0, controls how many enabled intervals are audible
     * @param keyRoot       Root note offset (0-11, for transposition)
     * @param enabledDegrees  Sorted list of scale degree offsets to use for chord building
     *                        (e.g., {0, 4, 7, 11} for root + intervals at degrees 4, 7, 11)
     * @param scaleDegreeCount  Total degrees in the scale (for octave wrapping)
     */
    std::vector<ChordVoice> generateChord(int rootMidiNote, int numVoices, float complexity,
                                           int keyRoot, const std::vector<int>& enabledDegrees,
                                           int scaleDegreeCount);

private:
    // Map MIDI note to nearest enabled scale degree
    int findNearestDegree(int midiNote, int keyRoot, const std::vector<int>& enabledDegrees,
                          int scaleDegreeCount) const;

    // Build chord intervals from enabled degrees, sorted by proximity to root
    std::vector<int> buildChordIntervals(int rootDegreeInScale, const std::vector<int>& enabledDegrees,
                                          int scaleDegreeCount) const;

    // Distribute voices across available intervals and octaves
    std::vector<ChordVoice> distributeVoices(int rootMidiNote, int rootDegreeInScale,
                                              const std::vector<int>& intervals, int numVoices,
                                              int scaleDegreeCount) const;
};
