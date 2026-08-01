/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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

    OverlapAddProcessor.h
    O-Texture - Overlap-add crossfading for neural decoder output
    Ouaricon Audio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "HannWindow.h"

class OverlapAddProcessor
{
public:
    static constexpr int BLOCK_SIZE = 4096;
    static constexpr int HOP_SIZE   = 2048;
    static constexpr int ACCUM_SIZE = BLOCK_SIZE + HOP_SIZE; // 6144

    OverlapAddProcessor() = default;

    void prepare(int numChannels)
    {
        channels = numChannels;
        accumulator.setSize(numChannels, ACCUM_SIZE, false, true);
        accumulator.clear();
        readPosition = 0;
    }

    void reset()
    {
        accumulator.clear();
        readPosition = 0;
    }

    void addDecodedBlock(int channel, const float* decodedBlock)
    {
        jassert(channel >= 0 && channel < channels);

        juce::FloatVectorOperations::multiply(windowedBlock.data(),
                                               decodedBlock,
                                               hannWindow.data(),
                                               BLOCK_SIZE);

        auto* accum = accumulator.getWritePointer(channel);
        juce::FloatVectorOperations::add(accum, windowedBlock.data(), BLOCK_SIZE);
    }

    /** Read numSamples from ALL channels into a stereo (or multi-channel) output buffer.
     *  Returns the number of hop boundaries crossed. When a hop is crossed, the caller
     *  should supply a new decoded block via addDecodedBlock.
     */
    int readSamples(juce::AudioBuffer<float>& output, int startSample, int numSamples)
    {
        int hopsCrossed = 0;
        int samplesRemaining = numSamples;
        int outputPos = startSample;

        while (samplesRemaining > 0)
        {
            int samplesUntilHop = HOP_SIZE - readPosition;
            int samplesToRead = std::min(samplesRemaining, samplesUntilHop);

            for (int ch = 0; ch < std::min(channels, output.getNumChannels()); ++ch)
            {
                const float* src = accumulator.getReadPointer(ch, readPosition);
                std::memcpy(output.getWritePointer(ch, outputPos), src,
                            sizeof(float) * static_cast<size_t>(samplesToRead));
            }

            readPosition += samplesToRead;
            outputPos += samplesToRead;
            samplesRemaining -= samplesToRead;

            if (readPosition >= HOP_SIZE)
            {
                shiftAccumulator();
                readPosition = 0;
                ++hopsCrossed;
            }
        }

        return hopsCrossed;
    }

    int getReadPosition() const noexcept { return readPosition; }
    int getSamplesUntilNextHop() const noexcept { return HOP_SIZE - readPosition; }
    static constexpr int getLatencySamples() noexcept { return ACCUM_SIZE; }

private:
    void shiftAccumulator()
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* buf = accumulator.getWritePointer(ch);
            std::memmove(buf, buf + HOP_SIZE, sizeof(float) * (ACCUM_SIZE - HOP_SIZE));
            juce::FloatVectorOperations::clear(buf + (ACCUM_SIZE - HOP_SIZE), HOP_SIZE);
        }
    }

    juce::AudioBuffer<float> accumulator;
    std::array<float, BLOCK_SIZE> windowedBlock{};
    HannWindow<BLOCK_SIZE> hannWindow;
    int readPosition = 0;
    int channels = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlapAddProcessor)
};
