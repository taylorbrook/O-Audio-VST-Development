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

GrainScheduler::GrainScheduler() = default;

void GrainScheduler::prepare(double sampleRate, float initialOutputGainDb)
{
    currentSampleRate = sampleRate;
    grainPool.setSampleRate(sampleRate);
    outputGainSmoothed.reset(sampleRate, 0.02);
    outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(initialOutputGainDb));
    densityNormSmoothed.reset(sampleRate, 0.1);
    densityNormSmoothed.setCurrentAndTargetValue(1.0f);
    droneTimerSamples = 0;
    droneStartPending = true;
    droneIntervalSamples = static_cast<int>(sampleRate * 0.05);  // 50ms default
}

void GrainScheduler::reset()
{
    grainPool.releaseAll();
    droneTimerSamples = 0;
    droneStartPending = true;
    grainCounter = 0;
    cache.valid = false;   // v1.8.0
    recentCount = 0;
    recentHead = 0;
}

void GrainScheduler::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages,
    const SharedCorpus* corpus,
    const SchedulerParams& params)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    if (params.rngSeed != rngSeed)
    {
        rngSeed = params.rngSeed;
        grainCounter = 0;
    }

    // Generative Drone mode (mode 2): spawn interval from density
    const bool droneOn = params.midiMode == 2 && corpus != nullptr;
    if (droneOn)
    {
        float densityFactor = static_cast<float>(params.grainDensity) / 64.0f;
        droneIntervalSamples = static_cast<int>(currentSampleRate * 0.02 / densityFactor);  // 20ms to 1280ms
        droneIntervalSamples = juce::jmax(100, droneIntervalSamples);

        // The first drone grain after a reset sounds on sample 0.
        if (droneStartPending)
        {
            droneTimerSamples = droneIntervalSamples;
            droneStartPending = false;
        }
    }

    // v1.6.0: sample-accurate events. The block is rendered in segments split at
    // each MIDI event (notes for modes 0/1, CCs for all) and each drone spawn, so
    // every grain starts on its own sample. v1.5.4 handled all MIDI and spawned
    // every due drone grain at offset 0. Events at offset 0 render as before.
    buffer.clear();

    auto midiIt = midiMessages.cbegin();
    const auto midiEnd = midiMessages.cend();
    if (corpus == nullptr)
        midiIt = midiEnd;

    int pos = 0;
    for (;;)
    {
        // Late events (past the block end) are handled on the last sample.
        const int nextMidi = midiIt != midiEnd ? juce::jlimit(pos, numSamples - 1, (*midiIt).samplePosition)
                                               : numSamples;
        const int nextDrone = droneOn ? juce::jmax(pos, droneIntervalSamples - droneTimerSamples)
                                      : numSamples;
        const int next = std::min({ nextMidi, nextDrone, numSamples });

        renderVoices(buffer, corpus, pos, next);
        pos = next;
        if (pos >= numSamples)
            break;

        // MIDI first at a shared sample, so a CC there shapes that sample's drone grain.
        if (nextMidi == pos)
        {
            handleMidiMessage((*midiIt).getMessage(), params, corpus);
            ++midiIt;
            continue;
        }

        droneTimerSamples -= droneIntervalSamples;
        triggerGrain(corpus, params, -1, -1, 1.0f);
    }

    if (droneOn)
        droneTimerSamples += numSamples;

    // Drone-mode density normalisation (v1.5.4): stacked drone grains summed
    // un-normalised and the processor's ±1 safety clip flattened dense patches.
    densityNormSmoothed.setTargetValue(params.midiMode == 2 ? computeDensityNorm(params) : 1.0f);
    if (densityNormSmoothed.isSmoothing())
        densityNormSmoothed.applyGain(buffer, numSamples);
    else if (densityNormSmoothed.getCurrentValue() != 1.0f)
        buffer.applyGain(densityNormSmoothed.getCurrentValue());

    // Apply output gain — the last gain stage (v1.5.1: smoothed; was a
    // per-block step that zippered under automation).
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(params.outputGainDb));
    if (outputGainSmoothed.isSmoothing())
        outputGainSmoothed.applyGain(buffer, numSamples);
    else
        buffer.applyGain(outputGainSmoothed.getCurrentValue());
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

float GrainScheduler::envelopeMeanSquare(float crossfadeFactor)
{
    constexpr int kSteps = 512;
    double acc = 0.0;
    for (int i = 0; i < kSteps; ++i)
    {
        const float w = GrainVoice::envelopeAt((static_cast<float>(i) + 0.5f) / kSteps, crossfadeFactor);
        acc += static_cast<double>(w) * static_cast<double>(w);
    }
    return static_cast<float>(acc / kSteps);
}

float GrainScheduler::computeDensityNorm(const SchedulerParams& params)
{
    // Window power relative to pure Hann (crossfade 0), cached per crossfade.
    const float crossfade = params.crossfadePercent / 100.0f;
    if (crossfade != normCrossfadeCache)
    {
        normCrossfadeCache = crossfade;
        normPowerRatio = (crossfade == 0.0f) ? 1.0f
                                             : envelopeMeanSquare(crossfade) / envelopeMeanSquare(0.0f);
    }

    // Mean overlap at the drone spawn rate, capped by the regular pool. Grains
    // are sprayed across the corpus (incoherent), so they sum in power.
    const float sizeSamples = params.grainSizeMs * static_cast<float>(currentSampleRate) / 1000.0f;
    const float overlap = std::min(static_cast<float>(GrainPool::MAX_VOICES),
                                   sizeSamples / static_cast<float>(std::max(1, droneIntervalSamples)))
                        * normPowerRatio;
    return overlap > 1.0f ? 1.0f / std::sqrt(overlap) : 1.0f;
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

    uint32_t grainIndex = queryGrainIndex(corpus, effectiveParams, grainCounter++);
    pushRecent(grainIndex);   // v1.8.0: no effect on the pick while Repeat Avoid is 0
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

    voice->readPosition = 0.0;
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

    // v1.7.0: band-limited read for up-transposed grains (grain-engine
    // BandlimitedReader). Octave level = floor(log2 rate), sinc set from the
    // residual rate on that level; rate <= 1 keeps the linear read (bit-identical
    // to v1.6.2), since it cannot alias.
    const double rate = static_cast<double>(voice->playbackRate);
    voice->bandlimited = rate > 1.0 && corpus->mip != nullptr;
    voice->mipLevel = voice->bandlimited ? ouaricon::grain::latchLevel(rate, SharedCorpus::kMipLevels) : 0;
    voice->sincSet = voice->bandlimited
                   ? ouaricon::grain::SincBank::setFor(rate / static_cast<double>(1 << voice->mipLevel)) : 0;
}

uint32_t GrainScheduler::queryGrainIndex(
    const SharedCorpus* corpus,
    const SchedulerParams& params,
    uint64_t counter)
{
    // Draw slots: 0..18 KD query jitter, 19..20 projection target, 21 temporal
    // offset, 22 softmax pick (v1.8.0, drawn only while Pick Spread > 0).
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

    // v1.8.0: the ring belongs to one corpus; grain indices mean nothing across.
    if (corpus != recentCorpus || corpus->generation != recentCorpusGeneration)
    {
        recentCorpus = corpus;
        recentCorpusGeneration = corpus->generation;
        recentCount = 0;
        recentHead = 0;
    }

    // v1.8.0: the steered tree when one is published for this corpus, else the
    // corpus's own (shipped weights). The query is built with the scale of the
    // tree it searches; for the corpus tree that is 5 on the macro dims, the
    // v1.7.0 MACRO_DIM_WEIGHT, so the default query is bit-identical.
    const SearchIndex* steered = (params.searchIndex != nullptr && params.searchIndex->corpus.get() == corpus)
                               ? params.searchIndex : nullptr;
    const KDTreeSearch* tree = (steered != nullptr && steered->tree != nullptr) ? steered->tree.get()
                                                                                : corpus->kdTree.get();

    // Use KD-tree if available
    if (tree != nullptr)
    {
        const DescriptorScale& scale = tree->getScale();

        // Build 19D query point — all dims neutral (0) except macro knobs
        std::array<float, 19> queryPoint;
        queryPoint.fill(0.0f);

        // Map macro knobs to their descriptor dimensions (z-score space: -1..+1),
        // scaled like the tree's points
        queryPoint[ENERGY_DIM]     = (params.energy * 2.0f - 1.0f) * scale[ENERGY_DIM];
        queryPoint[BRIGHTNESS_DIM] = (params.brightness * 2.0f - 1.0f) * scale[BRIGHTNESS_DIM];
        queryPoint[TEXTURE_DIM]    = (params.texture * 2.0f - 1.0f) * scale[TEXTURE_DIM];

        // v1.8.0 Pick Spread: refresh the candidate cache at the unjittered target.
        const bool cached = params.pickSpread > 0.0f;
        if (cached)
        {
            const std::array<float, 3> target { queryPoint[ENERGY_DIM], queryPoint[BRIGHTNESS_DIM], queryPoint[TEXTURE_DIM] };
            const uint64_t indexGen = steered != nullptr ? steered->generation : 0;
            if (!cache.valid || cache.tree != tree || cache.corpusGeneration != corpus->generation
                || cache.indexGeneration != indexGen || cache.target != target)
            {
                std::array<float, kCacheK> d2;
                cache.count = static_cast<int>(tree->findNeighbors(queryPoint.data(), kCacheK, cache.index.data(), d2.data()));
                for (int i = 0; i < cache.count; ++i)
                {
                    const auto& desc = corpus->grains[cache.index[static_cast<size_t>(i)]].descriptors;
                    for (int d = 0; d < DESCRIPTOR_DIMS; ++d)
                        cache.point[static_cast<size_t>(i)][static_cast<size_t>(d)] = desc[static_cast<size_t>(d)] * scale[static_cast<size_t>(d)];
                }
                cache.valid = true;
                cache.tree = tree;
                cache.corpusGeneration = corpus->generation;
                cache.indexGeneration = indexGen;
                cache.target = target;
            }
        }

        // Add variation randomization
        float variationScale = params.variation * 2.0f;
        for (uint32_t d = 0; d < queryPoint.size(); ++d)
            queryPoint[d] += (CounterRng::uniform(rngSeed, counter, d) - 0.5f) * variationScale;

        // Candidates: the 8 nearest to the jittered query (v1.7.0), or the cached
        // 16 with their distances to the jittered query recomputed exactly.
        std::array<uint32_t, kCacheK> indices;
        std::array<float, kCacheK> distances;
        int n = 0;
        if (!cached)
        {
            n = static_cast<int>(tree->findNeighbors(queryPoint.data(), kLegacyK, indices.data(), distances.data()));
        }
        else
        {
            n = cache.count;
            for (int i = 0; i < n; ++i)
            {
                indices[static_cast<size_t>(i)] = cache.index[static_cast<size_t>(i)];
                const auto& pt = cache.point[static_cast<size_t>(i)];
                float acc = 0.0f;
                for (int d = 0; d < DESCRIPTOR_DIMS; ++d)
                {
                    const float diff = pt[static_cast<size_t>(d)] - queryPoint[static_cast<size_t>(d)];
                    acc += diff * diff;
                }
                distances[static_cast<size_t>(i)] = acc;
            }
        }
        if (n <= 0)
            return 0;

        // Rescore candidates with temporal position + scatter position biases
        std::array<float, kCacheK> scores;
        for (int i = 0; i < n; ++i)
        {
            float score = distances[static_cast<size_t>(i)];

            // Temporal position bias
            float grainPos = static_cast<float>(indices[static_cast<size_t>(i)]) / static_cast<float>(totalGrains);
            score += std::abs(grainPos - params.position) * 5.0f;

            // 2D scatter position bias (UMAP or PCA)
            if (projX != nullptr)
            {
                float dx = projX[indices[static_cast<size_t>(i)]] - params.scatterX;
                float dy = projY[indices[static_cast<size_t>(i)]] - params.scatterY;
                score += (dx * dx + dy * dy) * 10.0f;
            }
            scores[static_cast<size_t>(i)] = score;
        }

        // Pick Spread temperature from the candidates' own spread, before the
        // penalty: tau = 4 s^2 * mean(score - best). Relative, so the knob means
        // the same on any corpus (descriptors are z-scored, but the position and
        // scatter biases are not).
        float tau = 0.0f;
        if (cached)
        {
            float lo = scores[0];
            for (int i = 1; i < n; ++i) lo = std::min(lo, scores[static_cast<size_t>(i)]);
            float gap = 0.0f;
            for (int i = 0; i < n; ++i) gap += scores[static_cast<size_t>(i)] - lo;
            gap /= static_cast<float>(n);
            tau = 4.0f * params.pickSpread * params.pickSpread * std::max(gap, 1.0e-6f);
        }

        if (params.repeatAvoid > 0.0f)
            applyRepeatPenalty(indices.data(), scores.data(), n, params.repeatAvoid);

        if (!cached)
        {
            float bestScore = std::numeric_limits<float>::max();
            uint32_t bestIndex = indices[0];
            for (int i = 0; i < n; ++i)
            {
                if (scores[static_cast<size_t>(i)] < bestScore)
                {
                    bestScore = scores[static_cast<size_t>(i)];
                    bestIndex = indices[static_cast<size_t>(i)];
                }
            }
            return bestIndex;
        }

        // softmax(-(score - best) / tau), sampled with the grain's own draw.
        float best = scores[0];
        for (int i = 1; i < n; ++i) best = std::min(best, scores[static_cast<size_t>(i)]);
        std::array<float, kCacheK> w;
        float total = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            w[static_cast<size_t>(i)] = std::exp(-(scores[static_cast<size_t>(i)] - best) / tau);
            total += w[static_cast<size_t>(i)];
        }
        float u = CounterRng::uniform(rngSeed, counter, 22) * total;
        for (int i = 0; i < n; ++i)
        {
            u -= w[static_cast<size_t>(i)];
            if (u < 0.0f)
                return indices[static_cast<size_t>(i)];
        }
        // Rounding left u >= 0 past the last weight: the last positive one.
        for (int i = n - 1; i >= 0; --i)
            if (w[static_cast<size_t>(i)] > 0.0f)
                return indices[static_cast<size_t>(i)];
        return indices[0];
    }

    // Fallback: prefer grains near scatter position in 2D projection space
    if (projX != nullptr)
    {
        float targetX = params.scatterX + (CounterRng::uniform(rngSeed, counter, 19) - 0.5f) * params.variation * 0.5f;
        float targetY = params.scatterY + (CounterRng::uniform(rngSeed, counter, 20) - 0.5f) * params.variation * 0.5f;

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
    int randomOffset = CounterRng::uniformInt(rngSeed, counter, 21, spread * 2) - spread;
    int grainIdx = juce::jlimit(0, totalGrains - 1, centerGrain + randomOffset);
    return static_cast<uint32_t>(grainIdx);
}

int GrainScheduler::recentAge(uint32_t grainIndex) const noexcept
{
    for (int age = 0; age < recentCount; ++age)
    {
        const int slot = (recentHead - 1 - age + kRecentLen) % kRecentLen;
        if (recent[static_cast<size_t>(slot)] == grainIndex)
            return age;
    }
    return -1;
}

void GrainScheduler::pushRecent(uint32_t grainIndex) noexcept
{
    recent[static_cast<size_t>(recentHead)] = grainIndex;
    recentHead = (recentHead + 1) % kRecentLen;
    recentCount = std::min(recentCount + 1, kRecentLen);
}

// v1.8.0 Repeat Avoid: a grain picked `age` picks ago gets
// avoid * 2 * range * (kRecentLen - age) / kRecentLen added to its score, where
// range is the candidates' own score spread. At 100 % the last pick scores past
// every unpenalised candidate, so it never repeats while another is available.
void GrainScheduler::applyRepeatPenalty(const uint32_t* indices, float* scores, int n, float avoid) const noexcept
{
    float lo = scores[0], hi = scores[0];
    for (int i = 1; i < n; ++i) { lo = std::min(lo, scores[i]); hi = std::max(hi, scores[i]); }
    const float range = std::max(hi - lo, 1.0e-3f);
    for (int i = 0; i < n; ++i)
    {
        const int age = recentAge(indices[i]);
        if (age >= 0)
            scores[i] += avoid * 2.0f * range * static_cast<float>(kRecentLen - age) / static_cast<float>(kRecentLen);
    }
}

void GrainScheduler::renderVoices(juce::AudioBuffer<float>& buffer, const SharedCorpus* corpus,
                                  int startSample, int endSample)
{
    if (corpus == nullptr || corpus->audioData.getNumSamples() == 0 || startSample >= endSample)
        return;

    auto& voices = grainPool.getVoices();

    for (auto& voice : voices)
    {
        if (!voice.active)
            continue;

        float* leftOut = buffer.getWritePointer(0);
        float* rightOut = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int i = startSample; i < endSample; ++i)
        {
            if (voice.isFinished())
            {
                voice.active = false;
                break;
            }

            // Rate > 1: band-limited mipmap read (v1.7.0); else linear interpolation.
            // The mip check covers a grain spawned on a corpus that had one and
            // carried onto a swapped-in corpus that does not.
            const double readPos = static_cast<double>(voice.grainStartSample) + voice.readPosition;
            const float sample = (voice.bandlimited && corpus->mip != nullptr)
                               ? corpus->readBandlimited(voice.mipLevel, voice.sincSet, readPos)
                               : sampleFromCorpus(corpus, readPos);

            // Apply grain envelope (Hann window)
            float envelope = voice.getEnvelope();
            float outputSample = sample * envelope * voice.gain * voice.stealGain;

            leftOut[i] += outputSample;
            if (rightOut != nullptr)
                rightOut[i] += outputSample;

            // Advance read position
            voice.readPosition += voice.playbackRate;
            ++voice.samplesElapsed;

            if (voice.stealStep > 0.0f)
            {
                voice.stealGain -= voice.stealStep;
                if (voice.stealGain <= 0.0f)
                {
                    voice.active = false;
                    break;
                }
            }
        }
    }
}

float GrainScheduler::sampleFromCorpus(const SharedCorpus* corpus, double readPosition) const
{
    const int numSamples = corpus->audioData.getNumSamples();
    if (numSamples == 0)
        return 0.0f;

    // Clamp to valid range
    if (readPosition < 0.0 || readPosition >= static_cast<double>(numSamples - 1))
        return 0.0f;

    // Linear interpolation
    const int index0 = static_cast<int>(readPosition);
    const int index1 = index0 + 1;
    const float frac = static_cast<float>(readPosition - static_cast<double>(index0));

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
