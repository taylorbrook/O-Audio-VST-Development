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
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "BandlimitedReader.h"   // grain-engine 1.1.0 (NOT edited; G5 module-gap candidate)

#ifndef OGI_TEST_HOOKS
 #define OGI_TEST_HOOKS 0
#endif

// ============================================================================
// MipTape<L> — the positional (non-circular) sibling of grain-engine's
// MipRing / MipSnapshot (ARCHITECTURE §7, RESEARCH-2.1 §3, PLAN-2.1 T6).
//
// Storage = the MipRing convention, index for index (P7):
//   level 0 index p holds input frame p;
//   level l index j holds the half-band of level (l−1) when its index 2j + 1
//   arrives (the child is taken on ODD parent indices only, as MipRing::push).
// No delay is baked into the index: reads go through MipRing<L>::levelPos,
// which applies D(l) = 63 (2^l − 1) at read time. History before index 0 is
// zero (MipRing's cleared history), so writeRun ≡ MipRing::push bitwise on
// every stored index (row T1).
//
// Sizes (P8): level l stores ((F + margin (L)) >> l) + 2·kTaps + 4 indices
// (margin (4) = 1074), after a kPre-sample zero pre-roll that keeps a 16-tap
// read at position 0 inside the array. The writer flushes margin (L) zero
// frames past F (flushTail) so every level is settled over [0, F).
//
// The half-band is a private duplicate of MipRing::halfband (double
// accumulation, 0.5·c[0] + 32 folded odd taps, the same operand order),
// reading its history from the tape's own level (l − 1) array.
//
// readStereo (level, pos, set, l, r): positional, double pos in level-0
// frames, the MipSnapshot::locateStart model; the 16-tap start is clamped to
// the stored span (a guard: region fit + flush keep reads inside).
// Callers latch with latchLevel (rate, 4) — ALWAYS pass 4 (P6).
//
// Threads: prepare runs on the loader worker (it allocates); flushTail is
// offline (loader). writeRun / settleRun are audio-thread safe as well (no
// allocation; `written` stays a max): the 2.4 record heads write the working
// copy at grid ticks (PLAN-2.4 PD10). readStereo is audio-thread safe (const).
// ============================================================================
namespace ogi
{
   #if OGI_TEST_HOOKS
    namespace test
    {
        /** T1 / T2 negative control: index m (0..31) of the duplicated half-band's
            odd taps is moved by 1 ulp (std::nextafter toward +1). −1 = off. */
        inline int mipTapePerturbTap = -1;
    }
   #endif

    template <int L>
    class MipTape
    {
    public:
        static_assert (L >= 0 && L <= 6, "MipTape: 0..6 octave levels");
        using Ring = ouaricon::grain::MipRing<L>;
        static constexpr int kLevels    = L;
        static constexpr int kNumLevels = L + 1;
        static constexpr int kTaps      = ouaricon::grain::blr::kTaps;
        static constexpr int kPre       = ouaricon::grain::blr::kTaps;   // zero pre-roll before index 0

        /** Zero frames the writer flushes past F (= MipRing<L>::margin (L)). */
        static constexpr int tailFrames() noexcept { return Ring::margin (L); }

        /** Stored indices of level l for a tape of `frames` level-0 frames. */
        static constexpr int levelSizeFor (int level, int frames) noexcept
        {
            return ((frames + tailFrames()) >> level) + 2 * kTaps + 4;
        }

        //======================================================================
        /** Allocates every level for F frames, zeroed (worker thread). */
        void prepare (int numFrames)
        {
            bank   = &ouaricon::grain::sincBank();   // touched once, off the audio thread
            frames = std::max (0, numFrames);
            for (int l = 0; l < kNumLevels; ++l)
            {
                sizes[(size_t) l] = levelSizeFor (l, frames);
                for (int ch = 0; ch < 2; ++ch)
                    data[(size_t) l][(size_t) ch].assign ((size_t) (sizes[(size_t) l] + kPre), 0.0f);
            }
            written = 0;
        }

        int getFrames() const noexcept                 { return frames; }
        int levelSize (int level) const noexcept       { return sizes[(size_t) level]; }
        std::int64_t getWrittenFrames() const noexcept { return written; }
        bool isPrepared() const noexcept               { return bank != nullptr; }

        /** Level sample at level index j (0 outside the stored span). Tests / T1. */
        float levelSample (int level, int ch, std::int64_t j) const noexcept
        {
            if (level < 0 || level > L || j < 0 || j >= sizes[(size_t) level])
                return 0.0f;
            return data[(size_t) level][(size_t) ch][(size_t) (j + kPre)];
        }

        //======================================================================
        /** Writes level-0 frames [pos, pos + n), then every child index j whose
            parent index 2j + 1 lies in the newly written parent span, level by
            level. Sequential runs (pos == getWrittenFrames()) reproduce MipRing
            exactly for any split into runs. Indices past the stored span are
            dropped. */
        void writeRun (std::int64_t pos, int n, const float* inL, const float* inR) noexcept
        {
            if (n <= 0 || pos < 0 || bank == nullptr)
                return;

            std::int64_t pStart = pos;
            std::int64_t pEnd   = std::min<std::int64_t> (pos + n, sizes[0]);
            for (std::int64_t p = pStart; p < pEnd; ++p)
            {
                const auto src = static_cast<std::size_t> (p - pos);
                data[0][0][(size_t) (p + kPre)] = inL[src];
                data[0][1][(size_t) (p + kPre)] = inR[src];
            }
            written = std::max (written, pEnd);

            for (int l = 1; l <= L; ++l)
            {
                if (pEnd <= pStart)
                    break;
                // Children with 2j + 1 in [pStart, pEnd): j in [pStart >> 1, pEnd >> 1).
                const std::int64_t cStart = pStart >> 1;
                const std::int64_t cEnd   = std::min<std::int64_t> (pEnd >> 1, sizes[(size_t) l]);
                for (std::int64_t j = cStart; j < cEnd; ++j)
                {
                    const std::int64_t parent = 2 * j + 1;
                    for (int ch = 0; ch < 2; ++ch)
                        data[(size_t) l][(size_t) ch][(size_t) (j + kPre)] = halfband (l - 1, ch, parent);
                }
                pStart = cStart;
                pEnd   = cEnd;
            }
        }

        /** PLAN-2.4 PD10: re-derives every child whose parent index lies in
            [pos, pos + n) (writeRun's child loop; level 0 untouched). The record
            head calls it past a wrap point and past a park point, so children
            whose window reaches newly written frames are never stale. */
        void settleRun (std::int64_t pos, int n) noexcept
        {
            if (n <= 0 || pos < 0 || bank == nullptr)
                return;
            // Never past the flushed span F + margin (L): flushTail derives the
            // children of exactly that span, so a settle reaching further would
            // derive children no build ever derives (and no read reaches).
            std::int64_t pStart = pos;
            std::int64_t pEnd   = std::min<std::int64_t> ({ pos + n, (std::int64_t) sizes[0],
                                                            (std::int64_t) frames + tailFrames() });
            for (int l = 1; l <= L; ++l)
            {
                if (pEnd <= pStart)
                    break;
                const std::int64_t cStart = pStart >> 1;
                const std::int64_t cEnd   = std::min<std::int64_t> (pEnd >> 1, sizes[(size_t) l]);
                for (std::int64_t j = cStart; j < cEnd; ++j)
                {
                    const std::int64_t parent = 2 * j + 1;
                    for (int ch = 0; ch < 2; ++ch)
                        data[(size_t) l][(size_t) ch][(size_t) (j + kPre)] = halfband (l - 1, ch, parent);
                }
                pStart = cStart;
                pEnd   = cEnd;
            }
        }

       #if OGI_TEST_HOOKS
        /** FNV-1a 64 over every level, both channels, the full stored span
            (PLAN-2.4 PD10; the FL3 / FL4 / RS / MS1 hashes). */
        std::uint64_t hashForTest() const noexcept
        {
            std::uint64_t hsh = 0xcbf29ce484222325ull;
            for (int l = 0; l < kNumLevels; ++l)
                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto* b = reinterpret_cast<const unsigned char*> (data[(size_t) l][(size_t) ch].data() + kPre);
                    const std::size_t nb = static_cast<std::size_t> (sizes[(size_t) l]) * sizeof (float);
                    for (std::size_t i = 0; i < nb; ++i)
                    {
                        hsh ^= b[i];
                        hsh *= 0x100000001b3ull;
                    }
                }
            return hsh;
        }

        /** max |x| of one level over the stored span (both channels). */
        float levelPeak (int level) const noexcept
        {
            float p = 0.0f;
            for (int ch = 0; ch < 2; ++ch)
                for (int j = 0; j < sizes[(size_t) level]; ++j)
                    p = std::max (p, std::abs (data[(size_t) level][(size_t) ch][(size_t) (j + kPre)]));
            return p;
        }

        /** Non-finite / subnormal samples over every level (FL1 end scan). */
        void scanForTest (std::uint64_t& nonFinite, std::uint64_t& subnormal) const noexcept
        {
            for (int l = 0; l < kNumLevels; ++l)
                for (int ch = 0; ch < 2; ++ch)
                    for (int j = 0; j < sizes[(size_t) l]; ++j)
                    {
                        const float x = data[(size_t) l][(size_t) ch][(size_t) (j + kPre)];
                        if (! std::isfinite (x)) ++nonFinite;
                        else if (std::fpclassify (x) == FP_SUBNORMAL) ++subnormal;
                    }
        }
       #endif

        /** Writes the margin (L) zero frames past F, so every level is settled
            over [0, F). Call once after the last writeRun (worker thread). */
        void flushTail() noexcept
        {
            static constexpr int kChunk = 256;
            float zeros[kChunk] {};
            std::int64_t pos = std::max<std::int64_t> (written, frames);
            const std::int64_t end = static_cast<std::int64_t> (frames) + tailFrames();
            while (pos < end)
            {
                const int n = static_cast<int> (std::min<std::int64_t> (kChunk, end - pos));
                writeRun (pos, n, zeros, zeros);
                pos += n;
            }
        }

        //======================================================================
        /** Positional band-limited read: pos in level-0 frames (double), level
            and set latched by the caller. Audio thread. */
        void readStereo (int level, double pos, int set, float& outL, float& outR) const noexcept
        {
            float k[kTaps];
            const std::int64_t s = locateStart (level, pos, set, k);
            outL = ouaricon::grain::blr::dot16 (data[(size_t) level][0].data() + s, k);
            outR = ouaricon::grain::blr::dot16 (data[(size_t) level][1].data() + s, k);
        }

        float readMono (int level, double pos, int set) const noexcept
        {
            float k[kTaps];
            const std::int64_t s = locateStart (level, pos, set, k);
            return 0.5f * ouaricon::grain::blr::dot16Sum (data[(size_t) level][0].data() + s,
                                                         data[(size_t) level][1].data() + s, k);
        }

    private:
        /** Storage offset (index + kPre) of the first of the 16 taps; fills k. */
        std::int64_t locateStart (int level, double pos, int set, float* k) const noexcept
        {
            const double bI = std::floor (pos);
            const auto p = Ring::levelPos (level, static_cast<std::int64_t> (bI), pos - bI);
            bank->kernel (set, p.frac, k);
            const std::int64_t s  = p.index - ouaricon::grain::blr::kTapsBack;
            const std::int64_t hi = std::max<std::int64_t> (-kPre, static_cast<std::int64_t> (sizes[(size_t) level]) - kTaps);
            return std::clamp<std::int64_t> (s, -kPre, hi) + kPre;
        }

        static double coef (int m) noexcept
        {
            double c = ouaricon::grain::blr::kHalfbandOdd[m];
           #if OGI_TEST_HOOKS
            if (m == test::mipTapePerturbTap)
                c = std::nextafter (c, 1.0);
           #endif
            return c;
        }

        /** Level (pl + 1) sample for the odd parent index n of level pl:
            y = 0.5·x[n − 63] + Σ_m h_m (x[n − 64 − 2m] + x[n − 62 + 2m]),
            x[i < 0] = 0 — MipRing::halfband's terms in its order. */
        float halfband (int pl, int ch, std::int64_t n) const noexcept
        {
            const float* x = data[(size_t) pl][(size_t) ch].data() + kPre;
            const std::int64_t size = sizes[(size_t) pl];
            const std::int64_t c = n - ouaricon::grain::blr::kHbDelay;
            auto at = [x, size] (std::int64_t i) noexcept -> double
            {
                return (i < 0 || i >= size) ? 0.0 : static_cast<double> (x[i]);
            };

            double acc = 0.5 * at (c);
            for (int m = 0; m < 32; ++m)
            {
                const std::int64_t o = 2 * m + 1;
                acc += coef (m) * (at (c - o) + at (c + o));
            }
            return static_cast<float> (acc);
        }

        const ouaricon::grain::SincBank* bank = nullptr;
        std::array<std::array<std::vector<float>, 2>, kNumLevels> data;
        std::array<int, kNumLevels> sizes {};
        int frames = 0;
        std::int64_t written = 0;
    };
} // namespace ogi
