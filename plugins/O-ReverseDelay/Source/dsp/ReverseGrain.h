/*
  ==============================================================================

    O-ReverseDelay - ReverseGrain POD + GrainPool

    Grain slot per Stage-2 RESEARCH §3.2 — heavily simplified from
    O-simpleGrain's Grain: no double-precision readPos, no rate, no per-grain
    AA filter (those exist for pitch-shifted reads; reverse speed here is
    exactly 1.0 with integer stepping).

    Per-sample render (in PluginProcessor):
        s = capture.monoSum (readAbs);
        e = hannLut.read (n * invG);
        wetL += s * e * gain * gL;  wetR += s * e * gain * gR;
        --readAbs;  ++n;

    Pool: fixed 32 preallocated slots (max sustained overlap is 8; headroom
    covers delay-time transitions). find-inactive round-robin + steal-oldest —
    the proven O-GrainScatter GrainPool shape, minus everything spatial/freeze.
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
    int         G           = 0;            // latched length (samples)
    float       invG        = 0.0f;         // 1/G for window phase
    float       gain        = 0.0f;         // 1/sqrt(overlap), latched at spawn
    float       gL          = 0.70710677f;  // equal-power pan, latched (center in Phase 2.1)
    float       gR          = 0.70710677f;
    int         age         = 0;            // samples alive, for steal-oldest
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

    // find-inactive round-robin; steal-oldest (largest age) on exhaustion.
    // Alloc-free, bounded (2 fixed scans of 32).
    ReverseGrain& obtain() noexcept
    {
        for (int i = 0; i < kMaxGrains; ++i)
        {
            const int idx = (nextSlot + i) % kMaxGrains;
            if (! grains[static_cast<size_t> (idx)].active)
            {
                nextSlot = (idx + 1) % kMaxGrains;
                return grains[static_cast<size_t> (idx)];
            }
        }

        int oldest = 0;
        for (int i = 1; i < kMaxGrains; ++i)
            if (grains[static_cast<size_t> (i)].age > grains[static_cast<size_t> (oldest)].age)
                oldest = i;

        nextSlot = (oldest + 1) % kMaxGrains;
        return grains[static_cast<size_t> (oldest)];
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
