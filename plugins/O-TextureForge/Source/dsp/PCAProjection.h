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
