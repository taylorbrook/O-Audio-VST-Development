/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

    O-Emulator — OutputStage (Stage 2, Task 5)

    Per-console DAC output model at HOST rate, post-upsample (ARCHITECTURE
    "Output Stage Model" — filters live in one fixed domain so corners never
    shift with the console rate):

        color:    per-console LP (SNES ~10 kHz) -> soft clip
        dcBlock:  10 Hz HP (structural NES-DPCM DC removal; harmless for SNES)

    color and dcBlock are exposed SEPARATELY because Phase 2.4's Age bed
    injects between them (noise/hum ride ON TOP of the output LP, then the DC
    blocker runs last before the mixer — ARCHITECTURE Processing Order 6/7).

    Per-sample TPT recursions need a manual snapToZero() — the engine calls it
    once per fixed chunk (denormal purge; ScopedNoDenormals covers the rest).

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>

#include <cmath>

namespace oemu
{

class OutputStage
{
public:
    /** Per-console clip character (ARCHITECTURE Pipeline Manager table):
        soft = gentle tanh (SNES), hard = int16-rail clamp (PS1/NES/Genesis),
        crunchy = sharp-knee cubic with flat tops (Game Boy). */
    enum class ClipMode { soft, hard, crunchy };

    void prepare (const juce::dsp::ProcessSpec& spec, float lpCutoffHz, ClipMode mode)
    {
        clipMode = mode;
        currentLpHz = lpCutoffHz;

        lp.prepare (spec);
        lp.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
        lp.setCutoffFrequency (lpCutoffHz);

        dc.prepare (spec);
        dc.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
        dc.setCutoffFrequency (10.0f);

        reset();
    }

    /** Age dulling (Task 17): per-chunk corner update, gated on the
        CONTROLLING VALUE — the guard caches the value itself, never an
        enabled flag, so nothing can go stale
        (pattern_conditional_coeff_update_leaks_enabled_flag). TPT structure
        is modulation-safe, so this is just a cheap coefficient refresh. */
    void setLpCutoffHz (float hz) noexcept
    {
        if (! juce::approximatelyEqual (hz, currentLpHz))
        {
            currentLpHz = hz;
            lp.setCutoffFrequency (hz);
        }
    }

    void reset()
    {
        lp.reset();
        dc.reset();
    }

    /** DAC LP + per-console clip. Soft (tanh) is transparent below ~-10 dBFS
        and rounds the int16-rail peaks the codec domain already bounded;
        hard clamps at the rails; crunchy is the classic sharp-knee cubic
        (1.5y − 0.5y³, clamped — flat tops with a harder onset than tanh). */
    float processColor (int channel, float x) noexcept
    {
        const float y = lp.processSample (channel, x);

        switch (clipMode)
        {
            case ClipMode::hard:
                return juce::jlimit (-1.0f, 1.0f, y);

            case ClipMode::crunchy:
                return juce::jlimit (-1.0f, 1.0f, 1.5f * y - 0.5f * y * y * y);

            case ClipMode::soft:
            default:
                return std::tanh (y);
        }
    }

    /** 10 Hz DC blocker — LAST stage before the mixer. */
    float processDcBlock (int channel, float x) noexcept
    {
        return dc.processSample (channel, x);
    }

    /** Once per fixed chunk (TPT filters do not self-purge denormals in
        per-sample use). */
    void snapToZero() noexcept
    {
        lp.snapToZero();
        dc.snapToZero();
    }

#if OUARICON_RENDER_HARNESS
    /** The LIVE DAC-LP corner as prepare() configured it — probe M2 reads
        this from every pipeline (a contract check against the ARCHITECTURE
        table, from the prepared object rather than a mirrored constant). */
    float getLpCutoffForTest() const noexcept { return lp.getCutoffFrequency(); }
#endif

private:
    juce::dsp::FirstOrderTPTFilter<float> lp, dc;
    ClipMode clipMode = ClipMode::soft;
    float currentLpHz = 0.0f;
};

} // namespace oemu
