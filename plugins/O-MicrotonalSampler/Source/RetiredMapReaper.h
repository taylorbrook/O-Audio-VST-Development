/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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

    RetiredMapReaper.h
    Microtonal Sample Engine - off-audio-thread SampleMap reaper
    Ouaricon Audio
    Developer: Taylor Brook

    v1.23.2 — W10 (REVIEW WR-02). Retiring a SampleMap can free hundreds of MB:
    its `std::vector<SampleCell>`, every `SampleVariant`, and every
    `shared_ptr<AudioBuffer<float>>`. On a reload boundary (background ReplaceAll
    has already atomic-stored the NEW map into the processor slot), an in-flight
    voice can hold the LAST reference to the PREVIOUS map when it re-snapshots at
    `startNote`. Letting that voice-local `prevMap` shared_ptr drop its last ref
    on return would run the whole `free()` on the audio thread — a
    non-deterministic RT stall. (Related to the shipped C1 use-after-free fix:
    same reload boundary, different failure mode.)

    This reaper is the message-thread hand-off the review recommends. It is a
    single-producer / single-consumer ring:

      * Producer — the ONE audio thread. All voices render sequentially inside a
        single (non-reentrant) processBlock, so even 16 voices retiring maps in
        one block are a single producer.
      * Consumer — a message-thread juce::Timer that drains and destroys.

    The voice hands `prevMap` here only when it differs from the freshly
    snapshotted map (a genuine reload boundary), so steady-state playback
    generates zero traffic.

  ==============================================================================
*/

#pragma once

#include <juce_events/juce_events.h>   // juce::Timer

#include <array>
#include <atomic>
#include <memory>

#include "SampleMap.h"

class RetiredMapReaper : private juce::Timer
{
public:
    RetiredMapReaper()            { startTimerHz (kDrainHz); }
    ~RetiredMapReaper() override  { stopTimer(); drain(); }

    // Audio thread. Non-blocking, no allocation. Moves `m` into the ring so its
    // destructor (the big free) runs on the message thread when the Timer
    // drains. Degenerate fallback: if the ring is somehow full (the message
    // thread hasn't ticked across a flood of reload boundaries) `m` destructs
    // here — i.e. the pre-fix behaviour, never worse. Callers should gate this
    // with `m && m != currentMap` so only real reload boundaries push.
    void retire (std::shared_ptr<SampleMap>&& m) noexcept
    {
        if (m == nullptr)
            return;

        const int w    = writeIdx.load (std::memory_order_relaxed);
        const int r    = readIdx.load  (std::memory_order_acquire);
        const int next = (w + 1) % kCapacity;

        if (next == r)
            return;                            // ring full → drop (rare); m frees here

        // The consumer reset() this slot before advancing readIdx past it, so it
        // is empty here — the move-assign transfers ownership with no refcount
        // decrement (no free on the audio thread).
        slots[(size_t) w] = std::move (m);
        writeIdx.store (next, std::memory_order_release);
    }

private:
    static constexpr int kCapacity = 64;   // >> 16 voices; reload boundaries are user-paced
    static constexpr int kDrainHz  = 8;

    void timerCallback() override { drain(); }

    void drain() noexcept
    {
        int       r = readIdx.load  (std::memory_order_relaxed);
        const int w = writeIdx.load (std::memory_order_acquire);

        while (r != w)
        {
            slots[(size_t) r].reset();         // the big free — on the message thread
            r = (r + 1) % kCapacity;
        }

        readIdx.store (r, std::memory_order_release);
    }

    std::array<std::shared_ptr<SampleMap>, (size_t) kCapacity> slots {};
    std::atomic<int> writeIdx { 0 };
    std::atomic<int> readIdx  { 0 };
};
