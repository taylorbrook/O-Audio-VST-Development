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
    /// v1.15.0 (B8) — the air blend's shape. Exact at 0 and 1, C1 at both ends.
    inline float smoothstep (float t) noexcept { return t * t * (3.0f - 2.0f * t); }
}


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
    //
    //     v1.15.0 (B13): not ramps any more — two-tap crossfades over kDelayFadeMs. Same step, same
    //     reason; the fade length is the rate-dependent part.
    for (auto& t : delayTap)
    {
        t.setLength (static_cast<int> (std::lround (kDelayFadeMs * 0.001 * sampleRateToUse)));
        t.unprime();
    }

    //     ── v1.5.0's two decorrelator ramps, on the SAME 5 ms, in the SAME step ──────────────
    //     The mix is a bypass crossfade and the depth is a control ramp, but both want the length
    //     everything else in this class uses: a decorrelator that faded in over a different time
    //     than the width ramp it belongs to would make one gesture arrive twice.
    decorrMix.reset   (sampleRateToUse, 0.005);
    decorrDepth.reset (sampleRateToUse, 0.005);

    //     ── v1.15.0's two air blends (B8), on the SAME 5 ms, in the SAME step ─────────────────
    airMixL.reset (sampleRateToUse, 0.005);
    airMixR.reset (sampleRateToUse, 0.005);

    //     ── v1.15.2's SAFE <-> binaural blend (B15), on the SAME 5 ms, in the SAME step ────────
    //     Its starting value is the first boundary's job, not this one's: prepare() has no
    //     binauralOn. Parked at 0 until then, so nothing reads a stale target.
    binauralMix.reset (sampleRateToUse, 0.005);
    binauralMix.setCurrentAndTargetValue (0.0f);
    binauralRun = false;

    //     v1.15.2 — everything below starts cleared, so nothing is stale; the first boundary of
    //     process() finishes the job prepare() cannot (it has no host clock and no bus facts).
    realStale     = false;
    firstBoundary = true;

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

    airL.prepare (sampleRateToUse);
    airR.prepare (sampleRateToUse);

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
    airRunL    = airRunR    = false;
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
    //     from silence. v1.15.2: in teleportToTargets(), because the first motion boundary of
    //     process() needs exactly the same teleport (B15).
    teleportToTargets();
}

//==============================================================================
void GainStage::teleportToTargets() noexcept
{
    // The gains — so sample 0 is already correct rather than 5 ms into a fade-in from silence.
    for (auto& s : gL) s.setCurrentAndTargetValue (s.getTargetValue());
    for (auto& s : gR) s.setCurrentAndTargetValue (s.getTargetValue());
    outGain.setCurrentAndTargetValue (outGain.getTargetValue());

    //     The delay ramps teleport too, and for a sharper reason than the gains': a render whose
    //     venue already carries delays must START aligned. Ramping 0 -> d over the first 5 ms
    //     would slide the read pointer across the head of every bounce, which is both audible and
    //     NOT block-size invariant to measure — QUAL-03 would still pass (both renders would slide
    //     identically) and QUAL-01's lead-in would be wrong.
    for (std::size_t i = 0; i < static_cast<std::size_t> (kNumSpeakers); ++i)
        delayTap[i].snap (delayTarget[i]);

    //     v1.15.0 (B8) — the air blends and the G ramps, for the gains' reason: a render that
    //     BEGINS outside the near field must start filtered, not 5 ms into a fade from dry.
    airMixL.setCurrentAndTargetValue (airMixL.getTargetValue());
    airMixR.setCurrentAndTargetValue (airMixR.getTargetValue());
    airL.snapToTarget();
    airR.snapToTarget();

    //     v1.15.1 — and the run flags re-derive, for decorrEngaged's reason below. v1.15.0 said
    //     they needed none; they do. SmoothedValue::reset (rate, ramp) in step 2 KEEPS the old
    //     target, so a prepare() after a run that ended filtered starts step 3 with a blend at 1.
    //     If the incoming patch is inactive, step 3 sets the target to 0, sees the blend still at
    //     1 and fading, and takes the run edge — then the teleport above lands the blend on 0
    //     under a flag that stays true, which the dirty check then never revisits. Settled now,
    //     so this is airRun = airActive, the pending seed survives exactly when the filter runs,
    //     and a later re-entry takes a real edge (seed + G snap).
    deriveAirRun();

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
        delayActive[i] = delayTap[i].to > 0.0f;
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

            // Read BEFORE updateControl() overwrites lastSolvedParams — prepare()'s solve stored
            // exactly what it was handed, so equality here means nothing moved since.
            const bool unchangedSincePrepare
                = firstBoundary && haveSolved
               && snapshot.generation == lastSolvedGeneration
               && std::memcmp (p.data(), lastSolvedParams.data(), sizeof (float) * params::kCount) == 0;

            updateControl (snapshot, p, offset, motionOn);

            // ── v1.15.2 (B15) — PREPARE'S MOTION TELEPORT, DEFERRED TO HERE ──────────────────
            //
            // prepare() solves with Motion OFF — it has no host clock — and teleports to THAT,
            // the anchor. The first boundary then solved anchor + offset and the gains walked
            // there over their ramp, so every motion render opened at the anchor and converged
            // over ~15 ms. This boundary is the first that knows the offset, so it repeats
            // prepare()'s step 4 against its own solve.
            //
            // ONLY WHEN THE OFFSET IS ALL THAT IS NEW. If the parameters or the venue moved
            // between prepare() and here, the walk from prepare()'s values is the ordinary 5 ms
            // ramp every static render also takes from the same place — teleporting would make a
            // moving render and its static equivalent disagree on sample 0 (probe DD's Z
            // companion), and doing it for static renders too moves the CU/DC anchored digests.
            // A bounce restores its state before prepare(), so it lands on this branch.
            if (firstBoundary && motionOn && unchangedSincePrepare)
                teleportToTargets();

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
            //
            // ── v1.15.2 (B15) — THE SAFE <-> BINAURAL BLEND, AND HOW THE FOLD ENGAGES ────────
            //
            // binauralOn no longer switches the stereo bus between the dry SAFE pair and the fold;
            // it sets the blend's target, and the arm keeps running while the blend fades out
            // (airRun's shape). The processor already guarantees binauralOn implies !mapped and a
            // two-channel buffer; the arm re-checks both so a fade-out the layout has overtaken
            // cannot select it.
            binauralMix.setTargetValue (binauralOn ? 1.0f : 0.0f);

            if (firstBoundary)
                binauralMix.setCurrentAndTargetValue (binauralOn ? 1.0f : 0.0f);

            binauralRun = ! mapped && numOut == 2
                       && (binauralOn || binauralMix.getCurrentValue() > 0.0f || binauralMix.isSmoothing());

            const bool foldWanted = (monitorOn && snapshot.monitorSlot[0] >= 0) || binauralRun;

            // Two engages land at mix 1 rather than ramping from the raw lanes: the first after
            // prepare() (which forces the fold to mix 0 — every Logic offline bounce of a stereo
            // track opened with a 240-sample swell from speaker 1/2 into the fold), and a binaural
            // engage from a stopped fold, whose crossfade the blend above already makes.
            //
            // Unmapped, nothing clocks the fold once the binaural arm stops — its blend has
            // already faded the fold out — so the disengage lands at once rather than leaving a
            // fade-out parked half-run with isRunning() true.
            if (foldWanted && (firstBoundary || (binauralRun && ! monitorFold.isRunning())))
                monitorFold.engageNow();
            else if (! foldWanted && ! mapped)
                monitorFold.disengageNow();
            else
                monitorFold.setEngaged (foldWanted);

            firstBoundary = false;
        }

        const int toBoundary = static_cast<int> (kControlBlock) - phase;
        const int chunk      = juce::jmin (numSamples - n, toBoundary);

        // v1.15.2: the arm is binauralRun (wanted, or fading out), not the processor's flag.
        renderChunk (buffer, n, chunk, numIn, numOut, mapped, binauralRun, snapshot, ping);

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
    {
        // v1.15.1 — NOT a bare return. The solve is skipped; the run flags are not, because a
        // fade begun by the last solve is still landing (see deriveAirRun()).
        deriveAirRun();
        deriveDecorrEngaged();
        deriveDelayEngaged();
        return;
    }

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

    // v1.13.0 — the air filter is driven by the sub-point's distance from the LISTENER (the rig
    // centroid, less the near field), NOT by d_hull. d_hull still drives the trim above. The
    // UNPROJECTED sub-point is used so the distance keeps growing beyond the hull rather than
    // freezing at the boundary. See HullProcessor.h for the measurement that forced this.
    const float dAirL = hullproc::airDistanceMetres (subPoints.left.x,  subPoints.left.y,
                                                     snapshot.centroid.x, snapshot.centroid.y,
                                                     snapshot.rigScale);
    const float dAirR = hullproc::airDistanceMetres (subPoints.right.x, subPoints.right.y,
                                                     snapshot.centroid.x, snapshot.centroid.y,
                                                     snapshot.rigScale);

    // The cutoff is EVALUATED unconditionally, even for a filter that is about to be skipped: it
    // is free at control rate and it keeps the counter identity airCutoffUpdates == solveRuns * 2
    // exact (P32).
    //
    // v1.15.0 (B8): it is APPLIED only while the filter is WANTED, below — the decorrDepth rule.
    // airAmount -> 0 and a source entering the near field both drive the cutoff to the CEILING
    // at the same boundary the blend starts fading the filter out, so applying it there would
    // sweep the filter from ~550 Hz to 20 kHz underneath the fade (measured: 12x a control at
    // 200 Hz, against 1.5x with the cutoff held). Held, the fade-out is a clean blend at the
    // cutoff the listener was hearing.
    const float fcL = hullproc::airCutoffHz (p[params::airAmount], dAirL, snapshot.rigScale, sampleRate);
    const float fcR = hullproc::airCutoffHz (p[params::airAmount], dAirR, snapshot.rigScale, sampleRate);

    instr::countAirCutoffUpdate();
    instr::countAirCutoffUpdate();

    // ── The D2 amendment: the skip condition is the PRODUCT being zero ────────────────────────
    //
    // airAmount == 0 defeats the filter everywhere (DSP-07/5); d_air == 0 defeats it inside the
    // near field at ANY airAmount (DSP-07/6, boundary moved from the hull to the near field in
    // v1.13.0), which is what keeps the shipping default patch bit-transparent. Both halves are
    // the same test.
    airActiveL = p[params::airAmount] > 0.0f && dAirL > 0.0f;
    airActiveR = p[params::airAmount] > 0.0f && dAirR > 0.0f;

    // ── v1.15.0 (B8) — THE BLEND, AND THE RUN FLAG IT KEEPS ALIVE ─────────────────────────────
    //
    // airActive no longer switches the filter; it sets the blend's target. The filter keeps RUNNING
    // while the blend fades out beneath it — the decorrelator's exact shape (decorrEngaged).
    airMixL.setTargetValue (airActiveL ? 1.0f : 0.0f);
    airMixR.setTargetValue (airActiveR ? 1.0f : 0.0f);

    // The G ramp's TARGET — the block that follows interpolates to it per sample from the previous
    // boundary's value instead of stepping (AirFilter.h). Only while wanted; see above.
    if (airActiveL) airL.setCutoff (fcL);
    if (airActiveR) airR.setCutoff (fcR);

    // The run flags and their entry edge (P27). In a helper since v1.15.1 because the dirty-check
    // skip above must re-derive them too — see deriveAirRun().
    deriveAirRun();

    // ── DSP-07/7 — THE airAmount -> 0 RESET IS GONE (v1.15.0) ─────────────────────────────────
    //
    // It zeroed both filters on the edge — the very block the new blend starts fading the filtered
    // signal OUT, so it would have stepped the output to 0 under a mix of 1 (B8's "air lane to 0",
    // 52x). Nothing needed it: a filter that stops running is seeded with s = x by the run edge
    // above before it produces another sample, so a zeroed state was always overwritten unread
    // (BD's scope note already said the two policies are observationally equivalent). The
    // criterion's intent — no stale state carried into a re-engage — is met by the seed.

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

    // The engage flag and its edge — deriveDecorrEngaged(), shared with the dirty-check skip.
    deriveDecorrEngaged();

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

        delayTarget[k] = samples;

    }

    // The per-lane active flags, the global gate and its engage edge — deriveDelayEngaged(),
    // shared with the dirty-check skip.
    deriveDelayEngaged();

    instr::countSolveRun();
}

//==============================================================================
// v1.15.1 — THE THREE RUN FLAGS, DERIVED IN ONE PLACE EACH
//
// Each flag below is "wanted, OR its fade still has something to fade". The second half changes
// while the parameters do NOT: a 5 ms fade outlives the one control block that started it, so the
// flag has to be re-derived on every boundary until the fade lands. Until v1.15.0 these lived
// inline past the dirty check, so with Motion off the solve that STARTED a fade-out was the last
// one ever to look — every later block skipped, and the flag latched true for the rest of the
// render. Settled at 0 the output stayed exact (mix 0 -> sL, a zero tap -> y), but the stage kept
// running, and the latched air flag swallowed the next false->true edge: re-entry was neither
// re-seeded nor snapped.
//
// So updateControl() calls all three from BOTH exits — the full solve and the skip — and
// prepare() re-derives after its step-4 teleport. On the skip only a falling edge is possible:
// nothing that sets a "wanted" half changed, so a flag that is false stays false.
//==============================================================================

void GainStage::deriveAirRun() noexcept
{
    const bool wasRunL = airRunL;
    const bool wasRunR = airRunR;

    airRunL = airActiveL || airMixL.getCurrentValue() > 0.0f || airMixL.isSmoothing();
    airRunR = airActiveR || airMixR.getCurrentValue() > 0.0f || airMixR.isSmoothing();

    // P27 / H1 — the false->true RUN edge arms a re-seed, and the seed makes the entry BIT-EXACT.
    // processSample computes v = G*(x - s), so seeding s = x gives v = 0.0f exactly and y = x on
    // every toolchain, at every cutoff, at every entry speed. §3.5.2's original policy (reset to 0)
    // produces a click at 430% of the signal's own maximum per-sample slew at 1 kHz, and D2's
    // "leave the state resident" produces 521%. This produces zero — and since v1.15.0 the blend
    // then fades the filter in from that exact start.
    //
    // THE RUN EDGE, NOT THE airActive EDGE. A source re-entering while its fade-out is still
    // running has a LIVE filter whose state is the programme's own; re-seeding it there would
    // step the output the blend is in the middle of carrying.
    //
    // The coefficient SNAPS on the same edge: a filter that was not running has no trajectory to
    // continue, and ramping G in from whatever it was held at last time would sweep the cutoff
    // under the fade-in for no reason.
    if (airRunL && ! wasRunL) { airSeedPendingL = true; airL.snapToTarget(); }
    if (airRunR && ! wasRunR) { airSeedPendingR = true; airR.snapToTarget(); }

    // A filter that is not running has nothing to seed. Normally the pending flag is consumed by
    // the very next renderChunk, before any boundary could get here; the one exception is
    // prepare(), whose step-3 solve can take the edge on a blend left at 1 by the previous run
    // (SmoothedValue::reset keeps the target) that step 4 then teleports to 0.
    if (! airRunL) airSeedPendingL = false;
    if (! airRunR) airSeedPendingR = false;
}

void GainStage::deriveDecorrEngaged() noexcept
{
    // Clocked while wanted AND for as long as the fade-out still has something to fade. Dropping
    // the chains the moment the parameter hit zero would cut the crossfade off at its start, which
    // is the click the crossfade exists to prevent. "Wanted" is the mix's target: updateControl()
    // sets it to exactly 1 or 0 from decorrWanted, and only there.
    const bool wasDecorrEngaged = decorrEngaged;

    decorrEngaged = decorrMix.getTargetValue() > 0.0f
                 || decorrMix.getCurrentValue() > 0.0f
                 || decorrMix.isSmoothing();

    // The engage edge. A memset over eight rings plus two index fills — no allocation, RT-safe,
    // and the same class of operation as the alignment lines' reset below.
    if (decorrEngaged && ! wasDecorrEngaged)
    {
        decorrL.reset();
        decorrR.reset();
    }

    decorrWasEngaged = decorrEngaged;
}

void GainStage::deriveDelayEngaged() noexcept
{
    bool anyDelay = false;

    for (std::size_t k = 0; k < static_cast<std::size_t> (kNumSpeakers); ++k)
    {
        // ACTIVE WHILE RAMPING DOWN, NOT ONLY WHILE THE TARGET IS NONZERO. A speaker returning
        // 12 ms -> 0 has a target of 0 the instant the edit lands; gating on the target alone
        // would drop it out of the delayed path on that same control block and CUT 12 ms of audio
        // that has been written to the line and not yet read — an audible click on exactly the
        // edit that was supposed to remove one. isSmoothing() keeps it reading until the ramp
        // reaches zero, where the two paths meet continuously.
        //
        // v1.15.0 (B13): "still ramping" is now "the tap still reads a nonzero position, or is
        // fading" — a target of 0 is reached when the sample loop's crossfade lands on it.
        delayActive[k] = delayTarget[k] > 0.0f || delayTap[k].to > 0.0f || delayTap[k].isFading();

        anyDelay = anyDelay || delayActive[k];
    }

    // ── THE ENGAGE EDGE ───────────────────────────────────────────────────────────────────────
    //
    // reset() on the false->true transition, so a line that last ran before the operator zeroed
    // every delay cannot emit the audio it was still holding. It is a memset over bufferData plus
    // two index fills — no allocation, RT-safe, and the same class of operation as the air
    // filter's recovery reset().
    //
    // A RECOVERY/ARMING SITE, NOT AN INITIALISATION SITE. P23/P30's "step 2 of prepare() is the
    // only place any DSP state is initialised, ever" is untouched: this restores the invariant
    // that rule establishes rather than establishing it a second time.
    if (anyDelay && ! delayEngaged)
        for (auto& line : alignDelay)
            line.reset();

    delayEngaged = anyDelay;
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

        // ── v1.15.2 (B20, v1.3.0 IN-04) — BACK FROM A STRETCH THAT DID NOT CLOCK THIS ARM ─────
        //
        // A SAFE chunk (the F3 flip, or a stereo bus with Headphones off) or a host bypass renders
        // without pushing the alignment lines, running the decorrelator rings or the air filters,
        // or folding. Each was left holding audio from before — up to 50 ms of it in a line — and
        // the first REAL chunk back played it out. They are cleared here, before this chunk's
        // first sample, which is the engage edges' own policy (delayEngaged, decorrEngaged,
        // MonitorFold::setEngaged): a network that last ran before the gap has no continuity
        // worth keeping. The air filters take the P27 seed instead of a zero, so a running filter
        // re-enters bit-exact on this chunk's first sample.
        if (realStale)
        {
            for (auto& line : alignDelay)
                line.reset();

            decorrL.reset();
            decorrR.reset();

            airSeedPendingL = airRunL;
            airSeedPendingR = airRunR;

            monitorFold.clearState();

            realStale = false;
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
            //
            // v1.15.0 (B8): the blends advance unconditionally too, and the filter runs on airRun
            // (active, or fading out). LERP, the decorrelator's form: at a blend of exactly 0 it is
            // `sL + 0.0f * (y - sL)` = sL, and at exactly 1 y is taken directly rather than through
            // `sL + (y - sL)`, which is NOT always y in float.
            //
            // The blend is SHAPED by smoothstep on top of the linear 5 ms ramp, which keeps both
            // ends exact (S(0) = 0, S(1) = 1) and removes the ramp's corners. Unshaped, those
            // corners alone left an air 1 -> 0 fade at 2.7x a control at 200 Hz (probe DU): the
            // filter runs as low as 552 Hz, so LP(x) − x is a large fraction of the programme.
            const float mL = smoothstep (airMixL.getNextValue());
            const float mR = smoothstep (airMixR.getNextValue());

            // The G ramps advance unconditionally as well — run or not — for the same reason.
            const float GL = airL.nextGain();
            const float GR = airR.nextGain();

            float fL = sL;
            float fR = sR;

            if (airRunL)
            {
                const float y = airL.process (sL, GL);
                fL = mL == 1.0f ? y : sL + mL * (y - sL);
                instr::countAirSampleFiltered();
                lastL = y;
            }

            if (airRunR)
            {
                const float y = airR.process (sR, GR);
                fR = mR == 1.0f ? y : sR + mR * (y - sR);
                instr::countAirSampleFiltered();
                lastR = y;
            }

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
                // delayEngaged can flip between chunks, and a fade that froze while unengaged
                // would resume mid-flight on the chunk that re-engages it.
                //
                // v1.15.0 (B13): a two-tap crossfade, not a sliding read position.
                auto&       tap = delayTap[k];
                const float w   = tap.next (delayTarget[k]);

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
                    instr::countDelaySample();

                    // Settled (w == 1): the single read v1.4.0 made, at a stationary position —
                    // CS's bit-exact shift is this line. Fading: the old tap WITHOUT advancing the
                    // read pointer, then the new one WITH it, so the pair is still one pop per push.
                    float delayed;

                    if (w == 1.0f)
                    {
                        delayed = alignDelay[k].popSample (0, tap.to);
                    }
                    else
                    {
                        const float a = alignDelay[k].popSample (0, tap.from, false);
                        const float b = alignDelay[k].popSample (0, tap.to);
                        delayed = a + w * (b - a);
                    }

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

            // ── v1.15.2 (B15) — THE SAFE <-> BINAURAL BLEND ───────────────────────────────────
            //
            // Settled at 1: the v1.15.1 copy, unchanged. Otherwise each sample is
            // `dry + m·(fold − dry)`, where dry is EXACTLY what the SAFE arm writes (the input at
            // unity, in1 = in0 on a mono input) and m is the 5 ms ramp through smoothstep. Both
            // dry values are read before either channel is written: in0 and in1 alias channels 0
            // and 1, and on a mono input in1 IS channel 0.
            if (binauralMix.getCurrentValue() == 1.0f && ! binauralMix.isSmoothing())
            {
                binauralMix.skip (count);

                for (int ch = 0; ch < numWrite; ++ch)
                    juce::FloatVectorOperations::copy (buffer.getWritePointer (ch) + start, out[ch], count);
            }
            else
            {
                float* const w0 = buffer.getWritePointer (0);
                float* const w1 = numWrite > 1 ? buffer.getWritePointer (1) : nullptr;

                for (int j = 0; j < count; ++j)
                {
                    const int   n  = start + j;
                    const float dL = in0 != nullptr ? in0[n] : 0.0f;
                    const float dR = in1 != nullptr ? in1[n] : 0.0f;
                    const float m  = smoothstep (binauralMix.getNextValue());

                    w0[n] = dL + m * (out[0][j] - dL);

                    if (w1 != nullptr)
                        w1[n] = dR + m * (out[1][j] - dR);
                }
            }
        }
        else if (monitorFold.isRunning())
        {
            monitorFold.fold (out, snapshot.monitorSlot[0], snapshot.monitorSlot[1], start, count);
        }

        // v1.15.2 — the blend advances in every arm; on the 8-channel bus it is parked at 0.
        if (! binaural)
            binauralMix.skip (count);
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

            // v1.15.0 (B8) — the air blends and G ramps advance here for the seventeen's reason.
            // The filter STATES are untouched, exactly as before.
            const float advancedML = airMixL.getNextValue();
            const float advancedMR = airMixR.getNextValue();
            const float advancedGL = airL.nextGain();
            const float advancedGR = airR.nextGain();
            juce::ignoreUnused (advancedML, advancedMR, advancedGL, advancedGR);

            // v1.15.2 (B20) — and the decorrelator's two. Until v1.15.1 this arm skipped them,
            // against the rule this arm opens with: a decorr move made in SAFE resumed its 5 ms
            // ramp on the first REAL chunk, from where it had frozen.
            const float advancedDM = decorrMix.getNextValue();
            const float advancedDD = decorrDepth.getNextValue();
            juce::ignoreUnused (advancedDM, advancedDD);

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
                const float advancedD = delayTap[static_cast<std::size_t> (i)]
                                            .next (delayTarget[static_cast<std::size_t> (i)]);

                juce::ignoreUnused (advancedL, advancedR, advancedD);
            }

            for (int ch = 0; ch < numWrite; ++ch)
                out[ch][n] = ch == 0 ? sL : sR;

            instr::countSampleAdvance();
        }

        // v1.15.2 — the binaural blend advances here too (parked at 0: binauralRun is false), and
        // this chunk clocked nothing in the REAL arm, so its state is stale (B20).
        binauralMix.skip (count);
        realStale = true;
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
