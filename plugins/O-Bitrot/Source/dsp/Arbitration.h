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

    O-Bitrot - Arbitration (Stage 2 — completed in Phase 2.2)

    Per-tick transport-family arbitration (ARCHITECTURE "Clock arbitration"):

      1. Each ENABLED family rolls against its probability using ITS OWN RNG
         stream, in FIXED order tape -> cd -> vinyl. The roll ORDER and
         per-tick draw pattern are identical to Phase 2.1 (where cd/vinyl
         rolled but could not win), so 2.1 renders with cd/vinyl disabled are
         unchanged by this file's completion.
      2. If several fire, the `arbitration` stream picks uniformly among them.
      3. A disabled family never rolls and never wins. The winner takes over:
         the other families release gracefully (tape ramps back, a CD loop
         recovers forward, a locked groove stops re-jumping; the bounded CD
         conceal/mute rungs finish naturally).
      4. No firer => everything ramps back toward NORMAL.

    A tape win installs one of THREE things as of v1.4.0: a stop, a dropout,
    or a bend. The dropout roll is skipped when its share is 0, which keeps the
    tape stream's draw pattern — and every pre-v1.4.0 render — bit-identical at
    the default.

    RNG is consumed ONLY here (at ticks) plus at deterministic jump instants
    (pops) and on the wow bed's own private sample counter — never per-sample
    on a SHARED stream, never per-block
    (pattern_rng_stream_interleave_blocksize).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RngBank.h"
#include "TapeTransport.h"
#include "TapeDropout.h"
#include "CDSkip.h"
#include "VinylTransport.h"
#include "ReadHead.h"
#include "CaptureRing.h"
#include "ArtifactSynth.h"

//==============================================================================
/** Linear ~10 ms enable fade for the serial post stages (Packet/Codec/Crush).

    The transport families need no gain fade — enabling one only changes
    arbitration eligibility at the NEXT tick (events always start with ramps),
    and disabling mid-event goes through the graceful release paths. The post
    stages engage through this gain fade when they land in Phases 2.3-2.5.
*/
class EnableFade
{
public:
    void prepare (double sampleRate, bool initiallyEnabled) noexcept
    {
        step = 1.0f / static_cast<float> (juce::jmax (1.0, 0.010 * sampleRate));
        gain = initiallyEnabled ? 1.0f : 0.0f;
        targetOn = initiallyEnabled;
    }

    void setEnabled (bool shouldBeEnabled) noexcept { targetOn = shouldBeEnabled; }

    // Per-sample fade gain toward the target (linear, ~10 ms).
    float next() noexcept
    {
        const float target = targetOn ? 1.0f : 0.0f;
        if (gain < target)      gain = juce::jmin (target, gain + step);
        else if (gain > target) gain = juce::jmax (target, gain - step);
        return gain;
    }

    bool isFullyOff() const noexcept { return ! targetOn && gain <= 0.0f; }

private:
    float gain     = 0.0f;
    float step     = 1.0f;
    bool  targetOn = false;
};

//==============================================================================
class Arbitration
{
public:
    struct Params
    {
        bool   tapeEnabled   = true;
        bool   cdEnabled     = true;
        bool   vinylEnabled  = true;
        double tapeProb      = 0.25;   // 0..1
        double cdProb        = 0.25;   // 0..1
        double vinylProb     = 0.25;   // 0..1
        double tapeStopShare = 0.10;   // 0..1
        double tapeDropShare = 0.0;    // 0..1, share of NON-stop tape wins
        double tapeRampMs    = 150.0;
        double cdSeverity    = 0.5;    // 0..1
        double cdSegmentMs   = 100.0;
        int    vinylRpmIndex = 0;      // 0 = 33 1/3, 1 = 45
        float  vinylPop01    = 0.5f;   // 0..1
    };

    struct TickContext
    {
        TapeTransport&     tape;
        TapeDropout&       tapeDropout;
        CDSkip&            cd;
        VinylTransport&    vinyl;
        ReadHead&          head;
        const CaptureRing& ring;
        ArtifactSynth&     art;
        double             currentRate;   // rate applied last sample (ramp start)
        bool               hardEdges;
    };

    void onTick (RngBank& rng, const Params& p, TickContext& ctx) noexcept
    {
        enum { kTape = 0, kCd = 1, kVinyl = 2 };

        int firers[3] = { 0, 0, 0 };
        int numFirers = 0;

        // Fixed roll order: tape -> cd -> vinyl. Enabled families always
        // consume exactly one draw from their own stream per tick (identical
        // to Phase 2.1's pattern).
        if (p.tapeEnabled
            && rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeProb))
            firers[numFirers++] = kTape;

        if (p.cdEnabled
            && rng.get (RngBank::cd).nextFloat() < static_cast<float> (p.cdProb))
            firers[numFirers++] = kCd;

        if (p.vinylEnabled
            && rng.get (RngBank::vinyl).nextFloat() < static_cast<float> (p.vinylProb))
            firers[numFirers++] = kVinyl;

        if (numFirers == 0)
        {
            // No firer: ramp back toward NORMAL (all no-ops when idle).
            ctx.tape.release (p.tapeRampMs);
            ctx.cd.release (ctx.head, ctx.ring, ctx.hardEdges);
            ctx.vinyl.release();
            return;
        }

        // Collision: uniform pick via the arbitration stream.
        const int winner = (numFirers > 1)
                               ? firers[rng.get (RngBank::arbitration).nextInt (numFirers)]
                               : firers[0];

        switch (winner)
        {
            case kTape:
            {
                ctx.cd.release (ctx.head, ctx.ring, ctx.hardEdges);
                ctx.vinyl.release();

                if (rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeStopShare))
                {
                    ctx.tape.installStop (p.tapeRampMs, ctx.currentRate);
                }
                // Dropout (v1.4.0) takes a share of the NON-stop tape wins.
                // The roll is skipped entirely at share 0 — which is the
                // default — so the tape stream's draw pattern, and therefore
                // every render made before this feature existed, is
                // bit-identical until the knob is turned up.
                //
                // Unlike a stop or a bend, a dropout installs no rate event:
                // it is gain and filter domain only, so a bend already in
                // flight keeps ramping underneath it (the OVERLAY class that
                // brief item 6 generalises). The cd/vinyl releases above still
                // apply — single-winner arbitration is unchanged.
                else if (p.tapeDropShare > 0.0
                         && rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeDropShare))
                {
                    ctx.tapeDropout.trigger (rng.get (RngBank::tape));
                }
                else
                {
                    const int idx = rng.get (RngBank::tape).nextInt (TapeTransport::kNumBendIntervals);
                    ctx.tape.installBend (TapeTransport::kIntervals[idx + 1],
                                          p.tapeRampMs, ctx.currentRate);
                }
                break;
            }

            case kCd:
                ctx.tape.release (p.tapeRampMs);
                ctx.vinyl.release();
                ctx.cd.onWin (rng, p.cdSeverity, p.cdSegmentMs,
                              ctx.head, ctx.ring, ctx.hardEdges, ctx.art);
                break;

            case kVinyl:
                ctx.tape.release (p.tapeRampMs);
                ctx.cd.release (ctx.head, ctx.ring, ctx.hardEdges);
                ctx.vinyl.onWin (rng, p.vinylRpmIndex, p.vinylPop01,
                                 ctx.head, ctx.ring, ctx.hardEdges, ctx.art);
                break;

            default:
                break;
        }
    }
};
