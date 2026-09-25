/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    SVFFilter.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <cmath>

class SVFFilter
{
public:
    SVFFilter() = default;

    void prepare (double sampleRate);
    void reset();

    void setType (int type);
    void setCutoff (double hz);
    void setResonance (double res);
    void setDrive (double drive);

    double processSample (double input);

private:
    void updateCoefficients();

    /** The shared TPT core. `r2`/`hCoeff` are passed in rather than read from
        the members so the 24 dB cascade's second stage can run the same code
        with its own Butterworth pair instead of open-coding the core (IN-04).
        The return is selected by `filterType`, including Notch as case 6
        (IN-05) — the notch is this core with a `yLP + yHP` return. */
    double processSingleSVF (double input, double& s1, double& s2,
                             double r2, double hCoeff);
    double flushIfNonFinite (double output);

    double currentSampleRate = 44100.0;
    double cutoffHz = 20000.0;
    double resonance = 0.0;    // User param 0-1
    double driveAmount = 0.0;  // User param 0-1
    int filterType = 0;        // 0=LP12, 1=LP24, 2=HP12, 3=HP24, 4=BP12, 5=BP24, 6=Notch

    // SVF state (stage 1)
    double ic1eq_1 = 0.0, ic2eq_1 = 0.0;
    // SVF state (stage 2, for 24dB modes)
    double ic1eq_2 = 0.0, ic2eq_2 = 0.0;

    // Precomputed coefficients. All four are pure functions of cutoffHz,
    // resonance and currentSampleRate, so they are computed once in
    // updateCoefficients() rather than per sample (IN-04).
    double g = 0.0;   // tan(pi * fc / fs)
    double R2 = 0.0;  // 2 * resonance (inverse Q mapping), resonant stage
    double h = 0.0;   // 1 / (1 + R2 * g + g * g), resonant stage
    double butterR2 = 0.0; // 2 * 0.707, the 24 dB cascade's second stage
    double butterH = 0.0;  // 1 / (1 + butterR2 * g + g * g)

    bool coeffsDirty = true;
};
