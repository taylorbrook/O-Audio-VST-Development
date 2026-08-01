/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

    GrainScheduler.cpp
    Grain scheduler implementation

  ==============================================================================
*/

#include "GrainScheduler.h"
#include "KDTreeSearch.h"
#include <cmath>

GrainScheduler::GrainScheduler()
{
    random.setSeedRandomly();
}

void GrainScheduler::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    droneTimerSamples = 0;
    droneIntervalSamples = static_cast<int>(sampleRate * 0.05);  // 50ms default
}

void GrainScheduler::reset()
{
    grainPool.releaseAll();
    droneTimerSamples = 0;
}

void GrainScheduler::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages,
    const SharedCorpus* corpus,
    const SchedulerParams& params)
{
    const int numSamples = buffer.getNumSamples();

    // Handle MIDI input for all modes (notes for 0/1, CCs for all)
    if (corpus != nullptr)
    {
        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            handleMidiMessage(msg, params, corpus);
        }
    }

    // Generative Drone mode (mode 2)
    if (params.midiMode == 2 && corpus != nullptr)
    {
        processDroneMode(numSamples, params, corpus);
    }

    // Render active voices
    renderVoices(buffer, corpus);

    // Apply output gain
    float outputGainLinear = juce::Decibels::decibelsToGain(params.outputGainDb);
    buffer.applyGain(outputGainLinear);
}

void GrainScheduler::handleMidiMessage(const juce::MidiMessage& msg, const SchedulerParams& params, const SharedCorpus* corpus)
{
    // Note-on/off: only modes 0 (Pitch-Mapped) and 1 (Trigger+Modulate)
    if (msg.isNoteOn() && params.midiMode != 2)
    {
        // Handle velocity-0 note-on as note-off
        if (msg.getVelocity() == 0)
        {
            grainPool.releaseByNote(msg.getNoteNumber(), msg.getChannel());
            return;
        }

        const int midiNote = msg.getNoteNumber();
        const int midiChannel = msg.getChannel();
        const float velocity = msg.getFloatVelocity();

        // Mode 1: monophonic — kill previous MIDI-triggered grains
        if (params.midiMode == 1)
            grainPool.releaseAllMidiVoices();

        triggerGrain(corpus, params, midiNote, midiChannel, velocity);
    }
    else if (msg.isNoteOff() && params.midiMode != 2)
    {
        grainPool.releaseByNote(msg.getNoteNumber(), msg.getChannel());
    }
    // CC handling for all modes
    else if (msg.isController())
    {
        const int cc = msg.getControllerNumber();
        const float value = msg.getControllerValue() / 127.0f;

        if (params.midiMode == 1)
        {
            // Mode 1: CC1 (mod wheel) → energy bias for grain selection
            if (cc == 1)
                ccEnergy = value;
        }
        else if (params.midiMode == 2)
        {
            // Mode 2 (Drone): CC → parameter overrides for hardware control
            if (cc == 1)       droneCCEnergy = value;      // Mod wheel → Energy
            else if (cc == 11) droneCCBrightness = value;  // Expression → Brightness
            else if (cc == 74) droneCCTexture = value;     // Filter cutoff → Texture
        }
    }
    // Channel pressure (aftertouch) for Mode 1
    else if (msg.isChannelPressure() && params.midiMode == 1)
    {
        ccBrightness = msg.getChannelPressureValue() / 127.0f;
    }
}

void GrainScheduler::processDroneMode(int numSamples, const SchedulerParams& params, const SharedCorpus* corpus)
{
    // Update interval based on density
    float densityFactor = static_cast<float>(params.grainDensity) / 64.0f;
    droneIntervalSamples = static_cast<int>(currentSampleRate * 0.02 / densityFactor);  // 20ms to 1280ms
    droneIntervalSamples = juce::jmax(100, droneIntervalSamples);

    droneTimerSamples += numSamples;

    while (droneTimerSamples >= droneIntervalSamples)
    {
        droneTimerSamples -= droneIntervalSamples;
        triggerGrain(corpus, params, -1, -1, 1.0f);
    }
}

void GrainScheduler::triggerGrain(const SharedCorpus* corpus, const SchedulerParams& params, int midiNote, int midiChannel, float velocity)
{
    if (corpus == nullptr || corpus->grains.empty())
        return;

    // Build effective params with mode-specific overrides for grain selection
    SchedulerParams effectiveParams = params;

    if (params.midiMode == 0 && midiNote >= 0)
    {
        // Mode 0 (Pitch-Mapped): velocity biases Energy descriptor for grain selection
        effectiveParams.energy = velocity;
    }
    else if (params.midiMode == 1)
    {
        // Mode 1 (Trigger+Modulate): CC1→energy, aftertouch→brightness, velocity→variation
        effectiveParams.energy = ccEnergy;
        effectiveParams.brightness = ccBrightness;
        effectiveParams.variation = velocity * params.variation;
    }
    else if (params.midiMode == 2)
    {
        // Mode 2 (Drone): apply CC overrides if set (-1 = use APVTS value)
        if (droneCCEnergy >= 0.0f)     effectiveParams.energy = droneCCEnergy;
        if (droneCCBrightness >= 0.0f) effectiveParams.brightness = droneCCBrightness;
        if (droneCCTexture >= 0.0f)    effectiveParams.texture = droneCCTexture;
    }

    uint32_t grainIndex = queryGrainIndex(corpus, effectiveParams, random);
    triggerGrainByIndex(corpus, params, grainIndex, midiNote, midiChannel, velocity);
}

void GrainScheduler::triggerSpecificGrain(const SharedCorpus* corpus, const SchedulerParams& params, uint32_t grainIndex)
{
    if (corpus == nullptr || grainIndex >= corpus->grains.size())
        return;

    triggerGrainByIndex(corpus, params, grainIndex, -1, -1, 1.0f);
}

void GrainScheduler::triggerGrainByIndex(const SharedCorpus* corpus, const SchedulerParams& params,
                                          uint32_t grainIndex, int midiNote, int midiChannel, float velocity)
{
    const auto& grainMeta = corpus->grains[grainIndex];

    GrainVoice* voice = grainPool.allocate();
    if (voice == nullptr)
        return;

    voice->active = true;
    voice->grainIndex = grainIndex;
    voice->grainStartSample = grainMeta.startSample;

    // Apply GRAIN_SIZE parameter: override default grain length
    float grainSizeSamples = params.grainSizeMs * static_cast<float>(currentSampleRate) / 1000.0f;
    uint32_t requestedLength = static_cast<uint32_t>(grainSizeSamples);
    // Clamp to available audio (don't read past end of corpus)
    uint32_t maxLength = static_cast<uint32_t>(corpus->audioData.getNumSamples()) - grainMeta.startSample;
    voice->grainLengthSamples = std::min(requestedLength, maxLength);

    // Apply CROSSFADE: modulate envelope width via crossfade factor
    voice->crossfadeFactor = params.crossfadePercent / 100.0f;

    voice->readPosition = 0.0f;
    voice->samplesElapsed = 0;
    voice->gain = velocity;
    voice->midiNote = midiNote;
    voice->midiChannel = midiChannel;

    // Compute playback rate based on MIDI mode
    if (params.midiMode == 0 && midiNote >= 0)
    {
        float semitones = static_cast<float>(midiNote - 60);
        voice->playbackRate = std::pow(2.0f, semitones / 12.0f);
    }
    else
    {
        voice->playbackRate = 1.0f;
    }
}

uint32_t GrainScheduler::queryGrainIndex(
    const SharedCorpus* corpus,
    const SchedulerParams& params,
    juce::Random& rng) const
{
    if (corpus->grains.empty())
        return 0;

    const int totalGrains = static_cast<int>(corpus->grains.size());

    // Resolve 2D projection data for scatter position bias (prefer UMAP, fall back to PCA)
    const float* projX = nullptr;
    const float* projY = nullptr;

    if (corpus->umapReady.load(std::memory_order_acquire)
        && static_cast<int>(corpus->umapX.size()) >= totalGrains)
    {
        projX = corpus->umapX.data();
        projY = corpus->umapY.data();
    }
    else if (static_cast<int>(corpus->pcaX.size()) >= totalGrains)
    {
        projX = corpus->pcaX.data();
        projY = corpus->pcaY.data();
    }

    // Use KD-tree if available
    if (corpus->kdTree != nullptr)
    {
        // Build 19D query point — all dims neutral (0) except macro knobs
        std::array<float, 19> queryPoint;
        queryPoint.fill(0.0f);

        // Map macro knobs to their descriptor dimensions (z-score space: -1..+1)
        // Apply MACRO_DIM_WEIGHT to match the KD-tree adaptor weighting
        queryPoint[ENERGY_DIM]     = (params.energy * 2.0f - 1.0f) * MACRO_DIM_WEIGHT;
        queryPoint[BRIGHTNESS_DIM] = (params.brightness * 2.0f - 1.0f) * MACRO_DIM_WEIGHT;
        queryPoint[TEXTURE_DIM]    = (params.texture * 2.0f - 1.0f) * MACRO_DIM_WEIGHT;

        // Add variation randomization
        float variationScale = params.variation * 2.0f;
        for (auto& dim : queryPoint)
            dim += (rng.nextFloat() - 0.5f) * variationScale;

        // Query KD-tree for K nearest neighbors
        constexpr int K = 8;
        std::array<uint32_t, K> indices;
        std::array<float, K> distances;
        corpus->kdTree->findNeighbors(queryPoint.data(), K, indices.data(), distances.data());

        // Rescore candidates with temporal position + scatter position biases
        float bestScore = std::numeric_limits<float>::max();
        uint32_t bestIndex = indices[0];

        for (int i = 0; i < K; ++i)
        {
            float score = distances[i];

            // Temporal position bias
            float grainPos = static_cast<float>(indices[i]) / static_cast<float>(totalGrains);
            score += std::abs(grainPos - params.position) * 5.0f;

            // 2D scatter position bias (UMAP or PCA)
            if (projX != nullptr)
            {
                float dx = projX[indices[i]] - params.scatterX;
                float dy = projY[indices[i]] - params.scatterY;
                score += (dx * dx + dy * dy) * 10.0f;
            }

            if (score < bestScore)
            {
                bestScore = score;
                bestIndex = indices[i];
            }
        }
        return bestIndex;
    }

    // Fallback: prefer grains near scatter position in 2D projection space
    if (projX != nullptr)
    {
        float targetX = params.scatterX + (rng.nextFloat() - 0.5f) * params.variation * 0.5f;
        float targetY = params.scatterY + (rng.nextFloat() - 0.5f) * params.variation * 0.5f;

        float bestDist = std::numeric_limits<float>::max();
        uint32_t bestIndex = 0;

        for (int i = 0; i < totalGrains; ++i)
        {
            float dx = projX[i] - targetX;
            float dy = projY[i] - targetY;
            float dist = dx * dx + dy * dy;
            if (dist < bestDist)
            {
                bestDist = dist;
                bestIndex = static_cast<uint32_t>(i);
            }
        }
        return bestIndex;
    }

    // Last resort: random grain biased by temporal position
    int centerGrain = static_cast<int>(params.position * static_cast<float>(totalGrains - 1));
    int spread = static_cast<int>(params.variation * static_cast<float>(totalGrains) * 0.25f);
    spread = juce::jmax(1, spread);
    int randomOffset = rng.nextInt(spread * 2) - spread;
    int grainIdx = juce::jlimit(0, totalGrains - 1, centerGrain + randomOffset);
    return static_cast<uint32_t>(grainIdx);
}

void GrainScheduler::renderVoices(juce::AudioBuffer<float>& buffer, const SharedCorpus* corpus)
{
    if (corpus == nullptr || corpus->audioData.getNumSamples() == 0)
    {
        buffer.clear();
        return;
    }

    buffer.clear();

    auto& voices = grainPool.getVoices();

    for (auto& voice : voices)
    {
        if (!voice.active)
            continue;

        const int numSamples = buffer.getNumSamples();
        float* leftOut = buffer.getWritePointer(0);
        float* rightOut = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            if (voice.isFinished())
            {
                voice.active = false;
                break;
            }

            // Sample from corpus with linear interpolation
            float readPos = static_cast<float>(voice.grainStartSample) + voice.readPosition;
            float sample = sampleFromCorpus(corpus, readPos);

            // Apply grain envelope (Hann window)
            float envelope = voice.getEnvelope();
            float outputSample = sample * envelope * voice.gain;

            leftOut[i] += outputSample;
            if (rightOut != nullptr)
                rightOut[i] += outputSample;

            // Advance read position
            voice.readPosition += voice.playbackRate;
            ++voice.samplesElapsed;
        }
    }
}

float GrainScheduler::sampleFromCorpus(const SharedCorpus* corpus, float readPosition) const
{
    const int numSamples = corpus->audioData.getNumSamples();
    if (numSamples == 0)
        return 0.0f;

    // Clamp to valid range
    if (readPosition < 0.0f || readPosition >= static_cast<float>(numSamples - 1))
        return 0.0f;

    // Linear interpolation
    const int index0 = static_cast<int>(readPosition);
    const int index1 = index0 + 1;
    const float frac = readPosition - static_cast<float>(index0);

    const float* audioPtr = corpus->audioData.getReadPointer(0);
    const float sample0 = audioPtr[index0];
    const float sample1 = audioPtr[index1];

    return sample0 + frac * (sample1 - sample0);
}

void GrainScheduler::getActiveGrains(ActiveGrainInfo* outGrains, int& outCount) const
{
    outCount = 0;
    const auto& voices = grainPool.getVoices();

    for (const auto& voice : voices)
    {
        if (!voice.active)
            continue;

        if (outCount >= 64)
            break;

        outGrains[outCount].grainIndex = voice.grainIndex;
        outGrains[outCount].envelope = voice.getEnvelope();
        outGrains[outCount].readPositionNorm = static_cast<float>(voice.samplesElapsed) / static_cast<float>(voice.grainLengthSamples);

        ++outCount;
    }
}
