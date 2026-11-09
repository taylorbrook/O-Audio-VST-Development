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
#include "forked/EngineConstants.h"

// ============================================================================
// Instrument-only constants (Phases 2.1 / 2.2). The forked EngineConstants.h keeps
// the parent's names; everything the instrument adds lives here so the
// adapted leaf stays a small, recorded diff (FORK-MANIFEST).
// ============================================================================
namespace ogi
{
    // ── Voices (ARCHITECTURE §3) ────────────────────────────────────────────
    inline constexpr int kVoiceSlots       = 20;   // <= 16 counted + 4 steal spares
    inline constexpr int kMaxCountedVoices = 16;   // polyphony max
    inline constexpr int kStealSpares      = 4;
    static_assert (kMaxCountedVoices + kStealSpares == kVoiceSlots, "20 voice slots = 16 counted + 4 spares");

    // ── Slots / streams ─────────────────────────────────────────────────────
    inline constexpr int kTapeSlots = 4;           // file / record slots (stream N reads slot N)
    inline constexpr int kGiStreams = 4;           // 2.2: s1...s4 (2.1: s1 only)
    inline constexpr int kPairs     = kVoiceSlots * kGiStreams;   // (voice, stream) demands: 80

    // ── Tape (ARCHITECTURE §7, S6) ──────────────────────────────────────────
    inline constexpr int kTapeLevels = kMaxLevel;  // MipTape<4>; latchLevel (rate, 4) ALWAYS (P6)
    static_assert (kTapeLevels == 4, "±48 st needs four octave levels");

    // ── Times (ms) ──────────────────────────────────────────────────────────
    inline constexpr double kStealMs     = 5.0;    // voice steal / polyphony shrink forced release
    inline constexpr double kEnvFloorMs  = 2.0;    // amp attack / decay / release de-click floor
    inline constexpr double kXfadeMs     = 20.0;   // SR-rebuild dual-read crossfade
    inline constexpr double kMinLoopMs   = 10.0;   // loop region minimum width (tape time)
    inline constexpr double kMinSourceMs = 10.0;   // a shorter source is refused by the loader

    // ── Levels ──────────────────────────────────────────────────────────────
    // D16 / PLAN-2.2 PD1: kVoiceGainDb = measured target + ear trim.
    //   kVoiceGainMeasuredDb: measured vs O-Granulator 82406fce, mix 100 %,
    //     PLAN-2.2 T12, user 2026-11-08. GV0 target +0.0212 dB (4 seeds
    //     +0.025 / +0.010 / +0.019 / +0.031, spread 0.021 dB; parent s1 defaults
    //     live, ours 1 voice root 48 vel 127 defaults; −30 dBFS RMS noise),
    //     rounded to 0.01 dB. Was −6.0 (the 2.1 placeholder).
    //   kVoiceGainEarTrimDb: HANDS-ON-2.2 row 6 (0.0 until the user trims).
    inline constexpr double kVoiceGainMeasuredDb = 0.02;
    inline constexpr double kVoiceGainEarTrimDb  = 0.0;
    inline constexpr double kVoiceGainDb = kVoiceGainMeasuredDb + kVoiceGainEarTrimDb;   // per-voice headroom
    inline constexpr double kEnvEndLevel = 1.5848931924611134e-5;   // −96 dB: a voice ends below this
    inline constexpr float  kWetCeil     = 1.99526231f;            // legacy wet safety (2.1–2.3); the legacyWetTanh hook only (PLAN-2.4 PD1)

    // ── Slot store (D2, S8) ─────────────────────────────────────────────────
    inline constexpr double kSlotMaxSeconds = 30.0;
    inline constexpr int    kSlotMaxFrames  = 2880000;   // session frames (30 s ≤ 96 k, 15 s @ 192 k)
    inline constexpr int    kOverviewBin    = 512;       // one (min, max) pair per 512 tape frames

    // ── Handoff (ARCHITECTURE §6) ───────────────────────────────────────────
    inline constexpr int kRetireRingCap = 16;      // audio → message SPSC retire ring
    inline constexpr int kMaxRetiring   = 16;      // audio-side tapes waiting for their grains to end

    // ── Scheduler ───────────────────────────────────────────────────────────
    inline constexpr int kOnsetCap = kCell + 1;    // <= 1 onset per sample per voice and segment
} // namespace ogi
