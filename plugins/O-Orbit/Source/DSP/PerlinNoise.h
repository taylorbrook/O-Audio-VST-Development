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
