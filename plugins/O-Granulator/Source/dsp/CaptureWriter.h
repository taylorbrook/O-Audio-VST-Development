/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

#include "ControlGrid.h"
#include "EngineConstants.h"

// ============================================================================
// I/O adapter + capture writer (ARCHITECTURE §1–3, PLAN-2.1 T5).
//
// Engine input is always stereo: a mono bus feeds L = R (the caller passes the
// same pointer twice; channel counts come from the BUS layout, never from the
// buffer width). Per sample:
//
//     ring.push (inL·g + fb[n − 32].L,  inR·g + fb[n − 32].R)
//
// g = the input-gain ramp lerped inside the cell. fb is the z^-32 feedback
// return: slot (n mod 32) still holds what the feedback chain wrote 32 samples
// ago, and is overwritten after this piece's feedback step (2.4). In 2.1 the
// return is all zero. The freeze write arbiter (no push while frozen, unfreeze
// content crossfade) arrives in 2.4.
//
// ring.clear() / prepare() happen ONLY in Engine::prepare() on the message
// thread — never here (a 27.6 MB memset on the audio thread).
// ============================================================================
namespace ogran
{
    struct CaptureWriter
    {
        /** Pushes piece samples [0, n) into the ring. cellPos = the piece's first
            sample's position inside its cell (piece ⊂ one cell, so cellPos + n <= kCell,
            and since kFbDelay == kCell the return slot of sample i is cellPos + i). */
        static void write (Ring& ring, const float* inL, const float* inR, int n, int cellPos,
                           const ExactRamp& inputGain, const float (&fbReturn)[2][kFbDelay]) noexcept
        {
            static_assert (kFbDelay == kCell, "return-slot indexing below assumes kFbDelay == kCell");
            for (int i = 0; i < n; ++i)
            {
                const int c = cellPos + i;
                const float g = inputGain.value (c);
                ring.push (inL[i] * g + fbReturn[0][c], inR[i] * g + fbReturn[1][c]);
            }
        }
    };
} // namespace ogran
