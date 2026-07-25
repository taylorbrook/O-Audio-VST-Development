/*
  ==============================================================================

    O-ReverseDelay - ReverseGrain POD + GrainPool

    Grain slot per Stage-2 RESEARCH §3.2 — heavily simplified from
    O-simpleGrain's Grain: no double-precision readPos, no rate, no per-grain
    AA filter (those exist for pitch-shifted reads; reverse speed here is
    exactly 1.0 with integer stepping).

    Per-sample render (in PluginProcessor), v1.1.0 — TWO accumulations:
        s = capture.monoSum (readAbs);
        e = hannLut.read (n * invG);
        v = s * e * gain;
        wetL  += v * gLout;  wetR  += v * gRout;   // output: per-grain random gain
        loopL += v * gL;     loopR += v * gR;      // feedback tap: never randomised
        --readAbs;  ++n;

    The split exists because gainRandom must be applied AFTER the feedback tap:
    randomising the gain the loop sees would modulate the per-generation decay
    rate, i.e. the knob would change how long the tail lasts rather than how it
    shimmers. At gainRandom = 0 the latched gLout/gRout are bitwise equal to
    gL/gR, so the two buffers hold identical values and the v1.0 sound is
    reproduced exactly.

    Pool: fixed 32 preallocated slots (max sustained overlap is 8; headroom
    covers delay-time transitions). find-inactive round-robin, and — since
    v1.1.0 — REFUSE on exhaustion rather than steal-oldest. The proven
    O-GrainScatter GrainPool shape, minus everything spatial/freeze.
    Zero allocation in processBlock.

    `startOffset` is a per-block transient: the sample offset within the
    CURRENT block before which a freshly-spawned grain is silent. The render
    loop resets it to 0 after each block so continuing grains render from the
    block start.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

struct ReverseGrain
{
    bool        active      = false;
    juce::int64 readAbs     = 0;            // next capture index to read (steps −1 per sample)
    int         n           = 0;            // samples emitted
    int         G           = 0;            // latched length (samples) — v1.1: ± sizeRandom
    float       invG        = 0.0f;         // 1/G for window phase
    float       gain        = 0.0f;         // 1/sqrt(overlap), latched at spawn
    float       gL          = 0.70710677f;  // equal-power pan, latched (center in Phase 2.1)
    float       gR          = 0.70710677f;
    // v1.1.0: pan × per-grain random gain. Latched at spawn like everything
    // else, so a mid-flight gainRandom change never re-gains a live grain.
    // Written as gL * gainRand with gainRand == 1.0f when the control is off,
    // which is bitwise identity — not "close to" gL.
    float       gLout       = 0.70710677f;
    float       gRout       = 0.70710677f;
    int         age         = 0;            // samples alive
    int         startOffset = 0;            // block-transient: first render sample this block
};

class GrainPool
{
public:
    static constexpr int kMaxGrains = 32;

    void clear() noexcept
    {
        for (auto& g : grains)
            g.active = false;
        nextSlot = 0;
    }

    // find-inactive round-robin; REFUSE (nullptr) on exhaustion.
    // Alloc-free, bounded (one fixed scan of 32).
    //
    // ── v1.1.0: steal-oldest removed ───────────────────────────────────────
    // v1.0 overwrote the oldest slot's state in place. That grain's Hann
    // envelope jumped from wherever it was — often near the window's peak — to
    // zero in a single sample, which is a click, not a crossfade. It was
    // unreachable in v1.0 steady state (max overlap 8 against 32 slots), but it
    // was reachable on a fast grainSize/delayTime sweep with 500 ms grains in
    // flight, and ALL FOUR v1.1 randomisations raise the transient concurrent-
    // grain peak: jitter can shorten a spawn interval to 0.1x nominal, and
    // sizeRandom can hold grains alive longer than the block's nominal G.
    //
    // Refusing costs one grain out of a wash of 8-32 and is inaudible — the
    // overlap-add simply has one fewer contributor for that window. Cutting a
    // live envelope is audible every time. The caller drops the spawn; the
    // scheduler's countdown is unaffected, so the grid does not shift.
    ReverseGrain* obtain() noexcept
    {
        for (int i = 0; i < kMaxGrains; ++i)
        {
            const int idx = (nextSlot + i) % kMaxGrains;
            if (! grains[static_cast<size_t> (idx)].active)
            {
                nextSlot = (idx + 1) % kMaxGrains;
                return &grains[static_cast<size_t> (idx)];
            }
        }

        return nullptr;
    }

    int countActive() const noexcept
    {
        int c = 0;
        for (const auto& g : grains)
            if (g.active) ++c;
        return c;
    }

    std::array<ReverseGrain, kMaxGrains> grains {};

private:
    int nextSlot = 0;
};
