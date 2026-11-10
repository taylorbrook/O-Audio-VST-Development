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

#include <atomic>
#include <cstdint>

// ============================================================================
// GiTapeSnapshot — the Save-recording copy-out request (PLAN-3.2 PD18, D46,
// RESEARCH-3.2 §7 option A). The slot's working copy is audio-owned, so the
// AUDIO thread copies level 0 of the tape into a buffer the OWNER allocated, at
// most kChunkFrames frames per Engine::process / bypassedBlock call. One reader
// and one writer of the tape (the audio thread): no tear, no tape lifetime
// question.
//
// The ENGINE owns this object (Engine::tapeSnapshot()); it lives as long as the
// engine. The OWNER (the processor's save flow: its worker thread, and the
// message thread in prepareToPlay / releaseResources / the destructor) owns the
// two sample buffers. The engine only ever holds the raw pointers.
//
// States (one atomic word) and who may write each transition:
//
//   Idle ──request()──▶ Arming ──▶ Requested          OWNER (any one thread; a
//                                                     losing concurrent caller
//                                                     gets false = busy)
//   Requested ──▶ Copying                             AUDIO (tape + epoch captured)
//   Requested ──▶ Aborted                             AUDIO (no tape in the slot, or
//                                                     its frame count != frames)
//   Copying ──▶ Copying (chunk, progress + 1)         AUDIO
//   Copying ──▶ Done                                  AUDIO (all frames copied)
//   Copying ──▶ Aborted                               AUDIO (the slot's tape pointer,
//                                                     frame count or tape epoch
//                                                     changed: take, clear, reset)
//   Requested | Copying ──cancel()──▶ CancelRequested OWNER
//   CancelRequested ──▶ Cancelled                     AUDIO (the acknowledgement)
//   Done | Aborted | Cancelled ──release()──▶ Idle    OWNER
//   any ──resetAudioStopped()──▶ Idle                 OWNER, audio STOPPED only
//
// Done / Aborted / Cancelled are TERMINAL: the audio thread wrote them as its
// last access and never touches L / R again until a new Requested. The audio
// thread leaves Requested / Copying only through a compare-exchange, so a
// cancel() that lands while it is copying is never overwritten: the call that
// was copying ends in Cancelled (even if it had just copied the last frame).
// After cancel() returned true the only terminal state is Cancelled.
//
// BUFFER LIFETIME INVARIANT (PLAN-3.2 PD18). L and R may be freed only when
//   (a) release() returned true (the state was terminal: the audio thread has
//       acknowledged Done, Aborted or the cancel), or
//   (b) resetAudioStopped() was called while the audio callback is guaranteed
//       not to run (prepareToPlay, releaseResources, the processor destructor)
//       AND no other owner thread is inside request() / cancel() / release()
//       (stop or join the save worker first, or serialise with it).
// Never on a timeout alone. With the host idle (no process calls) a request
// stays in Requested / Copying / CancelRequested: the owner keeps the buffer
// allocated, request() keeps answering false (the processor's `busy`), and the
// pending acknowledgement arrives with the first block after audio resumes;
// then release() succeeds. The timeout itself keys on progress() (PD19: no
// movement for 2000 ms), not on wall time since the request.
//
// Owner sequence (worker):
//   allocate L, R (frames = SlotInfo::frames)  →  request (slot, L, R, frames)
//   poll state() / progress():
//     Done     → release(); write the WAV from L / R; free
//     Aborted  → release(); free; answer `changed`
//     timeout  → cancel(); wait (bounded) for isTerminal (state());
//                terminal → release(); free; else KEEP the buffer and retry
//                release() later (or free under rule (b)); answer `timeout`
// isTerminal (state()) after a timeout may read Done or Aborted instead of
// Cancelled when the audio thread finished before cancel() landed (cancel()
// then returned false); release() handles all three.
//
// Audio thread cost: one relaxed load per call while nothing is in flight; no
// allocation, no lock, no logging. The tape is only read.
// ============================================================================
namespace ogi
{
    struct SlotTape;

    struct GiTapeSnapshot
    {
        static constexpr int kChunkFrames = 16384;   // level-0 frames per process call (PD18)

        enum State : std::uint32_t
        {
            Idle = 0,
            Arming,            // owner is writing the request fields (the audio thread ignores it)
            Requested,
            Copying,
            CancelRequested,
            Done,              // terminal: L / R hold [0, frames) of level 0
            Aborted,           // terminal: `changed` (contents of L / R unspecified)
            Cancelled          // terminal: the cancel was acknowledged
        };

        static constexpr bool isTerminal (State s) noexcept { return s == Done || s == Aborted || s == Cancelled; }

        //======================================================================
        // OWNER side (worker / message thread). Never the audio thread.

        /** Idle → Requested. L and R must each hold `numFrames` floats and stay
            allocated per the lifetime invariant. False = not Idle (busy) or bad
            arguments; nothing is retained then. */
        bool request (int slot0, float* bufL, float* bufR, int numFrames) noexcept
        {
            if (bufL == nullptr || bufR == nullptr || numFrames <= 0)
                return false;
            auto expected = static_cast<std::uint32_t> (Idle);
            if (! state_.compare_exchange_strong (expected, Arming, std::memory_order_acq_rel, std::memory_order_acquire))
                return false;
            slot = slot0;
            L = bufL;
            R = bufR;
            frames = numFrames;
            progress_.store (0, std::memory_order_relaxed);
            state_.store (Requested, std::memory_order_release);   // publishes the fields
            return true;
        }

        /** Requested | Copying → CancelRequested. True = the audio thread now
            owes a Cancelled; false = nothing was in flight (the state is Idle,
            terminal, or a cancel is already pending). */
        bool cancel() noexcept
        {
            auto s = state_.load (std::memory_order_acquire);
            while (s == Requested || s == Copying)
                if (state_.compare_exchange_weak (s, CancelRequested, std::memory_order_acq_rel, std::memory_order_acquire))
                    return true;
            return false;
        }

        /** Terminal → Idle. True = released: L / R are the owner's again (free
            or reuse). False = not terminal (the audio thread may still hold the
            buffers, or nothing was requested). */
        bool release() noexcept
        {
            auto s = state_.load (std::memory_order_acquire);
            while (isTerminal (static_cast<State> (s)))
                if (state_.compare_exchange_weak (s, Idle, std::memory_order_acq_rel, std::memory_order_acquire))
                    return true;
            return false;
        }

        /** Any state → Idle. ONLY with the audio callback stopped
            (prepareToPlay / releaseResources / destructor) and no other owner
            thread inside this object — lifetime rule (b). */
        void resetAudioStopped() noexcept { state_.store (Idle, std::memory_order_release); }

        /** Any thread. An acquire load: a Done read here sees the copied samples. */
        State state() const noexcept { return static_cast<State> (state_.load (std::memory_order_acquire)); }

        /** Any thread: +1 per chunk the audio thread copied since request()
            (the PD19 no-progress timeout keys on this). */
        std::uint32_t progress() const noexcept { return progress_.load (std::memory_order_relaxed); }

        //======================================================================
        // AUDIO side (Engine::serviceTapeSnapshot only).

        /** The one test per block while nothing is in flight. */
        bool audioActive() const noexcept
        {
            const auto s = state_.load (std::memory_order_relaxed);
            return s == Requested || s == Copying || s == CancelRequested;
        }

        /** Requested → Copying (false: a cancel landed; Cancelled was written). */
        bool audioBegin() noexcept { return audioMove (Requested, Copying); }

        /** `from` → `to` by compare-exchange; when the owner moved the state to
            CancelRequested meanwhile, writes the Cancelled acknowledgement and
            returns false. */
        bool audioMove (State from, State to) noexcept
        {
            auto expected = static_cast<std::uint32_t> (from);
            if (state_.compare_exchange_strong (expected, to, std::memory_order_acq_rel, std::memory_order_acquire))
                return true;
            if (expected == CancelRequested)
                state_.store (Cancelled, std::memory_order_release);
            return false;
        }

        void audioAckCancel() noexcept
        {
            auto expected = static_cast<std::uint32_t> (CancelRequested);
            state_.compare_exchange_strong (expected, Cancelled, std::memory_order_acq_rel, std::memory_order_acquire);
        }

        void audioBumpProgress() noexcept { progress_.fetch_add (1, std::memory_order_relaxed); }

        // Request fields: written by the owner in Arming, read by the audio
        // thread from Requested on (published by the Requested store).
        int    slot = 0;
        float* L = nullptr;
        float* R = nullptr;
        int    frames = 0;

        // Audio thread only (valid in Copying).
        const SlotTape* tape = nullptr;      // the slot's tape at Requested → Copying
        std::uint32_t   tapeEpoch = 0;       // GiOverviewLive::epoch (slot) at that moment
        int             cursor = 0;          // next level-0 frame to copy

    private:
        std::atomic<std::uint32_t> state_ { Idle };
        std::atomic<std::uint32_t> progress_ { 0 };
    };
} // namespace ogi
