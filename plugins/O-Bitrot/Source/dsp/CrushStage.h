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

    O-Bitrot - CrushStage + FractionalHoldLatch (Stage 2, Phase 2.4)

    Sample-rate reduction via the fractional-crossing interpolated hold
    (DSP-06/08). Adapted from Airwindows DeRez/DeRez2 (MIT — adaptation
    permitted): `phase += rate`; at a crossing the held value is interpolated
    at the exact sub-sample crossing position,

        pos  = (phase - 1) / step        (how far PAST the crossing we are)
        held = last * pos + input * (1 - pos)

    which kills the hold-period warble of a naive integer latch and makes the
    rate fully sweepable. The phase accumulator is NEVER reset by parameter
    changes (anti-zipper rule 3); CRUSH_RATE is smoothed at the TARGET,
    per-sample (juce::SmoothedValue), runtime-clamped to fs/2.

    CRUSH_JITTER hashes the S&H clock: `phase += rate * (1 + amt * noise)`
    (Decimort-style inharmonic sidebands), noise from the jitter stream.

    FractionalHoldLatch is the SHARED PRIMITIVE CodecStage's fixed 8 kHz grid
    reuses in Phase 2.5 (separate instance, separate state, jitterFactor 1,
    rate = 8000/fs). It is RNG-free — the caller supplies the jitter factor.

    DETERMINISM CONVENTION (same family as PacketLossStage): the latch and
    the jitter-stream draw (exactly ONE per sample) run UNCONDITIONALLY —
    CRUSH_ENABLE only gates audibility through the ~10 ms EnableFade, whose
    exact 0.0 rail keeps the bypass bit-transparent (FUNC-02 regression
    gate). Per-sample unconditional consumption is a pure function of the
    sample count, so same-seed renders are bit-identical at any block size.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RngBank.h"
#include "Arbitration.h"   // EnableFade

//==============================================================================
/** The shared fractional-crossing interpolated hold. One clock, per-channel
    held state (stereo-coherent S&H — it models a converter, not two).
    step = rate * jitterFactor must stay <= 1.0 (rate <= 0.5, factor <= 2). */
class FractionalHoldLatch
{
public:
    void reset() noexcept
    {
        phase = 0.0;
        heldL = heldR = 0.0f;
        lastL = lastR = 0.0f;
    }

    void processSample (float inL, float inR, double rate, double jitterFactor,
                        float& outL, float& outR) noexcept
    {
        const double step = rate * jitterFactor;
        phase += step;

        if (phase >= 1.0)
        {
            phase -= 1.0;

            // Fractional crossing position: 0 = crossing exactly now,
            // 1 = crossing happened a full sample ago.
            const double pos = juce::jlimit (0.0, 1.0, phase / juce::jmax (1.0e-9, step));

            heldL = (float) ((double) lastL * pos + (double) inL * (1.0 - pos));
            heldR = (float) ((double) lastR * pos + (double) inR * (1.0 - pos));
        }

        lastL = inL;
        lastR = inR;
        outL  = heldL;
        outR  = heldR;
    }

private:
    double phase = 0.0;
    float  heldL = 0.0f, heldR = 0.0f;
    float  lastL = 0.0f, lastR = 0.0f;
};

//==============================================================================
class CrushStage
{
public:
    void prepare (double sampleRate, bool initiallyEnabled, double initialRateHz)
    {
        fs = sampleRate;
        rateSmooth.reset (fs, 0.05);                       // 50 ms target glide
        rateSmooth.setCurrentAndTargetValue (initialRateHz);
        enableFade.prepare (fs, initiallyEnabled);
        latch.reset();
    }

    void reset() noexcept
    {
        latch.reset();
        rateSmooth.setCurrentAndTargetValue (rateSmooth.getTargetValue());
    }

    // Per-block snapshot.
    void setParams (float rateHz, float jitter01, bool enabled) noexcept
    {
        rateSmooth.setTargetValue ((double) rateHz);
        jitterAmt = juce::jlimit (0.0, 1.0, (double) jitter01);
        enableFade.setEnabled (enabled);
    }

    // Per-sample. Consumes EXACTLY one jitter-stream draw, unconditionally.
    void processSample (RngBank& rng, float& left, float& right) noexcept
    {
        const float noise = rng.get (RngBank::jitter).nextFloat() * 2.0f - 1.0f;

        const double targetHz = rateSmooth.getNextValue();
        const double rate     = juce::jlimit (500.0, 0.5 * fs, targetHz) / fs;
        const double jf       = juce::jmax (0.0, 1.0 + jitterAmt * (double) noise);

        float pL = left, pR = right;
        latch.processSample (left, right, rate, jf, pL, pR);

        const float g = enableFade.next();
        if (g >= 1.0f)
        {
            left  = pL;
            right = pR;
        }
        else if (g > 0.0f)
        {
            left  = left  * (1.0f - g) + pL * g;
            right = right * (1.0f - g) + pR * g;
        }
        // g == 0: untouched — bit-transparent bypass.
    }

private:
    double fs        = 48000.0;
    double jitterAmt = 0.0;

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> rateSmooth;
    FractionalHoldLatch latch;
    EnableFade          enableFade;
};
