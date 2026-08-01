/*
   This file is part of O-Reed, an Ouaricon Audio plugin.
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

    MouthpieceChamber.h
    O-Reed - Physical Modeling Reed Wind Synthesizer
    Lumped-element mouthpiece chamber compliance (symplectic Euler)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <algorithm>

class MouthpieceChamber
{
public:
    MouthpieceChamber() = default;

    void prepare(double sampleRate)
    {
        dt = 1.0f / static_cast<float>(sampleRate);
        reset();
    }

    // Set physical parameters from APVTS-mapped values
    // volume_m3: chamber volume in m^3 (0 = bypass)
    // boreArea_m2: bore cross-section area at throat (m^2)
    // rho: air density (kg/m^3)
    // c: speed of sound (m/s)
    void setParams(float volume_m3, float boreArea_m2, float rho, float c)
    {
        if (volume_m3 < 1e-8f)
        {
            active = false;
            return;
        }

        active = true;

        // Lumped element model:
        // L_m = effective length of mouthpiece "tube"
        // C_m = acoustic compliance = V / (rho * c^2)
        // M_m = acoustic inertance = rho * L_m / A_bore
        float L_m = volume_m3 / boreArea_m2;
        C_m = volume_m3 / (rho * c * c);
        M_m = rho * L_m / boreArea_m2;
        damping = 0.05f;  // Realistic mouthpiece damping (Q ≈ 3)
    }

    // Process one sample through the mouthpiece chamber
    // u_reed: volume flow from reed (m^3/s)
    // p_bore_minus: wave arriving from bore (Pa)
    // Z_c: characteristic impedance (Pa*s/m^3)
    // Returns p_bore_plus: wave injected into bore
    float processSample(float u_reed, float p_bore_minus, float Z_c)
    {
        if (!active)
        {
            // Bypass: direct flow-to-wave conversion (Phase 3.1 behavior)
            return Z_c * u_reed + p_bore_minus;
        }

        // Symplectic Euler integration of lumped chamber ODE:
        // dp_chamber/dt = (u_reed - u_bore) / C_m
        // du_bore/dt = (p_chamber - p_bore_minus) / M_m

        // Velocity first (symplectic)
        u_bore += (p_chamber - p_bore_minus) / M_m * dt;
        u_bore *= (1.0f - damping);  // Light damping

        // Pressure update
        p_chamber += (u_reed - u_bore) / C_m * dt;

        // Clamp state to prevent blowup at extreme parameter combos
        constexpr float maxPressure = 1e6f;
        constexpr float maxFlow = 1.0f;
        p_chamber = std::max(-maxPressure, std::min(p_chamber, maxPressure));
        u_bore = std::max(-maxFlow, std::min(u_bore, maxFlow));

        // NaN guard: reset state if corrupt
        if (!(std::isfinite(p_chamber) && std::isfinite(u_bore)))
        {
            p_chamber = 0.0f;
            u_bore = 0.0f;
        }

        // Flow-to-wave conversion using bore flow (not reed flow)
        return Z_c * u_bore + p_bore_minus;
    }

    // Initialize state to avoid transients when chamber becomes active
    void initializeState(float p_mouth)
    {
        p_chamber = p_mouth;
        u_bore = 0.0f;
    }

    void reset()
    {
        p_chamber = 0.0f;
        u_bore = 0.0f;
        active = false;
        C_m = 1e-10f;
        M_m = 1.0f;
    }

private:
    float dt      = 1.0f / 44100.0f;
    float C_m     = 1e-10f;  // Acoustic compliance (m^3/Pa)
    float M_m     = 1.0f;    // Acoustic inertance (Pa*s^2/m^3)
    float damping = 0.05f;

    // State variables
    float p_chamber = 0.0f;  // Chamber pressure (Pa)
    float u_bore    = 0.0f;  // Flow into bore (m^3/s)

    bool active = false;
};
