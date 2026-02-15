/*
  ==============================================================================

    KDTreeSearch.cpp
    KD-tree search implementation

  ==============================================================================
*/

#include "KDTreeSearch.h"

KDTreeSearch::KDTreeSearch(const std::vector<GrainMetadata>& grains)
    : adaptor(grains)
{
    index = std::make_unique<KDTree>(
        DESCRIPTOR_DIM,
        adaptor,
        nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */)
    );

    index->buildIndex();
}

KDTreeSearch::~KDTreeSearch() = default;

size_t KDTreeSearch::findNeighbors(
    const float* queryPoint,
    size_t maxNeighbors,
    uint32_t* outIndices,
    float* outDistances) const
{
    nanoflann::KNNResultSet<float, uint32_t> resultSet(maxNeighbors);
    resultSet.init(outIndices, outDistances);

    nanoflann::SearchParameters searchParams;
    index->findNeighbors(resultSet, queryPoint, searchParams);

    return resultSet.size();
}
