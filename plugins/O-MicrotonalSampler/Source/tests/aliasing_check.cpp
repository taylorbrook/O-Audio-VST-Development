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

    aliasing_check.cpp
    O-MicrotonalSampler — Phase 2.1 Gate 1 RQ-1 manual aliasing measurement.

    Status: SCAFFOLDING (Phase 2.1) — manual run by developer only. Not part of
    automated CI / Gate 1 pass. Excluded from build by default; build with:

        ninja O-MicrotonalSampler_AliasingCheck

    Method (per RESEARCH RQ-1, lines 73-86):
      1. Synthesize a 5 s exponential sine sweep 20 Hz → 24 kHz at host SR.
      2. Run it through the same cubicInterp helper used by the voice once at
         playRate = 1.0 (reference) and once at playRate = 2^(50/1200) ≈ 1.02930.
      3. FFT both outputs; compute the +50c residual after image-bin matching.
      4. Threshold: any alias bin > -60 dBFS above 8 kHz → pre-filter required.

    Action on threshold breach:
      - Add juce::dsp::IIR::Filter<float> antiAlias to MicrotonalSamplerVoice
        (per channel — RESEARCH pitfall #10).
      - juce::dsp::IIR::Coefficients<float>::makeLowPass (hostSR, 22000.0f) in
        prepareToPlay.
      - Run after cubicInterp, before envelope multiply.
      - Re-run this test to confirm ≤ -60 dBFS.

    Implementation: TODO (deferred). The Phase 2.1 commit ships this file as a
    placeholder so CMake registration is in place; the body will be filled in
    when the developer runs the manual measurement.

  ==============================================================================
*/

#include <cstdio>

int main (int /*argc*/, char** /*argv*/)
{
    std::printf ("[O-MicrotonalSampler] aliasing_check: scaffolding — body deferred.\n");
    std::printf ("[O-MicrotonalSampler]   See RESEARCH.md RQ-1 (lines 49-89) for method.\n");
    std::printf ("[O-MicrotonalSampler]   Phase 2.1 assumes 'no pre-filter required' until\n");
    std::printf ("[O-MicrotonalSampler]   measurement proves otherwise.\n");
    return 0;
}
