/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
#pragma once

#include "PerlinNoise.h"
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

struct MotionState
{
    float azimuth   = 0.0f;
    float elevation = 0.0f;
    float distance  = 1.0f;
};

class MotionEngine
{
public:
    void prepare (double sampleRate);
    void advance (int numSamples);
    MotionState getCurrentState() const { return currentState; }

    void setPath (int pathIndex);
    void setSpeed (float hz);
    void setWidth (float degrees);
    void setDepth (float percent);
    void setTilt (float degrees);
    void setPhase (float degrees);
    void setElevationEnabled (bool enabled);
    void setElevationRange (float degrees);
    void setTempoSync (int divisionIndex);
    void setHostBpm (double bpm);

private:
    float getEffectiveSpeed() const;

    static constexpr float tempoMultipliers[] = {
        0.0f,         // Off
        4.0f / 3.0f,  // 1/16T
        1.0f,         // 1/16
        2.0f / 3.0f,  // 1/16D
        2.0f / 3.0f,  // 1/8T
        0.5f,         // 1/8
        1.0f / 3.0f,  // 1/8D
        1.0f / 3.0f,  // 1/4T
        0.25f,        // 1/4
        1.0f / 6.0f,  // 1/4D
        0.125f,       // 1/2
        1.0f / 12.0f, // 1/2D
        0.0625f,      // 1 Bar
        0.03125f,     // 2 Bars
        0.015625f     // 4 Bars
    };

    double sampleRate = 48000.0;
    float phaseAccumulator = 0.0f;
    float noiseTime = 0.0f;

    int   pathIndex        = 0;
    float speed            = 1.0f;
    float width            = 180.0f;
    float depth            = 0.0f;
    float tilt             = 0.0f;
    float phase            = 0.0f;
    bool  elevationEnabled = false;
    float elevationRange   = 45.0f;
    int   tempoSyncIndex   = 0;
    double hostBpm         = 120.0;

    PerlinNoise perlin;
    MotionState currentState;
};
