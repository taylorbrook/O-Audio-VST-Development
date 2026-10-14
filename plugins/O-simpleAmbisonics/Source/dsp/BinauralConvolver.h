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

    BinauralConvolver — the Phase 2.3 HRTF leg: a GRID-SYNCHRONOUS fixed-
    partition uniformly-partitioned overlap-save (UPOLS) convolver of the
    rotated 16-channel bus with the per-order SADIE II MagLS filter banks
    (BinauralFilterData.h), plus the dual-bank order crossfade.

    PROVENANCE. Own transcription of research-2.3/r6_upols_scratch.cpp
    (`Upols`, validated 7.1e-7 / 8.8e-7 against direct-form double at five
    host block sizes and two K's, slicing bit-equal, fade + queue 8.1e-7 —
    RESEARCH-2.3 R6), written from the binaural-decoding deep-dive §4.2–4.4
    and ARCHITECTURE §Fixed-partition UPOLS convolver. No IEM / SPARTA code
    was read (CONTEXT-2.3 D8).

    D1 — GRID-SYNCHRONOUS, NO FIFO. The partition P = kControlGrid = 64 and
    the convolver is stepped from SpatialEngine::controlStep() at gridPos ==
    0: the grid that just ended (in[16][64], filled by push() sample by
    sample) is transformed and the result for the grid that now begins is
    written into out[2][64]. There is no phase counter and no host-sized
    buffer: the output emitted during grid s is the convolution of the input
    of grid s − 1, so the latency is EXACTLY 64 samples by construction and
    out[0 … 63] == 0 on the first grid. kPartition stays the processor's one
    latency constant.

    THE FFT CONTRACT (R6, measured on juce::dsp::FFT (7), N = 128, AppleFFT;
    FFTFallback read): performRealOnlyForwardTransform (buf, true) with the
    time signal in floats [0, 128) and zeros in [128, 256) leaves bins 0 … 64
    INTERLEAVED re / im in floats [0, 130) — DC at [0, 1], Nyquist at
    [128, 129] as an ordinary complex entry with imaginary 0 — unscaled.
    performRealOnlyInverseTransform carries the 1/N and reads bins 0 … 64
    ONLY (bins 65+ are ignored on both engines). So every spectrum here is 130
    floats, the complex MAC runs over 65 bins, and no explicit 1/N or 0.5
    appears anywhere. `onlyCalculateNonNegativeFrequencies = true` on every
    forward transform.

    C3 — RESAMPLING AT fs ≠ 48 000 (prepare only). Each 256-tap header FIR is
    resampled by juce::Interpolators::WindowedSinc at ratio 48000 / fs with a
    prefix of zpre zeros chosen as the smallest zpre ≥ 0 with (100 + zpre)·
    fs/48000 an integer (0 for the 48 k family, 60 for the 44.1 k family, 220
    at 22.05 k); the output is read from delay = (100 + zpre)·fs/48000. The
    interpolator's kernel has a DC gain of 0.990 (−0.087 dB, R5 / F2), so a
    Kronecker delta is resampled first through the identical path, its output
    summed (g_fs), and every FIR is divided by g_fs (resamplerDcGain()). NOTE
    (execute finding, a C3 correction): g_fs = kernel area / ratio — 1.98 at
    96 k, 0.91 at 44.1 k — not ≈ 0.99; the 1/ratio is the point. An
    interpolator preserves the WAVEFORM amplitude, and a filter whose taps
    are merely interpolated to twice the rate has twice the DC gain (+6 dB
    at 96 k). Dividing by the resampled delta's SUM makes Σ taps equal the
    48 k bank's — the frequency response, level match, ILD and the FUNC-05
    reading are then rate-independent. resamplerKernelGain() = g_fs · ratio
    is the ≈ 0.99 of R5. Table
    length L = ceil (256·fs/48000) rounded up to a multiple of 64, K = L / 64:
    44.1 k 256 / 4, 88.2 k and 96 k 512 / 8, 176.4 k and 192 k 1024 / 16,
    22.05 k 128 / 2. At exactly 48 k the header floats are copied unmodified
    (bit-exact table, no resampler call, g = 1.0f).

    D4 — SET-n CHANNEL LOOPS, DUAL-BANK FADE, QUEUE. The 16 input FFTs always
    run (the FDL must stay complete for a later higher order); the accumulate
    loop covers (order + 1)² channels of the active set. An order change
    starts a 20 ms equal-gain linear fade (LinearRamp, exact arrival — the
    2.1 shape) from the current set to the pending one, both convolved during
    the fade; a request during a fade is QUEUED, latest wins, and is applied
    on the arrival step (arrive → requestSet → step, R10). The blend is the A3
    form out + m·(outB − out) with m lerped per sample.

    COST: ≈ 110 kflop per 64 samples at order 3 / K 4 (16 forward FFTs, 16 ×
    4 × 2 complex MACs over 65 bins, 2 inverse FFTs; ×2 MACs + 2 IFFTs during
    a fade). The scalar scratch ran 273× real time at 48 k, 91× at 96 k (R6).

    D6 — DETERMINISM. vDSP (macOS) and FFTFallback (Windows / Linux) run the
    identical bookkeeping; only roundoff differs, so the Binaural render
    digest (digest-2.3) is informational, never a cross-platform verdict. The
    Stereo path does not touch this class — digest-2.1 / 2.2 stay verdicts.

    ALLOCATION: every std::vector is assigned in prepare() only (spectra,
    resampled tables, the FDL — K and L depend on fs, up to K = 16); the
    per-grid state is fixed arrays; juce::dsp::FFT is a member constructed
    once (its constructor allocates — at processor construction, never in
    process). WindowedSinc lives on the stack inside prepare().

    This is the ONE Source/dsp header that includes JUCE (CONTEXT-2.3 D6):
    juce_dsp for the FFT, which pulls juce_audio_basics for the interpolator.
    Inside namespace OSimpleAmbisonics::dsp, `juce::dsp::FFT` is written fully
    qualified — a bare `dsp::` names this namespace.

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <vector>

#include "BinauralFilterData.h"
#include "Smoothers.h"

namespace OSimpleAmbisonics::dsp
{

inline constexpr int kConvPartition  = kControlGrid;        // 64 — the UPOLS partition IS the control grid
inline constexpr int kConvFftOrder   = 7;
inline constexpr int kConvFftSize    = 128;                 // 2 · P
inline constexpr int kConvBins       = 65;                  // P + 1 non-negative bins
inline constexpr int kConvSpecFloats = 130;                 // 65 interleaved re / im
inline constexpr int kConvMaxK       = 32;                  // 384 k → L 2048 (Stage 4 IN-04; documented expectation, not a clamp)
inline constexpr int kNumFilterSets  = 3;                   // orders 1 / 2 / 3
inline constexpr int kConvChannels   = 16;

static_assert (kConvPartition == 64 && kBinauralFilterTaps == 256 && kBinauralFilterSampleRate == 48000,
               "BinauralConvolver is written for P = 64 and the 256-tap 48 k header bank");
static_assert (kConvFftSize == 2 * kConvPartition && kConvBins == kConvPartition + 1 && kConvSpecFloats == 2 * kConvBins,
               "FFT contract constants (R6)");
static_assert ((1 << kConvFftOrder) == kConvFftSize, "FFT order / size");

class BinauralConvolver
{
public:
    BinauralConvolver() = default;

    //==========================================================================
    // prepare (fs): tables, spectra and the FDL for this sample rate. The ONLY
    // place that allocates or runs the resampler. Ends in reset(); the state
    // machine is set by snapToSet() (the engine's snapToParams).
    void prepare (double fs)
    {
        sampleRate = fs;

        if (fs == 48000.0)
        {
            L = kBinauralFilterTaps;
            K = L / kConvPartition;
            zpre = 0;
            delay = 100;
            dcGain = kernelGain = 1.0f;

            for (int order = 1; order <= kNumFilterSets; ++order)
            {
                const int nch = channelsFor (order);
                auto& table = taps[(size_t) (order - 1)];
                table.assign ((size_t) nch * 2 * (size_t) L, 0.0f);

                for (int ch = 0; ch < nch; ++ch)
                    for (int ear = 0; ear < 2; ++ear)
                        std::copy (headerTaps (order, ch, ear), headerTaps (order, ch, ear) + kBinauralFilterTaps,
                                   table.begin() + (std::ptrdiff_t) tapIdx (ch, ear));
            }
        }
        else
        {
            const double ratio = 48000.0 / fs;                       // input samples per output sample

            const int lenExact = (int) std::ceil ((double) kBinauralFilterTaps * fs / 48000.0);
            L = ((lenExact + kConvPartition - 1) / kConvPartition) * kConvPartition;
            K = L / kConvPartition;

            // zpre: the smallest prefix making the interpolator's 100-sample base
            // latency land on an integer output sample (R5).
            zpre = 0;
            for (; zpre <= 48000; ++zpre)
            {
                const double d = (100.0 + (double) zpre) * fs / 48000.0;
                if (std::fabs (d - std::round (d)) < 1.0e-9)
                    break;
            }
            const double delayExact = (100.0 + (double) zpre) * fs / 48000.0;
            jassert (std::fabs (delayExact - std::round (delayExact)) < 1.0e-9);
            delay = (int) std::llround (delayExact);

            int pad  = 400;
            int nIn  = zpre + kBinauralFilterTaps + pad;
            int nOut = (int) std::floor ((double) nIn / ratio);
            while (nOut < delay + L)
            {
                pad  += kConvPartition;
                nIn   = zpre + kBinauralFilterTaps + pad;
                nOut  = (int) std::floor ((double) nIn / ratio);
            }

            std::vector<float> rin ((size_t) nIn, 0.0f), rout ((size_t) nOut, 0.0f);

            // DC calibration FIRST: a Kronecker delta through the identical path (F2).
            rin[(size_t) zpre] = 1.0f;
            {
                juce::Interpolators::WindowedSinc ws;
                ws.reset();
                ws.process (ratio, rin.data(), rout.data(), nOut, nIn, 0);
            }
            double g = 0.0;
            for (float v : rout)
                g += (double) v;
            dcGain     = (float) g;                                  // kernel area / ratio — the divisor
            kernelGain = (float) (g * ratio);                        // ≈ 0.99 (R5)
            jassert (kernelGain > 0.9f && kernelGain < 1.1f);

            for (int order = 1; order <= kNumFilterSets; ++order)
            {
                const int nch = channelsFor (order);
                auto& table = taps[(size_t) (order - 1)];
                table.assign ((size_t) nch * 2 * (size_t) L, 0.0f);

                for (int ch = 0; ch < nch; ++ch)
                    for (int ear = 0; ear < 2; ++ear)
                    {
                        std::fill (rin.begin(), rin.end(), 0.0f);
                        std::copy (headerTaps (order, ch, ear), headerTaps (order, ch, ear) + kBinauralFilterTaps,
                                   rin.begin() + zpre);
                        std::fill (rout.begin(), rout.end(), 0.0f);

                        juce::Interpolators::WindowedSinc ws;
                        ws.reset();
                        ws.process (ratio, rin.data(), rout.data(), nOut, nIn, 0);

                        float* dst = table.data() + tapIdx (ch, ear);
                        for (int i = 0; i < L; ++i)
                            dst[i] = rout[(size_t) (delay + i)] / dcGain;
                    }
            }
        }

        jassert (K >= 1 && K <= kConvMaxK);

        // Spectra: per (ch, ear, k) the 64 taps of partition k, zero-padded to 128, forward.
        for (int order = 1; order <= kNumFilterSets; ++order)
        {
            const int nch = channelsFor (order);
            auto& sp = spec[(size_t) (order - 1)];
            sp.assign ((size_t) nch * 2 * (size_t) K * kConvSpecFloats, 0.0f);
            const float* table = taps[(size_t) (order - 1)].data();

            for (int ch = 0; ch < nch; ++ch)
                for (int ear = 0; ear < 2; ++ear)
                    for (int k = 0; k < K; ++k)
                    {
                        float buf[2 * kConvFftSize] {};
                        std::copy (table + tapIdx (ch, ear) + (std::ptrdiff_t) k * kConvPartition,
                                   table + tapIdx (ch, ear) + (std::ptrdiff_t) (k + 1) * kConvPartition, buf);
                        fft.performRealOnlyForwardTransform (buf, true);
                        std::copy (buf, buf + kConvSpecFloats, sp.begin() + (std::ptrdiff_t) specIdx (ch, ear, k));
                    }
        }

        fdl.assign ((size_t) kConvChannels * (size_t) K * kConvSpecFloats, 0.0f);
        reset();
    }

    // Grid buffers and the FDL zeroed, head restarted. No allocation. The bank
    // state machine is NOT touched here — snapToSet() sets it.
    void reset() noexcept
    {
        std::memset (in,   0, sizeof (in));
        std::memset (prev, 0, sizeof (prev));
        std::memset (out,  0, sizeof (out));
        std::memset (outB, 0, sizeof (outB));
        std::fill (fdl.begin(), fdl.end(), 0.0f);
        head = 0;
    }

    //==========================================================================
    // Bank state machine (R10 / C6). `order` ∈ {1, 2, 3} is the set.

    void snapToSet (int order) noexcept
    {
        cur = order;
        pending = -1;
        queued = -1;
        fading = false;
        bankMix.snapTo (0.0f);
        prevBank = curBank = 0.0f;
    }

    // Arrival handling — FIRST in the control step, before requestSet (R10).
    void arrive() noexcept
    {
        if (fading && curBank == 1.0f)
        {
            cur = pending;
            pending = -1;
            fading = false;
            curBank = prevBank = 0.0f;

            if (queued >= 0)
            {
                const int q = queued;
                queued = -1;
                requestSet (q);
            }
        }
    }

    // Called EVERY control step with params.order: idle & n == cur → no-op;
    // fading → queued = n (latest wins, overwriting).
    void requestSet (int order) noexcept
    {
        if (fading)
        {
            queued = order;
            return;
        }

        if (order != cur)
        {
            pending = order;
            fading = true;
            bankMix.snapTo (0.0f);
            prevBank = curBank = 0.0f;
        }
    }

    // The UPOLS step (R6 block): ramp, 16 input FFTs, accumulate cur (+ pending).
    void step (float rampStep) noexcept
    {
        prevBank = curBank;
        if (fading)
            curBank = bankMix.step (1.0f, rampStep);

        head = (head + K - 1) % K;                               // newest FDL slot

        for (int ch = 0; ch < kConvChannels; ++ch)
        {
            std::copy (prev[ch], prev[ch] + kConvPartition, tmp);
            std::copy (in[ch],   in[ch]   + kConvPartition, tmp + kConvPartition);
            std::fill (tmp + kConvFftSize, tmp + 2 * kConvFftSize, 0.0f);
            std::copy (in[ch], in[ch] + kConvPartition, prev[ch]);  // AFTER forming tmp

            fft.performRealOnlyForwardTransform (tmp, true);
            std::copy (tmp, tmp + kConvSpecFloats, fdl.begin() + (std::ptrdiff_t) fdlIdx (ch, head));
        }

        accumulate (cur, acc, out);

        if (fading)
            accumulate (pending, accB, outB);
    }

    //==========================================================================
    // Per-sample I/O. pos = gridPos + j ∈ [0, 64).

    void push (const float* tap16, int pos) noexcept
    {
        for (int ch = 0; ch < kConvChannels; ++ch)
            in[ch][pos] = tap16[ch];
    }

    // Exactly out[ear][pos] when not fading; the A3 blend with m lerped by t otherwise.
    float outputAt (int ear, int pos, float t) const noexcept
    {
        if (! fading)
            return out[ear][pos];

        const float m = prevBank + t * (curBank - prevBank);
        return out[ear][pos] + m * (outB[ear][pos] - out[ear][pos]);
    }

    //==========================================================================
    // Accessors for the gates.

    // The resampled (or copied) table of one FIR at this rate — tapsAtRate() floats.
    const float* filterTaps (int order, int ch, int ear) const noexcept
    {
        return taps[(size_t) (order - 1)].data() + tapIdx (ch, ear);
    }

    int   tapsAtRate() const noexcept      { return L; }
    int   partitions() const noexcept      { return K; }
    int   resamplerPrefix() const noexcept { return zpre; }
    int   resamplerDelay() const noexcept  { return delay; }
    float resamplerDcGain() const noexcept     { return dcGain; }      // the divisor: Σ resampled delta = kernel area / ratio
    float resamplerKernelGain() const noexcept { return kernelGain; }  // dcGain · ratio ≈ 0.99 (R5), 1.0f at 48 k
    int   activeSet() const noexcept       { return cur; }
    int   pendingSet() const noexcept      { return pending; }
    int   queuedSet() const noexcept       { return queued; }
    bool  isFading() const noexcept        { return fading; }
    float bankMixCur() const noexcept      { return curBank; }

    static constexpr int channelsFor (int order) noexcept { return (order + 1) * (order + 1); }

private:
    static const float* headerTaps (int order, int ch, int ear) noexcept
    {
        switch (order)
        {
            case 1:  return kMagLsFilters1[ch][ear];
            case 2:  return kMagLsFilters2[ch][ear];
            default: return kMagLsFilters3[ch][ear];
        }
    }

    std::ptrdiff_t tapIdx (int ch, int ear) const noexcept
    {
        return (std::ptrdiff_t) (ch * 2 + ear) * L;
    }

    size_t specIdx (int ch, int ear, int k) const noexcept
    {
        return (size_t) (((ch * 2 + ear) * K + k) * kConvSpecFloats);
    }

    size_t fdlIdx (int ch, int slot) const noexcept
    {
        return (size_t) ((ch * K + slot) * kConvSpecFloats);
    }

    // dst[ear] = Σ_ch<(order+1)² Σ_k FDL[ch][(head + k) % K] ⊙ H[order][ch][ear][k], IFFT, copy [64, 128).
    void accumulate (int order, float (&dst)[2][2 * kConvFftSize], float (&dstOut)[2][kConvPartition]) noexcept
    {
        for (int ear = 0; ear < 2; ++ear)
            std::fill (dst[ear], dst[ear] + 2 * kConvFftSize, 0.0f);

        const int    nch = channelsFor (order);
        const float* sp  = spec[(size_t) (order - 1)].data();

        for (int ch = 0; ch < nch; ++ch)
            for (int k = 0; k < K; ++k)
            {
                const float* X = fdl.data() + fdlIdx (ch, (head + k) % K);

                for (int ear = 0; ear < 2; ++ear)
                {
                    const float* H = sp + specIdx (ch, ear, k);
                    float*       A = dst[ear];

                    for (int b = 0; b < kConvBins; ++b)
                    {
                        const float xr = X[2 * b], xi = X[2 * b + 1];
                        const float hr = H[2 * b], hi = H[2 * b + 1];
                        A[2 * b]     += xr * hr - xi * hi;
                        A[2 * b + 1] += xr * hi + xi * hr;
                    }
                }
            }

        for (int ear = 0; ear < 2; ++ear)
        {
            fft.performRealOnlyInverseTransform (dst[ear]);
            std::copy (dst[ear] + kConvPartition, dst[ear] + kConvFftSize, dstOut[ear]);
        }
    }

    // ── state ──────────────────────────────────────────────────────────────
    juce::dsp::FFT fft { kConvFftOrder };                       // constructed once (C14)

    double sampleRate = 48000.0;
    int    L = kBinauralFilterTaps, K = kBinauralFilterTaps / kConvPartition;
    int    zpre = 0, delay = 100;
    float  dcGain = 1.0f, kernelGain = 1.0f;

    std::vector<float> taps[kNumFilterSets];                    // [order-1] → [ch][ear][L]
    std::vector<float> spec[kNumFilterSets];                    // [order-1] → [ch][ear][k][130]
    std::vector<float> fdl;                                     // [ch][slot][130]
    int head = 0;

    float in[kConvChannels][kConvPartition] {};
    float prev[kConvChannels][kConvPartition] {};
    float out[2][kConvPartition] {};
    float outB[2][kConvPartition] {};
    float tmp[2 * kConvFftSize] {};
    float acc[2][2 * kConvFftSize] {};
    float accB[2][2 * kConvFftSize] {};

    int        cur = 1, pending = -1, queued = -1;
    bool       fading = false;
    LinearRamp bankMix;
    float      prevBank = 0.0f, curBank = 0.0f;
};

} // namespace OSimpleAmbisonics::dsp
