/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    ElastoPlasticFriction.h
    O-Bowed - Elasto-Plastic Bristle Friction Model (Enhanced Tier)
    Ouaricon Audio
    Developer: Taylor Brook

    Serafin/Avanzini 2003 single-state bristle model with passivity fix.
    Stick-slip hysteresis via bristle displacement state variable z.
    O(1) per sample, forward Euler integration.

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <algorithm>

class ElastoPlasticFriction
{
public:
    // Compute reflection coefficient from differential velocity and bow force.
    // NON-const: updates internal bristle state z.
    float computeReflectionCoefficient (float v_delta, float F_bow, float dt) noexcept
    {
        float v_rel = v_delta;
        float absV = std::abs (v_rel);

        // Steady-state bristle displacement (Stribeck curve)
        float mu = mu_d + (mu_s - mu_d) * std::exp (-absV * 10.0f);
        float z_ss_val = (absV > 1e-10f)
                             ? mu * F_bow / sigma_0 * (v_rel > 0.0f ? 1.0f : -1.0f)
                             : 0.0f;

        // Compute alpha (transition function)
        float absZ = std::abs (z);
        float alpha;

        if (absZ < z_ss)
        {
            alpha = 0.0f; // Elastic stick region
        }
        else if (absZ < z_ba)
        {
            // Sinusoidal transition
            float z_m = 0.5f * (z_ba + z_ss);
            float pi = 3.14159265358979323846f;
            alpha = 0.5f * (1.0f + std::sin (pi * (absZ - z_m) / (z_ba - z_ss)));
        }
        else
        {
            alpha = 1.0f; // Breakaway / slip
        }

        // Forward Euler integration of bristle displacement
        float dz = v_rel * (1.0f - alpha * z / (z_ss_val != 0.0f ? z_ss_val : 1e-10f));
        z += dz * dt;

        // Clamp bristle displacement to prevent runaway
        float z_max = 1.5f * z_ba;
        z = std::max (-z_max, std::min (z_max, z));

        // Denormal protection
        if (std::abs (z) < 1e-15f)
            z = 0.0f;

        // Passivity fix: velocity-dependent damping
        float sigma_1_eff = sigma_1 * (1.0f + v_rel * v_rel);

        // Total friction force
        float F_friction = sigma_0 * z + sigma_1_eff * dz + sigma_2 * v_rel;

        // Convert friction force to waveguide reflection coefficient
        float r = std::abs (F_friction) / (4.0f * R_s);
        float rho = r / (1.0f + r);

        // Bound rho to [0, 1]
        rho = std::max (0.0f, std::min (1.0f, rho));

        return rho;
    }

    // Derivative of friction w.r.t. v_rel for Newton-Raphson solver (optional use)
    float computeFrictionDerivative (float v_rel, float F_bow, float dt) const noexcept
    {
        // Numerical derivative via finite difference
        float dv = 0.001f;

        // Create temporary copies for evaluation
        // We approximate using current z state (frozen)
        auto evalForce = [&] (float v) -> float
        {
            float absVv = std::abs (v);
            float mu = mu_d + (mu_s - mu_d) * std::exp (-absVv * 10.0f);
            float z_ss_v = (absVv > 1e-10f)
                               ? mu * F_bow / sigma_0 * (v > 0.0f ? 1.0f : -1.0f)
                               : 0.0f;

            float absZz = std::abs (z);
            float alpha;
            if (absZz < z_ss)
                alpha = 0.0f;
            else if (absZz < z_ba)
            {
                float z_m = 0.5f * (z_ba + z_ss);
                alpha = 0.5f * (1.0f + std::sin (3.14159265f * (absZz - z_m) / (z_ba - z_ss)));
            }
            else
                alpha = 1.0f;

            float dz = v * (1.0f - alpha * z / (z_ss_v != 0.0f ? z_ss_v : 1e-10f));
            float sigma_1_e = sigma_1 * (1.0f + v * v);
            return sigma_0 * z + sigma_1_e * dz + sigma_2 * v;
        };

        float F_plus = evalForce (v_rel + dv);
        float F_minus = evalForce (v_rel - dv);

        return (F_plus - F_minus) / (2.0f * dv);
    }

    void resetState() noexcept
    {
        z = 0.0f;
    }

    // Match HyperbolicFriction API
    void setRosin (float rosinParam) noexcept
    {
        // Stiffness base range: stiffnessParam 0.0 = soft (2000), 1.0 = stiff (10000)
        float baseStiffness = 2000.0f + 8000.0f * stiffnessParam;
        // Rosin modulates ±50% around the stiffness base
        sigma_0 = baseStiffness * (0.5f + rosinParam);

        z_ss = 0.0005f * (1.0f - 0.5f * rosinParam);  // Smaller stick zone when aggressive
        z_ba = z_ss * 2.0f;
    }

    void setStringImpedance (float impedance) noexcept
    {
        // Floor away from 0: R_s divides the friction-force → rho conversion, so a
        // zero impedance would yield Inf → NaN. Latent until WR-02 activated this
        // path; guarded here as its required companion (IN-03).
        R_s = std::max (1.0e-3f, impedance);
    }

    // Bow hair stiffness: 0.0 = soft/flexible (2000), 1.0 = stiff/aggressive (10000)
    void setBristleStiffness (float stiffness) noexcept
    {
        stiffnessParam = stiffness;
    }

private:
    // Bristle state (per-instance, persists between samples)
    float z = 0.0f;

    // Bristle model parameters (Serafin/Avanzini 2003)
    float sigma_0 = 6000.0f;    // bristle stiffness
    float sigma_1 = 0.005f;     // bristle damping (kept low so stiffness term drives rho)
    float sigma_2 = 0.002f;     // viscous friction
    float mu_s = 0.8f;          // static friction coefficient
    float mu_d = 0.3f;          // dynamic friction coefficient

    // Transition thresholds
    float z_ss = 0.0005f;       // stick-to-slip threshold
    float z_ba = 0.001f;        // breakaway threshold

    // String wave impedance
    float R_s = 0.5f;

    // Bow hair stiffness (0.0-1.0)
    float stiffnessParam = 0.5f;
};
