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

    O-Bitrot - ComfortNoise (v1.5.0, improvement brief item 19)

    Comfort noise generation under extended packet concealment.

    PacketLossStage decays every concealment mode toward digital zero, and
    Decay hard-floors to exact silence by the end of the third repetition
    (~60 ms) because that is what real PLC does. What real PLC does NEXT,
    which this engine did not, is fill the hole: G.711 Appendix II CNG and
    GSM's SID frames both substitute low-level spectrally-shaped noise
    matched to the background. That hiss floor is precisely the cue that says
    "the call is still up but dying" — without it a long burst is
    indistinguishable from the far end hanging up.

    SPECTRAL MATCHING
    -----------------
    Two one-pole trackers run on GOOD packets only: a level estimate (RMS over
    ~200 ms) and a tilt estimate — the energy split either side of 1 kHz. The
    bed is white noise split by the SAME 1 kHz corner and recombined with
    weights sqrt(lowShare) and sqrt(highShare), so a dark source gets a dark
    floor and a bright one gets a bright floor.

    Each half is divided by its exact analytic noise gain so the weights mean
    what they say:
        lowpass          y += a * (x - y)   ->  RMS = sqrt(a / (2 - a))
        its complement   x - y              ->  RMS = sqrt(1 - 2a + a/(2-a))
    (the complement's cross term is E[x[n] * y[n]] = a * sigma^2, since only
    the current sample of x appears in y[n] at lag 0). The two halves are not
    orthogonal, so the recombined RMS is approximate rather than exact — but
    both gains track fs, which is the property that matters: the bed does not
    change level between 48 and 96 kHz.

    HONEST LIMIT: the level tracker follows the PROGRAMME, not the background,
    because this stage never sees a speech/silence decision. Sitting the bed
    far below it is what makes that approximation work — a floor at -42 dB
    under the recent programme level reads as room tone in practice.

    LEVEL LAW
    ---------
    PACKET_COMFORT is squared before scaling, so the knob's useful range is
    spread across its travel rather than crowded at the bottom:
        100% -> -30 dB relative,  50% -> -42 dB (the G.711 App II figure),
         25% -> -54 dB,            0% -> exactly, bit-exactly, nothing.

    ADDITIVE, NOT A REPLACEMENT
    ---------------------------
    The bed is added UNDER whatever renderConceal produces, for all four
    concealment modes. Under Decay and Substitute — already at or near
    silence by the time it arrives — that reads as the crossfade the brief
    asks for. Under Repeat, which repeats a packet verbatim forever and never
    decays, replacing the output would dissolve the machine-gun edge that IS
    that mode's identity; the bed sits beneath it instead. Under Silence it is
    all there is.

    On burst end the ramp falls back over one packet, so the bed tails off
    under the returning live signal instead of stopping dead.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MediaNoise.h"      // BedLevel, unitRmsWhite, one-pole helpers

class ComfortNoise
{
public:
    // Bursts shorter than this conceal cleanly on their own; this is also
    // exactly where Decay's -6 dB/repetition ramp reaches its silence floor,
    // so the bed arrives as that mode runs out of signal.
    static constexpr int    kBurstPackets  = 3;          // ~60 ms at 20 ms packets

    static constexpr double kTiltHz        = 1000.0;
    static constexpr double kRmsTauSeconds = 0.200;
    static constexpr double kRampSeconds   = 0.020;      // one packet

    // Relative level at PACKET_COMFORT = 100%, before the squaring law.
    static constexpr float  kMaxRelative   = 0.0316228f; // -30 dB

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        aTilt = onePoleCoeff (kTiltHz, fs);
        aRms  = onePoleCoeff (1.0 / (juce::MathConstants<double>::twoPi * kRmsTauSeconds), fs);

        // Exact analytic noise gains of the split, inverted once here.
        const float lowGain  = onePoleNoiseGain (aTilt);
        const float highGain = std::sqrt (juce::jmax (1.0e-9f,
                                   1.0f - 2.0f * aTilt + aTilt / juce::jmax (1.0e-9f, 2.0f - aTilt)));

        invLowGain  = 1.0f / juce::jmax (1.0e-9f, lowGain);
        invHighGain = 1.0f / highGain;

        ramp.prepare (fs, kRampSeconds);

        reset();
    }

    void reset() noexcept
    {
        ramp.reset();

        lpObs = 0.0f;
        rmsSq = lowSq = highSq = 0.0f;
        nLpL  = nLpR  = 0.0f;
    }

    /** Per-block snapshot: the PACKET_COMFORT knob, 0..1. */
    void setLevel (float comfort01) noexcept
    {
        const float c = juce::jlimit (0.0f, 1.0f, comfort01);
        relative = c * c * kMaxRelative;
    }

    /** Feed one GOOD sample. Called only while the current packet is not
        lost, so the estimate holds through a burst instead of following the
        concealment machinery's own decay down to nothing. */
    void observe (float left, float right) noexcept
    {
        const float mid = 0.5f * (left + right);

        lpObs += aTilt * (mid - lpObs);
        const float hi = mid - lpObs;

        rmsSq  += aRms * (mid   * mid   - rmsSq);
        lowSq  += aRms * (lpObs * lpObs - lowSq);
        highSq += aRms * (hi    * hi    - highSq);
    }

    /** Target state for this sample: true once the burst has run long enough
        for concealment to have given up. */
    void setActive (bool active) noexcept { ramp.setTarget (active ? 1.0f : 0.0f); }

    /** Stereo bed sample. Two unconditional draws — the noise-shaping filters
        run every sample so their state stays continuous between bursts, and
        the draw sequence stays a pure function of the sample count. */
    void renderSample (juce::Random& rng, float& outL, float& outR) noexcept
    {
        const float wl = unitRmsWhite (rng);
        const float wr = unitRmsWhite (rng);

        nLpL += aTilt * (wl - nLpL);
        nLpR += aTilt * (wr - nLpR);

        // A silent source estimates rmsSq == 0 and therefore emits exactly
        // nothing: comfort noise under silence would be the plugin talking,
        // not the media.
        const float g = ramp.next() * relative * std::sqrt (rmsSq);

        if (g == 0.0f)
        {
            outL = 0.0f;
            outR = 0.0f;
            return;
        }

        const float total = lowSq + highSq;
        const float wLow  = total > 0.0f ? std::sqrt (lowSq / total)
                                         : juce::MathConstants<float>::sqrt2 * 0.5f;
        const float wHigh = std::sqrt (juce::jmax (0.0f, 1.0f - wLow * wLow));

        outL = g * (wLow * nLpL * invLowGain + wHigh * (wl - nLpL) * invHighGain);
        outR = g * (wLow * nLpR * invLowGain + wHigh * (wr - nLpR) * invHighGain);
    }

private:
    double fs = 48000.0;

    float aTilt = 0.0f;
    float aRms  = 0.0f;

    float invLowGain  = 1.0f;
    float invHighGain = 1.0f;

    float relative = 0.0f;

    // Observation state (good packets only)
    float lpObs  = 0.0f;
    float rmsSq  = 0.0f;
    float lowSq  = 0.0f;
    float highSq = 0.0f;

    // Noise-shaping state
    float nLpL = 0.0f;
    float nLpR = 0.0f;

    BedLevel ramp;
};
