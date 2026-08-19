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

    O-Bitrot - Arbitration (Stage 2, Phase 2.2; OVERLAY CLASS added v1.9.0)

    Per-tick transport-family arbitration (ARCHITECTURE "Clock arbitration").

    THE TWO EVENT CLASSES (v1.9.0, improvement brief item 6)
    --------------------------------------------------------
    Until v1.8.0 exactly one family could act per tick. That is right for the
    transport and wrong for everything else: real broken media fail in parallel,
    and the engine already half-admitted it — CDSkip::release only ever touched
    the Loop state, so a bounded conceal/mute rung could FINISH under a tape win
    even though it could never START under one. v1.9.0 makes the distinction
    explicit and applies it in both directions.

      OWNER   — drives the transport: a rate (tape stop, tape bend) or a head
                position (CD loop, vinyl jump, vinyl locked groove). There is
                one transport and one read head, so these still contend
                single-winner. A coherent playback position is the whole reason
                the arbitration existed.
      OVERLAY — gain, filter and artifact domain, applied orthogonally to
                position and rate: the CD conceal dip, the CD mute + residual
                tick, the tape dropout, the vinyl standalone pop, and — as of
                v1.10.0 — every Rot event. These fire whenever their family's
                roll succeeds, regardless of who owns the tick.

    THE FOURTH FAMILY (v1.10.0, improvement brief item 8)
    -----------------------------------------------------
    Rot is the first family that is overlay-ONLY: bit flips, sticky holds and
    wrong-decode stretches all rewrite the rendered pair and none of them moves
    the head or the rate, so rot never enters the ownership contest and never
    draws from the `arbitration` stream. Adding it is therefore a pure append —
    it rolls last, on its own stream, after the trio have finished with theirs.

    CONTAINMENT, RESTATED FOR THIS RELEASE: with ROT_ENABLE off the gate
    short-circuits before its draw, so the rot family consumes NOTHING and every
    render made before v1.10.0 is bit-identical. That is what keeps the A3, V1,
    N7 and N8 cross-version anchors green through this feature rather than
    forcing yet another re-anchor.

    A vinyl roll that loses is the one case that changes kind rather than simply
    proceeding: the groove cannot skip while another family holds the head, but
    the stylus still met the debris, so it degrades to its overlay residue — the
    pop alone. That is the surface crackle that keeps vinyl audible under a tape
    bend, and it is what makes "Total Media Failure" sound like everything
    failing at once rather than like a queue.

    PER-TICK SEQUENCE
    -----------------
      1. Gates. Each ENABLED family rolls against its probability using ITS OWN
         stream, in FIXED order tape -> cd -> vinyl -> rot — the same single
         draw per enabled family that every release since Phase 2.1 has taken.
      2. Kinds. Each FIRER draws its event kind from its own stream, same fixed
         order. This is the step that moved in v1.9.0; it used to sit inside the
         winner's branch, which is precisely what made an overlay unreachable on
         a tick some other family had won.
      3. Ownership. The owner-class firers contend; the `arbitration` stream
         picks uniformly among them when there is more than one.
      4. Releases, then 5. overlays, then 6. the owner's install. Releases run
         first because cd.release() can take a recovery jump to live and the
         winner's own jump is measured from wherever that leaves the head.

    CONTAINMENT
    -----------
    On any tick with AT MOST ONE firer, the draw sequence, the release calls and
    the installed event are all exactly what v1.8.0 produced: a lone firer
    either owns the tick or is a lone overlay, and either way the two silent
    families release just as the old winner released them. So renders move ONLY
    where two or more families fire on the same tick. Probe A3 in the render
    harness pins that against digests recorded from the v1.8.0 tree.

    RNG is consumed ONLY here (at ticks) plus at deterministic jump instants
    (pops) and on the wow bed's own private sample counter — never per-sample
    on a SHARED stream, never per-block
    (pattern_rng_stream_interleave_blocksize). Overlay decisions are drawn at
    tick time in the fixed order above, so block-size invariance is unaffected
    by the split.

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
#include "RotStage.h"

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

        // Rot (v1.10.0). Default OFF: the family is opt-in, and while it is off
        // it takes no draws at all (see the containment note above).
        bool   rotEnabled    = false;
        double rotProb       = 0.25;   // 0..1
        double rotStickShare = 0.25;   // 0..1, share of rot wins that HOLD
        double rotGarbleShare= 0.25;   // 0..1, share of NON-stick wins
        double rotDepth      = 0.5;    // 0..1, flip severity
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
        RotStage&          rot;
        double             currentRate;   // rate applied last sample (ramp start)
        bool               hardEdges;
    };

    void onTick (RngBank& rng, const Params& p, TickContext& ctx) noexcept
    {
        enum { kNone = -1, kTape = 0, kCd = 1, kVinyl = 2 };

        // ── 1. Gates ────────────────────────────────────────────────────────
        // Fixed roll order: tape -> cd -> vinyl. Enabled families always
        // consume exactly one draw from their own stream per tick (identical
        // to Phase 2.1's pattern).
        const bool tapeFired = p.tapeEnabled
            && rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeProb);

        const bool cdFired = p.cdEnabled
            && rng.get (RngBank::cd).nextFloat() < static_cast<float> (p.cdProb);

        const bool vinylFired = p.vinylEnabled
            && rng.get (RngBank::vinyl).nextFloat() < static_cast<float> (p.vinylProb);

        // Rot rolls LAST and on its own stream, so its arrival cannot perturb
        // the trio's draw pattern. Disabled short-circuits before the draw,
        // which is what makes every pre-v1.10.0 render bit-identical.
        const bool rotFired = p.rotEnabled
            && rng.get (RngBank::rot).nextFloat() < static_cast<float> (p.rotProb);

        // ── 2. Classification ───────────────────────────────────────────────
        // Same fixed order; each firer draws its event KIND from its OWN
        // stream. This is the step that moved in v1.9.0: the kind rolls used to
        // live inside the winner's branch, which is exactly what made an
        // overlay unreachable on a tick another family had won.
        TapeKind tapeKind = TapeKind::Bend;
        if (tapeFired)
            tapeKind = rollTapeKind (rng, p);

        CDSkip::Rung cdRung = CDSkip::Rung::Conceal;
        if (cdFired)
            cdRung = CDSkip::rollRung (rng.get (RngBank::cd), p.cdSeverity);

        VinylTransport::Kind vinylKind = VinylTransport::Kind::Jump;
        if (vinylFired)
            vinylKind = ctx.vinyl.rollKind (rng, p.vinylRpmIndex, p.vinylPop01,
                                            ctx.head, ctx.ring);

        juce::ignoreUnused (vinylKind);   // both vinyl kinds are OWNER; see below

        RotStage::Kind rotKind = RotStage::Kind::Flip;
        if (rotFired)
            rotKind = rollRotKind (rng, p);

        // ── 3. Ownership ────────────────────────────────────────────────────
        // OWNER events drive the transport — a rate (tape stop/bend) or a head
        // position (CD loop, vinyl jump/locked groove). There is one transport,
        // so exactly one of them can win, and the arbitration stream picks
        // uniformly among the contenders just as it always has. The draw is
        // taken only when there IS a contest, which is now measured in OWNERS
        // rather than in firers.
        const bool tapeOwnerClass  = tapeFired && tapeKind != TapeKind::Dropout;
        const bool cdOwnerClass    = cdFired   && cdRung   == CDSkip::Rung::Loop;
        const bool vinylOwnerClass = vinylFired;   // jump and locked groove both move the head

        int owners[3]  = { 0, 0, 0 };
        int numOwners  = 0;

        if (tapeOwnerClass)  owners[numOwners++] = kTape;
        if (cdOwnerClass)    owners[numOwners++] = kCd;
        if (vinylOwnerClass) owners[numOwners++] = kVinyl;

        const int owner = numOwners == 0
                              ? kNone
                              : (numOwners > 1
                                     ? owners[rng.get (RngBank::arbitration).nextInt (numOwners)]
                                     : owners[0]);

        // ── 4. Releases ─────────────────────────────────────────────────────
        // A family gives up its transport state when it does not own the tick
        // — UNLESS it fired an overlay and nobody owns, in which case there is
        // nothing to yield to and whatever it was already doing keeps running.
        // That exception is not new: v1.8.0 already left a bend ramping under a
        // tape-dropout win, and this is the general form of that rule.
        //
        // Releases run BEFORE any install, which is load-bearing and always
        // was: cd.release() can take a recovery jump to live, and the winning
        // family's own jump is measured from wherever that leaves the head.
        const bool tapeFiredOverlay = tapeFired && tapeKind == TapeKind::Dropout;
        const bool cdFiredOverlay   = cdFired   && cdRung   != CDSkip::Rung::Loop;

        if (owner != kTape && ! (tapeFiredOverlay && owner == kNone))
            ctx.tape.release (p.tapeRampMs);

        // A cd overlay is NOT released here: applyOverlay leaves a running loop
        // through its own recovery jump rather than through endLoop, because
        // endLoop can enter the servo seek and swallow the rung being installed.
        if (owner != kCd && ! cdFiredOverlay)
            ctx.cd.release (ctx.head, ctx.ring, ctx.hardEdges);

        // Likewise vinyl: a firing that lost releases inside applyStandalonePop.
        if (owner != kVinyl && ! vinylFired)
            ctx.vinyl.release();

        // ── 5. Overlays ─────────────────────────────────────────────────────
        // Gain / filter / artifact domain, orthogonal to head position and to
        // rate. These fire whenever their family's roll succeeded, whoever owns
        // the tick — the whole point of item 6, and what makes a total failure
        // sound like everything failing at once instead of like a queue.
        // Fixed order tape -> cd -> vinyl -> rot, matching the roll order.
        if (tapeFiredOverlay)
            ctx.tapeDropout.trigger (rng.get (RngBank::tape));

        if (cdFiredOverlay)
            ctx.cd.applyOverlay (rng, cdRung, p.cdSeverity,
                                 ctx.head, ctx.ring, ctx.hardEdges, ctx.art);

        if (vinylFired && owner != kVinyl)
            ctx.vinyl.applyStandalonePop (ctx.art, rng);

        // Rot last in the overlay order, matching its position in the roll
        // order. Nothing about it is conditional on the owner: a corrupt file
        // is corrupt whether or not the tape is also dragging.
        if (rotFired)
            ctx.rot.trigger (rng.get (RngBank::rot), rotKind, p.rotDepth, ctx.hardEdges);

        // ── 6. Owner install ────────────────────────────────────────────────
        switch (owner)
        {
            case kTape:
                if (tapeKind == TapeKind::Stop)
                {
                    ctx.tape.installStop (p.tapeRampMs, ctx.currentRate);
                }
                else
                {
                    const int idx = rng.get (RngBank::tape).nextInt (TapeTransport::kNumBendIntervals);
                    ctx.tape.installBend (TapeTransport::kIntervals[idx + 1],
                                          p.tapeRampMs, ctx.currentRate);
                }
                break;

            case kCd:
                ctx.cd.applyLoop (rng, p.cdSeverity, p.cdSegmentMs,
                                  ctx.head, ctx.ring, ctx.hardEdges, ctx.art);
                break;

            case kVinyl:
                ctx.vinyl.applyOwner (ctx.head, ctx.ring, ctx.hardEdges, ctx.art, rng);
                break;

            default:
                break;
        }
    }

private:
    enum class TapeKind { Stop, Dropout, Bend };

    /** Pure classification for the tape family. Consumes 1 tape draw (stop?)
        plus, only when the dropout share is above 0, a second (dropout?).

        The conditional second draw is deliberate and predates this split: at
        share 0 — the default — the roll is skipped entirely, so the tape
        stream's pattern, and therefore every render made before the dropout
        feature existed, stays bit-identical until the knob is turned up. The
        bend's INTERVAL draw is not taken here: it is a parameter of an event
        that may never install, so it belongs to the apply step, which is the
        same division CDSkip::rollRung draws. */
    static TapeKind rollTapeKind (RngBank& rng, const Params& p) noexcept
    {
        if (rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeStopShare))
            return TapeKind::Stop;

        if (p.tapeDropShare > 0.0
            && rng.get (RngBank::tape).nextFloat() < static_cast<float> (p.tapeDropShare))
            return TapeKind::Dropout;

        return TapeKind::Bend;
    }

    /** Pure classification for the rot family (v1.10.0). Consumes EXACTLY TWO
        draws from the `rot` stream on every call — the STICK test, then the
        GARBLE test over what is left — which is the same share ladder the tape
        family uses.

        Unlike rollTapeKind the second draw is UNCONDITIONAL. That function
        skips its dropout roll at share 0 so renders predating the dropout
        feature stay bit-identical; rot is a new family with no earlier pattern
        to protect, and an unconditional pair is the simpler invariant to hold
        on to. */
    static RotStage::Kind rollRotKind (RngBank& rng, const Params& p) noexcept
    {
        auto& s = rng.get (RngBank::rot);

        const float rStick  = s.nextFloat();
        const float rGarble = s.nextFloat();

        if (rStick < static_cast<float> (p.rotStickShare))
            return RotStage::Kind::Stick;

        if (rGarble < static_cast<float> (p.rotGarbleShare))
            return RotStage::Kind::Garble;

        return RotStage::Kind::Flip;
    }
};
