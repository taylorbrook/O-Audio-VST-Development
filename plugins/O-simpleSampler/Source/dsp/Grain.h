/*
   This file is part of O-simpleSampler, an Ouaricon Audio plugin.
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

    O-simpleGrain - Grain POD

    A single grain's state. A preallocated std::array<Grain, kMaxGrainsPerVoice>
    lives in every GrainVoice; grains are spawned (find-inactive / steal-oldest)
    and advanced per sample by the overlap-add render loop. Never heap-allocated
    on the audio thread.

    Forward-phase model (ARCHITECTURE §Core Components), NOT GrainScatter's
    samplesRemaining countdown: each sample we read the static source at readPos,
    then readPos += rate; phase += phaseInc; ++age. A grain is done when phase>=1.

    Fields per RESEARCH §2.1.

  ==============================================================================
*/

#pragma once

struct Grain
{
    bool   active        = false;
    float  readPos       = 0.0f;   // absolute fractional position in the SOURCE buffer (samples)
    float  rate          = 1.0f;   // read increment = voiceRate * 2^((grainPitch + spray)/12)
    float  phase         = 0.0f;   // window phase 0..1
    float  phaseInc      = 0.0f;   // = 1.0f / lengthSamples
    float  lengthSamples = 0.0f;   // grainSize ms * fs
    float  pan           = 0.5f;   // 0 = L .. 1 = R (equal-power) — kept for the viz tap
    int    shape         = 4;      // window LUT index (0=rect .. 4=Hann)
    int    age           = 0;      // ++ per sample — for steal-oldest

    // Precomputed equal-power pan gains (pan is fixed for a grain's life, so the
    // cos/sin are evaluated ONCE on spawn — never per sample in the render loop).
    float  panL          = 0.70710677f;   // cos(pan*π/2)
    float  panR          = 0.70710677f;   // sin(pan*π/2)

    // Anti-aliasing one-pole (per grain). The cutoff depends only on `rate`, which
    // is constant for the grain, so the smoothing coefficient is precomputed ONCE
    // on spawn (no per-sample std::exp in the render loop). `aaEngaged` is true only
    // for up-transposed grains (rate > 1); otherwise the read is passed through.
    // `aaState` is primed to the first read sample on spawn.
    float  aaCoeff       = 0.0f;
    bool   aaEngaged     = false;
    float  aaState       = 0.0f;
};
