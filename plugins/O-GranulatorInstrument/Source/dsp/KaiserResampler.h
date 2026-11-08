/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include "BandlimitedReader.h"   // grain-engine 1.1.0: blr::besselI0

// ============================================================================
// Offline Kaiser-windowed sinc resampler (ARCHITECTURE §5, RESEARCH-2.1 §7,
// PLAN-2.1 T5). Loader worker thread only: it allocates.
//
//   fc        = 0.95 · min (fsIn, fsOut) / 2          (cutoff, Hz)
//   fcIn      = fc / fsIn                              (cycles per input sample)
//   halfWidth = 64 zero-crossings = 64 / (2·fcIn)      (input samples)
//   h (t)     = 2·fcIn · sinc (2·fcIn·t) · I0 (β·√(1 − (t/halfWidth)²)) / I0 (β)
//
// A 4096-phase table (double, + 1 row for the lerp end) is built once per
// call with blr::besselI0, β = 9. Output sample j sits at input position
// x = j · fsIn / fsOut (computed per sample, never accumulated); the kernel is
// the lerp of the two phase rows around frac (x), accumulated in double, and
// EACH output sample is divided by the sum of its weights, so a constant
// input reproduces exactly (DC = 1 to ~1e-15 at any ratio). Out-of-range input
// samples are zero (the tape starts and ends with the file's own band-limited
// onset / release); their weights still enter the sum, so the edge behaves as
// zero padding. Equal rates: a bit-exact copy, no filtering.
//
// The negative controls (K1: unnormalised weights; K2: β = 2) are runtime
// Options, not globals.
// ============================================================================
namespace ogi::kaiser
{
    struct Options
    {
        double beta          = 9.0;
        bool   normalise     = true;    // K1 control: false → raw weighted sum
        int    zeroCrossings = 64;      // per side (32 leaves 0.9·min/2 inside the transition band: K2, 2.1 execute)
        double cutoff        = 0.95;    // × min (fsIn, fsOut) / 2
        int    phases        = 4096;
    };

    inline bool ratesEqual (double a, double b) noexcept { return ! (a < b) && ! (a > b); }

    /** Output frames for nIn input frames: round (nIn · fsOut / fsIn), nIn when equal. */
    inline std::int64_t outputLength (std::int64_t nIn, double fsIn, double fsOut) noexcept
    {
        if (nIn <= 0 || ! (fsIn > 0.0) || ! (fsOut > 0.0))
            return 0;
        if (ratesEqual (fsIn, fsOut))
            return nIn;
        return std::max<std::int64_t> (1, static_cast<std::int64_t> (std::llround (static_cast<double> (nIn) * fsOut / fsIn)));
    }

    /** Input taps per side for a ratio (diagnostics / tests): ceil (halfWidth). */
    inline int tapsPerSide (double fsIn, double fsOut, const Options& opt = {}) noexcept
    {
        const double fcIn = opt.cutoff * 0.5 * std::min (fsIn, fsOut) / fsIn;
        return static_cast<int> (std::ceil (static_cast<double> (opt.zeroCrossings) / (2.0 * fcIn)));
    }

    /** Resamples in[0, nIn) from fsIn to fsOut into out (resized). Polls
        isCancelled (may be empty) every 65536 output frames; returns false if
        it reported true (out is then incomplete). */
    inline bool resample (const float* in, std::int64_t nIn, double fsIn, double fsOut,
                          std::vector<float>& out, const std::function<bool()>& isCancelled,
                          const Options& opt = {})
    {
        const std::int64_t nOut = outputLength (nIn, fsIn, fsOut);
        out.assign (static_cast<std::size_t> (nOut), 0.0f);
        if (nOut <= 0 || in == nullptr)
            return true;

        if (ratesEqual (fsIn, fsOut))
        {
            std::copy (in, in + nIn, out.begin());
            return true;
        }

        const double fcIn      = opt.cutoff * 0.5 * std::min (fsIn, fsOut) / fsIn;
        const double halfWidth = static_cast<double> (opt.zeroCrossings) / (2.0 * fcIn);
        const int    K         = std::max (1, static_cast<int> (std::ceil (halfWidth)));
        const int    taps      = 2 * K;
        const int    P         = std::max (1, opt.phases);
        const double i0b       = ouaricon::grain::blr::besselI0 (opt.beta);
        constexpr double pi    = 3.14159265358979323846;

        // Row p = kernel for frac p / P; tap m reads input index i0 + (m − K + 1).
        std::vector<double> table (static_cast<std::size_t> (P + 1) * static_cast<std::size_t> (taps), 0.0);
        for (int p = 0; p <= P; ++p)
        {
            const double fr = static_cast<double> (p) / static_cast<double> (P);
            double* row = table.data() + static_cast<std::size_t> (p) * static_cast<std::size_t> (taps);
            for (int m = 0; m < taps; ++m)
            {
                const double t = static_cast<double> (m - K + 1) - fr;
                if (! (std::abs (t) < halfWidth))
                {
                    row[m] = 0.0;
                    continue;
                }
                const double x    = 2.0 * fcIn * t;
                const double sinc = (std::abs (x) < 1.0e-300) ? 1.0 : std::sin (pi * x) / (pi * x);
                const double u    = t / halfWidth;
                const double w    = ouaricon::grain::blr::besselI0 (opt.beta * std::sqrt (std::max (0.0, 1.0 - u * u))) / i0b;
                row[m] = 2.0 * fcIn * sinc * w;
            }
        }

        const double step = fsIn / fsOut;
        for (std::int64_t j = 0; j < nOut; ++j)
        {
            const double x  = static_cast<double> (j) * step;
            const double xf = std::floor (x);
            const std::int64_t i0 = static_cast<std::int64_t> (xf);
            const double ph = (x - xf) * static_cast<double> (P);
            const int    ip = std::clamp (static_cast<int> (ph), 0, P - 1);
            const double u  = ph - static_cast<double> (ip);

            const double* a = table.data() + static_cast<std::size_t> (ip) * static_cast<std::size_t> (taps);
            const double* b = a + taps;

            double acc = 0.0, wsum = 0.0;
            const std::int64_t first = i0 - K + 1;
            for (int m = 0; m < taps; ++m)
            {
                const double w = a[m] + u * (b[m] - a[m]);
                wsum += w;
                const std::int64_t n = first + m;
                if (n >= 0 && n < nIn)
                    acc += w * static_cast<double> (in[n]);
            }

            double y = acc;
            if (opt.normalise)
                y = (wsum > 0.0 || wsum < 0.0) ? acc / wsum : 0.0;
            out[static_cast<std::size_t> (j)] = static_cast<float> (y);

            if ((j & 0xFFFF) == 0xFFFF && isCancelled && isCancelled())
                return false;
        }
        return true;
    }
} // namespace ogi::kaiser
