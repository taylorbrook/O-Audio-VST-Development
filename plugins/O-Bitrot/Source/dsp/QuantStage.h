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

    O-Bitrot - QuantStage (Stage 2, Phase 2.4)

    Fractional-bit quantizer with TPDF dither and envelope-driven depth
    (DSP-06/07/08):

      * Mid-tread quantize, continuously sweepable bits:
            delta = 2 * exp2(-bits),  out = delta * floor(x/delta + 0.5)
        CRUSH_BITS is smoothed at the TARGET, per-sample (zipper-free by
        construction — the staircase glides one level at a time).
      * TPDF dither: (r1 - r2) * delta * (CRUSH_DITHER / 2) added
        pre-quantize (dither stream; one dither value applied to both
        channels — converter-style, stereo-coherent).
      * Envelope-driven depth (Digitalis framing): a PER-SAMPLE one-pole
        follower (attack ~5 ms, release ~120 ms — a block-rate follower
        breaks offline-bounce invariance), env -> dB -> t over a -60 dB
        floor; bitsNow slides from CRUSH_BITS toward a floor of 1.0 scaled by
        |CRUSH_ENV_AMT|. Sign: + = pump (transients, high env, crush harder);
        - = duck (tails, low env, crush harder).
      * NaN hygiene: the follower input is sanitized (std::isfinite -> 0) and
        the state is range-guarded (a NaN fails the range test and resets to
        0) — the follower can never hold NaN
        (pattern_envelope_follower_state_sticky_nan). The quantizer itself is
        stateless on the audio path, so injected NaN passes through only
        while present and flushes with the delays (QUAL-01 recovery).

    DETERMINISM CONVENTION (same as CrushStage): the follower and the TWO
    dither-stream draws per sample run UNCONDITIONALLY; CRUSH_ENABLE gates
    audibility through the ~10 ms EnableFade with a bit-transparent 0.0 rail.
    Note: quantization at CRUSH_BITS = 16 is NOT bit-transparent on float
    input — the fade rail, not the neutral value, is what preserves FUNC-02.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RngBank.h"
#include "Arbitration.h"   // EnableFade

class QuantStage
{
public:
    void prepare (double sampleRate, bool initiallyEnabled, double initialBits)
    {
        fs = sampleRate;
        atkCoef = 1.0 - std::exp (-1.0 / (0.005 * fs));    // ~5 ms attack
        relCoef = 1.0 - std::exp (-1.0 / (0.120 * fs));    // ~120 ms release
        bitsSmooth.reset (fs, 0.05);                        // 50 ms target glide
        bitsSmooth.setCurrentAndTargetValue (initialBits);
        enableFade.prepare (fs, initiallyEnabled);
        env = 0.0;
    }

    void reset() noexcept
    {
        env = 0.0;
        bitsSmooth.setCurrentAndTargetValue (bitsSmooth.getTargetValue());
    }

    // Per-block snapshot. envAmt01 is signed (-1..+1).
    void setParams (float bits, float ditherLsb, float envAmt01, bool enabled) noexcept
    {
        bitsSmooth.setTargetValue (juce::jlimit (1.0, 16.0, (double) bits));
        ditherHalf  = juce::jlimit (0.0, 1.0, (double) ditherLsb * 0.5);   // 0..2 LSB -> 0..1
        envDepth    = std::abs ((double) envAmt01);
        envPositive = envAmt01 >= 0.0f;
        enableFade.setEnabled (enabled);
    }

    // Per-sample. Consumes EXACTLY two dither-stream draws, unconditionally.
    void processSample (RngBank& rng, float& left, float& right) noexcept
    {
        const float r1 = rng.get (RngBank::dither).nextFloat();
        const float r2 = rng.get (RngBank::dither).nextFloat();

        // Per-SAMPLE one-pole follower, input sanitized, state range-guarded
        // (a NaN state fails the range test — comparisons with NaN are false
        // — and resets to 0; never sticky).
        float x = juce::jmax (std::abs (left), std::abs (right));
        if (! std::isfinite (x))
            x = 0.0f;

        env += (((double) x > env) ? atkCoef : relCoef) * ((double) x - env);
        if (! (env >= 0.0 && env < 1.0e6))
            env = 0.0;

        const double bitsBase = bitsSmooth.getNextValue();
        double bitsNow = bitsBase;

        if (envDepth > 0.0)
        {
            const double envDb = 20.0 * std::log10 (juce::jmax (env, 1.0e-6));
            const double t     = juce::jlimit (0.0, 1.0, (envDb + 60.0) / 60.0);
            const double mod   = envPositive ? t : (1.0 - t);   // pump : duck
            bitsNow = juce::jmax (1.0, bitsBase - (bitsBase - 1.0) * envDepth * mod);
        }

        const double delta = 2.0 * std::exp2 (-bitsNow);
        const float  d     = (float) (delta * ditherHalf) * (r1 - r2);

        const float pL = (float) (delta * std::floor (((double) left  + (double) d) / delta + 0.5));
        const float pR = (float) (delta * std::floor (((double) right + (double) d) / delta + 0.5));

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
    double fs      = 48000.0;
    double atkCoef = 0.01;
    double relCoef = 0.001;
    double env     = 0.0;

    double ditherHalf  = 0.0;
    double envDepth    = 0.0;
    bool   envPositive = true;

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> bitsSmooth;
    EnableFade enableFade;
};
