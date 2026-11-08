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

#include <juce_core/juce_core.h>   // juce::SpinLock
#include <array>
#include <atomic>
#include <cstdint>
#include "GiConstants.h"
#include "forked/ControlGrid.h"
#include "forked/ScaleQuantiser.h"

// ============================================================================
// GiScaleState — the engine side of the scale state (CONTEXT-2.2 D17,
// PLAN-2.2 T6 / PD3, RESEARCH-2.2 §4.1).
//
// A port of the parent's Engine::copyCustomTable / updateScaleState /
// tableForChoice (82406fce Engine.cpp:255-326) minus the 2.5b absolute-follow
// block (absSets), which the instrument does not have. The leaf types
// (ScaleSource, ActiveSet, scaleMaskIndex, scaleChoiceToId) come from the
// forked ScaleQuantiser.h unchanged; PitchSel is re-declared here (it lives in
// the parent's Spawner.h, which is not a leaf).
//
// Publish (PD3, the parent mechanism verbatim):
//   masks        128 relaxed std::atomic<uint64_t> owned by the processor
//                (canonical: bits >= degree count set), loaded at every tick;
//   custom table processor-owned, guarded by a juce::SpinLock + an
//                atomic<uint32_t> version. At a grid tick the audio thread
//                copies it ONLY when the version moved, through a
//                NON-BLOCKING ScopedTryLockType; a miss retries next tick.
//                The blocking copy runs only from bind() / Engine::prepare()
//                (host / message thread, process() not running).
// Nothing here parses, allocates or blocks on the audio thread.
//
// Per stream at each tick (update): scale Off or stream disabled →
// PitchSel {} (the continuous path); otherwise rebuild the ActiveSet if its
// key (choice, mask, span, customVersion) moved, then the pick window for the
// stream's base pitch: [lo, hi) = window (sN_pitch, span).
// ============================================================================
namespace ogi
{
    /** The scale-on pitch source of one stream at this grid (parent Spawner.h):
        set != nullptr ⇔ sN_scale ≠ Off (and the stream is enabled);
        [lo, hi) = ActiveSet::window (pitch, span). */
    struct PitchSel
    {
        const ActiveSet* set = nullptr;
        int lo = 0, hi = 0;
    };

    class GiScaleState
    {
    public:
        enum class CopyResult { none, copied, missed };

        /** Message thread (Engine constructor): builtIns() is already warm
            (the processor constructor builds it first). */
        GiScaleState()
            : builtIns (&PitchCollections::builtIns())
        {
            custom = (*builtIns)[static_cast<size_t> (PitchCollections::Custom)];
        }

        /** Message thread (processor constructor, after Engine::bind). */
        void bind (const ScaleSource& s) noexcept
        {
            src = s;
            copyCustomTable (true);
            invalidate();
        }

        bool isBound() const noexcept { return src.masks != nullptr; }

        /** P11 (parent Engine.cpp:269-298). blocking = true only from bind() /
            Engine::prepare(); the audio thread passes false and never blocks. */
        CopyResult copyCustomTable (bool blocking) noexcept
        {
            if (src.lock == nullptr || src.customTable == nullptr || src.customVersion == nullptr)
                return CopyResult::none;
            const std::uint32_t v = src.customVersion->load (std::memory_order_acquire);
            if (blocking)
            {
                const juce::SpinLock::ScopedLockType sl (*src.lock);
                custom = *src.customTable;
                copiedVersion = v;
                return CopyResult::copied;
            }
            if (v == copiedVersion)
                return CopyResult::none;
            const juce::SpinLock::ScopedTryLockType tl (*src.lock);
            if (! tl.isLocked())
                return CopyResult::missed;   // retried at the next tick
            custom = *src.customTable;       // trivially copyable, ≈ 264 B
            copiedVersion = v;
            return CopyResult::copied;
        }

        /** sN_scale choice → table: 31 = the engine's custom copy; else the built-in. */
        const PitchCollections::Table& tableForChoice (int choice) const noexcept
        {
            const int id = scaleChoiceToId (choice);
            if (id == PitchCollections::Custom)
                return custom;
            return (*builtIns)[static_cast<size_t> (std::clamp (id, 0, PitchCollections::kCount - 1))];
        }

        /** Grid tick, after the latch (parent updateScaleState minus the 2.5b
            follow block). Returns the number of ActiveSet rebuilds. */
        int update (const GiGridParams& gp) noexcept
        {
            int rebuilds = 0;
            for (int s = 0; s < kGiStreams; ++s)
            {
                const auto& st = gp.s[s];
                if (! st.enable || st.scale <= 0)
                {
                    sel[s] = {};
                    continue;
                }
                const std::uint64_t mask = src.masks != nullptr
                                               ? src.masks[scaleMaskIndex (s, st.scale)].load (std::memory_order_relaxed)
                                               : ~std::uint64_t (0);
                const float spanCents = st.pitchRand * 100.0f;
                if (sets[s].rebuildIfNeeded (st.scale, mask, spanCents, copiedVersion, tableForChoice (st.scale)))
                    ++rebuilds;
                auto& ps = sel[s];
                ps.set = &sets[s];
                sets[s].window (static_cast<double> (st.pitchSt), static_cast<double> (spanCents), ps.lo, ps.hi);
            }
            return rebuilds;
        }

        /** prepare(): every set rebuilds at the next tick; no selection held. */
        void invalidate() noexcept
        {
            for (auto& a : sets)
                a.invalidate();
            for (auto& p : sel)
                p = {};
        }

        const PitchSel& selection (int s) const noexcept { return sel[std::clamp (s, 0, kGiStreams - 1)]; }
        const ActiveSet& activeSet (int s) const noexcept { return sets[std::clamp (s, 0, kGiStreams - 1)]; }
        const PitchCollections::Table& customCopy() const noexcept { return custom; }
        std::uint32_t customCopyVersion() const noexcept { return copiedVersion; }

    private:
        ScaleSource src;
        const std::array<PitchCollections::Table, PitchCollections::kCount>* builtIns = nullptr;
        PitchCollections::Table custom;
        std::uint32_t copiedVersion = 0;
        ActiveSet sets[kGiStreams];
        PitchSel  sel[kGiStreams];
    };
} // namespace ogi
