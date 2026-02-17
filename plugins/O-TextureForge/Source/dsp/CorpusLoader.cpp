/*
  ==============================================================================

    CorpusLoader.cpp
    Background corpus loading implementation

  ==============================================================================
*/

#include "CorpusLoader.h"
#include "KDTreeSearch.h"
#include <cmath>

CorpusLoader::CorpusLoader()
    : juce::Thread("CorpusLoader")
{
}

CorpusLoader::~CorpusLoader()
{
    stopThread(5000);
}

void CorpusLoader::loadFile(const juce::File& file, double targetSR,
                            CompletionCallback callback,
                            ProgressCallback umapProgress,
                            FailureCallback onFailure)
{
    cancelLoad();

    // Guard against startThread assertion if thread didn't exit in time
    if (isThreadRunning())
    {
        juce::Logger::writeToLog("CorpusLoader: previous thread still running, skipping new load");
        if (onFailure)
        {
            juce::MessageManager::callAsync([onFailure]() {
                onFailure("Previous load still in progress");
            });
        }
        return;
    }

    pendingFile = file;
    targetSampleRate = targetSR;
    completionCallback = callback;
    umapProgressCallback = umapProgress;
    failureCallback = onFailure;
    umapCancelRequested.store(false, std::memory_order_relaxed);

    startThread();
}

void CorpusLoader::cancelLoad()
{
    umapCancelRequested.store(true, std::memory_order_relaxed);
    signalThreadShouldExit();
    waitForThreadToExit(5000);
}

void CorpusLoader::cancelUmap()
{
    umapCancelRequested.store(true, std::memory_order_relaxed);
}

void CorpusLoader::run()
{
    auto corpus = std::make_shared<SharedCorpus>();

    // Step 1: Load audio file
    if (threadShouldExit())
        return;

    double sourceSR = 44100.0;
    juce::AudioBuffer<float> buffer;
    if (!loadAudioFile(pendingFile, buffer, sourceSR))
    {
        juce::Logger::writeToLog("Failed to load audio file: " + pendingFile.getFullPathName());
        auto onFailure = failureCallback;
        auto fileName = pendingFile.getFileName();
        juce::MessageManager::callAsync([onFailure, fileName]() {
            if (onFailure)
                onFailure("Unsupported format: " + fileName);
        });
        return;
    }

    corpus->filePath = pendingFile.getFullPathName();

    // Step 2: Downmix to mono
    if (threadShouldExit())
        return;
    downmixToMono(buffer);

    // Step 3: Resample to target sample rate
    if (threadShouldExit())
        return;
    resampleToTargetRate(buffer, sourceSR, targetSampleRate);

    corpus->sampleRate = targetSampleRate;
    corpus->audioData = std::move(buffer);

    // Step 4: Segment and analyze grains
    if (threadShouldExit())
        return;
    segmentAndAnalyze(corpus->audioData, targetSampleRate, corpus->grains, corpus->normStats);

    // Step 5: Build KD-tree
    if (threadShouldExit())
        return;
    if (!corpus->grains.empty())
    {
        corpus->kdTree = std::make_unique<KDTreeSearch>(corpus->grains);
    }

    // Step 6: Compute PCA (instant, <10ms even for large corpora)
    if (threadShouldExit())
        return;
    if (!corpus->grains.empty())
    {
        pcaProjection.compute(corpus->grains, corpus->pcaX, corpus->pcaY);
    }

    // Step 7: Deliver PCA result to message thread (scatter plot can render immediately)
    auto completionCB = completionCallback;
    juce::MessageManager::callAsync([completionCB, corpus]()
    {
        if (completionCB)
            completionCB(corpus);
    });

    // Step 8: Compute UMAP (slower, with progress reporting)
    if (threadShouldExit() || umapCancelRequested.load(std::memory_order_relaxed))
        return;
    if (!corpus->grains.empty() && corpus->grains.size() >= 15)
    {
        auto umapCB = umapProgressCallback;
        umapProjection.compute(
            corpus->grains,
            corpus->umapX,
            corpus->umapY,
            [umapCB](float progress) {
                if (umapCB)
                {
                    juce::MessageManager::callAsync([umapCB, progress]() {
                        umapCB(progress);
                    });
                }
            },
            [this]() { return threadShouldExit() || umapCancelRequested.load(std::memory_order_relaxed); }
        );

        if (umapCancelRequested.load(std::memory_order_relaxed))
        {
            // UMAP was cancelled — PCA layout preserved, notify JS
            auto umapCB2 = umapProgressCallback;
            juce::MessageManager::callAsync([umapCB2]() {
                if (umapCB2)
                    umapCB2(-1.0f);  // -1 signals cancellation
            });
            return;
        }

        corpus->umapReady.store(true, std::memory_order_release);
    }
}

bool CorpusLoader::loadAudioFile(const juce::File& file, juce::AudioBuffer<float>& buffer, double& sampleRate)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    sampleRate = reader->sampleRate;
    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples = static_cast<int>(reader->lengthInSamples);

    buffer.setSize(numChannels, numSamples);
    reader->read(&buffer, 0, numSamples, 0, true, true);

    return true;
}

void CorpusLoader::downmixToMono(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() == 1)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float sum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sum += buffer.getSample(ch, i);
        buffer.setSample(0, i, sum / static_cast<float>(numChannels));
    }

    buffer.setSize(1, numSamples, true);
}

void CorpusLoader::resampleToTargetRate(juce::AudioBuffer<float>& buffer, double sourceSR, double targetSR)
{
    if (std::abs(sourceSR - targetSR) < 0.1)
        return;

    // speedRatio = input samples per output sample
    const double speedRatio = sourceSR / targetSR;
    const int newLength = static_cast<int>(std::ceil(buffer.getNumSamples() / speedRatio));

    juce::AudioBuffer<float> resampled(1, newLength);
    juce::LagrangeInterpolator interpolator;

    interpolator.process(speedRatio,
                         buffer.getReadPointer(0),
                         resampled.getWritePointer(0),
                         newLength,
                         buffer.getNumSamples(),
                         0);

    buffer = std::move(resampled);
}

void CorpusLoader::segmentAndAnalyze(
    const juce::AudioBuffer<float>& audioData,
    double sampleRate,
    std::vector<GrainMetadata>& outGrains,
    NormalizationStats& outNormStats)
{
    const int grainSizeSamples = static_cast<int>(sampleRate * DEFAULT_GRAIN_SIZE_MS / 1000.0);
    const int hopSizeSamples = static_cast<int>(sampleRate * DEFAULT_HOP_SIZE_MS / 1000.0);
    const int totalSamples = audioData.getNumSamples();

    outGrains.clear();

    const float* audioPtr = audioData.getReadPointer(0);

    for (int startSample = 0; startSample + grainSizeSamples <= totalSamples; startSample += hopSizeSamples)
    {
        if (threadShouldExit())
            return;

        GrainMetadata grain;
        grain.startSample = static_cast<uint32_t>(startSample);
        grain.lengthSamples = static_cast<uint32_t>(grainSizeSamples);

        // Extract 19D descriptor
        grain.descriptors = descriptorExtractor.extract(audioPtr + startSample, grainSizeSamples, sampleRate);

        outGrains.push_back(grain);
    }

    // Compute normalization statistics
    if (!outGrains.empty())
    {
        DescriptorExtractor::computeNormalizationStats(
            outGrains,
            outNormStats.means,
            outNormStats.stddevs);

        DescriptorExtractor::normalizeDescriptors(
            outGrains,
            outNormStats.means,
            outNormStats.stddevs);
    }
}
