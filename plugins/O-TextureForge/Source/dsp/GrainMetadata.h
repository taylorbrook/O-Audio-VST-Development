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
