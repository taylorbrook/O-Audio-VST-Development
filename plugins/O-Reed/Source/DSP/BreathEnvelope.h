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

    BreathEnvelope.h
    O-Reed - Physical Modeling Reed Wind Synthesizer
    Breath pressure envelope with attack chiff overshoot
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <algorithm>

class BreathEnvelope
{
public:
    enum class State { Off, Attack, Sustain, Release };

    BreathEnvelope() = default;

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);
        reset();
    }

    // Trigger note onset with velocity (0-1)
    void noteOn(float velocity, float chiffAmount)
    {
        currentState = State::Attack;

        // Velocity-scaled attack time: 50ms (vel=0) -> 5ms (vel=1)
        float attackTimeMs = 50.0f - velocity * 45.0f;
        float attackTimeSamples = sr * attackTimeMs * 0.001f;
        attackCoeff = 1.0f - std::exp(-1.0f / std::max(attackTimeSamples, 1.0f));

        // Chiff overshoot: 0-30% above target based on chiffAmount param * velocity
        overshootFactor = 1.0f + chiffAmount * velocity * 0.3f;

        // Ensure we start from zero for clean onset
        if (currentLevel < 1e-6f)
            currentLevel = 0.0f;
    }

    // Trigger note release
    void noteOff()
    {
        currentState = State::Release;

        // ~150ms exponential release
        float releaseSamples = sr * 0.15f;
        releaseCoeff = 1.0f - std::exp(-1.0f / std::max(releaseSamples, 1.0f));
    }

    // Set the sustain breath pressure target (0-1 normalized)
    void setTarget(float breathPressure)
    {
        targetLevel = breathPressure;
    }

    // Returns current breath pressure (in physical units after scaling)
    float processSample()
    {
        switch (currentState)
        {
            case State::Off:
                currentLevel = 0.0f;
                break;

            case State::Attack:
            {
                // Ramp toward overshoot target
                float attackTarget = targetLevel * overshootFactor;
                currentLevel += (attackTarget - currentLevel) * attackCoeff;

                // Transition to sustain when close to target
                if (currentLevel >= targetLevel * 0.99f)
                {
                    currentState = State::Sustain;
                    overshootFactor = 1.0f;
                }
                break;
            }

            case State::Sustain:
            {
                // Smoothly track target level (allows breath controller changes)
                float sustainSmooth = 0.001f;  // ~1ms response at 44.1kHz
                currentLevel += (targetLevel - currentLevel) * sustainSmooth;
                break;
            }

            case State::Release:
            {
                // Exponential decay toward zero
                currentLevel -= currentLevel * releaseCoeff;

                // Clean cutoff when very quiet
                if (currentLevel < 1e-6f)
                {
                    currentLevel = 0.0f;
                    currentState = State::Off;
                }
                break;
            }
        }

        return currentLevel;
    }

    bool isActive() const
    {
        return currentState != State::Off;
    }

    State getState() const { return currentState; }

    void reset()
    {
        currentState = State::Off;
        currentLevel = 0.0f;
        targetLevel = 0.5f;
        overshootFactor = 1.0f;
        attackCoeff = 0.01f;
        releaseCoeff = 0.001f;
    }

private:
    float sr = 44100.0f;

    State currentState = State::Off;
    float currentLevel = 0.0f;
    float targetLevel = 0.5f;

    float attackCoeff = 0.01f;
    float releaseCoeff = 0.001f;
    float overshootFactor = 1.0f;
};
