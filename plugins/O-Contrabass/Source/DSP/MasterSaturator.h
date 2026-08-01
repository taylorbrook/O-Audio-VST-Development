/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    MasterSaturator.h
    O-Contrabass — Master Polynomial Saturator (Phase 2.6a R39a)
    Ouaricon Audio
    Developer: Taylor Brook

    Wet/dry polynomial saturator per RESEARCH §22.2.4 + PLAN rev-13 R39a
    locked design contract. Memoryless cubic waveshaper with Option B
    wet/dry mix:

        out = (1 - a) · in + a · (xClamp − xClamp³ / 3)
        xClamp = jlimit(-1.0, 1.0, in)   // WR-03: ±1.0 keeps f monotonic

    NOTE (WR-03): f(x)=x−x³/3 has f'(x)=1−x² ≤ 0 for |x|>1, so the correct
    clamp for this cubic is ±1.0 (output plateaus at ±2/3). A ±1.5 clamp
    admitted the fold-back region where rising |input| 1.0→1.5 *lowers* the
    wet output — a level dip + inharmonic fold-back on loud peaks.

    True bypass at amount=0; full ARCHITECTURE-spec saturator at amount=1.
    Default amount = 0.5 (50% wet/dry). 30 ms zipper-free SmoothedValue
    ramp on amount.

    Public API:
      - void prepare(double sampleRate)
      - void reset()
      - void setAmount(float amount)              [0, 1]
      - float processSample(float in) noexcept
      - void processBlock(juce::AudioBuffer<float>& buffer)

    Determinism: deterministic SmoothedValue ramp; no juce::Random; no
    cross-block memory beyond the smoother target.
    RT-safety: no allocations, no locks, no I/O.
    Latency:   0 (memoryless polynomial waveshaper).

    Block-API loops getNextValue() once per sample (NOT per channel) so
    L and R stay phase-locked per sample (matches O-Bowed precedent).

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class MasterSaturator
{
public:
    void prepare (double sampleRate)
    {
        amountSmoothed.reset (sampleRate, 0.030);  // 30 ms ramp
    }

    void reset()
    {
        // WR-13: reset(int) binds SmoothedValue::reset(int numSteps) — it sets
        // stepsToTarget, NOT the value, destroying the 30 ms ramp prepare() set.
        // Use setCurrentAndTargetValue to seed the value (matches MasterLimiter::reset).
        amountSmoothed.setCurrentAndTargetValue (0.0f);
    }

    void setAmount (float amount) noexcept
    {
        amountSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, amount));
    }

    // Phase 2.6a-bis Risk #22 — seed both current and target so the smoother
    // does NOT ramp at first processBlock. Used by OCBS_DISABLE_DECORRELATOR
    // bit-equivalence build path; safe to call anytime (no allocation).
    void setAmountImmediate (float amount) noexcept
    {
        amountSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, amount));
    }

    float processSample (float in) noexcept
    {
        const float a      = amountSmoothed.getNextValue();
        const float xClamp = juce::jlimit (-1.0f, 1.0f, in);   // WR-03: ±1.0 monotonic
        const float wet    = xClamp - xClamp * xClamp * xClamp / 3.0f;
        return (1.0f - a) * in + a * wet;
    }

    // Block API — advance the amount smoother ONCE per sample (not per channel)
    // so L and R stay synchronized per-sample. Matches O-Bowed precedent.
    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float a = amountSmoothed.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data        = buffer.getWritePointer (ch);
                const float in     = data[i];
                const float xClamp = juce::jlimit (-1.0f, 1.0f, in);   // WR-03: ±1.0 monotonic
                const float wet    = xClamp - xClamp * xClamp * xClamp / 3.0f;
                data[i]            = (1.0f - a) * in + a * wet;
            }
        }
    }

private:
    juce::SmoothedValue<float> amountSmoothed { 0.0f };
};
