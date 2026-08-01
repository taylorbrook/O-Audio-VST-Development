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
