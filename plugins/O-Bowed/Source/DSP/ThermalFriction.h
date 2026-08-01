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

    ThermalFriction.h
    O-Bowed - Thermal Friction Model (Quality Tier)
    Ouaricon Audio
    Developer: Taylor Brook

    Composes ElastoPlasticFriction with temperature-dependent coefficient
    scaling. Models rosin softening under sustained bowing (glass transition).
    Precomputed exp() LUT for real-time performance.

  ==============================================================================
*/

#pragma once
#include "ElastoPlasticFriction.h"
#include <cmath>

class ThermalFriction
{
public:
    // Must be called in prepareToPlay to build exp() LUT
    void prepare (double sampleRate) noexcept
    {
        // Build exp LUT: 256 entries from T_glass to T_max
        for (int i = 0; i < lutSize; ++i)
        {
            float T = T_glass + (T_max - T_glass) * static_cast<float> (i) / static_cast<float> (lutSize - 1);
            expLUT[i] = std::exp (-alpha_T * (T - T_glass));
        }
    }

    float computeReflectionCoefficient (float v_delta, float F_bow, float dt) noexcept
    {
        // 1. Get base rho from elasto-plastic model
        float rho_base = elastoPlastic.computeReflectionCoefficient (v_delta, F_bow, dt);

        // 2. Approximate friction force for heating calculation
        //    F_friction ~ rho_base * 4 * R_s * |v_delta|
        float F_friction_approx = rho_base * 4.0f * R_s * std::abs (v_delta);

        // 3. Update temperature with Euler step
        float Q_gen = std::abs (F_friction_approx * v_delta);     // Frictional heating
        float Q_loss = k_cool * (T_contact - T_ambient);          // Newton cooling
        float dT = (Q_gen - Q_loss) / C_thermal;
        T_contact += dT * dt;

        // Clamp temperature
        T_contact = std::max (T_ambient, std::min (T_max, T_contact));

        // 4. Compute thermal scale factor
        float thermalScale = computeThermalScale (T_contact);

        // 5. Return thermally-modulated reflection coefficient
        float rho = rho_base * thermalScale;
        return std::max (0.0f, std::min (1.0f, rho));
    }

    float computeFrictionDerivative (float v_rel, float F_bow, float dt) const noexcept
    {
        // Delegate to elasto-plastic (thermal modulation is slow-varying)
        return elastoPlastic.computeFrictionDerivative (v_rel, F_bow, dt);
    }

    void resetState() noexcept
    {
        elastoPlastic.resetState();
        T_contact = T_ambient;
    }

    // Match HyperbolicFriction API
    void setRosin (float rosinParam) noexcept
    {
        elastoPlastic.setRosin (rosinParam);
    }

    void setStringImpedance (float impedance) noexcept
    {
        R_s = impedance;
        elastoPlastic.setStringImpedance (impedance);
    }

    void setBristleStiffness (float stiffness) noexcept
    {
        elastoPlastic.setBristleStiffness (stiffness);
    }

private:
    float computeThermalScale (float T) const noexcept
    {
        if (T <= T_glass)
            return 1.0f;

        // LUT interpolation for exp(-alpha_T * (T - T_glass))
        float normalized = (T - T_glass) / (T_max - T_glass);
        normalized = std::max (0.0f, std::min (1.0f, normalized));
        float index = normalized * static_cast<float> (lutSize - 1);
        int idx0 = static_cast<int> (index);
        int idx1 = std::min (idx0 + 1, lutSize - 1);
        float frac = index - static_cast<float> (idx0);

        return expLUT[idx0] + frac * (expLUT[idx1] - expLUT[idx0]);
    }

    ElastoPlasticFriction elastoPlastic;

    // Temperature state
    float T_contact = 20.0f;      // Current contact temperature (Celsius)

    // Thermal parameters
    float T_glass   = 49.0f;      // Glass transition temperature
    float T_ambient = 20.0f;      // Ambient temperature
    float T_max     = 150.0f;     // Maximum temperature clamp
    float alpha_T   = 0.15f;      // Thermal decay rate
    float C_thermal = 0.002f;     // Thermal mass
    float k_cool    = 0.5f;       // Cooling coefficient

    // String impedance (duplicated for force approximation)
    float R_s = 0.5f;

    // Precomputed exp() LUT
    static constexpr int lutSize = 256;
    float expLUT[lutSize] = {};
};
