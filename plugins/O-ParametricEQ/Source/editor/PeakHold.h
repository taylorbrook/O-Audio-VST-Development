/*
   This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
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

    O-ParametricEQ - editor-side display helpers (Stage 3, Phase 3.3)

    Header-only, standard library only (no JUCE), so the render harness
    tests the shipping code (row M1).

    PeakHold (PLAN-3.3 P4; RESEARCH-3.3 Q2). The editor reads
    readAndResetOutputPeak once per 60 Hz tick. A host block longer than a
    tick leaves ticks with no block since the last read, which read 0: fed
    raw, the meter's release pulls the bar down on every empty tick (a
    sawtooth, 47 / 165 empty ticks at 1024 / 44.1 kHz). The editor keeps
    the last H reads per channel and shows their max, with
        H = min (16, ceil (1.3 x blockSize / fs x 60) + 1)
    from the prepared block size (the host's largest), and H = 2 before a
    rate is known. This adds at most H ticks of hold to a real drop, which
    the page's 20 dB/s release dominates. The 1.3 (PLAN-4.1, row M1j, Taylor's
    call 2026-11-16): block completions that jitter +-30 % of a block period
    leave gaps of up to 1.6 periods; without it, 2048 / 44.1 kHz showed 4
    empty reads in 587 ticks.

    encodeAnalyzerPoints (PLAN-3.3 P2). The analyzer's 256 display points
    as integer tenths of a dB, clamped to [-1200, +600] (-120 dB lies below
    the page's -96 dB floor). A span that starts at or above fs / 2 holds
    no bin and is sent as JSON null: it is marked here with
    kAnalyzerNoData and the editor turns it into var().

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace opeq
{
    //==========================================================================
    class PeakHold
    {
    public:
        static constexpr int capacity = 16;

        /** Block completions may jitter +-30 % of a block period (row M1j). */
        static constexpr double jitterMargin = 1.3;

        /** H for a prepared block size and rate at a tick rate (60 Hz). */
        static int depthFor (int blockSize, double sampleRateHz, double tickHz = 60.0) noexcept
        {
            if (! (sampleRateHz > 0.0) || ! std::isfinite (sampleRateHz))
                return 2;

            const double ticksPerBlock = (double) std::max (0, blockSize) / sampleRateHz * tickHz;
            const int h = (int) std::ceil (jitterMargin * ticksPerBlock) + 1;
            return std::clamp (h, 1, capacity);
        }

        /** The number of reads max() looks at, 1 ... capacity. The ring keeps
            the last `capacity` reads whatever the depth, so a change of
            depth takes effect on the next max(). */
        void setDepth (int h) noexcept      { depth = std::clamp (h, 1, capacity); }
        int getDepth() const noexcept       { return depth; }

        void reset() noexcept
        {
            ring.fill (0.0f);
            pos = 0;
            count = 0;
        }

        /** One read (a linear gain, >= 0). */
        void push (float gain) noexcept
        {
            ring[(std::size_t) pos] = gain;
            pos = (pos + 1) % capacity;
            count = std::min (count + 1, capacity);
        }

        /** The largest of the last min (depth, reads so far) reads; 0 before
            any read. inf passes through (the meter shows the overflow). */
        float max() const noexcept
        {
            const int n = std::min (depth, count);
            float m = 0.0f;

            for (int i = 1; i <= n; ++i)
            {
                const float v = ring[(std::size_t) ((pos - i + capacity) % capacity)];
                if (v > m)
                    m = v;
            }

            return m;
        }

    private:
        std::array<float, capacity> ring {};
        int pos = 0;
        int count = 0;
        int depth = 2;
    };

    //==========================================================================
    /** A linear gain as tenths of a dB, clamped to [lo, hi]; 0, a negative
        or a NaN gain reads lo, inf reads hi. */
    inline int gainToTenthsDb (float gain, int lo, int hi) noexcept
    {
        if (std::isnan (gain) || ! (gain > 0.0f))
            return lo;

        if (std::isinf (gain))
            return hi;

        const double tenths = std::round (200.0 * std::log10 ((double) gain));
        return (int) std::clamp (tenths, (double) lo, (double) hi);
    }

    //==========================================================================
    inline constexpr int kAnalyzerPoints  = 256;
    inline constexpr int kAnalyzerLoTenths = -1200;
    inline constexpr int kAnalyzerHiTenths = 600;

    /** Marks a span that starts at or above fs / 2 (sent as JSON null). */
    inline constexpr std::int32_t kAnalyzerNoData = INT32_MIN;

    /** Start of display span p: 10 x 3000^(p / 256) Hz (SpectrumAnalyzer). */
    inline double analyzerSpanStartHz (int p) noexcept
    {
        return 10.0 * std::pow (3000.0, (double) p / (double) kAnalyzerPoints);
    }

    /** 256 display points (dB) -> tenths of a dB, clamped to [-1200, +600];
        spans starting at or above fs / 2 -> kAnalyzerNoData. A non-finite
        point reads the floor. */
    inline void encodeAnalyzerPoints (const float* pointsDb, double sampleRateHz,
                                      std::array<std::int32_t, kAnalyzerPoints>& out) noexcept
    {
        const double nyquist = sampleRateHz * 0.5;

        for (int p = 0; p < kAnalyzerPoints; ++p)
        {
            if (sampleRateHz > 0.0 && analyzerSpanStartHz (p) >= nyquist)
            {
                out[(std::size_t) p] = kAnalyzerNoData;
                continue;
            }

            const float db = pointsDb[p];
            const double tenths = std::isfinite (db) ? std::round ((double) db * 10.0)
                                                     : (double) kAnalyzerLoTenths;
            out[(std::size_t) p] = (std::int32_t) std::clamp (tenths, (double) kAnalyzerLoTenths,
                                                              (double) kAnalyzerHiTenths);
        }
    }
}
