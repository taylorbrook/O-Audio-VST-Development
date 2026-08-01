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

#include <cmath>
#include <cstdint>

class PerlinNoise
{
public:
    void seed (uint32_t s)
    {
        for (int i = 0; i < 256; ++i)
            perm[i] = (uint8_t) i;

        for (int i = 255; i > 0; --i)
        {
            s = s * 1664525u + 1013904223u;
            int j = (int) ((s >> 16) % (uint32_t) (i + 1));
            uint8_t tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }

        for (int i = 0; i < 256; ++i)
            perm[256 + i] = perm[i];
    }

    float noise (float x) const
    {
        int xi = (int) std::floor (x) & 255;
        float xf = x - std::floor (x);
        float u = fade (xf);

        float a = grad (perm[xi], xf);
        float b = grad (perm[xi + 1], xf - 1.0f);
        return lerp (a, b, u);
    }

    float fbm (float x, int octaves = 4, float lacunarity = 2.0f,
               float persistence = 0.5f) const
    {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxAmplitude = 0.0f;

        for (int i = 0; i < octaves; ++i)
        {
            value += noise (x * frequency) * amplitude;
            maxAmplitude += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return value / maxAmplitude;
    }

private:
    uint8_t perm[512] {};

    static float fade (float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static float lerp (float a, float b, float t)
    {
        return a + t * (b - a);
    }

    static float grad (int hash, float x)
    {
        return (hash & 1) ? -x : x;
    }
};
