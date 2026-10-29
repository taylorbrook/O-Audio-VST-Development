/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// v3.5.0 Grain Adapt: extrema density of the held freeze loop (Nielsen, DAFx26).
//
// The span of the next K local extrema ahead of a read point measures how much is
// going on there: a bass note has few, widely spaced extrema (long span), noise or a
// transient has them packed tight (short span). Frozen grains take their length from
// that span, calibrated so Grain Size stays the mean length.
//
// Analysis (the loop as it is heard: mid channel, seam crossfade applied as
// FreezeManager::locate applies it, periodic with period loopLength):
//   d[i] = (y[i+1] - y[i-1]) / 2, the cubic B-spline derivative at the knot (zero at
//   Nyquist); a sign change against the last NONZERO d is a candidate (runs of exact
//   zeros never fire); its position is (i - 1) + |d-| / (|d-| + |d|) (reverse linear);
//   it is kept iff the B-spline value there differs by more than kEps from the last
//   KEPT extremum (hysteresis). A warm-up over the end of the loop seeds that state, so
//   the extrema just after the seam are judged like any other, and the main pass runs
//   i = 1 .. loopLength (i = loopLength is i = 0), so positions come out sorted in
//   [0, loopLength).
//
// The work is spread over blocks: kRate units per output sample since engage (one unit
// per analysed sample, kCalibUnits per calibration step). The analysis is complete,
// for any block split, by readyAbs = engageAbs + ceil(W / kRate).
//
// Calibration: K0 (fractional) is fixed per snapshot so the median span over kPoints
// read points is kTrefMs. A grain at loop point p then gets
//   len = clamp(gs * exp(a * (ln L(p) - lnMed) + c), lo, hi)
// with c solved per (gs, a) so the clamped mean over the kPoints points is gs.
//
// Allocation-free after prepare(). No JUCE.
class ExtremaMap
{
public:
    static constexpr float  kEps        = 0.001f;   // -60 dBFS (paper 2.2)
    static constexpr int    kRate       = 32;       // analysis units per output sample
    static constexpr int    kChunk      = 2048;     // samples per ring read
    static constexpr int    kWarmup     = 2048;
    static constexpr int    kCalibSteps = 5;        // 4 K0 fixed-point steps + the final table
    static constexpr int    kCalibUnits = 4096;
    static constexpr int    kPoints     = 256;
    static constexpr int    kMinExtrema = 8;
    static constexpr double kTrefMs     = 100.0;
    static constexpr double kMinLoopSec = 0.2;

    void prepare (double newSampleRate, int maxLoopSamples)
    {
        sampleRate = newSampleRate;
        positions.assign (static_cast<size_t> (std::max (1, maxLoopSamples)), 0.0f);
        for (auto* v : { &bufL, &bufR, &seamL, &seamR })
            v->assign (static_cast<size_t> (kChunk + 8), 0.0f);
        yb.assign (static_cast<size_t> (kChunk + 8), 0.0f);
        clear();
    }

    void clear() noexcept
    {
        state = State::Idle;
        valid = false;
        M = 0;
        ++generation;
    }

    void cancel() noexcept { clear(); }

    // O(1). origin = the ring's absolute level-0 index of snapshot position 0.
    void begin (std::int64_t newOrigin, int newLength, int newXfade, int newLoopLength,
                std::int64_t newEngageAbs) noexcept
    {
        ++generation;
        origin = newOrigin;
        length = newLength;
        xfade = newXfade;
        lp = newLoopLength;
        engageAbs = newEngageAbs;
        valid = false;
        M = 0;
        unitsDone = 0;

        if (lp < static_cast<int> (std::ceil (kMinLoopSec * sampleRate)) || lp > static_cast<int> (positions.size()))
        {
            state = State::Done;   // too short to adapt: fixed length
            totalUnits = 0;
            readyAbs = engageAbs;
            return;
        }

        warm = std::min (kWarmup, lp - 1);
        totalUnits = static_cast<std::int64_t> (warm) + lp + static_cast<std::int64_t> (kCalibSteps) * kCalibUnits;
        readyAbs = engageAbs + (totalUnits + kRate - 1) / kRate;
        q = 0;
        calibStep = 0;
        sPrev = 0;
        dPrev = 0.0f;
        vPrev = 0.0f;
        haveV = false;
        state = State::Analysing;
    }

    // Runs work until the cumulative quota (blockEndAbs - engageAbs) * kRate is used.
    template <class Ring>
    void step (const Ring& ring, std::int64_t blockEndAbs) noexcept
    {
        if (state != State::Analysing)
            return;
        const std::int64_t quota = std::min (totalUnits, (blockEndAbs - engageAbs) * kRate);
        const std::int64_t passUnits = static_cast<std::int64_t> (warm) + lp;

        while (unitsDone < quota)
        {
            if (unitsDone < passUnits)
            {
                // q = 0 .. warm - 1: i = lp - warm + 1 .. lp (not stored); then i = 1 .. lp
                const bool warming = q < warm;
                const int i0 = warming ? lp - warm + 1 + q : q - warm + 1;   // both phases end at i = lp
                const std::int64_t budget = std::min (quota, passUnits) - unitsDone;
                int n = std::min ({ kChunk, lp - i0 + 1, static_cast<int> (std::min<std::int64_t> (budget, kChunk)) });
                if (warming)
                    n = std::min (n, warm - q);
                scan (ring, i0, n, ! warming);
                q += n;
                unitsDone += n;
            }
            else
            {
                if (unitsDone + kCalibUnits > quota)
                    break;
                calibrate();
                unitsDone += kCalibUnits;
                if (++calibStep == kCalibSteps)
                {
                    state = State::Done;
                    break;
                }
            }
        }
    }

    bool isDone() const noexcept { return state == State::Done && valid; }
    bool ready (std::int64_t abs) const noexcept { return isDone() && abs >= readyAbs; }
    std::int64_t getReadyAbs() const noexcept { return readyAbs; }
    std::uint32_t getGeneration() const noexcept { return generation; }
    int getNumExtrema() const noexcept { return M; }
    double getK0() const noexcept { return K0; }
    bool isAnalysing() const noexcept { return state == State::Analysing; }

    // Loop coordinate of a snapshot position: the same mapping as FreezeManager::locate.
    double loopCoord (double pos) const noexcept
    {
        const double l = static_cast<double> (lp);
        double m = std::fmod (pos - static_cast<double> (xfade), l);
        if (m < 0.0) m += l;
        return m;
    }

    // Span of the K0 extrema ahead of loop point p (forward) or behind it (reverse).
    double spanForward (double p) const noexcept
    {
        return spanAhead (p, K0);
    }

    double spanBackward (double p) const noexcept
    {
        const long m = static_cast<long> (std::lower_bound (positions.data(), positions.data() + M, static_cast<float> (p))
                                          - positions.data());   // first >= p
        const long k0 = static_cast<long> (K0);
        const double f = K0 - static_cast<double> (k0);
        return nAt (m) - ((1.0 - f) * nAt (m - k0) + f * nAt (m - k0 - 1));
    }

    // Clamped mean over the kPoints calibration points = gs. Monotone in c: bisection over
    // prefix sums of exp(a * lr) (lr sorted, a > 0, so they ascend).
    double solveC (double gs, double a, double lo, double hi) const noexcept
    {
        double E[kPoints], S[kPoints + 1];
        S[0] = 0.0;
        for (int j = 0; j < kPoints; ++j)
        {
            E[j] = std::exp (a * lr[static_cast<size_t> (j)]);
            S[j + 1] = S[j] + E[j];
        }
        auto mean = [&] (double c)
        {
            const double g = gs * std::exp (c);
            const int iLo = static_cast<int> (std::lower_bound (E, E + kPoints, lo / g) - E);
            const int iHi = static_cast<int> (std::upper_bound (E, E + kPoints, hi / g) - E);
            return (lo * iLo + g * (S[iHi] - S[iLo]) + hi * (kPoints - iHi)) / kPoints;
        };
        double c0 = -12.0, c1 = 12.0;
        for (int it = 0; it < 40; ++it)
        {
            const double cm = 0.5 * (c0 + c1);
            (mean (cm) < gs ? c0 : c1) = cm;
        }
        return 0.5 * (c0 + c1);
    }

    // Grain length (samples) at loop point p.
    double lengthAt (double p, bool reverse, double c, double gs, double a, double lo, double hi) const noexcept
    {
        const double L = std::max (1.0, reverse ? spanBackward (p) : spanForward (p));
        return std::clamp (gs * std::exp (a * (std::log (L) - lnMed) + c), lo, hi);
    }

   #ifdef GS_TEST_ADAPT_FIXED_K
    // Negative control only (tests/adapt-gate N1): CONTEXT's first guess, K = gs * M /
    // loopLength and the span used directly (no K0, no c solve).
    double lengthAtFixedK (double p, bool reverse, double gs, double a, double lo, double hi) const noexcept
    {
        const double K = std::max (1.0, gs * M / static_cast<double> (lp));
        double L = spanAhead (p, K);
        if (reverse)
        {
            const long m = static_cast<long> (std::lower_bound (positions.data(), positions.data() + M, static_cast<float> (p))
                                              - positions.data());
            const long k0 = static_cast<long> (K);
            const double f = K - static_cast<double> (k0);
            L = nAt (m) - ((1.0 - f) * nAt (m - k0) + f * nAt (m - k0 - 1));
        }
        L = std::max (1.0, L);
        return std::clamp (std::exp (a * std::log (L) + (1.0 - a) * std::log (gs)), lo, hi);
    }
   #endif

private:
    enum class State { Idle, Analysing, Done };

    // Unwrapped position of extremum j (any integer): pos[j mod M] + lp * floor(j / M).
    double nAt (long j) const noexcept
    {
        long qq = j / M, r = j % M;
        if (r < 0) { r += M; --qq; }
        return static_cast<double> (positions[static_cast<size_t> (r)]) + static_cast<double> (qq) * lp;
    }

    double spanAhead (double p, double K) const noexcept
    {
        const long m = static_cast<long> (std::upper_bound (positions.data(), positions.data() + M, static_cast<float> (p))
                                          - positions.data()) - 1;   // -1 = the previous period's last
        const long k0 = static_cast<long> (K);
        const double f = K - static_cast<double> (k0);
        return (1.0 - f) * nAt (m + k0) + f * nAt (m + k0 + 1) - nAt (m);
    }

    // y[j0 .. j0 + n) of the loop signal into dst (j taken modulo lp).
    template <class Ring>
    void fillY (const Ring& ring, int j0, int n, float* dst) noexcept
    {
        constexpr float halfPi = 1.57079632679489661923f;
        int j = j0 % lp;
        if (j < 0) j += lp;
        const int seamJ = lp - xfade;   // b = xfade + j >= length - xfade
        while (n > 0)
        {
            const int run = std::min (n, lp - j);
            ring.copyLevel (0, 0, origin + xfade + j, run, bufL.data());
            ring.copyLevel (0, 1, origin + xfade + j, run, bufR.data());
            for (int k = 0; k < run; ++k)
                dst[k] = 0.5f * (bufL[static_cast<size_t> (k)] + bufR[static_cast<size_t> (k)]);
            if (xfade > 0 && j + run > seamJ)
            {
                const int s0 = std::max (j, seamJ);
                const int sn = j + run - s0;
                ring.copyLevel (0, 0, origin + xfade + s0 - lp, sn, seamL.data());
                ring.copyLevel (0, 1, origin + xfade + s0 - lp, sn, seamR.data());
                const double x = static_cast<double> (xfade);
                const double seamStart = static_cast<double> (length) - x;
                for (int k = 0; k < sn; ++k)
                {
                    const double b = x + static_cast<double> (s0 + k);
                    const float fade = static_cast<float> ((b - seamStart) / x);
                    const float v2 = 0.5f * (seamL[static_cast<size_t> (k)] + seamR[static_cast<size_t> (k)]);
                    float& v = dst[s0 - j + k];
                    v = std::cos (fade * halfPi) * v + std::sin (fade * halfPi) * v2;
                }
            }
            dst += run;
            n -= run;
            j = 0;
        }
    }

    // Detection over i = i0 .. i0 + n - 1 (i <= lp; i = lp is i = 0).
    template <class Ring>
    void scan (const Ring& ring, int i0, int n, bool store) noexcept
    {
        constexpr int kBack = 3;   // y from i0 - 3 (B-spline taps k - 1 .. k + 2, k >= i - 1)
        fillY (ring, i0 - kBack, n + 2 * kBack, yb.data());
        const auto y = [this, i0] (int i) { return yb[static_cast<size_t> (i - i0 + kBack)]; };   // i in [i0 - 3, i0 + n + 3)

        for (int i = i0; i < i0 + n; ++i)
        {
            const float d = 0.5f * (y (i + 1) - y (i - 1));
            const int s = (d > 0.0f) - (d < 0.0f);
            if (s == 0)
                continue;
            if (sPrev != 0 && s != sPrev)
            {
                const float a0 = std::fabs (dPrev), a1 = std::fabs (d);
                const double nm = static_cast<double> (i - 1) + static_cast<double> (a0 / (a0 + a1));
                const int k = static_cast<int> (std::floor (nm));
                const float t = static_cast<float> (nm - k);
                const float u = 1.0f - t, t2 = t * t, t3 = t2 * t;
                const float vm = y (k - 1) * (u * u * u * (1.0f / 6.0f))
                               + y (k)     * ((3.0f * t3 - 6.0f * t2 + 4.0f) * (1.0f / 6.0f))
                               + y (k + 1) * ((-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * (1.0f / 6.0f))
                               + y (k + 2) * (t3 * (1.0f / 6.0f));
                if (! haveV || std::fabs (vm - vPrev) > kEps)
                {
                    if (store && M < static_cast<int> (positions.size()))
                        positions[static_cast<size_t> (M++)] = static_cast<float> (std::min (nm, static_cast<double> (lp)));
                    vPrev = vm;
                    haveV = true;
                }
            }
            sPrev = s;
            dPrev = d;
        }
    }

    void spansAt (double K, std::array<double, kPoints>& out) const noexcept
    {
        for (int j = 0; j < kPoints; ++j)
            out[static_cast<size_t> (j)] = spanAhead ((j + 0.5) * lp / kPoints, K);
    }

    static double median (std::array<double, kPoints> t) noexcept
    {
        std::nth_element (t.begin(), t.begin() + kPoints / 2, t.end());
        return t[kPoints / 2];
    }

    void calibrate() noexcept
    {
        if (calibStep == 0)
        {
            valid = false;
            if (M < kMinExtrema)
            {
                calibStep = kCalibSteps - 1;   // nothing to calibrate: fixed length
                return;
            }
            K0 = std::max (1.0, static_cast<double> (M) * tref() / lp);   // count-based first guess
        }

        std::array<double, kPoints> L {};
        spansAt (K0, L);
        if (calibStep < kCalibSteps - 1)
        {
            K0 = std::clamp (K0 * tref() / std::max (median (L), 1e-9), 1.0, static_cast<double> (M) - 2.0);
            return;
        }
        lnMed = std::log (std::max (median (L), 1e-9));
        for (int j = 0; j < kPoints; ++j)
            lr[static_cast<size_t> (j)] = std::log (std::max (L[static_cast<size_t> (j)], 1e-9)) - lnMed;
        std::sort (lr.begin(), lr.end());
        valid = true;
    }

    double tref() const noexcept { return kTrefMs * 1e-3 * sampleRate; }

    double sampleRate = 44100.0;
    std::vector<float> positions;              // sorted extremum positions, loop coordinates
    std::vector<float> bufL, bufR, seamL, seamR, yb;
    std::array<double, kPoints> lr {};         // sorted ln (L / Lmed) at the calibration points

    State state = State::Idle;
    bool valid = false;
    std::uint32_t generation = 0;
    std::int64_t origin = 0, engageAbs = 0, readyAbs = 0, totalUnits = 0, unitsDone = 0;
    int length = 0, xfade = 0, lp = 0, warm = 0, q = 0, calibStep = 0, M = 0;
    double K0 = 1.0, lnMed = 0.0;

    // detection state, carried across chunks
    int sPrev = 0;
    float dPrev = 0.0f, vPrev = 0.0f;
    bool haveV = false;
};
