/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

    // A fresh session starts with no rotation state: any shadow left from the
    // previous table is the wrong geometry and the wrong content.
    shadowTable.reset();
    dirtyFrames.clear();
    coolingResync.clear();
}

void WavetableEditor::clearWorkingTable()
{
    workingTable.reset();
    shadowTable.reset();
    dirtyFrames.clear();
    coolingResync.clear();
}

std::unique_ptr<WavetableData> WavetableEditor::releaseShadowTable()
{
    dirtyFrames.clear();
    coolingResync.clear();
    return std::move (shadowTable);
}

// ═══════════════════════════════════════════════════════════════════
// Copy-on-write publish rotation (CR-01, coalesced per REG-01)
//
// startEditing() publishes getWorkingTable() to userTablePtrA/B, so from that
// moment the audio thread reads this exact buffer every block for live
// preview. Mutating it in place raced the render: the voice saw level-0
// samples and mipmap levels that were half-old and half-new. Every operation
// below therefore writes into the PRIVATE shadow buffer, and the publish is a
// pointer swap performed by the processor.
//
// The first shape of this fix cloned the whole 20 MiB table per operation. The
// harmonic editor drives operations from requestAnimationFrame, so a drag ran
// that at 60 Hz — 1.2 GB/s of allocation, against a 500 ms reaper that let
// ~30 of them (630 MB) pile up before the first sweep. Worse, the reaper's
// +2-generation rule keys off processBlock, so a host that idles the audio
// thread while the editor is open never expired any of them at all.
//
// Now: at most one buffer is ever cooling, an edit that cannot publish yet
// coalesces into the shadow for free, and a returned buffer is repaired on
// only the frames that diverged.
// ═══════════════════════════════════════════════════════════════════

WavetableData* WavetableEditor::acquireShadow()
{
    if (workingTable == nullptr)
        return nullptr;

    if (shadowTable == nullptr)
    {
        // Cold path: the previous shadow is still cooling. Allocate — this is
        // the old per-edit cost, but it is now paid at most once per publish
        // cycle instead of once per operation, and not at all whenever the
        // cooled buffer is back before the next edit (every buffer size where
        // two blocks are shorter than a display frame).
        shadowTable = std::make_unique<WavetableData>();
        shadowTable->numFrames = workingTable->numFrames;
        shadowTable->data = workingTable->data;   // all 10 levels + guard samples
        ++shadowAllocations;
    }

    return shadowTable.get();
}

void WavetableEditor::markDirty (int frame)
{
    if (workingTable == nullptr || frame < 0 || frame >= workingTable->numFrames)
        return;

    if (std::find (dirtyFrames.begin(), dirtyFrames.end(), frame) == dirtyFrames.end())
        dirtyFrames.push_back (frame);
}

void WavetableEditor::markDirty (const std::vector<int>& frames)
{
    for (int f : frames)
        markDirty (f);
}

void WavetableEditor::copyFramesInto (WavetableData& dst, const std::vector<int>& frames) const
{
    if (workingTable == nullptr)
        return;

    for (int frame : frames)
    {
        if (frame < 0 || frame >= workingTable->numFrames || frame >= dst.numFrames)
            continue;

        for (int level = 0; level < WavetableData::kNumMipmapLevels; ++level)
        {
            const float* src = workingTable->getFrameData (level, frame);
            float* out = dst.getFrameData (level, frame);
            std::copy (src, src + WavetableData::kFrameSize, out);
        }
    }
}

std::unique_ptr<WavetableData> WavetableEditor::commitPendingEdits()
{
    if (dirtyFrames.empty() || shadowTable == nullptr || workingTable == nullptr)
        return nullptr;

    auto displaced = std::move (workingTable);
    workingTable = std::move (shadowTable);   // shadowTable is now null

    // `displaced` differs from the new live table on exactly these frames —
    // that is what returnCooledTable() repairs when it comes back.
    coolingResync = std::move (dirtyFrames);
    dirtyFrames.clear();

    return displaced;
}

void WavetableEditor::returnCooledTable (std::unique_ptr<WavetableData> cooled)
{
    if (cooled == nullptr)
        return;

    // A shadow already exists when an edit arrived during cooling and took the
    // allocating branch of acquireShadow(). That buffer carries live edits, so
    // it keeps its place and the returned one is freed here (message thread —
    // the audio thread provably cannot reach it, which is why it came back).
    // Geometry is re-checked because loadTable() can have swapped tables under
    // a buffer that was already in flight.
    if (shadowTable != nullptr || workingTable == nullptr
        || cooled->numFrames != workingTable->numFrames
        || cooled->data.size() != workingTable->data.size())
    {
        coolingResync.clear();
        return;
    }

    copyFramesInto (*cooled, coolingResync);
    coolingResync.clear();
    shadowTable = std::move (cooled);
}

std::vector<float> WavetableEditor::getFrameHarmonics (int frameIndex, int numBins) const
{
    // latest(), not workingTable: an edit still coalesced in the shadow must
    // be what the UI reads back, or the harmonic display snaps to the last
    // published state mid-drag (REG-01).
    const auto* source = latest();

    if (source == nullptr || frameIndex < 0 || frameIndex >= source->numFrames)
        return {};

    numBins = std::min (numBins, kFFTSize / 2);

    std::vector<float> fftBuffer (static_cast<size_t> (kFFTSize * 2), 0.0f);
    const float* frameData = source->getFrameData (0, frameIndex);
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

    auto* next = acquireShadow();   // private buffer — never the one the voices read
    if (next == nullptr)
        return;

    // Forward FFT to get current phase information
    std::vector<float> fftBuffer (static_cast<size_t> (kFFTSize * 2), 0.0f);
    const float* frameData = next->getFrameData (0, frameIndex);
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

    // Store in the clone's level 0
    float* dest = next->getFrameData (0, frameIndex);
    std::copy (fftBuffer.begin(), fftBuffer.begin() + kFFTSize, dest);

    // Regenerate mipmaps for this frame only
    WavetableGenerator::generateMipmapsForFrame (*next, frameIndex);

    markDirty (frameIndex);
}

std::vector<float> WavetableEditor::getFrameWaveform (int frameIndex) const
{
    const auto* source = latest();

    if (source == nullptr || frameIndex < 0 || frameIndex >= source->numFrames)
        return {};

    const float* data = source->getFrameData (0, frameIndex);
    return { data, data + kFFTSize };
}

std::vector<std::vector<float>> WavetableEditor::getAllFrameWaveforms (int samplesPerFrame) const
{
    const auto* source = latest();

    if (source == nullptr)
        return {};

    samplesPerFrame = std::max (2, samplesPerFrame);
    std::vector<std::vector<float>> result;
    result.reserve (static_cast<size_t> (source->numFrames));

    for (int frame = 0; frame < source->numFrames; ++frame)
    {
        const float* data = source->getFrameData (0, frame);
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
    auto* next = acquireShadow();   // private buffer — never the one the voices read
    if (next == nullptr)
        return;

    if (perFrame)
    {
        for (int fi : frames)
        {
            if (fi < 0 || fi >= next->numFrames) continue;
            float* data = next->getFrameData (0, fi);
            float peak = 0.0f;
            for (int i = 0; i < kFFTSize; ++i)
                peak = std::max (peak, std::abs (data[i]));
            if (peak > 0.0f)
            {
                float gain = 1.0f / peak;
                for (int i = 0; i < kFFTSize; ++i)
                    data[i] *= gain;
            }
            WavetableGenerator::generateMipmapsForFrame (*next, fi);
        }
    }
    else
    {
        // Global normalization: find peak across all selected frames
        float globalPeak = 0.0f;
        for (int fi : frames)
        {
            if (fi < 0 || fi >= next->numFrames) continue;
            const float* data = next->getFrameData (0, fi);
            for (int i = 0; i < kFFTSize; ++i)
                globalPeak = std::max (globalPeak, std::abs (data[i]));
        }

        if (globalPeak > 0.0f)
        {
            float gain = 1.0f / globalPeak;
            for (int fi : frames)
            {
                if (fi < 0 || fi >= next->numFrames) continue;
                float* data = next->getFrameData (0, fi);
                for (int i = 0; i < kFFTSize; ++i)
                    data[i] *= gain;
                WavetableGenerator::generateMipmapsForFrame (*next, fi);
            }
        }
    }

    markDirty (frames);
}

void WavetableEditor::fadeEdges (const std::vector<int>& frames, float fadePercent)
{
    if (! workingTable)
        return;

    fadePercent = juce::jlimit (0.0f, 50.0f, fadePercent);
    int fadeSamples = static_cast<int> (kFFTSize * fadePercent / 100.0f);
    if (fadeSamples < 1) return;

    auto* next = acquireShadow();   // private buffer — never the one the voices read
    if (next == nullptr)
        return;

    for (int fi : frames)
    {
        if (fi < 0 || fi >= next->numFrames) continue;
        float* data = next->getFrameData (0, fi);

        // Fade in
        for (int i = 0; i < fadeSamples; ++i)
            data[i] *= static_cast<float> (i) / static_cast<float> (fadeSamples);

        // Fade out
        for (int i = 0; i < fadeSamples; ++i)
            data[kFFTSize - 1 - i] *= static_cast<float> (i) / static_cast<float> (fadeSamples);

        WavetableGenerator::generateMipmapsForFrame (*next, fi);
    }

    markDirty (frames);
}

void WavetableEditor::reverseFrames (const std::vector<int>& frames)
{
    auto* next = acquireShadow();   // private buffer — never the one the voices read
    if (next == nullptr)
        return;

    for (int fi : frames)
    {
        if (fi < 0 || fi >= next->numFrames) continue;
        float* data = next->getFrameData (0, fi);
        std::reverse (data, data + kFFTSize);
        WavetableGenerator::generateMipmapsForFrame (*next, fi);
    }

    markDirty (frames);
}

void WavetableEditor::reverseOrder (const std::vector<int>& frameIndices)
{
    if (! workingTable || frameIndices.size() < 2)
        return;

    auto* next = acquireShadow();   // private buffer — never the one the voices read
    if (next == nullptr)
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
        if (a < 0 || a >= next->numFrames || b < 0 || b >= next->numFrames)
            continue;

        float* dataA = next->getFrameData (0, a);
        float* dataB = next->getFrameData (0, b);

        std::copy (dataA, dataA + kFFTSize, tempFrame.begin());
        std::copy (dataB, dataB + kFFTSize, dataA);
        std::copy (tempFrame.begin(), tempFrame.end(), dataB);

        WavetableGenerator::generateMipmapsForFrame (*next, a);
        WavetableGenerator::generateMipmapsForFrame (*next, b);
    }

    markDirty (frameIndices);
}

void WavetableEditor::smoothFrames (const std::vector<int>& frames, float strength)
{
    auto* next = acquireShadow();   // private buffer — never the one the voices read
    if (next == nullptr)
        return;

    // IN-12: strength 1.0 = maximum smoothing (lowest cutoff), 0.0 = no-op.
    // The cutoff maps inversely to strength — previously it was inverted
    // (0.0 kept 1 harmonic), which never surfaced because the only caller
    // passes the symmetric 0.5.
    strength = juce::jlimit (0.0f, 1.0f, strength);
    int cutoffHarmonic = std::max (1, static_cast<int> ((1.0f - strength) * (kFFTSize / 2)));

    std::vector<float> fftBuffer (static_cast<size_t> (kFFTSize * 2), 0.0f);

    for (int fi : frames)
    {
        if (fi < 0 || fi >= next->numFrames) continue;
        float* frameData = next->getFrameData (0, fi);

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

        WavetableGenerator::generateMipmapsForFrame (*next, fi);
    }

    markDirty (frames);
}

bool WavetableEditor::saveAsUserWavetable (const juce::String& nameIn, UserWavetableManager& manager,
                                           std::unique_ptr<WavetableData>& replacedOut)
{
    replacedOut = nullptr;

    // The name arrives from the WebView — sanitize before it becomes a file
    // path, or "../../Desktop/x" writes a .wav outside the wavetable dir (WR-10)
    // latest(), not workingTable: saving mid-gesture must write what the user
    // is looking at. Sourcing the published table would silently drop every
    // edit still coalesced in the shadow (REG-01).
    const auto* source = latest();

    const auto name = UserWavetableManager::legalTableName (nameIn);
    if (source == nullptr || name.isEmpty())
        return false;

    // Save as WAV to user directory using manager's save infrastructure
    auto dir = manager.getWavetableDirectory();

    // Build concatenated frame buffer
    int totalSamples = source->numFrames * kFFTSize;
    juce::AudioBuffer<float> buffer (1, totalSamples);

    for (int frame = 0; frame < source->numFrames; ++frame)
    {
        const float* frameData = source->getFrameData (0, frame);
        int startSample = frame * kFFTSize;
        for (int i = 0; i < kFFTSize; ++i)
            buffer.setSample (0, startSample + i, frameData[i]);
    }

    auto destFile = dir.getChildFile (name + ".wav");

    // FileOutputStream positions at end-of-file — delete first or an
    // overwrite appends a second WAV and the re-import below reads the
    // ORIGINAL header at offset 0, silently discarding this edit.
    destFile.deleteFile();

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

    // Register just the saved table with the manager. Never loadFromDisk()
    // here — that destroys EVERY user WavetableData while the audio thread
    // may still hold pointers to them (CR-03).
    return manager.replaceOrInsertFromFile (name, destFile, replacedOut);
}

int WavetableEditor::getNumFrames() const
{
    const auto* source = latest();
    return source != nullptr ? source->numFrames : 0;
}
