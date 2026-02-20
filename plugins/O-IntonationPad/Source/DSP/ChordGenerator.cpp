/*
  ==============================================================================

    ChordGenerator.cpp
    v1.5.0: Dynamic interval-based chord generation

  ==============================================================================
*/

#include "ChordGenerator.h"
#include <algorithm>
#include <cmath>

std::vector<ChordVoice> ChordGenerator::generateChord(int rootMidiNote, int numVoices, float complexity,
                                                       int keyRoot, const std::vector<int>& enabledDegrees,
                                                       int scaleDegreeCount)
{
    if (enabledDegrees.empty() || scaleDegreeCount <= 0)
    {
        // Fallback: single voice at root
        return {{ rootMidiNote, 0, 0.0f }};
    }

    // Find which enabled degree the played note maps to
    int rootDegreeInScale = findNearestDegree(rootMidiNote, keyRoot, enabledDegrees, scaleDegreeCount);

    // Build chord intervals from enabled degrees
    auto intervals = buildChordIntervals(rootDegreeInScale, enabledDegrees, scaleDegreeCount);

    // Distribute voices across the intervals
    return distributeVoices(rootMidiNote, rootDegreeInScale, intervals, numVoices, scaleDegreeCount);
}

int ChordGenerator::findNearestDegree(int midiNote, int keyRoot, const std::vector<int>& enabledDegrees,
                                       int scaleDegreeCount) const
{
    // Calculate the scale degree position of this MIDI note relative to keyRoot
    // In TuningEngine, MIDI note 60 + tonicOffset = degree 0
    // keyRoot acts as the tonic offset for chord generation
    int relativeMidi = midiNote - 60 - keyRoot;

    // Normalize to positive scale degree
    int degree = ((relativeMidi % scaleDegreeCount) + scaleDegreeCount) % scaleDegreeCount;

    // Find nearest enabled degree
    int bestDegree = enabledDegrees[0];
    int bestDist = scaleDegreeCount; // max possible

    for (int d : enabledDegrees)
    {
        int dist = std::min(std::abs(degree - d),
                           scaleDegreeCount - std::abs(degree - d));
        if (dist < bestDist)
        {
            bestDist = dist;
            bestDegree = d;
        }
    }

    return bestDegree;
}

std::vector<int> ChordGenerator::buildChordIntervals(int rootDegreeInScale,
                                                      const std::vector<int>& enabledDegrees,
                                                      int scaleDegreeCount) const
{
    // Build intervals as degree offsets from the root degree
    // These become MIDI note offsets (since TuningEngine maps MIDI linearly through degrees)
    std::vector<int> intervals;

    for (int d : enabledDegrees)
    {
        int offset = d - rootDegreeInScale;
        if (offset < 0)
            offset += scaleDegreeCount;
        intervals.push_back(offset);
    }

    // Sort by offset (ascending) — root (0) first, then upward
    std::sort(intervals.begin(), intervals.end());

    return intervals;
}

std::vector<ChordVoice> ChordGenerator::distributeVoices(int rootMidiNote, int rootDegreeInScale,
                                                          const std::vector<int>& intervals,
                                                          int numVoices, int scaleDegreeCount) const
{
    std::vector<ChordVoice> voices;
    int availableIntervals = static_cast<int>(intervals.size());

    if (availableIntervals == 0)
        return {{ rootMidiNote, 0, 0.0f }};

    // Assign complexity thresholds: root interval gets 0.0 (always on),
    // subsequent intervals get progressively higher thresholds
    auto getThreshold = [&](int intervalIndex) -> float {
        if (intervalIndex == 0) return 0.0f;  // Root always audible
        if (availableIntervals <= 1) return 0.0f;
        // Distribute thresholds evenly from 0.0 to ~0.85
        return static_cast<float>(intervalIndex) / static_cast<float>(availableIntervals) * 0.85f;
    };

    for (int i = 0; i < numVoices; ++i)
    {
        int intervalIndex;
        int octaveOffset;

        if (numVoices <= availableIntervals)
        {
            intervalIndex = i;
            octaveOffset = 0;
        }
        else
        {
            intervalIndex = (i * availableIntervals) / numVoices;
            octaveOffset = i / availableIntervals;
        }

        int degreeOffset = intervals[intervalIndex];

        ChordVoice voice;
        voice.midiNote = rootMidiNote + degreeOffset + (octaveOffset * scaleDegreeCount);
        voice.octaveShift = octaveOffset;
        voice.complexityThreshold = getThreshold(intervalIndex);

        voices.push_back(voice);
    }

    return voices;
}
