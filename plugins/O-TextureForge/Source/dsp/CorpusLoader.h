/*
  ==============================================================================

    CorpusLoader.h
    Background thread for corpus analysis and grain extraction

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SharedCorpus.h"
#include "DescriptorExtractor.h"
#include "PCAProjection.h"
#include "UMAPProjection.h"
#include <functional>
#include <memory>
#include <string>
#include <atomic>

class CorpusLoader : public juce::Thread
{
public:
    using CompletionCallback = std::function<void(std::shared_ptr<SharedCorpus>)>;
    using ProgressCallback = std::function<void(float)>;  // UMAP progress 0-1
    using FailureCallback = std::function<void(const juce::String&)>;

    CorpusLoader();
    ~CorpusLoader() override;

    // Load and analyze audio file
    void loadFile(const juce::File& file, double targetSampleRate,
                  CompletionCallback callback,
                  ProgressCallback umapProgress = nullptr,
                  FailureCallback failureCallback = nullptr);

    // Cancel current operation
    void cancelLoad();

    // Cancel only UMAP (PCA layout preserved)
    void cancelUmap();

private:
    void run() override;

    bool loadAudioFile(const juce::File& file, juce::AudioBuffer<float>& buffer, double& sampleRate);
    void downmixToMono(juce::AudioBuffer<float>& buffer);
    void resampleToTargetRate(juce::AudioBuffer<float>& buffer, double sourceSR, double targetSR);
    void segmentAndAnalyze(
        const juce::AudioBuffer<float>& audioData,
        double sampleRate,
        std::vector<GrainMetadata>& outGrains,
        NormalizationStats& outNormStats);

    juce::File pendingFile;
    double targetSampleRate = 44100.0;
    CompletionCallback completionCallback;
    ProgressCallback umapProgressCallback;
    FailureCallback failureCallback;
    std::atomic<bool> umapCancelRequested { false };
    DescriptorExtractor descriptorExtractor;
    PCAProjection pcaProjection;
    UMAPProjection umapProjection;

    static constexpr int DEFAULT_GRAIN_SIZE_MS = 50;
    static constexpr int DEFAULT_HOP_SIZE_MS = 25;
};
