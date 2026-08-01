/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    VowelMorpher.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Shepard (IDW) interpolation between 5 cardinal vowels.
    Frequencies blended in log domain, bandwidths and gains linear.

  ==============================================================================
*/

#pragma once
#include "VowelData.h"
#include <cmath>
#include <algorithm>

class VowelMorpher
{
public:
    // Compute interpolated formant parameters from XY cursor position
    void compute (float cursorX, float cursorY, float focus,
                  float outFreq[5], float outBW[5], float outGain[5]) const noexcept
    {
        // IN-06: clamp the cursor to the unit square on entry. Callers are not
        // guaranteed to pre-clamp — lyric syllable targets and the MPE-timbre
        // vowelY offset can both push the cursor out of [0,1], which skews the
        // inverse-distance weights. Defend here so every caller is covered.
        cursorX = std::max (0.0f, std::min (cursorX, 1.0f));
        cursorY = std::max (0.0f, std::min (cursorY, 1.0f));

        float weights[VowelData::kNumVowels];
        float weightSum = 0.0f;

        for (int v = 0; v < VowelData::kNumVowels; ++v)
        {
            float dx = cursorX - VowelData::vowels[v].x;
            float dy = cursorY - VowelData::vowels[v].y;
            float dist = std::sqrt (dx * dx + dy * dy);

            if (dist < 1e-6f)
            {
                // Snap directly to this vowel
                for (int f = 0; f < 5; ++f)
                {
                    outFreq[f] = VowelData::vowels[v].freq[f];
                    outBW[f]   = VowelData::vowels[v].bandwidth[f];
                    outGain[f] = VowelData::vowels[v].gain[f];
                }
                return;
            }

            // WR-03: floor the distance and cap the weight. Without the floor,
            // 1/pow(dist, focus) just above the 1e-6 snap epsilon (with large
            // focus) can underflow pow() toward 0 → weight Inf → weightSum Inf
            // → invSum 0 → weight = Inf*0 = NaN → NaN formant frequencies, which
            // then poison the biquads (see WR-05). Flooring dist and capping
            // keeps every weight finite; the vowel nearest the cursor still
            // dominates, so the near-snap behaviour is audibly unchanged.
            dist = std::max (dist, 1e-3f);
            float w = 1.0f / std::pow (dist, focus);
            weights[v] = std::min (w, 1.0e12f);
            weightSum += weights[v];
        }

        // WR-03: guard the sum before dividing. After the floor/cap this should
        // always be finite and > 0, but if it ever is not, fall back to an equal
        // blend so the output stays finite and the filter never goes silent.
        if (! std::isfinite (weightSum) || weightSum <= 0.0f)
        {
            const float equal = 1.0f / static_cast<float> (VowelData::kNumVowels);
            for (int v = 0; v < VowelData::kNumVowels; ++v)
                weights[v] = equal;
        }
        else
        {
            float invSum = 1.0f / weightSum;
            for (int v = 0; v < VowelData::kNumVowels; ++v)
                weights[v] *= invSum;
        }

        // Interpolate: frequencies in log domain, bandwidths and gains linear
        for (int f = 0; f < 5; ++f)
        {
            float logFreq = 0.0f;
            float bw = 0.0f;
            float g = 0.0f;

            for (int v = 0; v < VowelData::kNumVowels; ++v)
            {
                logFreq += weights[v] * std::log (VowelData::vowels[v].freq[f]);
                bw      += weights[v] * VowelData::vowels[v].bandwidth[f];
                g       += weights[v] * VowelData::vowels[v].gain[f];
            }

            outFreq[f] = std::exp (logFreq);
            outBW[f]   = bw;
            outGain[f] = g;
        }
    }
};
