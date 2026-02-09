#pragma once

#include <cmath>
#include <algorithm>

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
    //   speed: trajectory speed multiplier (1.0 = normal, 0→4.0 range)
    //   rngState: mutable state for random walk, updated in place
    inline Result update (int type, float baseAz, float baseEl,
                          float t, float width, float speed, unsigned int& rngState)
    {
        // Fast LCG for random trajectory (audio-thread safe, no allocation)
        auto nextRng = [&rngState]() -> float {
            rngState = rngState * 1664525u + 1013904223u;
            return static_cast<float> (rngState & 0x7FFFFFu) / static_cast<float> (0x7FFFFFu) * 2.0f - 1.0f;
        };

        const float pi = 3.14159265359f;
        float ts = std::min (t * speed, 1.0f);  // speed-scaled progress, clamped

        switch (type)
        {
            case Orbital:
            {
                // Orbit around center at constant elevation
                float orbitRange = pi * width * speed;  // faster = wider arc
                return { baseAz + orbitRange * t, baseEl };
            }

            case Spiral:
            {
                // Spiral outward: azimuth rotates, elevation rises
                float spiralRange = pi * width * speed;
                float elRange = (pi / 4.0f) * width * speed;
                return { baseAz + spiralRange * t,
                         baseEl + elRange * t };
            }

            case Random:
            {
                // Brownian-style random walk, scaled by width, speed, and progress
                float stepScale = 0.3f * width * speed;
                float azOffset = nextRng() * stepScale * ts;
                float elOffset = nextRng() * stepScale * ts * 0.5f;
                return { baseAz + azOffset, baseEl + elOffset };
            }

            case Static:
            default:
                return { baseAz, baseEl };
        }
    }
}
