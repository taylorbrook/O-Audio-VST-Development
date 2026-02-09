#pragma once

#include <cmath>
#include <algorithm>
#include <array>

// HOA3 Ambisonics Encoding (ACN/SN3D)
// 16-channel 3rd-order spherical harmonics for per-grain spatial encoding.
// All math derived from research/ambisonics-encoding-deep-dive.md

static constexpr int kHOA3Channels = 16;

// SN3D normalization constants
static constexpr float kSqrt3_2  = 0.86602540378f;  // sqrt(3)/2
static constexpr float kSqrt15_2 = 1.93649167310f;  // sqrt(15)/2
static constexpr float kSqrt5_8  = 0.79056941504f;  // sqrt(5/8)
static constexpr float kSqrt3_8  = 0.61237243570f;  // sqrt(3/8)

// Precomputed trig values for SH encoding.
// Uses double/triple-angle identities to derive all terms from just
// sin(a), cos(a), sin(e), cos(e) -- only 2 trig call pairs needed.
struct AmbiTrigCache
{
    float sinA, cosA;
    float sinE, cosE;
    float sin2A, cos2A;
    float sin3A, cos3A;
    float cosE2, cosE3;
    float sinE2;
    float sin2E;

    void compute (float azimuth, float elevation)
    {
        sinA = std::sin (azimuth);
        cosA = std::cos (azimuth);
        sinE = std::sin (elevation);
        cosE = std::cos (elevation);

        sin2A = 2.0f * sinA * cosA;
        cos2A = cosA * cosA - sinA * sinA;

        float sinA2 = sinA * sinA;
        float cosA2 = cosA * cosA;
        sin3A = sinA * (3.0f * cosA2 - sinA2);
        cos3A = cosA * (cosA2 - 3.0f * sinA2);

        cosE2 = cosE * cosE;
        cosE3 = cosE2 * cosE;
        sinE2 = sinE * sinE;
        sin2E = 2.0f * sinE * cosE;
    }
};

// Compute all 16 SN3D encoding coefficients for direction (azimuth, elevation).
// azimuth: 0=front, +pi/2=left, -pi/2=right  (radians)
// elevation: 0=horizon, +pi/2=up, -pi/2=down  (radians)
inline void encodeSH16 (float azimuth, float elevation, float* outCoeffs)
{
    AmbiTrigCache c;
    c.compute (azimuth, elevation);

    // Order 0
    outCoeffs[0]  = 1.0f;

    // Order 1
    outCoeffs[1]  = c.sinA * c.cosE;          // Y
    outCoeffs[2]  = c.sinE;                    // Z
    outCoeffs[3]  = c.cosA * c.cosE;          // X

    // Order 2
    outCoeffs[4]  = kSqrt3_2 * c.sin2A * c.cosE2;
    outCoeffs[5]  = kSqrt3_2 * c.sinA * c.sin2E;
    outCoeffs[6]  = 0.5f * (3.0f * c.sinE2 - 1.0f);
    outCoeffs[7]  = kSqrt3_2 * c.cosA * c.sin2E;
    outCoeffs[8]  = kSqrt3_2 * c.cos2A * c.cosE2;

    // Order 3
    outCoeffs[9]  = kSqrt5_8 * c.sin3A * c.cosE3;
    outCoeffs[10] = kSqrt15_2 * c.sin2A * c.sinE * c.cosE2;
    outCoeffs[11] = kSqrt3_8 * c.sinA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    outCoeffs[12] = 0.5f * c.sinE * (5.0f * c.sinE2 - 3.0f);
    outCoeffs[13] = kSqrt3_8 * c.cosA * c.cosE * (5.0f * c.sinE2 - 1.0f);
    outCoeffs[14] = kSqrt15_2 * c.cos2A * c.sinE * c.cosE2;
    outCoeffs[15] = kSqrt5_8 * c.cos3A * c.cosE3;
}

// One-pole smoothed SH coefficients for click-free grain movement.
// Smoothing the 16 coefficients linearly avoids per-sample trig.
struct SmoothedSHCoeffs
{
    alignas (16) float current[kHOA3Channels] {};
    alignas (16) float target[kHOA3Channels] {};
    float smoothingCoeff = 0.0f;

    void prepare (float sampleRate, float smoothTimeMs = 5.0f)
    {
        cachedSampleRate = sampleRate;
        setSmoothTime (smoothTimeMs);
    }

    void setSmoothTime (float smoothTimeMs)
    {
        if (cachedSampleRate <= 0.0f) return;
        float tau = std::max (smoothTimeMs, 0.5f) * 0.001f;
        smoothingCoeff = 1.0f - std::exp (-1.0f / (tau * cachedSampleRate));
    }

    float cachedSampleRate = 0.0f;

    void setTarget (float azimuth, float elevation)
    {
        encodeSH16 (azimuth, elevation, target);
    }

    void advance()
    {
        for (int ch = 0; ch < kHOA3Channels; ++ch)
            current[ch] += smoothingCoeff * (target[ch] - current[ch]);
    }

    void snapToTarget()
    {
        std::copy (std::begin (target), std::end (target), std::begin (current));
    }
};
