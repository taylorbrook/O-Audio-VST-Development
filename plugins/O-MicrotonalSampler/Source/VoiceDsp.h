/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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

    VoiceDsp.h
    Microtonal Sample Engine - leaf DSP helpers for the voice render path
    Ouaricon Audio
    Developer: Taylor Brook

    v1.23.2: extracted from MicrotonalSamplerVoice.cpp's anonymous namespace so
    the pure varispeed-read helpers can be exercised directly by a standalone
    regression test (Source/tests/loop_crossfade_check.cpp) instead of a mirror
    copy. These are `inline` (external linkage, ODR-safe) rather than the old
    `static inline` — the definitions stay header-visible, so the compiler
    inlines them into the render loop identically. Behaviour is byte-for-byte
    unchanged from v1.23.1 EXCEPT the two flagged fixes below:

      * readVariantWithLoop — W9 (REVIEW WR-01): continuous crossfade phase
        reaching 1.0 at the wrap point, replacing the 7/8-capped 8-entry LUT.
      * wrapLoopPosition / computePlayRateForVariant — W11 (REVIEW WR-03):
        finite-safe wrap + hostSR==0 divisor guard.

    Consumed by MicrotonalSamplerVoice.cpp (via `using namespace OMtsVoiceDsp;`)
    and the loop_crossfade_check test. Depends only on juce_core / juce_audio_basics
    + SampleMap.h so it links into the lightweight test executables.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <cmath>
#include <utility>

#include "SampleMap.h"

namespace OMtsVoiceDsp
{
    inline double referenceFrequencyForNote (int midiNote) noexcept
    {
        return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
    }

    // Equal-POWER pan/crossfade law: cos²+sin² = 1, so the summed *power* of
    // the two legs is constant across the fade. Used for the loop-boundary
    // crossfade (readVariantWithLoop below), the velocity-layer crossfade, and
    // the CC-dynamics timbre morph.
    //
    // v1.23.2 (REVIEW IN-01): the deliberate choice is constant-power, NOT
    // constant-gain. For UNCORRELATED content (the general case here — adjacent
    // velocity layers are different takes/dynamics, and CC-morph neighbours are
    // distinct recordings) constant power keeps perceived loudness flat through
    // the fade. The known trade-off: for highly CORRELATED (near phase-coherent)
    // content the two legs sum to cos45+sin45 ≈ 1.414 amplitude at the 50/50
    // point — a ~+3 dB bump. We accept this rather than switch to equal-gain
    // (which would instead DIP ~-3 dB for the uncorrelated common case) because
    // the sampler's layers are essentially never phase-identical. Documented
    // here so a future edit doesn't "fix" the bump and regress the common path.
    inline std::pair<float, float> equalPowerWeights (float x) noexcept
    {
        const float t = juce::jlimit (0.0f, 1.0f, x) * juce::MathConstants<float>::halfPi;
        return { std::cos (t), std::sin (t) };
    }

    inline float cubicInterp (const float* buf, int N, double pos) noexcept
    {
        const int  i      = (int) std::floor (pos);
        const auto offset = (float) (pos - (double) i);

        auto clamp = [N] (int idx) noexcept -> int
        {
            return juce::jlimit (0, N - 1, idx);
        };

        const float y0 = buf[clamp (i - 1)];
        const float y1 = buf[clamp (i)];
        const float y2 = buf[clamp (i + 1)];
        const float y3 = buf[clamp (i + 2)];

        const float halfY0 = 0.5f * y0;
        const float halfY3 = 0.5f * y3;

        return y1 + offset * ((0.5f * y2 - halfY0)
                  + (offset * (((y0 + 2.0f * y2) - (halfY3 + 2.5f * y1))
                  + (offset * ((halfY3 + 1.5f * y1) - (halfY0 + 1.5f * y2))))));
    }

    inline float readVariantWithLoop (const float* buf, int N, double pos,
                                      int lpStart, int lpEnd) noexcept
    {
        if (lpEnd <= 0)
        {
            const double clamped = juce::jmin (pos, (double) (N - 1));
            return cubicInterp (buf, N, clamped);
        }

        const int lpLen = lpEnd - lpStart;
        if (lpLen <= 0)
        {
            const double clamped = juce::jmin (pos, (double) (N - 1));
            return cubicInterp (buf, N, clamped);
        }

        const double fadeStart = (double) (lpEnd - 8);
        if (pos < fadeStart)
            return cubicInterp (buf, N, pos);

        // v1.23.2 (W9 / REVIEW WR-01): drive the 8-sample loop crossfade with a
        // CONTINUOUS phase that reaches 1.0 at the wrap point (pos == lpEnd),
        // instead of the old 8-entry LUT whose largest weight was
        // equalPowerWeights(7/8) → outgoing ≈ 0.195, incoming ≈ 0.981. That
        // residual ~0.195×tail term vanished instantly at the wrap (a per-cycle
        // click on sustained looped samples) and quantized the fade to 8 steps.
        // With x → 1 the outgoing leg → 0, so hand-off to the post-wrap signal
        // is continuous. The cos/sin only fire for the final 8 samples of each
        // loop cycle, so the RT cost over the sustained region is negligible.
        const float x = juce::jlimit (0.0f, 1.0f, (float) ((pos - fadeStart) / 8.0));
        const auto  w = equalPowerWeights (x);

        const float outSample = cubicInterp (buf, N, pos);
        const float inSample  = cubicInterp (buf, N, pos - (double) lpLen + 8.0);

        return outSample * w.first + inSample * w.second;
    }

    inline void wrapLoopPosition (double& pos, int lpStart, int lpEnd) noexcept
    {
        if (lpEnd <= 0) return;
        const int lpLen = lpEnd - lpStart;
        if (lpLen <= 0) return;

        // v1.23.2 (W11 / REVIEW WR-03): a non-finite `pos` (e.g. +Inf produced
        // when a stray playRate divided by a zero host sample rate) would make
        // `while (pos >= lpEnd) pos -= lpLen` spin forever → audio-thread hang.
        // Snap it back to the loop start instead of looping unboundedly.
        if (! std::isfinite (pos))
        {
            pos = (double) lpStart;
            return;
        }

        while (pos >= (double) lpEnd)
            pos -= (double) lpLen;
    }

    inline double computePlayRateForVariant (const SampleVariant& variant,
                                             int                  cellMidiNote,
                                             double               desiredFreq,
                                             double               hostSR) noexcept
    {
        // v1.23.2 (W11 / REVIEW WR-03): guard the divisor. JUCE normally sets a
        // playback sample rate before startNote, but if hostSR is ever 0.0 the
        // old `slotSR / hostSR` produced +Inf, which propagated into pos += Inf
        // and hung wrapLoopPosition (see finite guard above). Clamp to a sane
        // default so the rate stays finite in every path.
        const double sr          = (hostSR > 0.0) ? hostSR : 44100.0;
        const double cellRefFreq = referenceFrequencyForNote (cellMidiNote);
        const double slotSR      = variant.sourceSampleRate > 0.0
                                       ? variant.sourceSampleRate
                                       : sr;
        return (desiredFreq / cellRefFreq) * (slotSR / sr);
    }
}
