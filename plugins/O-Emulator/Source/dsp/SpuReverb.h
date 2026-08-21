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

    O-Emulator — SpuReverb (Stage 2, Task 10)

    PS1 SPU reverb, the psx-spx register model, run at a FIXED 22050 Hz tick
    (the real SPU reverb ran at half rate — one tick per two output samples;
    that half-rate murk is the character). Topology per ARCHITECTURE
    "Algorithm Details", per tick:

      1. input × vLIN/vRIN
      2. same-side reflections:  [mLSAME] = (Lin + [dLSAME]·vWALL − [mLSAME−1])·vIIR + [mLSAME−1]
         different-side (cross): [mLDIFF] = (Lin + [dRDIFF]·vWALL − [mLDIFF−1])·vIIR + [mLDIFF−1]
         (mirrored for R)
      3. early echo: Lout = Σ vCOMBi·[mLCOMBi]
      4. two series all-pass stages (dAPF1/dAPF2)
      5. advance the circular work-buffer pointer by 1

    One shared circular work buffer; every register is an OFFSET added to the
    advancing pointer, so a write at offset m read at offset d is a delay of
    (m − d) ticks. Register offsets are laid out in disjoint descending
    regions so every read resolves to its intended writer (the most recent
    write to that address).

    ── Preset provenance (flagged in the Phase 2.2 report) ───────────────────
    The TOPOLOGY is the psx-spx register model verbatim. The "Hall" register
    VALUES below are a self-derived, named, spec-shaped set (delays in ms
    chosen for the PsyQ-Hall character: ~16–54 ms early reflections, ~65–77 ms
    wall recirculation loops, RT60 ≈ 0.9 s murky decay; gains inside the SPU's
    /0x8000 ranges) rather than a verbatim PsyQ table transcription — same
    reasoning as the Gaussian table (no reference access; hallucinated hex
    constants in a FEEDBACK network are worse than a derived stable set).
    A verbatim psx-spx Hall table can replace the constants data-only later
    (digest re-anchor required). GPL hygiene: nothing ported from any GPL
    reverb implementation.

    Stability is by construction: the reflection recursion is
    y[n] = vIIR·in + vIIR·vWALL·y[n−D] + (1−vIIR)·y[n−1], and
    |1−vIIR| + |vIIR·vWALL| = 0.45 + 0.3025 < 1; APF gains < 1. The
    std::isfinite guard is belt-and-braces: it resets STATE ONLY (the work
    buffer) and keeps coefficients — non-sticky
    (pattern_biquad_nan_guard_sticky_silence).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace oemu
{

class SpuReverb
{
public:
    /** The fixed reverb domain rate — one tick per 22050 Hz sample pair. */
    static constexpr double kRate = 22050.0;

    /** Clears the work buffer and pointer. Prepare-time / switch-time. */
    void reset() noexcept;

    /** One reverb tick: stereo in (already send-scaled), stereo out. */
    void processTick (float inL, float inR, float& outL, float& outR) noexcept;

private:
    static constexpr int kBufSize = 32768;   // ~1.49 s @ 22.05 kHz, power of two
    static_assert ((kBufSize & (kBufSize - 1)) == 0, "index walk uses & (kBufSize - 1)");

    float& at (int offset) noexcept
    {
        return buf[(size_t) ((pos + offset) & (kBufSize - 1))];
    }

    float buf[kBufSize] {};
    int pos = 0;
};

} // namespace oemu
