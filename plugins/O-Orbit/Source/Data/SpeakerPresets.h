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

#include "SpeakerLayout.h"

namespace SpeakerPresets
{

inline SpeakerLayout stereo()
{
    return { "Stereo", {
        { -30.0f, 0.0f, 1.0f, "L" },
        {  30.0f, 0.0f, 1.0f, "R" }
    }, false };
}

inline SpeakerLayout quad()
{
    return { "Quad", {
        { -45.0f,  0.0f, 1.0f, "L" },
        {  45.0f,  0.0f, 1.0f, "R" },
        { -135.0f, 0.0f, 1.0f, "Ls" },
        {  135.0f, 0.0f, 1.0f, "Rs" }
    }, false };
}

inline SpeakerLayout surround51()
{
    return { "5.1", {
        { -30.0f,  0.0f, 1.0f, "L",   false },
        {  30.0f,  0.0f, 1.0f, "R",   false },
        {   0.0f,  0.0f, 1.0f, "C",   false },
        {   0.0f,  0.0f, 1.0f, "LFE", true  },
        { -110.0f, 0.0f, 1.0f, "Ls",  false },
        {  110.0f, 0.0f, 1.0f, "Rs",  false }
    }, false };
}

inline SpeakerLayout surround71()
{
    return { "7.1", {
        { -30.0f,  0.0f, 1.0f, "L",   false },
        {  30.0f,  0.0f, 1.0f, "R",   false },
        {   0.0f,  0.0f, 1.0f, "C",   false },
        {   0.0f,  0.0f, 1.0f, "LFE", true  },
        { -110.0f, 0.0f, 1.0f, "Ls",  false },
        {  110.0f, 0.0f, 1.0f, "Rs",  false },
        { -90.0f,  0.0f, 1.0f, "Lss", false },
        {  90.0f,  0.0f, 1.0f, "Rss", false }
    }, false };
}

inline SpeakerLayout surround514()
{
    return { "5.1.4", {
        { -30.0f,   0.0f, 1.0f, "L",   false },
        {  30.0f,   0.0f, 1.0f, "R",   false },
        {   0.0f,   0.0f, 1.0f, "C",   false },
        {   0.0f,   0.0f, 1.0f, "LFE", true  },
        { -110.0f,  0.0f, 1.0f, "Ls",  false },
        {  110.0f,  0.0f, 1.0f, "Rs",  false },
        { -45.0f,  45.0f, 1.0f, "Ltf", false },
        {  45.0f,  45.0f, 1.0f, "Rtf", false },
        { -135.0f, 45.0f, 1.0f, "Ltr", false },
        {  135.0f, 45.0f, 1.0f, "Rtr", false }
    }, true };
}

inline SpeakerLayout surround714()
{
    return { "7.1.4", {
        { -30.0f,   0.0f, 1.0f, "L",   false },
        {  30.0f,   0.0f, 1.0f, "R",   false },
        {   0.0f,   0.0f, 1.0f, "C",   false },
        {   0.0f,   0.0f, 1.0f, "LFE", true  },
        { -110.0f,  0.0f, 1.0f, "Ls",  false },
        {  110.0f,  0.0f, 1.0f, "Rs",  false },
        { -90.0f,   0.0f, 1.0f, "Lss", false },
        {  90.0f,   0.0f, 1.0f, "Rss", false },
        { -45.0f,  45.0f, 1.0f, "Ltf", false },
        {  45.0f,  45.0f, 1.0f, "Rtf", false },
        { -135.0f, 45.0f, 1.0f, "Ltr", false },
        {  135.0f, 45.0f, 1.0f, "Rtr", false }
    }, true };
}

inline SpeakerLayout hexaphonic()
{
    return { "Hexaphonic", {
        {   0.0f, 0.0f, 1.0f, "1" },
        { -60.0f, 0.0f, 1.0f, "2" },
        { -120.0f, 0.0f, 1.0f, "3" },
        { 180.0f, 0.0f, 1.0f, "4" },
        { 120.0f, 0.0f, 1.0f, "5" },
        {  60.0f, 0.0f, 1.0f, "6" }
    }, false };
}

inline SpeakerLayout octaphonic()
{
    return { "Octaphonic", {
        {    0.0f, 0.0f, 1.0f, "1" },
        {  -45.0f, 0.0f, 1.0f, "2" },
        {  -90.0f, 0.0f, 1.0f, "3" },
        { -135.0f, 0.0f, 1.0f, "4" },
        {  180.0f, 0.0f, 1.0f, "5" },
        {  135.0f, 0.0f, 1.0f, "6" },
        {   90.0f, 0.0f, 1.0f, "7" },
        {   45.0f, 0.0f, 1.0f, "8" }
    }, false };
}

inline SpeakerLayout getPreset (int index)
{
    switch (index)
    {
        case 0: return stereo();
        case 1: return quad();
        case 2: return surround51();
        case 3: return surround71();
        case 4: return surround514();
        case 5: return surround714();
        case 6: return hexaphonic();
        case 7: return octaphonic();
        default: return stereo();
    }
}

} // namespace SpeakerPresets
