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
        float Tp;   // Time of max flow derivative (normalized to period = 1.0)
        float Te;   // Time of excitation (glottal closure)
        float Ta;   // Return phase time constant
        float Tc;   // Full period (always 1.0)
    };

    static LFTimingParams computeTimingFromRd (float Rd);

    // Render one period of LF derivative waveform into buffer
    static void renderLFPeriod (float* buffer, int size, const LFTimingParams& params);

    // FFT-based mipmap generation (adapted from O-Prism)
    static void generateMipmaps (GlottalWavetable& table);

    // Newton-Raphson solvers (offline only)
    static float solveAlpha (float Tp, float Te);
    static float solveEpsilon (float Ta, float Te, float Tc);
};
