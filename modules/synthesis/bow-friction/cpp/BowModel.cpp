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

    BowModel.cpp
    Ouaricon (bow-friction module) - Bow Excitation Envelope Model
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1b extraction — implementation unchanged from O-Bowed.

  ==============================================================================
*/

#include "BowModel.h"
#include <cmath>

void BowModel::prepare (double sr)
{
    sampleRate = sr;

    // Default release coefficient: 30ms exponential decay
    releaseCoeff = std::exp (-1.0f / (0.030f * static_cast<float> (sampleRate)));
}

void BowModel::startBow (float velocity)
{
    // Start each note from silence so retriggers during a previous note's
    // release decay don't carry residual bow state into the new attack.
    v_bow = 0.0f;
    F_bow = 0.0f;

    // Map MIDI velocity (0-1) to bow speed multiplier: minimum 20% speed
    float multiplier = 0.2f + 0.8f * velocity;
    v_bow_target = bowSpeedParam * multiplier;
    F_bow_target = bowPressureParam;

    // Velocity-dependent attack time: 50ms at vel=0, 5ms at vel=1
    float attackTime = 0.050f - 0.045f * velocity;
    attackCoeff = 1.0f - std::exp (-1.0f / (attackTime * static_cast<float> (sampleRate)));

    bowActive = true;
}

void BowModel::stopBow()
{
    bowActive = false;
    // Release ramp begins in updateEnvelope()
}

void BowModel::reset()
{
    v_bow = 0.0f;
    F_bow = 0.0f;
    v_bow_target = 0.0f;
    F_bow_target = 0.0f;
    bowActive = false;
}

void BowModel::updateEnvelope()
{
    if (bowActive)
    {
        // One-pole smooth toward targets
        v_bow += (v_bow_target - v_bow) * attackCoeff;
        F_bow += (F_bow_target - F_bow) * attackCoeff;
    }
    else
    {
        // Exponential decay toward zero
        v_bow *= releaseCoeff;
        F_bow *= releaseCoeff;
    }
}

void BowModel::setBowSpeed (float speed)
{
    // Update target if bow is active (real-time parameter changes during sustain)
    if (bowActive && bowSpeedParam > 0.0001f)
    {
        // Preserve the velocity multiplier ratio from startBow()
        float ratio = v_bow_target / bowSpeedParam;
        v_bow_target = speed * ratio;
    }

    bowSpeedParam = speed;
}

void BowModel::setBowPressure (float pressure)
{
    bowPressureParam = pressure;

    if (bowActive)
        F_bow_target = pressure;
}

bool BowModel::isActive() const noexcept
{
    return bowActive || (std::abs (v_bow) > 1e-7f);
}
