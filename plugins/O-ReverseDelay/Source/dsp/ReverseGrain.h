/*
  ==============================================================================

    O-ReverseDelay - ReverseGrain POD + GrainPool

    Grain slot per Stage-2 RESEARCH §3.2 — heavily simplified from
    O-simpleGrain's Grain: no double-precision readPos, no rate, no per-grain
    AA filter (those exist for pitch-shifted reads; reverse speed here is
    exactly 1.0 with integer stepping).

    Per-sample render (in PluginProcessor), v1.2.0 — TWO accumulations:
        p = n * invG;
        q = min (p, tiltT)·tiltA + max (p - tiltT, 0)·tiltB;   // B1 window tilt
        s = capture.monoSum (readAbs);
        e = windowLuts.readAt (win, q);                        // B1 window shape
        v = s * e * gain;
        wetL  += v * gLout;  wetR  += v * gRout;   // output: per-grain random gain
        loopL += v * gL;     loopR += v * gR;      // feedback tap: never randomised
        readAbs += step;  ++n;                     // v1.6.0: step is ±1 (B4 #2)

    The wet/loop split was introduced in v1.1.0 because gainRandom must be
    applied AFTER the feedback tap: randomising the gain the loop sees would
    modulate the per-generation decay rate, i.e. the knob would change how long
    the tail lasts rather than how it shimmers.

    v1.2.0 uses the same split in the other direction. The two paths sum
    different things — the output sums decorrelated grains reading different
    stretches of the ring, the loop recirculates self-similar material — so they
    obey different summing laws and take different window normalisations: POWER
    on the output, AMPLITUDE on the loop (see WindowLut::getLoopNorm). Neither
    constant may cross over.

    At the shipped defaults (gainRandom 0, Hann, tilt 0.5) both multipliers are
    exactly 1.0f, so gLout/gRout are bitwise equal to gL/gR, the two buffers hold
    identical values, and the v1.0 sound is reproduced exactly.

    Pool: fixed 32 preallocated slots. v1.3.0 raised the max sustained overlap
    from 8 to 16 (B2), so the headroom over steady state fell from 4x to 2x —
    which is what the review budgeted, and the remaining 2x is what covers
    delay-time transitions and the transient peaks the v1.1 randomisations
    create. find-inactive round-robin, and — since v1.1.0 — REFUSE on exhaustion
    rather than steal-oldest. The proven O-GrainScatter GrainPool shape, minus
    everything spatial/freeze. Zero allocation in processBlock.

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
    // v1.2.0: the FEEDBACK-TAP gains — equal-power pan x loopTrim, the window's
    // amplitude-duty normalisation. Never carries gainRandom (probe X).
    float       gL          = 0.70710677f;  // equal-power pan, latched (center in Phase 2.1)
    float       gR          = 0.70710677f;
    // v1.1.0: the OUTPUT gains — pan x per-grain random gain. Latched at spawn
    // like everything else, so a mid-flight gainRandom change never re-gains a
    // live grain. Never carries loopTrim, which would undo the window's POWER
    // normalisation on the output path.
    //
    // Both are written as pan * <multiplier> with the multiplier exactly 1.0f at
    // its no-op, which is bitwise identity — not "close to" the pan value. At
    // the shipped defaults (Hann, tilt 0.5, gainRandom 0) all four are therefore
    // bitwise equal to each other and to what v1.0.0 latched.
    float       gLout       = 0.70710677f;
    float       gRout       = 0.70710677f;
    int         age         = 0;            // samples alive
    int         startOffset = 0;            // block-transient: first render sample this block

    // ── v1.2.0 (B1): window shape + tilt, both latched at spawn ─────────────
    // Latched for the same reason everything else here is: the render loop must
    // be able to finish a live grain on the settings it started with. A grain
    // whose envelope SHAPE changed mid-flight would step from one window's value
    // to another's at the switch sample — the exact discontinuity the
    // latch-at-spawn contract exists to prevent, and one that no amount of
    // parameter smoothing can remove (the two windows disagree at every phase,
    // not just at the endpoints).
    //
    // `shape` is an index rather than a table pointer: the render loop resolves
    // the pointer once per grain per pass, so the indirection costs nothing in
    // the inner loop, and a stale index is a clamped read while a stale pointer
    // would not be.
    //
    // tiltT/A/B are WindowLut::Tilt's three fields, flattened to keep this a
    // plain POD. At grainTilt's 0.5 default they are { 0.5, 1.0f, 1.0f } and the
    // warp is the bitwise identity — see WindowLut.h.
    int         shape       = 0;            // WindowLut::hann
    float       tiltT       = 0.5f;
    float       tiltA       = 1.0f;
    float       tiltB       = 1.0f;

    // ── v1.4.0: Tukey taper α, latched at spawn like everything above ────────
    // Stored as the resolved GEOMETRY rather than as α, so the render loop does
    // no division: `taperActive` is false for all four non-Tukey shapes and the
    // remap is skipped entirely. Both fields are WindowLut::Taper flattened, for
    // the same reason tiltT/A/B are — this stays a plain POD.
    //
    // Latched for the usual reason and one extra: α changes the window's WIDTH,
    // so a live grain adopting a new α mid-flight would jump from one plateau
    // edge to another — a bigger discontinuity than a shape swap, not a smaller
    // one.
    bool        taperActive = false;
    float       taperInv    = 0.0f;         // 1 / (α/2)

    // ── v1.6.0 (B4 #2): read direction, latched at spawn ─────────────────────
    // −1 = reverse (the shipped law), +1 = forward. The ONE field that changes
    // what the inner loop computes rather than how loudly it computes it, and
    // an int64 rather than an int because `readAbs += step` must not promote
    // through a narrower type on the hot path.
    //
    // Latched for the usual reason — a grain that flipped direction mid-flight
    // would jump its read position by 2n samples in one sample, which is not a
    // click so much as a splice — and defaulted to −1 so a slot that is somehow
    // read before being written behaves as it always did.
    //
    // Collision safety is UNCHANGED and needs no extra clamp, which is worth
    // recording because it is the opposite of the intuition (a forward read head
    // moves TOWARD the write head rather than away from it). At pass-relative
    // index k a forward grain reads `passStartAbs − gD + k`, and A2's pass bound
    // already guarantees k < passLen <= grainDelayFloor <= gD — so the read is
    // strictly behind the write head at every k, at every host block size. The
    // ring span a forward grain needs is gD + G, which is SMALLER than the
    // reverse case's gD + 2·G that kCaptureSeconds is sized for.
    juce::int64 step        = -1;
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
