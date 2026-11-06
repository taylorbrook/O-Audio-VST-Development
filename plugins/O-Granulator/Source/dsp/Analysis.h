/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
#pragma once

#include <juce_dsp/juce_dsp.h>   // juce::dsp::FFT (vDSP on macOS: no heap; FFTFallback: alloca 16.4 KB — R76)
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include "EngineConstants.h"

// ============================================================================
// Input analysis (Stage 2 / Phase 2.5b, PLAN-2.5b P61–P69; RESEARCH-2.5b
// R59–R69, R74–R77; CONTEXT-2.5b D17, D20 as amended by the sign-offs R60 /
// R66 / R64).
//
// ONE owner for everything the engine learns from its input:
//   OnsetDetector  edge-triggered transient detector (R60, R75)
//   Decimator      64-tap Blackman sinc, Σh = 1, polyphase at abs % D == 0
//   Mpm            McLeod pitch method on 1024 decimated samples (FFT 2048)
//   OctaveGuard    D20: median-of-3 + asymmetric harmonic-class lookahead
//   PitchTracker   live note, 250 ms hold, Pitch source slew (P68)
//   CellLog        {cellStart, W, live} per cell (R63: ring-domain W)
//   F0Ring         tagged post-guard f0 per hop of the ring (D17, R65)
//   Analysis       the tap, per-piece process → event, the pending hop
//                  (R59: committed at the next grid tick), sources, lookup
//
// Tap (R74): a = 0.5·(L + R)·inGain(i) — the conditioned input, the env
// follower's expression evaluated by the analysis itself (never the ring
// write, the feedback return or the dry path). The analysis ALWAYS runs and
// touches no audio statement: at neutral settings the engine is bit-identical
// to the 2.5a tree (N3).
//
// Block-size invariance: every clock is absolute (int64 sample counter):
// the decimator phase (abs % D), the hop counter (decimated index), the
// refractory (per sample), events (absolute samples). Hop RESULTS are held in
// a one-entry pending slot and committed at the next grid tick (R59), so
// spawns, the Pitch source, telemetry and lookups read state that depends on
// the tick alone. The Onset source is analytic at the tick (R75).
//
// Fixed capacity, no allocation after construction: std::array everywhere;
// the FFT object is constructed with the engine (message thread).
// ============================================================================
namespace ogran
{
    inline constexpr bool kAnalysisTestBuild = OGRAN_TEST_HOOKS != 0;

    //==========================================================================
    /** Every rate-dependent analysis constant (prepare, message thread). */
    struct AnalysisDims
    {
        int    D = 1;            // decimation factor max (1, round (fs/12 k))
        int    Hs = kMpmHop;     // hop in input samples (128·D)
        int    N = 0;            // f0 ring entries ceil (kRingSamples/Hs) + 2
        int    logLen = 0;       // cell log length ceil ((lagNewest + 32)/32) + 2
        int    tauMin = 0;       // floor (fs_d/1500)
        int    tauMax = 0;       // ceil (fs_d/40) + 1 (R68)
        int    hold = 1;         // ceil (0.25·fs/Hs) hops
        double fsd = 12000.0;
        double lagCentre = 0.0;  // window-centre lag 511.5·D + 31.5 (R69)
        double lagNewest = 0.0;  // + 3·Hs: the newest decided entry (R66 uniform lag)

        static AnalysisDims compute (double fs) noexcept
        {
            AnalysisDims d;
            d.D         = std::max (1, static_cast<int> (std::lround (fs / kDecimTargetHz)));
            d.Hs        = kMpmHop * d.D;
            d.fsd       = fs / static_cast<double> (d.D);
            d.N         = static_cast<int> (std::ceil (static_cast<double> (kRingSamples (fs)) / static_cast<double> (d.Hs))) + 2;
            d.tauMin    = static_cast<int> (std::floor (d.fsd / kMpmMaxHz));
            d.tauMax    = static_cast<int> (std::ceil (d.fsd / kMpmMinHz)) + 1;
            d.hold      = static_cast<int> (std::ceil (kPitchHoldS * fs / static_cast<double> (d.Hs)));
            d.lagCentre = (0.5 * static_cast<double> (kMpmWindow) - 0.5) * static_cast<double> (d.D)
                        + 0.5 * static_cast<double> (kDecimTaps - 1);
            d.lagNewest = d.lagCentre + static_cast<double> (kGuardN * d.Hs);
            d.logLen    = static_cast<int> (std::ceil ((d.lagNewest + static_cast<double> (kCell)) / static_cast<double> (kCell))) + 2;
            return d;
        }
    };

    //==========================================================================
    /** R60 (signed off): edge-triggered with a +3 dB re-arm; R75 linear compare.
        Per sample on the tap a:
          TPT one-pole HP 150 Hz: v = (a − s)·G, lp = v + s, s = lp + v, hp = a − lp
          x = |hp|; fast (0.5 / 10 ms), slow (80 / 80 ms) one-poles
          re-arm:  ! armed && fast < 10^(3/20)·slow → armed
          refractory decremented per sample (round (0.05·fs))
          fire:    armed && refr == 0 && fast > thrLin && fast > 10^(6/20)·slow
        Firing disarms. levelTrigger (test hook OD1n) is the ARCH §8 form (no
        armed state: re-fires while the condition holds). */
    class OnsetDetector
    {
    public:
        void prepare (double fs) noexcept
        {
            const double g = std::tan (kFxPi * kOnsetHpHz / fs);
            G   = static_cast<float> (g / (1.0 + g));
            cFa = coeff (kOnsetFastAttMs, fs);
            cFr = coeff (kOnsetFastRelMs, fs);
            cSa = coeff (kOnsetSlowMs, fs);
            cSr = coeff (kOnsetSlowMs, fs);
            refrLen = static_cast<int> (std::lround (kOnsetRefractS * fs));
            reset();
        }

        void reset() noexcept
        {
            s = fast = slow = 0.0f;
            armed = true;
            refr = 0;
        }

        /** One sample. Returns true when an event fires at this sample. */
        inline bool step (float a, float thrLin, bool levelTrigger = false) noexcept
        {
            const float v  = (a - s) * G;
            const float lp = v + s;
            s = lp + v;
            const float hp = a - lp;
            const float x  = std::abs (hp);
            fast += (x > fast ? cFa : cFr) * (x - fast);
            slow += (x > slow ? cSa : cSr) * (x - slow);
            if (! armed && fast < kRearmF * slow)
                armed = true;
            if (refr > 0)
                --refr;
            if ((armed || levelTrigger) && refr == 0 && fast > thrLin && fast > kRatioF * slow)
            {
                refr  = refrLen;
                armed = false;
                return true;
            }
            return false;
        }

        /** Tick check: any non-finite state → zeroed, armed. */
        bool scrub() noexcept
        {
            if (std::isfinite (s) && std::isfinite (fast) && std::isfinite (slow))
                return false;
            reset();
            return true;
        }

        static constexpr float kRatioF = static_cast<float> (kOnsetRatio);
        static constexpr float kRearmF = static_cast<float> (kOnsetRearm);

        float hpState() const noexcept   { return s; }
        float fastEnv() const noexcept   { return fast; }
        float slowEnv() const noexcept   { return slow; }
        bool  isArmed() const noexcept   { return armed; }
        int   refractory() const noexcept { return refr; }
        int   refractoryLength() const noexcept { return refrLen; }
        float hpG() const noexcept { return G; }
        float coeffFastAtt() const noexcept { return cFa; }
        float coeffFastRel() const noexcept { return cFr; }
        float coeffSlow() const noexcept    { return cSa; }
       #if OGRAN_TEST_HOOKS
        void injectStateForTest (float v) noexcept { fast = v; }
       #endif

    private:
        static float coeff (double ms, double fs) noexcept
        {
            return static_cast<float> (1.0 - std::exp (-1.0 / (ms * 0.001 * fs)));
        }
        float G = 0.0f, cFa = 0.0f, cFr = 0.0f, cSa = 0.0f, cSr = 0.0f;
        float s = 0.0f, fast = 0.0f, slow = 0.0f;
        bool  armed = true;
        int   refr = 0, refrLen = 2400;
    };

    //==========================================================================
    /** 64-tap Blackman-windowed sinc at 0.4·fs/D, designed in double and
        normalised to Σh = 1 (NOT juce::dsp::FilterDesign: its WindowedSinc DC
        gain is 0.990). Polyphase: every input sample is pushed, the output is
        computed only at abs % D == 0 (64 MACs per decimated sample). The
        history persists across pieces. y[m] = Σ_k h[k]·x[m·D − k]: group delay
        31.5 input samples. */
    class Decimator
    {
    public:
        void prepare (int D) noexcept
        {
            const double fc = kDecimCutoff / static_cast<double> (D);   // cycles per input sample
            double h[kDecimTaps] {};
            double sum = 0.0;
            for (int k = 0; k < kDecimTaps; ++k)
            {
                const double n = static_cast<double> (k) - 0.5 * static_cast<double> (kDecimTaps - 1);
                const double x = 2.0 * fc * n;
                const double sinc = std::abs (x) < 1.0e-12 ? 1.0 : std::sin (kFxPi * x) / (kFxPi * x);
                const double t = static_cast<double> (k) / static_cast<double> (kDecimTaps - 1);
                const double w = 0.42 - 0.5 * std::cos (2.0 * kFxPi * t) + 0.08 * std::cos (4.0 * kFxPi * t);
                h[k] = 2.0 * fc * sinc * w;
                sum += h[k];
            }
            for (int j = 0; j < kDecimTaps; ++j)
                hr[static_cast<size_t> (j)] = static_cast<float> (h[kDecimTaps - 1 - j] / sum);   // reversed: oldest first
            reset();
        }

        void reset() noexcept
        {
            hist.fill (0.0f);
            idx = 0;
        }

        inline void push (float a) noexcept
        {
            hist[static_cast<size_t> (idx)] = a;
            hist[static_cast<size_t> (idx + kDecimTaps)] = a;
            idx = (idx + 1) & (kDecimTaps - 1);
        }

        /** The FIR output at the newest pushed sample. */
        inline float output() const noexcept
        {
            const float* x = hist.data() + idx;   // x[0] oldest … x[63] newest
            float y = 0.0f;
            for (int j = 0; j < kDecimTaps; ++j)
                y += hr[static_cast<size_t> (j)] * x[j];
            return y;
        }

        float coeff (int k) const noexcept { return hr[static_cast<size_t> (kDecimTaps - 1 - k)]; }   // h[k]
        const std::array<float, 2 * kDecimTaps>& history() const noexcept { return hist; }

    private:
        static_assert ((kDecimTaps & (kDecimTaps - 1)) == 0, "the history mask needs a power-of-two tap count");
        std::array<float, kDecimTaps> hr {};
        std::array<float, 2 * kDecimTaps> hist {};
        int idx = 0;
    };

    //==========================================================================
    /** McLeod pitch method (ARCH §15 / Algorithm §7; R68). One window of
        kMpmWindow decimated samples:
          level = mean square (double) of the window, gate >= 10^(−50/10)
          x = w − mean, zero-padded to 2048; r(τ) = IFFT |FFT x|² (JUCE real FFT)
          m(τ) running difference (double) for τ <= τ_max + 1 only (R68)
          NSDF n(τ) = 2r/m; skip the zero-lag lobe; key maxima of the positive
          regions in [τ_min, τ_max]; the first >= 0.9·max; parabolic refinement;
          clarity = the interpolated peak; confident iff clarity >= 0.8, the
          level gate passes and everything is finite.
          note = 69 + 12·log2 ((fs_d/τ)/440). */
    class Mpm
    {
    public:
        struct Result
        {
            double tau       = 0.0;
            double clarity   = 0.0;
            double level     = 0.0;     // mean square
            float  note      = std::numeric_limits<float>::quiet_NaN();
            bool   confident = false;
            bool   peak      = false;   // a key maximum was found
        };

        void prepare (double fsdIn) noexcept
        {
            fsd    = fsdIn;
            tauMin = static_cast<int> (std::floor (fsd / kMpmMaxHz));
            tauMax = std::min (kMpmWindow - 2, static_cast<int> (std::ceil (fsd / kMpmMinHz)) + 1);
            buf.fill (0.0f);
            xs.fill (0.0f);
            nsdf.fill (0.0);
        }

        /** w: kMpmWindow contiguous decimated samples (oldest first). */
        Result analyse (const float* w) noexcept
        {
            Result res;
            double sum = 0.0, sq = 0.0;
            for (int i = 0; i < kMpmWindow; ++i)
            {
                const double v = static_cast<double> (w[i]);
                sum += v;
                sq  += v * v;
            }
            const double mean = sum / static_cast<double> (kMpmWindow);
            res.level = sq / static_cast<double> (kMpmWindow);

            for (int i = 0; i < kMpmWindow; ++i)
            {
                xs[static_cast<size_t> (i)]  = static_cast<float> (static_cast<double> (w[i]) - mean);
                buf[static_cast<size_t> (i)] = xs[static_cast<size_t> (i)];
            }
            std::fill (buf.begin() + kMpmWindow, buf.end(), 0.0f);
            fft.performRealOnlyForwardTransform (buf.data(), true);
            for (int k = 0; k <= kFftSize / 2; ++k)
            {
                const float re = buf[static_cast<size_t> (2 * k)], im = buf[static_cast<size_t> (2 * k + 1)];
                buf[static_cast<size_t> (2 * k)]     = re * re + im * im;
                buf[static_cast<size_t> (2 * k + 1)] = 0.0f;
            }
            std::fill (buf.begin() + kFftSize + 2, buf.end(), 0.0f);
            fft.performRealOnlyInverseTransform (buf.data());   // buf[τ] = r(τ)

            // m(τ) and the NSDF up to the scan horizon H = τ_max + 1.
            const int H = tauMax + 1;
            double m = 0.0;
            for (int i = 0; i < kMpmWindow; ++i)
                m += static_cast<double> (xs[static_cast<size_t> (i)]) * static_cast<double> (xs[static_cast<size_t> (i)]);
            m *= 2.0;
            for (int t = 0; t <= H; ++t)
            {
                if (t > 0)
                {
                    const double a = static_cast<double> (xs[static_cast<size_t> (t - 1)]);
                    const double b = static_cast<double> (xs[static_cast<size_t> (kMpmWindow - t)]);
                    m -= a * a + b * b;
                }
                nsdf[static_cast<size_t> (t)] = m > 0.0 ? 2.0 * static_cast<double> (buf[static_cast<size_t> (t)]) / m : 0.0;
            }

            // Peak pick (the RESEARCH mpm.py model, horizon H).
            int t = 1;
            while (t < H && nsdf[static_cast<size_t> (t)] > 0.0) ++t;   // the zero-lag lobe
            int peaks[kMaxPeaks] {};
            int numPeaks = 0;
            double mx = 0.0;
            while (t < tauMax)
            {
                while (t < H && nsdf[static_cast<size_t> (t)] <= 0.0) ++t;   // positive-going crossing
                int best = t;
                while (t < H && nsdf[static_cast<size_t> (t)] > 0.0)
                {
                    if (nsdf[static_cast<size_t> (t)] > nsdf[static_cast<size_t> (best)]) best = t;
                    ++t;
                }
                if (best >= tauMin && best <= tauMax && best > 0 && best < H && numPeaks < kMaxPeaks
                    && nsdf[static_cast<size_t> (best)] > 0.0)
                {
                    peaks[numPeaks++] = best;
                    mx = std::max (mx, nsdf[static_cast<size_t> (best)]);
                }
            }
            for (int k = 0; k < numPeaks; ++k)
            {
                const int p = peaks[k];
                if (nsdf[static_cast<size_t> (p)] >= kMpmKeyMax * mx)
                {
                    const double a = nsdf[static_cast<size_t> (p - 1)], b = nsdf[static_cast<size_t> (p)], c = nsdf[static_cast<size_t> (p + 1)];
                    const double den = a - 2.0 * b + c;
                    const double d = den != 0.0 ? 0.5 * (a - c) / den : 0.0;
                    res.tau     = static_cast<double> (p) + d;
                    res.clarity = b - 0.25 * (a - c) * d;
                    res.peak    = true;
                    break;
                }
            }
            const bool finite = std::isfinite (res.tau) && std::isfinite (res.clarity) && std::isfinite (res.level) && res.tau > 0.0;
            res.confident = res.peak && finite && res.clarity >= kMpmClarity && res.level >= kGateMs;
            if (res.peak && finite)
                res.note = static_cast<float> (69.0 + 12.0 * std::log2 ((fsd / res.tau) / 440.0));
            if (! res.confident)
                res.confident = false;
            return res;
        }

        /** After analyse(): r(τ) (buf) and the NSDF scratch. */
        const float*  autocorr() const noexcept { return buf.data(); }
        const double* nsdfData() const noexcept { return nsdf.data(); }
        int getTauMin() const noexcept { return tauMin; }
        int getTauMax() const noexcept { return tauMax; }
        double getFsd() const noexcept { return fsd; }

        static constexpr int    kFftSize = 1 << kMpmFftOrder;
        static constexpr double kGateMs  = 1.0e-5;   // 10^(kMpmGateDb/10)

    private:
        static_assert (kFftSize >= 2 * kMpmWindow, "zero-padded linear autocorrelation");
        static constexpr int kMaxPeaks = 512;
        juce::dsp::FFT fft { kMpmFftOrder };                      // the ONLY FFT (constructed with the engine)
        std::array<float, 2 * kFftSize> buf {};
        std::array<float, kMpmWindow> xs {};
        std::array<double, kMpmWindow> nsdf {};
        double fsd = 12000.0;
        int tauMin = 8, tauMax = 301;
    };

    //==========================================================================
    /** D20 (R66 signed off). Raw hop i (committed) → the centred median
        m_{i−1} over the CONFIDENT raw hops {i − 2, i − 1, i}: 3 → the median,
        2 → the newer one, < 2 → NaN. The decision for median j is made at raw
        hop j + 3 (median j + 2 known): a uniform 3-hop lag.
        Against the accepted note acc:
          NaN → NaN (acc kept); acc expires after `hold` consecutive NaN;
          acc empty, |m_j − acc| <= 0.5, or a non-harmonic move → accept;
          a harmonic-class jump (||m_j − acc| − h| <= 0.5, h ∈ {7.02, 12,
          19.02, 24}) → accept only if m_j, m_{j+1}, m_{j+2} are confident and
          within ±0.5 st of m_j; otherwise output acc (held).
        guardOff (OG2n): raw hop j straight through, same lag. */
    class OctaveGuard
    {
    public:
        struct Decision
        {
            bool         valid = false;    // a decision was made at this push
            std::int64_t index = -1;       // the decided hop j (= raw − 3)
            float        note  = std::numeric_limits<float>::quiet_NaN();
            bool         held  = false;    // a harmonic jump was not confirmed (output = acc)
        };

        void prepare (int holdHops) noexcept
        {
            hold = std::max (1, holdHops);
            reset();
        }

        void reset() noexcept
        {
            for (auto& r : raw) r = { false, 0.0f };
            med.fill (std::numeric_limits<float>::quiet_NaN());
            accValid = false;
            acc = 0.0f;
            nanRun = 0;
        }

        static bool isHarmonic (double d) noexcept
        {
            const double ad = std::abs (d);
            for (double h : kGuardHarm)
                if (std::abs (ad - h) <= kGuardTolSt)
                    return true;
            return false;
        }

        Decision push (std::int64_t i, bool confident, float note, bool guardOff = false) noexcept
        {
            raw[slot (i)] = { confident && std::isfinite (note), note };
            if (i >= 1)
            {
                // centred median m_{i−1}
                float v[3] {};
                int n = 0;
                for (std::int64_t k = std::max<std::int64_t> (0, i - 2); k <= i; ++k)
                    if (raw[slot (k)].conf)
                        v[n++] = raw[slot (k)].note;
                float mj = std::numeric_limits<float>::quiet_NaN();
                if (n == 3)
                {
                    std::sort (v, v + 3);
                    mj = v[1];
                }
                else if (n == 2)
                {
                    mj = v[1];   // collected oldest → newest: the newer one
                }
                med[slot (i - 1)] = mj;
            }

            Decision dc;
            if (i < kGuardN)
                return dc;
            const std::int64_t j = i - kGuardN;
            dc.valid = true;
            dc.index = j;
            if (guardOff)
            {
                const auto& r = raw[slot (j)];
                dc.note = r.conf ? r.note : std::numeric_limits<float>::quiet_NaN();
                return dc;
            }
            const float m0 = med[slot (j)];
            if (std::isnan (m0))
            {
                if (++nanRun >= hold)
                    accValid = false;
                return dc;   // NaN, acc kept
            }
            nanRun = 0;
            const double d = static_cast<double> (m0) - static_cast<double> (acc);
            if (! accValid || std::abs (d) <= kGuardTolSt || ! isHarmonic (d))
            {
                acc = m0;
                accValid = true;
                dc.note = m0;
                return dc;
            }
            bool confirm = true;
            for (int k = 0; k < kGuardN; ++k)
            {
                const float mk = med[slot (j + k)];
                confirm = confirm && ! std::isnan (mk) && std::abs (static_cast<double> (mk) - static_cast<double> (m0)) <= kGuardTolSt;
            }
            if (confirm)
            {
                acc = m0;
                dc.note = m0;
            }
            else
            {
                dc.note = acc;
                dc.held = true;
            }
            return dc;
        }

        bool  hasAccepted() const noexcept { return accValid; }
        float accepted() const noexcept    { return acc; }

    private:
        static constexpr int kHist = 8;
        static std::size_t slot (std::int64_t k) noexcept { return static_cast<std::size_t> (k & (kHist - 1)); }
        struct Raw { bool conf = false; float note = 0.0f; };
        std::array<Raw, kHist> raw {};
        std::array<float, kHist> med {};
        bool  accValid = false;
        float acc = 0.0f;
        int   nanRun = 0;
        int   hold = 24;
    };

    //==========================================================================
    /** P68: the live note (newest decided value), the live confidence (true
        until `hold` decided hops have passed since the last confident one) and
        the Pitch source (once per tick: y += a·(clamp ((note − 24)/84, 0, 1) − y),
        a = 1 − exp (−32/(0.02·fs)); 0 until the first confident hop; the target
        holds while unconfident). The hold is NOT written into the f0 ring (R65). */
    class PitchTracker
    {
    public:
        void prepare (double fs, int holdHops) noexcept
        {
            hold = std::max (1, holdHops);
            a = static_cast<float> (1.0 - std::exp (-static_cast<double> (kCell) / (kPitchSlewS * fs)));
            reset();
        }
        void reset() noexcept
        {
            everConfident = false;
            sinceConfident = 0;
            note = 0.0f;
            target = 0.0f;
            y = 0.0f;
        }

        /** One decided hop (NaN = unconfident). */
        void onDecision (float decided) noexcept
        {
            if (std::isnan (decided))
            {
                if (everConfident && sinceConfident <= hold)
                    ++sinceConfident;
                return;
            }
            everConfident  = true;
            sinceConfident = 0;
            note   = decided;
            target = targetFor (decided);
        }

        static float targetFor (float n) noexcept
        {
            return std::clamp ((n - static_cast<float> (kPitchSrcLo)) / static_cast<float> (kPitchSrcSpan), 0.0f, 1.0f);
        }

        /** Once per tick. */
        float tickSource() noexcept
        {
            y += a * (target - y);
            return y;
        }

        bool  liveConfident() const noexcept { return everConfident && sinceConfident <= hold; }
        float liveNote() const noexcept      { return note; }
        float source() const noexcept        { return y; }
        float slewCoeff() const noexcept     { return a; }
        int   holdHops() const noexcept      { return hold; }
        bool  scrub() noexcept
        {
            if (std::isfinite (y) && std::isfinite (target) && std::isfinite (note))
                return false;
            reset();
            return true;
        }

    private:
        bool  everConfident = false;
        int   sinceConfident = 0;
        float note = 0.0f, target = 0.0f, y = 0.0f, a = 0.0f;
        int   hold = 24;
    };

    //==========================================================================
    /** R63: {cellStart, W at cell start, live} per cell, at the tick (after
        the freeze edges). A freezePending cell is live (the head still moves). */
    class CellLog
    {
    public:
        struct Entry { std::int64_t cellStart = -1; std::int64_t W = 0; bool live = false; };

        void prepare (int len) noexcept
        {
            n = std::clamp (len, 1, kCellLogCap);
            for (auto& e : log) e = {};
        }
        void record (std::int64_t cellStart, std::int64_t W, bool live) noexcept
        {
            log[slotOf (cellStart / kCell)] = { cellStart, W, live };
        }
        /** The entry of the cell holding absolute sample c (nullptr if not logged / overwritten). */
        const Entry* find (std::int64_t c) const noexcept
        {
            if (c < 0) return nullptr;
            const std::int64_t cell = c / kCell;
            const Entry& e = log[slotOf (cell)];
            return e.cellStart == cell * kCell ? &e : nullptr;
        }
        int length() const noexcept { return n; }

    private:
        std::size_t slotOf (std::int64_t cell) const noexcept { return static_cast<std::size_t> (cell % static_cast<std::int64_t> (n)); }
        std::array<Entry, kCellLogCap> log {};
        int n = 1;
    };

    //==========================================================================
    /** D17 via R62 / R63 / R65: one entry per hop of the RING domain,
        [k mod N] = {tag k, post-guard note (NaN = unconfident)}, tags −1 =
        never written. Lookup at a ring position Wq: k = floor (Wq/Hs); newer than
        the newest entry → the newest (R62; strictNewest → miss); tag ≠ k →
        miss 2 (missing / overwritten); NaN → miss 1 (unconfident). */
    class F0Ring
    {
    public:
        enum Miss : int { Hit = 0, Unconfident = 1, Missing = 2, EmptySet = 3, NoMember = 4 };
        struct Entry { std::int64_t tag = -1; float note = std::numeric_limits<float>::quiet_NaN(); };

        void prepare (int entries, int hopSamples) noexcept
        {
            N  = std::clamp (entries, 1, kF0RingCap);
            Hs = std::max (1, hopSamples);
            for (auto& e : ring) e = {};
            kNewest = -1;
        }

        /** Unit form: write slot k directly. */
        void writeSlot (std::int64_t k, float note) noexcept
        {
            if (k < 0) return;
            ring[static_cast<std::size_t> (k % N)] = { k, note };
            kNewest = std::max (kNewest, k);
        }

        /** Write the decided hop centred at absolute sample c. Returns 0 written,
            1 skipped (frozen cell), 2 no log entry. */
        int write (double c, float note, const CellLog& cells) noexcept
        {
            const auto ci = static_cast<std::int64_t> (std::floor (c));
            const auto* e = cells.find (ci);
            if (e == nullptr) return 2;
            if (! e->live)   return 1;
            const double W = static_cast<double> (e->W) + (c - static_cast<double> (e->cellStart));
            writeSlot (static_cast<std::int64_t> (std::floor (W / static_cast<double> (Hs))), note);
            return 0;
        }

        int lookup (double Wq, bool strictNewest, float& f0) const noexcept
        {
            if (kNewest < 0) return Missing;
            std::int64_t k = static_cast<std::int64_t> (std::floor (Wq / static_cast<double> (Hs)));
            if (k > kNewest)
            {
                if (strictNewest) return Missing;
                k = kNewest;
            }
            if (k < 0) return Missing;
            const Entry& e = ring[static_cast<std::size_t> (k % N)];
            if (e.tag != k)  return Missing;
            if (std::isnan (e.note)) return Unconfident;
            f0 = e.note;
            return Hit;
        }

        std::int64_t newest() const noexcept { return kNewest; }
        int size() const noexcept { return N; }
        int hop() const noexcept { return Hs; }
        const Entry& entry (int slot) const noexcept { return ring[static_cast<std::size_t> (slot)]; }

    private:
        std::array<Entry, kF0RingCap> ring {};
        int N = 1, Hs = kMpmHop;
        std::int64_t kNewest = -1;
    };

    //==========================================================================
    /** The analysis owner (P61). Engine::prepare → prepare (fs); per piece
        processPiece (after the env scrub, before scheduleStreams) → the event
        offset in the piece or −1; per tick tick (after handleFreezeEdges,
        before tickSources) → cell log, thrLin, scrub checks, the R59 commit,
        the Pitch / Onset sources. */
    class Analysis
    {
    public:
        struct Stats
        {
            std::int64_t onsetEvents = 0, maxEventsPerPiece = 0;
            std::int64_t mpmHops = 0, mpmConfident = 0, guardHeld = 0;
            std::int64_t f0Writes = 0, f0Skips = 0, f0NoLog = 0;
            std::int64_t scrubs = 0;
        };
        struct Flags
        {
            bool levelTrigger = false, commitHopImmediately = false, guardOff = false, ringHold = false;
        };
        struct Inject { std::int64_t at = -1; float value = 0.0f; bool done = false; };

       #if OGRAN_TEST_HOOKS
        struct EventLogEntry { std::int64_t at = 0; };
        struct HopLogEntry
        {
            std::int64_t index = 0, aHop = 0, commitAt = 0;
            float  rawNote = 0.0f;
            double tau = 0.0, clarity = 0.0, level = 0.0;
            bool   confident = false;
            std::int64_t decided = -1;     // the decided hop index (−1 none)
            float  decidedNote = 0.0f;     // NaN = unconfident
            double decidedCentre = 0.0;    // its window centre (absolute samples)
            int    ringResult = -1;        // F0Ring::write result (−1: no decision)
        };
        static constexpr int kLogSize = 4096;
        std::array<EventLogEntry, kLogSize> eventLog {};
        std::uint64_t eventCount = 0;
        std::array<HopLogEntry, kLogSize> hopLog {};
        std::uint64_t hopCount = 0;
       #endif

        void prepare (double fsIn) noexcept
        {
            fs   = fsIn;
            dims = AnalysisDims::compute (fs);
            jassert (dims.N <= kF0RingCap && dims.logLen <= kCellLogCap && dims.tauMax <= kMpmWindow - 2);
            detector.prepare (fs);
            decim.prepare (dims.D);
            mpm.prepare (dims.fsd);
            guard.prepare (dims.hold);
            tracker.prepare (fs, dims.hold);
            cells.prepare (dims.logLen);
            f0ring.prepare (dims.N, dims.Hs);
            win.fill (0.0f);
            pending = {};
            hopIndex = 0;
            lastEventAbs = -1;
            thrLin = 0.0f;
            onsetSrc = 0.0f;
            pitchSrc = 0.0f;
            stats = {};
            inject = {};
           #if OGRAN_TEST_HOOKS
            eventCount = hopCount = 0;
           #endif
        }

        /** One piece of the conditioned input. Returns the event offset in the
            piece (at most one: the refractory is >= 400 samples > kCell for
            fs >= 8 k), or −1. */
        template <typename GainAt>
        int processPiece (const float* inL, const float* inR, int n, int cellPos, std::int64_t absStart, GainAt&& gainAt) noexcept
        {
            int event = -1, events = 0;
            const bool levelTrig = kAnalysisTestBuild && flags.levelTrigger;
            for (int i = 0; i < n; ++i)
            {
                float a = 0.5f * (inL[i] + inR[i]) * gainAt (cellPos + i);
                const std::int64_t abs = absStart + i;
                if constexpr (kAnalysisTestBuild)
                    if (inject.at >= 0 && ! inject.done && abs == inject.at)
                    {
                        a = inject.value;
                        inject.done = true;
                    }
                if (! (std::abs (a) <= kAnalysisTapMax))   // non-finite or absurd → 0 (P61 sanitiser)
                {
                    a = 0.0f;
                    ++stats.scrubs;
                }

                if (detector.step (a, thrLin, levelTrig))
                {
                    if (event < 0) event = i;
                    ++events;
                    lastEventAbs = abs;
                    ++stats.onsetEvents;
                   #if OGRAN_TEST_HOOKS
                    eventLog[static_cast<std::size_t> (eventCount++ % kLogSize)] = { abs };
                   #endif
                }

                decim.push (a);
                if (abs % dims.D == 0)
                {
                    const std::int64_t m = abs / dims.D;
                    const float y = decim.output();
                    const auto w = static_cast<std::size_t> (m % kMpmWindow);
                    win[w] = y;
                    win[w + kMpmWindow] = y;
                    if ((m + 1) % kMpmHop == 0 && m + 1 >= kMpmWindow)
                        computeHop (m, abs);
                }
            }
            stats.maxEventsPerPiece = std::max<std::int64_t> (stats.maxEventsPerPiece, events);
            return event;
        }

        /** Grid tick at absolute sample T (after handleFreezeEdges): the cell log
            {T, W, live}, thrLin, the detector / tracker scrub checks, the R59
            commit, then the Pitch and Onset sources of this cell. */
        void tick (std::int64_t T, std::int64_t W, bool live, float thresholdDb) noexcept
        {
            cells.record (T, W, live);
            thrLin = static_cast<float> (std::pow (10.0, static_cast<double> (thresholdDb) / 20.0));
            if (detector.scrub()) ++stats.scrubs;
            if (tracker.scrub())  ++stats.scrubs;
            if (pending.valid)
            {
                commit (pending, T);
                pending.valid = false;
            }
            pitchSrc = tracker.tickSource();
            onsetSrc = onsetSourceAt (T);
        }

        /** P64 / R75: exp (−(T − e)/(0.15·fs)) in double → float; 0 before the
            first event and exactly 0 once T − e > 16 τ. */
        float onsetSourceAt (std::int64_t T) const noexcept
        {
            if (lastEventAbs < 0) return 0.0f;
            const double dt = static_cast<double> (T - lastEventAbs);
            if (dt > kOnsetZeroTaus * kOnsetTauS * fs) return 0.0f;
            return static_cast<float> (std::exp (-dt / (kOnsetTauS * fs)));
        }

        float pitchSource() const noexcept  { return pitchSrc; }
        float onsetSource() const noexcept  { return onsetSrc; }
        float liveNote() const noexcept     { return tracker.liveNote(); }
        bool  liveConfident() const noexcept { return tracker.liveConfident(); }

        const AnalysisDims& getDims() const noexcept { return dims; }
        const OnsetDetector& getDetector() const noexcept { return detector; }
        const Decimator& getDecimator() const noexcept { return decim; }
        const PitchTracker& getTracker() const noexcept { return tracker; }
        const F0Ring& getRing() const noexcept { return f0ring; }
        const CellLog& getCells() const noexcept { return cells; }
        const std::array<float, 2 * kMpmWindow>& getWindow() const noexcept { return win; }
        std::int64_t getLastEvent() const noexcept { return lastEventAbs; }
        float getThrLin() const noexcept { return thrLin; }

        Stats  stats;
        Flags  flags;
        Inject inject;

    private:
        struct Pending
        {
            bool         valid = false;
            std::int64_t index = 0, aHop = 0;
            Mpm::Result  r;
        };

        void computeHop (std::int64_t m, std::int64_t abs) noexcept
        {
            const float* w = win.data() + static_cast<std::size_t> ((m + 1) % kMpmWindow);   // [m − 1023, m], oldest first
            Pending p;
            p.valid = true;
            p.index = hopIndex++;
            p.aHop  = abs;
            p.r     = mpm.analyse (w);
            ++stats.mpmHops;
            if (p.r.confident) ++stats.mpmConfident;
            if (kAnalysisTestBuild && flags.commitHopImmediately)
            {
                commit (p, abs);   // the R59 failure class (N9n)
                return;
            }
            jassert (! pending.valid);   // Hs >= 128 > kCell: <= 1 hop per cell
            pending = p;
        }

        void commit (const Pending& p, std::int64_t at) noexcept
        {
            const bool guardOff = kAnalysisTestBuild && flags.guardOff;
            const auto dc = guard.push (p.index, p.r.confident, p.r.note, guardOff);
            int ringResult = -1;
            double centre = 0.0;
            if (dc.valid)
            {
                if (dc.held) ++stats.guardHeld;
                const bool wasConfident = tracker.liveConfident();
                const float liveBefore  = tracker.liveNote();
                tracker.onDecision (dc.note);
                // Centre of the decided hop j (absolute samples, R69).
                const double aHopJ = static_cast<double> ((static_cast<std::int64_t> (kMpmWindow - 1) + static_cast<std::int64_t> (kMpmHop) * dc.index)
                                                          * static_cast<std::int64_t> (dims.D));
                centre = aHopJ - dims.lagCentre;
                float ringNote = dc.note;
                if (kAnalysisTestBuild && flags.ringHold && std::isnan (dc.note) && wasConfident && tracker.liveConfident())
                    ringNote = liveBefore;   // FR7n: the held value (R65 control)
                ringResult = f0ring.write (centre, ringNote, cells);
                if (ringResult == 0) ++stats.f0Writes;
                else if (ringResult == 1) ++stats.f0Skips;
                else ++stats.f0NoLog;
            }
           #if OGRAN_TEST_HOOKS
            auto& h = hopLog[static_cast<std::size_t> (hopCount++ % kLogSize)];
            h.index = p.index;
            h.aHop = p.aHop;
            h.commitAt = at;
            h.rawNote = p.r.note;
            h.tau = p.r.tau;
            h.clarity = p.r.clarity;
            h.level = p.r.level;
            h.confident = p.r.confident;
            h.decided = dc.valid ? dc.index : -1;
            h.decidedNote = dc.note;
            h.decidedCentre = centre;
            h.ringResult = ringResult;
           #else
            juce::ignoreUnused (at);
           #endif
        }

        double        fs = 48000.0;
        AnalysisDims  dims;
        OnsetDetector detector;
        Decimator     decim;
        Mpm           mpm;
        OctaveGuard   guard;
        PitchTracker  tracker;
        CellLog       cells;
        F0Ring        f0ring;
        std::array<float, 2 * kMpmWindow> win {};   // decimated window ring, doubled (contiguous reads)
        Pending       pending;
        std::int64_t  hopIndex = 0;
        std::int64_t  lastEventAbs = -1;
        float         thrLin = 0.0f;
        float         onsetSrc = 0.0f, pitchSrc = 0.0f;
    };
} // namespace ogran
