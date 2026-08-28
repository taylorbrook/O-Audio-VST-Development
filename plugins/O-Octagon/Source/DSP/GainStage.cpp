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
#include "GainStage.h"

#include "ConvexHull2D.h"
#include "VerifyPing.h"

#include "../Data/VenueGeometry.h"

#include <cmath>
#include <cstring>

namespace oo
{

namespace
{
    //==========================================================================
    // v1.3.0 — the srcZ proximity cue (§ audibility fix).
    //
    // DBAP normalises to Σ v_i² = 1, so a source rising toward (or above) the speaker plane changes
    // all eight distances nearly equally and the normalisation cancels almost everything: measured
    // over the default rig, the full −2 → +8 m sweep moved individual channels by ~1-2 dB and the
    // overall level by exactly nothing. Inside the hull there is no trim and no air filter either
    // (designed — see the note at solveSubPoint), so srcZ had no audible pathway at all.
    //
    // The cue restores one from the quantity the solver already computes: 1/k = √denom, the field
    // BEFORE normalisation (P69). It rises as the source nears the speakers and falls as it recedes.
    // Each sub-point's gain vector is trimmed by (invK_z / invK_0)^kZCueExponent, where invK_0 is
    // the SAME solve with srcZ removed — same (possibly hull-projected) x/y, same a, same r_s, same
    // weights, ear-plane height. Referencing to the ear plane rather than to an absolute distance
    // means the cue is EXACTLY 1.0 at srcZ = 0 (identical inputs → bit-identical invK → pow(1, γ)
    // = 1.0), so the shipped default is bit-transparent to this feature.
    //
    // Measured on the default rig at the puck's front-left position: −3 dB at z = −2 (sunk below
    // the plane), +4.5 dB approaching the speaker plane, −2 dB at z = +8 flying above the array —
    // a monotonic rise-then-recede narrative on top of the (sharper) focus change the solve itself
    // contributes. The clamp keeps a pathological venue from turning the cue into a fader.
    //
    // The reference solve runs UNCONDITIONALLY, srcZ = 0 included — same rule as the width = 0
    // second solve below: no branch may change the arithmetic path between control boundaries
    // (QUAL-03), and it keeps probe AE/BJ's pow budget EXACT (now 32 per solve pair, was 16).
    inline constexpr float kZCueExponent = 2.5f;
    inline constexpr float kZCueMinGain  = 0.5011872f;   // −6 dB
    inline constexpr float kZCueMaxGain  = 1.9952623f;   // +6 dB

    float zCueGain (float invK, float invKRef) noexcept
    {
        // Degenerate on either side (all-zero weights writes 0.0f) → no cue rather than a NaN/inf
        // that would latch the SmoothedValue targets.
        if (! (invK > 0.0f) || ! (invKRef > 0.0f))
            return 1.0f;

        const float cue = std::pow (invK / invKRef, kZCueExponent);

        return cue < kZCueMinGain ? kZCueMinGain
             : cue > kZCueMaxGain ? kZCueMaxGain
             : cue;
    }

    /** §5 steps 4 and 5 for one sub-point: classify against the hull, project if outside, solve.

        @param srcZ    the height offset already baked into `point`, so the z-cue reference solve
                       can strip it back off without re-deriving the audience plane.
        @param outZCue the srcZ proximity trim for this sub-point (see block comment above).

        @returns d_hull — the distance from the sub-point to the hull boundary, in metres, and
                 EXACTLY 0.0f when the sub-point is inside. §5 step 6 consumes it for both the gain
                 trim and the air cutoff.
    */
    float solveSubPoint (const VenueSnapshot& s, Vec3 point, float srcZ, float a, float rs,
                         const float w[GainStage::kNumSpeakers],
                         float outV[GainStage::kNumSpeakers], float& outZCue) noexcept
    {
        Vec3 solvePosition = point;

        const Vec2 floorPoint { point.x, point.y };

        // Returned EXPLICITLY on the inside path rather than left to fall out of a declaration —
        // an uninitialised d_hull would feed hullTrimGain and airCutoffHz with garbage that is
        // usually small enough to look plausible.
        float dHull = 0.0f;

        // PERF-01 acceptance criterion 3 — "hull projection only when outside" — is satisfied
        // STRUCTURALLY by this `if`, not by a comment. instr::hullProjections counts inside the
        // branch, so the criterion becomes a number rather than an argument about code placement.
        if (! hull::isInside (s.hullPts.data(), s.hullCount, floorPoint, s.hullEpsCross))
        {
            const auto projection = hull::project (s.hullPts.data(), s.hullCount, floorPoint);

            solvePosition.x = projection.point.x;
            solvePosition.y = projection.point.y;

            // Always >= 0, always finite, and 0.0f on every degenerate count
            // (ConvexHull2D.cpp:86-119) — so §5 step 6 needs no guard of its own.
            dHull = projection.distance;

            instr::countHullProjection();
        }

        // The hull is 2D on the floor while DBAP distances stay 3D (§3.1.1). A source at srcZ = +8 m
        // above room centre is therefore INSIDE the hull and receives no attenuation — designed
        // behaviour, srcZ being a musical control rather than an error condition. Not a bug to fix.
        // (Since v1.3.0 srcZ IS audible inside the hull, via the proximity cue below — a level
        // trim, not a hull attenuation.)
        float invK = 0.0f;
        dbap::solve (s.spk.data(), w, solvePosition, a, rs, outV, &invK);

        // The z-cue reference: the same solve with the height offset stripped. Same projected x/y,
        // same everything else, so at srcZ = 0 the two solves have IDENTICAL inputs and the cue is
        // exactly 1.0. The gain vector it writes is discarded — only the un-normalised field is
        // wanted here.
        Vec3 refPosition = solvePosition;
        refPosition.z -= srcZ;

        float vRef[GainStage::kNumSpeakers];
        float invKRef = 0.0f;
        dbap::solve (s.spk.data(), w, refPosition, a, rs, vRef, &invKRef);

        outZCue = zCueGain (invK, invKRef);

        return dHull;
    }
}

//==============================================================================
void GainStage::prepare (double sampleRateToUse, int samplesPerBlock,
                         const VenueSnapshot& snapshot, const ParamSnapshot& p) noexcept
{
    // 1 — the grid restarts. The ONLY site that touches this counter (P23).
    absoluteSampleCounter = 0;

    // v1.8.0. Free-mode motion restarts from phase 0 on re-prepare, like every other piece of DSP
    // state (RESEARCH Q7 / H9). The Perlin table is re-filled at the first motion boundary.
    motionClock = {};
    seededWith  = -1;

    // 2 — 5 ms linear ramps (§3.6.5). Long enough to kill zipper noise on fast weight and position
    //     automation (QUAL-04), short enough that a fast puck sweep tracks without audible lag.
    for (auto& s : gL) s.reset (sampleRateToUse, 0.005);
    for (auto& s : gR) s.reset (sampleRateToUse, 0.005);
    outGain.reset (sampleRateToUse, 0.005);

    //     ── v1.4.0's eight delay ramps, on the SAME 5 ms, in the SAME step ───────────────────
    //     Same ramp length as the gains and that is deliberate: a venue edit moves a trim and a
    //     delay together, and two different ramp lengths would make the pair audibly disagree
    //     about when the edit landed.
    for (auto& s : delaySamples) s.reset (sampleRateToUse, 0.005);

    //     ── v1.5.0's two decorrelator ramps, on the SAME 5 ms, in the SAME step ──────────────
    //     The mix is a bypass crossfade and the depth is a control ramp, but both want the length
    //     everything else in this class uses: a decorrelator that faded in over a different time
    //     than the width ramp it belongs to would make one gesture arrive twice.
    decorrMix.reset   (sampleRateToUse, 0.005);
    decorrDepth.reset (sampleRateToUse, 0.005);

    //     ── and the air filters, in the SAME step (PLAN-2.3 P30) ─────────────────────────────
    //     filter.prepare() is the analogous state teleport, so it belongs beside the seventeen
    //     rather than in a fourth place. See the header for why it is mandatory (H6: the default
    //     s1 is size 1 holding 2.0f) and why numChannels must be 1 rather than 2 (G is per-filter,
    //     so one 2-channel instance would share a cutoff between two sub-points that need
    //     different ones).
    //
    //     s1.resize() can allocate; it does not here (size 1 -> numChannels 1), and prepareToPlay
    //     is not a real-time context in any case — probe AO arms its counter after negotiate().
    sampleRate = sampleRateToUse;

    const juce::dsp::ProcessSpec spec { sampleRateToUse,
                                        static_cast<juce::uint32> (samplesPerBlock),
                                        1 };

    airL.prepare (spec);
    airR.prepare (spec);

    //     ── and the eight alignment lines, in the same step, for the same reason (v1.4.0) ────
    //
    //     ORDER IS FIXED: setMaximumDelayInSamples() sets `totalSize` and resizes bufferData
    //     against the CURRENT channel count (zero on a fresh object); prepare() then resizes it
    //     against spec.numChannels and resets. Reversing the two leaves an 8-sample line.
    //
    //     +2 on top of the ceiling, and it is not superstition: Linear interpolation reads
    //     index1 AND index1 + 1 (juce_DelayLine.h:220-232), so a read at exactly the maximum
    //     needs one sample beyond it. JUCE's own setMaximumDelayInSamples adds 2 as well; the
    //     margin here is over the ROUNDING of ms to samples, which std::ceil already covers, plus
    //     one for the fractional read.
    maxDelaySamples = static_cast<float> (std::ceil (kMaxAlignDelayMs * 0.001
                                                     * sampleRateToUse));

    const int maxDelayInt = static_cast<int> (maxDelaySamples) + 2;

    for (auto& line : alignDelay)
    {
        line.setMaximumDelayInSamples (maxDelayInt);
        line.prepare (spec);            // numChannels 1 — see the header on why not one x8 instance
    }

    //     ── and the two decorrelation chains, in the same step, for the same reason (v1.5.0) ─
    //
    //     prepare() here is a SCALE-AND-CLEAR, not an allocation: the rings are std::array members
    //     sized at the 192 kHz worst case, so the only rate-dependent thing is how many of those
    //     samples a section reads. That is what keeps the dispersion in MILLISECONDS across rates
    //     — see Decorrelator::prepare().
    decorrL.prepare (sampleRateToUse, decorr::kBasesLeft);
    decorrR.prepare (sampleRateToUse, decorr::kBasesRight);

    //     Unclocked and un-faded, exactly as the delay lines are. updateControl() below takes the
    //     false->true edge if the incoming patch already wants decorrelation.
    decorrEngaged = decorrWasEngaged = false;

    //     The lines start unclocked. updateControl() below establishes the real state from the
    //     first solve, so a render that BEGINS with delays already in the venue takes the
    //     false->true edge on its first control block and gets its reset there — the same shape
    //     the air filter's seed uses, rather than a second initialisation path here.
    delayEngaged = false;
    delayActive.fill (false);

    // The filters start inactive and unseeded; updateControl() below establishes the real state
    // from the first solve, so a render that begins outside the hull takes the false->true edge and
    // gets its seed exactly as a mid-render crossing would.
    airActiveL = airActiveR = false;
    airSeedPendingL = airSeedPendingR = false;

    // 3 — force a solve. haveSolved is cleared so the dirty check cannot short-circuit the one
    //     update that establishes the starting gain vector.
    haveSolved = false;

    // v1.7.0. Step 2's teleport for the monitor, in the class that owns it. MonitorFold::prepare()
    // allocates the eight ITD lines, prepares the sixteen shadow filters (MANDATORY: an unprepared
    // FirstOrderTPTFilter decays from 2.0f) and teleports its own ramps to a freshly derived
    // geometry. It also forces itself DISARMED — a monitor cannot survive a sample-rate change
    // armed, because every ramp is about to be re-derived at the new rate and a half-faded
    // crossfade would resume against coefficients belonging to the old one.
    monitorFold.prepare (sampleRateToUse, snapshot);

    // Motion OFF here regardless of p[motionOn]: prepare() has no host clock and sample 0's
    // offset is the first grid boundary's job. That boundary fires at absoluteSampleCounter == 0.
    updateControl (snapshot, p, {}, false);

    // 4 — teleport to that solve, so sample 0 is already correct rather than 5 ms into a fade-in
    //     from silence.
    for (auto& s : gL) s.setCurrentAndTargetValue (s.getTargetValue());
    for (auto& s : gR) s.setCurrentAndTargetValue (s.getTargetValue());
    outGain.setCurrentAndTargetValue (outGain.getTargetValue());

    //     The delay ramps teleport too, and for a sharper reason than the gains': a render whose
    //     venue already carries delays must START aligned. Ramping 0 -> d over the first 5 ms
    //     would slide the read pointer across the head of every bounce, which is both audible and
    //     NOT block-size invariant to measure — QUAL-03 would still pass (both renders would slide
    //     identically) and QUAL-01's lead-in would be wrong.
    for (auto& s : delaySamples) s.setCurrentAndTargetValue (s.getTargetValue());

    //     And the decorrelator's two, for the gains' reason rather than the delays': a render that
    //     BEGINS decorrelated must start at full mix, or its first 5 ms would be a crossfade out
    //     of a dry signal that was never playing. The depth teleports with it so the chains do not
    //     sweep across the head of the render either.
    decorrMix.setCurrentAndTargetValue   (decorrMix.getTargetValue());
    decorrDepth.setCurrentAndTargetValue (decorrDepth.getTargetValue());

    //     Same re-derivation the delay ramps need one block down, and for the same reason: the
    //     teleport moved decorrMix under an `decorrEngaged` that updateControl() computed while it
    //     was still at zero.
    decorrEngaged    = decorrMix.getCurrentValue() > 0.0f;
    decorrWasEngaged = decorrEngaged;

    if (decorrEngaged)
    {
        decorrL.reset();
        decorrR.reset();
    }

    //     Teleporting the ramps moved the targets under delayEngaged, which updateControl()
    //     computed one line earlier from a smoother that was still at zero. Re-derive it here so
    //     sample 0 of a delayed venue is already clocked rather than taking the edge — and reset
    //     the lines, because after a teleport there is no history worth keeping anyway.
    delayEngaged = false;

    for (std::size_t i = 0; i < static_cast<std::size_t> (kNumSpeakers); ++i)
    {
        delayActive[i] = delaySamples[i].getCurrentValue() > 0.0f;
        delayEngaged   = delayEngaged || delayActive[i];
    }

    if (delayEngaged)
        for (auto& line : alignDelay)
            line.reset();
}

//==============================================================================
void GainStage::process (juce::AudioBuffer<float>& buffer, int numIn, int numOut, bool mapped,
                         const VenueSnapshot& snapshot, const ParamSnapshot& p,
                         VerifyPing* ping, bool monitorOn, const motion::HostClock* clock,
                         bool binauralOn) noexcept
{
    const int numSamples = buffer.getNumSamples();

    // v1.8.0. The host block's first absolute sample — the PPQ in `clock` is the PPQ HERE, and
    // each grid boundary inside the block extrapolates from it by (boundary - blockStart) / sr.
    const std::uint64_t blockStart = absoluteSampleCounter;

    // numSamples == 0 leaves the body unexecuted — pluginval issues those blocks. A buffer LARGER
    // than the prepared samplesPerBlock (pluginval at strictness 10 issues those too) is handled by
    // the chunk loop with no allocation and no special case.
    int n = 0;

    while (n < numSamples)
    {
        // & (kControlBlock - 1) rather than %, per the power-of-two static_assert in the header.
        const auto phase = static_cast<int> (absoluteSampleCounter & (kControlBlock - 1));

        if (phase == 0)
        {
            // ── v1.8.0 — THE MOTION OFFSET, A PURE FUNCTION OF ABSOLUTE POSITION ─────────────
            //
            // Evaluated at the grid boundary and NOWHERE ELSE, from absoluteSampleCounter (Free)
            // or the block-start PPQ extrapolated to this boundary (Sync). Nothing here is
            // `+= rate * n / sr`: a 4096-sample block reaches the same boundaries as sixty-four
            // 64-sample blocks and computes the same offsets (probes DE/DF; negative control NC2).
            //
            // GATED, NOT ZEROED. When motion is off none of this runs and updateControl() takes
            // the v1.7.0 branch verbatim — the digest probe DC is that claim.
            Vec3       offset {};
            const bool motionOn = p[params::motionOn] > 0.5f;

            if (motionOn)
            {
                // seed() is a bounded 512-entry table fill: no allocation, and it runs only on a
                // seed CHANGE, at the boundary, never per sample.
                const int seed = static_cast<int> (p[params::motionSeed]);

                if (seed != seededWith)
                {
                    perlin.seed (static_cast<std::uint32_t> (seed));
                    seededWith = seed;
                }

                const double cycles = motion::cyclesAt (absoluteSampleCounter, sampleRate,
                                                        absoluteSampleCounter - blockStart, clock,
                                                        static_cast<int> (p[params::motionSync]),
                                                        p[params::motionRate], motionClock);

                const motion::MotionParams mp { static_cast<int> (p[params::motionPath]),
                                                p[params::motionSize],  p[params::motionRatio],
                                                p[params::motionAngle], p[params::motionHeight],
                                                p[params::motionPhase] };

                offset = motion::evaluate (mp, cycles, perlin);
            }

            liveOffset[0].store (offset.x, std::memory_order_relaxed);
            liveOffset[1].store (offset.y, std::memory_order_relaxed);
            liveOffset[2].store (offset.z, std::memory_order_relaxed);

            updateControl (snapshot, p, offset, motionOn);

            // ── v1.7.0 — THE MONITOR, DRIVEN FROM THE CONTROL BOUNDARY ───────────────────────
            //
            // OUTSIDE updateControl() ON PURPOSE. That function's dirty check is a memcmp of the
            // PARAMETER snapshot, and the fold is a function of the ROOM — it must re-derive on a
            // venue publish and must NOT re-derive when the source moves. Its own generation gate
            // is the right place for that; folding it into the parameter memcmp would couple two
            // independent staleness questions and get both subtly wrong.
            //
            // The monitor is refused on an unresolved pair here as well as upstream. The upstream
            // check is the gate; this is the backstop, and it is free.
            monitorFold.updateGeometry (snapshot);
            //
            // v1.11.0: the binaural arm engages the SAME fold. It needs no resolved monitor pair —
            // it folds a scratch whose slots are {0, 1} by construction — so the pair check
            // belongs only to the 8-channel monitor term.
            monitorFold.setEngaged ((monitorOn && snapshot.monitorSlot[0] >= 0) || binauralOn);
        }

        const int toBoundary = static_cast<int> (kControlBlock) - phase;
        const int chunk      = juce::jmin (numSamples - n, toBoundary);

        renderChunk (buffer, n, chunk, numIn, numOut, mapped, binauralOn, snapshot, ping);

        n                     += chunk;
        absoluteSampleCounter += static_cast<std::uint64_t> (chunk);
    }
}

//==============================================================================
void GainStage::updateControl (const VenueSnapshot& snapshot, const ParamSnapshot& p,
                               const Vec3 offset, const bool motionOn) noexcept
{
    // ── §5 step 1 — the dirty check ───────────────────────────────────────────────────────────
    //
    // memcmp, and it is memcmp BECAUSE OF NaN rather than despite it: `x != x` is true for a NaN
    // against itself, so an element-wise comparison would report "changed" every control block
    // forever and PERF-02's skip-when-unchanged would never fire again. The 17 floats are sanitised
    // at ingestion (P17) so a NaN should not arrive, but this comparison must not DEPEND on that.
    //
    // Known and accepted: -0.0f and +0.0f differ bitwise, so a parameter crossing zero from below
    // costs exactly one spurious re-solve. That is cheap and correct. Do NOT "fix" it to a tolerance
    // compare — a tolerance would start skipping real changes.
    //
    // The generation comes from INSIDE the snapshot (P16). Reading a separately-published counter is
    // the H1 bug: the check would go permanently stale against a venue edit, not transiently wrong.
    //
    // v1.8.0: `! motionOn` joins the conjunction. With motion on the offset changes every grid
    // while the parameter snapshot does not, so the skip MUST be bypassed; with motion off the
    // predicate is v1.7.0's, and PERF-02's skip-when-unchanged survives for every session that
    // does not use motion. Negative control NC1 (drop the `! motionOn`) is what proves DE
    // exercises this line.
    if (haveSolved
        && ! motionOn
        && snapshot.generation == lastSolvedGeneration
        && std::memcmp (p.data(), lastSolvedParams.data(), sizeof (float) * params::kCount) == 0)
        return;

    lastSolvedParams     = p;
    lastSolvedGeneration = snapshot.generation;
    haveSolved           = true;

    // ── §5 steps 2 and 3 — puck to two sub-points ─────────────────────────────────────────────
    const float widthMetres = p[params::width];

    // ── v1.8.0 — THE BRANCH (RESEARCH Q2). ────────────────────────────────────────────────────
    //
    // The else arm is the v1.7.0 call, character for character. The motion arm denormalises the
    // anchor through the SAME plane::normToMetres the shaper uses, adds the metric offset (D2 —
    // after denormalisation, so 6 m is 6 m in any hall) and enters at the metres seam. The
    // effective Z is computed ONCE and reaches BOTH consumers below: the shaper (sub-point
    // heights) and the z-cue solve — missing the second would move the source without moving its
    // level cue, a defect no digest catches (Risk 6).
    const float zEff = motionOn ? p[params::srcZ] + offset.z : p[params::srcZ];

    SubPoints subPoints;

    if (motionOn)
    {
        const Vec2 anchor = plane::normToMetres (snapshot.bbMinX, snapshot.bbMaxX,
                                                 snapshot.bbMinY, snapshot.bbMaxY,
                                                 p[params::srcX], p[params::srcY]);

        subPoints = shaper::shapeAt (snapshot,
                                     Vec2 { anchor.x + offset.x, anchor.y + offset.y },
                                     zEff, widthMetres);

        instr::countMotionSolve();
    }
    else
    {
        subPoints = shaper::shape (snapshot,
                                   p[params::srcX], p[params::srcY], p[params::srcZ],
                                   widthMetres);
    }

    // ── §5 steps 4 and 5 — hull, then DBAP, PER SUB-POINT ─────────────────────────────────────
    const float a  = dbap::rolloffToAlpha (p[params::rolloff]);
    const float rs = dbap::blurToRadius   (p[params::blur], snapshot.rigScale);

    float w[kNumSpeakers];

    for (int i = 0; i < kNumSpeakers; ++i)
        w[i] = p[static_cast<std::size_t> (params::w1 + i)];

    float vL[kNumSpeakers];
    float vR[kNumSpeakers];

    // BOTH solves run unconditionally, INCLUDING at width = 0 where the two sub-points coincide and
    // the second is arithmetically redundant. It still must not be elided: a branch that changes
    // the arithmetic path between control boundary A and boundary B is exactly the class of bug
    // QUAL-03 exists to catch (§3.4.3), and powCalls == 32 must hold EXACTLY at every width
    // (16 for the two sub-point solves + 16 for their z-cue reference solves since v1.3.0).
    float zCueL = 1.0f;
    float zCueR = 1.0f;

    // zEff IS p[params::srcZ] bit-for-bit when motion is off (a float copy, no arithmetic).
    const float dHullL = solveSubPoint (snapshot, subPoints.left,  zEff, a, rs, w, vL, zCueL);
    const float dHullR = solveSubPoint (snapshot, subPoints.right, zEff, a, rs, w, vR, zCueR);

    // ── §5 step 6 — hull gain trim, z-cue and air cutoff, PER SUB-POINT ───────────────────────
    //
    // The trims fold into v BEFORE step 7 rather than at the setTargetValue line, which is what
    // makes DSP-07/1's "folded into that sub-point's gain vector" literally true rather than
    // approximately true. The z-cue multiplies in at the same site: at srcZ = 0 it is exactly
    // 1.0f, and x * 1.0f is bit-identical, so the pre-1.3.0 arithmetic is preserved verbatim at
    // the default height.
    const float trimL = hullproc::hullTrimGain (p[params::hullAtten], dHullL) * zCueL;
    const float trimR = hullproc::hullTrimGain (p[params::hullAtten], dHullR) * zCueR;

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        vL[i] *= trimL;
        vR[i] *= trimR;
    }

    // The cutoff is set UNCONDITIONALLY, even on a filter that is about to be skipped. Setting it
    // is free at control rate, it keeps the counter identity airCutoffUpdates == solveRuns * 2
    // exact (P32), and it means a filter re-entering the active state already carries the right
    // coefficient rather than one control block of a stale one.
    airL.setCutoffFrequency (hullproc::airCutoffHz (p[params::airAmount], dHullL, sampleRate));
    airR.setCutoffFrequency (hullproc::airCutoffHz (p[params::airAmount], dHullR, sampleRate));

    instr::countAirCutoffUpdate();
    instr::countAirCutoffUpdate();

    // ── The D2 amendment: the skip condition is the PRODUCT being zero ────────────────────────
    //
    // airAmount == 0 defeats the filter everywhere (DSP-07/5); d_hull == 0 defeats it inside the
    // hull at ANY airAmount (DSP-07/6), which is what makes the shipping default patch
    // bit-transparent. Both halves are the same test.
    const bool wasActiveL = airActiveL;
    const bool wasActiveR = airActiveR;

    airActiveL = p[params::airAmount] > 0.0f && dHullL > 0.0f;
    airActiveR = p[params::airAmount] > 0.0f && dHullR > 0.0f;

    // P27 / H1 — the false->true edge arms a re-seed, and the seed makes the entry BIT-EXACT.
    // processSample computes v = G*(x - s), so seeding s = x gives v = 0.0f exactly and y = x on
    // every toolchain, at every cutoff, at every entry speed. §3.5.2's original policy (reset to 0)
    // produces a click at 430% of the signal's own maximum per-sample slew at 1 kHz, and D2's
    // "leave the state resident" produces 521%. This produces zero.
    if (airActiveL && ! wasActiveL) airSeedPendingL = true;
    if (airActiveR && ! wasActiveR) airSeedPendingR = true;

    // ── DSP-07/7 — reset ONLY on the airAmount -> 0 TRANSITION ────────────────────────────────
    //
    // A transition, not a state test: `if (airAmount == 0) reset()` would re-zero on every control
    // block for as long as the control stays down, which is observationally the same here but is
    // not what the criterion says, and it invites the collapse below.
    //
    // AND NOT ON EVERY d_hull == 0 BLOCK — the distinction is the single most likely thing to be
    // "simplified" away later. A puck oscillating across the hull edge crosses d_hull == 0
    // repeatedly; re-zeroing there would be the self-inflicted damage D2 rejected, and it would
    // also destroy the bit-exact entry above, because a filter that was just zeroed has s = 0
    // rather than s = x. Turning the CONTROL off is a different act: the user has defeated the
    // stage, so there is no continuity left to preserve.
    const bool airEnabled = p[params::airAmount] > 0.0f;

    if (airWasEnabled && ! airEnabled)
    {
        airL.reset();
        airR.reset();
    }

    airWasEnabled = airEnabled;

    // ── v1.5.0 — THE DECORRELATOR'S GATE AND ITS DEPTH ────────────────────────────────────────
    //
    // ── WHY THE GATE IS ON wEff AND NOT ONLY ON THE PARAMETER ─────────────────────────────────
    //
    // This is the whole correctness argument for the feature, and it is not obvious.
    //
    // At wEff == 0 the two sub-points COINCIDE. Probe AY asserts that v_L is then bit-for-bit
    // v_R, which is what makes §3.4.3's degenerate path — v_i * 0.5*(L+R), with no branch — a
    // clean mono sum. Decorrelating two feeds that are about to be multiplied by the SAME gain
    // vector and added does not widen anything: it makes a coherent sum incoherent, which costs
    // 3 dB and sounds phasey, at the one setting where this plugin currently guarantees the
    // arithmetic is transparent. That is the defect this feature exists to remove, reproduced by
    // the feature itself.
    //
    // So the decorrelator is wanted only where the sub-points are actually apart. wEff — not
    // p[width] — because the rFade collapse near the centroid drives the spread to zero WITHOUT
    // the parameter moving (§3.4.2), and a gate on the parameter would run the chains through the
    // collapse and back out.
    //
    // ── AND THE DEPTH IS SCALED BY IT, WHICH IS THE OTHER HALF ────────────────────────────────
    //
    // A gate alone would step from nothing to 22 ms of dispersion the instant wEff left zero.
    // Scaling depth by the same spread means that at small widths both chains sit near their
    // one-sample floor, where Decorrelator.h shows they CONVERGE TO THE SAME FILTER — so the pair
    // is still correlated, the sum is still flat, and the boundary is a fade rather than a switch.
    const float wEff = subPoints.wEff;

    const float widthRamp = juce::jlimit (0.0f, 1.0f, wEff / decorr::kFullDepthWidthMetres);

    const bool decorrWanted = p[params::decorr] > 0.0f && wEff > 0.0f;

    decorrMix.setTargetValue (decorrWanted ? 1.0f : 0.0f);

    // ONLY WHILE WANTED. Writing the target unconditionally would ramp depth toward zero during
    // the fade-out and sweep eight delay lines on the way — see the member's comment.
    if (decorrWanted)
        decorrDepth.setTargetValue (p[params::decorr] * widthRamp);

    // Clocked while wanted AND for as long as the fade-out still has something to fade. Dropping
    // the chains the moment the parameter hit zero would cut the crossfade off at its start, which
    // is the click the crossfade exists to prevent.
    const bool wasDecorrEngaged = decorrEngaged;

    decorrEngaged = decorrWanted
                 || decorrMix.getCurrentValue() > 0.0f
                 || decorrMix.isSmoothing();

    // The engage edge. A memset over eight rings plus two index fills — no allocation, RT-safe,
    // and the same class of operation as the alignment lines' reset a few lines down.
    if (decorrEngaged && ! wasDecorrEngaged)
    {
        decorrL.reset();
        decorrR.reset();
    }

    decorrWasEngaged = decorrEngaged;

    // ── §5 step 7 — set the 17 targets ────────────────────────────────────────────────────────
    for (int i = 0; i < kNumSpeakers; ++i)
    {
        // FUNC-07. trimLin has ridden in the snapshot since Phase 2.1 applied nowhere, so this is
        // a multiply rather than a plumbing change — and the multiply is what arms RESEARCH-2.3's
        // H5: v_i is EXACTLY 0.0f whenever w_i == 0 (DSP-05/1), so an unsanitised trimDb of 1e30
        // would give 0.0f * inf = NaN and latch the SmoothedValue into permanent silence. That is
        // closed at publishSnapshot(), the single funnel — see PluginProcessor.cpp.
        gL[static_cast<std::size_t> (i)].setTargetValue (vL[i] * snapshot.trimLin[static_cast<std::size_t> (i)]);
        gR[static_cast<std::size_t> (i)].setTargetValue (vR[i] * snapshot.trimLin[static_cast<std::size_t> (i)]);
    }

    outGain.setTargetValue (juce::Decibels::decibelsToGain (p[params::outputGain]));

    // ── §5 step 8 — v1.4.0's eight delay targets, and the two gates ───────────────────────────
    //
    // ms -> SAMPLES HAPPENS HERE AND NOWHERE ELSE. The snapshot carries milliseconds because
    // publishSnapshot() does not know the sample rate (see VenueSnapshot::delayMs); this function
    // does, as a member set in prepare(). One conversion site means a rate change cannot leave a
    // stale sample count anywhere — prepareToPlay() re-runs prepare(), which re-runs this.
    //
    // The value arrives ALREADY RAILED to [0, kVenueDelayClampMs] by the funnel. The jlimit is not
    // a second rail on the same quantity: it bounds the CONVERTED count against the line that was
    // actually allocated, which is a different number derived from a different input (the rate).
    // Without it a rate the host raised after prepare() — hosts do — would hand popSample a
    // position past the end of the buffer, where DelayLine jasserts in Debug and silently
    // jlimits in Release into a delay quietly shorter than the table shows.
    bool anyDelay = false;

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const auto k = static_cast<std::size_t> (i);

        // ── THE ARITHMETIC IS DOUBLE, AND THE CAST IS THE LAST THING THAT HAPPENS ─────────────
        //
        // `ms * 0.001f * (float) sampleRate` is the obvious spelling and it is WRONG BY AN ULP at
        // the values operators actually type. 10 ms at 48 kHz: 0.001f is 1.0000000474974513e-3,
        // 10.0f * that rounds UP to 1.00000007078e-2, and × 48000 gives 480.000033975 — which is
        // nearer to the float ABOVE 480 than to 480 itself, so the result is 480.0000305 and
        // delayFrac is 3.05e-5 instead of 0.
        //
        // That is not a rounding curiosity, it is audible arithmetic: popSample interpolates at
        // that fraction, so a delay the operator entered as a round number comes out as
        // (1-f)·x[n-480] + f·x[n-481] — a lowpass and a sub-sample error on EVERY sample, measured
        // at 1e-5 against a signal whose per-sample slope is 0.33 (probe CS found it by failing a
        // bit-compare that shift 480 otherwise matched to 5 decimal places).
        //
        // Done in double and cast once, 10 ms at 48 kHz is 480.0f EXACTLY, delayFrac is 0, and
        // Linear interpolation returns the stored sample untouched. Every round millisecond at
        // every standard rate lands on an exact sample count the same way.
        const float samples = juce::jlimit (0.0f, maxDelaySamples,
                                            static_cast<float> (static_cast<double> (snapshot.delayMs[k])
                                                                * 0.001 * sampleRate));

        delaySamples[k].setTargetValue (samples);

        // ACTIVE WHILE RAMPING DOWN, NOT ONLY WHILE THE TARGET IS NONZERO. A speaker returning
        // 12 ms -> 0 has a target of 0 the instant the edit lands; gating on the target alone
        // would drop it out of the delayed path on that same control block and CUT 12 ms of audio
        // that has been written to the line and not yet read — an audible click on exactly the
        // edit that was supposed to remove one. isSmoothing() keeps it reading until the ramp
        // reaches zero, where the two paths meet continuously.
        delayActive[k] = samples > 0.0f || delaySamples[k].isSmoothing();

        anyDelay = anyDelay || delayActive[k];
    }

    // ── THE ENGAGE EDGE ───────────────────────────────────────────────────────────────────────
    //
    // reset() on the false->true transition, so a line that last ran before the operator zeroed
    // every delay cannot emit the audio it was still holding. It is a memset over bufferData plus
    // two index fills — no allocation, RT-safe, and the same class of operation as the air
    // filter's recovery reset() a few lines up.
    //
    // A RECOVERY/ARMING SITE, NOT AN INITIALISATION SITE. P23/P30's "step 2 of prepare() is the
    // only place any DSP state is initialised, ever" is untouched: this restores the invariant
    // that rule establishes rather than establishing it a second time.
    if (anyDelay && ! delayEngaged)
        for (auto& line : alignDelay)
            line.reset();

    delayEngaged = anyDelay;

    instr::countSolveRun();
}

//==============================================================================
void GainStage::renderChunk (juce::AudioBuffer<float>& buffer, int start, int count,
                             int numIn, int numOut, bool mapped, bool binauralOn,
                             const VenueSnapshot& snapshot, VerifyPing* ping) noexcept
{
    // ── H7 — INPUT ALIASING, sharper at 2.2 than it was at 2.1 ────────────────────────────────
    //
    // out[0] and in[0] are the same memory, and PERF-01 forbids the scratch buffer that would make
    // this go away. At 2.1 every lane received the same value, so a channel-major write happened to
    // survive; at 2.2 the lanes differ and it does not.
    //
    // The rule below is therefore load-bearing: sL and sR are read at the TOP of each sample's
    // iteration, BEFORE any output write for that sample. The plausible-looking "optimisation" that
    // hoists a read pointer and writes channel-major WOULD PASS AT blockSize 1 and fail everywhere
    // else (pattern_grain_read_before_capture_write_blocksize) — probe AM's ragged sizes are what
    // catch it.
    const float* const in0 = numIn > 0 ? buffer.getReadPointer (0) : nullptr;

    // in1 IS in0 when the input bus is mono. numIn == 0 yields silence rather than a read of
    // channel 0 through a pointer that may not be an input at all.
    const float* const in1 = numIn > 1 ? buffer.getReadPointer (1) : in0;

    const int last = start + count;

    // v1.11.0. The binaural arm is the REAL arm with its eight write pointers aimed at the scratch
    // instead of the host buffer. The processor guarantees mapped and binauralOn are exclusive
    // (binauralOn requires !mapped); the jassert states it, and `mapped` wins if it were not.
    jassert (! (mapped && binauralOn));

    const bool binaural = binauralOn && ! mapped;

    if (mapped || binaural)
    {
        // REAL mode. numOut is exactly 8 here — mappedOutputAvailable() requires it — OR the
        // binaural arm, where numOut is 2 and the eight lanes below are the scratch.
        //
        // THE ONLY PLACE IN THIS PLUGIN THAT INDEXES AN OUTPUT CHANNEL is snapshot.speakerToBuffer.
        // Hoisting the WRITE pointers is safe; hoisting a READ pointer is what H7 forbids.
        //
        // `off` is the index the sample loop subtracts: 0 for the host buffer (the v1.10.1
        // expression exactly), `start` for the scratch, which is chunk-relative. An integer
        // subtraction on the index changes no float, so the 8-channel render is bit-identical.
        float* out[kNumSpeakers];
        const int off = binaural ? start : 0;

        if (binaural)
        {
            jassert (count <= static_cast<int> (kControlBlock));

            for (int i = 0; i < kNumSpeakers; ++i)
                out[i] = binauralLanes[static_cast<std::size_t> (i)].data();
        }
        else
        {
            for (int i = 0; i < kNumSpeakers; ++i)
                out[i] = buffer.getWritePointer (snapshot.speakerToBuffer[static_cast<std::size_t> (i)]);
        }

        // ── P27's re-seed, HOISTED OUT OF THE LOOP ────────────────────────────────────────────
        //
        // renderChunk is called immediately after the control boundary that set the flag, so the
        // first sample of this chunk IS the edge sample. No per-sample branch is added.
        //
        // THE EXPRESSION MUST MATCH sL/sR BELOW EXACTLY, nullptr case included. If it does not,
        // the seeded s differs from the x that arrives one line later, v = G*(x - s) is no longer
        // 0.0f, and the bit-exactness claim is false at precisely the one edge it exists to protect.
        if (airSeedPendingL)
        {
            airL.reset (in0 != nullptr ? 0.5f * in0[start] : 0.0f);
            airSeedPendingL = false;
        }

        if (airSeedPendingR)
        {
            airR.reset (in1 != nullptr ? 0.5f * in1[start] : 0.0f);
            airSeedPendingR = false;
        }

        // Tracked for the per-block NaN guard below. The filter state is private with no accessor
        // (juce_FirstOrderTPTFilter.h:149), and the OUTPUT check is exactly equivalent and
        // IMMEDIATE rather than one sample late: y = G*x + (1-G)*s is affine in s on the same
        // sample, so a non-finite s shows up in y at once (RESEARCH-2.3 Q1).
        //
        // THE LAST OUTPUT, NOT THE BLOCK MAXIMUM, AND THAT IS DELIBERATE TWICE OVER:
        //
        //   1. It is sufficient. Poisoning is STICKY BY CONSTRUCTION — s = y + v re-derives s from
        //      a value that is already non-finite — so if any sample in this chunk poisoned the
        //      filter, every later sample including this one is non-finite too. (An +inf input
        //      poisons on the FOLLOWING sample: v = G*(x - inf) = -inf, y = -inf + inf = NaN.)
        //
        //   2. A max would be WRONG. juce::jmax is `a < b ? b : a`, and `worst < NaN` is false, so
        //      a running max SILENTLY DISCARDS the NaN it exists to catch and the guard never
        //      fires. A probe driving a non-finite sample would then fail for a reason three lines
        //      away from where it looks.
        //
        // airActive cannot change inside a chunk — chunks never span a control boundary — so these
        // hold the last FILTERED output whenever the filter ran at all.
        float lastL = 0.0f, lastR = 0.0f;

        // The same tracking for the two decorrelation chains, which are recursive and therefore
        // sticky in the same way: w[n] = x[n] + g*w[n-M] re-derives the state from a value that is
        // already non-finite, so one poisoned sample silences the chain forever. The LAST output
        // is sufficient for the air filter's reason — poisoning is sticky by construction — and a
        // running max would be actively wrong for its reason too (jmax discards NaN silently).
        float lastDecorrL = 0.0f, lastDecorrR = 0.0f;

        for (int n = start; n < last; ++n)
        {
            const float sL = in0 != nullptr ? 0.5f * in0[n] : 0.0f;   // §3.4.3 level convention:
            const float sR = in1 != nullptr ? 0.5f * in1[n] : 0.0f;   // always two feeds at 0.5

            // §5 step 6, per sample. H7's aliasing rule is UNCHANGED: the filter inserts AFTER the
            // read of in0[n]/in1[n] and before any output write, so no read pointer is hoisted.
            //
            // The skip gates processSample AND NOTHING ELSE — every getNextValue() below still runs
            // exactly once, unconditionally (constraint 6, H10). That is what keeps QUAL-03 true
            // across a hull crossing.
            const float fL = airActiveL ? airL.processSample (0, sL) : sL;
            const float fR = airActiveR ? airR.processSample (0, sR) : sR;

            if (airActiveL) { instr::countAirSampleFiltered(); lastL = fL; }
            if (airActiveR) { instr::countAirSampleFiltered(); lastR = fR; }

            // ── v1.5.0 — THE DECORRELATOR ─────────────────────────────────────────────────────
            //
            // ADVANCED UNCONDITIONALLY, OUTSIDE THE BRANCH. The same rule the seventeen and the
            // eight delay ramps live under (§3.6.4, constraint 6/H10), and it matters here for
            // the delay ramps' exact reason: decorrEngaged can flip between chunks, and a ramp
            // frozen while disengaged would resume from a stale currentValue on the chunk that
            // re-engages it — which QUAL-03 would not catch, because both block sizes would
            // freeze identically.
            const float dMix   = decorrMix.getNextValue();
            const float dDepth = decorrDepth.getNextValue();

            float xL = fL;
            float xR = fR;

            if (decorrEngaged)
            {
                const float wetL = decorrL.process (fL, dDepth);
                const float wetR = decorrR.process (fR, dDepth);

                // LERP, NOT (1-m)*dry + m*wet. At m == 0 this is `fL + 0.0f * (wetL - fL)`, which
                // is fL EXACTLY — the v1.4.0 expression reached by arithmetic that cannot round
                // away from it, so the settled-but-not-yet-disengaged block is already
                // bit-transparent rather than transparent to within one ulp. The complementary
                // form would give (1-0)*fL + 0*wetL, whose first product is fL * 1.0f — also
                // exact, but only while the compiler keeps the multiply, which -ffast-math is
                // free not to.
                xL = fL + dMix * (wetL - fL);
                xR = fR + dMix * (wetR - fR);

                lastDecorrL = wetL;
                lastDecorrR = wetR;

                instr::countDecorrSample();
            }

            const float g = outGain.getNextValue();

            for (int i = 0; i < kNumSpeakers; ++i)
            {
                const auto k = static_cast<std::size_t> (i);

                const float y = (gL[k].getNextValue() * xL
                               + gR[k].getNextValue() * xR) * g;

                // v1.4.0. ADVANCED UNCONDITIONALLY, OUTSIDE THE delayEngaged BRANCH — the same
                // rule the seventeen live under (§3.6.4), and it matters here for the same reason:
                // delayEngaged can flip between chunks, and a ramp that froze while unengaged
                // would resume from a stale currentValue on the chunk that re-engages it.
                const float d = delaySamples[k].getNextValue();

                // ── THE ZERO-DELAY VENUE IS THE LITERAL v1.3.5 EXPRESSION ────────────────────
                //
                // Not "arithmetically equivalent to" — the same statement, reached without
                // touching a delay line at all. That is what makes "every session and .venue
                // written before v1.4.0 renders bit-identically" a structural claim rather than
                // a claim about Linear interpolation returning value1 when delayFrac is 0 (which
                // it does, right up until value2 is non-finite and 0.0f * inf is NaN).
                if (delayEngaged)
                {
                    // Push and pop are a MATCHED PAIR, both or neither. popSample advances
                    // readPos and pushSample advances writePos; clocking one without the other
                    // walks them apart and the delay grows by a sample per sample. This is why
                    // the gate is global rather than per-speaker — see the header.
                    alignDelay[k].pushSample (0, y);

                    const float delayed = alignDelay[k].popSample (0, d);

                    out[i][n - off] = delayActive[k] ? delayed : y;
                }
                else
                {
                    out[i][n - off] = y;
                }
            }

            instr::countSampleAdvance();
        }

        // ── DSP-07/8 — the NaN guard, ONCE PER BLOCK (P31, risk R6) ───────────────────────────
        //
        // The filter is the plugin's only recursive element on the signal path, and its state is
        // STICKY BY CONSTRUCTION: s = y + v re-derives s from a value that is already NaN, so once
        // poisoned it never recovers (pattern_envelope_follower_state_sticky_nan). That stickiness
        // is exactly what makes the last-output check above sufficient.
        //
        // reset() ALONE FULLY RESTORES THE FILTER — no coefficient preservation, and that is not an
        // oversight. pattern_biquad_nan_guard_sticky_silence does not apply verbatim here because G
        // is recomputed from cutoffFrequency at every control block and is never derived from the
        // state. There is no last-known-good coefficient to keep.
        //
        // This is a RECOVERY site, not an initialisation site — see the header, where the
        // one-reset-site rule is stated, so that this is read as an exception with a reason rather
        // than as the rule being broken.
        if (! std::isfinite (lastL)) airL.reset();
        if (! std::isfinite (lastR)) airR.reset();

        // v1.5.0, on the same schedule and for the same reason. Both chains are cleared when
        // EITHER poisons: they are a matched pair whose whole purpose is to differ from each
        // other, and restarting one against a ring the other has been filling for minutes would
        // leave the pair correlated in a way no probe would name.
        if (! std::isfinite (lastDecorrL) || ! std::isfinite (lastDecorrR))
        {
            decorrL.reset();
            decorrR.reset();
        }

        // ── FUNC-04 — THE VERIFY PING, AS A POST-WRITE OVERWRITE (§7.2 / §OQ2 / P60) ──────────────
        //
        // AFTER the write and AFTER the NaN guard, through the SAME out[] pointers — which are
        // snapshot.speakerToBuffer. Every part of that sentence is load-bearing:
        //
        //   * AFTER THE WRITE, not folded into the gain path. Bypassing DBAP, the weights, the hull
        //     trim, the air filter and outputGain is the entire point: a ping that comes out of the
        //     wrong speaker then has exactly ONE possible cause, which is the map. Probe BR asserts
        //     the level holds at outputGain +12 dB AND trim +6 dB simultaneously.
        //   * THROUGH out[], not through buffer.getWritePointer(i). Writing to raw channel i would
        //     make this a test of nothing — it would be correct on an identity map and wrong on
        //     every other one, which is why probe BQ uses a NON-IDENTITY map.
        //   * NO reset() ANYWHERE. The seventeen smoothers above have already advanced this sample,
        //     unconditionally, so on ping stop the DBAP signal resumes from state that never froze
        //     — there is nothing to teleport. P23/P30's one-reset-site-ever is untouched, and the
        //     ping's own 20 ms raised cosine owns both discontinuities (2.3's H1 argument).
        //
        // In SAFE mode this does not run at all, and it must not: the ping names a speaker, and in
        // SAFE mode there is no speaker N to name. The processor refuses to start one there and
        // aborts a running one on the flip (Q5).
        //
        // v1.11.0: never in the binaural arm either. The processor aborts a ping the moment
        // `mapped` goes false, and `mapped` is the guard here rather than `binaural` so the
        // scratch — which has no speaker N to name — can never receive one.
        if (mapped && ping != nullptr && ping->isActive())
            ping->overwrite (out, kNumSpeakers, start, count);

        // ── v1.7.0 — THE MONITOR FOLD, AND IT IS THE LAST THING THAT HAPPENS ──────────────────
        //
        // AFTER the ping so its six-lane mute is authoritative. The two are mutually exclusive
        // upstream — arming either drops the other — so this ordering is a backstop rather than a
        // policy, but the ordering still has to be decided and this is the safe direction: if both
        // were somehow live, "the rig lanes are silent" stays true.
        //
        // isRunning() FALSE IS THE STRUCTURAL BYPASS, and it is what makes "every session written
        // before v1.7.0 renders bit-identically" a claim about CONSTRUCTION rather than about
        // arithmetic. Nothing below is clocked, no line is pushed, no filter advances — the same
        // shape as `delayEngaged` false clocking no delay line, and deliberately NOT the
        // advance-unconditionally rule the smoothers live under (see the member's comment).
        if (binaural)
        {
            // ── v1.11.0 — THE STEREO-BUS BINAURAL ARM'S TAIL ─────────────────────────────────
            //
            // The eight solved feeds are in the scratch, chunk-relative. Three things, in order:
            //
            //   1. METER THE LANES BEFORE THE FOLD. The host buffer will only ever carry the
            //      folded pair, and the operator on a stereo bus is working on exactly the
            //      per-speaker picture the eight meters exist to show. Same post-map, post-trim
            //      signal the 8-channel meters read; max-merged into atomics the processor
            //      exchanges to zero on its poll.
            //   2. FOLD IN PLACE with slots {0, 1}: the scratch is identity-ordered by
            //      construction, so no monitor-pair resolution is needed or consulted — the
            //      resolved pair in the snapshot describes the HOST buffer of an 8-channel bus
            //      and means nothing here. fold() reads all eight before writing any, so the
            //      in-place write is safe (its own H7 rule).
            //   3. COPY THE PAIR to host channels 0 and 1. The input pointers in0/in1 alias
            //      those channels, and every read of them for this chunk happened in the sample
            //      loop above — the copy is the last write and nothing reads after it.
            //
            // isRunning() is true for the whole time the arm is selected: setEngaged (true) was
            // issued at the control boundary. During the 5 ms engage ramp the pair carries
            // (1 - m) of speakers 1 and 2's raw feeds — the same crossfade the 8-channel monitor
            // makes, and inaudible for the same reason.
            for (int i = 0; i < kNumSpeakers; ++i)
            {
                const auto k = static_cast<std::size_t> (i);
                const auto mm = juce::FloatVectorOperations::findMinAndMax (out[i], count);
                const float pk = juce::jmax (std::abs (mm.getStart()), std::abs (mm.getEnd()));

                if (pk > binauralLanePeak[k].load (std::memory_order_relaxed))
                    binauralLanePeak[k].store (pk, std::memory_order_relaxed);
            }

            if (monitorFold.isRunning())
                monitorFold.fold (out, 0, 1, 0, count);

            const int numWrite = juce::jmin (numOut, 2);

            for (int ch = 0; ch < numWrite; ++ch)
                juce::FloatVectorOperations::copy (buffer.getWritePointer (ch) + start, out[ch], count);
        }
        else if (monitorFold.isRunning())
        {
            monitorFold.fold (out, snapshot.monitorSlot[0], snapshot.monitorSlot[1], start, count);
        }
    }
    else
    {
        // ── SAFE mode (§5, ARCHITECTURE OQ1) ──────────────────────────────────────────────────
        //
        // Load-bearing for AU, not only for Standalone on a stereo interface: JUCE derives the
        // (1,1), (1,2), (2,1) and (2,2) channel configs from isBusesLayoutSupported() and auval
        // exercises all of them. Also the F3 path, where the map is valid but the buffer is
        // narrower than it.
        //
        // ALL 17 SMOOTHERS STILL ADVANCE, EXACTLY ONCE PER SAMPLE (§3.6.4 says *unconditionally*).
        // A mode branch that skipped getNextValue() would be precisely the branch that section
        // forbids — and the F3 hazard can flip modes BETWEEN blocks with no intervening
        // prepareToPlay(), at which point frozen smoothers would resume from a stale currentValue.
        //
        // CONTRACT-MANDATED SURPRISE: §5 says the per-sample stage writes "the dry input at unity",
        // so outGain is deliberately NOT applied here — the Output knob is inert on a mono/stereo
        // output bus. Probe AT asserts it. If that is to change it changes at a discuss boundary,
        // not silently in this function.
        //
        // THE AIR FILTER IS NOT APPLIED HERE EITHER, AND THE PENDING SEED FLAGS ARE NOT CONSUMED
        // (RESEARCH-2.3 H10). Consuming them would be worse than useless: SAFE mode never calls
        // processSample, so the seed would be spent on a filter that is not running, and the F3
        // hazard can flip back to REAL mode BETWEEN blocks with no intervening prepareToPlay() —
        // at which point the filter would enter active with a stale resident state and no pending
        // reset, which is exactly the click P27 exists to prevent. Left pending, the seed is
        // applied by the first REAL chunk instead.
        float* out[kNumSpeakers];

        // The bus predicate caps the output bus at 8 channels and the input bus at 2, and JUCE sizes
        // the block buffer at max(totalIn, totalOut) — so numOut can never exceed 8 here. jmin is a
        // statement of that bound, not a truncation.
        const int numWrite = juce::jmin (numOut, kNumSpeakers);

        for (int ch = 0; ch < numWrite; ++ch)
            out[ch] = buffer.getWritePointer (ch);

        for (int n = start; n < last; ++n)
        {
            const float sL = in0 != nullptr ? in0[n] : 0.0f;          // unity, not 0.5
            const float sR = in1 != nullptr ? in1[n] : 0.0f;

            const float g = outGain.getNextValue();
            juce::ignoreUnused (g);

            for (int i = 0; i < kNumSpeakers; ++i)
            {
                const float advancedL = gL[static_cast<std::size_t> (i)].getNextValue();
                const float advancedR = gR[static_cast<std::size_t> (i)].getNextValue();

                // v1.4.0's eight, advanced here for EXACTLY the reason stated at the top of this
                // arm. The F3 hazard flips REAL <-> SAFE between blocks with no intervening
                // prepareToPlay(); a delay ramp frozen through a SAFE stretch would resume from a
                // stale currentValue and slide the read pointer across the first samples back in
                // REAL mode. The lines themselves are NOT clocked here — SAFE mode writes the dry
                // input at unity and there is no speaker N to align — which is the same split the
                // air filter uses (state untouched, pending seed left pending).
                const float advancedD = delaySamples[static_cast<std::size_t> (i)].getNextValue();

                juce::ignoreUnused (advancedL, advancedR, advancedD);
            }

            for (int ch = 0; ch < numWrite; ++ch)
                out[ch][n] = ch == 0 ? sL : sR;

            instr::countSampleAdvance();
        }
    }
}

//==============================================================================
std::array<float, GainStage::kNumSpeakers> GainStage::readAndZeroBinauralLanePeaks() noexcept
{
    std::array<float, kNumSpeakers> peaks {};

    for (int i = 0; i < kNumSpeakers; ++i)
        peaks[static_cast<std::size_t> (i)] =
            binauralLanePeak[static_cast<std::size_t> (i)].exchange (0.0f, std::memory_order_relaxed);

    return peaks;
}

//==============================================================================
#if OOCTAGON_INSTRUMENT
std::array<float, GainStage::kNumSmoothers> GainStage::currentSmoothedValues() const noexcept
{
    std::array<float, kNumSmoothers> values {};

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        values[static_cast<std::size_t> (i)]                  = gL[static_cast<std::size_t> (i)].getCurrentValue();
        values[static_cast<std::size_t> (i + kNumSpeakers)]   = gR[static_cast<std::size_t> (i)].getCurrentValue();
    }

    values[kNumSmoothers - 1] = outGain.getCurrentValue();

    return values;
}
#endif

} // namespace oo
