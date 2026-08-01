/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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

    StereoWidth.h
    O-Wind - Allpass Decorrelator + Mid-Side Width Processing
    Ouaricon Audio
    Developer: Taylor Brook

    Shared pattern with O-Bowed. Creates stereo from mono waveguide output
    via allpass decorrelation on R channel + mid-side width control.
    Applied post-voice-summation in processBlock.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>

class StereoWidthProcessor
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec {
            sampleRate,
            static_cast<juce::uint32> (maxBlockSize),
            1
        };
        decorrelator.prepare (spec);
        decorrelator.coefficients =
            juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, 800.0f, 0.7f);
        widthSmoothed.reset (sampleRate, 0.02);  // 20ms smoothing
    }

    void reset()
    {
        decorrelator.reset();
        // Snap to target without touching the ramp configuration —
        // reset(0) sets steps-to-target to 0, silently disabling width
        // smoothing until the next prepare()
        widthSmoothed.setCurrentAndTargetValue (widthSmoothed.getTargetValue());
    }

    // Process stereo buffer in-place.
    // Input: channel 0 has mono voice output (summed by Synthesiser),
    //        channel 1 is a copy of channel 0 (Synthesiser writes to all channels).
    // Output: decorrelated stereo with width applied.
    void processBlock (juce::AudioBuffer<float>& buffer, float widthFactor)
    {
        if (buffer.getNumChannels() < 2)
            return;

        widthSmoothed.setTargetValue (widthFactor);

        auto* leftData  = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float mono = leftData[i];

            // Create stereo via allpass decorrelation on R channel
            float left  = mono;
            float right = decorrelator.processSample (mono);

            // Mid-side width processing
            float w = widthSmoothed.getNextValue();
            float mid  = (left + right) * 0.5f;
            float side = (left - right) * 0.5f;
            side *= w;

            leftData[i]  = mid + side;
            rightData[i] = mid - side;
        }
    }

private:
    juce::dsp::IIR::Filter<float> decorrelator;
    juce::SmoothedValue<float> widthSmoothed { 1.0f };
};
