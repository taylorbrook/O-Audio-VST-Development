/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include <cmath>

class ScaleQuantizer
{
public:
    // Scale lookup tables: input semitone (0-11) → nearest scale degree
    static constexpr int chromatic[12]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    static constexpr int major[12]      = { 0, 0, 2, 2, 4, 5, 5, 7, 7, 9, 9, 11 };
    static constexpr int minor[12]      = { 0, 0, 2, 3, 3, 5, 5, 7, 8, 8, 10, 10 };
    static constexpr int pentatonic[12] = { 0, 0, 2, 2, 4, 4, 7, 7, 7, 9, 9, 9 };
    static constexpr int wholeTone[12]  = { 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10 };

    // Scale degree counts for ladder modes
    static constexpr int majorDegrees[7]      = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minorDegrees[7]      = { 0, 2, 3, 5, 7, 8, 10 };
    static constexpr int pentatonicDegrees[5] = { 0, 2, 4, 7, 9 };
    static constexpr int wholeToneDegrees[6]  = { 0, 2, 4, 6, 8, 10 };

    enum class PitchMode { Random = 0, LadderUp, LadderDown, Pendulum };
    enum class Scale     { Chromatic = 0, Major, Minor, Pentatonic, WholeTone };

    static int quantizeToScale (int semitones, int scaleIndex)
    {
        int note = ((semitones % 12) + 12) % 12;  // Always-positive modulo
        switch (scaleIndex)
        {
            case 1:  return major[note];
            case 2:  return minor[note];
            case 3:  return pentatonic[note];
            case 4:  return wholeTone[note];
            default: return chromatic[note];
        }
    }

    float getNextPitch (int pitchModeIndex, int scaleIndex, int rootNote,
                        float pitchRandom, juce::Random& rng)
    {
        if (pitchRandom < 0.01f)
            return 1.0f;  // No pitch change

        int semitones = 0;

        if (pitchModeIndex == 0)  // Random
        {
            int raw = rng.nextInt (25) - 12;  // -12 to +12
            semitones = static_cast<int> (static_cast<float> (raw) * (pitchRandom / 100.0f));
        }
        else  // Ladder modes
        {
            const int* degrees = nullptr;
            int numDegrees = 0;
            getScaleDegrees (scaleIndex, degrees, numDegrees);

            int degreeSemitones = degrees[ladderIndex % numDegrees];
            semitones = static_cast<int> (static_cast<float> (degreeSemitones) * (pitchRandom / 100.0f));

            advanceLadder (pitchModeIndex, numDegrees);
        }

        // Quantize to scale (relative to root)
        int shifted = semitones + rootNote;
        int quantized = quantizeToScale (shifted, scaleIndex) - rootNote;

        return std::pow (2.0f, static_cast<float> (quantized) / 12.0f);
    }

    void resetLadder()
    {
        ladderIndex = 0;
        ladderDirection = 1;
    }

private:
    int ladderIndex = 0;
    int ladderDirection = 1;  // 1 = ascending, -1 = descending

    static void getScaleDegrees (int scaleIndex, const int*& degrees, int& count)
    {
        switch (scaleIndex)
        {
            case 1:  degrees = majorDegrees;      count = 7; break;
            case 2:  degrees = minorDegrees;      count = 7; break;
            case 3:  degrees = pentatonicDegrees;  count = 5; break;
            case 4:  degrees = wholeToneDegrees;   count = 6; break;
            default: // Chromatic: all 12 semitones
            {
                static constexpr int chromaticDegrees[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
                degrees = chromaticDegrees;
                count = 12;
                break;
            }
        }
    }

    void advanceLadder (int pitchModeIndex, int numDegrees)
    {
        if (pitchModeIndex == 1)  // Ladder Up
        {
            ladderIndex = (ladderIndex + 1) % numDegrees;
        }
        else if (pitchModeIndex == 2)  // Ladder Down
        {
            ladderIndex = (ladderIndex - 1 + numDegrees) % numDegrees;
        }
        else if (pitchModeIndex == 3)  // Pendulum
        {
            ladderIndex += ladderDirection;
            if (ladderIndex >= numDegrees - 1)
            {
                ladderIndex = numDegrees - 2;  // Skip boundary on reversal
                ladderDirection = -1;
            }
            else if (ladderIndex <= 0)
            {
                ladderIndex = 1;  // Skip boundary on reversal
                ladderDirection = 1;
            }
        }
    }
};
