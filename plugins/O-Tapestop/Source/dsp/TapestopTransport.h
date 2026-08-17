/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop - TapestopTransport (Phase 2.2: Stop mode + resync)

    State machine + curve-morph ramp generator driving the CARRIER voice per
    sample (Stage-2 PLAN Tasks 5, 8, 9). States:

        Bypassed -> SpinDown -> Stopped -> SpinUp -> Catchup -> ResyncXfade -> Bypassed

    Curve morph (DSP-02, exact at the contract point):

        spin-down: r(u) = (1 - u)^p
        spin-up:   r(u) = u^p          with p = 2^(2c), c = curve/100
        c = 0.5 -> p = 2 -> exactly x^2 (turntable physics default)

    Per-sample pow; ramp phase u in DOUBLE (float drifts ~0.1 % on an 8 s ramp
    at 192 kHz). Durations latched IN SAMPLES at the triggering edge.

    Resync (DSP-03, Signalsmith KVR t=538470): SpinUp completes lagging real
    time -> Catchup holds r = kCatchupRatio = 1.25 for at most kMaxCatchupMs
    (shaves debt, telegraphs the rejoin) -> ResyncXfade spawns the OTHER voice
    at the live head (d = 0, r = 1, INTEGER-offset read — bitwise the current
    input under per-sample write-then-read) and runs a 50 ms crossfade, then
    discards the old voice -> Bypassed. Post-crossfade output is bitwise dry
    BY CONSTRUCTION.

    Retrigger-everywhere (CONTEXT decision 2): the state machine drives "the
    carrier voice", not "voice A". Two voices suffice in every state:
      - SpinDown/SpinUp: mid-ramp reversal, same voice (inverse-curve seed
        u0 = clamp(r0,0,1)^(1/p_new), mirrored for spin-down).
      - Catchup: hand the gesture to the other voice AT THE SAME POSITION via
        a 50 ms crossfade; the new ramp starts from r matched to 1.0.
      - ResyncXfade: drive the new gesture ON THE RIDER (it sits at r = 1,
        exactly where every ramp starts); the old voice's fade-out completes
        as scheduled — the crossfade engine never restarts mid-fade.
      - Stopped: re-seed via the reversal rule (u0 from r0 = 0).
    The 2-slot pool is structural: a new fade force-completes a still-running
    one (drops the old fading voice, whose residual gain is small in every
    timing reachable outside pathological < 20 ms gesture chains); a third
    voice is never needed.

    Skip-splice gain laws (Task 8 / CONTEXT open question) — BOTH compiled,
    selected by a harness/dev flag for the Phase-2.2 A/B. NOTE both are
    amplitude-sum-of-1 (equal-GAIN) laws; neither is equal-power:
      - EqualPower: fadeOut = hann(0.5 + phi/2), fadeIn = hann(phi/2)
        Raised-cosine, = cos^2/sin^2(pi*phi/2). The enum name is a misnomer:
        sin^2 + cos^2 = 1 constrains the AMPLITUDE sum, so the law is exact
        (0 dB, flat) on CORRELATED material and DIPS on decorrelated material,
        where the power sum (1 + cos^2(pi*phi))/2 bottoms out at 0.5 — an
        analytic 3.01 dB floor for two equal-power decorrelated sources. True
        equal-power would be the SQUARE ROOTS of these gains, which inverts the
        trade — flat when decorrelated, +3 dB over-sum when correlated. Resync
        splices the live-head rider against a voice seconds behind it, so the
        decorrelated (dipping) case is the one in play.
      - Linear:     fadeOut = 1 - phi,           fadeIn = phi
        Also amplitude-sum-1, sharing the SAME 0.5 power floor exactly at
        phi = 0.5, but falling off faster either side of it (0.625 vs the
        raised cosine's 0.750 at phi = 0.25).

    Measured, not assumed — the real dip is deeper than the analytic floor,
    because the fading voice is a varispeed read of different material at a
    different level rather than an equal-power twin. Harness (v1.3.3):

        AB-splice-equal-power   bump = -0.48 dB,  dip = -6.21 dB
        AB-splice-linear        bump = -0.58 dB,  dip = -6.99 dB

    The near-zero bump on both is the direct confirmation that neither law
    over-sums correlated material — the +3 dB claim this comment used to carry
    was describing the sqrt law, which is not implemented. Whether to ADD that
    sqrt variant is improvements/2026-08-16-audit-queue.md item 4 (B2b); it
    changes the sound, so the enum name stays until that decision lands. Note
    the probe currently asserts only |bump| < 4.0, so the dip is reported but
    ungated — item 4 step 3 tightens that.

    Engaged-trim blend (`trimAmount`): OUTPUT_GAIN must never touch the
    Bypassed path (bitwise contract), so a non-default trim would STEP by
    (g-1)*in at the resync->Bypassed handoff. The transport ramps trimAmount
    1 -> 0 across exactly the ResyncXfade window (and back toward 1 whenever
    engaged); the processor applies gain as 1 + (g-1)*trimAmount, which lands
    on EXACTLY 1.0 as the fade ends. At the default trim the term is ~1e-7
    and invisible.

    Two things make that "EXACTLY" true rather than approximate (v1.3.2 — both
    were wrong before): the target is latched from the state ABOVE the
    crossfade-completion block, so the ResyncXfade -> Bypassed flip cannot
    retarget the ramp on the very sample that has to land it; and the ramp is an
    integer position over [0, xfLenSamples] rather than a float accumulator, so
    both rails are exact at every sample rate. See the two comments in tick().

    Stopped (FUNC-04): entered at r < kStopEps = 0.001 (or ramp exhaustion);
    10 ms linear wet-gain fade landing on EXACT 0.0f. The ring keeps recording
    while Stopped (CONTEXT decision 1); the debt clamp applies once at SpinUp
    entry: readAbsFrac = max(readAbsFrac, totalWritten - (ringSpan - guard)).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <cmath>
#include <cstring>

#include "CaptureBuffer.h"
#include "ContinuousMotion.h"
#include "VarispeedVoice.h"
#include "WindowLut.h"

class TapestopTransport
{
public:
    enum class State
    {
        Bypassed,
        SpinDown,
        Stopped,
        SpinUp,
        Catchup,
        ResyncXfade,
        ScratchPass,
        ContinuousMotionState   // v1.1: r driven per sample by `motion`
    };

    enum class SpliceLaw { EqualPower, Linear };

    // Must match ScratchEnvelope::kLutSize (static_assert in PluginProcessor.h).
    static constexpr int ScratchLutSize = 2048;

    static constexpr double kStopEps       = 0.001;
    static constexpr double kStoppedFadeMs = 10.0;
    static constexpr double kCatchupRatio  = 1.25;
    static constexpr double kMaxCatchupMs  = 250.0;
    static constexpr double kSkipXfadeMs   = 50.0;

    void prepare (double sampleRate) noexcept
    {
        fadeStep          = (float) (1.0 / juce::jmax (1.0, kStoppedFadeMs * 0.001 * sampleRate));
        xfLenSamples      = juce::jmax (2, (int) std::lround (kSkipXfadeMs  * 0.001 * sampleRate));
        catchupCapSamples = juce::jmax (1, (int) std::lround (kMaxCatchupMs * 0.001 * sampleRate));
        trimStepD         = 1.0 / (double) xfLenSamples;
        motion.prepare (sampleRate);
        reset();
    }

    void reset() noexcept
    {
        state    = State::Bypassed;
        u        = 0.0;
        p        = 1.0;
        uInc     = 0.0;
        wetFade  = 1.0f;
        trim     = 0.0f;   // Bypassed baseline: trim fully released
        trimPos  = 0;      // ...and the integer ramp position that produced it
        carrier  = 0;
        xfActive = false;
        xfPos    = 0;
        fadingIdx = 1;
        rFading  = 1.0;
        gFading  = 1.0f;
        catchupElapsed = 0;
        scratchLut = nullptr;
        scratchPos = 0;
        scratchLenSamples = 1;
        lastScratchR = 1.0;
        motion.reset();
        lastContinuousR = 1.0;
    }

    State getState() const noexcept        { return state; }
    bool  isBypassed() const noexcept      { return state == State::Bypassed; }
    int   getCarrierIndex() const noexcept { return carrier; }

    /** UI readback (Stage 3): scratch-pass phase φ ∈ [0,1]; 0 outside
        ScratchPass. Read at the END of processBlock only (audio thread) —
        a pure observer, touches no state. */
    double getScratchPhase() const noexcept
    {
        return (state == State::ScratchPass && scratchLenSamples > 0)
             ? juce::jlimit (0.0, 1.0,
                             (double) scratchPos / (double) scratchLenSamples)
             : 0.0;
    }

    void setSpliceLaw (SpliceLaw law) noexcept { spliceLaw = law; }

    /** ENGAGE=true edge (block header). Honored in EVERY state (CONTEXT
        decision 2). Duration and curve are latched HERE. */
    void engage (double durationSamples, double curveP,
                 VarispeedVoice* v, const CaptureBuffer& ring) noexcept
    {
        switch (state)
        {
            case State::Bypassed:
                v[carrier].readAbsFrac = (double) (ring.getTotalWritten() - 1);
                u = 0.0;   // first tick: r = 1 exactly — continuous with dry
                break;

            case State::SpinUp:
            {
                // Mid-ramp reversal: seed the spin-down at the phase whose
                // ratio equals the current one under the NEW curve.
                const double r0 = juce::jlimit (0.0, 1.0, std::pow (juce::jmin (1.0, u), p));
                u = 1.0 - std::pow (r0, 1.0 / curveP);
                break;
            }

            case State::Catchup:
            {
                // Catchup retrigger: hand the gesture to the OTHER voice at
                // the SAME position via a 50 ms crossfade; the new ramp
                // starts from r matched to 1.0 (u0 = 0). A at 1.25x and B at
                // <=1x from the same origin skew <=12 ms across the fade.
                startXfade (carrier, kCatchupRatio, wetFade, xfLenSamples);

                const int newIdx = 1 - carrier;
                v[newIdx].readAbsFrac = v[carrier].readAbsFrac;
                carrier = newIdx;
                u = 0.0;
                break;
            }

            case State::ResyncXfade:
                // Drive the new gesture ON THE RIDER — it is at r = 1,
                // exactly where every spin-down starts (u0 = 0). The old
                // voice's fade-out completes as scheduled; the crossfade
                // engine never restarts mid-fade.
                u = 0.0;
                break;

            case State::SpinDown:
            case State::Stopped:
            case State::ScratchPass:
            case State::ContinuousMotionState:
            default:
                return;   // already engaged — a true edge cannot arrive here
        }

        p     = curveP;
        uInc  = 1.0 / juce::jmax (1.0, durationSamples);
        state = State::SpinDown;
    }

    /** ENGAGE=true edge with MODE = Scratch (latched at the edge). Starts one
        pass of the drawn envelope on the CARRIER voice IMMEDIATELY in every
        state — an r slope change, never a position jump (FUNC-02). A running
        crossfade continues untouched.

        The caller load(acquire)s the envelope's published LUT at this edge; we
        COPY it here (8 KB memcpy, no allocation, no lock — RT-safe) and read
        only the copy for the rest of the pass.

        The copy is load-bearing, not defensive (v1.3.1). ScratchEnvelope bakes
        into "whichever of its two buffers is not published", which survives
        exactly ONE outstanding reader generation — but a pass runs up to 8 s
        (ENV_FREE_MS max) while the editor commits 50 ms after every pointer-up.
        The SECOND commit during a live pass therefore baked straight into the
        buffer this transport was reading: torn envelope mid-pass (an r step)
        plus a formal data race. Owning the bytes makes the latch contract
        structural — edits mid-pass affect the next pass only, provably. */
    void engageScratch (double envLengthSamples, const float* lut2048,
                        VarispeedVoice* v, const CaptureBuffer& ring) noexcept
    {
        jassert (lut2048 != nullptr);

        if (state == State::Bypassed)
        {
            v[carrier].readAbsFrac = (double) (ring.getTotalWritten() - 1);
        }
        // Every other state: the pass drives the carrier from its CURRENT
        // position (mid-resync/mid-catchup/mid-ramp/Stopped re-engage).

        std::memcpy (scratchLutCopy.data(), lut2048, sizeof (float) * (size_t) ScratchLutSize);

        scratchLut        = scratchLutCopy.data();
        scratchLenSamples = juce::jmax (1, (int) std::lround (envLengthSamples));
        scratchPos        = 0;
        lastScratchR      = (double) scratchLutCopy[0];
        state             = State::ScratchPass;
    }

    /** ENGAGE=true edge with MODE = Continuous (v1.1, latched at the edge).
        Follows the engageScratch takeover contract: Bypassed spawns the
        carrier at the live head; every other state drives the carrier from
        its CURRENT position (an r change, never a position jump). A running
        crossfade continues untouched. The generator re-seeds its RNG streams
        here — every engage replays the same stochastic sequence. */
    void engageContinuous (const ContinuousMotion::Params& mp,
                           VarispeedVoice* v, const CaptureBuffer& ring) noexcept
    {
        if (state == State::Bypassed)
        {
            v[carrier].readAbsFrac = (double) (ring.getTotalWritten() - 1);
        }

        auto mpLatched = mp;
        mpLatched.debtNowSamples = (double) (ring.getTotalWritten() - 1) - v[carrier].readAbsFrac;
        motion.engage (mpLatched);

        lastContinuousR = 1.0;
        state           = State::ContinuousMotionState;
    }

    /** Live DEPTH/RATE updates while in Continuous mode (audio thread,
        absolute 16-sample grid — the toneTrack cadence). Inert in every
        other state. */
    void setContinuousTargets (double mPeak, double depth01, double periodSamples) noexcept
    {
        if (state == State::ContinuousMotionState)
            motion.setTargets (mPeak, depth01, periodSamples);
    }

    /** ENGAGE=false edge (block header). Starts the spin-up from the current
        ratio (SpinDown reversal) or from 0 (Stopped). Applies the Stopped-
        hold debt clamp at SpinUp entry (CONTEXT decision 1). */
    void release (double durationSamples, double curveP,
                  VarispeedVoice* v, const CaptureBuffer& ring) noexcept
    {
        // Scratch abort (FUNC-02): disengage mid-pass goes STRAIGHT to
        // ResyncXfade — forcing the pass complete makes the next tick take
        // the φ = 1 → resync path in its correct in-tick context.
        if (state == State::ScratchPass)
        {
            scratchPos = scratchLenSamples;
            return;
        }

        double r0 = 0.0;

        switch (state)
        {
            case State::SpinDown:
                r0 = juce::jlimit (0.0, 1.0, std::pow (juce::jmax (0.0, 1.0 - u), p));
                break;

            case State::Stopped:
                r0 = 0.0;
                break;

            case State::ContinuousMotionState:
                // Continuous release rides the FULL resync path (SpinUp →
                // Catchup → ResyncXfade) so START time/curve shape the return
                // and the debt clamp below applies. Mid-reverse (r < 0) seeds
                // from 0 — a spin-up from stopped, speed-continuous enough
                // behind the 2 ms glitch slew.
                r0 = juce::jlimit (0.0, 1.0, lastContinuousR);
                break;

            // ScratchPass is deliberately absent: the early return above owns
            // it, so a case label here would be unreachable.
            case State::Bypassed:
            case State::SpinUp:
            case State::Catchup:
            case State::ResyncXfade:
            default:
                return;   // already disengaged — a false edge cannot arrive here
        }

        u     = std::pow (r0, 1.0 / curveP);   // spin-up inverse: u0 = r0^(1/p_new)
        p     = curveP;
        uInc  = 1.0 / juce::jmax (1.0, durationSamples);

        // Debt clamp at SpinUp entry: a hold-forever session resumes on the
        // OLDEST VALID material — specified behavior, not an error path. The
        // crossfade-skip resync absorbs any size of content jump.
        const double maxDebt = (double) ring.getBufferSize()
                             - (double) VarispeedVoice::kInterpGuard;
        v[carrier].readAbsFrac = juce::jmax (v[carrier].readAbsFrac,
                                             (double) ring.getTotalWritten() - maxDebt);

        state = State::SpinUp;
    }

    struct Tick
    {
        int    carrierIdx;      // may have flipped THIS sample (resync entry)
        double carrierR;        // the ratio applied to the carrier this sample
        float  carrierWetGain;  // stopped-silence fade (exact 0.0f while parked)
        float  trimAmount;      // 0..1 — processor applies 1 + (g-1)*trimAmount
        bool   xfActive;
        int    fadingIdx;
        float  fadeOutGain;     // includes the fading voice's latched wet gain
        float  fadeInGain;
    };

    /** Advances one sample: state machine -> carrier ratio -> voice position
        advances (carrier + fading) -> crossfade bookkeeping -> fades.
        Callers must re-check isBypassed() after tick() (resync completion). */
    Tick tick (VarispeedVoice* v, const CaptureBuffer& ring) noexcept
    {
        const juce::int64 live = ring.getTotalWritten() - 1;
        double r = 0.0;

        switch (state)
        {
            case State::Bypassed:
                break;   // not reached — processBlock guards

            case State::SpinDown:
                r = std::pow (juce::jmax (0.0, 1.0 - u), p);
                u += uInc;
                if (r < kStopEps || u >= 1.0)
                {
                    state = State::Stopped;
                    r = 0.0;
                }
                break;

            case State::Stopped:
                r = 0.0;
                break;

            case State::SpinUp:
                r = std::pow (juce::jmin (1.0, u), p);
                u += uInc;
                if (u >= 1.0)
                {
                    r = 1.0;
                    state = State::Catchup;
                    catchupElapsed = 0;
                }
                break;

            case State::Catchup:
            {
                r = kCatchupRatio;
                ++catchupElapsed;

                const double debt = (double) live - v[carrier].readAbsFrac;

                if (debt <= kCatchupRatio || catchupElapsed > catchupCapSamples)
                {
                    enterResync (v, live, kCatchupRatio);
                    r = 1.0;   // the rider (new carrier) advances at 1
                }
                break;
            }

            case State::ResyncXfade:
                r = 1.0;   // rider: integer live read — bitwise dry
                break;

            case State::ScratchPass:
            {
                if (scratchPos >= scratchLenSamples)
                {
                    // φ = 1 (natural completion) or forced by a mid-pass
                    // disengage — straight to ResyncXfade (no Catchup); the
                    // fading scratch voice keeps its last speed (sign and
                    // all) through the fade.
                    enterResync (v, live, lastScratchR);
                    r = 1.0;
                    break;
                }

                // r = lut[φ·2047] with linear interp; r ∈ [−2, +2]. A sign
                // flip is a palindrome corner (position stays continuous) —
                // NOT a Stopped entry; the stop-fade never fires here.
                const double phi  = (double) scratchPos / (double) scratchLenSamples;
                const double fpos = phi * (double) (ScratchLutSize - 1);
                const int    i0   = (int) fpos;
                const int    i1   = juce::jmin (i0 + 1, ScratchLutSize - 1);
                const double fr   = fpos - (double) i0;

                r = (double) scratchLut[i0] + fr * ((double) scratchLut[i1] - (double) scratchLut[i0]);
                lastScratchR = r;
                ++scratchPos;
                break;
            }

            case State::ContinuousMotionState:
            {
                // The generator owns r; the transport owns POSITION. A
                // requested jump (stutter loop-back / resync-snap) is executed
                // here through the same 2-voice splice machinery as every
                // other position discontinuity — r may step, P may not.
                const double debt = (double) live - v[carrier].readAbsFrac;
                const auto   mo   = motion.tick (debt);

                if (mo.jumpKind != 0)
                {
                    const double target = (mo.jumpKind == 1)
                        ? v[carrier].readAbsFrac - mo.jumpAmount   // loop back one slice
                        : (double) live - mo.jumpAmount;           // snap toward the live head

                    spliceCarrierTo (v, ring, target, lastContinuousR,
                                     juce::jmax (2, mo.xfLen));
                }

                r = mo.r;
                lastContinuousR = r;
                break;
            }
        }

        // Engaged-trim target, LATCHED HERE — after the state machine above has
        // resolved this sample's state, and before the crossfade-completion
        // block below can flip ResyncXfade -> Bypassed. Both edges matter and
        // this is the only point between them:
        //   - Reading `state` BELOW the completion block sets the target back to
        //     1 on the FINAL fade sample, ticking trim UP on the one sample that
        //     has to land it on 0 (it ended at 2/xfLen, so neither the header's
        //     "EXACTLY 1.0 as the fade ends" nor reset()'s documented Bypassed
        //     baseline of 0 actually held).
        //   - Latching on ENTRY to tick() is not enough either: the fade's FIRST
        //     sample is the one where the switch above ENTERS ResyncXfade via
        //     enterResync(), so an entry latch loses that decrement and lands
        //     trim on 1/xfLen instead.
        // The resync fade is exactly xfLenSamples ticks (enterResync passes
        // xfLenSamples to startXfade) and the trim ramp spans exactly that many
        // positions, so targeting 0 across ALL of them releases a saturated
        // trim to EXACTLY 0.
        const float trimTarget = (state == State::ResyncXfade) ? 0.0f : 1.0f;

        // Advance the carrier: never past the write head, and never further
        // behind it than the ring can serve (release-build debt clamp at the
        // SOURCE — keeps the stored position, and therefore the debt
        // accessor, provably in bounds under full-reverse scratch and
        // > kCaptureSeconds Stopped holds alike). At the lower rail the
        // playhead rides the oldest valid material — specified behavior.
        v[carrier].readAbsFrac = VarispeedVoice::clampToRing (v[carrier].readAbsFrac + r, ring);

        Tick out;
        out.carrierIdx  = carrier;
        out.carrierR    = r;
        out.xfActive    = xfActive;
        out.fadingIdx   = fadingIdx;
        out.fadeOutGain = 0.0f;
        out.fadeInGain  = 1.0f;

        if (xfActive)
        {
            const float phi = (float) xfPos / (float) (xfLenCur - 1);

            if (spliceLaw == SpliceLaw::EqualPower)
            {
                out.fadeOutGain = hann.readAt (0.5f + 0.5f * phi) * gFading;
                out.fadeInGain  = hann.readAt (0.5f * phi);
            }
            else
            {
                out.fadeOutGain = (1.0f - phi) * gFading;
                out.fadeInGain  = phi;
            }

            // The fading voice keeps doing what it was doing (latched rate,
            // sign included — an aborted reverse scratch keeps reversing
            // through its fade-out). Same double-ended clamp as the carrier.
            v[fadingIdx].readAbsFrac = VarispeedVoice::clampToRing (v[fadingIdx].readAbsFrac + rFading, ring);

            if (++xfPos >= xfLenCur)
            {
                xfActive = false;

                if (state == State::ResyncXfade)
                    state = State::Bypassed;   // resync complete → bitwise dry
            }
        }

        // 10 ms linear wet fade: target 0 while Stopped, 1 otherwise; lands
        // EXACTLY on the endpoints (no decaying tails, no denormals).
        const float fadeTarget = (state == State::Stopped) ? 0.0f : 1.0f;

        if (wetFade < fadeTarget)
            wetFade = juce::jmin (fadeTarget, wetFade + fadeStep);
        else if (wetFade > fadeTarget)
            wetFade = juce::jmax (fadeTarget, wetFade - fadeStep);

        // Engaged-trim blend: releases to EXACTLY 0 across the resync fade
        // (so the applied gain is exactly 1.0 at the Bypassed handoff), and
        // re-engages toward 1 in every other engaged state. `trimTarget` was
        // latched ABOVE the crossfade-completion block on purpose — see there.
        //
        // The ramp is an INTEGER POSITION in [0, xfLenSamples], not a float
        // accumulator: `trim ± trimStep` clamped at the rails only lands on the
        // rail if the accumulated rounding happens to overshoot it, which is a
        // property of the sample rate, not of the algorithm. Measured across
        // the supported rates, xfLenSamples subtractions of (float)(1/xfLen)
        // from 1.0f leave: 44.1k → 0, 48k → 0, 88.2k → 4.07e-5, 96k → 1.88e-5,
        // 176.4k → 6.73e-5, 192k → 0. Deriving trim from the counter makes both
        // rails exact at every rate by construction, and makes the intermediate
        // values the ideal linear ramp rather than a drifting one.
        trimPos = juce::jlimit (0, xfLenSamples,
                                trimPos + (trimTarget > 0.0f ? 1 : -1));

        trim = (trimPos == xfLenSamples) ? 1.0f
             : (trimPos == 0)            ? 0.0f
             : (float) ((double) trimPos * trimStepD);

        out.carrierWetGain = wetFade;
        out.trimAmount     = trim;
        return out;
    }

private:
    /** Spawn the rider at the live head and start the skip crossfade. The old
        carrier fades out continuing at `rLatchForFading` (catchup rate from
        Stop mode; last scratch speed — sign included — from a scratch pass). */
    void enterResync (VarispeedVoice* v, juce::int64 live, double rLatchForFading) noexcept
    {
        startXfade (carrier, rLatchForFading, wetFade, xfLenSamples);

        const int rider = 1 - carrier;
        v[rider].readAbsFrac = (double) (live - 1);   // +1.0 this tick → live
        carrier = rider;
        state   = State::ResyncXfade;
    }

    /** Continuous-mode position jump (stutter loop-back / resync-snap): the
        same voice-swap-through-a-splice as enterResync, but the STATE does not
        change — the fade completing while ContinuousMotionState never touches
        Bypassed (the ResyncXfade guard in tick() sees to that). Target is
        clamped to the ring's serviceable span before it lands in a voice. */
    void spliceCarrierTo (VarispeedVoice* v, const CaptureBuffer& ring,
                          double targetAbs, double rLatchForFading, int xfLen) noexcept
    {
        targetAbs = VarispeedVoice::clampToRing (targetAbs, ring);

        startXfade (carrier, rLatchForFading, wetFade, xfLen);

        const int rider = 1 - carrier;
        v[rider].readAbsFrac = targetAbs;
        carrier = rider;
    }

    void startXfade (int idx, double rLatch, float gLatch, int xfLen) noexcept
    {
        // Force-complete: when a fade is already running, retargeting fadingIdx
        // drops the outgoing voice outright. The pool is 2 voices by contract
        // (no third voice, no steal policy beyond this). Reachable only when a
        // full release→spin-up→catchup cycle fits inside one 50 ms fade
        // (sub-20 ms gesture chains); the dropped voice's residual gain is
        // fadeOut(phi), small by then in every musical timing.
        fadingIdx = idx;
        rFading   = rLatch;
        gFading   = gLatch;
        xfPos     = 0;
        xfLenCur  = juce::jmax (2, xfLen);
        xfActive  = true;
    }

    State  state    = State::Bypassed;
    double u        = 0.0;    // ramp phase, DOUBLE (accumulation exactness)
    double p        = 1.0;    // curve exponent 2^(2c), latched at the edge
    double uInc     = 0.0;    // 1 / durationSamples, latched at the edge
    float  wetFade  = 1.0f;
    float  fadeStep = 1.0f;

    float  trim      = 0.0f;  // engaged-trim blend (see header comment)
    int    trimPos   = 0;     // integer ramp position in [0, xfLenSamples]
    double trimStepD = 0.5;   // 1 / xfLenSamples (prepare; matches the len-2 default)

    int    carrier  = 0;      // which voice the state machine drives

    bool   xfActive = false;  // skip-splice crossfade (orthogonal to state)
    int    fadingIdx = 1;
    int    xfPos    = 0;
    int    xfLenSamples = 2;  // default 50 ms length (prepare)
    int    xfLenCur = 2;      // THIS fade's length (stutter splices are shorter)
    double rFading  = 1.0;    // fading voice's latched rate
    float  gFading  = 1.0f;   // fading voice's latched wet gain

    int    catchupElapsed    = 0;
    int    catchupCapSamples = 1;

    // Scratch pass (latched at the engage edge — LUT bytes, length). The
    // transport OWNS the table it reads: engageScratch memcpys the published
    // envelope in, so message-thread re-bakes cannot reach an in-flight pass
    // (v1.3.1). scratchLut points into scratchLutCopy whenever a pass is live
    // and is null otherwise — the state gate is what keeps the read path safe,
    // the pointer is just the read cursor's base.
    std::array<float, (size_t) ScratchLutSize> scratchLutCopy {};
    const float* scratchLut      = nullptr;
    int          scratchPos      = 0;
    int          scratchLenSamples = 1;
    double       lastScratchR    = 1.0;

    // Continuous mode (v1.1): per-sample rate generator + the last r it
    // produced (release seed; latched fading rate for its own splices).
    ContinuousMotion motion;
    double           lastContinuousR = 1.0;

    SpliceLaw spliceLaw = SpliceLaw::EqualPower;
    WindowLut hann;
};
