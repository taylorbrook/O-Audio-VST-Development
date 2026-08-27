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
#include <juce_dsp/juce_dsp.h>

#include <array>
#include "../Data/VenueGeometry.h"
#include "../Data/VenueSnapshot.h"
#include "DbapSolver.h"   // oo::instr — countMonitorSample(), and resetCounters() owns the reset

namespace oo
{

//==============================================================================
/**
    The binaural / stereo monitoring fold-down — the away-from-the-hall listening path.

    Folds the eight SOLVED speaker feeds to a headphone pair using the venue's own measured
    geometry: per-speaker azimuth and elevation about a listener at the audience centroid, a
    Woodworth inter-aural delay, a constant-power lateral pan, a 1/r distance gain and a
    head-shadow tilt on the shadowed ear.

    ── IT IS A POST-WRITE FOLD, NOT A SECOND RENDERER (the VerifyPing shape) ─────────────────────
    It runs at the END of GainStage::renderChunk()'s REAL arm, through the SAME `out[]` pointers,
    and it READS THE EIGHT LANES THAT WERE JUST WRITTEN. That placement is the whole design:

      * What you hear on headphones is the ACTUAL rig feed — post-solve, post-weights, post-hull
        trim, post-air, post-decorrelator, post-alignment-delay, post-trim, post-outputGain. A
        fold that re-derived the eight feeds from the parameters would be a SECOND implementation
        of the render, free to drift from the one the hall hears, and it would silently stop
        agreeing the first time either side changed. The whole value of a monitor mode is that it
        is not a different mix.
      * Two lanes are overwritten and the other SIX ARE HARD-ZEROED, exactly as the verify ping
        zeroes its seven. "Only the monitor pair sounds" is a property of this routine rather than
        of the material.

    ── THE FOLD IS NOT PART OF THE BIT-IDENTITY CONTRACT, AND CANNOT BREAK IT (§F9) ──────────────
    `isRunning()` is false whenever the fold is disengaged AND its crossfade has reached zero, and
    GainStage does not call fold() at all in that state. Nothing is clocked, no line is pushed, no
    filter is advanced. A session that never arms the monitor renders BIT-IDENTICALLY to v1.5.0 by
    CONSTRUCTION — the same structural argument `delayEngaged` makes for the alignment delay, and
    for the same reason it is a structural argument rather than an arithmetic one.

    The crossfade itself uses the LERP form `y + m * (fold - y)`, so at m == 0 the expression IS
    `y` exactly rather than to within an ulp — the `decorrMix` argument, reused.

    ── LATENCY IS DELIBERATELY NOT REPORTED ─────────────────────────────────────────────────────
    The ITD lines add up to ~0.66 ms. setLatencySamples() IS NOT CALLED FOR THEM AND MUST NOT BE.
    Reporting latency would move the host's delay compensation, which would change the RENDER — the
    exact contamination this feature is built to be incapable of. The monitor path is a listening
    aid; it never gets to influence what a bounce looks like. If a future reader "fixes" the
    unreported latency, they have inverted the feature's primary constraint.

    ── WHAT THIS IS NOT: HRTF ───────────────────────────────────────────────────────────────────
    There is no measured HRIR anywhere in this class and it does not claim externalisation on a par
    with one. A pure ITD/ILD model is FRONT-BACK AMBIGUOUS — a source at azimuth 0 and one at
    azimuth 180 produce the same inter-aural cues. The rear darkening below is a cheap standing-in
    for the pinna cue and gives a usable, not a convincing, front/back distinction. This is stated
    here so the limitation is a documented design point rather than a bug report.
*/
class MonitorFold
{
public:
    //==============================================================================
    static constexpr int kNumSpeakers = 8;

    /** Head radius for the Woodworth ITD, metres. The standard spherical-head value. */
    static constexpr float kHeadRadiusM = 0.0875f;

    /** Woodworth's maximum, at azimuth ±90°: (a/c)(π/2 + 1) ≈ 0.656 ms. Named so the allocation
        below is derived from the model rather than from a transcribed millisecond count
        (pattern_test_fixture_mirrors_drift_silently). */
    static constexpr float kMaxItdSeconds
        = (kHeadRadiusM / plane::kSpeedOfSoundMps) * (3.14159265f * 0.5f + 1.0f);

    /** What each line is ALLOCATED for. Comfortably above kMaxItdSeconds so the jlimit that keeps
        setDelay() off its Debug jassert is a statement of the bound rather than a truncation. */
    static constexpr float kLineSeconds = 0.0015f;

    /** THE FOLD TRIM, AND IT IS DERIVED RATHER THAN TASTED.

        The solver normalises the gain vector to Σvᵢ² = 1, and the pan below is constant power
        (gLᵢ² + gRᵢ² = 1). The worst case for one ear is every speaker panned hard to it and every
        lane coherent, giving Σvᵢ — which Cauchy-Schwarz bounds at √8·√(Σvᵢ²) = √8. Trimming by
        1/√8 therefore makes it STRUCTURALLY IMPOSSIBLE for the fold to exceed the peak of the
        material that fed it, at any position, in any venue.

        It is a CEILING argument, so the typical case — energy across two or three speakers — lands
        quieter than unity and the operator makes it up on the headphone amp. That is the correct
        trade for a path whose defining requirement is that it never surprises anyone. Probe DB
        asserts the resulting LEVEL; a probe written against this constant would agree with any
        value it was given and measure nothing (the kPinkNormScalar rule).
    */
    static constexpr float kFoldTrim = 0.35355339f;   // 1/√8

    // ── The head shadow ──────────────────────────────────────────────────────────────────────
    /// Ipsilateral: high enough to be transparent, low enough to stay off Nyquist at 44.1 kHz.
    static constexpr float kOpenCutoffHz = 18000.0f;
    /// Fully contralateral. The shadowed ear of a spherical head loses its top octaves.
    static constexpr float kShadowCutoffHz = 2500.0f;
    /// Multiplies the cutoff for a source directly behind — the pinna cue, approximated.
    static constexpr float kRearDarkenRatio = 0.55f;

    /** THE ILD CEILING, AND IT IS LOAD-BEARING RATHER THAN A TASTE CONTROL.

        Compresses the lateral coordinate before the constant-power pan so the far ear is never
        driven to silence. A REAL HEAD HAS A BROADBAND ILD OF ROUGHLY 15-20 dB AT 90 DEGREES, NOT
        AN INFINITE ONE, and the difference is not cosmetic:

          * An uncompressed constant-power pan gives gain 0 in the far ear at lat = +/-1. The
            inter-aural DELAY then has nothing to delay and the head SHADOW has nothing to filter,
            so both cues below silently stop existing for exactly the lateral sources they matter
            most for. The fold degrades into a hard-panned stereo mix that happens to own eight
            delay lines.
          * It is invisible to every "does the fold work" test that measures the NEAR ear. It was
            caught by asserting that a hard-right speaker reaches the LEFT ear at all.

        0.82 puts the far ear at cos(0.91*pi/2) = 0.141 against a near ear of 0.990 — 16.9 dB,
        inside the measured range. Probe DB asserts the resulting ILD lies in a PLAUSIBLE BAND
        rather than merely being non-zero, so neither this rail nor its absence can pass silently.
    */
    static constexpr float kPanDepth = 0.82f;

    /// Below this the 1/r gain is a division by nearly zero. A speaker AT the listener is a venue
    /// typo, not a mix decision.
    static constexpr float kMinRadiusM = 0.30f;

    static constexpr float kGainFloor = 0.05f;
    static constexpr float kGainCeil  = 4.0f;

    /// The engage/disengage crossfade, and the gain/delay ramps. The same 5 ms everything else in
    /// this plugin smooths over.
    static constexpr double kFadeSeconds = 0.005;

    //==============================================================================
    MonitorFold() = default;

    /** THE SINGLE INITIALISATION SITE for this class, mirroring GainStage::prepare()'s rule.

        Allocates the eight lines, prepares the sixteen shadow filters — `prepare()` on a
        FirstOrderTPTFilter is MANDATORY, not advisable: its `std::vector<SampleType> s1 { 2 }` is
        the initializer-list constructor, so an unprepared filter has ONE element holding 2.0f and
        processSample(1, ...) on it is out of bounds (RESEARCH-2.3 H6, the airL/airR trap) — and
        teleports every ramp to a freshly derived geometry so sample 0 is already correct.
    */
    void prepare (double sampleRateToUse, const VenueSnapshot& snapshot);

    /** Control boundary, audio thread. Re-derives the per-speaker fold coefficients when the venue
        has moved, and nothing at all when it has not.

        Gated on `snapshot.generation` ALONE and not on the parameter snapshot, because the fold is
        a function of the ROOM and the listener, never of the source position — the source's
        movement reaches the fold through the eight lane VALUES it reads, which is exactly what
        makes this a fold of the real feed rather than a second panner.
    */
    void updateGeometry (const VenueSnapshot& snapshot) noexcept;

    /** Control boundary, audio thread. Arms or disarms; the crossfade does the rest. */
    void setEngaged (bool shouldBeEngaged) noexcept;

    /** Is there anything to do? False when disengaged AND the crossfade has reached zero — which
        is the STRUCTURAL BYPASS the bit-identity claim rests on. GainStage must not call fold()
        when this is false. */
    bool isRunning() const noexcept { return engaged || mix.isSmoothing(); }

    /** AUDIO THREAD. Reads the eight written lanes, folds them, writes the monitor pair and
        hard-zeroes the other six — all through the caller's `out[]`.

        @param out          out[i] is speaker (i+1)'s buffer, already resolved through
                            snapshot.speakerToBuffer by the caller
        @param slotL,slotR  INDICES INTO out[], not buffer channels — the slots whose buffer
                            channel carries ChannelType left / right. Resolved on the message
                            thread and published in the snapshot, so this function performs no
                            channel lookup of its own and adds no second output-indexing site.
        @param start,count  the chunk, exactly as renderChunk() received it
    */
    void fold (float* const* out, int slotL, int slotR, int start, int count) noexcept;

private:
    //==============================================================================
    /** Woodworth's ITD, SIGNED, in seconds. Positive means the source is to the RIGHT and the LEFT
        ear is the far one.

        Extended past ±90° by the standard reflection so that a source directly BEHIND has zero
        ITD, as it must: f(θ) = θ + sin θ for |θ| ≤ π/2, and (π − θ) + sin θ beyond it. The two
        agree at π/2 — both give π/2 + 1 — so the curve is continuous where a naive clamp would
        put a corner.
    */
    static float woodworthSeconds (float azimuthRad) noexcept;

    void teleportRamps() noexcept;

    //==============================================================================
    using Smoother = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    /** ONE LINE PER SPEAKER, POPPED TWICE — not two lines per speaker.

        popSample (ch, d, false) reads at a fractional position WITHOUT advancing readPos
        (juce_DelayLine.cpp), so the near ear reads with updateReadPointer false and the far ear
        reads with it true. Eight lines carry both ears.

        THE PER-INSTANCE TRAP STILL APPLIES AND IS WHY THERE ARE EIGHT AND NOT ONE. DelayLine keeps
        `delay`, `delayInt` and `delayFrac` PER INSTANCE and only the ring per channel, so a single
        instance prepared with numChannels = 8 would hold eight histories sharing one delay time —
        silently right while every speaker agrees and silently wrong the moment two differ, which
        is every venue this feature exists for. (airL/airR, alignDelay, decorrL/decorrR: the same
        trap, now in its fourth class.)
    */
    using ItdLine = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;

    std::array<ItdLine, kNumSpeakers> itd {};

    /// SIXTEEN filters, per speaker per ear, and mandatory for the airL/airR reason: `G` is a
    /// per-FILTER member, so one instance with two channels would carry two states under ONE
    /// cutoff — and the two ears differing in cutoff is the entire content of a head shadow.
    std::array<juce::dsp::FirstOrderTPTFilter<float>, kNumSpeakers> shadowL, shadowR;

    /// Per speaker per ear. Ramped for the reason alignDelay's are: the coefficients jump on a
    /// venue edit, and a jump in either a gain or a fractional read position clicks.
    std::array<Smoother, kNumSpeakers> gainL, gainR;
    std::array<Smoother, kNumSpeakers> delayL, delayR;

    /// The dry -> folded crossfade. 0 or 1; only in between during a 5 ms transition.
    Smoother mix {};

    bool   engaged { false };
    double sampleRate { 0.0 };
    float  maxDelaySamples { 0.0f };

    /// The venue this geometry was derived from. 0 is "never derived" — publish() stamps from 1.
    std::uint32_t lastGeometryGeneration { 0 };
    bool          haveGeometry { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonitorFold)
};

} // namespace oo
