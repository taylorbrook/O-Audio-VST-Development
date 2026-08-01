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
    }

    void updateDistance (float dist, float airAbs, int curveType)
    {
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

        currentGain = std::clamp (gain, 0.0f, 1.0f);

        float cutoff = 20000.0f / (1.0f + airAbs * 0.01f * (dist / 10.0f));
        cutoff = std::clamp (cutoff, 100.0f, 20000.0f);

        *lpf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, cutoff);
    }

    float processSample (float sample)
    {
        return currentGain * lpf.processSample (sample);
    }

    void reset()
    {
        lpf.reset();
        currentGain = 1.0f;
    }

private:
    double sampleRate = 48000.0;
    juce::dsp::IIR::Filter<float> lpf;
    float currentGain = 1.0f;
};
