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

    // 2 — 5 ms linear ramps (§3.6.5). Long enough to kill zipper noise on fast weight and position
    //     automation (QUAL-04), short enough that a fast puck sweep tracks without audible lag.
    for (auto& s : gL) s.reset (sampleRateToUse, 0.005);
    for (auto& s : gR) s.reset (sampleRateToUse, 0.005);
    outGain.reset (sampleRateToUse, 0.005);

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

    // The filters start inactive and unseeded; updateControl() below establishes the real state
    // from the first solve, so a render that begins outside the hull takes the false->true edge and
    // gets its seed exactly as a mid-render crossing would.
    airActiveL = airActiveR = false;
    airSeedPendingL = airSeedPendingR = false;

    // 3 — force a solve. haveSolved is cleared so the dirty check cannot short-circuit the one
    //     update that establishes the starting gain vector.
    haveSolved = false;
    updateControl (snapshot, p);

    // 4 — teleport to that solve, so sample 0 is already correct rather than 5 ms into a fade-in
    //     from silence.
    for (auto& s : gL) s.setCurrentAndTargetValue (s.getTargetValue());
    for (auto& s : gR) s.setCurrentAndTargetValue (s.getTargetValue());
    outGain.setCurrentAndTargetValue (outGain.getTargetValue());
}

//==============================================================================
void GainStage::process (juce::AudioBuffer<float>& buffer, int numIn, int numOut, bool mapped,
                         const VenueSnapshot& snapshot, const ParamSnapshot& p,
                         VerifyPing* ping) noexcept
{
    const int numSamples = buffer.getNumSamples();

    // numSamples == 0 leaves the body unexecuted — pluginval issues those blocks. A buffer LARGER
    // than the prepared samplesPerBlock (pluginval at strictness 10 issues those too) is handled by
    // the chunk loop with no allocation and no special case.
    int n = 0;

    while (n < numSamples)
    {
        // & (kControlBlock - 1) rather than %, per the power-of-two static_assert in the header.
        const auto phase = static_cast<int> (absoluteSampleCounter & (kControlBlock - 1));

        if (phase == 0)
            updateControl (snapshot, p);

        const int toBoundary = static_cast<int> (kControlBlock) - phase;
        const int chunk      = juce::jmin (numSamples - n, toBoundary);

        renderChunk (buffer, n, chunk, numIn, numOut, mapped, snapshot, ping);

        n                     += chunk;
        absoluteSampleCounter += static_cast<std::uint64_t> (chunk);
    }
}

//==============================================================================
void GainStage::updateControl (const VenueSnapshot& snapshot, const ParamSnapshot& p) noexcept
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
    if (haveSolved
        && snapshot.generation == lastSolvedGeneration
        && std::memcmp (p.data(), lastSolvedParams.data(), sizeof (float) * params::kCount) == 0)
        return;

    lastSolvedParams     = p;
    lastSolvedGeneration = snapshot.generation;
    haveSolved           = true;

    // ── §5 steps 2 and 3 — puck to two sub-points ─────────────────────────────────────────────
    const float widthMetres = p[params::width];

    const auto subPoints = shaper::shape (snapshot,
                                          p[params::srcX], p[params::srcY], p[params::srcZ],
                                          widthMetres);

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

    const float dHullL = solveSubPoint (snapshot, subPoints.left,  p[params::srcZ], a, rs, w, vL, zCueL);
    const float dHullR = solveSubPoint (snapshot, subPoints.right, p[params::srcZ], a, rs, w, vR, zCueR);

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

    instr::countSolveRun();
}

//==============================================================================
void GainStage::renderChunk (juce::AudioBuffer<float>& buffer, int start, int count,
                             int numIn, int numOut, bool mapped,
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

    if (mapped)
    {
        // REAL mode. numOut is exactly 8 here — mappedOutputAvailable() requires it.
        //
        // THE ONLY PLACE IN THIS PLUGIN THAT INDEXES AN OUTPUT CHANNEL is snapshot.speakerToBuffer.
        // Hoisting the WRITE pointers is safe; hoisting a READ pointer is what H7 forbids.
        float* out[kNumSpeakers];

        for (int i = 0; i < kNumSpeakers; ++i)
            out[i] = buffer.getWritePointer (snapshot.speakerToBuffer[static_cast<std::size_t> (i)]);

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

            const float g = outGain.getNextValue();

            for (int i = 0; i < kNumSpeakers; ++i)
                out[i][n] = (gL[static_cast<std::size_t> (i)].getNextValue() * fL
                           + gR[static_cast<std::size_t> (i)].getNextValue() * fR) * g;

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
        if (ping != nullptr && ping->isActive())
            ping->overwrite (out, kNumSpeakers, start, count);
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

                juce::ignoreUnused (advancedL, advancedR);
            }

            for (int ch = 0; ch < numWrite; ++ch)
                out[ch][n] = ch == 0 ? sL : sR;

            instr::countSampleAdvance();
        }
    }
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
