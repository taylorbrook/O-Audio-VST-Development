/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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
    // ── v1.7.2 (WR-02): 128 -> 2048, and the DERIVATION is corrected ─────────
    //
    // The v1.3.0 derivation was unsound. It read:
    //
    //   spawns per pass  = passLen / interval
    //   passLen         <= kDelayTimeMinMs·fs             (the A2 pass bound)
    //   interval         = G / overlap >= (kGrainSizeMinMs·fs) / overlapMax
    //   -> spawns <= overlapMax · kDelayTimeMinMs / kGrainSizeMinMs = 16
    //
    // and concluded "16 nominal against a cap of 128 — 8x margin", plus a
    // sample-rate-independence claim. The second line is only true when
    // delayScatter > 0. With scatter at 0 — the shipped DEFAULT and every factory
    // preset — PluginProcessor's passBound is D, so passLen = min(numSamples, D)
    // and the governing quantity is the HOST BLOCK SIZE, which appears nowhere in
    // the old bound. Probe AB reproduced the same derivation in its comment and
    // then pinned delayTime to kDelayTimeMinMs, i.e. the single configuration in
    // which the false premise happens to hold (D == minDelaySamples).
    //
    // The real worst case is delayTime at its MAXIMUM (so passLen = numSamples)
    // with grainSize at its minimum and the overlap ceiling at 16:
    //
    //   passLen  <= min(hostBlock, kDelayTimeMaxMs·fs)  ->  hostBlock
    //   interval  = max(1, (int)(G / overlap))
    //            >= (kGrainSizeMinMs·fs/1000) / kOverlapCeilingMax
    //   -> nominal spawns <= hostBlock · kOverlapCeilingMax
    //                          / (kGrainSizeMinMs·fs/1000)
    //
    // At 44.1 kHz that interval is 2205/16 = 137 samples, so the NOMINAL count is
    // 30 at a 4096-sample block and 119 at 16384 (an offline bounce in several
    // hosts) — against the old cap of 128. The claimed 8x margin was really 1.07x,
    // and `droppedSpawns` is asserted == 0, so a live assertion was resting on it.
    //
    // This cap is now the HARD bound rather than a probabilistic one, because
    // jitter's floor is itself hard: nextInterval() returns
    // jmax(1, (int)(interval · mul)) with mul >= 1 - kMaxJitterDeviation = 0.1, so
    // no draw sequence however unlucky can produce an interval below
    //
    //   minInterval = max(1, (int)(0.1 · 137)) = 13 samples   (44.1 kHz)
    //   -> spawns   <= 16384 / 13 = 1261
    //
    // 2048 covers that with ~1.6x margin, and covers the realistic 119 by 17x.
    // Note the sensitivity has INVERTED relative to the old comment: because
    // interval scales with fs while the host's block size does not, the pressure
    // comes from LOW sample rates and LARGE blocks, not from high ones. The two
    // documented assumptions are therefore fs >= 44100 and hostBlock <= 16384;
    // prepareToPlay jasserts the second, and getDroppedSpawnCount() remains the
    // standing tripwire for both (it is counted, not trusted — which is the one
    // thing v1.3.0 got right).
    //
    // Cost of the raise: 2048 ints in one preallocated member array, 8 KB, sized
    // at compile time and never touched on the audio thread.
    static constexpr int kMaxSpawnsPerBlock = 2048;

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
