/*
  ==============================================================================

    O-ReverseDelay - GrainScheduler

    Per-sample countdown emitting spawn offsets, from O-GrainScatter's
    free-mode scheduler shape with the Stage-2 contract changes
    (PLAN Task 3 / RESEARCH §3.3):

      * Interval from ARCHITECTURE, not the exponential map:
            overlap = 2 + (density/100)·6      (v1.0.1; was 1 + d·7 at v1.0.0)
            intervalSamples = max (1, (int) (G / overlap))
        (both computed by the caller — the scheduler only counts).
      * No probability gate, no Euclidean.
      * Hard spawn cap 32/block (kMaxSpawnsPerBlock, WR-04 pattern) into a
        FIXED array — zero allocation on the audio thread.

    The scheduler ALWAYS runs this free countdown; tempo sync (Phase 2.3)
    only changes the VALUE of D per block, never the spawn timing — no
    conditional routing (Locked Decision).

    ── v1.1.0: spawn-time jitter (B3 #1) ────────────────────────────────────
    The countdown was strictly periodic through v1.0.1. A fixed spawn interval
    against a fixed grain length is a comb: every grain starts at the same
    phase relative to its neighbours, and on sustained material that reads as
    a metallic ring rather than a cloud. Jitter re-rolls the interval at each
    spawn — interval·(1 ± dev), dev symmetric so the MEAN interval, and with
    it the average density and the loop's duty cycle, are unchanged.

    The RNG is injected rather than owned: PluginProcessor holds one xorshift32
    stream shared by the pan spread and all four v1.1 randomisations, and a
    single stream is what makes the whole engine reproducible from one seed.

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

    // The scheduler counts in SAMPLES and the caller supplies intervalSamples
    // already converted, so no sample rate is needed. v1.0.0 stored one in a
    // `sampleRate` member that nothing ever read — removed in v1.0.1. The
    // parameter stays for signature symmetry with the other DSP components.
    void prepare (double /*sampleRate*/) noexcept
    {
        samplesUntilNextGrain = 0;
    }

    void clear() noexcept
    {
        samplesUntilNextGrain = 0;
    }

    // Largest fraction of the nominal interval that jitter may remove or add.
    // Not 1.0: at ±100 % the low tail approaches a zero-length interval, i.e. a
    // spawn every sample. 0.9 caps the worst-case instantaneous spawn rate at
    // 10x nominal while keeping the multiplier symmetric about 1, so the mean
    // interval — and therefore the average overlap and the loop duty cycle —
    // is exactly unchanged at every jitter setting.
    static constexpr float kMaxJitterDeviation = 0.9f;

    // Emits spawn offsets for this block into the fixed request array.
    // Returns the number of requests written (<= kMaxSpawnsPerBlock).
    //
    // `nextRand01` must return [0, 1). It is a template parameter so the
    // processor's xorshift inlines here — no std::function, no allocation, no
    // indirect call on the audio thread.
    template <typename RandomFn>
    int processBlock (int numSamples, int intervalSamples, float jitterAmount,
                      std::array<SpawnRequest, kMaxSpawnsPerBlock>& outRequests,
                      RandomFn&& nextRand01) noexcept
    {
        const int interval = juce::jmax (1, intervalSamples);
        int count = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            --samplesUntilNextGrain;
            if (samplesUntilNextGrain <= 0)
            {
                samplesUntilNextGrain = nextInterval (interval, jitterAmount, nextRand01);
                if (count < kMaxSpawnsPerBlock)
                    outRequests[static_cast<size_t> (count++)] = { i };
            }
        }

        return count;
    }

private:
    // LOAD-BEARING: at jitterAmount == 0 this draws NOTHING.
    //
    // The processor's xorshift is a single shared stream — pan spread, delay
    // scatter, size random and gain random all pull from it. An unconditional
    // draw here would advance that stream one step per spawn even with jitter
    // off, shifting every subsequent pan value and changing the shipped v1.0
    // sound on existing sessions. Every v1.1 randomisation follows the same
    // rule at its own call site, which is what makes "all four at 0" render
    // bit-identically to v1.0.1 (render-harness probe T asserts exactly this).
    template <typename RandomFn>
    static int nextInterval (int interval, float jitterAmount, RandomFn&& nextRand01) noexcept
    {
        if (jitterAmount <= 0.0f)
            return interval;

        const float dev = juce::jmin (1.0f, jitterAmount) * kMaxJitterDeviation;
        const float mul = 1.0f + dev * (2.0f * nextRand01() - 1.0f);   // mean exactly 1.0
        return juce::jmax (1, static_cast<int> (static_cast<float> (interval) * mul));
    }

    int samplesUntilNextGrain = 0;
};
