/*
  ==============================================================================

    HannWindow.h
    O-Texture - Pre-computed periodic Hann window
    Ouaricon Audio

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>

template <int BlockSize>
class HannWindow
{
public:
    HannWindow()
    {
        constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
        for (size_t n = 0; n < static_cast<size_t>(BlockSize); ++n)
            window[n] = 0.5f * (1.0f - std::cos(twoPi * static_cast<float>(n)
                                                        / static_cast<float>(BlockSize)));
    }

    const float* data() const noexcept { return window.data(); }
    float operator[](int n) const noexcept { return window[static_cast<size_t>(n)]; }
    static constexpr int size() noexcept { return BlockSize; }

private:
    std::array<float, BlockSize> window{};
};
