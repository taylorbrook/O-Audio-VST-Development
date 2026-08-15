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

    O-Bitrot - Arbitration (Stage 2, Phase 2.1 skeleton — completed in 2.2)

    Per-tick transport-family arbitration (ARCHITECTURE "Clock arbitration"):

      1. Each ENABLED family rolls against its probability using ITS OWN RNG
         stream, in FIXED order tape -> cd -> vinyl (fixed so identical seeds
         give identical draws regardless of which families later change).
      2. If several fire, the `arbitration` stream picks uniformly among them.
      3. A disabled family never rolls and never wins. Disabling a family
         mid-event releases the event gracefully (handled by the processor
         calling tape.release(), not by teleporting to NORMAL).
      4. No firer => ramp back toward NORMAL.

    Phase 2.1: only the tape family can actually fire. CD and vinyl still
    consume their per-tick rolls (comparison discarded) so that Phase 2.2's
    completion changes only which families can WIN, not the tape stream's
    draw sequence.

    RNG is consumed ONLY here (at ticks) — never per-sample, never per-block
    (pattern_rng_stream_interleave_blocksize).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RngBank.h"
#include "TapeTransport.h"

//==============================================================================
/** Linear ~10 ms enable fade for the serial post stages (Packet/Codec/Crush).

    Phase 2.1 note: the transport families need no gain fade — enabling one
    only changes arbitration eligibility at the NEXT tick (events always start
    with ramps), and disabling mid-event goes through the graceful release
    path. The post stages engage through this gain fade when they land in
    Phases 2.3-2.5.
*/
class EnableFade
{
public:
    void prepare (double sampleRate, bool initiallyEnabled) noexcept
    {
        step = 1.0f / static_cast<float> (juce::jmax (1.0, 0.010 * sampleRate));
        gain = initiallyEnabled ? 1.0f : 0.0f;
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
        double tapeRampMs    = 150.0;
    };

    // currentRate: the rate applied last sample — tape ramps start from it.
    void onTick (RngBank& rng, TapeTransport& tape,
                 const Params& p, double currentRate) noexcept
    {
        enum { kTape = 0, kCd = 1, kVinyl = 2 };

        int firers[3] = { 0, 0, 0 };
        int numFirers = 0;

        // Fixed roll order: tape -> cd -> vinyl. Enabled families always
        // consume exactly one draw from their own stream per tick.
        if (p.tapeEnabled)
        {
            if (rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeProb))
                firers[numFirers++] = kTape;
        }

        if (p.cdEnabled)
        {
            // Phase 2.2: `wouldFire` joins the firers. The draw is consumed
            // NOW so completing the family does not shift the tape stream.
            const bool wouldFire =
                rng.get (RngBank::cd).nextFloat() < static_cast<float> (p.cdProb);
            juce::ignoreUnused (wouldFire);
        }

        if (p.vinylEnabled)
        {
            const bool wouldFire =
                rng.get (RngBank::vinyl).nextFloat() < static_cast<float> (p.vinylProb);
            juce::ignoreUnused (wouldFire);
        }

        if (numFirers == 0)
        {
            // No firer: ramp back toward NORMAL (no-op when already Idle).
            tape.release (p.tapeRampMs);
            return;
        }

        // Collision: uniform pick via the arbitration stream. (Unreachable
        // with a single implemented family in 2.1; the code path is the
        // 2.2 contract.)
        const int winner = (numFirers > 1)
                               ? firers[rng.get (RngBank::arbitration).nextInt (numFirers)]
                               : firers[0];

        if (winner == kTape)
        {
            if (rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeStopShare))
            {
                tape.installStop (p.tapeRampMs, currentRate);
            }
            else
            {
                const int idx = rng.get (RngBank::tape).nextInt (TapeTransport::kNumBendIntervals);
                tape.installBend (TapeTransport::kIntervals[idx + 1], p.tapeRampMs, currentRate);
            }
        }
        // winner == kCd / kVinyl: Phase 2.2.
    }
};
