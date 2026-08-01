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

    PCAProjection.h
    PCA dimensionality reduction (19D -> 2D) using Eigen

  ==============================================================================
*/

#pragma once

#include "GrainMetadata.h"
#include <vector>

class PCAProjection
{
public:
    PCAProjection() = default;

    // Compute PCA on z-score-normalized grain descriptors
    // Output: pcaX, pcaY vectors (normalized to [0,1])
    void compute(const std::vector<GrainMetadata>& grains,
                 std::vector<float>& outX,
                 std::vector<float>& outY);
};
