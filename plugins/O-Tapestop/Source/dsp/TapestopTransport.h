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

    O-Tapestop - TapestopTransport (Phase 2.1 subset: Stop mode)

    State machine + curve-morph ramp generator driving r(t) per sample
    (Stage-2 PLAN Task 5). Phase 2.1 states:

        Bypassed -> SpinDown -> Stopped -> SpinUp -> Bypassed

    Phase 2.2 adds Catchup and ResyncXfade between SpinUp and Bypassed
    (fall-behind -> 1.25x catchup -> 50 ms crossfade-skip). Until then the
    spin-up ramps simply back onto the LAGGING playhead and the handoff to
    Bypassed at u = 1 is a known, documented content splice — the resync
    controller replaces it.

    Curve morph (DSP-02, exact at the contract point):

        spin-down: r(u) = (1 - u)^p
        spin-up:   r(u) = u^p          with p = 2^(2c), c = curve/100
        c = 0.5 -> p = 2 -> exactly x^2 (turntable physics default)

    Per-sample pow in this phase (P3 exactness; optimise only on measured
    need). Ramp phase u advances in DOUBLE (`u += 1/durationSamples`) — float
    accumulation drifts ~0.1 % on an 8 s ramp at 192 kHz.

    Mid-ramp reversal (FUNC-01): an edge mid-ramp seeds the NEW ramp at the
    phase whose ratio equals the current ratio under the NEW curve
    (u0 = clamp(r0,0,1)^(1/p_new) for spin-up, mirrored for spin-down) —
    speed-continuous, position C1, no click, same voice.

    Stopped (FUNC-04): entered at r < kStopEps = 0.001 (or ramp exhaustion);
    a 10 ms linear wet-gain fade lands on EXACT 0.0f (a frozen playhead
    otherwise holds a DC sample). The ring keeps recording while Stopped
    (CONTEXT decision 1); the debt clamp is applied once at SpinUp entry:
    readAbsFrac = max(readAbsFrac, totalWritten - (ringSpan - kInterpGuard)).

    All durations are latched IN SAMPLES at the triggering edge (latch-at-
    spawn contract — a mid-ramp tempo/param change does not retarget a live
    ramp).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <cmath>

#include "CaptureBuffer.h"
#include "VarispeedVoice.h"
#include "WindowLut.h"   // Phase 2.2 crossfade gains (equal-power Hann halves);
                         // compiled from day one so the table cannot rot unseen.

class TapestopTransport
{
public:
    enum class State
    {
        Bypassed,
        SpinDown,
        Stopped,
        SpinUp
        // Phase 2.2: Catchup, ResyncXfade
    };

    static constexpr double kStopEps       = 0.001;
    static constexpr double kStoppedFadeMs = 10.0;

    void prepare (double sampleRate) noexcept
    {
        fadeStep = (float) (1.0 / juce::jmax (1.0, kStoppedFadeMs * 0.001 * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        state   = State::Bypassed;
        u       = 0.0;
        p       = 1.0;
        uInc    = 0.0;
        wetFade = 1.0f;
    }

    State getState() const noexcept  { return state; }
    bool  isBypassed() const noexcept { return state == State::Bypassed; }

    /** ENGAGE=true edge (block header). Seeds the voice at the live head from
        Bypassed, or reverses a live spin-up speed-continuously (FUNC-01).
        Duration and curve are latched HERE — the edge is the latch point. */
    void engage (double durationSamples, double curveP,
                 VarispeedVoice& voice, const CaptureBuffer& ring) noexcept
    {
        switch (state)
        {
            case State::Bypassed:
                voice.active      = true;
                voice.readAbsFrac = (double) (ring.getTotalWritten() - 1);
                u = 0.0;   // first tick: r = (1-0)^p = 1 exactly — continuous with dry
                break;

            case State::SpinUp:
            {
                // Mid-ramp reversal: seed the spin-down at the phase whose
                // ratio equals the current one under the NEW curve.
                const double r0 = juce::jlimit (0.0, 1.0, std::pow (juce::jmin (1.0, u), p));
                u = 1.0 - std::pow (r0, 1.0 / curveP);
                break;
            }

            case State::SpinDown:
            case State::Stopped:
            default:
                return;   // already engaged — a true edge cannot arrive here
        }

        p    = curveP;
        uInc = 1.0 / juce::jmax (1.0, durationSamples);
        state = State::SpinDown;
    }

    /** ENGAGE=false edge (block header). Starts the spin-up from the current
        ratio (SpinDown reversal) or from 0 (Stopped). Applies the Stopped-
        hold debt clamp at SpinUp entry (CONTEXT decision 1). */
    void release (double durationSamples, double curveP,
                  VarispeedVoice& voice, const CaptureBuffer& ring) noexcept
    {
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
            default:
                return;   // already disengaged — a false edge cannot arrive here
        }

        u    = std::pow (r0, 1.0 / curveP);   // spin-up inverse: u0 = r0^(1/p_new)
        p    = curveP;
        uInc = 1.0 / juce::jmax (1.0, durationSamples);

        // Debt clamp at SpinUp entry: a hold-forever session resumes on the
        // OLDEST VALID material — specified behavior, not an error path. The
        // Phase-2.2 crossfade-skip resync absorbs any size of content jump.
        const double maxDebt = (double) ring.getBufferSize()
                             - (double) VarispeedVoice::kInterpGuard;
        voice.readAbsFrac = juce::jmax (voice.readAbsFrac,
                                        (double) ring.getTotalWritten() - maxDebt);

        state = State::SpinUp;
    }

    struct Tick
    {
        double r;        // instantaneous ratio for THIS sample
        float  wetGain;  // stopped-silence fade (exact 0.0f while parked)
    };

    /** Advances one sample: computes r at the current phase, then steps u.
        May transition SpinDown->Stopped (r < kStopEps or ramp exhaustion) and
        SpinUp->Bypassed (u >= 1 — the Phase-2.1 splice; resync replaces it).
        Callers must re-check isBypassed() after tick(). */
    Tick tick() noexcept
    {
        double r = 0.0;

        switch (state)
        {
            case State::Bypassed:
                return { 1.0, 1.0f };   // not reached — processBlock guards

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
                    state = State::Bypassed;   // 2.1 splice; 2.2: -> Catchup
                }
                break;
        }

        // 10 ms linear wet fade: target 0 while Stopped, 1 otherwise; lands
        // EXACTLY on the endpoints (no decaying tails, no denormals).
        const float target = (state == State::Stopped) ? 0.0f : 1.0f;

        if (wetFade < target)
            wetFade = juce::jmin (target, wetFade + fadeStep);
        else if (wetFade > target)
            wetFade = juce::jmax (target, wetFade - fadeStep);

        return { r, wetFade };
    }

private:
    State  state    = State::Bypassed;
    double u        = 0.0;   // ramp phase, DOUBLE (accumulation exactness)
    double p        = 1.0;   // curve exponent 2^(2c), latched at the edge
    double uInc     = 0.0;   // 1 / durationSamples, latched at the edge
    float  wetFade  = 1.0f;
    float  fadeStep = 1.0f;
};
