/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - spectrum tap: the analyzer's ring (Stage 2, Phase 2.6;
    PLAN-2.6 P2; RESEARCH-2.6 Q2)

    One single-producer / single-consumer ring of floats per tap (pre, post).
    The audio thread pushes the mono sum of each block (the processor forms
    it in a stack piece); the message thread drains it into the analyzer's
    history. Fixed storage, std::atomic indices, no framework FIFO, no lock,
    no wait, no allocation.

    Indices. writeIndex and readIndex are free-running 32-bit counts of
    samples written and read since the last reset, masked by capacity - 1 on
    access (capacity is a power of two, so the masking survives the wrap of
    the counts). writeIndex - readIndex (modulo 2^32) is the number of unread
    samples and never exceeds capacity.

    Why it is safe (SPSC). The producer alone writes writeIndex and the
    ring cells in [writeIndex, readIndex + capacity); the consumer alone
    reads the cells in [readIndex, writeIndex) and writes readIndex. The
    producer copies the samples, then publishes writeIndex with release; the
    consumer loads writeIndex with acquire, so every cell it then reads was
    written before that store. The consumer copies, then publishes readIndex
    with release; the producer loads it with acquire, so it never writes a
    cell the consumer is still reading. Neither side waits for the other.

    Overflow: newest fit. When a push does not fit in the free space, the
    NEWEST samples that fit are written (the tail of the block) and the rest
    is added to the drop counter. The ring is never overwritten under the
    reader and the producer never blocks. A host block longer than the ring
    (an offline bounce with blocks above 32 768 samples while the analyzer is
    open) then still delivers its last 32 768 samples instead of being
    dropped whole every time. The drop counter only grows (modulo 2^32); the
    reader that sees it move discards its history, so no analysis window
    straddles a gap. It is incremented before writeIndex is published, so a
    drain that loads it after writeIndex sees every drop that precedes the
    samples it copies. That guarantee holds for a drain that takes
    everything available (maxN >= capacity): a gap inside its run was then
    counted before its writeIndex load. A PARTIAL drain can report the moved
    counter while the gap still lies in the unread part, and the next drain
    then returns the gap with an unmoved counter; a reader that applies the
    rule above drains everything each time (or treats the counter as moved
    until it has drained everything).

    Reset and the generation. reset() is called from prepareToPlay only. The
    host does not run prepareToPlay concurrently with processBlock, so reset
    has no concurrent producer; but it can run on another thread than the
    reader's, while a drain is in flight. Storing readIndex = writeIndex is
    therefore not enough: the reader could publish its own stale readIndex
    after it. reset stores writeIndex = 0, then the read state = (generation
    + 1, readIndex 0), and publishes the bumped generation last, with release.

    The read state packs the generation (high 32 bits) and readIndex (low 32
    bits) into one 64-bit atomic. A drain publishes its readIndex with a
    compare-exchange against the read state it started from, so a reset that
    lands anywhere inside the drain makes the exchange fail (the generation
    differs; a reset cannot repeat a generation short of 2^32 prepares): the
    reset's indices stand, and the drain returns 0 samples and the new
    generation. A drain whose generation moved therefore returns no sample,
    and its caller, seeing the generation move, discards its history. This is
    the one point where this class extends PLAN-2.6 P2 (separate 32-bit
    readIndex and generation atomics): with those, a drain that loaded the
    generation, then lost the processor to a reset, then stored its readIndex
    would leave readIndex ahead of writeIndex for good.

    Why the storage is fixed at construction. The ring is a member array
    (capacity floats), never sized in prepare. A host may call prepareToPlay
    on another thread than the one running the reader; re-sizing a ring the
    reader is draining would be a use after free. With fixed storage prepare
    only resets the indices and bumps the generation; the rate the reader
    needs is published by the processor separately.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace eq
{

class SpectrumTap
{
public:
    /** Samples the ring holds: a power of two. 170 ms at 192 kHz. */
    static constexpr int capacity = 32768;

    static_assert ((capacity & (capacity - 1)) == 0, "capacity must be a power of two");
    static_assert (std::atomic<std::uint32_t>::is_always_lock_free, "the indices must be lock-free");
    static_assert (std::atomic<std::uint64_t>::is_always_lock_free, "the read state must be lock-free");

    //==========================================================================
    // Producer (the audio thread)
    //==========================================================================

    /** Appends n samples. When they do not all fit, the newest that fit are
        written and the rest counted as dropped. Never blocks. */
    void push (const float* x, int n) noexcept
    {
        if (n <= 0 || x == nullptr)
            return;

        const std::uint32_t w = writeIndex.load (std::memory_order_relaxed);
        const std::uint32_t r = readIndexOf (readState.load (std::memory_order_acquire));
        const std::uint32_t used = w - r;
        const std::uint32_t space = (used < ucapacity) ? ucapacity - used : 0u;

        std::uint32_t count = static_cast<std::uint32_t> (n);
        const float* src = x;

        if (count > space)
        {
            const std::uint32_t lost = count - space;
            drops.fetch_add (lost, std::memory_order_relaxed);
            src += lost;
            count = space;
        }

        if (count == 0u)
            return;

        copyIn (w, src, count);
        writeIndex.store (w + count, std::memory_order_release);
    }

    //==========================================================================
    // Consumer (the message thread)
    //==========================================================================

    /** Copies up to maxN of the oldest unread samples into dest and marks
        them read. Returns the number copied. genOut receives the generation
        the samples belong to, dropsOut the drop counter (loaded after
        writeIndex). A reset that lands inside the drain: 0 samples, and
        genOut is the bumped generation. */
    int drain (float* dest, int maxN, std::uint32_t& genOut, std::uint32_t& dropsOut) noexcept
    {
        std::uint64_t state = readState.load (std::memory_order_acquire);
        const std::uint32_t gen = generationOf (state);
        const std::uint32_t r = readIndexOf (state);
        const std::uint32_t w = writeIndex.load (std::memory_order_acquire);

        dropsOut = drops.load (std::memory_order_relaxed);
        genOut = gen;

        std::uint32_t available = w - r;

        // Larger than the ring only on a view torn by a reset: the exchange below fails.
        if (available > ucapacity)
            available = 0u;

        std::uint32_t count = available;

        if (maxN <= 0 || dest == nullptr)
            count = 0u;
        else if (count > static_cast<std::uint32_t> (maxN))
            count = static_cast<std::uint32_t> (maxN);

        copyOut (r, dest, count);

        const std::uint64_t next = packState (gen, r + count);

        if (! readState.compare_exchange_strong (state, next, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            // A reset ran during the drain: its indices stand, nothing is returned.
            genOut = generationOf (state);
            return 0;
        }

        if (generation.load (std::memory_order_acquire) != gen)
        {
            genOut = generation.load (std::memory_order_acquire);
            return 0;
        }

        return static_cast<int> (count);
    }

    //==========================================================================
    // Prepare (no producer running)
    //==========================================================================

    /** Empties the ring: writeIndex 0, then readIndex 0 with the generation
        bumped (one store), then the generation published with release. For
        prepareToPlay only. The drop counter is not reset. */
    void reset() noexcept
    {
        const std::uint32_t nextGen = generationOf (readState.load (std::memory_order_acquire)) + 1u;

        writeIndex.store (0u, std::memory_order_release);
        readState.store (packState (nextGen, 0u), std::memory_order_release);
        generation.store (nextGen, std::memory_order_release);
    }

    //==========================================================================
    // Read only, any thread (for the reader's checks and the harness)
    //==========================================================================

    std::uint32_t getWriteIndex() const noexcept    { return writeIndex.load (std::memory_order_acquire); }
    std::uint32_t getReadIndex() const noexcept     { return readIndexOf (readState.load (std::memory_order_acquire)); }
    std::uint32_t getGeneration() const noexcept    { return generation.load (std::memory_order_acquire); }
    std::uint32_t getDropCount() const noexcept     { return drops.load (std::memory_order_acquire); }

    /** Unread samples (a snapshot; 0 on a view torn by a reset). */
    int getNumAvailable() const noexcept
    {
        const std::uint32_t r = getReadIndex();
        const std::uint32_t w = getWriteIndex();
        const std::uint32_t available = w - r;
        return (available > ucapacity) ? 0 : static_cast<int> (available);
    }

private:
    static constexpr std::uint32_t ucapacity = static_cast<std::uint32_t> (capacity);
    static constexpr std::uint32_t mask = ucapacity - 1u;

    static constexpr std::uint32_t generationOf (std::uint64_t state) noexcept
    {
        return static_cast<std::uint32_t> (state >> 32);
    }

    static constexpr std::uint32_t readIndexOf (std::uint64_t state) noexcept
    {
        return static_cast<std::uint32_t> (state & 0xffffffffu);
    }

    static constexpr std::uint64_t packState (std::uint32_t gen, std::uint32_t index) noexcept
    {
        return (static_cast<std::uint64_t> (gen) << 32) | static_cast<std::uint64_t> (index);
    }

    /** count samples from src into the ring from free-running position at,
        in at most two runs. */
    void copyIn (std::uint32_t at, const float* src, std::uint32_t count) noexcept
    {
        const std::uint32_t start = at & mask;
        const std::uint32_t first = (count < ucapacity - start) ? count : ucapacity - start;

        for (std::uint32_t i = 0; i < first; ++i)
            ring[start + i] = src[i];

        for (std::uint32_t i = first; i < count; ++i)
            ring[i - first] = src[i];
    }

    /** count samples from the ring at free-running position at into dest. */
    void copyOut (std::uint32_t at, float* dest, std::uint32_t count) const noexcept
    {
        const std::uint32_t start = at & mask;
        const std::uint32_t first = (count < ucapacity - start) ? count : ucapacity - start;

        for (std::uint32_t i = 0; i < first; ++i)
            dest[i] = ring[start + i];

        for (std::uint32_t i = first; i < count; ++i)
            dest[i] = ring[i - first];
    }

    float ring[capacity] = {};

    std::atomic<std::uint32_t> writeIndex { 0u };
    std::atomic<std::uint64_t> readState { 0u };     // generation (high 32) : readIndex (low 32)
    std::atomic<std::uint32_t> generation { 0u };
    std::atomic<std::uint32_t> drops { 0u };
};

} // namespace eq
