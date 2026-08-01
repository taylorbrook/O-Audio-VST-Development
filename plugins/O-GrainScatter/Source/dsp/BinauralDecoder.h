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
#include <array>
#include <cstring>
#include "SpatialEncoder.h"
#include "HRIRData.h"

// HOA3-to-binaural decoder using direct-form FIR convolution.
// Convolves 16 ambisonics channels with 32 pre-computed binaural filters
// (16 channels x 2 ears, each kBinauralFilterLength taps).
//
// Zero latency: uses direct-form (time-domain) FIR, no FFT partitioning.
// At 128 taps this is ~2-5% CPU overhead, well within budget.

class BinauralDecoder
{
public:
    void prepare (double /*sampleRate*/, int /*maxBlockSize*/)
    {
        constexpr size_t numChannels = kHOA3Channels;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            delayLines[ch].resize (static_cast<size_t> (kBinauralFilterLength), 0.0f);
            writePos[ch] = 0;
        }
    }

    void reset()
    {
        constexpr size_t numChannels = kHOA3Channels;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            std::fill (delayLines[ch].begin(), delayLines[ch].end(), 0.0f);
            writePos[ch] = 0;
        }
    }

    // Process one audio block:
    //   hoaBus: 16 float arrays, each containing numSamples of HOA3 data
    //   outL, outR: output stereo buffers (numSamples each)
    //
    // This WRITES (not accumulates) to outL/outR.
    void process (const float* const* hoaBus, int numSamples,
                  float* outL, float* outR)
    {
        std::memset (outL, 0, static_cast<size_t> (numSamples) * sizeof (float));
        std::memset (outR, 0, static_cast<size_t> (numSamples) * sizeof (float));

        constexpr size_t numChannels = kHOA3Channels;

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            const float* input = hoaBus[ch];
            const float* filterL = kBinauralFilters[ch][0];
            const float* filterR = kBinauralFilters[ch][1];
            auto& dl = delayLines[ch];
            int wp = static_cast<int> (writePos[ch]);

            for (int s = 0; s < numSamples; ++s)
            {
                // Write input sample into circular delay line
                dl[static_cast<size_t> (wp)] = input[s];

                // Direct-form FIR convolution for both ears
                float sumL = 0.0f;
                float sumR = 0.0f;
                int readIdx = wp;

                for (int t = 0; t < kBinauralFilterLength; ++t)
                {
                    float dlSample = dl[static_cast<size_t> (readIdx)];
                    sumL += dlSample * filterL[t];
                    sumR += dlSample * filterR[t];

                    --readIdx;
                    if (readIdx < 0)
                        readIdx = kBinauralFilterLength - 1;
                }

                outL[s] += sumL;
                outR[s] += sumR;

                ++wp;
                if (wp >= kBinauralFilterLength)
                    wp = 0;
            }

            writePos[ch] = static_cast<size_t> (wp);
        }
    }

private:
    std::array<std::vector<float>, kHOA3Channels> delayLines;
    std::array<size_t, kHOA3Channels> writePos {};
};
