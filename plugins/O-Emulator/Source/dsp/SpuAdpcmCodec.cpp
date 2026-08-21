/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

#include "SpuAdpcmCodec.h"

namespace oemu
{

namespace
{
    /*  PS1 SPU-ADPCM predictor pairs (f0, f1) in /64 fixed point, exactly as
        published in the psx-spx SPU chapter (ARCHITECTURE "SPU-ADPCM Codec"):

            (0, 0), (60, 0), (115, −52), (98, −55), (122, −60)

        Hardware-constant data entered from spec (GPL hygiene note in
        SpuAdpcmCodec.h). Note filters 1/2 coincide with BRR F1/F2's /64
        reductions — the two codec families share the same fixed-point
        predictor form, which is what makes AdpcmEncoder a single skeleton. */
    const AdpcmEncoder<SpuAdpcmCodec::kBlockLen, 5>::CoeffTable kSpuCoeffs { {
        {   0,   0 },
        {  60,   0 },
        { 115, -52 },
        {  98, -55 },
        { 122, -60 },
    } };
} // namespace

void SpuAdpcmCodec::reset()
{
    encoder.prepare (kSpuCoeffs);
}

} // namespace oemu
