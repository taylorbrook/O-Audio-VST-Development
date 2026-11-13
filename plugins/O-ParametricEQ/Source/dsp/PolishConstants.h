/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - accuracy polish, constants (Stage 2, Phase 2.1b)

    The ONLY place these numbers live (PLAN-2.1b P10). They are part of the
    design: every acceptance figure of gate G1 from Phase 2.1b on was measured
    with exactly these values, on both threads. Changing one is a design
    change, not a tuning step. tests/model/polish_model.py mirrors them.

  ==============================================================================
*/

#pragma once

namespace eq
{
namespace polish
{
    /** Fixed match points, log-spaced over the band. */
    inline constexpr int fixedPoints = 16;

    /** Points tied to the analog section: for the zero pair and for the pole
        pair, the centre and w0 (sqrt (1 + h^2) +- h) with h = s / (2 Q). */
    inline constexpr int numTiedScales = 2;
    inline constexpr double tiedScales[numTiedScales] = { 0.5, 1.5 };
    inline constexpr int tiedPoints = 2 * (1 + 2 * numTiedScales);
    inline constexpr int numPoints = fixedPoints + tiedPoints;

    /** Anchored Gauss-Newton steps, and the further steps of the blended
        extension for hard cases. */
    inline constexpr int steps = 2;
    inline constexpr int extraSteps = 2;

    /** Free log-parameters: g, k, mB, mH, G, cH. */
    inline constexpr int numFree = 6;

    /** Marquardt diagonal scaling, and the pull toward the seed in dB^2 per
        unit log-parameter^2. */
    inline constexpr double lambda = 1e-3;
    inline constexpr double mu = 0.1;

    /** Match-grid RMS error of the seed (dB) at or below which the seed is
        kept exactly, and at which the accept weight's first factor is 1. */
    inline constexpr double seedKeepRms = 0.002;
    inline constexpr double seedFullRms = 0.006;

    /** Match-grid RMS error after `steps` (dB): the extension fades in from
        this value to twice this value. */
    inline constexpr double extensionRms = 0.03;

    /** Relative reduction of the objective at which the accept weight's
        second factor is 1. */
    inline constexpr double acceptRamp = 0.1;

    /** Smooth limit on the length of every step (Euclidean norm over the six
        free log-parameters): delta <- delta R / sqrt (R^2 + |delta|^2).
        From RESEARCH-2.1b-continuity Q4 (b). */
    inline constexpr double stepLimit = 0.35;

    /** Below this sample rate the designer does not polish at all: the match
        band reaches Nyquist there and the polished map is not continuous
        (RESEARCH-2.1b-continuity Q7). Read once, in Designer::prepare. */
    inline constexpr double minSampleRateHz = 44100.0;

    /** The match band: bandLowHz ... min (bandHighHz, bandTopFraction fs). */
    inline constexpr double bandLowHz = 20.0;
    inline constexpr double bandHighHz = 20000.0;
    inline constexpr double bandTopFraction = 0.499;
}
} // namespace eq
