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

    SpatialEngine — the Stage-2 DSP seam (DECLARED, NO-OP at Stage 1).

    Stage 1 does NOT call this from processBlock; the pass-through
    (conditioner → FixedDelay<kPartition> → seeded gain) is wired directly in
    PluginProcessor.cpp. The interface exists now so the render harness can
    compile against it today and so Phase 2.1 has a fixed shape to fill:

      encoder (ACN/SN3D, up to order 3) → distance model → rotator
      → always-running binaural (UPOLS, P = kPartition) + stereo decoders
      → 20 ms decoder crossfade.

    Stage-2 handoff: the processor's slice loop (≤ maxBlock per call) is
    already present; replace `delay.process(s)` with a per-slice
    `engine.process(mono, outL, outR, len)`. The host-visible latency stays
    kPartition in both decoder modes at every sample rate (ARCHITECTURE.md
    §Latency) — never change setLatencySamples.

    Headless: <cstddef>, <algorithm> only. No JUCE.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cstddef>

#include "FixedDelay.h"

namespace OSimpleAmbisonics::dsp
{

// The ONE latency constant. Processor (setLatencySamples), harness (P0/P4),
// and Stage 2's UPOLS partition size all read this. 64 samples, fixed, at
// every sample rate and in both decoder modes.
inline constexpr int kPartition = 64;

// The stereo-decoder path delay that time-aligns it with the binaural UPOLS
// path. Stage 1 uses it as the pass-through delay.
using DecoderDelay = FixedDelay<kPartition>;

struct Params
{
    float azimuthDeg   = 0.0f;   // 0 front, +90 left (AmbiX), ±180 behind
    float elevationDeg = 0.0f;   // +90 overhead
    float distanceM    = 1.0f;   // 0.2 … 20 m
    int   order        = 1;      // 1 / 2 / 3
    float yawDeg       = 0.0f;   // scene rotation, + = CCW from above
    float pitchDeg     = 0.0f;   // + = front rises
    float rollDeg      = 0.0f;   // + = left rises
    int   decoder      = 0;      // 0 Binaural, 1 Stereo
    bool  orbit        = false;
    float orbitRateHz  = 0.1f;
};

class SpatialEngine
{
public:
    SpatialEngine() = default;

    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        fs       = sampleRate;
        maxBlock = maxBlockSize;
    }

    void reset() noexcept {}

    void setParams (const Params& p) noexcept { params = p; }

    // Stage 1 stub: writes silence. Stage 2 fills in the real chain.
    void process (const float* monoIn, float* outL, float* outR, int numSamples) noexcept
    {
        (void) monoIn;
        std::fill (outL, outL + numSamples, 0.0f);
        std::fill (outR, outR + numSamples, 0.0f);
    }

    static constexpr int latencySamples() noexcept { return kPartition; }

private:
    double fs       = 48000.0;
    int    maxBlock = 0;
    Params params;
};

} // namespace OSimpleAmbisonics::dsp
