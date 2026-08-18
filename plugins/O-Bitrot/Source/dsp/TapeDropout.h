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

    O-Bitrot - TapeDropout (v1.4.0, improvement brief item 2)

    The signature dying-cassette artifact, and the tape family's third event
    kind. Oxide shed and creased tape lift the coating off the head for a few
    milliseconds: the level dips PARTWAY (rarely to silence) and the top end
    goes with it, because head-to-tape spacing loss is frequency dependent —
    the classic "wow, that tape has been played to death" flutter of level.

    The engine already contained this shape. CDSkip's interpolation-conceal
    rung is a triangular log-frequency cutoff sweep (CDSkip.h:170-176); this
    class runs the same sweep and multiplies a triangular gain dip alongside
    it. Both endpoints are then EXACTLY the identity because tri(0) == tri(1)
    == 0, so the event is click-free by construction, with no ramp bookkeeping
    and no HARD_EDGES special case — the same reason the CD conceal rung
    ignores HARD_EDGES too.

    One correction to that shape, though, and it is load-bearing: the filter is
    BLENDED in by `tri` rather than switched in at cutoff fMax. A one-pole at
    fMax is not transparent (see processSample), so "the endpoints are the
    identity" is only true of the gain unless the filter is faded too. The
    unblended form measured a 0.15 output step on a 0.5-amplitude sine.

    A dropout changes no position and no rate, so it composes with everything:
    it is applied to the rendered wet pair after ReadHead::renderSample and
    needs no ring headroom and no ReadHead contract. Notably it installs NO
    rate event, so a bend already in flight keeps ramping underneath it — this
    was the first instance in the engine of the OVERLAY class, and as of v1.9.0
    Arbitration classifies it as one explicitly (improvement brief item 6). The
    practical consequence: a dropout no longer needs the tape family to WIN the
    tick, only to roll one. It now layers under a CD loop or a groove jump the
    same way it always layered under a bend.

    Determinism: triggered only at ticks, drawing exactly 2 values from the
    `tape` stream every time it is called, whether or not the event is
    actually installed (the CDSkip "the rung draw is already consumed"
    convention). A retrigger while one is running is discarded rather than
    restarted — restarting would step the gain from the floor back to 1.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class TapeDropout
{
public:
    // Floor: dips to 10-70% of level. Real dropouts almost never mute — a
    // full mute reads as an edit, not as tape.
    static constexpr double kFloorMin    = 0.10;
    static constexpr double kFloorMax    = 0.70;

    // 5-150 ms: brief oxide gaps through to a full crease.
    static constexpr double kDurMinSec   = 0.005;
    static constexpr double kDurMaxSec   = 0.150;

    // Cutoff at the trough of the sweep — the CD conceal rung's own value.
    static constexpr double kTroughCutHz = 2000.0;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        const juce::dsp::ProcessSpec spec { fs, 512u, 2u };
        dipFilter.prepare (spec);
        dipFilter.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);

        fMax = juce::jmin (20000.0, 0.45 * fs);

        reset();
    }

    void reset() noexcept
    {
        eventT   = 0;
        eventDur = 0;
        floorGain = 1.0;
        dipFilter.reset();
    }

    bool isActive() const noexcept { return eventT < eventDur; }

    /** Tick-time install. ALWAYS consumes exactly 2 draws from the `tape`
        stream so the draw pattern does not depend on whether an event happened
        to be running. */
    void trigger (juce::Random& tapeStream) noexcept
    {
        const double r1 = tapeStream.nextDouble();
        const double r2 = tapeStream.nextDouble();

        if (isActive())
            return;                       // let the running dip finish

        floorGain = kFloorMin + (kFloorMax - kFloorMin) * r1;
        eventDur  = juce::jmax (1, static_cast<int> ((kDurMinSec
                                                      + (kDurMaxSec - kDurMinSec) * r2) * fs));
        eventT    = 0;

        dipFilter.reset();
        dipFilter.setCutoffFrequency (static_cast<float> (fMax));
    }

    /** Per-sample, on the rendered wet pair. Exact no-op while idle — the
        filter is not even in the path, so the all-off passthrough stays
        bit-transparent (FUNC-02). */
    void processSample (float& left, float& right) noexcept
    {
        if (! isActive())
            return;

        const double x   = static_cast<double> (eventT) / static_cast<double> (eventDur);
        const double tri = 1.0 - std::abs (2.0 * x - 1.0);              // 0 -> 1 -> 0
        const double g   = 1.0 + (floorGain - 1.0) * tri;               // 1 -> floor -> 1
        const double cut = fMax * std::pow (kTroughCutHz / fMax, tri);  // log-f sweep

        // The filter is BLENDED in by the same triangle, not switched in.
        //
        // Setting the cutoff to fMax at the endpoints does NOT make a one-pole
        // transparent — at fMax = 0.45*fs the TPT gain coefficient is
        // G/(1+G) = tan(pi*0.45)/(1+tan(pi*0.45)) ~= 0.79, so a filter entered
        // with zero state emits 0.79*x on its first sample. Measured as a 0.15
        // output step on a 0.5-amplitude sine: a click, at the exact instant
        // the event is supposed to be inaudible. Blending by `tri` makes both
        // endpoints the EXACT identity (tri == 0 => the dry term is the whole
        // output and the filter's contribution is multiplied by 0), which is
        // what the "click-free by construction" claim above actually needs.
        const float wet = static_cast<float> (tri);
        const float dry = 1.0f - wet;

        dipFilter.setCutoffFrequency (static_cast<float> (cut));

        const float fl = dipFilter.processSample (0, left);
        const float fr = dipFilter.processSample (1, right);

        left  = (left  * dry + fl * wet) * static_cast<float> (g);
        right = (right * dry + fr * wet) * static_cast<float> (g);

        ++eventT;
    }

private:
    double fs   = 48000.0;
    double fMax = 20000.0;

    juce::dsp::FirstOrderTPTFilter<float> dipFilter;

    int    eventT    = 0;
    int    eventDur  = 0;
    double floorGain = 1.0;
};
