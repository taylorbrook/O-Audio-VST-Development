/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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
#include "MonoSummer.h"

void MonoSummer::prepare(int /* maxBlockSize */)
{
    // No pre-allocation needed - simple mono sum/expand
}

void MonoSummer::reset()
{
    // No state to reset
}

void MonoSummer::sumToMono(const juce::AudioBuffer<float>& stereoInput,
                           juce::AudioBuffer<float>& monoOutput)
{
    jassert(stereoInput.getNumChannels() >= 2);
    jassert(monoOutput.getNumChannels() >= 1);
    jassert(stereoInput.getNumSamples() == monoOutput.getNumSamples());

    const int numSamples = stereoInput.getNumSamples();
    const float* left = stereoInput.getReadPointer(0);
    const float* right = stereoInput.getReadPointer(1);
    float* mono = monoOutput.getWritePointer(0);

    // Simple (L+R)/2 sum
    for (int i = 0; i < numSamples; ++i)
    {
        mono[i] = (left[i] + right[i]) * 0.5f;
    }
}

void MonoSummer::expandToStereo(const juce::AudioBuffer<float>& monoInput,
                                 juce::AudioBuffer<float>& stereoOutput)
{
    jassert(monoInput.getNumChannels() >= 1);
    jassert(stereoOutput.getNumChannels() >= 2);
    jassert(monoInput.getNumSamples() == stereoOutput.getNumSamples());

    const int numSamples = monoInput.getNumSamples();
    const float* mono = monoInput.getReadPointer(0);
    float* left = stereoOutput.getWritePointer(0);
    float* right = stereoOutput.getWritePointer(1);

    // Mono to stereo - same signal to both channels
    // This is intentional for bass frequencies to maintain phase coherence
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = mono[i];
        right[i] = mono[i];
    }
}
