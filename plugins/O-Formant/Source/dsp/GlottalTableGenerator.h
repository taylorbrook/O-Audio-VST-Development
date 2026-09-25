/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    GlottalTableGenerator.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Offline generation of LF glottal pulse wavetable with FFT-based mipmaps.
    Runs once at plugin init. Not real-time.

  ==============================================================================
*/

#pragma once
#include "GlottalWavetable.h"

class GlottalTableGenerator
{
public:
    // Generate full wavetable: 128 Rd steps x 10 mipmap levels
    // Call once at plugin construction. NOT real-time safe.
    static void generate (GlottalWavetable& table);

private:
    // Fant 1995 regression: Rd -> R-parameters -> timing
    struct LFTimingParams
    {
        double Tp;   // Time of max flow derivative (normalized to period = 1.0)
        double Te;   // Time of excitation (glottal closure)
        double Ta;   // Return phase time constant
        double Tc;   // Full period (always 1.0)
    };

    static LFTimingParams computeTimingFromRd (float Rd);

    // Render one period of LF derivative waveform into buffer
    static void renderLFPeriod (float* buffer, int size, const LFTimingParams& params);

    // FFT-based mipmap generation (adapted from O-Prism)
    static void generateMipmaps (GlottalWavetable& table);

    // Bracketed bisection solvers (offline only)
    static double netArea (double alpha, const LFTimingParams& p, double epsilon);
    static double solveAlpha (const LFTimingParams& p, double epsilon);
    static double solveEpsilon (const LFTimingParams& p);
};
