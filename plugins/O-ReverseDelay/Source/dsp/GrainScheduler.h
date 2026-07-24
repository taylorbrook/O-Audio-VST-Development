/*
  ==============================================================================

    O-ReverseDelay - GrainScheduler

    Per-sample countdown emitting spawn offsets, from O-GrainScatter's
    free-mode scheduler shape with the Stage-2 contract changes
    (PLAN Task 3 / RESEARCH §3.3):

      * Interval from ARCHITECTURE, not the exponential map:
            overlap = 1 + (density/100)·7
            intervalSamples = max (1, (int) (G / overlap))
        (both computed by the caller — the scheduler only counts).
      * No probability gate, no Euclidean.
      * Hard spawn cap 32/block (kMaxSpawnsPerBlock, WR-04 pattern) into a
        FIXED array — zero allocation on the audio thread.

    The scheduler ALWAYS runs this free countdown; tempo sync (Phase 2.3)
    only changes the VALUE of D per block, never the spawn timing — no
    conditional routing (Locked Decision).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

struct SpawnRequest
{
    int sampleOffset = 0;
};

class GrainScheduler
{
public:
    // Matches the GrainPool size — excess spawns would only steal grains anyway.
    static constexpr int kMaxSpawnsPerBlock = 32;

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        samplesUntilNextGrain = 0;
    }

    void clear() noexcept
    {
        samplesUntilNextGrain = 0;
    }

    // Emits spawn offsets for this block into the fixed request array.
    // Returns the number of requests written (<= kMaxSpawnsPerBlock).
    int processBlock (int numSamples, int intervalSamples,
                      std::array<SpawnRequest, kMaxSpawnsPerBlock>& outRequests) noexcept
    {
        const int interval = juce::jmax (1, intervalSamples);
        int count = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            --samplesUntilNextGrain;
            if (samplesUntilNextGrain <= 0)
            {
                samplesUntilNextGrain = interval;
                if (count < kMaxSpawnsPerBlock)
                    outRequests[static_cast<size_t> (count++)] = { i };
            }
        }

        return count;
    }

private:
    double sampleRate = 44100.0;
    int samplesUntilNextGrain = 0;
};
