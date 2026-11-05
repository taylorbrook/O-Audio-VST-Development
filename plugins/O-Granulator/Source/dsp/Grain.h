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

#include <cstdint>
#include <type_traits>

namespace ogran
{
    // ========================================================================
    // Grain — the GrainPool's POD, declared at its FINAL layout (CONTEXT-2.1
    // default). GrainPool::activate() value-initialises a slot (Grain {}), so
    // every field below starts at its default for every new grain: no state
    // bleeds from the previous occupant (DSP-05 criterion 2). A stolen grain is
    // COPIED to a fade slot with all of its state.
    //
    // FX / window-morph fields exist now so 2.2 / 2.3 add behaviour, not layout.
    // They are value-initialised and unused in 2.1.
    // ========================================================================
    struct Grain
    {
        // ── identity / reader latch ─────────────────────────────────────────
        int stream = 0;     // 0..3
        int level  = 0;     // MipRing octave level, latched (latchLevel (rate, 2))
        int set    = 0;     // SincBank set, latched (setFor (rate / 2^level))
        int dir    = 1;     // +1 forward, −1 reverse
        int age    = 0;     // output samples rendered so far

        // ── read position (absolute level-0 sample index, double) ───────────
        double rate    = 1.0;   // level-0 samples per output sample (> 0)
        double step    = 1.0;   // dir · rate, added to readAbs per sample
        double readAbs = 0.0;   // anchor + step · (age + frac); never narrowed
        double frac    = 0.0;   // onset fraction in [0, 1)
        double len     = 0.0;   // length in output samples (fractional)
        double invLen  = 0.0;   // 1 / len
        double startDelay = 0.0; // D at spawn (delay behind the head) — telemetry / tests

        // ── gains ───────────────────────────────────────────────────────────
        float gainL = 1.0f;     // per-grain spread pan × gain_rand (2.2); exactly 1 at spread 0 / gain_rand 0
        float gainR = 1.0f;
        float killGain = 1.0f;  // D1 forced release (R3 cap): exactly 1.0f unless killing
        float killStep = 0.0f;  // > 0 while killing: 1 / (5 ms · fs)

        // ── window morph (2.2) ──────────────────────────────────────────────
        int   windowStep = 0;       // 0..64 (WindowMorphBank step), latched at spawn
        float windowSkew = 0.0f;    // telemetry only (the tilt below is what renders)
        float tiltT = 0.5f;         // P13: WindowLut::Tilt {t, a, b}, makeTilt (0.5 + 0.45·skew)
        float tiltA = 1.0f;         //      — {0.5, 1, 1} is the bitwise identity warp
        float tiltB = 1.0f;
        float pitchCents = 0.0f;    // P13: latched scale pick / continuous random offset (cents)

        // ── per-grain FX state (2.3) ────────────────────────────────────────
        std::uint32_t fxActive = 0;            // bit flags: SVF, drive, crush, ring
        int   svfMode = 0;                     // 0 off, 1 LP, 2 BP, 3 HP
        float svfK = 0.0f, svfA1 = 0.0f, svfA2 = 0.0f, svfA3 = 0.0f;
        float svfIc1[2] {};
        float svfIc2[2] {};
        float driveD = 1.0f;
        float driveNorm = 1.0f;                // D6 latched normaliser r / tanh (d·r) (2.3)
        // crushHoldPhase defaults to 1 (P24): value-initialisation gives the
        // "fresh hold" — the first sample is captured.
        float crushHoldInc = 0.0f, crushHoldPhase = 1.0f, crushScale = 0.0f;
        float crushHeld[2] {};
        float ringRe = 1.0f, ringIm = 0.0f, ringRotRe = 1.0f, ringRotIm = 0.0f, ringMix = 0.0f;
    };

    static_assert (std::is_trivially_copyable_v<Grain>,
                   "GrainPool copies grains into fade slots — Grain must stay a trivially copyable POD");
} // namespace ogran
