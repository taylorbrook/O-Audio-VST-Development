/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - TapeTransport (Stage 2, Phase 2.1)

    Read-rate state machine for the tape family (DSP-01):

      * Bends pick a target from the musical-interval table
        {1.0, 0.67, 1.5, 0.5, 2.0} and ramp the read rate to it linearly over
        TAPE_RAMP ms — the ramp IS the glide sound and the click safety. Rate
        changes never crossfade; only position jumps do (ReadHead).
      * Stop = ramp rate to 0.0 exactly, hold (the held sample decays to a
        constant — no denormal generation), ramp back to 1.0 on release.
      * The position/phase accumulator lives in ReadHead and is NEVER reset
        by rate changes (anti-zipper rule 3).
      * Up-bends consume read-head lag (there is no future tape). When the
        remaining lag can no longer absorb the release ramp, the bend
        early-releases through the SAME linear ramp — a continuous
        frequency ramp, never a pitch step (DSP-01 acceptance).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class TapeTransport
{
public:
    // Interval table (ARCHITECTURE / deep-dive section 4.2). Index 0 is the
    // NORMAL rate; bends draw from indices 1..4.
    static constexpr double kIntervals[5] = { 1.0, 0.67, 1.5, 0.5, 2.0 };
    static constexpr int    kNumBendIntervals = 4;

    void prepare (double sampleRate) noexcept
    {
        fs = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        state         = State::Idle;
        applied       = 1.0;
        target        = 1.0;
        step          = 0.0;
        rampRemaining = 0;
        rampMs        = 150.0;
    }

    bool isIdle() const noexcept { return state == State::Idle; }

    // fromRate: the rate that was actually applied last sample (may include
    // the ReadHead re-approach trim) — ramping from it avoids a rate step at
    // event onset.
    void installBend (double interval, double newRampMs, double fromRate) noexcept
    {
        state = State::Bend;
        beginRamp (interval, newRampMs, fromRate);
    }

    void installStop (double newRampMs, double fromRate) noexcept
    {
        state = State::Stop;
        beginRamp (0.0, newRampMs, fromRate);
    }

    // Graceful release toward NORMAL (no-firer tick, or mid-event disable).
    // No-op while Idle; an in-flight release is not restarted (restarting
    // every block would re-derive the step from a shrinking distance and
    // never complete).
    void release (double newRampMs) noexcept
    {
        if (state == State::Idle || state == State::Releasing)
            return;

        state = State::Releasing;
        beginRamp (1.0, newRampMs, applied);
    }

    // Per-sample rate. lagSamples = current read-head lag, used for the
    // up-bend early release.
    double nextRate (double lagSamples) noexcept
    {
        if (state == State::Idle)
            return 1.0;

        if (rampRemaining > 0)
        {
            applied += step;
            if (--rampRemaining == 0)
                applied = target;       // land EXACTLY on the target
        }
        else
        {
            applied = target;           // hold
        }

        // Up-bend headroom check: the release ramp from rate r consumes
        // ~(r - 1) * rampSamples / 2 samples of lag; release before the head
        // would overrun the write head so the frequency ramp stays continuous.
        if (state == State::Bend && applied > 1.0)
        {
            const double rampSamples = msToSamples (rampMs);
            const double needed      = (applied - 1.0) * 0.5 * rampSamples + 64.0;

            if (lagSamples <= needed)
            {
                state = State::Releasing;
                beginRamp (1.0, rampMs, applied);
            }
        }

        if (state == State::Releasing && rampRemaining == 0)
        {
            state   = State::Idle;
            applied = 1.0;
        }

        return applied;
    }

private:
    enum class State { Idle, Bend, Stop, Releasing };

    double msToSamples (double ms) const noexcept
    {
        return juce::jmax (1.0, ms * 0.001 * fs);
    }

    void beginRamp (double newTarget, double newRampMs, double fromRate) noexcept
    {
        rampMs        = newRampMs;
        target        = newTarget;
        applied       = fromRate;
        rampRemaining = juce::jmax (1, static_cast<int> (msToSamples (newRampMs)));
        step          = (target - applied) / static_cast<double> (rampRemaining);
    }

    State  state         = State::Idle;
    double fs            = 48000.0;
    double applied       = 1.0;
    double target        = 1.0;
    double step          = 0.0;
    int    rampRemaining = 0;
    double rampMs        = 150.0;
};
