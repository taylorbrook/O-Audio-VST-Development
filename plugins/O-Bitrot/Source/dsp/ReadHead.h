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
      * The fade length is PER JUMP (v1.6.0). 3 ms is right for a recovery
        skip or a vinyl revolution, but a CD anti-shock loop wrapping a 45 ms
        segment spent 7% of every repeat inside the splice — audibly smoother
        than the hard-edged buzz the artifact is. Loop wraps therefore pass
        kLoopSpliceFadeSeconds; everything else keeps the default.
      * A jump arriving MID-FADE is FOLDED into the running crossfade rather
        than restarting it — the dominant head is carried over as the outgoing
        head at the gain already reached (v1.2.1). Restarting dropped the
        outgoing contribution as a step of up to the full jump discontinuity.
        The running fade's OWN length (fadeLenActive) is what the fold
        arithmetic uses, and the fold keeps it whenever it keeps fadeCount —
        adopting a caller's shorter length mid-fade would re-scale the
        outgoing head's (1 - t) weight and reintroduce the very step the fold
        exists to prevent (pattern_splice_jump_guard_previous_fade).
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

    // Default jump crossfade (spec window 1-5 ms).
    static constexpr double kDefaultFadeSeconds = 0.003;

    // Hard-splice crossfade for CD loop wraps (v1.6.0, brief item 14a). The
    // shortest fade the click-safety contract will carry: 24 samples at 48 kHz,
    // 22 at 44.1 kHz — enough that the splice is a ramp rather than a sample
    // step, 6x harder than the default, and 1.1% rather than 6.7% of the
    // Skipping Disc preset's 45 ms segment.
    static constexpr double kLoopSpliceFadeSeconds = 0.0005;

    void prepare (double sampleRate, int ringSizeSamples)
    {
        fs = sampleRate;

        // Jump crossfades, computed once here. fadeLenActive is whichever of
        // these (or any caller override) the fade currently running was
        // started with — see clampAndScheduleJump.
        fadeLenDefault = juce::jmax (1, static_cast<int> (kDefaultFadeSeconds * fs));
        fadeLenSplice  = juce::jmax (1, static_cast<int> (kLoopSpliceFadeSeconds * fs));
        fadeLenActive  = fadeLenDefault;

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
        pos           = 0.0;
        lastRate      = 1.0;
        trim          = 0.0;
        fadeActive    = false;
        fadeCount     = 0;
        fadeLenActive = fadeLenDefault;
        oldPos        = 0.0;
        oldRate       = 1.0;
    }

    double getPosition() const noexcept { return pos; }
    double getMaxLag() const noexcept   { return maxLagSamples; }

    // Fade lengths a caller may pass to clampAndScheduleJump. Exposed rather
    // than recomputed at each call site so there is exactly one definition of
    // each (and so the harness can assert against it instead of mirroring it —
    // pattern_test_fixture_mirrors_drift_silently).
    int getDefaultFadeSamples() const noexcept { return fadeLenDefault; }
    int getSpliceFadeSamples()  const noexcept { return fadeLenSplice; }

    // Lag in samples behind the last-written ring sample (>= 0 in practice).
    double getLag (juce::int64 totalWritten) const noexcept
    {
        return static_cast<double> (totalWritten - 1) - pos;
    }

    // THE choke point. Every position change — CD recovery skips, vinyl
    // revolution jumps, lag-overflow clamps — lands here. The target is
    // clamped into the valid ring window, then applied through a two-head
    // crossfade (instant under hardEdges).
    //
    // fadeSamples <= 0 means "the default 3 ms". A caller passes a length only
    // when the SPLICE ITSELF is the artifact: CD loop wraps pass
    // getSpliceFadeSamples(). Recovery jumps, vinyl revolutions and the
    // overflow clamp all take the default — a recovery skip is the player
    // getting its act together, not a glitch, and a hard edge there would be a
    // second artifact on top of the one that caused it.
    void clampAndScheduleJump (double newPosAbs, juce::int64 totalWritten, bool hardEdges,
                               int fadeSamples = 0) noexcept
    {
        const double hi = static_cast<double> (totalWritten - 1);
        const double lo = hi - maxLagSamples;
        const double p  = juce::jlimit (lo, hi, newPosAbs);

        const int requestedFade = fadeSamples > 0 ? fadeSamples : fadeLenDefault;

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
            //
            // t is measured against fadeLenActive — the length the RUNNING
            // fade was started with, which need not be the length this caller
            // is asking for (v1.6.0).
            const double t = static_cast<double> (fadeCount)
                             / static_cast<double> (fadeLenActive);

            if (t >= 0.5)
            {
                // The incoming head already dominates: it becomes the new
                // outgoing head and gets the full (1 - t) role from scratch.
                // fadeCount restarts, so the requested length can be adopted.
                oldPos        = pos;
                oldRate       = lastRate;
                fadeCount     = 0;
                fadeLenActive = requestedFade;
            }
            // else: the outgoing head still dominates — keep oldPos, oldRate,
            // fadeCount AND fadeLenActive so its (1 - t) share is bit-unchanged
            // and only the incoming head's target moves. Adopting a shorter
            // requested length here would rescale t from fadeCount/A to
            // fadeCount/B and step the outgoing head's weight — for a 3 ms
            // fade 100 samples in, retargeted with a 24-sample splice, from
            // 0.31 straight to 0 (pattern_splice_jump_guard_previous_fade). A
            // mid-fade collision therefore SPENDS the running length; the
            // splice request is dropped, never honoured at the cost of a step.
        }
        else
        {
            oldPos        = pos;
            oldRate       = lastRate;   // the old head keeps its own rate for the fade
            fadeCount     = 0;
            fadeLenActive = requestedFade;
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
    //
    // readOffset (v1.4.0): a NON-NEGATIVE extra read lag in samples, applied to
    // BOTH heads — the wow/flutter bed. It is deliberately not a rate
    // multiplier: modulating the rate would falsify the loopOwnsRate premise
    // below AND, at the engine's lag-0 steady state, drive `pos` into the
    // write-slot pin every time the deviation went positive (a zero-order hold,
    // not wow). Offsetting the READ instead is the same physics — pitch
    // deviation is the derivative of delay — while `pos`, the lag budget and
    // the whole jump contract stay exactly as they were. Both heads share the
    // offset because they are one physical transport.
    //
    // `pos - 0.0` is bit-identical to `pos`, so an offset of exactly 0 leaves
    // the passthrough on CaptureRing's exact-integer fast path (FUNC-02).
    void renderSample (const CaptureRing& ring, double rate, bool hardEdges,
                       bool loopOwnsRate, double readOffset, float& outL, float& outR) noexcept
    {
        const juce::int64 totalWritten = ring.getTotalWritten();
        const double      hi           = static_cast<double> (totalWritten - 1);

        const float mL = ring.readFrac (0, pos - readOffset);
        const float mR = ring.readFrac (1, pos - readOffset);

        if (fadeActive)
        {
            const float oL = ring.readFrac (0, oldPos - readOffset);
            const float oR = ring.readFrac (1, oldPos - readOffset);

            ++fadeCount;
            const float t = static_cast<float> (fadeCount) / static_cast<float> (fadeLenActive);

            outL = mL * t + oL * (1.0f - t);
            outR = mR * t + oR * (1.0f - t);

            oldPos += oldRate;
            if (oldPos > hi)
                oldPos = hi;

            if (fadeCount >= fadeLenActive)
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

    // Two-head jump crossfade. fadeLenDefault/fadeLenSplice are the two
    // lengths prepare() computes; fadeLenActive is the one the fade currently
    // in flight is running on, and is the ONLY denominator the fold and the
    // render loop may use.
    bool   fadeActive     = false;
    int    fadeCount      = 0;
    int    fadeLenDefault = 1;
    int    fadeLenSplice  = 1;
    int    fadeLenActive  = 1;
    double oldPos         = 0.0;
    double oldRate        = 1.0;
};
