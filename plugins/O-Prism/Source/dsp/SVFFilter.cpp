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

    SVFFilter.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "SVFFilter.h"
#include "MathConstants.h"
#include <algorithm>

void SVFFilter::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    updateCoefficients();
    coeffsDirty = false;
}

void SVFFilter::reset()
{
    ic1eq_1 = ic2eq_1 = 0.0;
    ic1eq_2 = ic2eq_2 = 0.0;
}

void SVFFilter::setType (int type)
{
    filterType = type;
}

void SVFFilter::setCutoff (double hz)
{
    if (cutoffHz != hz)
    {
        cutoffHz = hz;
        coeffsDirty = true;
    }
}

void SVFFilter::setResonance (double res)
{
    if (resonance != res)
    {
        resonance = res;
        coeffsDirty = true;
    }
}

void SVFFilter::setDrive (double drive)
{
    driveAmount = drive;
}

void SVFFilter::updateCoefficients()
{
    // Clamp below Nyquist as well as the nominal 20 kHz ceiling: at fs < 40 kHz
    // a 20 kHz cutoff makes tan(pi*fc/fs) go negative and the TPT integrators
    // blow up to NaN, which then recirculates through delay/reverb (WR-01).
    const double maxCutoff = std::min (20000.0, 0.49 * currentSampleRate);
    double fc = std::max (20.0, std::min (maxCutoff, cutoffHz));
    g = std::tan (kPi * fc / currentSampleRate);

    // Map user resonance (0-1) to SVF inverse Q
    // resonance=0 -> R2=2 (Butterworth), resonance=1 -> R2~0.1 (self-osc)
    double svfRes = 1.0 / (1.0 + resonance * 19.0);
    R2 = 2.0 * svfRes;
    h = 1.0 / (1.0 + R2 * g + g * g);

    // The 24 dB cascade's second stage runs at fixed Butterworth damping. The
    // literal 0.707 is deliberate and MUST NOT be "corrected" to sqrt(2)/2: it
    // sits 1.510e-4 relative below it (2*0.707 = 1.414 against sqrt(2) =
    // 1.4142135623730951, measured), and substituting the exact value changes
    // rendered output for every existing patch on LP24/HP24/BP24. Kept as
    // shipped; only the recomputation moved out of the per-sample path (IN-04).
    butterR2 = 2.0 * 0.707;
    butterH = 1.0 / (1.0 + butterR2 * g + g * g);
}

double SVFFilter::processSingleSVF (double input, double& s1, double& s2,
                                   double r2, double hCoeff)
{
    double yHP = hCoeff * (input - s1 * (r2 + g) - s2);
    double yBP = yHP * g + s1;
    s1 = yHP * g + yBP;
    double yLP = yBP * g + s2;
    s2 = yBP * g + yLP;

    switch (filterType)
    {
        case 0: return yLP;  // LP12
        case 1: return yLP;  // LP24 (per stage)
        case 2: return yHP;  // HP12
        case 3: return yHP;  // HP24 (per stage)
        case 4: return yBP;  // BP12
        case 5: return yBP;  // BP24 (per stage)
        case 6: return yLP + yHP;  // Notch = LP + HP (IN-05)
        default: return yLP;
    }
}

double SVFFilter::processSample (double input)
{
    if (coeffsDirty)
    {
        updateCoefficients();
        coeffsDirty = false;
    }

    // Pre-filter drive
    if (driveAmount > 0.0)
        input = std::tanh (input * (1.0 + driveAmount * 9.0));

    // Single-stage modes: the three 12 dB responses and Notch, which is the
    // same core with a different return (IN-05).
    if (filterType == 0 || filterType == 2 || filterType == 4 || filterType == 6)
        return flushIfNonFinite (processSingleSVF (input, ic1eq_1, ic2eq_1, R2, h));

    // 24dB modes (cascaded: two stages, resonance on first only). Both stages
    // are the same core; the second gets the Butterworth pair (IN-04).
    double stage1 = processSingleSVF (input, ic1eq_1, ic2eq_1, R2, h);
    double stage2 = processSingleSVF (stage1, ic1eq_2, ic2eq_2, butterR2, butterH);

    // filterType is an APVTS Choice over 0-6, so only 1/3/5 reach here and
    // both stages map identically for those. An out-of-range type cannot
    // occur, but the pre-IN-04 code ran the second stage's integrators and
    // then returned stage1 — preserved verbatim so the refactor is
    // bit-identical even on the unreachable branch.
    if (filterType != 1 && filterType != 3 && filterType != 5)
        return flushIfNonFinite (stage1);

    return flushIfNonFinite (stage2);
}

double SVFFilter::flushIfNonFinite (double output)
{
    if (std::isfinite (output))
        return output;

    // A NaN/inf in the integrator state is sticky and recirculates through
    // downstream feedback paths forever — flush and go silent for one sample
    // instead (WR-01).
    reset();
    return 0.0;
}
