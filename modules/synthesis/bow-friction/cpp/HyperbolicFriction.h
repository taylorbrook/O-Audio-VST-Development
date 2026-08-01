/*
   This file is part of the Ouaricon Audio bow-friction module.
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

    HyperbolicFriction.h
    Ouaricon - Memoryless Hyperbolic Friction Model
    (shared module: bow-friction v1.0.0; consumers: O-Bowed, O-Contrabass)
    Ouaricon Audio
    Developer: Taylor Brook

    STK-style memoryless friction curve. Computes waveguide reflection
    coefficient from differential velocity and bow force. O(1) per sample,
    no iteration required. Always stable (rho bounded in [0, ~0.5]).

    Treble defaults (mu_s = 0.8, mu_d = 0.3) preserved verbatim from
    O-Bowed pre-extraction state. Bass-string consumers (O-Contrabass)
    inject bass-coefficient overrides via setStaticFrictionCoefficient
    + setDynamicFrictionCoefficient setter calls in prepareToPlay
    (Phase 2.1b extraction; RESEARCH §13.3-Q5).

  ==============================================================================
*/

#pragma once
#include <cmath>

class HyperbolicFriction
{
public:
    // Compute reflection coefficient from differential velocity and bow force.
    // v_delta = v_bow - v_string_incoming (already computed by caller)
    // F_bow   = current bow force from BowModel envelope
    // Returns rho in [0, ~0.5] -- bounded, always stable.
    float computeReflectionCoefficient (float v_delta, float F_bow) const noexcept
    {
        float absV = std::abs (v_delta);
        float mu = mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);

        // Convert friction coefficient to waveguide reflection coefficient
        // r = (1/4) * mu * F_bow / R_s
        float r = 0.25f * mu * F_bow / R_s;
        float rho = r / (1.0f + r);

        return rho;
    }

    // ROSIN 0.0 = smooth (v_0 = 0.1), ROSIN 1.0 = aggressive (v_0 = 0.01)
    void setRosin (float rosinParam) noexcept
    {
        v_0 = 0.1f * std::exp (-4.6f * rosinParam);
    }

    void setStringImpedance (float impedance) noexcept
    {
        R_s = impedance;
    }

    // Phase 2.1b — bass-string consumer hooks (RESEARCH §13.3-Q5).
    // O-Bowed never calls these (treble = init defaults, mu_s=0.8, mu_d=0.3).
    // O-Contrabass calls both in prepareToPlay (bass = mu_s=0.85, mu_d=0.25).
    void setStaticFrictionCoefficient  (float mu) noexcept   { mu_s = mu; }
    void setDynamicFrictionCoefficient (float mu) noexcept   { mu_d = mu; }

private:
    float mu_s = 0.8f;   // static friction coefficient
    float mu_d = 0.3f;   // dynamic friction coefficient
    float v_0  = 0.05f;  // characteristic velocity (from ROSIN)
    float R_s  = 0.5f;   // string wave impedance (fixed for Phase 3.1)
};
