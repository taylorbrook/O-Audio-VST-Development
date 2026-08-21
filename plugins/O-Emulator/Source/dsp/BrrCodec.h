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

    O-Emulator — BrrCodec (Stage 2, Task 4)

    SNES S-DSP Bit Rate Reduction: 16-sample blocks, shift 0–12, prediction
    filters 0–3, closed-loop brute-force encoder (AdpcmEncoder<16, 4> with
    the S-DSP coefficient table — see BrrCodec.cpp for the ratios and their
    /64 reduction). Presents a constant 16-console-sample delay.

    GPL hygiene: filter ratios entered from the published S-DSP spec
    (ARCHITECTURE "BRR Codec"); no GPL implementation ported.

  ==============================================================================
*/

#pragma once

#include "AdpcmEncoder.h"

namespace oemu
{

class BrrCodec
{
public:
    static constexpr int kBlockLen = 16;

    /** Binds the S-DSP coefficient table and clears all state. Prepare-time
        only (never on the audio thread mid-stream). */
    void reset();

    void setShiftFloor (int f) noexcept { encoder.setShiftFloor (f); }

    /** Round trip: in [-1, 1] float, out = decode(encode(x)) delayed by
        exactly kBlockLen console samples. */
    float processSample (float x) noexcept { return encoder.processSample (x); }

private:
    AdpcmEncoder<kBlockLen, 4> encoder;
};

} // namespace oemu
