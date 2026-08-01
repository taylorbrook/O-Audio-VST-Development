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

#include <array>
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
