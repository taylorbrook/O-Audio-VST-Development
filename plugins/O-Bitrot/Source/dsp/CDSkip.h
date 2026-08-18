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

    O-Bitrot - CDSkip (Stage 2, Phase 2.2)

    CIRC failure ladder (DSP-02). CD_SEVERITY (0-1) positions a weighting
    across three rungs; an event picks its rung by a biased roll on the cd
    stream (rungFloat = severity*3 + (r-0.5)*1.5, clamped, floored):

      Rung 0 — interpolation concealment: 30-80 ms FirstOrderTPTFilter dip,
               cutoff swept 20 kHz -> ~2 kHz -> back (triangular in log-f).
               TPT structure is unconditionally stable for cutoff sweeps.
      Rung 1 — mute: 2 ms to a SEVERITY-SCALED ceiling (20 ms at severity 0,
               150 ms from ~0.6 up — v1.6.0), 1 ms gain ramps unless
               HARD_EDGES, with a residual synthesized tick at mute start.
      Rung 2 — buffer loop: the read head loops a CD_SEGMENT-ms window at
               EXACT intervals; a restart chirp fires at every boundary; the
               boundary jump is crossfaded by the ReadHead — a 0.5 ms HARD
               SPLICE, not the 3 ms default (v1.6.0) — unless HARD_EDGES. Above
               severity kSectorSeverity the window is quantised to the CD
               sector quantum, 1/75 s. On release the head recovers by jumping
               FORWARD toward writeAbs - minLag via the choke point.
      Rung 2t— servo seek, the terminal stage above severity kSeekSeverity
               (v1.6.0): the loop does not recover instantly, it goes FULLY
               SILENT for 100-400 ms first — buffer dry, sled hunting — and
               only then takes the recovery jump, with a re-lock chirp.

    State policy: Conceal/Mute/Seek are duration-bounded and always finish
    naturally — a new win or a release while they run does not abort them (an
    abort would step the filter/gain discontinuously). Loop persists while CD
    keeps winning ticks ("repeat count derives from state duration") and while
    the capture ring can still afford another pass — each pass ages the head by
    one segment, so the wrap is gated on the same lag budget the vinyl locked
    groove uses, and a loop that exhausts it self-releases through the recovery
    jump (v1.3.0). release() performs that same jump.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CaptureRing.h"
#include "ReadHead.h"
#include "RngBank.h"
#include "ArtifactSynth.h"

class CDSkip
{
public:
    // Severity above which a loop window is quantised to the CD sector
    // quantum (brief item 14c). Real anti-shock loops land on sector
    // boundaries; the audible signature is the 75 Hz-family buzz that comes
    // with them. Implicit rather than a toggle: it belongs to "how broken is
    // the disc", which is exactly what CD_SEVERITY already means.
    static constexpr double kSectorSeverity = 0.5;
    static constexpr double kSectorHz       = 75.0;   // 1/75 s = 13.33 ms

    // Severity above which a loop RELEASE goes through the servo-seek stage
    // instead of recovering instantly (brief item 18).
    static constexpr double kSeekSeverity  = 0.85;
    static constexpr double kSeekMinSeconds = 0.100;
    static constexpr double kSeekMaxSeconds = 0.400;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        const juce::dsp::ProcessSpec spec { fs, 512u, 2u };
        concealFilter.prepare (spec);
        concealFilter.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);

        fMax     = juce::jmin (20000.0, 0.45 * fs);
        muteStep = 1.0f / static_cast<float> (juce::jmax (1.0, 0.001 * fs));   // 1 ms ramp

        reset();
    }

    void reset() noexcept
    {
        state          = State::Idle;
        muteGain       = 1.0f;
        eventT         = 0;
        eventDur       = 0;
        loopEndAbs     = 0.0;
        segmentSamples = 0;
        seekDurSamples = 0;
        concealFilter.reset();
    }

    bool isActive() const noexcept  { return state != State::Idle; }
    bool isLooping() const noexcept { return state == State::Loop; }

    // Called from Arbitration when the cd family wins a tick. Consumes 1 cd
    // draw (rung) always, +1 cd draw (duration) for conceal/mute, +1 cd draw
    // (seek duration) for a loop ENTRY above kSeekSeverity, +1 artifactSynth
    // draw for the mute tick.
    void onWin (RngBank& rng, double severity, double segmentMs,
                ReadHead& head, const CaptureRing& ring, bool hardEdges,
                ArtifactSynth& art) noexcept
    {
        const float  r         = rng.get (RngBank::cd).nextFloat();
        const double rungFloat = juce::jlimit (0.0, 2.999,
                                               severity * 3.0
                                                   + (static_cast<double> (r) - 0.5) * 1.5);
        const int rung = static_cast<int> (rungFloat);

        // A bounded event is still running: let it finish (aborting a
        // mid-sweep filter, a mute ramp or a servo seek would be a
        // discontinuity, not a concealment). The rung draw is already
        // consumed — deterministic.
        if (state == State::Conceal || state == State::Mute || state == State::Seek)
            return;

        if (rung == 2)
        {
            if (state == State::Loop)
                return;                                    // extend the running loop

            segmentSamples = juce::jmax ((juce::int64) 16,
                                         (juce::int64) juce::roundToIntAccurate (segmentMs * 0.001 * fs));

            // Sector lock (v1.6.0, brief item 14c). CD_SEGMENT is a free
            // 10-400 ms; a real anti-shock buffer loops whole sectors, so
            // above kSectorSeverity the window snaps to the nearest multiple
            // of fs/75. At 100 ms / 48 kHz that is 4800 -> 5120 samples, and
            // the family of repeat rates it produces is the 75 Hz-related buzz
            // the artifact is known by. Below the threshold the free value is
            // used verbatim — a lightly damaged disc has not fallen back on
            // whole-sector re-reads yet.
            if (severity > kSectorSeverity)
            {
                const double      sector = fs / kSectorHz;
                const juce::int64 n      = juce::jmax (
                    (juce::int64) 1,
                    (juce::int64) juce::roundToIntAccurate (static_cast<double> (segmentSamples) / sector));

                segmentSamples = juce::jmax ((juce::int64) 16,
                                             (juce::int64) juce::roundToIntAccurate (static_cast<double> (n) * sector));
            }

            // Arm the servo seek for THIS loop's eventual release (item 18).
            // The duration is drawn here, at loop entry, rather than at
            // release: the two paths that end a loop are release() and the
            // lag-budget self-release inside processSample, and neither has an
            // RngBank — the harness's block-size invariance depends on RNG
            // being consumed only at ticks and at deterministic jump instants
            // (pattern_rng_stream_interleave_blocksize). The draw is skipped
            // entirely at or below kSeekSeverity, so the cd stream's pattern —
            // and every render made before this feature existed — is
            // bit-identical there.
            if (severity > kSeekSeverity)
            {
                const float rs = rng.get (RngBank::cd).nextFloat();
                seekDurSamples = juce::jmax (1, static_cast<int> (
                    (kSeekMinSeconds + (kSeekMaxSeconds - kSeekMinSeconds) * (double) rs) * fs));
            }
            else
            {
                seekDurSamples = 0;
            }

            const double pos = head.getPosition();
            loopEndAbs = pos;
            head.clampAndScheduleJump (pos - static_cast<double> (segmentSamples),
                                       ring.getTotalWritten(), hardEdges,
                                       head.getSpliceFadeSamples());
            art.triggerChirp();
            state = State::Loop;
            return;
        }

        // Leaving a loop for a different rung: recover first. Deliberately
        // NOT through the servo seek — the CD family is still winning ticks,
        // so this is the disc changing its failure mode, not the terminal
        // release the seek dramatises. Inserting 100-400 ms of silence here
        // would also swallow the very rung this call is installing.
        if (state == State::Loop)
            recoveryJump (head, ring, hardEdges);

        const float r2 = rng.get (RngBank::cd).nextFloat();

        if (rung == 0)
        {
            eventDur = juce::jmax (1, static_cast<int> ((0.030 + 0.050 * (double) r2) * fs));
            eventT   = 0;
            concealFilter.reset();
            concealFilter.setCutoffFrequency (static_cast<float> (fMax));
            state = State::Conceal;
        }
        else
        {
            // Severity-scaled mute ceiling (v1.6.0, brief item 14b). A real
            // E32 mute lengthens as the disc worsens; pinning it at 2-20 ms
            // made the whole rung sound the same at every setting. The span
            // (not the 2 ms floor) scales: 18 ms at severity 0 — the literal
            // v1.5.0 expression, bit-for-bit, since 0.130 * 0.0 is exactly
            // 0.0 — reaching 148 ms by the severity where the loop rung takes
            // over, for a 150 ms ceiling.
            const double spanSec = 0.018
                                   + 0.130 * juce::jlimit (0.0, 1.0, severity / 0.6);

            eventDur = juce::jmax (1, static_cast<int> ((0.002 + spanSec * (double) r2) * fs));
            eventT   = 0;
            art.triggerTick (rng.get (RngBank::artifactSynth));
            state = State::Mute;
        }
    }

    // No cd win this tick (or family disabled mid-event): the loop recovers
    // by jumping forward toward the write head; bounded rungs finish alone.
    void release (ReadHead& head, const CaptureRing& ring, bool hardEdges) noexcept
    {
        if (state == State::Loop)
            endLoop (head, ring, hardEdges);
    }

    // Per-sample, AFTER ReadHead::renderSample (the loop-wrap check needs the
    // advanced position so the jump lands before the next read).
    void processSample (ReadHead& head, const CaptureRing& ring, bool hardEdges,
                        ArtifactSynth& art, float& left, float& right) noexcept
    {
        switch (state)
        {
            case State::Conceal:
            {
                const double x   = static_cast<double> (eventT) / static_cast<double> (juce::jmax (1, eventDur));
                const double tri = 1.0 - std::abs (2.0 * x - 1.0);            // 0 -> 1 -> 0
                const double cut = fMax * std::pow (2000.0 / fMax, tri);      // log-f sweep

                concealFilter.setCutoffFrequency (static_cast<float> (cut));
                left  = concealFilter.processSample (0, left);
                right = concealFilter.processSample (1, right);

                if (++eventT >= eventDur)
                    state = State::Idle;
                break;
            }

            case State::Mute:
                if (++eventT >= eventDur)
                    state = State::Idle;
                break;

            case State::Seek:
                // Servo seek (v1.6.0, brief item 18). The output has been
                // fully muted since entry; when the timer expires the sled
                // finds the data again — the recovery jump this stage delayed,
                // plus the re-lock chirp. State goes Idle in the same sample,
                // so the mute ramp below starts releasing immediately and the
                // chirp (added on the artifact bus DOWNSTREAM of the mute
                // multiply) is audible over it.
                if (++eventT >= eventDur)
                {
                    recoveryJump (head, ring, hardEdges);
                    art.triggerChirp();
                    state = State::Idle;
                }
                break;

            case State::Loop:
                if (head.getPosition() >= loopEndAbs)
                {
                    // Lag budget, mirroring VinylTransport's locked-groove
                    // room gate (v1.3.0). Every pass ages the head by one
                    // segment; when the ring can no longer carry another,
                    // RELEASE through the intentional forward recovery jump
                    // instead of wrapping again. Until now there was no gate
                    // here at all: the loop kept wrapping until ReadHead's
                    // lag-overflow clamp teleported the head forward while
                    // this state machine still read Loop, and the next wrap
                    // re-jumped from the teleported position. Recovery is now
                    // a decision this family makes and owns.
                    const juce::int64 tw  = ring.getTotalWritten();
                    const double      lag = head.getLag (tw);

                    if (lag + static_cast<double> (segmentSamples)
                            <= head.getMaxLag() - 0.05 * fs)
                    {
                        head.clampAndScheduleJump (head.getPosition() - static_cast<double> (segmentSamples),
                                                   tw, hardEdges,
                                                   head.getSpliceFadeSamples());
                        art.triggerChirp();
                    }
                    else
                    {
                        endLoop (head, ring, hardEdges);
                    }
                }
                break;

            case State::Idle:
            default:
                break;
        }

        // Mute gain, 1 ms linear ramps (instant under HARD_EDGES). Settled at
        // exactly 1.0f while idle, so the multiply is bit-exact (FUNC-02).
        const float target = (state == State::Mute || state == State::Seek) ? 0.0f : 1.0f;
        if (hardEdges)
            muteGain = target;
        else if (muteGain < target)
            muteGain = juce::jmin (target, muteGain + muteStep);
        else if (muteGain > target)
            muteGain = juce::jmax (target, muteGain - muteStep);

        left  *= muteGain;
        right *= muteGain;
    }

private:
    enum class State { Idle, Conceal, Mute, Loop, Seek };

    void recoveryJump (ReadHead& head, const CaptureRing& ring, bool hardEdges) noexcept
    {
        // Skip-ahead: forward toward writeAbs - minLag (minLag = 0 here; the
        // choke point clamps). Default 3 ms fade — this one is the player
        // recovering, not the artifact.
        head.clampAndScheduleJump (static_cast<double> (ring.getTotalWritten() - 1),
                                   ring.getTotalWritten(), hardEdges);
    }

    // THE loop exit. Both terminal paths — release() and the lag-budget
    // self-release — come through here so the servo-seek rung cannot be
    // reachable from one and not the other.
    void endLoop (ReadHead& head, const CaptureRing& ring, bool hardEdges) noexcept
    {
        if (seekDurSamples > 0)
        {
            // Terminal stage: mute NOW, jump later. seekDurSamples was drawn
            // at loop entry (see onWin) and is non-zero only above
            // kSeekSeverity.
            eventT   = 0;
            eventDur = seekDurSamples;
            state    = State::Seek;
            return;
        }

        recoveryJump (head, ring, hardEdges);
        state = State::Idle;
    }

    State  state = State::Idle;
    double fs    = 48000.0;
    double fMax  = 20000.0;

    juce::dsp::FirstOrderTPTFilter<float> concealFilter;

    int   eventT   = 0;
    int   eventDur = 0;
    float muteGain = 1.0f;
    float muteStep = 1.0f;

    double      loopEndAbs     = 0.0;
    juce::int64 segmentSamples = 0;

    // Servo-seek duration for the running loop's eventual release, drawn at
    // loop entry. 0 means "this loop recovers instantly" — the only state
    // below kSeekSeverity.
    int seekDurSamples = 0;
};
