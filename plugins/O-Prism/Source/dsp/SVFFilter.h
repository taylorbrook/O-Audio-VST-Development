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
    void setKeyTrack (double amount, int midiNote);

    double processSample (double input);

private:
    void updateCoefficients();
    double processSingleSVF (double input, double& s1, double& s2);
    double processNotch (double input, double& s1, double& s2);
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

    // Precomputed coefficients
    double g = 0.0;   // tan(pi * fc / fs)
    double R2 = 0.0;  // 2 * resonance (inverse Q mapping)

    bool coeffsDirty = true;
};
