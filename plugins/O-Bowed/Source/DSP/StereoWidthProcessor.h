/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    StereoWidthProcessor.h
    O-Bowed - Allpass Decorrelator + Mid-Side Width Processing
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>

class StereoWidthProcessor
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        widthSmoothed.reset (sampleRate, 0.02);  // 20ms smoothing
    }

    void reset()
    {
        widthSmoothed.reset (0);
    }

    // Process true stereo buffer in-place with M/S width control.
    // Input: already has meaningful stereo from per-string panning + body resonator.
    // width=0: mono, width=1: preserve, width=2: exaggerated stereo.
    void processBlock (juce::AudioBuffer<float>& buffer, float widthFactor)
    {
        if (buffer.getNumChannels() < 2)
            return;

        widthSmoothed.setTargetValue (widthFactor);

        auto* leftData  = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float w = widthSmoothed.getNextValue();
            float mid  = (leftData[i] + rightData[i]) * 0.5f;
            float side = (leftData[i] - rightData[i]) * 0.5f;
            side *= w;

            leftData[i]  = mid + side;
            rightData[i] = mid - side;
        }
    }

private:
    juce::SmoothedValue<float> widthSmoothed { 1.0f };
};
