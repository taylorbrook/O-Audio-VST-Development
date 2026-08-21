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

#include "BrrCodec.h"

namespace oemu
{

namespace
{
    /*  S-DSP BRR prediction filters (ARCHITECTURE "BRR Codec", exact
        fixed-point ratios), reduced to the shared /64 form:

            F0: s = n                          -> (  0,   0)
            F1: s = n + p1·15/16               -> ( 60,   0)   15/16  = 60/64
            F2: s = n + p1·61/32 − p2·15/16    -> (122, −60)   61/32  = 122/64
            F3: s = n + p1·115/64 − p2·13/16   -> (115, −52)   13/16  = 52/64

        Hardware-constant data entered from the published spec (GPL hygiene
        note in BrrCodec.h). */
    const AdpcmEncoder<BrrCodec::kBlockLen, 4>::CoeffTable kBrrCoeffs { {
        {   0,   0 },   // F0
        {  60,   0 },   // F1
        { 122, -60 },   // F2
        { 115, -52 },   // F3
    } };
} // namespace

void BrrCodec::reset()
{
    encoder.prepare (kBrrCoeffs);
}

} // namespace oemu
