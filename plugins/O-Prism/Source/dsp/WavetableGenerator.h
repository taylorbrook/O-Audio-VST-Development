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

    WavetableGenerator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include <memory>

enum class WaveShape
{
    Saw = 0,
    Square,
    Triangle,
    Sine
};

class WavetableGenerator
{
public:
    static std::unique_ptr<WavetableData> generateProceduralTable (WaveShape shape);
    static void generateMipmaps (WavetableData& table);
    static void generateMipmapsForFrame (WavetableData& table, int frameIndex);

private:
    static void generateSaw (float* buffer, int size);
    static void generateSquare (float* buffer, int size);
    static void generateTriangle (float* buffer, int size);
    static void generateSine (float* buffer, int size);
};
