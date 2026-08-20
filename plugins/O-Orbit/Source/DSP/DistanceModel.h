/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>

class DistanceModel
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, 512, 1 };
        lpf.prepare (spec);
        lpf.reset();
        currentGain = 1.0f;
        targetGain = 1.0f;
        gainStep = 0.0f;
        lastDist = -1.0f;
        lastAirAbs = -1.0f;
        lastCurveType = -1;
    }

    void updateDistance (float dist, float airAbs, int curveType, int numSamples)
    {
        // Coefficients rebuilt only when the inputs actually changed. The rebuild
        // assigns a stack array into the existing coefficient storage
        // (ArrayCoefficients) — no audio-thread allocation.
        if (dist != lastDist || airAbs != lastAirAbs || curveType != lastCurveType)
        {
            lastDist = dist;
            lastAirAbs = airAbs;
            lastCurveType = curveType;

            float gain = 1.0f;

            switch (curveType)
            {
                case 0: // Linear
                    gain = std::clamp (1.0f - (dist - 0.1f) / 29.9f, 0.0f, 1.0f);
                    break;
                case 1: // Inverse
                    gain = 1.0f / std::max (dist, 0.1f);
                    break;
                case 2: // Inverse Square
                    gain = 1.0f / std::max (dist * dist, 0.01f);
                    break;
            }

            targetGain = std::clamp (gain, 0.0f, 1.0f);

            float cutoff = 20000.0f / (1.0f + airAbs * 0.01f * (dist / 10.0f));
            cutoff = std::clamp (cutoff, 100.0f, 20000.0f);

            *lpf.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, cutoff);
        }

        // Gain ramps to the target across the block — distance moves every block
        // once motion depth is nonzero, so a per-block step would zipper.
        gainStep = numSamples > 0 ? (targetGain - currentGain) / (float) numSamples : 0.0f;
    }

    float processSample (float sample)
    {
        currentGain += gainStep;
        return currentGain * lpf.processSample (sample);
    }

    void reset()
    {
        lpf.reset();
        currentGain = 1.0f;
        targetGain = 1.0f;
        gainStep = 0.0f;
    }

private:
    double sampleRate = 48000.0;
    juce::dsp::IIR::Filter<float> lpf;
    float currentGain = 1.0f;
    float targetGain = 1.0f;
    float gainStep = 0.0f;
    float lastDist = -1.0f;
    float lastAirAbs = -1.0f;
    int lastCurveType = -1;
};
