/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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

    O-simpleSubtractive - Cytomic ZDF State-Variable Filter (per voice)

    THE headline component. A zero-delay-feedback trapezoidal-integration SVF
    (Andrew Simper / Vadim Zavalishin). One 2-pole stage computes LP, BP and HP
    SIMULTANEOUSLY each sample; Notch = input - k·BP. All four modes from one
    structure — the "one filter, four shapes" pedagogical story.

    Slopes (handled by the voice, which owns one OnePole + two SvfZDF stages):
      6 dB  = a single 1-pole TPT (no resonance — "a 1-pole can't resonate").
      12 dB = one SvfZDF stage.
      24 dB = two SvfZDF stages cascaded at the same cutoff, resonance on stage 1
              only (prevents runaway peak — O-Prism precedent).

    Self-oscillation (Phase 2.2): the NONLINEAR form applies tanh to the resonant
    integrator state (ic1eq). As k→0 (resonance→max) the filter sustains a clean,
    bounded sine at cutoff; tanh caps the amplitude and IS the gain compensation
    (a linear SVF cannot self-oscillate). A tiny excitation injected by the voice
    when k is near zero lets the limit cycle start from silence.

    Coefficients (g, k) are recomputed PER SAMPLE by the voice for zipper-free
    envelope modulation. State (ic1eq/ic2eq) is cleared on voice (re)allocation
    only, never mid-note.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

//==============================================================================
// One 2-pole ZDF state-variable stage (LP/BP/HP simultaneous).
struct SvfZDF
{
    double ic1eq = 0.0;   // bandpass integrator state (the resonant path)
    double ic2eq = 0.0;   // lowpass integrator state

    void reset() noexcept { ic1eq = 0.0; ic2eq = 0.0; }

    // Process one sample. g = tan(π·fc/fs), k = 1/Q. When `nonlinear`, the
    // resonant integrator is tanh-bounded for clean self-oscillation.
    // Returns lp/bp/hp by reference; the caller derives notch = x - k·bp.
    inline void process (double x, double g, double k, bool nonlinear,
                         double& lp, double& bp, double& hp) noexcept
    {
        const double a1 = 1.0 / (1.0 + g * (g + k));
        const double a2 = g * a1;
        const double a3 = g * a2;

        const double v3 = x - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;

        if (nonlinear)
        {
            // Soft-knee limiter on the resonant (bandpass) integrator. IDENTITY
            // below the knee — so a slightly-negative-k resonator can BUILD from
            // silence — then soft-saturating above it → a clean, bounded limit-
            // cycle sine. (A plain tanh would damp even tiny signals and never let
            // the oscillation grow.)
            const double a = std::abs (ic1eq);
            constexpr double knee = 0.7;
            if (a > knee)
            {
                const double s = ic1eq < 0.0 ? -1.0 : 1.0;
                ic1eq = s * (knee + (1.0 - knee) * std::tanh ((a - knee) / (1.0 - knee)));
            }
            if (! std::isfinite (ic1eq)) ic1eq = 0.0;
            if (! std::isfinite (ic2eq)) ic2eq = 0.0;
        }

        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }
};

//==============================================================================
// One-pole TPT (6 dB/oct). LP/HP only; no resonance.
struct OnePoleTPT
{
    double z1 = 0.0;

    void reset() noexcept { z1 = 0.0; }

    inline void process (double x, double g, double& lp, double& hp) noexcept
    {
        const double G = g / (1.0 + g);
        const double v = (x - z1) * G;
        lp = v + z1;
        z1 = lp + v;
        hp = x - lp;
    }
};
