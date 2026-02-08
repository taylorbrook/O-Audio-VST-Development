#pragma once

#include <cmath>

// Per-grain trajectory patterns for spatial movement.
// Each trajectory computes updated (azimuth, elevation) based on
// the grain's normalized lifetime progress t (0 at start, 1 at end).

namespace GrainTrajectory
{
    // Trajectory type IDs (match parameter choice indices)
    enum Type
    {
        Static  = 0,
        Orbital = 1,
        Spiral  = 2,
        Random  = 3
    };

    struct Result
    {
        float azimuth;
        float elevation;
    };

    // Update grain position based on trajectory type.
    //   type: trajectory type (Static/Orbital/Spiral/Random)
    //   baseAz, baseEl: initial spawn position (radians)
    //   t: normalized grain lifetime (0→1)
    //   width: spatial_width parameter (0→1), controls movement extent
    //   rngState: mutable state for random walk, updated in place
    inline Result update (int type, float baseAz, float baseEl,
                          float t, float width, unsigned int& rngState)
    {
        // Fast LCG for random trajectory (audio-thread safe, no allocation)
        auto nextRng = [&rngState]() -> float {
            rngState = rngState * 1664525u + 1013904223u;
            return static_cast<float> (rngState & 0x7FFFFFu) / static_cast<float> (0x7FFFFFu) * 2.0f - 1.0f;
        };

        const float pi = 3.14159265359f;

        switch (type)
        {
            case Orbital:
            {
                // Orbit around center at constant elevation
                float orbitRange = pi * width;  // max full circle at width=1
                return { baseAz + orbitRange * t, baseEl };
            }

            case Spiral:
            {
                // Spiral outward: azimuth rotates, elevation rises
                float spiralRange = pi * width;
                float elRange = (pi / 4.0f) * width;
                return { baseAz + spiralRange * t,
                         baseEl + elRange * t };
            }

            case Random:
            {
                // Brownian-style random walk, scaled by width and progress
                float stepScale = 0.3f * width;
                float azOffset = nextRng() * stepScale * t;
                float elOffset = nextRng() * stepScale * t * 0.5f;
                return { baseAz + azOffset, baseEl + elOffset };
            }

            case Static:
            default:
                return { baseAz, baseEl };
        }
    }
}
