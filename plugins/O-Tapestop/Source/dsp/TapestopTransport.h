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
    selected by a harness/dev flag for the Phase-2.2 A/B:
      - EqualPower: fadeOut = hann(0.5 + phi/2), fadeIn = hann(phi/2)
        (sin^2 + cos^2 = 1 — but over-sums CORRELATED material by up to +3 dB)
      - Linear:     fadeOut = 1 - phi,           fadeIn = phi

    Engaged-trim blend (`trimAmount`): OUTPUT_GAIN must never touch the
    Bypassed path (bitwise contract), so a non-default trim would STEP by
    (g-1)*in at the resync->Bypassed handoff. The transport ramps trimAmount
    1 -> 0 across exactly the ResyncXfade window (and back toward 1 whenever
    engaged); the processor applies gain as 1 + (g-1)*trimAmount, which lands
    on EXACTLY 1.0 as the fade ends. At the default trim the term is ~1e-7
    and invisible.

    Stopped (FUNC-04): entered at r < kStopEps = 0.001 (or ramp exhaustion);
    10 ms linear wet-gain fade landing on EXACT 0.0f. The ring keeps recording
    while Stopped (CONTEXT decision 1); the debt clamp applies once at SpinUp
    entry: readAbsFrac = max(readAbsFrac, totalWritten - (ringSpan - guard)).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <cmath>

#include "CaptureBuffer.h"
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
        ScratchPass
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
        trimStep          = (float) (1.0 / (double) xfLenSamples);
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
                v[carrier].active      = true;
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
                startXfade (carrier, kCatchupRatio, wetFade, v);

                const int newIdx = 1 - carrier;
                v[newIdx].active      = true;
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
            default:
                return;   // already engaged — a true edge cannot arrive here
        }

        p     = curveP;
        uInc  = 1.0 / juce::jmax (1.0, durationSamples);
        state = State::SpinDown;
    }

    /** ENGAGE=true edge with MODE = Scratch (latched at the edge). Starts one
        pass of the drawn envelope on the CARRIER voice IMMEDIATELY in every
        state — an r slope change, never a position jump (FUNC-02). The LUT
        pointer was load(acquire)d by the caller at this edge; edits mid-pass
        affect the next pass only. A running crossfade continues untouched. */
    void engageScratch (double envLengthSamples, const float* lut2048,
                        VarispeedVoice* v, const CaptureBuffer& ring) noexcept
    {
        jassert (lut2048 != nullptr);

        if (state == State::Bypassed)
        {
            v[carrier].active      = true;
            v[carrier].readAbsFrac = (double) (ring.getTotalWritten() - 1);
        }
        // Every other state: the pass drives the carrier from its CURRENT
        // position (mid-resync/mid-catchup/mid-ramp/Stopped re-engage).

        scratchLut        = lut2048;
        scratchLenSamples = juce::jmax (1, (int) std::lround (envLengthSamples));
        scratchPos        = 0;
        lastScratchR      = (double) lut2048[0];
        state             = State::ScratchPass;
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

            case State::Bypassed:
            case State::SpinUp:
            case State::Catchup:
            case State::ResyncXfade:
            case State::ScratchPass:
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
        }

        // Advance the carrier: never past the write head, and never further
        // behind it than the ring can serve (release-build debt clamp at the
        // SOURCE — keeps the stored position, and therefore the debt
        // accessor, provably in bounds under full-reverse scratch and
        // > kCaptureSeconds Stopped holds alike). At the lower rail the
        // playhead rides the oldest valid material — specified behavior.
        {
            const double maxDebt = (double) ring.getBufferSize()
                                 - (double) VarispeedVoice::kInterpGuard;

            double pos = v[carrier].readAbsFrac + r;
            pos = juce::jmin (pos, (double) live);
            pos = juce::jmax (pos, (double) live - maxDebt);
            v[carrier].readAbsFrac = pos;
        }

        Tick out;
        out.carrierIdx  = carrier;
        out.carrierR    = r;
        out.xfActive    = xfActive;
        out.fadingIdx   = fadingIdx;
        out.fadeOutGain = 0.0f;
        out.fadeInGain  = 1.0f;

        if (xfActive)
        {
            const float phi = (float) xfPos / (float) (xfLenSamples - 1);

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
            {
                const double maxDebt = (double) ring.getBufferSize()
                                     - (double) VarispeedVoice::kInterpGuard;

                double pos = v[fadingIdx].readAbsFrac + rFading;
                pos = juce::jmin (pos, (double) live);
                pos = juce::jmax (pos, (double) live - maxDebt);
                v[fadingIdx].readAbsFrac = pos;
            }

            if (++xfPos >= xfLenSamples)
            {
                v[fadingIdx].active = false;
                xfActive = false;

                if (state == State::ResyncXfade)
                {
                    state = State::Bypassed;   // resync complete → bitwise dry
                    v[carrier].active = false;
                }
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
        // re-engages toward 1 in every other engaged state.
        const float trimTarget = (state == State::ResyncXfade) ? 0.0f : 1.0f;

        if (trim < trimTarget)
            trim = juce::jmin (trimTarget, trim + trimStep);
        else if (trim > trimTarget)
            trim = juce::jmax (trimTarget, trim - trimStep);

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
        startXfade (carrier, rLatchForFading, wetFade, v);

        const int rider = 1 - carrier;
        v[rider].active      = true;
        v[rider].readAbsFrac = (double) (live - 1);   // +1.0 this tick → live
        carrier = rider;
        state   = State::ResyncXfade;
    }

    void startXfade (int idx, double rLatch, float gLatch, VarispeedVoice* v) noexcept
    {
        if (xfActive)
        {
            // Force-complete: the pool is 2 voices by contract (no third
            // voice, no steal policy beyond this). Reachable only when a full
            // release→spin-up→catchup cycle fits inside one 50 ms fade
            // (sub-20 ms gesture chains); the dropped voice's residual gain
            // is fadeOut(phi), small by then in every musical timing.
            v[fadingIdx].active = false;
        }

        fadingIdx = idx;
        rFading   = rLatch;
        gFading   = gLatch;
        xfPos     = 0;
        xfActive  = true;
    }

    State  state    = State::Bypassed;
    double u        = 0.0;    // ramp phase, DOUBLE (accumulation exactness)
    double p        = 1.0;    // curve exponent 2^(2c), latched at the edge
    double uInc     = 0.0;    // 1 / durationSamples, latched at the edge
    float  wetFade  = 1.0f;
    float  fadeStep = 1.0f;

    float  trim     = 0.0f;   // engaged-trim blend (see header comment)
    float  trimStep = 1.0f;

    int    carrier  = 0;      // which voice the state machine drives

    bool   xfActive = false;  // skip-splice crossfade (orthogonal to state)
    int    fadingIdx = 1;
    int    xfPos    = 0;
    int    xfLenSamples = 2;
    double rFading  = 1.0;    // fading voice's latched rate
    float  gFading  = 1.0f;   // fading voice's latched wet gain

    int    catchupElapsed    = 0;
    int    catchupCapSamples = 1;

    // Scratch pass (latched at the engage edge — LUT pointer, length).
    const float* scratchLut      = nullptr;
    int          scratchPos      = 0;
    int          scratchLenSamples = 1;
    double       lastScratchR    = 1.0;

    SpliceLaw spliceLaw = SpliceLaw::EqualPower;
    WindowLut hann;
};
