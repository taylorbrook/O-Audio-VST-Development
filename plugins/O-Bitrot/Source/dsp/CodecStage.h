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

    O-Bitrot - CodecStage (Stage 2, Phase 2.1 skeleton — full codec in 2.5)

    CodecStage owns ALL of the plugin's latency (RESEARCH stage-2 section 3):
    it always presents exactly kCompLatency = ceil(0.020 * fs) samples of
    delay, in every state:

      * disabled / mu-law (Phase 2.1 + 2.5): a plain hand-rolled INTEGER
        delay ring — NOT juce::dsp::DelayLine (push-without-pop shifts the
        delay, and FUNC-02 needs a bit-exact path;
        critical_delayline_push_without_pop_shifts_delay).
      * GSM (Phase 2.5): the latch -> 160-frame accumulate -> encode/decode ->
        hold-out chain REPLACES the delay ring (structural delay is exactly
        0.020 * fs host samples).

    Phase 2.1: the enable/mode/mix parameters are intentionally ignored —
    this is the pure alignment delay the FUNC-02 null probe measures. The
    class keeps the shape it grows into (per-sample stereo processing, state
    prepared once, reset alloc-free).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CodecStage
{
public:
    // The only allocation. delaySamples = kCompLatency for this sample rate.
    void prepare (int delaySamples)
    {
        length = juce::jmax (1, delaySamples);
        delayBuffer.setSize (2, length);
        delayBuffer.clear();
        writeIndex = 0;
    }

    // Alloc-free reset.
    void reset() noexcept
    {
        delayBuffer.clear();
        writeIndex = 0;
    }

    int getDelaySamples() const noexcept { return length; }

    // Per-sample stereo, in place. Phase 2.1: pure integer delay of exactly
    // `length` samples (bit-exact — read old, write new, advance).
    void processSample (float& left, float& right) noexcept
    {
        auto* l = delayBuffer.getWritePointer (0);
        auto* r = delayBuffer.getWritePointer (1);

        const float outL = l[writeIndex];
        const float outR = r[writeIndex];

        l[writeIndex] = left;
        r[writeIndex] = right;

        if (++writeIndex >= length)
            writeIndex = 0;

        left  = outL;
        right = outR;
    }

private:
    juce::AudioBuffer<float> delayBuffer;
    int length     = 1;
    int writeIndex = 0;
};
