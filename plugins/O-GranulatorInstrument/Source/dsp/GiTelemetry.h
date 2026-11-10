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

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <type_traits>

#include "GiConstants.h"
#include "VoiceGrain.h"
#include "forked/UiTelemetry.h"   // VERBATIM (PLAN-2.6 PD15): TripleBuffer, pushMax

// ============================================================================
// GiTelemetry — the 2.6 engine-side feeds for Stage 3 (CONTEXT-2.6 Defaults,
// PLAN-2.6 T11 / PD15, RESEARCH-2.6 §8). The engine only WRITES these; nothing
// here is read by the DSP. Lock-free: relaxed atomics, max-since-read
// accumulators (pushMax + reader exchange (0)), and one TripleBuffer frame per
// host block (the reader always gets a complete frame; seqHead == seqTail).
//
//   recState / progress     GiRecorder (getPackedState, frames / target)
//   scInPeak / outPeak      max |x| of the raw sidechain / the output since read
//   headPos[4]              per-slot record-head position (tape frames)
//   swapState[4]            bit 0 pending tape, 1 crossfade, 2 head parked,
//                           3 swap waiting for the head
//   overview                per slot 5625 bins × {min, max} the record head
//                           rewrote, tagged with the slot's tapeEpoch (bumped
//                           at each new tape). A bin is valid only while
//                           seq[bin] == tapeEpoch[s]; else the reader uses the
//                           loader's pristine overview. Constructor-allocated,
//                           never reallocated (the PeakRing rule).
//   frames                  GiUiFrame: ≤ 288 grains + 20 voices.
// ============================================================================
namespace ogi
{
    struct GiUiGrain
    {
        std::uint8_t voice = 0, stream = 0, flags = 0;   // flags: 1 reversed, 2 killing / fade
        float        normPos = 0.0f;                      // read position / tape frames (0..1)
        float        gain = 0.0f;                         // window × kill gain (display)
    };

    struct GiUiVoice
    {
        std::int8_t note = -1;          // −1 = free
        float       amp = 0.0f;         // envelope level
        float       scan[kGiStreams] {};   // scan centre / tape frames (0..1), per stream
    };

    struct GiUiFrame
    {
        static constexpr int kMaxGrains = GiPool::kNumSlots;   // 288
        std::uint64_t seqHead = 0;      // written FIRST
        std::int32_t  count = 0;        // grains[0, count)
        GiUiGrain     grains[kMaxGrains] {};
        GiUiVoice     voices[kVoiceSlots] {};
        std::uint64_t seqTail = 0;      // written LAST
    };
    static_assert (std::is_trivially_copyable_v<GiUiFrame>, "TripleBuffer frames are PODs");

    struct GiOverviewLive
    {
        static constexpr int kBins = (kSlotMaxFrames + kOverviewBin - 1) / kOverviewBin;   // 5625

        GiOverviewLive()
        {
            for (int s = 0; s < kTapeSlots; ++s)
            {
                mn[s].reset (new std::atomic<float>[(size_t) kBins]);
                mx[s].reset (new std::atomic<float>[(size_t) kBins]);
                seq[s].reset (new std::atomic<std::uint32_t>[(size_t) kBins]);
                for (int b = 0; b < kBins; ++b)
                {
                    mn[s][(size_t) b].store (0.0f, std::memory_order_relaxed);
                    mx[s][(size_t) b].store (0.0f, std::memory_order_relaxed);
                    seq[s][(size_t) b].store (0, std::memory_order_relaxed);
                }
                tapeEpoch[s].store (1, std::memory_order_relaxed);
            }
        }

        /** Writer (audio): a new tape in slot s — every bin goes stale. */
        void bumpEpoch (int s) noexcept { tapeEpoch[s].fetch_add (1, std::memory_order_release); }

        /** Writer (audio): bin b of slot s now holds [lo, hi] for this epoch. */
        void write (int s, int b, float lo, float hi) noexcept
        {
            if (b < 0 || b >= kBins)
                return;
            mn[s][(size_t) b].store (lo, std::memory_order_relaxed);
            mx[s][(size_t) b].store (hi, std::memory_order_relaxed);
            seq[s][(size_t) b].store (tapeEpoch[s].load (std::memory_order_relaxed), std::memory_order_release);
        }

        /** Reader: true + the bin when it belongs to the slot's current tape. */
        bool read (int s, int b, float& lo, float& hi) const noexcept
        {
            if (s < 0 || s >= kTapeSlots || b < 0 || b >= kBins)
                return false;
            if (seq[s][(size_t) b].load (std::memory_order_acquire) != tapeEpoch[s].load (std::memory_order_acquire))
                return false;   // stale epoch: the pristine overview stands
            lo = mn[s][(size_t) b].load (std::memory_order_relaxed);
            hi = mx[s][(size_t) b].load (std::memory_order_relaxed);
            return true;
        }

        std::uint32_t epoch (int s) const noexcept { return tapeEpoch[s].load (std::memory_order_acquire); }

    private:
        std::unique_ptr<std::atomic<float>[]> mn[kTapeSlots], mx[kTapeSlots];
        std::unique_ptr<std::atomic<std::uint32_t>[]> seq[kTapeSlots];
        std::atomic<std::uint32_t> tapeEpoch[kTapeSlots];
    };

    struct GiTelemetry
    {
        std::atomic<float> scInPeak { 0.0f }, outPeak { 0.0f };
        std::array<std::atomic<std::int64_t>, kTapeSlots> headPos {};
        std::array<std::atomic<std::uint8_t>, kTapeSlots> swapState {};
        GiOverviewLive overview;
        TripleBuffer<GiUiFrame> frames;

        // The record head's running bin accumulator per slot (audio thread only).
        struct BinAcc { int bin = -1; float lo = 0.0f, hi = 0.0f; };
        BinAcc acc[kTapeSlots];
        std::uint64_t seq = 0;   // frame sequence (audio thread)

        /** Reader helpers (any thread). */
        float takeScInPeak() noexcept { return scInPeak.exchange (0.0f, std::memory_order_acq_rel); }
        float takeOutPeak() noexcept  { return outPeak.exchange (0.0f, std::memory_order_acq_rel); }
    };
} // namespace ogi
