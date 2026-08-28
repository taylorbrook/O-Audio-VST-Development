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
    void setHostPpq (double ppqPosition, bool transportPlaying);

private:
    float getEffectiveSpeed() const;

    // Cycles per BEAT (Hz = bpm/60 * mult). A division names the DURATION of one cycle: "1/4"
    // is one cycle per quarter note, "1 Bar" one cycle per four beats (4/4 assumed — no time
    // signature is read). v1.1.1: the v1.0 table was 4x slower than its labels and used 4/3 for
    // triplets (so 1/16D == 1/8T); a triplet is 3/2 the parent's rate, a dotted note 2/3.
    static constexpr float tempoMultipliers[] = {
        0.0f,         // Off
        6.0f,         // 1/16T
        4.0f,         // 1/16
        8.0f / 3.0f,  // 1/16D
        3.0f,         // 1/8T
        2.0f,         // 1/8
        4.0f / 3.0f,  // 1/8D
        1.5f,         // 1/4T
        1.0f,         // 1/4
        2.0f / 3.0f,  // 1/4D
        0.5f,         // 1/2
        1.0f / 3.0f,  // 1/2D
        0.25f,        // 1 Bar
        0.125f,       // 2 Bars
        0.0625f       // 4 Bars
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

    // PPQ lock (C1): when synced and the transport is rolling, phase is derived
    // from the host's beat position instead of free-running, so offline bounces
    // are deterministic and motion is downbeat-aligned. Free-run is the fallback
    // whenever the transport is stopped or provides no PPQ (Standalone).
    double hostPpq         = 0.0;
    bool   hostPpqValid    = false;

    PerlinNoise perlin;
    MotionState currentState;
};
