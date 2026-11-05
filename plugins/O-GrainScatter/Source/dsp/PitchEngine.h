/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>
#include "GrainScatterRng.h"
#include "PitchCollections.h"   // pitch-collections 1.0.0 (modules/tuning/pitch-collections/cpp)

// ============================================================================
// v3.0.0 — cents-based pitch collections (replaces the 12-TET ScaleQuantizer).
//
// A collection is a list of pitches in CENTS inside one period (1200 for an
// octave, 1901.955 for Bohlen-Pierce's tritave), degree 0 always being 0 ¢.
// Each degree can be switched off by a per-collection 64-bit mask, so no
// collection may exceed kMaxPitches degrees.
//
// The engine works on a sorted table of the ACTIVE pitches inside the span
// (Range x Pitch Rnd, sided by Direction), rebuilt on the audio thread only
// when an input changes. The table is a fixed array: no allocation, ever.
// ============================================================================

// v3.6.0: PitchCollections (Id enum, names, builtIns tables, Table and the
// text / Scala parsers) moved verbatim to the shared pitch-collections module.
// The alias keeps every call site, and the test drivers that also compile
// against older trees (whose own dsp/PitchEngine.h still defines the
// namespace), unchanged.
namespace PitchCollections = ouaricon::tuning::PitchCollections;

class PitchEngine
{
public:
    enum Mode      { Random = 0, LadderUp, LadderDown, Pendulum };
    enum Direction { Both = 0, Up, Down };

    struct Settings
    {
        int      collection = 0;
        int      rootNote   = 0;       // semitones above C
        int      mode       = Random;
        int      direction  = Both;
        float    amount     = 0.0f;    // Pitch Rnd, 0-1
        float    rangeSt    = 12.0f;   // 0-24
        float    transposeSt = 0.0f;
        float    fineCents  = 0.0f;
        uint64_t mask       = ~uint64_t (0);
        uint32_t customVersion = 0;    // bumps whenever the custom table changes
    };

    // Overall pitch is clamped to +-36 st (rate 0.125-8), the bound the delay
    // buffer and the spawn pre-roll are sized for.
    static constexpr float kMaxTotalSemitones = 36.0f;

    /** Audio thread. `custom` is the processor's audio-side copy. */
    float nextRate (const Settings& s, const PitchCollections::Table& custom, const CounterRng::Draw& rng)
    {
        rebuildIfNeeded (s, custom);

        float offsetCents = 0.0f;
        const float spanCents = s.amount * s.rangeSt * 100.0f;

        if (spanCents > 0.5f)
        {
            if (s.collection == PitchCollections::Free && s.mode == Random)
            {
                const float lo = (s.direction == Up)   ? 0.0f : -spanCents;
                const float hi = (s.direction == Down) ? 0.0f :  spanCents;
                offsetCents = lo + rng.nextFloat (CounterRng::SlotPitch) * (hi - lo);
            }
            else if (activeCount > 0)
            {
                if (s.mode == Random)
                    offsetCents = active[static_cast<size_t> (rng.nextInt (CounterRng::SlotPitch, activeCount))];
                else
                {
                    offsetCents = active[static_cast<size_t> (ladderIndex)];
                    advanceLadder (s.mode);
                }
            }
        }

        const float totalSt = juce::jlimit (-kMaxTotalSemitones, kMaxTotalSemitones,
                                            (offsetCents + s.fineCents) / 100.0f + s.transposeSt);
        return std::exp2 (totalSt / 12.0f);
    }

    void resetLadder()
    {
        ladderIndex = startIndex;
        ladderDirection = 1;
    }

    /** Forces a table rebuild on the next nextRate() (prepare/reset). */
    void invalidate() { built = false; }

private:
    static constexpr int kMaxActive = 512;   // 53-EDO over +-24 st = 213; Free ladder 49

    std::array<float, kMaxActive> active {};
    int activeCount = 0;
    int startIndex = 0;      // the active pitch nearest 0 ¢
    int ladderIndex = 0;
    int ladderDirection = 1;

    // Rebuild key
    bool built = false;
    int  kCollection = -1, kRoot = -1, kDirection = -1, kMode = -1;
    float kSpan = -1.0f;
    uint64_t kMask = 0;
    uint32_t kCustomVersion = 0;

    void rebuildIfNeeded (const Settings& s, const PitchCollections::Table& custom)
    {
        const float span = s.amount * s.rangeSt * 100.0f;

        const bool collectionChanged = (s.collection != kCollection || s.mode != kMode);
        if (built && ! collectionChanged && s.rootNote == kRoot && s.direction == kDirection
            && span == kSpan && s.mask == kMask && s.customVersion == kCustomVersion)
            return;

        const auto& table = (s.collection == PitchCollections::Custom)
                                ? custom
                                : PitchCollections::builtIns()[static_cast<size_t> (
                                      juce::jlimit (0, PitchCollections::kCount - 1, s.collection))];

        const float lo = (s.direction == Up)   ? 0.0f : -span;
        const float hi = (s.direction == Down) ? 0.0f :  span;
        constexpr float eps = 0.01f;
        const float rootCents = static_cast<float> (s.rootNote) * 100.0f;
        const float period = table.period > 1.0f ? table.period : 1200.0f;

        activeCount = 0;
        // Pitches are rootCents + degree + k*period. Walk k from the period
        // below lo to the period above hi, ascending, so the table is sorted.
        const int kLo = static_cast<int> (std::floor ((lo - rootCents) / period)) - 1;
        const int kHi = static_cast<int> (std::ceil  ((hi - rootCents) / period)) + 1;
        for (int k = kLo; k <= kHi && activeCount < kMaxActive; ++k)
        {
            for (int d = 0; d < table.count && activeCount < kMaxActive; ++d)
            {
                if (d < 64 && ((s.mask >> d) & 1u) == 0)
                    continue;
                const float c = rootCents + table.cents[static_cast<size_t> (d)]
                              + static_cast<float> (k) * period;
                if (c >= lo - eps && c <= hi + eps)
                    active[static_cast<size_t> (activeCount++)] = c;
            }
        }

        // The pitch nearest 0 ¢ — where a ladder (re)starts.
        startIndex = 0;
        for (int i = 1; i < activeCount; ++i)
            if (std::abs (active[static_cast<size_t> (i)]) < std::abs (active[static_cast<size_t> (startIndex)]))
                startIndex = i;

        if (! built || collectionChanged)
            resetLadder();
        else
            ladderIndex = juce::jlimit (0, juce::jmax (0, activeCount - 1), ladderIndex);

        built = true;
        kCollection = s.collection; kRoot = s.rootNote; kDirection = s.direction; kMode = s.mode;
        kSpan = span; kMask = s.mask;
        kCustomVersion = s.customVersion;
    }

    void advanceLadder (int mode)
    {
        if (activeCount <= 1) { ladderIndex = 0; return; }

        if (mode == LadderUp)
            ladderIndex = (ladderIndex + 1) % activeCount;
        else if (mode == LadderDown)
            ladderIndex = (ladderIndex - 1 + activeCount) % activeCount;
        else  // Pendulum: bounce, without repeating the turning pitch
        {
            ladderIndex += ladderDirection;
            if (ladderIndex >= activeCount - 1) { ladderIndex = activeCount - 1; ladderDirection = -1; }
            else if (ladderIndex <= 0)          { ladderIndex = 0;               ladderDirection =  1; }
        }
    }
};
