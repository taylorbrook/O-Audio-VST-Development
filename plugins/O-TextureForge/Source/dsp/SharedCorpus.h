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
