/*
   This file is part of O-IntonationPad, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    ChordGenerator.cpp
    v1.5.0: Dynamic interval-based chord generation
    v2.5.0: Voicing mode presets (Free, Close, Open, Drop-2, Thirds, Quartal, Quintal)

  ==============================================================================
*/

#include "ChordGenerator.h"
#include <algorithm>
#include <cmath>

ChordGenerator::ChordGenerator()
{
    // WR-01: reserve generously so generateChord() never reallocates on the audio thread.
    // numVoices is capped at 12; intervals ≤ scale-degree count (covers large microtonal scales).
    voicesBuf_.reserve(32);
    intervalsBuf_.reserve(256);
}

const std::vector<ChordVoice>& ChordGenerator::generateChord(int rootMidiNote, int numVoices,
                                                             int keyRoot, const std::vector<int>& enabledDegrees,
                                                             int scaleDegreeCount,
                                                             VoicingMode voicingMode)
{
    voicesBuf_.clear();

    if (enabledDegrees.empty() || scaleDegreeCount <= 0)
    {
        // Fallback: single voice at root
        voicesBuf_.push_back({ rootMidiNote, 0, 0.0f });
        return voicesBuf_;
    }

    // Find which enabled degree the played note maps to
    int rootDegreeInScale = findNearestDegree(rootMidiNote, keyRoot, enabledDegrees, scaleDegreeCount);

    // Build chord intervals from enabled degrees (into intervalsBuf_)
    buildChordIntervals(rootDegreeInScale, enabledDegrees, scaleDegreeCount);

    // Distribute voices across the intervals (into voicesBuf_)
    distributeVoices(rootMidiNote, numVoices, scaleDegreeCount, voicingMode);
    return voicesBuf_;
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

void ChordGenerator::buildChordIntervals(int rootDegreeInScale,
                                          const std::vector<int>& enabledDegrees,
                                          int scaleDegreeCount)
{
    // Build intervals as degree offsets from the root degree
    // These become MIDI note offsets (since TuningEngine maps MIDI linearly through degrees)
    intervalsBuf_.clear();

    for (int d : enabledDegrees)
    {
        int offset = d - rootDegreeInScale;
        if (offset < 0)
            offset += scaleDegreeCount;
        intervalsBuf_.push_back(offset);
    }

    // Sort by offset (ascending) — root (0) first, then upward
    std::sort(intervalsBuf_.begin(), intervalsBuf_.end());
}

void ChordGenerator::distributeVoices(int rootMidiNote, int numVoices,
                                      int scaleDegreeCount, VoicingMode voicingMode)
{
    // Aliases keep the mode-specific logic below unchanged while writing into the
    // pre-reserved member buffer (no per-call allocation on the audio thread).
    auto& voices = voicesBuf_;               // already cleared by generateChord()
    const auto& intervals = intervalsBuf_;
    int availableIntervals = static_cast<int>(intervals.size());

    if (availableIntervals == 0)
    {
        voices.push_back({ rootMidiNote, 0, 0.0f });
        return;
    }

    // Assign complexity thresholds: root interval gets 0.0 (always on),
    // subsequent intervals get progressively higher thresholds
    auto getThreshold = [](int voiceIndex, int totalVoices) -> float {
        if (voiceIndex == 0) return 0.0f;  // Root always audible
        if (totalVoices <= 1) return 0.0f;
        return static_cast<float>(voiceIndex) / static_cast<float>(totalVoices) * 0.85f;
    };

    switch (voicingMode)
    {
        case VoicingMode::Close:
        {
            // All voices within one octave — no octave shifts
            for (int i = 0; i < numVoices; ++i)
            {
                int intervalIndex = (availableIntervals > 0) ? (i % availableIntervals) : 0;
                int degreeOffset = intervals[intervalIndex];

                voices.push_back({
                    rootMidiNote + degreeOffset,
                    0,
                    getThreshold(i, numVoices)
                });
            }
            break;
        }

        case VoicingMode::Open:
        {
            // Spread across 2 octaves: odd-indexed voices shifted up one octave
            for (int i = 0; i < numVoices; ++i)
            {
                int intervalIndex = (availableIntervals > 0) ? (i % availableIntervals) : 0;
                int degreeOffset = intervals[intervalIndex];
                int octaveShift = (i % 2 != 0) ? 1 : 0;

                voices.push_back({
                    rootMidiNote + degreeOffset + (octaveShift * scaleDegreeCount),
                    octaveShift,
                    getThreshold(i, numVoices)
                });
            }
            break;
        }

        case VoicingMode::Drop2:
        {
            // Build close voicing directly in the output buffer, then drop 2nd-highest an octave
            for (int i = 0; i < numVoices; ++i)
            {
                int intervalIndex = (availableIntervals > 0) ? (i % availableIntervals) : 0;
                int degreeOffset = intervals[intervalIndex];

                voices.push_back({
                    rootMidiNote + degreeOffset,
                    0,
                    getThreshold(i, numVoices)
                });
            }

            // Sort by MIDI note (ascending) to find 2nd-highest
            std::sort(voices.begin(), voices.end(),
                      [](const ChordVoice& a, const ChordVoice& b) { return a.midiNote < b.midiNote; });

            // Drop the 2nd-highest note down one octave (if we have at least 2 voices)
            if (static_cast<int>(voices.size()) >= 2)
            {
                auto& drop = voices[voices.size() - 2];
                drop.midiNote -= 12;  // Always 12 semitones (one standard octave)
                drop.octaveShift = -1;
            }

            break;
        }

        case VoicingMode::Thirds:
        {
            // Stack in 3rds from root: approximate major 3rd = scaleDegreeCount * 4 / 12
            int thirdInterval = std::max(1, (scaleDegreeCount * 4 + 6) / 12);  // Rounded

            for (int i = 0; i < numVoices; ++i)
            {
                int degreeOffset = thirdInterval * i;
                int octaveShift = degreeOffset / scaleDegreeCount;
                int midiOffset = degreeOffset;

                voices.push_back({
                    rootMidiNote + midiOffset,
                    octaveShift,
                    getThreshold(i, numVoices)
                });
            }
            break;
        }

        case VoicingMode::Quartal:
        {
            // Stack in 4ths from root: approximate perfect 4th = scaleDegreeCount * 5 / 12
            int fourthInterval = std::max(1, (scaleDegreeCount * 5 + 6) / 12);  // Rounded

            for (int i = 0; i < numVoices; ++i)
            {
                int degreeOffset = fourthInterval * i;
                int octaveShift = degreeOffset / scaleDegreeCount;
                int midiOffset = degreeOffset;

                voices.push_back({
                    rootMidiNote + midiOffset,
                    octaveShift,
                    getThreshold(i, numVoices)
                });
            }
            break;
        }

        case VoicingMode::Quintal:
        {
            // Stack in 5ths from root: approximate perfect 5th = scaleDegreeCount * 7 / 12
            int fifthInterval = std::max(1, (scaleDegreeCount * 7 + 6) / 12);  // Rounded

            for (int i = 0; i < numVoices; ++i)
            {
                int degreeOffset = fifthInterval * i;
                int octaveShift = degreeOffset / scaleDegreeCount;
                int midiOffset = degreeOffset;

                voices.push_back({
                    rootMidiNote + midiOffset,
                    octaveShift,
                    getThreshold(i, numVoices)
                });
            }
            break;
        }

        case VoicingMode::Free:
        default:
        {
            // Original behavior: spread across octaves
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

                voices.push_back({
                    rootMidiNote + degreeOffset + (octaveOffset * scaleDegreeCount),
                    octaveOffset,
                    getThreshold(i, numVoices)
                });
            }
            break;
        }
    }
    // Result is in voicesBuf_ (aliased as `voices`)
}
