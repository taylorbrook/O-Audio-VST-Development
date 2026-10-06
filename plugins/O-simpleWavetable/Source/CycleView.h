/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - cycle view + bank thumbnails (Stage 3.2, D-W)
    Ouaricon Audio
    Developer: Taylor Brook

    CycleView   what the cycle / harmonics panels draw (filled in place, no
                heap). Built by the processor's buildCycleView().
    BankThumbs  the bank-stack thumbnails (bankUpdate; NOT on the 30 Hz path).
    CycleRenderer
                renders the heard cycle with the voice's OWN read code
                (wt::readSample) and quantizer (BitQuantizer), in the voice's
                order, then a 2048-point real FFT. So "bars = heard cycle"
                holds by construction (viz-check G-VIZ-EXACT gates it against
                an independent DFT of the rendered audio).

    juce_core + juce_dsp only: never the editor, never the UI resource header (the
    console test targets compile this file without the UI resources, P7).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

#include "BitQuantizer.h"
#include "WavetableBank.h"
#include "WtRead.h"

//==============================================================================
struct CycleView
{
    static constexpr int kPoints    = 256;     // point-sampled cycle (every 8th table sample)
    static constexpr int kHarmonics = 32;      // bars 1..32

    std::array<float, kPoints>    cycle {}, preQ {};                   // preQ = before the quantizer
    std::array<float, kHarmonics> harmonicsDb {}, harmonicsRawDb {};   // dB re heard max, [-60, 0]

    float pos      = 0.0f;   // effective position (lead voice) or the knob when silent
    int   frame    = -1;     // latched frame (Interp Off), -1 when Interp On
    int   level    = 0;      // mip level read (0 when silent)
    int   kmax     = 1023;   // WavetableBank::kmax (level)
    float nyquistH = 0.0f;   // (fs / 2) / f0; 0 when silent
    int   note     = -1;     // lead MIDI note; -1 when silent
    float f0       = 0.0f;   // Hz incl. bend; 0 when silent
    float lfo      = 0.0f;   // -1..1; 0 when lfo_depth is 0 (D-P)
    float menv     = 0.0f;   // 0..1 lead voice mod env
    float amp      = 0.0f;   // 0..1 lead voice amp env
    bool  sounding  = false;
    bool  quantized = false; // bit_depth != Full (preQ is sent)
    bool  empty     = false; // Imported with no bank: silence + prompt
};

//==============================================================================
struct BankThumbs
{
    int  bank      = 0;
    bool imported  = false;      // the Imported slot is selected (even when empty)
    int  numFrames = 0;
    juce::String filename;       // Imported only: cachedBlob.filename (D-S, P8)
    std::vector<float> points;   // numFrames * kThumbSize, level 0
};

static_assert (WavetableBank::kTableSize == 2048 && CycleView::kPoints * 8 == WavetableBank::kTableSize,
               "the cycle panel point-samples every 8th table sample");
static_assert (WavetableBank::kmax (0) == 1023 && WavetableBank::kmax (10) == 1,
               "kmax per mip level");

//==============================================================================
// Message thread only, non-reentrant (one instance per processor; documented,
// not asserted - the console harness calls it from main). render() allocates
// nothing and takes no lock: the FFT and every scratch buffer are sized at
// construction.
class CycleRenderer
{
public:
    // D-U: the harmonic dB reference is the max |X_k| over bins 1..kRefMaxBin
    // of the heard cycle (DC excluded). 1023 = every harmonic a level-0 table
    // can hold, so a frame whose strongest partial lies above h32 shows
    // honestly lower bars. Set to 32 to normalise to the visible bars instead.
    static constexpr int   kRefMaxBin = 1023;
    static constexpr float kFloorDb   = -60.0f;

    CycleRenderer() = default;

    // b == nullptr (or an empty bank): zeros, -60 dB everywhere, empty = true.
    // Fills cycle, preQ, harmonicsDb, harmonicsRawDb, quantized and empty;
    // the caller fills the scalar fields.
    void render (const WavetableBank* b, int level, bool interp, float pos, int latched,
                 int bitIdx, CycleView& v) noexcept;

private:
    static constexpr int kOrder = 11;
    static constexpr int kSize  = 1 << kOrder;
    static_assert (kSize == WavetableBank::kTableSize, "one FFT bin per harmonic");
    static_assert (kRefMaxBin >= CycleView::kHarmonics && kRefMaxBin < kSize / 2, "reference bins");

    using Cycle = std::array<float, (size_t) kSize>;

    // Exactly the voice's per-sample read at phase j / 2048 (WtVoice.h:398:
    // quant.apply (wt::readSample (...))). unquantized may be nullptr.
    static void renderCycle (const WavetableBank& b, int level, bool interp, float pos, int latched,
                             const BitQuantizer& q, Cycle& out, Cycle* unquantized) noexcept;

    void  transform (const Cycle& x) noexcept;                 // |X_k| into work[0..1024]
    float referenceMagnitude() const noexcept;                 // max work[1..kRefMaxBin]
    void  toDb (float ref, std::array<float, CycleView::kHarmonics>& out) const noexcept;

    juce::dsp::FFT fft { kOrder };
    std::array<float, (size_t) (2 * kSize)> work {};           // performFrequencyOnly needs 2 * size
    Cycle heard {}, pre {}, raw {};

    JUCE_DECLARE_NON_COPYABLE (CycleRenderer)
};
