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

            weights[v] = 1.0f / std::pow (dist, focus);
            weightSum += weights[v];
        }

        // Normalize weights
        float invSum = 1.0f / weightSum;
        for (int v = 0; v < VowelData::kNumVowels; ++v)
            weights[v] *= invSum;

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
