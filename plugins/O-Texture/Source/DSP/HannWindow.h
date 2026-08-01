/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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
        constexpr float twoPi = 6.283185307179586f; // M_PI is not portable (MSVC)
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
