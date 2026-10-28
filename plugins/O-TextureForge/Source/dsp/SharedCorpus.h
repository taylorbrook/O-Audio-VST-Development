/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

    SharedCorpus.h
    Shared audio corpus and grain database

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GrainMetadata.h"
#include "BandlimitedReader.h"   // grain-engine 1.1.0 (modules/synthesis/grain-engine/cpp)
#include <cstdint>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>

// Forward declaration
class KDTreeSearch;

struct SharedCorpus
{
    juce::AudioBuffer<float> audioData;  // Mono, resampled to DAW SR
    std::vector<GrainMetadata> grains;
    NormalizationStats normStats;
    std::unique_ptr<KDTreeSearch> kdTree;

    // PCA projection (19D -> 2D), normalized [0,1]
    std::vector<float> pcaX;
    std::vector<float> pcaY;

    // UMAP projection (19D -> 2D), normalized [0,1]
    std::vector<float> umapX;
    std::vector<float> umapY;
    std::atomic<bool> umapReady { false };

    // v1.7.0: band-limited octave mipmap of audioData, read by pitched-up grains
    // (rate > 1). A grain-engine MipRing<6> fed kPreRoll zeros, the corpus, then
    // kTail zeros, and never wrapped: it is the store, read by delay from its head
    // (no MipSnapshot, so the build does not hold a second full copy). The zero
    // padding gives the same silence outside the corpus that sampleFromCorpus()
    // returns, and every level is settled across the whole corpus. Built on the
    // loader thread before the corpus is published; null = every grain reads
    // the linear path (also the harness corpora that never call it).
    static constexpr int kMipLevels = 6;   // rate up to 128; MIDI reaches 2^(67/12) ~ 48
    using MipRingT = ouaricon::grain::MipRing<kMipLevels>;
    static constexpr int kMipPreRoll = 8192;                       // > 7 * 64 + 63 * 63 taps behind sample 0
    static constexpr int kMipTail    = 2 * MipRingT::margin(kMipLevels) + 8 * (1 << kMipLevels) + 4096;
    std::unique_ptr<MipRingT> mip;
    int64_t mipHead = 0;   // write count minus kMipPreRoll: delay of corpus sample 0 is mipHead

    // Builds `mip` from channel 0 of audioData. Allocates; loader thread only.
    // Returns false (mip left null) if shouldExit() fires mid-build.
    bool buildBandlimited(const std::function<bool()>& shouldExit = {})
    {
        mip.reset();
        const int n = audioData.getNumSamples();
        if (n <= 0)
            return true;

        const float* x = audioData.getReadPointer(0);
        auto ring = std::make_unique<MipRingT>();
        ring->prepare(kMipPreRoll + n + kMipTail);
        for (int i = 0; i < kMipPreRoll; ++i) ring->push(0.0f, 0.0f);
        for (int i = 0; i < n; ++i)
        {
            ring->push(x[i], 0.0f);
            if ((i & 0xFFFF) == 0xFFFF && shouldExit && shouldExit())
                return false;
        }
        for (int i = 0; i < kMipTail; ++i) ring->push(0.0f, 0.0f);

        mipHead = ring->getWriteCount() - kMipPreRoll;
        mip = std::move(ring);
        return true;
    }

    // Band-limited read at absolute corpus position pos (level-0 samples, >= 0).
    // Past the end it reads the zero tail; MipRing clamps the delay to the level's
    // margin, which sits inside that tail. Allocation- and lock-free.
    float readBandlimited(int level, int sincSet, double pos) const noexcept
    {
        return mip->read(level, 0, static_cast<double>(mipHead) - pos, sincSet);
    }

    // v1.8.0: set by the processor when it publishes the corpus (message thread,
    // before the audio thread can see it). Keys the scheduler's candidate cache
    // and recently-used ring, so a new corpus at a freed one's address cannot
    // reuse their grain indices. 0 = never published (harness corpora).
    uint64_t generation = 0;

    uint32_t grainSizeSamples = 0;
    uint32_t hopSizeSamples = 0;
    double sampleRate = 44100.0;
    juce::String filePath;

    SharedCorpus() = default;
    ~SharedCorpus() = default;

    // Non-copyable but movable
    SharedCorpus(const SharedCorpus&) = delete;
    SharedCorpus& operator=(const SharedCorpus&) = delete;
    SharedCorpus(SharedCorpus&&) = default;
    SharedCorpus& operator=(SharedCorpus&&) = default;
};
