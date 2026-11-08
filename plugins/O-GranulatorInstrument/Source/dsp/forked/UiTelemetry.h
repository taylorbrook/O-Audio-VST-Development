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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <type_traits>

// ============================================================================
// Stage 3 / Phase 3.2 — the buffer strip's transport (PLAN-3.2 P114,
// RESEARCH-3.2 R118–R122). Depends on <atomic> only.
//
//   TripleBuffer<T>  wait-free single-writer / single-reader frame exchange:
//                    the audio thread fills writeBuffer() and publish()es (one
//                    exchange); the reader's read() always returns a COMPLETE
//                    frame — the newest one, or the previous one again when
//                    nothing new arrived (no torn read, no retry, R117/R118).
//                    One reader at a time: the processor serialises readers
//                    with a SpinLock the audio thread never takes.
//   UiGrain          the per-slot POD the audio thread copies (R119). Every
//                    display quantity is DERIVED on the message thread.
//   UiSnapshot       one frame: ring-domain head, span, frozen, modulated
//                    bands (D30), stream pans, limit, ≤ 128 grains.
//   PeakRing         one peak per 32-sample RING cell (R120), constructor-
//                    allocated (never reallocated: a prepareToPlay off the
//                    message thread cannot free it under the reader).
//   binWave          the reader's absolute-cell wave binning (R121, R122, D31).
//
// Nothing here is read by the DSP: the engine only writes it.
// ============================================================================
namespace ogi
{
    template <class T>
    struct TripleBuffer
    {
        static_assert (std::is_trivially_copyable_v<T>, "TripleBuffer frames are copied as PODs");

        /** Writer (audio thread): the back buffer, owned by the writer until publish(). */
        T& writeBuffer() noexcept { return buf[back]; }

        /** Writer: hand the back buffer over (marked fresh), take the middle one back. */
        void publish() noexcept
        {
            back = static_cast<std::uint8_t> (mid.exchange (static_cast<std::uint8_t> (back | kFresh), std::memory_order_acq_rel) & kIndex);
        }

        /** Reader (one at a time): the newest complete frame. */
        const T& read() noexcept
        {
            if ((mid.load (std::memory_order_relaxed) & kFresh) != 0)
                front = static_cast<std::uint8_t> (mid.exchange (front, std::memory_order_acq_rel) & kIndex);
            return buf[front];
        }

    private:
        static constexpr std::uint8_t kIndex = 3, kFresh = 4;
        T buf[3] {};
        std::atomic<std::uint8_t> mid { 1 };
        std::uint8_t back = 0, front = 2;
    };

    /** ≈ 56 B per sounding slot. Copied, never computed, on the audio thread. */
    struct UiGrain
    {
        double readAbs = 0.0;          // g.readAbs (ring domain, R23)
        double phase   = 0.0;          // (g.age + g.frac) · g.invLen — the renderSegment expression
        double rate    = 1.0;          // g.rate (scale pick + follow included)
        float  lenSamples = 0.0f;      // g.len (output samples)
        float  gainL = 1.0f, gainR = 1.0f;   // latched spread × gain_rand
        float  killGain = 1.0f;
        float  tiltT = 0.5f, tiltA = 1.0f, tiltB = 1.0f;
        std::int16_t windowStep = 0;
        std::uint8_t stream1 = 0;      // g.stream + 1 — 1-based on the wire (R125)
        bool   killing = false;        // kill fade / pending kill / fade slot: not in counts
    };
    static_assert (std::is_trivially_copyable_v<UiGrain> && sizeof (UiGrain) == 56, "UiGrain is a 56-byte POD");

    struct UiSnapshot
    {
        static constexpr int kMaxGrains = 128;

        std::uint64_t seqHead = 0;     // written FIRST (TB1 checks seqHead == seqTail)
        std::int64_t  head = 0;        // ring.getWriteCount() at publish — W0 while frozen (R121)
        double        spanSamples = 0.0;   // modulated span; min (span, heldCaptured) frozen (R124)
        double        fs = 0.0;        // 0 = nothing published since construction
        float         bands[4][2] {};  // { position, spray } fractions after matrixTick (R123)
        float         streamPan[4] {}; // the bus pan (modulated), −1…1 — display only
        std::int32_t  limit = 0;       // gp.grainLimit
        std::int32_t  count = 0;       // grains[0 … count)
        bool          frozen = false;  // the engine's real head-stopped state (not freezePending)
        UiGrain       grains[kMaxGrains] {};
        std::uint64_t seqTail = 0;     // written LAST
    };
    static_assert (std::is_trivially_copyable_v<UiSnapshot> && sizeof (UiSnapshot) < 8 * 1024 + 256, "UiSnapshot ≈ 8.2 KB POD");

    struct PeakRing
    {
        static constexpr int kCells = 1 << 18;          // 1 MB of atomic<float>
        static constexpr int kCellSamples = 32;          // == kCell

        /** Message thread (processor constructor): the only allocation. */
        PeakRing() : cells (new std::atomic<float>[static_cast<std::size_t> (kCells)])
        {
            clear();
        }

        /** Message thread (Engine::prepare): zero-fill + entry width. Never reallocates. */
        void prepare (int ringSamples) noexcept
        {
            const std::int64_t perEntry = (static_cast<std::int64_t> (ringSamples) + kCellSamples * static_cast<std::int64_t> (kCells) - 1)
                                          / (kCellSamples * static_cast<std::int64_t> (kCells));
            samplesPerEntry.store (kCellSamples * static_cast<int> (std::max<std::int64_t> (1, perEntry)), std::memory_order_relaxed);
            ringSize.store (ringSamples, std::memory_order_relaxed);
            clear();
        }

        /** Audio thread: the peak of ring samples [w, w + n) (inside one entry). */
        void write (std::int64_t w, float pk) noexcept
        {
            const int P = samplesPerEntry.load (std::memory_order_relaxed);
            auto& c = cells[static_cast<std::size_t> ((w / P) % kCells)];
            if (w % P == 0)
                c.store (pk, std::memory_order_relaxed);
            else if (pk > c.load (std::memory_order_relaxed))
                c.store (pk, std::memory_order_relaxed);
        }

        /** Reader: entry `e` (absolute entry index, e >= 0). */
        float read (std::int64_t e) const noexcept
        {
            return cells[static_cast<std::size_t> (e % kCells)].load (std::memory_order_relaxed);
        }

        int getSamplesPerEntry() const noexcept { return samplesPerEntry.load (std::memory_order_relaxed); }
        int getRingSamples() const noexcept     { return ringSize.load (std::memory_order_relaxed); }

    private:
        void clear() noexcept
        {
            for (int i = 0; i < kCells; ++i)
                cells[static_cast<std::size_t> (i)].store (0.0f, std::memory_order_relaxed);
        }

        std::unique_ptr<std::atomic<float>[]> cells;
        std::atomic<int> samplesPerEntry { kCellSamples };
        std::atomic<int> ringSize { 0 };
    };

    /** D31: linear peak → 0…1, −48 dB floor (0 dBFS → 1, −30 → 0.375). */
    inline float waveDbMap (float pk) noexcept
    {
        if (! (pk > 0.0f))
            return 0.0f;
        return std::clamp ((20.0f * std::log10 (pk) + 48.0f) / 48.0f, 0.0f, 1.0f);
    }

    /** Reader (message thread): `n` bins over [head − span, head), oldest first.
        R122 absolute-cell anchoring: B = S / n cells per bin, k0 = floor ((Wc − S) / B),
        bin k = max over entries [⌊(k0 + k)·B⌋, ⌊(k0 + k + 1)·B⌋); entries >= Wc or
        older than the ring are 0. `headAnchored` = the R122 control (fractional
        bins re-anchored at the head: WV4n). Output is D31-mapped. */
    inline void binWave (const PeakRing& pr, std::int64_t head, double spanSamples, float* out, int n,
                         bool headAnchored = false) noexcept
    {
        const int P = pr.getSamplesPerEntry();
        const double Wc = static_cast<double> (head) / static_cast<double> (P);
        const double S  = std::max (1.0, spanSamples / static_cast<double> (P));
        const double B  = S / static_cast<double> (n);
        const std::int64_t wcInt   = head / P;                               // entries < wcInt are complete or partial-head
        const std::int64_t newest  = (head + P - 1) / P;                     // one past the newest written entry
        const std::int64_t oldest  = std::max<std::int64_t> (0, wcInt - pr.getRingSamples() / P);
        const std::int64_t lim     = std::min<std::int64_t> (newest, wcInt + 1);
        const double k0 = headAnchored ? (Wc - S) / B : std::floor ((Wc - S) / B);
        for (int k = 0; k < n; ++k)
        {
            std::int64_t a = static_cast<std::int64_t> (std::floor ((k0 + k) * B));
            std::int64_t b = static_cast<std::int64_t> (std::floor ((k0 + k + 1) * B));
            if (b <= a) b = a + 1;
            a = std::max (a, oldest);
            b = std::min (b, lim);
            float pk = 0.0f;
            for (std::int64_t e = a; e < b; ++e)
                pk = std::max (pk, pr.read (e));
            out[k] = waveDbMap (pk);
        }
    }

    //==========================================================================
    // 3.3 meters (PLAN-3.3 P129, R129): max-since-read accumulators. The audio
    // thread folds a block's peak in with a relaxed CAS fetch-max (once per
    // block); the reader exchanges the value with 0. A NaN `v` is a no-op
    // (`v > cur` is false), so a non-finite peak never reaches the reader.
    static_assert (std::atomic<float>::is_always_lock_free && std::atomic<std::uint32_t>::is_always_lock_free
                   && std::atomic<bool>::is_always_lock_free, "3.3 telemetry atomics must be lock-free");

    inline void pushMax (std::atomic<float>& a, float v) noexcept
    {
        float cur = a.load (std::memory_order_relaxed);
        while (v > cur && ! a.compare_exchange_weak (cur, v, std::memory_order_relaxed)) {}
    }
} // namespace ogi
