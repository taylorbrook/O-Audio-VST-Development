/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include "TempoTracker.h"
#include "CounterRng.h"

struct SpawnRequest
{
    int sampleOffset = 0;
    int gateSamples = 0;   // v3.1.0: > 0 = a primary sync grain that opens the stutter gate this long
};

class GrainScheduler
{
public:
    // WR-04: hard cap on spawn requests per block. Matches the processor's
    // spawnRequests.reserve(128), so push_back never reallocates on the audio thread.
    // Excess requests would only steal voices anyway (pool is 64).
    static constexpr size_t kMaxSpawnsPerBlock = 128;

    // v3.1.0: sync repeats that land past the block end wait here (absolute sample
    // times) and fire in the block they fall in. Bounded: at most repeats - 1 per
    // trigger, and a trigger's repeats are all due within repeats subdivisions, so
    // 16 repeats keep at most 15 * 16 / 2 = 120 in flight.
    static constexpr int kMaxPendingRepeats = 256;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        samplesUntilNextGrain = 0;
        euclideanStep = 0;
        blockStart = 0;
        freeTick = 0;
        numPending = 0;
    }

    // v3.1.0: the counter-based RNG's seed (saved with the session).
    void setSeed (uint64_t newSeed) { seed = newSeed; }

    // Free mode: density (1-100%) → inter-grain interval (exponential curve)
    void processBlockFree (int numSamples, float density, float probability,
                           std::vector<SpawnRequest>& outRequests)
    {
        // Exponential mapping: density 1% ≈ 955ms (~1/sec), 50% = 100ms (~10/sec), 100% = 10ms (~100/sec)
        float intervalMs = 1000.0f * std::pow (0.01f, density / 100.0f);
        int intervalSamples = juce::jmax (1, static_cast<int> (sampleRate * intervalMs / 1000.0));

        numPending = 0;   // sync repeats don't survive a switch to Free

        for (int i = 0; i < numSamples; ++i)
        {
            --samplesUntilNextGrain;
            if (samplesUntilNextGrain <= 0)
            {
                samplesUntilNextGrain = intervalSamples;
                const CounterRng::Draw draw (seed, CounterRng::SchedulerFree, freeTick++);
                if (draw.nextFloat (0) < probability / 100.0f
                    && outRequests.size() < kMaxSpawnsPerBlock)   // WR-04
                    outRequests.push_back ({ i });
            }
        }

        blockStart += numSamples;
    }

    // Sync mode: trigger on subdivision crossings with Euclidean gating
    void processBlockSync (int numSamples, const SyncInfo& syncInfo,
                           int subdivIndex, float probability, int repeats,
                           const std::array<bool, 16>& euclideanPattern,
                           int euclideanLength, int euclideanRotation,
                           float swingPct, bool stutterGateOn,
                           std::vector<SpawnRequest>& outRequests)
    {
        static constexpr double subdivPpq[] = { 0.0, 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
        double subdiv = subdivPpq[juce::jlimit (0, 6, subdivIndex)];

        // No valid tempo advance → no subdivision can be crossed this block (WR-09 companion).
        if (subdiv > 0.0 && syncInfo.ppqPerSample > 0.0)
        {
            // Swing: odd-numbered divisions (the off-beats) fire late by swingOffsetPpq.
            // swingRatio maps 50-75% → 0.0-0.5 of one subdivision.
            double swingRatio    = (static_cast<double> (swingPct) - 50.0) / 50.0;
            double swingOffsetPpq = swingRatio * subdiv;

            for (int i = 0; i < numSamples; ++i)
            {
                double ppqAtSample     = syncInfo.ppqAt (i);
                double ppqAtSamplePrev = syncInfo.ppqAt (i - 1);

                // The straight subdivision window this sample lies in.
                juce::int64 divIndex = static_cast<juce::int64> (std::floor (ppqAtSample / subdiv));
                if (divIndex < 0) continue;

                // WR-02: each division has its OWN trigger time (straight for even/on-beat,
                // swingOffset-delayed for odd/off-beat). Detecting that swung time directly —
                // rather than reusing the straight-boundary crossing and then rejecting — means
                // off-beat grains actually fire (they were being dropped) and the Euclidean step
                // advances exactly once per division, in order, so the pattern stays phase-locked.
                bool   isOffBeat  = (divIndex % 2) != 0;
                double triggerPpq = static_cast<double> (divIndex) * subdiv + (isOffBeat ? swingOffsetPpq : 0.0);

                if (triggerPpq > ppqAtSamplePrev && triggerPpq <= ppqAtSample)
                {
                    // Euclidean gate (advance the step once per division regardless of the gate result)
                    bool euclideanPass = true;
                    if (euclideanLength > 0)
                    {
                        int rotatedStep = (euclideanStep + euclideanRotation) % euclideanLength;
                        euclideanPass = euclideanPattern[static_cast<size_t> (rotatedStep)];
                        euclideanStep = (euclideanStep + 1) % euclideanLength;
                    }

                    // v3.1.0: the draw is keyed by the division, not by how many draws came before.
                    const CounterRng::Draw draw (seed, CounterRng::SchedulerSync,
                                                 static_cast<uint64_t> (divIndex));

                    if (euclideanPass && draw.nextFloat (0) < probability / 100.0f)
                    {
                        // Repeat grains at subdivision intervals.
                        // IN-06: jmax(1.0, bpm) hardens the divide against a pathological tiny bpm.
                        int repeatIntervalSamples = static_cast<int> (subdiv * 60.0 * sampleRate
                                                                      / juce::jmax (1.0, syncInfo.bpm));

                        // Primary grain; it carries the stutter gate's length
                        if (outRequests.size() < kMaxSpawnsPerBlock)   // WR-04
                            outRequests.push_back ({ i, stutterGateOn ? repeats * repeatIntervalSamples : 0 });

                        // v3.1.0: queue every repeat at its absolute time. The ones inside
                        // this block are emitted below with the carried-over ones; the rest
                        // fire in a later block (they used to be dropped at the block end).
                        for (int r = 1; r < repeats && numPending < kMaxPendingRepeats; ++r)
                            pending[static_cast<size_t> (numPending++)]
                                = blockStart + i + static_cast<juce::int64> (r) * repeatIntervalSamples;
                    }
                }
            }
        }

        // Emit every queued repeat due in this block (repeats are never late: one due in
        // this block was queued at or before its own sample).
        const juce::int64 blockEnd = blockStart + numSamples;
        int kept = 0;
        for (int p = 0; p < numPending; ++p)
        {
            const juce::int64 t = pending[static_cast<size_t> (p)];
            if (t < blockEnd)
            {
                if (outRequests.size() < kMaxSpawnsPerBlock)   // WR-04
                    outRequests.push_back ({ static_cast<int> (t - blockStart) });
            }
            else
                pending[static_cast<size_t> (kept++)] = t;
        }
        numPending = kept;

        blockStart = blockEnd;
    }

    void resetEuclideanStep (int newSteps)
    {
        if (newSteps > 0)
            euclideanStep = euclideanStep % newSteps;
        else
            euclideanStep = 0;
    }

    int getEuclideanStep() const { return euclideanStep; }

private:
    double sampleRate = 44100.0;
    int samplesUntilNextGrain = 0;
    int euclideanStep = 0;
    uint64_t seed = 0;
    juce::int64 blockStart = 0;   // absolute sample index of the current block's first sample
    uint64_t freeTick = 0;        // Free-mode tick counter (RNG event index)
    std::array<juce::int64, kMaxPendingRepeats> pending {};
    int numPending = 0;
};
