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

    O-Bitrot - VinylTransport (Stage 2, Phase 2.2)

    Revolution-quantized groove jumps (DSP-03):

      * Revolution quantum from VINYL_RPM: 1.8 s @ 33 1/3, 4/3 s @ 45,
        0.769 s @ 78 (v1.7.0).
      * BACKWARD jumps are always exactly one revolution — integer revolution
        multiples are the DSP-03 acceptance. Rate stays 1.0: pitch NEVER
        changes.
      * FORWARD jumps are ALSO exactly one revolution whenever the buffer can
        pay for it (v1.7.0, brief item 27a). Research 4.1 specifies "exactly
        +/- 1 revolution" in BOTH directions, and a groove that only ever
        skips backward and then teleports to live is half a physical model:
        the stylus jumping a ridge forward lands one groove on, not at the
        present. The gate is lag >= revSamples + minLag, i.e. the head has a
        whole revolution of recorded past between it and live. When it does
        not, the historical return-to-live skip is still the fallback — the
        buffer genuinely has no future to jump into. Draw count unchanged.
      * Locked groove: re-jump exactly one revolution each time the head
        returns to the jump point, with a pop per pass. Room-gated: a re-jump
        that would read material older than the ring span self-releases
        instead. At kRingSeconds = 2.5 that gate made a SECOND pass
        arithmetically impossible (it required a negative starting lag), so
        the groove always released after one; the 10 s ring of v1.3.0
        activates the general multi-pass path — >= 4 re-passes at 33 1/3.
        NOTE for the harness: re-passes fire when the head returns to
        lockedEndAbs, one revolution apart, which is NOT the clock grid that
        the initial jump landed on.
      * Every jump goes through ReadHead::clampAndScheduleJump() (crossfaded
        unless HARD_EDGES) and fires a pop (ArtifactSynth, level VINYL_POP).

    RNG discipline: exactly 2 vinyl-stream draws per win (type + direction),
    consumed regardless of which branch executes; pops consume artifactSynth
    draws at the jump instants only.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CaptureRing.h"
#include "ReadHead.h"
#include "RngBank.h"
#include "ArtifactSynth.h"
#include "VinylGeometry.h"

class VinylTransport
{
public:
    void prepare (double sampleRate) noexcept
    {
        fs = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        locked       = false;
        lockedEndAbs = 0.0;
        revSamples   = 0.0;
        popLevel     = 0.0f;
    }

    bool isLocked() const noexcept { return locked; }
    bool isActive() const noexcept { return locked; }

    // Called from Arbitration when the vinyl family wins a tick.
    void onWin (RngBank& rng, int rpmIndex, float pop01,
                ReadHead& head, const CaptureRing& ring, bool hardEdges,
                ArtifactSynth& art) noexcept
    {
        revSamples = static_cast<double> (VinylGeometry::revolutionSamples (fs, rpmIndex));
        popLevel   = pop01;

        // Fixed 2 draws per win, consumed before any branching.
        const float rType = rng.get (RngBank::vinyl).nextFloat();
        const float rDir  = rng.get (RngBank::vinyl).nextFloat();

        const juce::int64 tw     = ring.getTotalWritten();
        const double      lag    = head.getLag (tw);
        const double      minLag = 0.05 * fs;
        const double      budget = head.getMaxLag() - minLag;
        const bool        backRoom = lag + revSamples <= budget;

        locked = false;                       // a new win supersedes a locked state

        if (rType < 0.30f && backRoom)
        {
            // Locked groove: jump back one revolution now; re-jump per pass.
            lockedEndAbs = head.getPosition();
            jumpBack (head, ring, hardEdges, art, rng);
            locked = true;
            return;
        }

        const bool wantForward = rDir >= 0.65f;

        if (wantForward || ! backRoom)
        {
            if (lag >= revSamples + minLag)
            {
                // Forward, revolution-quantized (v1.7.0): there is a whole
                // revolution of recorded past between the head and live, so
                // the stylus can jump exactly +1 revolution and still land on
                // real material with minLag to spare. Rate is untouched, so
                // BRIEF:16's "pitch never changes" holds exactly as it does
                // for the backward jump this mirrors.
                head.clampAndScheduleJump (head.getPosition() + revSamples, tw, hardEdges);
                art.triggerPop (popLevel, rng.get (RngBank::artifactSynth));
            }
            else if (lag > minLag)
            {
                // Not a revolution's worth of lag to spend: fall back to the
                // historical return-to-live skip. The buffer has no future.
                head.clampAndScheduleJump (static_cast<double> (tw - 1), tw, hardEdges);
                art.triggerPop (popLevel, rng.get (RngBank::artifactSynth));
            }
            else if (backRoom)
            {
                jumpBack (head, ring, hardEdges, art, rng);
            }
            // else: no room in either direction — event skipped (draws consumed).
        }
        else
        {
            jumpBack (head, ring, hardEdges, art, rng);
        }
    }

    // Vinyl lost the tick (or family disabled): the locked groove stops
    // re-jumping; the head simply plays on past the loop point. Graceful.
    void release() noexcept { locked = false; }

    // Per-sample, AFTER ReadHead::renderSample (the wrap check needs the
    // advanced position so the re-jump lands before the next read).
    void processSample (ReadHead& head, const CaptureRing& ring, bool hardEdges,
                        ArtifactSynth& art, RngBank& rng) noexcept
    {
        if (! locked)
            return;

        if (head.getPosition() >= lockedEndAbs)
        {
            const juce::int64 tw  = ring.getTotalWritten();
            const double      lag = head.getLag (tw);

            if (lag + revSamples <= head.getMaxLag() - 0.05 * fs)
            {
                head.clampAndScheduleJump (head.getPosition() - revSamples, tw, hardEdges);
                art.triggerPop (popLevel, rng.get (RngBank::artifactSynth));
            }
            else
            {
                locked = false;   // ring cannot age the groove further — release
            }
        }
    }

private:
    void jumpBack (ReadHead& head, const CaptureRing& ring, bool hardEdges,
                   ArtifactSynth& art, RngBank& rng) noexcept
    {
        head.clampAndScheduleJump (head.getPosition() - revSamples,
                                   ring.getTotalWritten(), hardEdges);
        art.triggerPop (popLevel, rng.get (RngBank::artifactSynth));
    }

    double fs           = 48000.0;
    double revSamples   = 0.0;
    double lockedEndAbs = 0.0;
    float  popLevel     = 0.0f;
    bool   locked       = false;
};
