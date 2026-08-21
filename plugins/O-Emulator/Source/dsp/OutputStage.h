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
    void prepare (const juce::dsp::ProcessSpec& spec, float lpCutoffHz)
    {
        lp.prepare (spec);
        lp.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
        lp.setCutoffFrequency (lpCutoffHz);

        dc.prepare (spec);
        dc.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
        dc.setCutoffFrequency (10.0f);

        reset();
    }

    void reset()
    {
        lp.reset();
        dc.reset();
    }

    /** DAC LP + gentle soft clip (SNES character). tanh: transparent below
        ~-10 dBFS, rounds the int16-rail peaks the codec domain already
        bounded. */
    float processColor (int channel, float x) noexcept
    {
        return std::tanh (lp.processSample (channel, x));
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

private:
    juce::dsp::FirstOrderTPTFilter<float> lp, dc;
};

} // namespace oemu
