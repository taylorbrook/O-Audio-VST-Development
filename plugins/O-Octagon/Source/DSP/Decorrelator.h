/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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

#include <array>
#include <cstddef>

namespace oo
{

//==============================================================================
/**
    v1.5.0 — THE MONO DECORRELATOR BEHIND THE WIDTH CONTROL.

    ── The defect this exists to close (.planning/FEATURE-REVIEW.md, medium/small) ────────────────

    GainStage::renderChunk builds two feeds — `sL` and `sR` — and on a mono input bus `in1 IS in0`,
    so the two are BIT-FOR-BIT THE SAME SIGNAL. shaper::shape() then puts them metres apart and DBAP
    gives each its own gain vector, but the arithmetic that reaches the hall is two copies of one
    waveform arriving from two directions. That does not widen; it combs, at frequencies set by the
    path-length difference. Most fixed-media stems are effectively mono, so this is the common case,
    not the corner one — width was doing geometry with no signal diversity behind it.

    The fix is signal diversity: pass each feed through a DIFFERENT all-pass network so the two
    carry the same spectrum with unrelated phase. Two incoherent feeds a few metres apart spread;
    two coherent ones comb.

    ── WHY ALL-PASS AND NOT VELVET NOISE ─────────────────────────────────────────────────────────

    Both are standard. All-pass wins here on one specific property: it is EXACTLY unity-magnitude at
    every frequency, sample by sample, with no windowing and no tail normalisation. That matters
    more in this plugin than in most, because DBAP's constant-intensity claim (sum v_i^2 = 1,
    verified against an independent oracle to 1e-7 by the unit target) is a claim about the GAIN
    VECTORS, and it stays true only while the thing in front of them does not change a feed's
    energy. A velvet-noise FIR is unity-power in expectation over its length; an all-pass is unity
    at every bin, exactly. The second is the one that leaves the existing proof standing.

    It is also very much cheaper — eight ring reads per sample against a sparse convolution — which
    is what keeps PERF-01/PERF-02 unargued rather than re-measured.

    ── THE STRUCTURE: FOUR SCHROEDER SECTIONS, DIFFERENT DELAYS PER FEED ─────────────────────────

        H_k(z) = (-g + z^-M_k) / (1 - g z^-M_k)          g = kFeedback, |H_k| = 1 for all w

    Four in series per feed. The two feeds use DIFFERENT base delays (kBasesLeft vs kBasesRight),
    mutually incommensurate, so their group-delay curves share no structure. Both chains have a
    STRONG FIRST ARRIVAL at n = 0 with amplitude -g, which is why this does not pull the image:
    the direct sound is not delayed, so the precedence effect has nothing to act on. A pair of pure
    delays would decorrelate too, and would drag the phantom source onto the earlier feed.

    ── DEPTH SCALES THE DELAY LENGTHS, NOT g, AND NOT A DRY/WET MIX ──────────────────────────────

    The obvious continuous control is a dry/wet blend, and it is WRONG here in a way that is worth
    writing down because it looks right. Mixing a signal with an all-passed copy of itself gives
    |1 - m + m*e^(j phi)|, which sweeps from 1 down to |1 - 2m| — a null at every frequency where
    the chain has rotated by pi, and at m = 0.5 those nulls are infinitely deep. A "half
    decorrelated" feed would be a comb filter, which is the defect this file exists to remove.

    Scaling g is wrong for a different reason: at small g the section's first arrival (-g) is
    quieter than its first echo (1 - g^2), so the chain stops being a diffuser and becomes a slap.

    Scaling the DELAY LENGTHS is the axis that behaves. Dispersion time runs from ~0 to ~22 ms
    (left) / ~26 ms (right), which is the range decorrelators live in — below it the comb spacing
    becomes audible as a hollowness, above it the smear becomes an echo. And the low limit is
    benign in exactly the way the others are not: as depth falls, EVERY section in BOTH chains
    clamps to kMinDelaySamples, so the two chains CONVERGE TO THE SAME FILTER. Identical processing
    on both feeds means the pair stays perfectly correlated and their sum stays flat — the effect
    fades out to a common phase colour, never to a comb.

    ── NO JUCE, DELIBERATELY (the same rule SourceShaper.h lives under) ──────────────────────────

    <array> and nothing else. juce::dsp::DelayLine would do the job, but GainStage.cpp is
    deliberately absent from tests/unit/CMakeLists.txt, so anything that only exists inside it is
    unreachable by the unit target — and this class carries claims (the chains are all-pass; the
    two chains decorrelate; depth 0 collapses them onto each other) that are worth asserting
    directly rather than inferring from a render. Keeping the dependency at <array> is what lets
    the narrow unit link line survive, which gate 11 re-checks.
*/
namespace decorr
{
    /** Schroeder all-pass feedback. 0.7 is the middle of the usual 0.6-0.75 diffuser range: the
        first arrival (0.70) still dominates the first echo (0.51), so the section disperses rather
        than slapping. */
    inline constexpr float kFeedback = 0.7f;

    /** Base delays in SAMPLES AT 48 kHz, scaled to the running rate in prepare().

        Prime, and mutually incommensurate ACROSS the two sets as well as within them: a shared
        factor would put a common periodicity into both chains and hand back some of the coherence
        the whole file exists to remove. The right set sums longer than the left (26.1 ms against
        22.5 ms) so the two do not even share a total. */
    inline constexpr std::array<int, 4> kBasesLeft  { 113, 199, 317, 449 };
    inline constexpr std::array<int, 4> kBasesRight { 139, 233, 359, 521 };

    /** A section cannot read its own output: at M = 0 the feedback path has no delay in it and the
        difference equation has no solution. One sample is the floor, and it is also where both
        chains meet — see the note on the low limit above. */
    inline constexpr int kMinDelaySamples = 1;

    /** Width in metres at which depth reaches the dialled value.

        THE DEPTH IS SCALED BY THE EFFECTIVE WIDTH, NOT ONLY GATED ON IT, and that is the second
        half of the wEff rule stated in GainStage. A hard gate alone would step from "no filter" to
        "22 ms of dispersion" the instant width left zero. Ramping depth in over the first two
        metres means that at small spreads the two chains are still nearly converged, so the pair
        is still nearly correlated — which is what a listener expects from a narrow image, and it
        makes the boundary a fade rather than a switch.

        Two metres because that is roughly where the default rig's two sub-point gain vectors first
        differ enough to place the feeds separately at all; below it they are landing on the same
        speakers regardless. */
    inline constexpr float kFullDepthWidthMetres = 2.0f;
}

//==============================================================================
/** One Schroeder all-pass section over a power-of-two ring.

    POWER-OF-TWO SO THE WRAP IS A MASK. The read index is `writeIndex - delay`, which is far
    negative before wrapping, so the alternative is two modulos per section per sample — 16 of them
    per sample across the two chains, in the inner loop, for nothing.

    kSize is fixed at the worst case rather than sized per section: the longest base (521) at 192 kHz
    is 2084 samples, and 4096 covers it with room for the interpolated read. Eight of these is
    128 kB per plugin instance, which is not worth eight different template instantiations to halve.
*/
class AllPassSection
{
public:
    static constexpr int kSize     = 4096;
    static constexpr int kMask     = kSize - 1;

    /** The largest delay a read can ask for. One below the ring so the read and the write position
        can never land on the same slot. */
    static constexpr int kMaxDelaySamples = kSize - 1;

    static_assert ((kSize & kMask) == 0, "kSize must be a power of two: the wrap is a mask, not a %");

    void reset() noexcept
    {
        buffer.fill (0.0f);
        writeIndex = 0;
    }

    /** @param delaySamples  an INTEGER count, ALREADY CLAMPED by the caller to
                             [kMinDelaySamples, kMaxDelaySamples]
        @param g             feedback, |g| < 1

        ── WHY THE DELAY IS AN INTEGER AND NOT FRACTIONAL, MEASURED ─────────────────────────────

        The first draft read at a fractional position with linear interpolation, copying the
        alignment delay's approach one class over. IT DESTROYS THE ALL-PASS PROPERTY, which is the
        one thing this whole file is built on.

        A linear interpolator is a two-tap FIR with magnitude |(1-f) + f·e^(-jw)| — a lowpass that
        reaches |1-2f| at Nyquist, i.e. a NULL at f = 0.5. Outside a feedback loop that is a mild
        HF tilt. INSIDE one it is neither mild nor a tilt: it attenuates the recirculating signal
        and moves the pole, and the section stops being all-pass at all.

        Measured over 262144 samples of noise, chain gain against the input:

            depth  1.000    0.900    0.750    0.500    0.375    0.250    0.125
            gain   +0.00   -3.39    -3.95    -4.63    -4.09    -3.91    -3.22   dB

        Exactly unity at depth 1.0 — where base × depth lands on the integers the bases already
        are — and 3 to 4.6 dB down everywhere else. Summed with its partner feed the error reached
        -6.94 dB against a coherent sum, against the -3.01 dB incoherent addition predicts: not
        decorrelation, CANCELLATION, plus a lowpassed feed.

        THE FIRST DRAFT'S PROBE MEASURED DEPTH 1.0 AND PASSED. That is the trap this note exists
        to nail shut (pattern_test_fixture_mirrors_drift_silently, one layer up): depth 1.0 is the
        single value in the range at which the broken implementation is correct. Probe CV now
        sweeps nine depths and asserts the gain at every one.

        An integer read has no interpolator, so every section is exactly all-pass at every depth.
        The cost is that a delay STEPS by a sample as depth sweeps rather than gliding; see
        Decorrelator::process for why that is inaudible here and is not the alignment delay's
        situation.
    */
    float process (float x, int delaySamples, float g) noexcept
    {
        // `writeIndex` is where the NEXT sample lands, so `writeIndex - 1` holds w[n-1] and
        // `writeIndex - d` holds w[n-d].
        const float delayed = buffer[static_cast<std::size_t> ((writeIndex - delaySamples) & kMask)];

        // READ BEFORE WRITE. w[n] depends on w[n-M], so pushing first would let a section read the
        // sample it is about to store — the same ordering rule renderChunk states for the input
        // pointers, one layer down.
        const float w = x + g * delayed;

        buffer[static_cast<std::size_t> (writeIndex)] = w;
        writeIndex = (writeIndex + 1) & kMask;

        return delayed - g * w;
    }

private:
    std::array<float, kSize> buffer {};
    int                      writeIndex { 0 };
};

//==============================================================================
/** Four sections in series — one feed's chain. See the file header for the design. */
class Decorrelator
{
public:
    static constexpr int kNumSections = 4;

    /** Scales the base delays to the running rate, and clears the rings.

        THE BASES ARE IN SAMPLES AT 48 kHz AND THE SCALE IS WHAT KEEPS THEM IN MILLISECONDS. A
        chain left in samples would disperse over half the time at 96 kHz, so the same patch would
        widen differently on two machines — the same class of defect as a filter whose corner moves
        with the oversampling factor (critical_oversampled_path_filter_rate).

        Called ONLY from GainStage::prepare(), which is prepareToPlay and not a real-time context.
        Nothing here allocates in any case — the rings are std::array members.
    */
    void prepare (double sampleRateToUse, const std::array<int, kNumSections>& basesAt48k) noexcept
    {
        const auto scale = static_cast<float> (sampleRateToUse / 48000.0);

        for (int k = 0; k < kNumSections; ++k)
        {
            const float scaled = static_cast<float> (basesAt48k[static_cast<std::size_t> (k)]) * scale;

            // The clamp is a RAIL, not an expectation. 521 at 192 kHz is 2084 against a 4095
            // ceiling, so it never fires at any rate a host offers; if one ever does, a railed
            // chain is a shorter chain and an unrailed one is an out-of-bounds read.
            const auto ceiling = static_cast<float> (AllPassSection::kMaxDelaySamples);

            baseSamples[static_cast<std::size_t> (k)] = scaled < ceiling ? scaled : ceiling;
        }

        reset();
    }

    void reset() noexcept
    {
        for (auto& s : sections)
            s.reset();
    }

    /** @param x      the feed
        @param depth  0..1. Scales every section's delay; at 0 all four clamp to kMinDelaySamples,
                      which is where the left and right chains become the SAME filter.
    */
    float process (float x, float depth) noexcept
    {
        float y = x;

        for (int k = 0; k < kNumSections; ++k)
        {
            // ROUNDED, NOT TRUNCATED, AND INTEGER — see AllPassSection::process for the measured
            // reason the fractional read had to go.
            //
            // ── WHY STEPPING IS INAUDIBLE HERE AND WAS NOT FOR THE ALIGNMENT DELAY ────────────
            //
            // GainStage's alignment lines read fractionally on purpose: stepping a whole sample
            // there clicks on every venue edit, because that delay sits on an OUTPUT lane at
            // unity gain and a one-sample jump is a raw discontinuity in the programme material.
            //
            // Here the jump is INSIDE an all-pass, on the recirculating signal w, and it reaches
            // the output attenuated by the section's own gains and then by however many sections
            // follow it. The delays are also 56-521 samples long, so a one-sample step is under
            // 2% of the shortest — a change in the diffusion pattern, not in the waveform.
            //
            // And it only happens while the control is MOVING. Depth is a set-and-forget control
            // reached through a 5 ms ramp; a static setting re-derives the same integers every
            // control block and nothing steps at all.
            const auto wanted = static_cast<int> (baseSamples[static_cast<std::size_t> (k)] * depth
                                                  + 0.5f);

            const int d = wanted < decorr::kMinDelaySamples ? decorr::kMinDelaySamples
                        : wanted > AllPassSection::kMaxDelaySamples ? AllPassSection::kMaxDelaySamples
                                                                    : wanted;

            y = sections[static_cast<std::size_t> (k)].process (y, d, decorr::kFeedback);
        }

        return y;
    }

private:
    std::array<AllPassSection, kNumSections> sections {};

    /// The rate-scaled bases. Multiplied by depth at every sample — see process().
    std::array<float, kNumSections> baseSamples {};
};

} // namespace oo
