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

    O-simpleWavetable - per-voice position smoother (Stage 2.3, RESEARCH 3.2)
    Ouaricon Audio
    Developer: Taylor Brook

    Locked spec: one pole, alpha = 1 - exp(-1 / (tau * fs)), tau = 2 ms,
    seeded at the first rendered sample after note-on.

    D-N (pre-authorized fallback): if the Square / S&H 100%-depth click gate
    fails against its negative control, set kPoles = 2 (two 1 ms poles in
    series, zero initial velocity on a step). Log the change in SUMMARY.

    Test builds (OSIW_TEST_HOOKS) can override the pole count at runtime
    (testPolesOverride, set before prepare) so mod-check can print the D-N
    candidate next to the shipping verdict. Shipping builds: compile-time.

    Float state is fine (it settles in about 10 ms).

  ==============================================================================
*/

#pragma once

#include "TestHooks.h"   // first: the OSIW_TEST_HOOKS default (Stage 2 note 8)

#include <cmath>

struct PositionSmoother
{
    static constexpr int kPoles = 1;                                      // D-N: flip to 2 only if the click gate fails

   #if OSIW_TEST_HOOKS
    static inline int testPolesOverride = 0;                              // 0 = kPoles; 1 or 2 = forced (set before prepare)
    static int activePoles() noexcept
    {
        return (testPolesOverride == 1 || testPolesOverride == 2) ? testPolesOverride : kPoles;
    }
   #else
    static constexpr int activePoles() noexcept { return kPoles; }
   #endif

    static double tauSeconds() noexcept { return activePoles() == 1 ? 0.002 : 0.001; }

    float a  = 1.0f;
    float s1 = 0.0f;
    float s2 = 0.0f;

    void prepare (double fs) noexcept
    {
        const double f = (fs > 0.0 && std::isfinite (fs)) ? fs : 44100.0;
        a = (float) (1.0 - std::exp (-1.0 / (tauSeconds() * f)));
    }

    void seed (float v) noexcept
    {
        s1 = v;
        s2 = v;
    }

    float process (float x) noexcept
    {
        s1 += a * (x - s1);
        if (activePoles() == 1)
            return s1;
        s2 += a * (s1 - s2);
        return s2;
    }
};
