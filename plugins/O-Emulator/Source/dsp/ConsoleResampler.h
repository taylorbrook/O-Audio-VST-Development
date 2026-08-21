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

    O-Emulator — ConsoleResampler (Stage 2, Task 2 — SNES/Gaussian config)

    Host rate -> console rate -> host rate with authentic character
    (ARCHITECTURE "Console-Domain Resampler"):

      DOWN:  8th-order Butterworth AA (4 cascaded biquads, named Q table,
             plan decision #3) at 0.45·consoleRate, then
             juce::Interpolators::Lagrange decimation. The FIFO is advanced
             by the interpolator's RETURNED consumed count — never a
             precomputed figure — with one CLEARED GUARD SAMPLE past the fill
             (RESEARCH §1.1: the interpolator can over-read ~1 sample; house
             precedent O-MicrotonalSampler SampleLoader).

      UP:    4-tap S-DSP Gaussian at a RUNNING-DOUBLE phase, never reset per
             block (PERF-02 phase continuity). Later phases add the ZOH
             configs (NES/GB/Genesis).

    The upsample ring is PRIMED with zeros at prepare() so the wet path's
    structural delay lands on the reported latency figure (alignment computed
    by ConsoleEngine; the prime count is just handed in here).

    RT safety: all buffers are fixed member arrays; coefficients are assigned
    in prepare() only (ArrayCoefficients — no Ptr factories; the Phase 2.4
    crush AA-open will step between PRECOMPUTED coefficient sets, never
    allocate on the audio thread).

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "FixedChunkFeeder.h"
#include "GaussianInterpolator.h"

namespace oemu
{

class ConsoleResampler
{
public:
    /** Console -> host interpolation character (ARCHITECTURE resampler spec):
        gaussian = S-DSP 4-tap (SNES/PS1, the dark rolloff); zoh = zero-order
        hold (NES/GB/Genesis — the aliasing images ARE the sound; the output
        stage LP then dulls them as the real RC filters did). */
    enum class UpsampleMode { gaussian, zoh };

    /** Upsample ring capacity (power of two). Fill sits near the priming
        count (up to ~55 console samples across modes/rates) ± a few samples
        of production jitter. */
    static constexpr int kUpCap = 256;

    /** @param primeConsoleSamples  zeros pre-queued into the upsample ring —
        the latency-alignment term computed by ConsoleEngine. */
    void prepare (double hostRate, double consoleRate,
                  float aaCutoffHz, int primeConsoleSamples,
                  UpsampleMode upsampleMode);

    void reset();

    /** AA-filters `numHost` samples IN PLACE (l/r are the feeder's chunk
        buffers), appends them to the decimation FIFO, then produces console-
        domain samples into outL/outR. Returns the count produced (identical
        for both channels by construction). */
    int downsample (float* l, float* r, int numHost,
                    float* outL, float* outR, int maxOut) noexcept;

    /** Crush AA-open (Task 18): swap to PRECOMPUTED coefficient set k
        (0 = nominal 0.45·consoleRate). RT-safe: the sets are allocated in
        prepare() and permanently owned by aaSets, so the Ptr swap is pure
        refcounting — no allocation, no free, never a make* factory on the
        audio thread. Call only from a micro-fade trough (the state
        transient is masked by the dip). */
    void setAaOpenIndex (int k) noexcept;

    int getAaOpenIndex() const noexcept { return aaSetIndex; }

    /** Age drift (Task 17): per-chunk resample-ratio factor applied to the
        Lagrange decimation ONLY (production counts stay computed from the
        nominal ratio, so the walk lands in ring fill, which is bounded by
        the engine's drift-offset clamp + the priming floor). 1.0 = exact
        nominal path, bit-identical to no drift. */
    void setDriftRatioFactor (double f) noexcept { driftFactor = f; }

    /** Queue one decoded console-domain stereo sample for the upsampler. */
    void pushConsoleSample (float l, float r) noexcept;

    /** Produce `numHost` host-rate samples through the 4-tap Gaussian. */
    void upsample (float* outL, float* outR, int numHost) noexcept;

#if OUARICON_RENDER_HARNESS
    int getUpFillForTest() const noexcept { return upFill; }
    int getDownFillForTest() const noexcept { return downFill; }
#endif

private:
    /** Butterworth pole Qs for 4 cascaded biquads at one cutoff — the
        8th-order response reproduced allocation-free (plan decision #3;
        derivation Q_i = 1/(2·cos((2i+1)·π/16)), the same pole set JUCE's
        prepare-time-only FilterDesign would produce). */
    static constexpr float kButterworthQ8[4] { 0.50980f, 0.60134f, 0.89998f, 2.56292f };

    // ── Downsample path ──────────────────────────────────────────────────────
    // FIFO capacity: residual fill stays < 3 + hostRate/consoleRate (< 15 at
    // 192 kHz vs the slowest console) + one 32-sample chunk; 192 (+1 guard
    // slot) is comfortable headroom.
    static constexpr int kDownCap = 192;

    /** AA-open coefficient sets: cutoff multipliers per set (set 0 nominal). */
    static constexpr int kNumAaSets = 5;
    static constexpr double kAaOpenMult[kNumAaSets] { 1.0, 1.6, 2.5, 4.0, 6.0 };

    juce::dsp::IIR::Filter<float> aa[2][4];
    juce::dsp::IIR::Coefficients<float>::Ptr aaSets[kNumAaSets][4];
    int aaSetIndex = 0;
    double driftFactor = 1.0;
    juce::Interpolators::Lagrange lagrange[2];
    float downFifo[2][kDownCap + 1] {};   // +1: the cleared guard slot
    int downFill = 0;

    // ── Upsample path ────────────────────────────────────────────────────────
    static_assert ((kUpCap & (kUpCap - 1)) == 0, "ring walk uses & (kUpCap - 1)");

    GaussianInterpolator gauss[2];
    float upRing[2][kUpCap] {};
    int upWrite = 0, upRead = 0, upFill = 0;

    /** ZOH mode's held sample pair (the most recently consumed console
        sample — sample repeat between advances). */
    float zohL = 0.0f, zohR = 0.0f;

    /** Console samples per host sample; the phase accumulator is a running
        double, reset only in prepare()/reset(). */
    double upPhase = 0.0;
    double ratio = 1.5;        // host samples consumed per console sample
    int primeCount = 0;
    UpsampleMode mode = UpsampleMode::gaussian;
};

} // namespace oemu
