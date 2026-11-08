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

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
#include "GiConstants.h"
#include "SlotTape.h"

// ============================================================================
// Slot handoff — lock-free, allocation-free tape installation (ARCHITECTURE
// §6, CONTEXT-2.1 §6, RESEARCH-2.1 §8, PLAN-2.1 T7).
//
//   publish (off-audio):  old = pending[s].exchange (tape)
//                         old != nullptr → the audio thread never took it:
//                         it is superseded and freed by the sweep below.
//   take (audio, grid):   p = pending[s].exchange (nullptr)
//   retire (audio):       a tape the audio thread used goes into the SPSC ring
//                         (capacity 16) only once no live or fade grain holds
//                         it (the engine scans all 288 pool slots, P12).
//   sweep (off-audio):    the reaper Timer and every publish (producer-side
//                         sweep) delete what the ring holds, and free the
//                         superseded tapes.
//
// Superseded tapes use the memory pattern
// "generation reaper freezes when the host idles the audio thread": a
// deferred tape stamped with the audio EXIT count at supersession is freed
// when  quiescent (entries == exits, exits read first)  ||  exits >= stamp + 2.
// blockEntered() / blockExited() bracket every processBlock path. With the
// entry counter a host that never calls processBlock still frees every
// superseded tape at once (row H2); the H2 control (testNoEntryCounter) drops
// the quiescence clause and the deferred list grows.
//
// Retirement itself is audio-produced, so nothing the audio thread used can
// be freed while it might still read it, and pending holds at most one tape
// per slot by construction.
//
// Threads: publish / sweep / dtor take `lock` (off-audio only; host threads
// publish too — prepareToPlay re-rates, loadSlotSync). take / retire /
// blockEntered / blockExited never lock and never allocate.
// ============================================================================
namespace ogi
{
    class SlotHandoff
    {
    public:
        SlotHandoff() noexcept
        {
            for (auto& p : pending)
                p.store (nullptr, std::memory_order_relaxed);
        }

        ~SlotHandoff()
        {
            const juce::ScopedLock sl (lock);
            for (auto& p : pending)
                delete p.exchange (nullptr, std::memory_order_acq_rel);
            drainRingLocked();
            for (auto& d : deferred)
                delete d.tape;
            deferred.clear();
        }

        //======================================================================
        // Audio thread (and prepareToPlay, which never runs concurrently with it).

        /** Takes the pending tape of a slot (nullptr when none). */
        SlotTape* take (int slot) noexcept
        {
            if (slot < 0 || slot >= kTapeSlots)
                return nullptr;
            return pending[(size_t) slot].exchange (nullptr, std::memory_order_acq_rel);
        }

        /** Pushes a tape no grain holds any more. False when the ring is full
            (the caller keeps it and retries at a later grid tick). */
        bool retire (SlotTape* tape) noexcept
        {
            if (tape == nullptr)
                return true;
            const std::uint32_t w = ringWrite.load (std::memory_order_relaxed);
            const std::uint32_t r = ringRead.load (std::memory_order_acquire);
            if (w - r >= static_cast<std::uint32_t> (kRetireRingCap))
                return false;
            ring[(size_t) (w % static_cast<std::uint32_t> (kRetireRingCap))] = tape;
            ringWrite.store (w + 1u, std::memory_order_release);
            retiredCount.fetch_add (1, std::memory_order_relaxed);
            return true;
        }

        /** Top of EVERY processBlock path (memory: entries == exits ⇔ quiescent). */
        void blockEntered() noexcept { entries.fetch_add (1, std::memory_order_acq_rel); }
        /** Bottom of every processBlock path. */
        void blockExited() noexcept  { exits.fetch_add (1, std::memory_order_acq_rel); }

        //======================================================================
        // Off-audio (message / worker / host threads).

        /** Installs tape as the slot's pending tape (takes ownership). */
        void publish (int slot, SlotTape* tape)
        {
            if (slot < 0 || slot >= kTapeSlots)
            {
                delete tape;
                return;
            }
            const juce::ScopedLock sl (lock);
            SlotTape* old = pending[(size_t) slot].exchange (tape, std::memory_order_acq_rel);
            if (old != nullptr)
                deferred.push_back ({ old, exits.load (std::memory_order_acquire) });
            publishedCount.fetch_add (1, std::memory_order_relaxed);
            sweepLocked();
        }

        /** Reaper: deletes retired tapes and frees superseded ones when safe. */
        void sweep()
        {
            const juce::ScopedLock sl (lock);
            sweepLocked();
        }

        /** 1 if a tape waits for the audio thread, else 0 (row H2: <= 1). */
        int pendingDepth (int slot) const noexcept
        {
            if (slot < 0 || slot >= kTapeSlots)
                return 0;
            return pending[(size_t) slot].load (std::memory_order_acquire) != nullptr ? 1 : 0;
        }

        /** Superseded tapes not freed yet. */
        int deferredCount() const
        {
            const juce::ScopedLock sl (lock);
            return static_cast<int> (deferred.size());
        }

        std::uint64_t getRetiredCount() const noexcept    { return retiredCount.load (std::memory_order_relaxed); }
        std::uint64_t getReapedCount() const noexcept     { return reapedCount.load (std::memory_order_relaxed); }
        std::uint64_t getSupersededFreed() const noexcept { return supersededFreed.load (std::memory_order_relaxed); }
        std::uint64_t getPublishedCount() const noexcept  { return publishedCount.load (std::memory_order_relaxed); }
        std::uint64_t getBlockEntries() const noexcept    { return entries.load (std::memory_order_acquire); }
        std::uint64_t getBlockExits() const noexcept      { return exits.load (std::memory_order_acquire); }

       #if OGI_TEST_HOOKS
        /** H2 negative control: the reaper WITHOUT the entry counter (the
            +2-generation rule alone) — superseded tapes leak while the host idles. */
        std::atomic<bool> testNoEntryCounter { false };
       #endif

    private:
        struct Deferred
        {
            SlotTape*     tape  = nullptr;
            std::uint64_t stamp = 0;   // audio exit count when superseded
        };

        void drainRingLocked() noexcept
        {
            std::uint32_t r = ringRead.load (std::memory_order_relaxed);
            const std::uint32_t w = ringWrite.load (std::memory_order_acquire);
            while (r != w)
            {
                auto& slotRef = ring[(size_t) (r % static_cast<std::uint32_t> (kRetireRingCap))];
                delete slotRef;
                slotRef = nullptr;
                ++r;
                reapedCount.fetch_add (1, std::memory_order_relaxed);
            }
            ringRead.store (r, std::memory_order_release);
        }

        void sweepLocked()
        {
            drainRingLocked();

            // Read exits FIRST (memory pattern): entries >= exits always, so
            // entries == exits means no block was in flight at the entries read.
            const std::uint64_t ex = exits.load (std::memory_order_acquire);
            const std::uint64_t en = entries.load (std::memory_order_acquire);
            bool quiescent = (en == ex);
           #if OGI_TEST_HOOKS
            if (testNoEntryCounter.load (std::memory_order_relaxed))
                quiescent = false;
           #endif

            size_t keep = 0;
            for (size_t i = 0; i < deferred.size(); ++i)
            {
                const auto d = deferred[i];
                if (quiescent || ex >= d.stamp + 2)
                {
                    delete d.tape;
                    supersededFreed.fetch_add (1, std::memory_order_relaxed);
                }
                else
                {
                    deferred[keep++] = d;
                }
            }
            deferred.resize (keep);
        }

        std::array<std::atomic<SlotTape*>, kTapeSlots> pending;

        std::array<SlotTape*, kRetireRingCap> ring {};
        std::atomic<std::uint32_t> ringWrite { 0 }, ringRead { 0 };

        std::atomic<std::uint64_t> entries { 0 }, exits { 0 };
        std::atomic<std::uint64_t> retiredCount { 0 }, reapedCount { 0 }, supersededFreed { 0 }, publishedCount { 0 };

        juce::CriticalSection lock;
        std::vector<Deferred> deferred;   // guarded by lock

        JUCE_DECLARE_NON_COPYABLE (SlotHandoff)
    };
} // namespace ogi
