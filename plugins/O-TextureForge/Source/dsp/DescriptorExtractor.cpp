/*
  ==============================================================================

    DescriptorExtractor.cpp
    19D descriptor extraction implementation

  ==============================================================================
*/

#include "DescriptorExtractor.h"
#include <cmath>
#include <algorithm>

DescriptorExtractor::DescriptorExtractor()
{
    previousMagnitudes.clear();
}

std::array<float, 19> DescriptorExtractor::extract(const float* frame, int frameSize, double sampleRate)
{
    std::array<float, 19> descriptor;

    // Extract 13 MFCCs (indices 0-12)
    auto mfccs = mfccExtractor.extractMFCCs(frame, frameSize);
    std::copy(mfccs.begin(), mfccs.end(), descriptor.begin());

    // Get magnitude spectrum from MFCC extractor (reuse FFT)
    const auto& magnitudes = mfccExtractor.getMagnitudeSpectrum();

    // Spectral features (indices 13-16)
    descriptor[13] = computeSpectralCentroid(magnitudes, sampleRate);
    descriptor[14] = computeSpectralFlatness(magnitudes);
    descriptor[15] = computeSpectralFlux(magnitudes);
    descriptor[16] = computeSpectralRolloff(magnitudes, sampleRate);

    // Time-domain features from RAW samples (indices 17-18)
    descriptor[17] = computeRMS(frame, frameSize);
    descriptor[18] = computeZCR(frame, frameSize);

    return descriptor;
}

void DescriptorExtractor::computeNormalizationStats(
    std::vector<GrainMetadata>& grains,
    std::array<float, 19>& outMeans,
    std::array<float, 19>& outStddevs)
{
    if (grains.empty())
    {
        outMeans.fill(0.0f);
        outStddevs.fill(1.0f);
        return;
    }

    // First pass: compute means
    outMeans.fill(0.0f);
    for (const auto& grain : grains)
    {
        for (size_t d = 0; d < 19; ++d)
            outMeans[d] += grain.descriptors[d];
    }
    for (size_t d = 0; d < 19; ++d)
        outMeans[d] /= static_cast<float>(grains.size());

    // Second pass: compute standard deviations
    outStddevs.fill(0.0f);
    for (const auto& grain : grains)
    {
        for (size_t d = 0; d < 19; ++d)
        {
            float diff = grain.descriptors[d] - outMeans[d];
            outStddevs[d] += diff * diff;
        }
    }
    for (size_t d = 0; d < 19; ++d)
    {
        outStddevs[d] = std::sqrt(outStddevs[d] / static_cast<float>(grains.size()));
        if (outStddevs[d] < 1e-6f)  // Guard against zero stddev
            outStddevs[d] = 1.0f;
    }
}

void DescriptorExtractor::normalizeDescriptors(
    std::vector<GrainMetadata>& grains,
    const std::array<float, 19>& means,
    const std::array<float, 19>& stddevs)
{
    for (auto& grain : grains)
    {
        for (size_t d = 0; d < 19; ++d)
        {
            grain.descriptors[d] = (grain.descriptors[d] - means[d]) / stddevs[d];
        }
    }
}

float DescriptorExtractor::computeSpectralCentroid(const std::vector<float>& magnitudes, double sampleRate) const
{
    float numerator = 0.0f;
    float denominator = 0.0f;

    for (size_t i = 0; i < magnitudes.size(); ++i)
    {
        float freq = static_cast<float>(i * sampleRate) / (2.0f * static_cast<float>(magnitudes.size() - 1));
        numerator += freq * magnitudes[i];
        denominator += magnitudes[i];
    }

    return (denominator > 1e-10f) ? (numerator / denominator) : 0.0f;
}

float DescriptorExtractor::computeSpectralFlatness(const std::vector<float>& magnitudes) const
{
    float geometricMean = 0.0f;
    float arithmeticMean = 0.0f;
    int count = 0;

    for (float mag : magnitudes)
    {
        if (mag > 1e-10f)
        {
            geometricMean += std::log(mag);
            arithmeticMean += mag;
            ++count;
        }
    }

    if (count == 0)
        return 0.0f;

    geometricMean = std::exp(geometricMean / static_cast<float>(count));
    arithmeticMean /= static_cast<float>(count);

    return (arithmeticMean > 1e-10f) ? (geometricMean / arithmeticMean) : 0.0f;
}

float DescriptorExtractor::computeSpectralFlux(const std::vector<float>& magnitudes)
{
    if (previousMagnitudes.empty())
    {
        previousMagnitudes = magnitudes;
        return 0.0f;
    }

    float flux = 0.0f;
    size_t minSize = std::min(magnitudes.size(), previousMagnitudes.size());
    for (size_t i = 0; i < minSize; ++i)
    {
        float diff = magnitudes[i] - previousMagnitudes[i];
        flux += diff * diff;
    }

    previousMagnitudes = magnitudes;
    return std::sqrt(flux);
}

float DescriptorExtractor::computeSpectralRolloff(const std::vector<float>& magnitudes, double sampleRate) const
{
    float totalEnergy = 0.0f;
    for (float mag : magnitudes)
        totalEnergy += mag;

    float threshold = 0.85f * totalEnergy;
    float cumEnergy = 0.0f;

    for (size_t i = 0; i < magnitudes.size(); ++i)
    {
        cumEnergy += magnitudes[i];
        if (cumEnergy >= threshold)
        {
            float freq = static_cast<float>(i * sampleRate) / (2.0f * static_cast<float>(magnitudes.size() - 1));
            return freq;
        }
    }

    return static_cast<float>(sampleRate) / 2.0f;  // Nyquist
}

float DescriptorExtractor::computeRMS(const float* frame, int frameSize) const
{
    float sum = 0.0f;
    for (int i = 0; i < frameSize; ++i)
        sum += frame[i] * frame[i];
    return std::sqrt(sum / static_cast<float>(frameSize));
}

float DescriptorExtractor::computeZCR(const float* frame, int frameSize) const
{
    if (frameSize < 2)
        return 0.0f;

    int zeroCrossings = 0;
    for (int i = 1; i < frameSize; ++i)
    {
        if ((frame[i - 1] >= 0.0f && frame[i] < 0.0f) ||
            (frame[i - 1] < 0.0f && frame[i] >= 0.0f))
        {
            ++zeroCrossings;
        }
    }

    return static_cast<float>(zeroCrossings) / static_cast<float>(frameSize - 1);
}
