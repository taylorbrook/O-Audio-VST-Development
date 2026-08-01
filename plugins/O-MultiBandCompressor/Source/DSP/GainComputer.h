/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

    GainComputer.h
    Soft knee gain calculation for compressor
    O-MultiBandCompressor
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

class GainComputer
{
public:
    GainComputer() = default;
    ~GainComputer() = default;

    // Calculate gain reduction in dB
    // inputLevel: linear level from envelope detector
    // threshold, knee: in dB
    // ratio: compression ratio (1.0 to 20.0)
    float calculateGainReduction(float inputLevel, float thresholdDB, float ratio, float kneeDB)
    {
        // Convert input level to dB (with epsilon for log safety)
        float inputDB = 20.0f * std::log10(inputLevel + 1e-10f);

        // Calculate soft knee boundaries
        float kneeHalf = kneeDB / 2.0f;
        float kneeStart = thresholdDB - kneeHalf;
        float kneeEnd = thresholdDB + kneeHalf;

        float gainReductionDB = 0.0f;

        if (inputDB < kneeStart)
        {
            // Below knee: no compression
            gainReductionDB = 0.0f;
        }
        else if (inputDB < kneeEnd)
        {
            // Soft knee region: quadratic interpolation
            float x = inputDB - thresholdDB;
            float slope = (1.0f / ratio) - 1.0f;

            // Prevent division by zero when knee = 0
            if (kneeDB > 0.01f)
            {
                gainReductionDB = slope * (x + kneeHalf) * (x + kneeHalf) / (2.0f * kneeDB);
            }
            else
            {
                // Hard knee (knee = 0)
                gainReductionDB = slope * (inputDB - thresholdDB);
            }
        }
        else
        {
            // Above knee: full ratio compression
            gainReductionDB = thresholdDB + (inputDB - thresholdDB) / ratio - inputDB;
        }

        // Gain reduction is always negative or zero
        return std::min(0.0f, gainReductionDB);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainComputer)
};
