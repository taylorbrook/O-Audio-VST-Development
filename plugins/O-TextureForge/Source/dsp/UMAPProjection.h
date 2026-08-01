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

    UMAPProjection.h
    UMAP dimensionality reduction wrapper (19D -> 2D) using umappp

  ==============================================================================
*/

#pragma once

#include "GrainMetadata.h"
#include <vector>
#include <functional>

// Must be called on the main thread before any background thread uses Eigen.
// Forces Eigen's internal cache-size static to initialize, avoiding a PAC trap
// when __cxa_guard_acquire runs on arm64 plugin threads in arm64e AU hosts.
void initEigenOnMainThread();

class UMAPProjection
{
public:
    using ProgressCallback = std::function<void(float)>;

    UMAPProjection() = default;

    // Compute UMAP on z-score-normalized grain descriptors
    // progressCallback receives values 0.0-1.0
    // shouldCancel should return true to abort
    // Output: umapX, umapY vectors (normalized to [0,1])
    void compute(const std::vector<GrainMetadata>& grains,
                 std::vector<float>& outX,
                 std::vector<float>& outY,
                 ProgressCallback progressCallback = nullptr,
                 std::function<bool()> shouldCancel = nullptr);
};
