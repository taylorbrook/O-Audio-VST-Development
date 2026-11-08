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

#include <cstdint>
#include "GrainScheduler.h"   // grain-engine 1.1.0
#include "GiConstants.h"
#include "AmpEnv.h"
#include "ScanPlayhead.h"

// ============================================================================
// Voice allocator (ARCHITECTURE §3, RESEARCH-2.1 §5, PLAN-2.1 T8).
//
// 20 fixed slots = <= 16 COUNTED voices (gated or releasing; they spawn) +
// 4 steal spares. A stolen voice is not counted: a 5 ms forced release with
// its grains rendering under the ramp; it spawns nothing (demand 0). Poly
// only in 2.1 (Mono / Legato / glide / CC64 are 2.5).
//
// Victim policy = O-simpleWavetable WtSynthesiser.h:33-115, ported as a
// POLICY over the fixed array (the JUCE Synthesiser class mallocs per steal):
//   candidates = counted voices, oldest first by note serial;
//   low / top  = lowest / highest NON-released note (ties → oldest); if they
//                are the same voice only low is protected;
//   pick, in order: the oldest voice already on the target pitch → the oldest
//   released unprotected → the oldest unprotected with no key down → the oldest
//   unprotected → top → low.
// (Without CC64, "released" and "no key down" coincide; both rules stay so 2.5
// only changes what keyDown means.)
//
// Every decision keys on note SERIALS, never on slot indices, so a different
// slot assignment cannot change which NOTE is stolen (row V3).
// ============================================================================
namespace ogi
{
    struct Voice
    {
        enum class State { free, active, stolen };

        State         state    = State::free;
        int           note     = 0;
        int           channel  = 1;
        float         velocity = 0.0f;     // 0..1
        bool          gated    = false;    // key down (not released)
        std::uint64_t serial   = 0;
        std::int64_t  noteOnAbs = 0;

        AmpEnv                        env;
        ouaricon::grain::GrainScheduler sched;
        ScanAnchor                    scan;

        bool isFree() const noexcept     { return state == State::free; }
        bool isCounted() const noexcept  { return state == State::active; }
        bool isStolen() const noexcept   { return state == State::stolen; }
        bool isReleased() const noexcept { return state == State::active && ! gated; }
        bool isKeyDown() const noexcept  { return state == State::active && gated; }
    };

    struct VoiceAllocator
    {
        /** Counted (gated or releasing) voices. */
        static int countCounted (const Voice (&v)[kVoiceSlots]) noexcept
        {
            int n = 0;
            for (const auto& x : v)
                n += x.isCounted() ? 1 : 0;
            return n;
        }

        static int countStolen (const Voice (&v)[kVoiceSlots]) noexcept
        {
            int n = 0;
            for (const auto& x : v)
                n += x.isStolen() ? 1 : 0;
            return n;
        }

        /** Counted voices sorted oldest first (by serial); returns the count. */
        static int countedOldestFirst (const Voice (&v)[kVoiceSlots], int (&out)[kVoiceSlots]) noexcept
        {
            int n = 0;
            for (int k = 0; k < kVoiceSlots; ++k)
                if (v[k].isCounted())
                    out[n++] = k;
            for (int i = 1; i < n; ++i)
                for (int j = i; j > 0 && v[out[j]].serial < v[out[j - 1]].serial; --j)
                {
                    const int t = out[j];
                    out[j] = out[j - 1];
                    out[j - 1] = t;
                }
            return n;
        }

        /** The WtSynthesiser policy. −1 when no counted voice exists. */
        static int chooseVictim (const Voice (&v)[kVoiceSlots], int targetNote) noexcept
        {
            int usable[kVoiceSlots] {};
            const int n = countedOldestFirst (v, usable);
            if (n == 0)
                return -1;

            int low = -1, top = -1;
            for (int i = 0; i < n; ++i)   // oldest first → ties go to the oldest
            {
                const int k = usable[i];
                if (! v[k].isKeyDown())
                    continue;
                if (low < 0 || v[k].note < v[low].note) low = k;
                if (top < 0 || v[k].note > v[top].note) top = k;
            }
            if (top == low)
                top = -1;

            auto unprotected = [low, top] (int k) noexcept { return k != low && k != top; };

            for (int i = 0; i < n; ++i)
                if (v[usable[i]].note == targetNote)
                    return usable[i];
            for (int i = 0; i < n; ++i)
                if (unprotected (usable[i]) && v[usable[i]].isReleased())
                    return usable[i];
            for (int i = 0; i < n; ++i)
                if (unprotected (usable[i]) && ! v[usable[i]].isKeyDown())
                    return usable[i];
            for (int i = 0; i < n; ++i)
                if (unprotected (usable[i]))
                    return usable[i];
            if (top >= 0)
                return top;
            return low;
        }

        /** Oldest counted voice (polyphony shrink), −1 when none. */
        static int oldestCounted (const Voice (&v)[kVoiceSlots]) noexcept
        {
            int best = -1;
            for (int k = 0; k < kVoiceSlots; ++k)
                if (v[k].isCounted() && (best < 0 || v[k].serial < v[best].serial))
                    best = k;
            return best;
        }

        /** Quietest stolen voice (lowest env level; ties → oldest), −1 when none. */
        static int quietestStolen (const Voice (&v)[kVoiceSlots]) noexcept
        {
            int best = -1;
            for (int k = 0; k < kVoiceSlots; ++k)
            {
                if (! v[k].isStolen())
                    continue;
                if (best < 0)
                {
                    best = k;
                    continue;
                }
                const double a = v[k].env.getLevel(), b = v[best].env.getLevel();
                if (a < b || (! (b < a) && v[k].serial < v[best].serial))
                    best = k;
            }
            return best;
        }

        /** First free slot in `order` (a permutation of 0..19; nullptr = identity). */
        static int findFree (const Voice (&v)[kVoiceSlots], const int* order) noexcept
        {
            for (int i = 0; i < kVoiceSlots; ++i)
            {
                const int k = order != nullptr ? order[i] : i;
                if (k >= 0 && k < kVoiceSlots && v[k].isFree())
                    return k;
            }
            return -1;
        }
    };
} // namespace ogi
