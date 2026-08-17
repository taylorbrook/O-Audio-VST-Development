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
      Rung 1 — mute: 2-20 ms mute (1 ms gain ramps unless HARD_EDGES) with a
               residual synthesized tick at mute start (ArtifactSynth).
      Rung 2 — buffer loop: the read head loops a CD_SEGMENT-ms window at
               EXACT intervals; a restart chirp fires at every boundary; the
               boundary jump is crossfaded by the ReadHead (min ~1 ms) unless
               HARD_EDGES. On release the head recovers by jumping FORWARD
               toward writeAbs - minLag via the choke point.

    State policy: Conceal/Mute are duration-bounded (<= 80 ms) and always
    finish naturally — a new win or a release while they run does not abort
    them (an abort would step the filter/gain discontinuously). Loop persists
    while CD keeps winning ticks ("repeat count derives from state duration")
    and while the capture ring can still afford another pass — each pass ages
    the head by one segment, so the wrap is gated on the same lag budget the
    vinyl locked groove uses, and a loop that exhausts it self-releases
    through the recovery jump (v1.3.0). release() performs that same jump.

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
        concealFilter.reset();
    }

    bool isActive() const noexcept  { return state != State::Idle; }
    bool isLooping() const noexcept { return state == State::Loop; }

    // Called from Arbitration when the cd family wins a tick. Consumes 1 cd
    // draw (rung) always, +1 cd draw (duration) for conceal/mute, +1
    // artifactSynth draw for the mute tick.
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
        // mid-sweep filter or a mute ramp would be a discontinuity, not a
        // concealment). The rung draw is already consumed — deterministic.
        if (state == State::Conceal || state == State::Mute)
            return;

        if (rung == 2)
        {
            if (state == State::Loop)
                return;                                    // extend the running loop

            segmentSamples = juce::jmax ((juce::int64) 16,
                                         (juce::int64) juce::roundToIntAccurate (segmentMs * 0.001 * fs));

            const double pos = head.getPosition();
            loopEndAbs = pos;
            head.clampAndScheduleJump (pos - static_cast<double> (segmentSamples),
                                       ring.getTotalWritten(), hardEdges);
            art.triggerChirp();
            state = State::Loop;
            return;
        }

        // Leaving a loop for a different rung: recover first.
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
            eventDur = juce::jmax (1, static_cast<int> ((0.002 + 0.018 * (double) r2) * fs));
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
        {
            recoveryJump (head, ring, hardEdges);
            state = State::Idle;
        }
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
                                                   tw, hardEdges);
                        art.triggerChirp();
                    }
                    else
                    {
                        recoveryJump (head, ring, hardEdges);
                        state = State::Idle;
                    }
                }
                break;

            case State::Idle:
            default:
                break;
        }

        // Mute gain, 1 ms linear ramps (instant under HARD_EDGES). Settled at
        // exactly 1.0f while idle, so the multiply is bit-exact (FUNC-02).
        const float target = (state == State::Mute) ? 0.0f : 1.0f;
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
    enum class State { Idle, Conceal, Mute, Loop };

    void recoveryJump (ReadHead& head, const CaptureRing& ring, bool hardEdges) noexcept
    {
        // Skip-ahead: forward toward writeAbs - minLag (minLag = 0 here; the
        // choke point clamps).
        head.clampAndScheduleJump (static_cast<double> (ring.getTotalWritten() - 1),
                                   ring.getTotalWritten(), hardEdges);
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
};
