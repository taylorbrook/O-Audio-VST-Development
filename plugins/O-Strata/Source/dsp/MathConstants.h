/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    MathConstants.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Shared math constants used across DSP modules.

  ==============================================================================
*/

#pragma once

#include <cmath>

inline constexpr double kPi     = 3.141592653589793;
inline constexpr double kTwoPi  = 6.283185307179586;
inline constexpr double kHalfPi = 1.5707963267948966;

/** Oscillator Coarse (semitones) + Fine (cents) → frequency ratio. One law for the
    voice (StrataVoice::startNote / renderNextBlock) and the D3 top-note probe
    (TerrainScheduler::computeTopNote) — lifted here so neither mirrors the other
    (Stage 3 plan Task 2). Block-rate pow only. */
inline double pitchRatio (int coarseSemitones, double fineCents) noexcept
{
    return std::pow (2.0, (static_cast<double> (coarseSemitones) + fineCents / 100.0) / 12.0);
}
