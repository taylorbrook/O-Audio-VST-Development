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

    O-Emulator — ConsoleEngine (Stage 2, Task 6 — skeleton + SNES wiring)

    The shared processing skeleton (ARCHITECTURE "One shared engine
    skeleton"), run inside the fixed-chunk walk:

        AA -> downsample -> drive/clip -> codec round-trip -> upsample
           -> output stage (LP + clip) -> DC blocker

    Phase 2.1 activates the SNES row only (BRR + Gaussian). The per-console
    config table already carries all five rows' domain rates and output
    corners as data; PS1 lands in Phase 2.2, NES/GB/Genesis + the
    ConsoleCrossfader in Phase 2.3, Age/Crush polish in Phase 2.4.

    `crush` wiring (Phase 2.1 scope): drive 0..+12 dB into the codec domain
    and the BRR shift floor (CrushCurve SNES row). `age`/`reverb` are read by
    nobody yet — later phases.

    Control updates happen ONCE PER FIXED CHUNK against a SmoothedValue
    clocked at the chunk rate, so smoothing trajectories are a pure function
    of the absolute chunk index — block-size invariant by construction
    (O-Octagon GainStage control-grid model).

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "BrrCodec.h"
#include "ConsoleResampler.h"
#include "CrushCurve.h"
#include "FixedChunkFeeder.h"
#include "OutputStage.h"

#include <cmath>

namespace oemu
{

/** Per-console pipeline configuration (ARCHITECTURE Pipeline Manager table).
    Only row 0 (SNES) is wired in Phase 2.1; the rest are data awaiting their
    phases. */
struct ConsoleSpec
{
    double rate;         // fixed console-domain rate, Hz
    float outputLpHz;    // output-stage DAC LP corner at host rate
};

class ConsoleEngine
{
public:
    /** The single latency figure — plan decision #2, EXACT formula:

            aaGroupDelayBudget (16 host samples, fixed)
          + kFeederChunk (32)
          + ceil(28 · hostRate / 22050)     worst-case codec block (PS1)
          + ceil( 3 · hostRate / 22050)     Gaussian history, worst console rate

        Constant across console modes (worst case by construction) so mode
        switching never renegotiates PDC. ~116 samples @ 48 kHz, 319 @ 192 kHz
        — under kMaxWetLatencySamples = 1024 with margin. */
    static int computeLatencySamples (double hostRate) noexcept
    {
        constexpr int aaGroupDelayBudget = 16;

        const int codecWorst = (int) std::ceil (28.0 * hostRate / 22050.0);
        const int gaussHist  = (int) std::ceil (3.0 * hostRate / 22050.0);

        return aaGroupDelayBudget + FixedChunkFeeder::getLatencySamples()
             + codecWorst + gaussHist;
    }

    /** @param reportedLatencySamples  the figure handed to setLatencySamples /
        setWetLatency — the engine aligns its structural delay onto it via the
        upsample-ring priming (see ConsoleEngine.cpp). */
    void prepare (double hostRate, int maxBlockSize, int reportedLatencySamples);

    void reset();

    /** Crush target, percent 0–100, latched once per processBlock; consumed
        (smoothed) at fixed-chunk boundaries. Audio thread only. */
    void setCrushPercent (float pct) noexcept { crushTargetPct = pct; }

    /** In-place wet processing of channels 0/1. */
    void process (juce::AudioBuffer<float>& buffer) noexcept;

#if OUARICON_RENDER_HARNESS
    int getChunkPhaseForTest() const noexcept { return feeder.getChunkPhaseForTest(); }
    std::uint64_t getAbsoluteSamplesForTest() const noexcept { return feeder.getAbsoluteSamplesForTest(); }
    int getUpsampleFillForTest() const noexcept { return resampler.getUpFillForTest(); }
#endif

private:
    void processChunk (float* inL, float* inR, float* outL, float* outR) noexcept;

    // ── Latency-alignment estimates (host-domain, documented ±few samples;
    //    the harness xcorr probe budgets ~15 samples on top — L120) ──────────
    static constexpr double kAaGroupDelayEstHost   = 3.0;  // 8th-order Butterworth passband GD
    static constexpr double kDownHoldbackEstHost   = 5.0;  // FIFO residual (−3 margin) + Lagrange base latency
    static constexpr double kGaussHistoryEstConsole = 2.0; // interp point sits ~2 samples behind newest

    static constexpr int kConsoleCap = 128;   // per-chunk console-domain scratch

    static constexpr ConsoleSpec kConsoleSpecs[5] = {
        { 32000.0, 10000.0f },   // SNES     — ACTIVE (Phase 2.1)
        { 22050.0, 12000.0f },   // PS1      — Phase 2.2
        { 33144.0, 14000.0f },   // NES      — Phase 2.3
        { 16384.0,  8000.0f },   // Game Boy — Phase 2.3
        { 26320.0, 12000.0f },   // Genesis  — Phase 2.3
    };

    FixedChunkFeeder feeder;
    ConsoleResampler resampler;
    BrrCodec brr[2];
    OutputStage outputStage;

    /** Chunk-rate drive smoother (~20 ms). Clocked once per fixed chunk. */
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveGain;

    float crushTargetPct = 50.0f;   // parameter-spec default

    float consoleBuf[2][kConsoleCap] {};
};

} // namespace oemu
