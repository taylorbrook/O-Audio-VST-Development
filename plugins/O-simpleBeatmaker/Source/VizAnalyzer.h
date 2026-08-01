/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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

    O-simpleBeatmaker - VizAnalyzer (lock-free audio -> UI handoff)

    Carries, allocation-free, the per-hit trigger record the Stage-3 UI needs:
    voice, step, nominal vs applied sample-in-bar (UI computes Δt = applied −
    nominal), velocity, and source (0=sequencer, 1=host MIDI).

    QUAL-02 by construction: the producer pushes a VizEvent in the SAME code path
    that emits the note-on into the sequencer MidiBuffer (one emit = one push),
    so the displayed Δt IS the offset baked into the MidiMessage that drove the
    voice — never a UI-side recomputation of the swing/humanize formula.

    Audio thread = single producer (juce::AbstractFifo + POD store). Message
    thread (Stage-3 Timer) = single consumer. playheadStepPhase is a separate,
    continuous std::atomic<float> updated once per block (decoupled from the
    event ring).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

namespace OSimpleBeatmaker
{
    struct VizEvent
    {
        juce::uint8  voiceIndex        = 0;
        juce::int16  stepIndex         = 0;
        juce::int32  nominalSampleInBar = 0;
        juce::int32  appliedSampleInBar = 0;
        juce::uint8  velocity          = 0;
        juce::uint8  source            = 0;   // 0 = sequencer, 1 = host MIDI
    };

    class VizAnalyzer
    {
    public:
        static constexpr int kCapacity = 1024;

        // AUDIO THREAD — single contiguous slot, RAII commit in the scope dtor.
        void push (const VizEvent& ev) noexcept
        {
            const auto scope = fifo.write (1);
            if (scope.blockSize1 > 0)
                store[(size_t) scope.startIndex1] = ev;
            // (if the ring is momentarily full the event is dropped — viz only)
        }

        // MESSAGE THREAD — drain up to `max` events into dest; returns count.
        int drain (VizEvent* dest, int max) noexcept
        {
            int got = 0;
            while (got < max && fifo.getNumReady() > 0)
            {
                const auto scope = fifo.read (1);
                if (scope.blockSize1 > 0)
                    dest[got++] = store[(size_t) scope.startIndex1];
                else
                    break;
            }
            return got;
        }

        int  getNumReady() const noexcept { return fifo.getNumReady(); }

        void setPlayheadStepPhase (float phase) noexcept
        {
            playheadStepPhase.store (phase, std::memory_order_relaxed);
        }
        float getPlayheadStepPhase() const noexcept
        {
            return playheadStepPhase.load (std::memory_order_relaxed);
        }

        int getFrameCount() const noexcept { return frameCount.load (std::memory_order_relaxed); }
        void bumpFrame()    noexcept       { frameCount.fetch_add (1, std::memory_order_relaxed); }

    private:
        juce::AbstractFifo fifo { kCapacity };
        std::array<VizEvent, (size_t) kCapacity> store {};
        std::atomic<float> playheadStepPhase { 0.0f };
        std::atomic<int>   frameCount { 0 };
    };
}
