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

#include <juce_core/juce_core.h>
#include <vector>

struct Speaker
{
    float azimuth   = 0.0f;   // degrees, counter-clockwise: 0=front, +90=left
    float elevation = 0.0f;   // degrees, 0=horizon, +90=up
    float distance  = 1.0f;   // meters from center
    juce::String label;
    bool isLFE      = false;
};

struct SpeakerLayout
{
    juce::String name;
    std::vector<Speaker> speakers;
    bool is3D = false;

    int getChannelCount() const { return (int) speakers.size(); }
};
