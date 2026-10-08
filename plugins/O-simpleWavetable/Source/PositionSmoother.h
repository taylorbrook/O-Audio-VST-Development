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

    The D-N two-pole fallback (two 1 ms poles in series) was pre-authorized
    for Stage 2.3 but never needed: the click gate passed with one pole
    (click ratio 1.414), so v1.0.1 removed the pole-count scaffolding.

    Float state is fine (it settles in about 10 ms).

  ==============================================================================
*/

#pragma once

#include <cmath>

struct PositionSmoother
{
    static constexpr double kTauSeconds = 0.002;

    float a = 1.0f;
    float s = 0.0f;

    void prepare (double fs) noexcept
    {
        const double f = (fs > 0.0 && std::isfinite (fs)) ? fs : 44100.0;
        a = (float) (1.0 - std::exp (-1.0 / (kTauSeconds * f)));
    }

    void seed (float v) noexcept   { s = v; }

    float process (float x) noexcept
    {
        s += a * (x - s);
        return s;
    }
};
