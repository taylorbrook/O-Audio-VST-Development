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
                if (rng.nextFloat() < probability / 100.0f)
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

        // Swing: offset even-numbered subdivisions by swingPct (50% = no swing, 75% = max shuffle)
        // swingRatio converts 50-75% to 0.0-0.5 fractional offset of one subdivision
        double swingRatio = (static_cast<double> (swingPct) - 50.0) / 50.0;  // 0.0 to 0.5

        stutterGateStart = -1;
        stutterGateEnd = -1;

        for (int i = 0; i < numSamples; ++i)
        {
            double ppqAtSample     = syncInfo.ppqPosition + i * syncInfo.ppqPerSample;
            double ppqAtSamplePrev = syncInfo.ppqPosition + (i - 1) * syncInfo.ppqPerSample;

            if (i == 0 && syncInfo.ppqPerSample <= 0.0) continue;

            // Apply swing: shift even-numbered subdivision boundaries forward
            double currentDiv = std::floor (ppqAtSample / subdiv);
            double prevDiv    = std::floor (ppqAtSamplePrev / subdiv);

            if (currentDiv > prevDiv)
            {
                // Check if this is an even-numbered subdivision (0-indexed: 0,2,4... are on-beat)
                int divIndex = static_cast<int> (currentDiv);
                bool isEvenSubdiv = (divIndex % 2) != 0;  // odd divisions are the "off-beats" we swing

                if (isEvenSubdiv && swingRatio > 0.001)
                {
                    // Delay this trigger by swingRatio * subdiv in PPQ
                    double swingOffsetPpq = swingRatio * subdiv;
                    double swungBoundary = currentDiv * subdiv + swingOffsetPpq;

                    // Check if the swung boundary falls within [ppqAtSamplePrev, ppqAtSample)
                    if (swungBoundary > ppqAtSample || swungBoundary <= ppqAtSamplePrev)
                        continue;  // Not yet reached the swung position
                }

                // Euclidean gate check (with rotation offset)
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
                    outRequests.push_back ({ i });

                    // Spawn repeat grains at subdivision intervals
                    int repeatIntervalSamples = static_cast<int> (subdiv * 60.0 * sampleRate / syncInfo.bpm);
                    for (int r = 1; r < repeats; ++r)
                    {
                        int offset = i + r * repeatIntervalSamples;
                        if (offset < numSamples)
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
