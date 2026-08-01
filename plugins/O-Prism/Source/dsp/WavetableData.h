/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    WavetableData.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <vector>
#include <cstdint>

struct WavetableData
{
    static constexpr int kTableSize = 2048;
    static constexpr int kGuardSamples = 1;
    static constexpr int kFrameSize = kTableSize + kGuardSamples; // 2049
    static constexpr int kMaxFrames = 256;
    static constexpr int kNumMipmapLevels = 10;

    int numFrames = 0;
    std::vector<float> data; // Flat: [level][frame][sample+guard]

    void allocate (int frames)
    {
        numFrames = frames;
        data.resize (static_cast<size_t> (kNumMipmapLevels) * static_cast<size_t> (numFrames) * kFrameSize, 0.0f);
    }

    float getSample (int level, int frame, int sampleIndex) const
    {
        return data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize + sampleIndex)];
    }

    void setSample (int level, int frame, int sampleIndex, float value)
    {
        data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize + sampleIndex)] = value;
    }

    float* getFrameData (int level, int frame)
    {
        return &data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize)];
    }

    const float* getFrameData (int level, int frame) const
    {
        return &data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize)];
    }

    void setGuardSamples()
    {
        for (int level = 0; level < kNumMipmapLevels; ++level)
        {
            for (int frame = 0; frame < numFrames; ++frame)
            {
                // Guard sample = copy of first sample (wraps for interpolation)
                setSample (level, frame, kTableSize, getSample (level, frame, 0));
            }
        }
    }
};
