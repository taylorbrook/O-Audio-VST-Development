/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

#include <cmath>
#include <cstdint>
#include "BandlimitedReader.h"   // grain-engine 1.1.0 (MipRing, SincBank, latchLevel)

// Test hooks are compiled ONLY into the render-harness target (CMake defines
// OGRAN_TEST_HOOKS=1 there). The plugin builds with every hook compiled out.
#ifndef OGRAN_TEST_HOOKS
 #define OGRAN_TEST_HOOKS 0
#endif

// ============================================================================
// O-Granulator engine constants (Stage 2, Phase 2.1).
// ARCHITECTURE §2, §6, §16; RESEARCH-2.1 §1; PLAN-2.1 T1.
// ============================================================================
namespace ogran
{
    // ── Pass engine ─────────────────────────────────────────────────────────
    // The host block is cut at every ABSOLUTE multiple of kCell. Each piece
    // writes its input into the ring before any grain reads (write-before-
    // read). The feedback return is delayed kFbDelay samples, so every
    // feedback sample a piece writes was produced by an EARLIER piece.
    inline constexpr int kCell    = 32;
    inline constexpr int kFbDelay = 32;
    static_assert (kCell <= kFbDelay, "a pass may not be longer than the feedback return delay "
                                      "(write-before-read with feedback, ARCHITECTURE Algorithm §1)");

    inline constexpr int kMaxStreams = 4;     // shaped for 4; 2.1 honours stream 1 only
    inline constexpr int kPoolLive   = 128;   // GrainPool live slots (grain_limit max)
    inline constexpr int kPoolFade   = 16;    // fade-on-steal spares (bursts can steal > 8 in 5 ms)

    // ── Pitch / size limits ─────────────────────────────────────────────────
    inline constexpr double kMaxSemis  = 24.0;
    inline constexpr double kMaxRate   = 4.0;     // 2^(kMaxSemis / 12)
    inline constexpr double kMinSizeMs = 5.0;
    inline constexpr double kMaxSizeMs = 2000.0;

    // Rate <= 4 → latchLevel <= 2 → residual r' in [1, 2] → sinc sets S0..S6.
    inline constexpr int kMaxLevel = 2;
    using Ring = ouaricon::grain::MipRing<kMaxLevel>;

    // ── Ring capacity derivation ────────────────────────────────────────────
    //   kMaxSpanS  : buffer_length max (30 s) — how far back a grain may START.
    //   kMaxReachS : the most source a grain consumes beyond its start while the
    //                head keeps advancing: a reverse grain at rate 4 for 2 s
    //                reads (4 + 1) × 2 s = 10 s.
    //   kGuardS    : slack (margins, the ≤ 31-sample render-head lead, reflect).
    // D2: the full 41 s at every sample rate (27.6 MB @ 48 k, 110 MB @ 192 k).
    inline constexpr double kMaxSpanS    = 30.0;
    inline constexpr double kMaxReachS   = 10.0;
    inline constexpr double kGuardS      = 1.0;
    inline constexpr double kRingSeconds = 41.0;
    static_assert ((kMaxRate + 1.0) * (kMaxSizeMs / 1000.0) <= kMaxReachS,
                   "max grain reach must cover a reverse grain at max rate and max size");
    static_assert (kMaxSpanS + kMaxReachS + kGuardS <= kRingSeconds,
                   "ring must hold max span + max reach + guard (memory: ring invariant needs a static_assert)");

    /** Level-0 ring length passed to ring.prepare(), and the bound of the reverse
        reach clamp (RESEARCH §1: reads stay valid to at least this + 61). */
    inline int kRingSamples (double fs) noexcept
    {
        return static_cast<int> (std::ceil (kRingSeconds * fs));
    }

    // ── Ramps / fades ───────────────────────────────────────────────────────
    inline constexpr double kBaseRampMs = 20.0;   // control-grid base ramp (rounded UP to whole cells)
    inline constexpr double kKillFadeMs = 5.0;    // R3 / D1 forced release (Grain::killStep)
    inline constexpr double kPoolFadeMs = 5.0;    // GrainPool fade-on-steal

    // ── CounterRng streams and slots ────────────────────────────────────────
    // Draw (seed, base + streamIndex, counter). Scheduler and grain draws use
    // different stream bases, so they never share values for the same counter.
    inline constexpr std::uint64_t kSchedStream = 0x0A5C'0000ull;
    inline constexpr std::uint64_t kGrainStream = 0x0A6E'0000ull;

    /** Fixed slot per randomised quantity. APPEND-ONLY: 2.2/2.3 use the reserved
        entries; never renumber (it would change every seeded render). */
    namespace Slot
    {
        enum : std::uint32_t
        {
            Jitter = 0,     // scheduler (kSchedStream)
            PosSpray,       // grain (kGrainStream) from here on
            Size,
            Pitch,
            Reverse,
            // reserved — 2.2 / 2.3
            Pan,
            Gain,
            FxCutoff,
            FxReso,
            FxDrive,
            FxCrush,
            FxCrushProb,
            FxRing,
            Window,
            Skew
        };
    }
} // namespace ogran
