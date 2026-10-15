/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    DecoderMixer — click-free Binaural ↔ Stereo selection with equal latency
    (FUNC-05, DSP-07; ARCHITECTURE §Decoder Mixer).

      stereo leg → FixedDelay<N> × 2  (N = kPartition, time-aligns it with the
                                       binaural UPOLS path, which carries its
                                       64 samples intrinsically)
      mix: LinearRamp, target 1 (Binaural) / 0 (Stereo), stepped at the
           control grid (20 ms, exact arrival), lerped per sample
      out = stereoDelayed + mix·(binaural − stereoDelayed)      ← A3 form

    THE BLEND FORM MATTERS (RESEARCH-2.1 F2): b + mix·(a − b) returns exactly
    b when a == b for any mix. In 2.1 that made the A14 placeholder (the
    binaural input WAS the delayed stereo leg) bit-exact in both decoder
    modes; from Phase 2.3 the placeholder is gone (the binaural leg is the
    BinauralConvolver output) and the A3 form still removes one rounding from
    the live crossfade.

    The API stays split in two because the engine applies the D3 copy at the
    call site: when both mix endpoints are exactly 1 (bothEndpointsAtOne(),
    one bool per grid) the engine copies the convolver output instead of
    blending, so Binaural mode is bit-exact to the convolver and the D7 null
    holds to the float path alone:

      delayStereo (stL, stR, dL, dR)                — run the two delays
      blend (dL, dR, binL, binR, t, outL, outR)      — lerp mix, blend
      bothEndpointsAtOne()                           — prevMix == curMix == 1

    Headless: FixedDelay.h + Smoothers.h only. No JUCE.

  ==============================================================================
*/

#pragma once

#include "FixedDelay.h"
#include "Smoothers.h"

namespace OSimpleAmbisonics::dsp
{

template <int N>
struct DecoderMixer
{
    FixedDelay<N> delayL, delayR;
    LinearRamp    mix;

    void prepare() noexcept { reset(); }

    // Delays zeroed; the engine re-snaps `mix` from Params.
    void reset() noexcept
    {
        delayL.reset();
        delayR.reset();
    }

    void snapTo (int decoder) noexcept
    {
        mix.snapTo (targetFor (decoder));
        prevMix = curMix = mix.cur;
    }

    // One control step: previous endpoint ← current, then advance the ramp.
    void controlStep (int decoder, float rampStep) noexcept
    {
        prevMix = curMix;
        curMix  = mix.step (targetFor (decoder), rampStep);
    }

    void delayStereo (float stL, float stR, float& dL, float& dR) noexcept
    {
        dL = delayL.process (stL);
        dR = delayR.process (stR);
    }

    // t = (j + 1)/64 within the control grid (A5). A3 form throughout.
    void blend (float dL, float dR, float binL, float binR, float t,
                float& outL, float& outR) const noexcept
    {
        const float mixS = prevMix + t * (curMix - prevMix);
        outL = dL + mixS * (binL - dL);
        outR = dR + mixS * (binR - dR);
    }

    // The ramp's current value (published in MeterFrame::decoderMix).
    float currentMix() const noexcept { return curMix; }

    // D3 (Phase 2.3): true when the whole grid sits at mix == 1 — the engine
    // then copies the binaural leg instead of blending.
    bool bothEndpointsAtOne() const noexcept { return prevMix == 1.0f && curMix == 1.0f; }

private:
    static float targetFor (int decoder) noexcept { return decoder == 0 ? 1.0f : 0.0f; }

    float prevMix = 1.0f, curMix = 1.0f;
};

} // namespace OSimpleAmbisonics::dsp
