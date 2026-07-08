/*
  ==============================================================================

    ChordGenerator.h
    Chord generation from user-selected scale degree intervals
    v1.5.0: Replaced hardcoded 7-note scales with dynamic enabled-interval system

  ==============================================================================
*/

#pragma once
#include <vector>

// v2.5.0: Voicing mode for chord voice distribution
enum class VoicingMode
{
    Free = 0,     // Current behavior: spread across octaves
    Close,        // All notes within one octave
    Open,         // Spread across 2 octaves (alternating octave displacement)
    Drop2,        // Close voicing with 2nd-highest note dropped an octave
    Thirds,       // Stacked 3rds from root
    Quartal,      // Stacked 4ths from root
    Quintal       // Stacked 5ths from root
};

struct ChordVoice
{
    int midiNote;              // Absolute MIDI note number
    int octaveShift;           // Additional octave shifts for voicing
    float complexityThreshold; // Minimum complexity for this voice to be audible
};

class ChordGenerator
{
public:
    ChordGenerator();

    /**
     * Generate chord voicing from a single MIDI note using enabled scale degrees.
     *
     * @param rootMidiNote  The played MIDI note
     * @param numVoices     Max voices to generate (2-12)
     * @param keyRoot       Root note offset (0-11, for transposition)
     * @param enabledDegrees  Sorted list of scale degree offsets to use for chord building
     *                        (e.g., {0, 4, 7, 11} for root + intervals at degrees 4, 7, 11)
     * @param scaleDegreeCount  Total degrees in the scale (for octave wrapping)
     * @param voicingMode   How to distribute voices (Free, Close, Open, Drop2, Thirds, Quartal, Quintal)
     *
     * WR-01: returns a reference to an internal, pre-reserved buffer so there is NO
     * heap allocation on the audio thread (startNote). The returned reference is valid
     * until the next generateChord() call. ChordGenerator is accessed single-threaded
     * from the synth render callback, so the shared scratch is safe.
     */
    const std::vector<ChordVoice>& generateChord(int rootMidiNote, int numVoices,
                                                  int keyRoot, const std::vector<int>& enabledDegrees,
                                                  int scaleDegreeCount,
                                                  VoicingMode voicingMode = VoicingMode::Free);

private:
    // Map MIDI note to nearest enabled scale degree
    int findNearestDegree(int midiNote, int keyRoot, const std::vector<int>& enabledDegrees,
                          int scaleDegreeCount) const;

    // Build chord intervals from enabled degrees, sorted by proximity to root (fills intervalsBuf_)
    void buildChordIntervals(int rootDegreeInScale, const std::vector<int>& enabledDegrees,
                             int scaleDegreeCount);

    // Distribute voices across available intervals and octaves (reads intervalsBuf_, fills voicesBuf_)
    void distributeVoices(int rootMidiNote, int numVoices,
                          int scaleDegreeCount, VoicingMode voicingMode);

    // Pre-reserved scratch buffers (audio-thread use only; reserved in the ctor so
    // clear()+push_back() never reallocates in steady state)
    std::vector<ChordVoice> voicesBuf_;
    std::vector<int> intervalsBuf_;
};
