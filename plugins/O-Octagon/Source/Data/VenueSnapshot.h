/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#include <type_traits>

#include "../DSP/Vec.h"

namespace oo
{

//==============================================================================
/**
    Everything the audio thread needs to know about the room, as one flat POD (~280 bytes).

    Venue edits and hull rebuilds happen on the message thread. The audio thread must never touch
    the ValueTree, never allocate, and never lock — it reads this (ARCHITECTURE §3.6.6).
*/
struct VenueSnapshot
{
    std::array<Vec3,  8> spk {};                ///< speaker positions, metres, 3D
    std::array<float, 8> trimLin {};            ///< per-speaker trim as a linear factor

    /** v1.4.0 — per-speaker alignment delay, in MILLISECONDS, already railed to [0, 50].

        MILLISECONDS AND NOT SAMPLES, deliberately. publishSnapshot() is the single funnel and it
        does not know the sample rate — prepareToPlay() owns that, and a snapshot published before
        the first prepareToPlay() (setStateInformation can arrive first) would have nothing to
        convert with. GainStage already holds `sampleRate` as a member for the air filter's Nyquist
        clamp, so the conversion happens there, once, at the control boundary. Publishing samples
        would also make every stored snapshot silently wrong the moment the host changed rate.
    */
    std::array<float, 8> delayMs {};

    /// speaker n → output buffer index. THE ONLY THING IN THIS PLUGIN THAT INDEXES AN OUTPUT
    /// CHANNEL. Valid to use only when the processor's mappedOutputAvailable() says so.
    std::array<int, 8>   speakerToBuffer { 0, 1, 2, 3, 4, 5, 6, 7 };

    /** v1.7.0 — the two out[] SLOTS whose buffer channel carries ChannelType left / right.

        SLOTS INTO out[], NOT BUFFER CHANNELS, and the distinction is the whole reason this is a
        field rather than a lookup. renderChunk()'s out[i] is already
        buffer.getWritePointer (speakerToBuffer[i]), so the monitor fold writes through the SAME
        pointers every other write in this plugin goes through and adds no second output-indexing
        site. Resolved on the message thread by ochan::resolveMonitorSlots().

        NOT SPEAKERS 1 AND 2. The monitor has to reach the physical outputs a headphone amp is
        plugged into, which under the measured CoreAudio device order (OutputOrder.h) are outputs
        1-2 = left/right. Speakers 1 and 2 land wherever the venue's labels put them, which for any
        non-default wiring is somewhere else entirely.

        { -1, -1 } when the pair cannot be resolved. The monitor is then REFUSED, never guessed —
        an unresolvable pair is the same class of failure as an invalid map and gets the same
        treatment. */
    std::array<int, 2>   monitorSlot { -1, -1 };

    std::array<Vec2, 8>  hullPts {};
    int                  hullCount { 0 };

    /// Room-scaled area tolerance for the hull tests. NOT in ARCHITECTURE §3.6.6's field list, but
    /// required: hull::isInside() takes it, and recomputing it on the audio thread from the bbox
    /// would be a second derivation of a message-thread quantity — exactly the drift this snapshot
    /// exists to prevent.
    float hullEpsCross { 0.0f };

    Vec3  centroid {};
    float rigScale { 0.0f };

    float bbMinX { 0.0f }, bbMaxX { 0.0f }, bbMinY { 0.0f }, bbMaxY { 0.0f };
    float rakeFront { 0.0f }, rakeRear { 0.0f };

    /** Venue publication counter, STAMPED BY publish() — the solver's dirty check reads this and
        nothing else (PLAN-2.2 P16 / RESEARCH-2.2 H1).

        ── Why this is a field and not the separate atomic it used to be ──────────────────────────
        Publishing the slot and the generation as two atomics means the audio thread performs two
        separate acquires. A publish() landing between them hands the control block the NEW geometry
        with the OLD generation. That generation is then stored as lastSolvedGeneration and every
        subsequent block compares EQUAL — the venue edit is present in the snapshot and the solve
        never runs against it, PERMANENTLY. Reversing the read order only swaps which half is stale.

        Carried inside the payload, one acquire delivers data and generation together and the failure
        is structurally unreachable.

        NOT in ARCHITECTURE §3.6.6's field list — the same class of deviation as `hullEpsCross`, and
        recorded as one in SUMMARY-2.2. Probe AQ measures it by publishing a venue edit BETWEEN two
        processBlock calls; no probe that edits the venue while audio is stopped can see it.
    */
    std::uint32_t generation { 0 };
};

static_assert (std::is_trivially_copyable_v<VenueSnapshot>,
               "VenueSnapshot is copied into a double-buffer slot by assignment on the message "
               "thread and read by the audio thread. A non-trivial member would introduce a "
               "constructor, a destructor, or worse an allocation into that path.");

//==============================================================================
/**
    Lock-free single-writer publication of a VenueSnapshot.

    Two slots plus an atomic index. The message thread fills the INACTIVE slot and then releases the
    new index; the audio thread acquires the index ONCE PER CONTROL BLOCK and holds it for the whole
    block, so the geometry cannot change underneath a partially-computed gain vector.

    ── Why not std::atomic<std::shared_ptr<VenueSnapshot>> ───────────────────────────────────────
    Because a reference-count decrement is a potential `free`, and a `free` on the audio thread is
    the failure in `pattern_retired_map_reaper_rt_free`. The snapshot is small and POD; a double
    buffer costs 2 × ~250 bytes and cannot deallocate anything. DO NOT substitute a shared_ptr here.

    ABA is not reachable at venue-edit rates: there is exactly one writer, edits come from human
    typing on the message thread, and a slot is only overwritten after a full release/acquire cycle
    has published the other one.
*/
class VenueSnapshotPublisher
{
public:
    VenueSnapshotPublisher() = default;

    /** Message thread only. Fills the inactive slot, stamps its generation, then publishes it. */
    void publish (const VenueSnapshot& newSnapshot) noexcept
    {
        const int target = 1 - activeSlot.load (std::memory_order_relaxed);

        slots[(size_t) target] = newSnapshot;

        // Stamped INSIDE the payload, before the release store. publishCounter is a plain integer
        // because only the message thread ever touches it — the audio thread reads the copy that
        // rode across the release/acquire edge (P16).
        slots[(size_t) target].generation = ++publishCounter;

        // ONE release, pairing with the ONE acquire in read(): every write to slots[target] above —
        // geometry and generation alike — is visible to a thread that observes this store.
        activeSlot.store (target, std::memory_order_release);
    }

    /** Audio thread. Call ONCE per control block and hold the reference for the whole block —
        re-reading mid-block would let the geometry change between two speakers of one gain vector. */
    const VenueSnapshot& read() const noexcept
    {
        return slots[(size_t) activeSlot.load (std::memory_order_acquire)];
    }

    /** How many snapshots have been published. MESSAGE THREAD / DIAGNOSTICS ONLY.

        THE DIRTY CHECK MUST NOT READ THIS. It reads `snapshot.generation`, which arrives with the
        geometry it belongs to. Reading a counter separately from the payload is the H1 bug this
        class was changed to make unreachable — see VenueSnapshot::generation.
    */
    std::uint32_t getGeneration() const noexcept
    {
        return publishCounter;
    }

private:
    std::array<VenueSnapshot, 2> slots {};
    std::atomic<int>             activeSlot { 0 };

    /// Message thread only, deliberately not atomic — see publish().
    std::uint32_t publishCounter { 0 };
};

} // namespace oo
