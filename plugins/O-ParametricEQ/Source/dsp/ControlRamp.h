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

    O-ParametricEQ - control grid and control ramp (Stage 2, Phase 2.3)

    The control grid: band designs are made every K samples, on a sample
    counter that the processor owns (ARCHITECTURE "Parameter smoothing and the
    control grid"). K depends on the sample rate alone:

      controlGridSize (fs)    16 for fs <= 48 kHz, 32 for fs <= 96 kHz, 64 above
      rampLengthTicks (fs)    round (0.020 fs / K), at least 1
                              (28, 40, 55, 60, 55, 60, 55, 60 at 22.05, 32,
                              44.1, 48, 88.2, 96, 176.4, 192 kHz)

    The control ramp: one band's three continuous controls on ONE counter,
    stepped once per grid tick (PLAN-2.3 P2):

      ln f               natural log of the frequency in Hz
      effective gain     dB, already scaled by the gain scale (BandParams.h);
                         there is no separate gain-scale ramp, and a dynamic
                         offset is added by the band AFTER the ramp
      ln Q

      snap (settings)      current = target = the settings' values; counter 0.
                           The only way a ramp gets its first value.
      retarget (settings)  per-tick increments (target - current) / N,
                           counter N = the ramp length. A fresh target while
                           running restarts from the current value over the
                           full length.
      advance()            counter > 1: add the increments, count down.
                           counter == 1: counter 0, and the value IS the
                           target (assigned, not summed).

    Arrival is the counter reaching 0. No value is ever compared to decide it.

    getDesignSettings() is what the band hands to the designer at a tick:
      running   the target's settings with freqHz = exp (ln f), the ramped
                gain, q = exp (ln Q), each clamped to its range (an
                exp (log x) round trip can leave the range by one ulp);
      arrived   the target's OWN BandSettings, untouched. The settled design
                is then bit-identical to a static design of the same settings.

    Type and slope index are not ramped: they are read from the target. A
    change of either is structural and is the caller's business (Band::snapTo).

    Callers: Band.h. The designer harness may drive a ramp directly.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "BandParams.h"
#include "EqTypes.h"

#include <cmath>

namespace eq
{

//==============================================================================
// The control grid
//==============================================================================

inline constexpr double rampSeconds = 0.020;

/** K: the number of samples between two control ticks at this sample rate. */
constexpr int controlGridSize (double sampleRateHz) noexcept
{
    return (sampleRateHz <= 48000.0) ? 16 : ((sampleRateHz <= 96000.0) ? 32 : 64);
}

/** The length of a control ramp in ticks: 20 ms on the grid of this rate,
    rounded to nearest, at least 1. A rate that is not a positive finite
    number gives 1. */
inline int rampLengthTicks (double sampleRateHz) noexcept
{
    if (! std::isfinite (sampleRateHz) || ! (sampleRateHz > 0.0))
        return 1;

    const double grid = static_cast<double> (controlGridSize (sampleRateHz));
    const double ticks = rampSeconds * sampleRateHz / grid;

    if (ticks < 1.0)
        return 1;

    if (ticks > 1.0e6)
        return 1000000;

    return static_cast<int> (std::lround (ticks));
}

//==============================================================================
// The control ramp
//==============================================================================

class ControlRamp
{
public:
    /** N: the number of ticks a retarget takes to arrive (rampLengthTicks).
        Values below 1 are read as 1. A ramp that is running keeps its
        counter and its increments. */
    void setLength (int ticks) noexcept
    {
        length = (ticks < 1) ? 1 : ticks;
    }

    int getLength() const noexcept                              { return length; }

    //==========================================================================
    /** current = target = the values of `settings`; counter 0. */
    void snap (const BandSettings& settings) noexcept
    {
        setTargetValues (settings);

        lnF = targetLnF;
        gainDb = targetGainDb;
        lnQ = targetLnQ;

        incLnF = 0.0;
        incGainDb = 0.0;
        incLnQ = 0.0;
        counter = 0;
    }

    /** Starts (or restarts) the ramp from the current value toward
        `settings`, over the full length. */
    void retarget (const BandSettings& settings) noexcept
    {
        setTargetValues (settings);

        const double n = static_cast<double> (length);
        incLnF = (targetLnF - lnF) / n;
        incGainDb = (targetGainDb - gainDb) / n;
        incLnQ = (targetLnQ - lnQ) / n;
        counter = length;
    }

    /** One tick. Does nothing when the ramp is not running. */
    void advance() noexcept
    {
        if (counter > 1)
        {
            lnF += incLnF;
            gainDb += incGainDb;
            lnQ += incLnQ;
            --counter;
        }
        else if (counter == 1)
        {
            lnF = targetLnF;
            gainDb = targetGainDb;
            lnQ = targetLnQ;

            incLnF = 0.0;
            incGainDb = 0.0;
            incLnQ = 0.0;
            counter = 0;
        }
    }

    //==========================================================================
    /** True from a retarget until the advance that arrives. */
    bool isRunning() const noexcept                             { return counter > 0; }

    /** Ticks still to advance; 0 when arrived. */
    int getTicksLeft() const noexcept                           { return counter; }

    /** The settings last given to snap or retarget, untouched. */
    const BandSettings& getTarget() const noexcept              { return target; }

    /** The current ramp values. */
    double getLnFreq() const noexcept                           { return lnF; }
    double getGainDb() const noexcept                           { return gainDb; }
    double getLnQ() const noexcept                              { return lnQ; }

    /** What the designer is given at this tick: the target's own settings
        when arrived, otherwise the target's type and slope with the clamped
        ramp values. */
    BandSettings getDesignSettings() const noexcept
    {
        if (counter <= 0)
            return target;

        BandSettings s = target;
        s.freqHz = orderedClamp (std::exp (lnF), rawFreqMin, rawFreqMax);
        s.gainDbEff = orderedClamp (gainDb, gainEffMin, gainEffMax);
        s.q = orderedClamp (std::exp (lnQ), rawQMin, rawQMax);
        return s;
    }

private:
    /** A clamp whose result is always inside [lo, hi]: a value that is not
        a number comes out as lo. For a value inside the range it is the
        value itself. */
    static double orderedClamp (double v, double lo, double hi) noexcept
    {
        return (v >= lo) ? ((v <= hi) ? v : hi) : lo;
    }

    /** The three ramp values of a target. Settings from makeBandInput are
        inside the ranges already, and the clamps then change nothing; they
        keep the logarithms finite for any other caller. */
    void setTargetValues (const BandSettings& settings) noexcept
    {
        target = settings;
        targetLnF = std::log (orderedClamp (settings.freqHz, rawFreqMin, rawFreqMax));
        targetGainDb = orderedClamp (settings.gainDbEff, gainEffMin, gainEffMax);
        targetLnQ = std::log (orderedClamp (settings.q, rawQMin, rawQMax));
    }

    // The parameter defaults: Bell, 1 kHz, 0 dB, Q 1.
    BandSettings target { Type::bell, 1000.0, 0.0, 1.0, 0 };

    double targetLnF = 6.907755278982137;       // ln 1000
    double targetGainDb = 0.0;
    double targetLnQ = 0.0;

    double lnF = 6.907755278982137;
    double gainDb = 0.0;
    double lnQ = 0.0;

    double incLnF = 0.0;
    double incGainDb = 0.0;
    double incLnQ = 0.0;

    int length = 60;                            // 20 ms at 48 kHz, K = 16
    int counter = 0;
};

} // namespace eq
