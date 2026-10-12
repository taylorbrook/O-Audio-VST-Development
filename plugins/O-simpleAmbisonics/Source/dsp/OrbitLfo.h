/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    OrbitLfo — automatic azimuth motion (FUNC-06, ARCHITECTURE §Orbit LFO).

    Phase accumulator advanced ONCE PER CONTROL STEP by rate·kControlGrid/fs
    (double, so a 2 Hz orbit over an hour has no drift artefact), wrapped to
    [0, 1). Effective azimuth target = wrapTo180(azimuth + 360·phase); the
    positive sense is counter-clockwise from above (front → left → back →
    right), i.e. increasing AmbiX azimuth.

      Off → On : phase reset to 0 so the orbit departs from the puck.
      On  → Off: returns the plain azimuth; the WrapSmoother glides az_eff
                 back the short way.

    Never writes a host parameter (audio thread). The UI puck follows the
    published MeterFrame::azEffDeg.

    Lag note (CONTEXT-2.1): at 2 Hz with τ = 20 ms the smoothed azimuth trails
    this target by atan(2π·2·0.02) ≈ 14°, a constant phase offset of the orbit,
    not a wobble.

    Headless: <cmath> only. No JUCE.

  ==============================================================================
*/

#pragma once

#include <cmath>

#include "Smoothers.h"

namespace OSimpleAmbisonics::dsp
{

struct OrbitLfo
{
    double phase = 0.0;
    bool   wasOn = false;

    void reset() noexcept
    {
        phase = 0.0;
        wasOn = false;
    }

    // Called exactly once per control step.
    float targetAzimuthDeg (bool on, float rateHz, float azimuthDeg, double sampleRate) noexcept
    {
        if (on && ! wasOn)
            phase = 0.0;

        wasOn = on;

        if (! on)
            return azimuthDeg;

        phase += (double) rateHz * (double) kControlGrid / sampleRate;
        phase -= std::floor (phase);

        return wrapTo180 (azimuthDeg + (float) (360.0 * phase));
    }
};

} // namespace OSimpleAmbisonics::dsp
