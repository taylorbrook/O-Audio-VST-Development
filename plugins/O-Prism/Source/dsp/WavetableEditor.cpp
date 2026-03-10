/*
  ==============================================================================

    WavetableEditor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "WavetableEditor.h"
#include <cmath>
#include <algorithm>

static constexpr int kFFTSize = WavetableData::kTableSize; // 2048

WavetableEditor::WavetableEditor() = default;

void WavetableEditor::loadTable (const WavetableData* sourceTable)
{
    if (sourceTable == nullptr || sourceTable->numFrames == 0)
        return;

    workingTable = std::make_unique<WavetableData>();
    workingTable->allocate (sourceTable->numFrames);

    // Deep-copy level 0 data only, then regenerate mipmaps
    for (int frame = 0; frame < sourceTable->numFrames; ++frame)
    {
        const float* src = sourceTable->getFrameData (0, frame);
        float* dst = workingTable->getFrameData (0, frame);
        std::copy (src, src + kFFTSize, dst);
    }

    WavetableGenerator::generateMipmaps (*workingTable);
}

void WavetableEditor::clearWorkingTable()
{
    workingTable.reset();
}

std::vector<float> WavetableEditor::getFrameHarmonics (int frameIndex, int numBins) const
{
    if (! workingTable || frameIndex < 0 || frameIndex >= workingTable->numFrames)
        return {};

    numBins = std::min (numBins, kFFTSize / 2);

    std::vector<float> fftBuffer (static_cast<size_t> (kFFTSize * 2), 0.0f);
    const float* frameData = workingTable->getFrameData (0, frameIndex);
    std::copy (frameData, frameData + kFFTSize, fftBuffer.begin());

    // Use a non-const copy of the FFT (JUCE FFT is mutable-safe but needs non-const)
    juce::dsp::FFT tempFFT (11);
    tempFFT.performRealOnlyForwardTransform (fftBuffer.data(), true);

    // Extract magnitudes for bins 1..numBins (skip DC)
    std::vector<float> magnitudes (static_cast<size_t> (numBins));
    float maxMag = 0.0f;

    for (int k = 1; k <= numBins; ++k)
    {
        float re = fftBuffer[static_cast<size_t> (k * 2)];
        float im = fftBuffer[static_cast<size_t> (k * 2 + 1)];
        float mag = std::sqrt (re * re + im * im);
        magnitudes[static_cast<size_t> (k - 1)] = mag;
        maxMag = std::max (maxMag, mag);
    }

    // Normalize to 0..1
    if (maxMag > 0.0f)
        for (auto& m : magnitudes)
            m /= maxMag;

    return magnitudes;
}

void WavetableEditor::setFrameHarmonics (int frameIndex, const std::vector<float>& magnitudes)
{
    if (! workingTable || frameIndex < 0 || frameIndex >= workingTable->numFrames)
        return;

    int numBins = static_cast<int> (magnitudes.size());
    if (numBins == 0)
        return;

    // Forward FFT to get current phase information
    std::vector<float> fftBuffer (static_cast<size_t> (kFFTSize * 2), 0.0f);
    const float* frameData = workingTable->getFrameData (0, frameIndex);
    std::copy (frameData, frameData + kFFTSize, fftBuffer.begin());

    fft.performRealOnlyForwardTransform (fftBuffer.data(), false);

    // Find the max magnitude in the input to scale properly
    float maxInputMag = 0.0f;
    for (const auto& m : magnitudes)
        maxInputMag = std::max (maxInputMag, m);

    // Get current max magnitude for scaling reference
    float currentMaxMag = 0.0f;
    for (int k = 1; k <= kFFTSize / 2; ++k)
    {
        float re = fftBuffer[static_cast<size_t> (k * 2)];
        float im = fftBuffer[static_cast<size_t> (k * 2 + 1)];
        currentMaxMag = std::max (currentMaxMag, std::sqrt (re * re + im * im));
    }

    // Scale factor: user magnitudes are 0..1 normalized
    float scaleFactor = (currentMaxMag > 0.0f) ? currentMaxMag : 1.0f;

    // Apply new magnitudes while preserving phase
    for (int k = 1; k <= numBins; ++k)
    {
        float re = fftBuffer[static_cast<size_t> (k * 2)];
        float im = fftBuffer[static_cast<size_t> (k * 2 + 1)];
        float phase = std::atan2 (im, re);
        float newMag = magnitudes[static_cast<size_t> (k - 1)] * scaleFactor;

        fftBuffer[static_cast<size_t> (k * 2)] = newMag * std::cos (phase);
        fftBuffer[static_cast<size_t> (k * 2 + 1)] = newMag * std::sin (phase);
    }

    // Zero bins above numBins
    for (int k = numBins + 1; k <= kFFTSize / 2; ++k)
    {
        fftBuffer[static_cast<size_t> (k * 2)] = 0.0f;
        fftBuffer[static_cast<size_t> (k * 2 + 1)] = 0.0f;
    }

    // Zero DC
    fftBuffer[0] = 0.0f;
    fftBuffer[1] = 0.0f;

    // Mirror negative frequencies
    for (int k = 1; k < kFFTSize / 2; ++k)
    {
        int negBin = kFFTSize - k;
        fftBuffer[static_cast<size_t> (negBin * 2)] = fftBuffer[static_cast<size_t> (k * 2)];
        fftBuffer[static_cast<size_t> (negBin * 2 + 1)] = -fftBuffer[static_cast<size_t> (k * 2 + 1)];
    }

    // Inverse FFT
    fft.performRealOnlyInverseTransform (fftBuffer.data());

    // Store in working table level 0
    float* dest = workingTable->getFrameData (0, frameIndex);
    std::copy (fftBuffer.begin(), fftBuffer.begin() + kFFTSize, dest);

    // Regenerate mipmaps for this frame only
    WavetableGenerator::generateMipmapsForFrame (*workingTable, frameIndex);
}

std::vector<float> WavetableEditor::getFrameWaveform (int frameIndex) const
{
    if (! workingTable || frameIndex < 0 || frameIndex >= workingTable->numFrames)
        return {};

    const float* data = workingTable->getFrameData (0, frameIndex);
    return { data, data + kFFTSize };
}

std::vector<std::vector<float>> WavetableEditor::getAllFrameWaveforms (int samplesPerFrame) const
{
    if (! workingTable)
        return {};

    samplesPerFrame = std::max (2, samplesPerFrame);
    std::vector<std::vector<float>> result;
    result.reserve (static_cast<size_t> (workingTable->numFrames));

    for (int frame = 0; frame < workingTable->numFrames; ++frame)
    {
        const float* data = workingTable->getFrameData (0, frame);
        std::vector<float> downsampled (static_cast<size_t> (samplesPerFrame));

        // Min/max pairs for waveform display
        int samplesPerBucket = kFFTSize / samplesPerFrame;
        for (int i = 0; i < samplesPerFrame; ++i)
        {
            int start = i * samplesPerBucket;
            int end = std::min (start + samplesPerBucket, kFFTSize);
            float minVal = data[start], maxVal = data[start];
            for (int j = start + 1; j < end; ++j)
            {
                minVal = std::min (minVal, data[j]);
                maxVal = std::max (maxVal, data[j]);
            }
            // Alternate min/max for proper waveform envelope
            downsampled[static_cast<size_t> (i)] = (i % 2 == 0) ? minVal : maxVal;
        }

        result.push_back (std::move (downsampled));
    }

    return result;
}

void WavetableEditor::normalizeFrames (const std::vector<int>& frames, bool perFrame)
{
    if (! workingTable)
        return;

    if (perFrame)
    {
        for (int fi : frames)
        {
            if (fi < 0 || fi >= workingTable->numFrames) continue;
            float* data = workingTable->getFrameData (0, fi);
            float peak = 0.0f;
            for (int i = 0; i < kFFTSize; ++i)
                peak = std::max (peak, std::abs (data[i]));
            if (peak > 0.0f)
            {
                float gain = 1.0f / peak;
                for (int i = 0; i < kFFTSize; ++i)
                    data[i] *= gain;
            }
            WavetableGenerator::generateMipmapsForFrame (*workingTable, fi);
        }
    }
    else
    {
        // Global normalization: find peak across all selected frames
        float globalPeak = 0.0f;
        for (int fi : frames)
        {
            if (fi < 0 || fi >= workingTable->numFrames) continue;
            const float* data = workingTable->getFrameData (0, fi);
            for (int i = 0; i < kFFTSize; ++i)
                globalPeak = std::max (globalPeak, std::abs (data[i]));
        }

        if (globalPeak > 0.0f)
        {
            float gain = 1.0f / globalPeak;
            for (int fi : frames)
            {
                if (fi < 0 || fi >= workingTable->numFrames) continue;
                float* data = workingTable->getFrameData (0, fi);
                for (int i = 0; i < kFFTSize; ++i)
                    data[i] *= gain;
                WavetableGenerator::generateMipmapsForFrame (*workingTable, fi);
            }
        }
    }
}

void WavetableEditor::fadeEdges (const std::vector<int>& frames, float fadePercent)
{
    if (! workingTable)
        return;

    fadePercent = juce::jlimit (0.0f, 50.0f, fadePercent);
    int fadeSamples = static_cast<int> (kFFTSize * fadePercent / 100.0f);
    if (fadeSamples < 1) return;

    for (int fi : frames)
    {
        if (fi < 0 || fi >= workingTable->numFrames) continue;
        float* data = workingTable->getFrameData (0, fi);

        // Fade in
        for (int i = 0; i < fadeSamples; ++i)
            data[i] *= static_cast<float> (i) / static_cast<float> (fadeSamples);

        // Fade out
        for (int i = 0; i < fadeSamples; ++i)
            data[kFFTSize - 1 - i] *= static_cast<float> (i) / static_cast<float> (fadeSamples);

        WavetableGenerator::generateMipmapsForFrame (*workingTable, fi);
    }
}

void WavetableEditor::reverseFrames (const std::vector<int>& frames)
{
    if (! workingTable)
        return;

    for (int fi : frames)
    {
        if (fi < 0 || fi >= workingTable->numFrames) continue;
        float* data = workingTable->getFrameData (0, fi);
        std::reverse (data, data + kFFTSize);
        WavetableGenerator::generateMipmapsForFrame (*workingTable, fi);
    }
}

void WavetableEditor::reverseOrder (const std::vector<int>& frameIndices)
{
    if (! workingTable || frameIndices.size() < 2)
        return;

    // Sort indices to determine order
    auto sorted = frameIndices;
    std::sort (sorted.begin(), sorted.end());

    // Swap frame data between first/last, second/second-last, etc.
    std::vector<float> tempFrame (kFFTSize);
    for (size_t i = 0; i < sorted.size() / 2; ++i)
    {
        int a = sorted[i];
        int b = sorted[sorted.size() - 1 - i];
        if (a < 0 || a >= workingTable->numFrames || b < 0 || b >= workingTable->numFrames)
            continue;

        float* dataA = workingTable->getFrameData (0, a);
        float* dataB = workingTable->getFrameData (0, b);

        std::copy (dataA, dataA + kFFTSize, tempFrame.begin());
        std::copy (dataB, dataB + kFFTSize, dataA);
        std::copy (tempFrame.begin(), tempFrame.end(), dataB);

        WavetableGenerator::generateMipmapsForFrame (*workingTable, a);
        WavetableGenerator::generateMipmapsForFrame (*workingTable, b);
    }
}

void WavetableEditor::smoothFrames (const std::vector<int>& frames, float strength)
{
    if (! workingTable)
        return;

    strength = juce::jlimit (0.0f, 1.0f, strength);
    int cutoffHarmonic = std::max (1, static_cast<int> (strength * (kFFTSize / 2)));

    std::vector<float> fftBuffer (static_cast<size_t> (kFFTSize * 2), 0.0f);

    for (int fi : frames)
    {
        if (fi < 0 || fi >= workingTable->numFrames) continue;
        float* frameData = workingTable->getFrameData (0, fi);

        std::copy (frameData, frameData + kFFTSize, fftBuffer.begin());
        std::fill (fftBuffer.begin() + kFFTSize, fftBuffer.end(), 0.0f);

        fft.performRealOnlyForwardTransform (fftBuffer.data(), false);

        // Apply 6dB/oct rolloff above cutoff harmonic
        for (int k = cutoffHarmonic; k <= kFFTSize / 2; ++k)
        {
            float rolloff = static_cast<float> (cutoffHarmonic) / static_cast<float> (k);
            fftBuffer[static_cast<size_t> (k * 2)] *= rolloff;
            fftBuffer[static_cast<size_t> (k * 2 + 1)] *= rolloff;

            // Mirror
            int negBin = kFFTSize - k;
            if (negBin > 0 && negBin < kFFTSize)
            {
                fftBuffer[static_cast<size_t> (negBin * 2)] *= rolloff;
                fftBuffer[static_cast<size_t> (negBin * 2 + 1)] *= rolloff;
            }
        }

        fft.performRealOnlyInverseTransform (fftBuffer.data());
        std::copy (fftBuffer.begin(), fftBuffer.begin() + kFFTSize, frameData);

        WavetableGenerator::generateMipmapsForFrame (*workingTable, fi);
    }
}

bool WavetableEditor::saveAsUserWavetable (const juce::String& name, UserWavetableManager& manager)
{
    if (! workingTable || name.isEmpty())
        return false;

    // Save as WAV to user directory using manager's save infrastructure
    auto dir = manager.getWavetableDirectory();

    // Build concatenated frame buffer
    int totalSamples = workingTable->numFrames * kFFTSize;
    juce::AudioBuffer<float> buffer (1, totalSamples);

    for (int frame = 0; frame < workingTable->numFrames; ++frame)
    {
        const float* frameData = workingTable->getFrameData (0, frame);
        int startSample = frame * kFFTSize;
        for (int i = 0; i < kFFTSize; ++i)
            buffer.setSample (0, startSample + i, frameData[i]);
    }

    auto destFile = dir.getChildFile (name + ".wav");

    auto outputStream = std::make_unique<juce::FileOutputStream> (destFile);
    if (! outputStream->openedOk())
        return false;

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (outputStream.get(), 44100.0, 1, 32, {}, 0));

    if (! writer)
        return false;

    outputStream.release();
    if (! writer->writeFromAudioSampleBuffer (buffer, 0, totalSamples))
        return false;

    writer.reset();

    // Reload from disk so manager knows about it
    manager.loadFromDisk();
    return true;
}

int WavetableEditor::getNumFrames() const
{
    return workingTable ? workingTable->numFrames : 0;
}
