#pragma once

#include <array>
#include <algorithm>

namespace EuclideanGenerator
{
    inline std::array<bool, 16> generate (int steps, int pulses)
    {
        std::array<bool, 16> pattern {};
        if (steps <= 0) return pattern;
        pulses = std::clamp (pulses, 0, steps);

        for (int i = 0; i < steps && i < 16; ++i)
            pattern[static_cast<size_t> (i)] = ((i * pulses) % steps) < pulses;

        return pattern;
    }
}
