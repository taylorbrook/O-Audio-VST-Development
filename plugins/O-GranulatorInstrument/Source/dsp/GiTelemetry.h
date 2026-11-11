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
//   scInPeak / outPeakL / R max |x| of the raw sidechain / the output L, R since read
//   stolen / stolenCount    PLAN-3.3 PD2 steal ring (serials of stolen voices)
//   headPos[4]              per-slot record-head position (tape frames)
//   swapState[4]            bit 0 pending tape, 1 crossfade, 2 head parked,
//                           3 swap waiting for the head
//   overview                per slot 5625 bins × {min, max} the record head
//                           rewrote, tagged with the slot's tapeEpoch (bumped
//                           at each new tape). A bin is valid only while
//                           seq[bin] == tapeEpoch[s]; else the reader uses the
//                           loader's pristine overview. Constructor-allocated,
//                           never reallocated (the PeakRing rule).
//   ovWrites[4]             per-slot count of foldOverview calls (PLAN-3.2 PD6):
//                           the reader folds it into ovRev so a lane redraws
//                           while the head rewrites its tape. Never read by
//                           the engine.
//   frames                  GiUiFrame: ≤ 288 grains + 20 voices (+ newestSerial, 3.3).
// ============================================================================
namespace ogi
{
    struct GiUiGrain
    {
        std::uint8_t voice = 0, stream = 0, flags = 0;   // stream 0-based; flags: 1 reversed, 2 killing / fade
        std::uint8_t windowStep = 0;                      // Grain::windowStep (0..64)
        float        normPos = 0.0f;                      // read position / tape frames (0..1)
        float        gain = 0.0f;                         // the kill gain (Grain::killGain)
        // PLAN-3.2 PD1: plain copies of the Grain's fields (the audio thread
        // does no math beyond `phase`); pan / level / envelope / length are
        // derived by the message-thread reader (Engine::grainWindow for env).
        float        gainL = 1.0f, gainR = 1.0f;          // spread pan × gain_rand (NO stream pan: that is on the bus)
        float        phase = 0.0f;                        // (age + frac) · invLen
        float        lenSamples = 0.0f;                   // Grain::len (output samples)
        float        tiltT = 0.5f, tiltA = 1.0f, tiltB = 1.0f;   // Grain::tiltT / A / B
    };

    // PLAN-3.3 PD3: sizes of the per-voice mod blocks the frame carries (the
    // engine's kGiLfos / kGiModEnvs; Engine.cpp static_asserts the equality —
    // this header does not pull the forked ControlGrid in).
    inline constexpr int kUiLfos = 4, kUiModEnvs = 2;

    struct GiUiVoice
    {
        std::int8_t note = -1;          // −1 unless counted (3.2 meaning kept)
        float       amp = 0.0f;         // envelope level
        float       scan[kGiStreams] {};   // scan centre / tape frames (0..1), per stream
        // PLAN-3.3 PD3 (written in publishTelemetry only; never read by the DSP)
        std::uint8_t  state = 0;        // 0 free · 1 gated (incl. CC64-held) · 2 released (counted, gate closed) · 3 stolen
        std::uint8_t  ampStage = 0;     // page stages: 0 off · 1 A · 2 D · 3 S · 4 R (forced → 4)
        std::uint8_t  menvStage[kUiModEnvs] {};
        std::uint64_t serial = 0;       // the slot's note serial (0 when free)
        float         menv[kUiModEnvs] {};   // Mod Env 1 / 2 level
        float         lfoPh[kUiLfos] {};     // per-voice LFO phase 0..1 (offset removed); −1 = not per-voice / free
        float         wantSt[kGiStreams] {}; // voiceSemis before the ±48 clamp (counted voice + stream with a tape), else 0
    };

    struct GiUiFrame
    {
        static constexpr int kMaxGrains = GiPool::kNumSlots;   // 288
        std::uint64_t seqHead = 0;      // written FIRST
        std::int32_t  count = 0;        // grains[0, count)
        GiUiGrain     grains[kMaxGrains] {};
        GiUiVoice     voices[kVoiceSlots] {};
        std::uint64_t newestSerial = 0; // PLAN-3.3 PD3: the engine's newestSer (PD8 rule, 2.5b)
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
        // PLAN-3.3 PD5: the output peak per side (replaces the 2.6 outPeak =
        // max (|L|, |R|)); post output_gain, max since read.
        std::atomic<float> scInPeak { 0.0f }, outPeakL { 0.0f }, outPeakR { 0.0f };

        // PLAN-3.3 PD2: the steal ring. Written ONLY in Engine::stealVoice
        // (audio thread): the victim's serial at stolenCount % kStealRing
        // (relaxed), then stolenCount + 1 (release). Polyphony shrink, All
        // Sound Off and mono handovers are not steals and do not write it.
        // ONE reader (the processor's buildUiTelemetry) keeps its own cursor.
        static constexpr int kStealRing = 16;
        std::array<std::atomic<std::uint64_t>, kStealRing> stolen {};
        std::atomic<std::uint32_t> stolenCount { 0 };
        std::array<std::atomic<std::int64_t>, kTapeSlots> headPos {};
        std::array<std::atomic<std::uint8_t>, kTapeSlots> swapState {};
        GiOverviewLive overview;
        // PLAN-3.2 PD6: +1 per foldOverview call that had a tape (audio thread,
        // relaxed). Wraps; the reader only needs "moved".
        std::array<std::atomic<std::uint32_t>, kTapeSlots> ovWrites {};
        TripleBuffer<GiUiFrame> frames;

        // The record head's running bin accumulator per slot (audio thread only).
        struct BinAcc { int bin = -1; float lo = 0.0f, hi = 0.0f; };
        BinAcc acc[kTapeSlots];
        std::uint64_t seq = 0;   // frame sequence (audio thread)

        /** Reader helpers (any thread). */
        float takeScInPeak() noexcept { return scInPeak.exchange (0.0f, std::memory_order_acq_rel); }
        /** PLAN-3.3 PD5: ONE caller, the processor's buildUiTelemetry. */
        void takeOutPeaks (float& l, float& r) noexcept
        {
            l = outPeakL.exchange (0.0f, std::memory_order_acq_rel);
            r = outPeakR.exchange (0.0f, std::memory_order_acq_rel);
        }
        /** Writer (audio thread, Engine::stealVoice only). */
        void pushSteal (std::uint64_t serial) noexcept
        {
            const std::uint32_t c = stolenCount.load (std::memory_order_relaxed);
            stolen[(size_t) (c % (std::uint32_t) kStealRing)].store (serial, std::memory_order_relaxed);
            stolenCount.store (c + 1u, std::memory_order_release);
        }
        std::uint32_t getOvWrites (int s) const noexcept
        {
            return (s >= 0 && s < kTapeSlots) ? ovWrites[(size_t) s].load (std::memory_order_relaxed) : 0u;
        }
    };
} // namespace ogi
