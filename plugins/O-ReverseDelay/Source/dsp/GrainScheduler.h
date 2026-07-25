/*
  ==============================================================================

    O-ReverseDelay - GrainScheduler

    Per-sample countdown emitting spawn offsets, from O-GrainScatter's
    free-mode scheduler shape with the Stage-2 contract changes
    (PLAN Task 3 / RESEARCH §3.3):

      * Interval from ARCHITECTURE, not the exponential map:
            overlap = 2 + (density/100)·(ceiling − 2)   (v1.3.0; ceiling was a
                                                         fixed 8 at v1.0.1)
            intervalSamples = max (1, (int) (G / overlap))
        (both computed by the caller — the scheduler only counts).
      * No probability gate, no Euclidean.
      * Hard spawn cap per pass (kMaxSpawnsPerBlock, WR-04 pattern) into a
        FIXED array — zero allocation on the audio thread. Overflow is COUNTED
        and reported since v1.3.0; see kMaxSpawnsPerBlock.

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

/** What one pass of the countdown produced. `dropped` is the number of spawns
    the countdown reached but could not write because the request array was
    full — zero at every setting this plugin can reach (see
    kMaxSpawnsPerBlock), and asserted zero by render-harness probe AB at the
    worst case rather than assumed. */
struct SpawnResult
{
    int count   = 0;
    int dropped = 0;
};

class GrainScheduler
{
public:
    // ── v1.3.0 (B2): 32 -> 128, and overflow is no longer silent ─────────────
    //
    // v1.0's 32 was chosen to match GrainPool::kMaxGrains on the reasoning that
    // "excess spawns would only steal grains anyway". That reasoning died at
    // v1.1.0, when the pool started REFUSING on exhaustion instead of stealing:
    // dropping a request here and refusing it in the pool are now two different
    // events, and only the pool's is a deliberate design choice. A request
    // dropped here is the scheduler failing to report work it actually decided
    // to do, which is why the count is returned.
    //
    // The bound, worked at the shipped ranges — and it is SAMPLE-RATE
    // INDEPENDENT, which is what makes it a real bound rather than a guess:
    //
    //   spawns per pass  = passLen / interval
    //   passLen         <= kDelayTimeMinMs·fs             (the A2 pass bound)
    //   interval         = G / overlap >= (kGrainSizeMinMs·fs) / overlapMax
    //
    //   -> spawns <= overlapMax · kDelayTimeMinMs / kGrainSizeMinMs
    //              = 16 · 50 / 50 = 16                    (both minima are 50 ms)
    //
    // So 16 nominal at the v1.3.0 ceiling, against a cap of 128 — 8x margin.
    // Jitter can shorten one interval to 0.1x nominal, so a BURST can exceed 16;
    // exceeding 128 would need the mean jitter multiplier across 128 consecutive
    // draws to be <= 0.125 when its distribution is uniform on [0.1, 1.9] with
    // mean exactly 1.0. That is not a probability worth naming.
    //
    // The tripwire for a future release: the bound is
    // overlapMax · kDelayTimeMinMs / kGrainSizeMinMs, so it is grainSize's
    // MINIMUM — not the overlap ceiling — that this cap is most sensitive to.
    // Dropping grainSize's minimum below ~6 ms would reach 128 at ceiling 16.
    // Probe AB is what will notice.
    //
    // Cost of the raise: 128 ints in one preallocated member array, 512 bytes.
    static constexpr int kMaxSpawnsPerBlock = 128;

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
    // Returns the number of requests written (<= kMaxSpawnsPerBlock) alongside
    // the number the cap discarded.
    //
    // The countdown is re-armed BEFORE the cap is tested, and deliberately: the
    // spawn grid must not depend on whether the array had room, or a single
    // overflow would shift every later spawn in the pass and the engine would
    // stop being block-size invariant (probe W2). A dropped request is a missing
    // grain, never a moved one — the same contract GrainPool::obtain() honours
    // when it refuses.
    //
    // `nextRand01` must return [0, 1). It is a template parameter so the
    // processor's xorshift inlines here — no std::function, no allocation, no
    // indirect call on the audio thread.
    template <typename RandomFn>
    SpawnResult processBlock (int numSamples, int intervalSamples, float jitterAmount,
                              std::array<SpawnRequest, kMaxSpawnsPerBlock>& outRequests,
                              RandomFn&& nextRand01) noexcept
    {
        const int interval = juce::jmax (1, intervalSamples);
        SpawnResult result;

        for (int i = 0; i < numSamples; ++i)
        {
            --samplesUntilNextGrain;
            if (samplesUntilNextGrain <= 0)
            {
                samplesUntilNextGrain = nextInterval (interval, jitterAmount, nextRand01);

                if (result.count < kMaxSpawnsPerBlock)
                    outRequests[static_cast<size_t> (result.count++)] = { i };
                else
                    ++result.dropped;
            }
        }

        return result;
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
