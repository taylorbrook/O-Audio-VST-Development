/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - BankFactory (Stage 2.1)
    Ouaricon Audio
    Developer: Taylor Brook

    The five built-in 32-frame generators (ARCHITECTURE A3-A7, RESEARCH 5.1).
    Frame k = 1..32 is stored at frame index k - 1. Index == bank param 0..4.

      0 Sine->Saw     b_n = 1/n for n <= k (sine phase)
      1 Sine->Square  odd n = 2j-1: b_n = (1/n) * clamp(c(k) - (j-1), 0, 1),
                      c(k) = 1 + 15 (k-1)/31
      2 Pulse Width   cosine phase, a_n = (2/(n pi)) sin(n pi d_k) (-1)^n,
                      d_k = 0.5 (1/16)^((k-1)/31), n = 1..1023
      3 Formant       f_ref 110 Hz, n = 1..48, b_n = (1/n) prod R_i(n f_ref),
                      F1..F3 log-interpolated between vowel anchors A E I O U
                      at frames 1, 8.75, 16.5, 24.25, 32; B = 90/110/170 Hz
      4 Drive         x_k[i] = tanh(g_k sin(2 pi i/2048)) / tanh(g_k),
                      g_k = 0.25 * 100^((k-1)/31), forward FFT

    All peak-normalised to 1.0 at level 0. Message thread only.

  ==============================================================================
*/

#pragma once

#include "MipmapBuilder.h"
#include "WavetableBank.h"

namespace BankFactory
{
    inline constexpr int kFrames   = 32;
    inline constexpr int kNumBanks = 5;

    enum BankIndex : int
    {
        sineSaw    = 0,
        sineSquare = 1,
        pulseWidth = 2,
        formant    = 3,
        drive      = 4
    };

    // Builds bank `index` (0..4) into `out` (allocates 32 frames).
    void build (int index, WavetableBank& out, MipmapBuilder& builder);

    // ASCII bank name for index 0..4.
    const char* nameFor (int index) noexcept;

    //==========================================================================
    // Formula helpers (also read by the bank-check gates). k = 1..32.
    double squareFrontier (int k) noexcept;   // c(k)
    double pulseDuty      (int k) noexcept;   // d_k
    double driveGain      (int k) noexcept;   // g_k

    struct Formants { double f1, f2, f3; };
    inline constexpr int    kNumVowels       = 5;
    inline constexpr double kFormantRefHz    = 110.0;
    inline constexpr int    kFormantHarmonics = 48;
    Formants vowelAnchor (int vowel) noexcept;          // 0 A, 1 E, 2 I, 3 O, 4 U
    double   vowelAnchorFrame (int vowel) noexcept;     // 1, 8.75, 16.5, 24.25, 32
    Formants formantsAt (double frameK) noexcept;       // log-F interpolation, frameK in [1, 32]
    double   formantAmplitude (int n, const Formants& f) noexcept;   // b_n

    // Sine-phase saw spectrum b_n = 1/n for n = 1..numHarmonics (<= 1023) into
    // a packed MipmapBuilder::kPacked array (cleared first). Used by Sine->Saw
    // and by the dsp-check Saw-1023 arm.
    void fillSawSpectrum (float* packed, int numHarmonics) noexcept;
}
