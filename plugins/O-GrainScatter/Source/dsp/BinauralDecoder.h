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
    void prepare (double /*sampleRate*/, int maxBlockSize)
    {
        // Allocate delay lines for each of the 16 input channels
        for (size_t ch = 0; ch < static_cast<size_t> (kHOA3Channels); ++ch)
        {
            delayLines[ch].resize (static_cast<size_t> (kBinauralFilterLength), 0.0f);
            writePos[ch] = 0;
        }

        // Pre-allocate output scratch
        scratchL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        scratchR.resize (static_cast<size_t> (maxBlockSize), 0.0f);
    }

    void reset()
    {
        for (size_t ch = 0; ch < static_cast<size_t> (kHOA3Channels); ++ch)
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

        for (size_t ch = 0; ch < static_cast<size_t> (kHOA3Channels); ++ch)
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
    std::vector<float> scratchL, scratchR;
};
