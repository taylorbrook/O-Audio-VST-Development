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

    GrainMetadata.h
    Data structures for grain database and corpus management

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct GrainMetadata
{
    uint32_t startSample;        // Start position in corpus buffer
    uint32_t lengthSamples;      // Grain length in samples
    std::array<float, 19> descriptors;  // 19D feature vector (MFCCs + spectral)
};

// Dimension weighting for KD-tree search: macro-controlled dims get higher weight
// so Energy/Brightness/Texture knobs dominate grain selection over uncontrolled MFCCs.
// Weight of 5 = 25x per-dim contribution (75% total from 3 controlled vs 16 uncontrolled).
static constexpr float MACRO_DIM_WEIGHT = 5.0f;
static constexpr int BRIGHTNESS_DIM = 13;  // Spectral Centroid
static constexpr int TEXTURE_DIM    = 14;  // Spectral Flatness
static constexpr int ENERGY_DIM     = 17;  // RMS Energy

inline float descriptorDimWeight(int dim)
{
    if (dim == BRIGHTNESS_DIM || dim == TEXTURE_DIM || dim == ENERGY_DIM)
        return MACRO_DIM_WEIGHT;
    return 1.0f;
}

// v1.8.0: steerable descriptor weights. The KD-tree stays plain Euclidean: each
// dimension is pre-scaled by sqrt(w) (research granular-synthesis-2026-update
// §3.5), so a weight change means building a new tree (off the audio thread,
// SearchIndex.h), never a weighted metric in the query loop.
//
// The scale of dimension d is descriptorDimWeight(d) * sqrt(m), where m is the
// user's multiplier for d's group (1 = the shipped weight). At m = 1 sqrt(1) is
// exactly 1, so the steered scale equals the legacy one bit for bit.
static constexpr int DESCRIPTOR_DIMS = 19;
using DescriptorScale = std::array<float, DESCRIPTOR_DIMS>;

// The four weight groups: timbre = MFCC 0-12 plus the uncontrolled spectral and
// time-domain dims (15, 16, 18); the three macro dims are their own groups.
enum WeightGroup : int { kWeightTimbre = 0, kWeightBrightness, kWeightTexture, kWeightEnergy, kNumWeightGroups };
using WeightMults = std::array<float, kNumWeightGroups>;

inline int weightGroupOfDim(int dim)
{
    if (dim == BRIGHTNESS_DIM) return kWeightBrightness;
    if (dim == TEXTURE_DIM)    return kWeightTexture;
    if (dim == ENERGY_DIM)     return kWeightEnergy;
    return kWeightTimbre;
}

inline DescriptorScale defaultDescriptorScale()
{
    DescriptorScale s {};
    for (int d = 0; d < DESCRIPTOR_DIMS; ++d)
        s[static_cast<size_t>(d)] = descriptorDimWeight(d);
    return s;
}

inline DescriptorScale steeredDescriptorScale(const WeightMults& mults)
{
    DescriptorScale s {};
    for (int d = 0; d < DESCRIPTOR_DIMS; ++d)
    {
        const float m = std::max(0.0f, mults[static_cast<size_t>(weightGroupOfDim(d))]);
        s[static_cast<size_t>(d)] = descriptorDimWeight(d) * std::sqrt(m);
    }
    return s;
}

struct NormalizationStats
{
    std::array<float, 19> means;
    std::array<float, 19> stddevs;

    NormalizationStats()
    {
        means.fill(0.0f);
        stddevs.fill(1.0f);  // Avoid divide-by-zero
    }
};
