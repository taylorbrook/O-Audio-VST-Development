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
#pragma once

// Note division multipliers: beats per cycle, shared by the tempo-synced
// global LFOs (PluginProcessor::advanceGlobalLfoPhases), the tempo-synced
// delay (WR-03), and the per-voice LFOs (PrismVoice::renderNextBlock).
// Index order matches getLfoDivisionNames():
//   1/1, 1/2, 1/4, 1/8, 1/16, 1/32,
//   1/1D, 1/2D, 1/4D, 1/8D, 1/16D, 1/32D,
//   1/1T, 1/2T, 1/4T, 1/8T, 1/16T, 1/32T
namespace NoteDiv
{
    inline constexpr float kDivBeats[18] = {
        4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f,                   // straight
        6.0f, 3.0f, 1.5f, 0.75f, 0.375f, 0.1875f,                // dotted (1.5x)
        2.6667f, 1.3333f, 0.6667f, 0.3333f, 0.1667f, 0.0833f     // triplet (2/3x)
    };
}
