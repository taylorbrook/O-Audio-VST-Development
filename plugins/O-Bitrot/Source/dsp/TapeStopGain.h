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

    O-Bitrot - TapeStopGain (v1.4.0, improvement brief item 11)

    A tape head is a dPhi/dt transducer: its output is proportional to tape
    SPEED, and the high end dies first because the reproduce cutoff scales
    with speed at fixed recorded wavelength. Decks mute at transport stop.

    Before this, a tape stop ramped the read rate to exactly 0 and the read
    head then re-read one held sample forever at FULL amplitude — a DC step
    fed straight into Codec and Crush. The stop sounded like a freeze, not
    like a deck dying.

      g   = (rate / kRateThreshold) ^ kAlpha,   exactly 0 at rate 0
      cut = fMax * rate / kRateThreshold,       floored at kMinCutoffHz
      the filter is blended in by (1 - rate/kRateThreshold) — see processSample

    WHY AN ARM LATCH AND NOT JUST A RATE TEST
    -----------------------------------------
    Rate alone cannot tell a stop from a bend: the interval table's 0.5x
    down-bend also sits below the threshold, and attenuating it by 6 dB would
    silently rewrite the tape family's melodic voice. So the law is ARMED by a
    stop being installed and disarmed once the rate has recovered back through
    the threshold (or the whole event chain lands on Idle). The threshold
    itself is what keeps the law continuous at both ends: at rate ==
    kRateThreshold the gain is exactly 1 and the cutoff exactly fMax, so
    arming, disarming and the bypass all meet at the identity — there is no
    edge at which anything steps.

    A bend installed mid-stop therefore rides the gain smoothly back up as its
    ramp crosses the threshold, rather than snapping to full level from the
    floor. That is the reason the latch survives installBend.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class TapeStopGain
{
public:
    // Above this rate the law is the identity, so ordinary bends (and the
    // whole of NORMAL playback) are untouched.
    static constexpr double kRateThreshold = 0.9;

    // Output-vs-speed exponent. 1.0 is the textbook dPhi/dt law; 0.8 holds a
    // little more level through the middle of the ramp, which keeps the stop
    // audible as a pitch-down rather than collapsing to silence immediately.
    static constexpr double kAlpha         = 0.8;

    // The cutoff floor. Below this the gain is already ~0.03, so the filter is
    // only shaping what little is left; the floor exists to keep the TPT
    // coefficient well conditioned at rate -> 0.
    static constexpr double kMinCutoffHz   = 120.0;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        const juce::dsp::ProcessSpec spec { fs, 512u, 2u };
        speedFilter.prepare (spec);
        speedFilter.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);

        fMax = juce::jmin (20000.0, 0.45 * fs);

        reset();
    }

    void reset() noexcept
    {
        armed     = false;
        engaged   = false;
        speedGain = 1.0f;
        speedFilter.reset();
    }

    bool isEngaged() const noexcept { return engaged; }

    /** The speed gain applied to the LAST processed sample; exactly 1.0f
        whenever the law is not applying.

        Published for the tape hiss bed (v1.5.0): hiss is recorded material
        like everything else on the tape, so it has to die with the transport
        too. A bed that kept running at full level through a stop would
        announce that the noise is synthetic. */
    float currentGain() const noexcept { return speedGain; }

    /** Per-sample, on the rendered wet pair, immediately after
        ReadHead::renderSample.

        stopInstalled — TapeTransport::consumeStopInstalled(), the one-shot
                        that fires on the sample a stop is installed.
        tapeIdle      — TapeTransport::isIdle(); disarms a stop that was
                        released before its ramp ever reached the threshold.
        appliedRate   — the transport rate applied to THIS sample.

        Exact no-op while disarmed or above the threshold: neither the gain nor
        the filter is in the path, so the all-off passthrough stays
        bit-transparent (FUNC-02). */
    void processSample (bool stopInstalled, bool tapeIdle, double appliedRate,
                        float& left, float& right) noexcept
    {
        if (stopInstalled)
            armed = true;

        if (tapeIdle)
        {
            armed   = false;
            engaged = false;
        }

        if (! armed)
        {
            speedGain = 1.0f;
            return;
        }

        if (appliedRate >= kRateThreshold)
        {
            speedGain = 1.0f;

            // Recovered through the threshold at gain exactly 1 — the law is
            // done. Guarded on `engaged` so that installing a stop at rate 1.0
            // does not disarm it on its very first sample, before the ramp has
            // had a chance to descend.
            if (engaged)
            {
                engaged = false;
                armed   = false;
            }
            return;
        }

        if (! engaged)
        {
            engaged = true;
            speedFilter.reset();
        }

        const double x   = juce::jmax (0.0, appliedRate) / kRateThreshold;   // 0..1
        const double g   = std::pow (x, kAlpha);
        const double cut = juce::jmax (kMinCutoffHz, fMax * x);

        // The filter is BLENDED in by (1 - x), not switched in at the
        // threshold crossing. Cutoff fMax does NOT make a one-pole
        // transparent: at fMax = 0.45*fs the TPT gain coefficient is
        // G/(1+G) = tan(pi*0.45)/(1+tan(pi*0.45)) ~= 0.79, and at the 0.9*fMax
        // the crossing actually enters at, ~0.70 — so a filter entered with
        // zero state drops 30% of the sample it is handed. Measured as a 0.126
        // step on a 0.5-amplitude sine, i.e. a click at the very moment the
        // stop is supposed to be inaudible. With the blend, x == 1 gives dry
        // == 1 and g == 1: entry, exit and bypass are all the same identity.
        const float wet = static_cast<float> (1.0 - x);
        const float dry = 1.0f - wet;

        speedFilter.setCutoffFrequency (static_cast<float> (cut));

        const float fl = speedFilter.processSample (0, left);
        const float fr = speedFilter.processSample (1, right);

        speedGain = static_cast<float> (g);

        left  = (left  * dry + fl * wet) * speedGain;
        right = (right * dry + fr * wet) * speedGain;
    }

private:
    double fs   = 48000.0;
    double fMax = 20000.0;

    juce::dsp::FirstOrderTPTFilter<float> speedFilter;

    bool  armed     = false;   // a stop was installed and has not recovered
    bool  engaged   = false;   // currently below the threshold, law applying
    float speedGain = 1.0f;    // gain applied last sample; 1.0 when bypassed
};
