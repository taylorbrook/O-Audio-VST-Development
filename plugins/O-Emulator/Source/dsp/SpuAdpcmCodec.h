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
/*
  ==============================================================================

    O-Emulator — SpuAdpcmCodec (Stage 2, Task 9)

    PS1 SPU-ADPCM: 28-sample blocks, shift 0–12, 5 predictor filter pairs,
    closed-loop brute-force encoder (AdpcmEncoder<28, 5> with the SPU
    coefficient table — see SpuAdpcmCodec.cpp). Presents a constant
    28-console-sample delay, exactly the figure the worst-case latency
    formula budgets (plan decision #2).

    GPL hygiene: coefficient pairs entered from the published psx-spx SPU
    spec (ARCHITECTURE "SPU-ADPCM Codec"); no GPL implementation ported.

  ==============================================================================
*/

#pragma once

#include "AdpcmEncoder.h"

namespace oemu
{

class SpuAdpcmCodec
{
public:
    static constexpr int kBlockLen = 28;

    /** Binds the SPU coefficient table and clears all state. Prepare-time
        only (never on the audio thread mid-stream). */
    void reset();

    void setShiftFloor (int f) noexcept { encoder.setShiftFloor (f); }

    /** Round trip: in [-1, 1] float, out = decode(encode(x)) delayed by
        exactly kBlockLen console samples. */
    float processSample (float x) noexcept { return encoder.processSample (x); }

private:
    AdpcmEncoder<kBlockLen, 5> encoder;
};

} // namespace oemu
