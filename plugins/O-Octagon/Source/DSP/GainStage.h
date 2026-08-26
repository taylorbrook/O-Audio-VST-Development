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

#include <juce_audio_basics/juce_audio_basics.h>

// Phase 2.3 — §3.5.2's air filter. juce_dsp was ALREADY LINKED by all three targets before this
// phase (RESEARCH-2.3 H7 corrected CONTEXT-2.3 constraint 9 on that point); the INCLUDE is what was
// missing. Nothing the fast unit target compiles includes GainStage.h — GainStage.cpp is
// deliberately absent from tests/unit/CMakeLists.txt — so the narrow unit link line survives, and
// gate 11 re-checks that rather than trusting it.
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "../Data/VenueGeometry.h"
#include "../Data/VenueSnapshot.h"
#include "DbapSolver.h"
#include "HullProcessor.h"
#include "SourceShaper.h"

namespace oo
{

/** Phase 3.2's verify ping. FORWARD-DECLARED rather than included: this class only ever holds a
    pointer to one and calls two of its methods, and a full include here would put VerifyPing.h in
    front of every TU that includes GainStage.h for no benefit. */
class VerifyPing;

//==============================================================================
/**
    The 17 musical parameters, in one fixed order.

    The order is the CONTROL-BLOCK SNAPSHOT LAYOUT, and it is what the dirty check memcmps. It has no
    relationship to the five APVTS groups (which are a host-menu presentation choice) and changing it
    changes nothing observable — but id() must move with it, which is why the two live side by side.
*/
namespace params
{
    enum Index
    {
        srcX = 0, srcY, srcZ, width,
        rolloff, blur,
        w1, w2, w3, w4, w5, w6, w7, w8,
        hullAtten, airAmount,
        outputGain,
        kNumParams
    };

    inline constexpr std::size_t kCount = static_cast<std::size_t> (kNumParams);

    static_assert (kCount == 17,
                   "parameter-spec.md specifies 17 musical parameters and Phase 2.2 adds none. If "
                   "this fires, the enum and the APVTS layout have diverged and the control-block "
                   "snapshot is reading the wrong atomics.");

    /** THE SINGLE MAPPING between this enum and the APVTS parameter IDs.

        Ordered exactly as the enum above. The static_assert makes a forgotten entry a build error
        rather than a silently short table.
    */
    inline const char* id (int index) noexcept
    {
        static constexpr const char* ids[]
            = { "srcX", "srcY", "srcZ", "width",
                "rolloff", "blur",
                "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8",
                "hullAtten", "airAmount",
                "outputGain" };

        static_assert (sizeof (ids) / sizeof (ids[0]) == kCount,
                       "the id table and the Index enum have diverged");

        return ids[index];
    }
}

/** A sanitised snapshot of the 17 atomics. Flat and trivially copyable so the dirty check is one
    memcmp (see GainStage::updateControl). */
using ParamSnapshot = std::array<float, params::kCount>;

static_assert (std::is_trivially_copyable_v<ParamSnapshot>,
               "the dirty check compares this by memcmp");

//==============================================================================
/**
    ARCHITECTURE §3.6 — the 64-sample control grid, the 17 smoothed gains, and the per-sample inner
    loop. This is where audio becomes spatial.

    ── Why a control grid at all (§3.6.1) ────────────────────────────────────────────────────────
    PERF-02 wants the gain vectors recomputed on parameter change, not per sample. The obvious
    implementation — solve once per processBlock — is INCOMPATIBLE with QUAL-03: at blockSize 512 the
    targets update every 512 samples, at 4096 every 4096, the smoothing ramps differ, and the outputs
    diverge (`pattern_block_rate_envelope_breaks_blocksize_invariance`). Keying the boundaries to an
    ABSOLUTE sample counter makes the update positions identical regardless of how the host chopped
    the buffer, so both requirements hold at once.

    ── This class does not ask the processor anything (PLAN-2.2 P24) ─────────────────────────────
    It never calls getTotalNumOutputChannels() — the accessor that lies under the F3 hazard — and
    never re-derives the SAFE/REAL condition. The processor evaluates mappedOutputAvailable() (the
    Phase 2.1 helper, inherited verbatim) and passes the answer in. G1 stays stated in exactly one
    place.
*/
class GainStage
{
public:
    /** 64 samples — 1.33 ms at 48 kHz, comfortably shorter than the 5 ms smoothing ramp so the
        ramps overlap smoothly rather than stepping (§3.6.5). */
    static constexpr std::uint64_t kControlBlock = 64;

    static_assert ((kControlBlock & (kControlBlock - 1)) == 0,
                   "kControlBlock must be a power of two: the grid walk uses & (kControlBlock - 1) "
                   "in place of %, and 64 dividing 2^64 is also what makes the absolute counter's "
                   "eventual wrap preserve grid phase "
                   "(pattern_ring_invariant_needs_static_assert).");

    static constexpr int kNumSpeakers = 8;

    /// gL[8] + gR[8] + outGain. "All 17 smoothers" throughout the plan means exactly these.
    ///
    /// v1.4.0 ADDS EIGHT MORE SMOOTHERS AND DELIBERATELY LEAVES THIS AT 17. The delay ramps are a
    /// SEPARATE FAMILY: they carry samples, not gains, `currentSmoothedValues()` does not report
    /// them, and probe AT's "all 17 have arrived" assertion means what it always meant. Folding
    /// them in would have silently rewritten that probe's subject.
    static constexpr std::size_t kNumSmoothers = 2 * kNumSpeakers + 1;

    /// v1.4.0. The eight alignment-delay ramps — counted separately, per the note above.
    static constexpr std::size_t kNumDelaySmoothers = kNumSpeakers;

    /** The longest alignment delay a line is sized for, in ms.

        AN ALIAS of `oo::plane::kMaxAlignDelayMs`, which is also what `publishSnapshot()` rails to.
        They cannot disagree because there is only one of them — and if they could, a railed value
        would overrun the allocated line, where juce::dsp::DelayLine::setDelay jasserts in Debug
        and silently jlimits in Release into a delay quietly shorter than the table shows.
    */
    static constexpr float kMaxAlignDelayMs = plane::kMaxAlignDelayMs;

    //==========================================================================
    GainStage() = default;

    /** Resets the grid and teleports all 17 smoothers to a freshly solved gain vector.

        THE ORDER MATTERS AND IT CHANGES THE FIRST 240 SAMPLES OF EVERY RENDER (PLAN-2.2 P23):

          1. absoluteSampleCounter = 0
          2. reset (sr, 0.005) on all 17, AND prepare() on both air filters
          3. one updateControl()
          4. setCurrentAndTargetValue() on all 17 from that solve

        SmoothedValue::reset() calls setCurrentAndTargetValue(target), and on a fresh object target is
        0 — so without step 4 every render would begin with a 5 ms fade-in from silence. Deterministic,
        so QUAL-03 would still pass, but it corrupts the lead-in of the unity, Layer-3 and Σv²=1
        probes. With step 4, sample 0 is already correct.

        THIS IS THE ONLY INITIALISATION SITE FOR ANY DSP STATE, EVER (RESEARCH-2.2 H9, extended at
        2.3 by P30). reset() is a STATE reset that teleports all 17 to target; calling it anywhere
        else would jump every gain, and QUAL-03 would still pass because both renders would teleport
        identically. Only QUAL-01 would catch it, and only by luck.

        `filter.prepare()` performs the ANALOGOUS teleport — it does `sampleRate = ...;
        s1.resize (n); update(); reset();` — so it belongs in step 2 beside the seventeen, which is
        what makes the rule read "step 2 is the only place any DSP state is initialised, ever".

        ── prepare() on the filters is MANDATORY, not advisable (RESEARCH-2.3 H6) ─────────────────
        `std::vector<SampleType> s1 { 2 }` in juce_FirstOrderTPTFilter.h:149 is the INITIALIZER-LIST
        constructor: the default vector has size 1 holding the value 2.0f. An unprepared filter
        decays from 2.0 on its first outputs, and processSample (1, ...) on one is out of bounds.

        ── THE ONE OTHER reset() ON A FILTER IS A RECOVERY SITE, AND IS LABELLED AS SUCH ──────────
        renderChunk()'s per-block NaN guard calls airL/airR.reset() on a non-finite output (DSP-07/8,
        P31). That is a RECOVERY path, not an initialisation path, and it does not break the rule
        above — it restores the invariant the rule exists to establish. It also needs NO
        coefficient preservation, and the reason is specific: `pattern_biquad_nan_guard_sticky_silence`
        does not apply here because G is recomputed from cutoffFrequency at every control block and
        is never derived from the state. There is no last-known-good coefficient to keep. Without
        this note the next reader adds preservation that does nothing and looks prudent.
    */
    void prepare (double sampleRateToUse, int samplesPerBlock,
                  const VenueSnapshot& snapshot, const ParamSnapshot& p) noexcept;

    /** Renders one host block through the control grid.

        @param buffer    in-place; output channel 0 ALIASES input channel 0
        @param numIn     usable input channels, already bounded by buffer.getNumChannels()
        @param numOut    buffer.getNumChannels() — never 8, never getTotalNumOutputChannels()
        @param mapped    the processor's mappedOutputAvailable(numOut); REAL mode iff true
        @param snapshot  acquired ONCE by the caller and held for the whole block
        @param p         the sanitised 17-float parameter snapshot
    */
    /** @param ping  optional; when non-null AND active, its output OVERWRITES the eight mapped
                     lanes at the end of the REAL arm — after the sample loop and after the NaN
                     guard, through the SAME out[] pointers (§7.2 / §OQ2 / PLAN-3.2 P60).

                     DEFAULTED so every Phase 2.x call site compiles unchanged. Passed in rather
                     than reached for: this class does not ask the processor anything and does not
                     decide when the ping runs (P24). */
    void process (juce::AudioBuffer<float>& buffer, int numIn, int numOut, bool mapped,
                  const VenueSnapshot& snapshot, const ParamSnapshot& p,
                  VerifyPing* ping = nullptr) noexcept;

   #if OOCTAGON_INSTRUMENT
    /** The 17 smoothers' CURRENT values — gL[0..7], gR[0..7], outGain.

        Probe AT asserts all 17 have ARRIVED at target after 240 still samples. A desynchronised
        smoother has not, which is the only externally visible symptom of a skipped getNextValue().
    */
    std::array<float, kNumSmoothers> currentSmoothedValues() const noexcept;
   #endif

private:
    //==========================================================================
    void updateControl (const VenueSnapshot& snapshot, const ParamSnapshot& p) noexcept;

    void renderChunk (juce::AudioBuffer<float>& buffer, int start, int count,
                      int numIn, int numOut, bool mapped,
                      const VenueSnapshot& snapshot, VerifyPing* ping) noexcept;

    //==========================================================================
    using Smoother = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    std::array<Smoother, kNumSpeakers> gL {};
    std::array<Smoother, kNumSpeakers> gR {};
    Smoother                           outGain {};

    //==========================================================================
    // ── v1.4.0 — THE PER-SPEAKER ALIGNMENT DELAY ──────────────────────────────────────────────
    //
    // Post-solve, post-trim, post-outputGain, on the eight OUTPUT lanes, REAL mode only. It is the
    // last thing that happens to a sample before the NaN guard, and it happens BEFORE the verify
    // ping's overwrite so the ping stays undelayed — the ping bypasses DBAP, the weights, the hull
    // trim, the air filter, the per-speaker trim and outputGain precisely so that a ping from the
    // wrong speaker has exactly ONE possible cause, and letting the delay colour it would add a
    // second.
    //
    // ── EIGHT SEPARATE MONO INSTANCES, AND THAT IS MANDATORY RATHER THAN STYLISTIC ────────────
    //
    // THE SAME TRAP AS airL/airR, in a different class. juce::dsp::DelayLine keeps `delay`,
    // `delayInt` and `delayFrac` as PER-INSTANCE members (juce_DelayLine.h:318-319) and only
    // `bufferData`, `readPos` and `writePos` per channel. One instance prepared with
    // numChannels = 8 would therefore carry eight independent HISTORIES sharing ONE delay time —
    // silently correct while every speaker happens to agree, and silently wrong the instant two
    // differ, which is the only configuration this feature exists to produce.
    //
    // ── LINEAR INTERPOLATION, AND popSample() RATHER THAN setDelay() ──────────────────────────
    //
    // The delay is read at a SMOOTHED, fractional sample position that ramps over 5 ms on a venue
    // edit. Calling setDelay() at the control boundary and stepping the read pointer by whole
    // samples would click on every edit; popSample (ch, d) takes the position per sample and
    // interpolates, so a delay change glides. Linear is the right order here: the position moves
    // only during a 5 ms ramp and is stationary the rest of the time, so Lagrange3rd's extra taps
    // would buy accuracy on a signal that is not moving.
    using AlignDelayLine = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;

    std::array<AlignDelayLine, kNumSpeakers> alignDelay {};

    /// Target is the venue delay in SAMPLES (converted from the snapshot's ms at the control
    /// boundary, where `sampleRate` is known). Advances once per sample UNCONDITIONALLY, in both
    /// mode arms, for the same reason the seventeen do — see the SAFE arm's comment.
    std::array<Smoother, kNumSpeakers> delaySamples {};

    /// Per-speaker: does this lane READ from its line this chunk? False when the speaker's delay is
    /// 0 and settled, which makes `out[i][n] = y` the literal v1.3.5 expression — the zero-delay
    /// case is bit-identical by CONSTRUCTION rather than by an interpolation identity that would
    /// hold only while `value2` stays finite.
    std::array<bool, kNumSpeakers> delayActive {};

    /** Does ANY speaker want delay this chunk? Gates whether the eight lines are CLOCKED at all.
        Evaluated at the control boundary; constant for the whole chunk.

        ── WHY A GLOBAL LATCH AND NOT EIGHT INDEPENDENT ONES ─────────────────────────────────────
        A line that is pushed but not popped desynchronises writePos from readPos and its delay
        grows by one sample per sample — so "push always, pop sometimes" is not available. The
        choice is therefore between clocking all eight always and clocking none until one is wanted.
        Clocking none is what makes a venue with no delays — the default, and EVERY session and
        .venue written before v1.4.0 — cost exactly nothing and render bit-identically.
        Clocking ALL EIGHT once any one is wanted is what keeps the other seven's history warm, so
        the second and every later adjustment glides instead of dropping out.

        The one cold start is the first engage, which is a setup-time action against a room that has
        never had delays. It is preceded by a reset() so no stale audio can leak out of a line that
        last ran minutes ago.
    */
    bool delayEngaged { false };

    /// The allocated ceiling in samples, for the jlimit that keeps popSample off DelayLine's
    /// Debug jassert. Set in prepare() from the sample rate, beside the allocation it describes.
    float maxDelaySamples { 0.0f };

    //==========================================================================
    // ── §3.5.2 — the air-absorption filters ───────────────────────────────────────────────────
    //
    // TWO MONO INSTANCES, AND THAT IS MANDATORY RATHER THAN STYLISTIC (RESEARCH-2.3 H6). `G` is a
    // PER-FILTER member of FirstOrderTPTFilter, not per-channel: one instance prepared with
    // numChannels = 2 would carry two independent STATES but ONE SHARED CUTOFF. The two sub-points
    // have different d_hull whenever width > 0 and the source straddles the hull, so a single
    // 2-channel instance is silently wrong in exactly that configuration.
    //
    // PER SUB-POINT, NOT PER SPEAKER: the filter sits on the source feed BEFORE the gain matrix.
    // Per-speaker would be 4x the cost and would not be what §3.5.2 describes.
    juce::dsp::FirstOrderTPTFilter<float> airL, airR;

    /// Needed by airCutoffHz()'s Nyquist clamp at every control block, so it is a member rather
    /// than re-derived (H4 / P28). Set in prepare(), before step 3's updateControl().
    double sampleRate { 0.0 };

    /// `airAmount > 0 && d_hull > 0` for that sub-point, evaluated at the control boundary. The D2
    /// amendment: the skip condition is the PRODUCT being zero, so a source inside the hull is
    /// bit-transparent at any airAmount.
    bool airActiveL { false }, airActiveR { false };

    /** Set on the airActive false->true edge; consumed at the top of the next REAL renderChunk,
        which seeds the filter state with the incoming sample (PLAN-2.3 P27 / RESEARCH-2.3 H1).

        WHY A PENDING FLAG RATHER THAN AN IMMEDIATE reset() AT THE BOUNDARY: the seed value is a
        SAMPLE, and updateControl() has no buffer. Deferring to renderChunk is what makes the seed
        the actual first sample of the chunk.

        NOT CONSUMED IN SAFE MODE (RESEARCH-2.3 H10). SAFE mode does not apply the filter at all, so
        consuming the flag there would leave a mode flip mid-render with a stale pending reset —
        precisely the F3 window GainStage.cpp already warns about.
    */
    bool airSeedPendingL { false }, airSeedPendingR { false };

    /// Tracks the `airAmount > 0` edge so DSP-07/7's reset is a TRANSITION rather than a state
    /// test. Initialised false so prepare()'s first solve arms it correctly from silence.
    bool airWasEnabled { false };

    /** Absolute samples rendered since prepareToPlay(). Monotonic, reset THERE AND NOWHERE ELSE.

        Deliberately uint64_t and deliberately NOT derived from the playhead: a host locate would jump
        the grid and a loop would rewind it, which would make QUAL-03 both untestable and untrue.
        Overflow is a non-issue twice over — 2^64 samples is ~12 million years at 48 kHz, and 64
        divides 2^64 so even the wrap preserves grid phase. Do not "fix" this to an int.
    */
    std::uint64_t absoluteSampleCounter { 0 };

    ParamSnapshot lastSolvedParams {};
    std::uint32_t lastSolvedGeneration { 0 };
    bool          haveSolved { false };
};

} // namespace oo
