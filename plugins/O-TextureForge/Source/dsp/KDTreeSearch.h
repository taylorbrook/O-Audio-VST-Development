/*
  ==============================================================================

    KDTreeSearch.h
    KD-tree wrapper for grain descriptor search using nanoflann

  ==============================================================================
*/

#pragma once

#include "GrainMetadata.h"
#include <nanoflann.hpp>
#include <vector>
#include <memory>
#include <cstdint>

// Adaptor for nanoflann to access GrainMetadata descriptors
class GrainDescriptorAdaptor
{
public:
    explicit GrainDescriptorAdaptor(const std::vector<GrainMetadata>& grains)
        : grains(grains) {}

    inline size_t kdtree_get_point_count() const { return grains.size(); }

    inline float kdtree_get_pt(const size_t idx, const size_t dim) const
    {
        return grains[idx].descriptors[dim] * descriptorDimWeight(static_cast<int>(dim));
    }

    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }

private:
    const std::vector<GrainMetadata>& grains;
};

class KDTreeSearch
{
public:
    static constexpr int32_t DESCRIPTOR_DIM = 19;

    using Distance = nanoflann::L2_Simple_Adaptor<float, GrainDescriptorAdaptor>;
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        Distance,
        GrainDescriptorAdaptor,
        DESCRIPTOR_DIM,
        uint32_t>;

    KDTreeSearch(const std::vector<GrainMetadata>& grains);
    ~KDTreeSearch();

    // Find nearest neighbors (allocation-free query)
    // Returns number of neighbors found (up to maxNeighbors)
    size_t findNeighbors(
        const float* queryPoint,
        size_t maxNeighbors,
        uint32_t* outIndices,
        float* outDistances) const;

private:
    GrainDescriptorAdaptor adaptor;
    std::unique_ptr<KDTree> index;
};
