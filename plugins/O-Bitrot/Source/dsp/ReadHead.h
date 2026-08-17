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

    O-Bitrot - ReadHead (Stage 2, Phase 2.1)

    Fractional-position read head over the shared CaptureRing. Both channels
    share one position (the failure is the PLAYER, not the channels); the
    per-channel interpolation state lives in CaptureRing::readFrac.

    Contract points (ARCHITECTURE + Stage-2 PLAN):

      * ALL position changes route through the single clampAndScheduleJump()
        choke point (runtime lag clamp into [minLag, ringSpan - margin]).
      * Jumps are TRUE two-head crossfades — the old head keeps reading at its
        own position/rate for the fade length while the new head ramps in,
        linear equal-gain, ~3 ms computed in prepare(). O-Polystutter's
        held-sample blend is deliberately NOT reproduced (it freezes the old
        material to DC). Skipped entirely under HARD_EDGES.
      * A jump arriving MID-FADE is FOLDED into the running crossfade rather
        than restarting it — the dominant head is carried over as the outgoing
        head at the gain already reached (v1.2.1). Restarting dropped the
        outgoing contribution as a step of up to the full jump discontinuity.
      * The lag-overflow clamp is SUPPRESSED while a loop transport owns the
        read rate (v1.3.0). CD loops and locked grooves hold rate at exactly
        1.0 and gate their own jumps on the same lag budget, so the clamp can
        only fire spuriously there — and firing teleported the head forward
        while CDSkip still read state == Loop, which re-jumped on the next
        wrap. When it does fire it now lands at a FIXED lag rather than half
        the ring span.
      * NORMAL-state gentle re-approach: when no transport event is active, a
        ramped rate trim of at most +2% pulls accumulated lag back toward 0.
        The trim is EXACTLY 0.0 when lag is 0, so the all-off passthrough
        stays on the bit-exact integer path (FUNC-02).
      * Rate 1.0 with lag 0 is legal because the processor writes the ring
        BEFORE reading, per sample (blockSize-invariance trap).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CaptureRing.h"

class ReadHead
{
public:
    // Landing lag for the last-resort overflow recovery. Deliberately a fixed
    // duration rather than a fraction of the ring — see prepare().
    static constexpr double kOverflowRecoveryLagSeconds = 1.2;

    void prepare (double sampleRate, int ringSizeSamples)
    {
        fs = sampleRate;

        // Jump crossfade: 3 ms (spec window 1-5 ms), computed once here.
        fadeLenSamples = juce::jmax (1, static_cast<int> (0.003 * fs));

        // Runtime lag ceiling: ring span minus the safety margin. This grows
        // with the ring, and is what the loop transports spend as their
        // multi-pass budget (CDSkip / VinylTransport room gates).
        maxLagSamples = static_cast<double> (ringSizeSamples)
                        - CaptureRing::kSafetySeconds * fs;

        // Where the last-resort overflow recovery LANDS is a separate
        // question, and must NOT scale with the ring. The pre-1.3.0
        // expression was 0.5 * maxLag — 1.2 s behind at the old 2.5 s ring,
        // but carried unchanged onto a 10 s ring it would teleport the head
        // 4.95 s back into stale material. Pinning the constant keeps the
        // safety net's landing distance exactly what it has always been; the
        // jmin only matters for a hypothetical ring too small to hold it.
        overflowRecoveryLagSamples = juce::jmin (kOverflowRecoveryLagSeconds * fs,
                                                 0.5 * maxLagSamples);

        // Re-approach trim shape: full +2% above 50 ms of lag, proportional
        // below; the trim itself ramps over ~10 ms (never a rate step).
        trimStep        = 0.02 / (0.010 * fs);
        trimFullLag     = 0.05 * fs;

        reset();
    }

    void reset() noexcept
    {
        pos        = 0.0;
        lastRate   = 1.0;
        trim       = 0.0;
        fadeActive = false;
        fadeCount  = 0;
        oldPos     = 0.0;
        oldRate    = 1.0;
    }

    double getPosition() const noexcept { return pos; }
    double getMaxLag() const noexcept   { return maxLagSamples; }

    // Lag in samples behind the last-written ring sample (>= 0 in practice).
    double getLag (juce::int64 totalWritten) const noexcept
    {
        return static_cast<double> (totalWritten - 1) - pos;
    }

    // THE choke point. Every position change — CD recovery skips, vinyl
    // revolution jumps, lag-overflow clamps — lands here. The target is
    // clamped into the valid ring window, then applied through a two-head
    // crossfade (instant under hardEdges).
    void clampAndScheduleJump (double newPosAbs, juce::int64 totalWritten, bool hardEdges) noexcept
    {
        const double hi = static_cast<double> (totalWritten - 1);
        const double lo = hi - maxLagSamples;
        const double p  = juce::jlimit (lo, hi, newPosAbs);

        if (hardEdges)
        {
            pos        = p;
            fadeActive = false;
            return;
        }

        if (fadeActive)
        {
            // A jump arrived MID-FADE. Blindly overwriting oldPos/oldRate and
            // zeroing fadeCount drops the outgoing head's contribution as an
            // output STEP of (1 - t) * |newHead - oldHead| — the FULL jump
            // discontinuity when t is near 0, which violates the click-safety
            // contract. This is reachable in a single tick: Arbitration's
            // kVinyl branch runs cd.release() (recovery jump) and then
            // vinyl.onWin() (second jump) with no render in between, so t is
            // exactly 0 and the material actually playing would be discarded.
            //
            // Fold instead: keep whichever head currently DOMINATES the mix as
            // the outgoing head and resume from the gain already reached, so
            // the dominant contribution is continuous across the retarget.
            // The residual step is bounded by 0.5 * |delta| in every case, and
            // is exactly 0 in the same-tick collision above.
            const double t = static_cast<double> (fadeCount)
                             / static_cast<double> (fadeLenSamples);

            if (t >= 0.5)
            {
                // The incoming head already dominates: it becomes the new
                // outgoing head and gets the full (1 - t) role from scratch.
                oldPos    = pos;
                oldRate   = lastRate;
                fadeCount = 0;
            }
            // else: the outgoing head still dominates — keep oldPos, oldRate
            // AND fadeCount so its (1 - t) share is bit-unchanged and only the
            // incoming head's target moves.
        }
        else
        {
            oldPos    = pos;
            oldRate   = lastRate;   // the old head keeps its own rate for the fade
            fadeCount = 0;
        }

        fadeActive = true;
        pos        = p;
    }

    // NORMAL-state rate (no transport event active): 1.0 plus a ramped trim
    // of at most +2% that gently re-approaches lag 0. Returns exactly 1.0
    // when lag is 0 and the trim has decayed (bit-exact passthrough path).
    double reapproachRate (double lagSamples) noexcept
    {
        const double desired = juce::jlimit (0.0, 0.02,
                                             0.02 * (lagSamples / trimFullLag));
        const double d = desired - trim;
        trim += juce::jlimit (-trimStep, trimStep, d);

        if (std::abs (trim) < 1.0e-9)
            trim = 0.0;

        return 1.0 + trim;
    }

    // Called while a transport event owns the rate: the trim must restart
    // from 0 when NORMAL resumes (a stale trim would be a rate STEP of up to
    // 2% at event end; from 0 the ramp is continuous by construction).
    void clearTrim() noexcept { trim = 0.0; }

    // Renders one stereo sample at the current position, then advances by
    // `rate`. Call AFTER the ring write for this sample.
    //
    // loopOwnsRate: a CD loop or a locked groove is driving this sample AND
    // no tape event is in flight underneath it — i.e. `rate` is exactly 1.0.
    // See the overflow guard at the bottom of this function.
    void renderSample (const CaptureRing& ring, double rate, bool hardEdges,
                       bool loopOwnsRate, float& outL, float& outR) noexcept
    {
        const juce::int64 totalWritten = ring.getTotalWritten();
        const double      hi           = static_cast<double> (totalWritten - 1);

        const float mL = ring.readFrac (0, pos);
        const float mR = ring.readFrac (1, pos);

        if (fadeActive)
        {
            const float oL = ring.readFrac (0, oldPos);
            const float oR = ring.readFrac (1, oldPos);

            ++fadeCount;
            const float t = static_cast<float> (fadeCount) / static_cast<float> (fadeLenSamples);

            outL = mL * t + oL * (1.0f - t);
            outR = mR * t + oR * (1.0f - t);

            oldPos += oldRate;
            if (oldPos > hi)
                oldPos = hi;

            if (fadeCount >= fadeLenSamples)
                fadeActive = false;
        }
        else
        {
            outL = mL;
            outR = mR;
        }

        lastRate = rate;
        pos += rate;

        // Pin at the next write slot: write-then-read makes lag 0 legal, so
        // the head may sit exactly one sample ahead of the last write (it
        // will be written before the next read). At rate 1.0 this is an
        // exact no-op.
        const double next = hi + 1.0;
        if (pos > next)
            pos = next;

        // Lag overflow (deep stops / down-bends stacking up): jump forward
        // through the choke point — never a hidden teleport.
        //
        // Suppressed while a loop transport owns the rate. Such a loop holds
        // `rate` at exactly 1.0, so lag is CONSTANT between its own jumps,
        // and each of those jumps is gated on this same budget — the head
        // provably never crosses maxLagSamples while the suppression is in
        // effect, so the clamp here could only ever fire spuriously. It used
        // to: on a sustained CD loop it teleported the head forward while
        // CDSkip still read state == Loop, and the very next wrap re-jumped
        // from the teleported position — an unplanned slip attributable to no
        // family. The tape-idle half of the flag matters: a CD or vinyl win
        // starts a tape RELEASE ramp that keeps running underneath the loop
        // for up to TAPE_RAMP ms, and during that ramp the rate is not 1.0,
        // so lag can genuinely drift and the clamp must stay armed.
        if (! loopOwnsRate && next - pos > maxLagSamples)
            clampAndScheduleJump (next - overflowRecoveryLagSamples, totalWritten, hardEdges);
    }

private:
    double fs             = 48000.0;
    double pos            = 0.0;    // fractional absolute read position
    double lastRate       = 1.0;
    double maxLagSamples  = 0.0;
    double overflowRecoveryLagSamples = 0.0;

    // NORMAL-state re-approach trim
    double trim        = 0.0;
    double trimStep    = 0.0;
    double trimFullLag = 1.0;

    // Two-head jump crossfade
    bool   fadeActive     = false;
    int    fadeCount      = 0;
    int    fadeLenSamples = 1;
    double oldPos         = 0.0;
    double oldRate        = 1.0;
};
