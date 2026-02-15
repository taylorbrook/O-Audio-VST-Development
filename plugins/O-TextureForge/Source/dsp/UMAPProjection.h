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
