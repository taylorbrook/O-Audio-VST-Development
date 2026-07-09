#pragma once

#include <JuceHeader.h>
#include "TempoTracker.h"

struct SpawnRequest
{
    int sampleOffset = 0;
};

class GrainScheduler
{
public:
    // WR-04: hard cap on spawn requests per block. Matches the processor's
    // spawnRequests.reserve(128), so push_back never reallocates on the audio thread.
    // Excess requests would only steal voices anyway (pool is 64).
    static constexpr size_t kMaxSpawnsPerBlock = 128;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        samplesUntilNextGrain = 0;
        euclideanStep = 0;
    }

    // Free mode: density (1-100%) → inter-grain interval (exponential curve)
    void processBlockFree (int numSamples, float density, float probability,
                           std::vector<SpawnRequest>& outRequests)
    {
        // Exponential mapping: density 1% ≈ 955ms (~1/sec), 50% = 100ms (~10/sec), 100% = 10ms (~100/sec)
        float intervalMs = 1000.0f * std::pow (0.01f, density / 100.0f);
        int intervalSamples = juce::jmax (1, static_cast<int> (sampleRate * intervalMs / 1000.0));

        for (int i = 0; i < numSamples; ++i)
        {
            --samplesUntilNextGrain;
            if (samplesUntilNextGrain <= 0)
            {
                samplesUntilNextGrain = intervalSamples;
                if (rng.nextFloat() < probability / 100.0f
                    && outRequests.size() < kMaxSpawnsPerBlock)   // WR-04
                    outRequests.push_back ({ i });
            }
        }
    }

    // Sync mode: trigger on subdivision crossings with Euclidean gating
    void processBlockSync (int numSamples, const SyncInfo& syncInfo,
                           int subdivIndex, float probability, int repeats,
                           const std::array<bool, 16>& euclideanPattern,
                           int euclideanLength, int euclideanRotation,
                           float swingPct, bool stutterGateOn,
                           std::vector<SpawnRequest>& outRequests,
                           int& stutterGateStart, int& stutterGateEnd)
    {
        static constexpr double subdivPpq[] = { 0.0, 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
        double subdiv = subdivPpq[juce::jlimit (0, 6, subdivIndex)];
        if (subdiv <= 0.0) return;

        stutterGateStart = -1;
        stutterGateEnd = -1;

        // No valid tempo advance → no subdivision can be crossed this block (WR-09 companion).
        if (syncInfo.ppqPerSample <= 0.0) return;

        // Swing: odd-numbered divisions (the off-beats) fire late by swingOffsetPpq.
        // swingRatio maps 50-75% → 0.0-0.5 of one subdivision.
        double swingRatio    = (static_cast<double> (swingPct) - 50.0) / 50.0;
        double swingOffsetPpq = swingRatio * subdiv;

        for (int i = 0; i < numSamples; ++i)
        {
            double ppqAtSample     = syncInfo.ppqPosition + i * syncInfo.ppqPerSample;
            double ppqAtSamplePrev = syncInfo.ppqPosition + (i - 1) * syncInfo.ppqPerSample;

            // The straight subdivision window this sample lies in.
            int divIndex = static_cast<int> (std::floor (ppqAtSample / subdiv));
            if (divIndex < 0) continue;

            // WR-02: each division has its OWN trigger time (straight for even/on-beat,
            // swingOffset-delayed for odd/off-beat). Detecting that swung time directly —
            // rather than reusing the straight-boundary crossing and then rejecting — means
            // off-beat grains actually fire (they were being dropped) and the Euclidean step
            // advances exactly once per division, in order, so the pattern stays phase-locked.
            bool   isOffBeat  = (divIndex % 2) != 0;
            double triggerPpq = divIndex * subdiv + (isOffBeat ? swingOffsetPpq : 0.0);

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

                if (euclideanPass && rng.nextFloat() < probability / 100.0f)
                {
                    // Spawn primary grain
                    if (outRequests.size() < kMaxSpawnsPerBlock)   // WR-04
                        outRequests.push_back ({ i });

                    // Spawn repeat grains at subdivision intervals.
                    // IN-06: jmax(1.0, bpm) hardens the divide against a pathological tiny bpm.
                    int repeatIntervalSamples = static_cast<int> (subdiv * 60.0 * sampleRate
                                                                  / juce::jmax (1.0, syncInfo.bpm));
                    for (int r = 1; r < repeats; ++r)
                    {
                        int offset = i + r * repeatIntervalSamples;
                        if (offset < numSamples && outRequests.size() < kMaxSpawnsPerBlock)   // WR-04
                            outRequests.push_back ({ offset });
                    }

                    // Stutter gate range
                    if (stutterGateOn)
                    {
                        stutterGateStart = i;
                        int totalDuration = repeats * repeatIntervalSamples;
                        stutterGateEnd = juce::jmin (i + totalDuration, numSamples);
                    }
                }
            }
        }
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
    juce::Random rng;
};
