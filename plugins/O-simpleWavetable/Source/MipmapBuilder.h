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

    O-simpleWavetable - MipmapBuilder (Stage 2.1)
    Ouaricon Audio
    Developer: Taylor Brook

    Spectrum (or one time-domain cycle) -> 11 strict mip levels.

      - DC bins (d[0], d[1]) and the Nyquist bin (d[2048], d[2049]) are
        zeroed. O-Prism kept Nyquist at level 0; that is the defect we fix.
      - Optional peak normalisation scales the SPECTRUM once, from level 0.
        Levels are never re-normalised (Gibbs overshoot above 1.0 at higher
        levels is expected and harmless).
      - Level L keeps bins 1..kmax(L) and zeroes kmax(L)+1..1024.
      - No level blend anywhere (Prism's trilinear blend fails QUAL-02).

    REENTRANT: every instance owns its own juce::dsp::FFT(11) and scratch,
    no statics. The 2.4 import worker and setStateInformation may build
    concurrently, each with its own builder.

    Packed spectrum layout = JUCE real-only FFT: 2 * 2048 floats, bin k at
    {d[2k] = re, d[2k+1] = im}, bins 0..1024 meaningful.
      sine amplitude b_n   -> d[2n+1] = -0.5 * 2048 * b_n
      cosine amplitude a_n -> d[2n]   =  0.5 * 2048 * a_n

    Message / worker thread ONLY (constructing the FFT allocates).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include <memory>
#include <vector>

#include "WavetableBank.h"

class MipmapBuilder
{
public:
    static constexpr int kOrder    = 11;
    static constexpr int kN        = WavetableBank::kTableSize;   // 2048
    static constexpr int kNyquist  = kN / 2;                      // bin 1024
    static constexpr int kPacked   = 2 * kN;                      // 4096 floats

    enum class Normalise { peakToUnity, none };

    MipmapBuilder();

    // spectrum: packed JUCE layout, kPacked floats (bins 0..1024 meaningful).
    // The bank must already be allocate()d with frame < numFrames.
    void buildFromSpectrum (WavetableBank& bank, int frameIndex, const float* spectrum, Normalise norm);

    // x: one 2048-sample cycle. Forward FFT, then buildFromSpectrum.
    void buildFromTime (WavetableBank& bank, int frameIndex, const float* x2048, Normalise norm);

   #if OSIW_TEST_HOOKS
    // Negative-control hook for bank-check G-NEG: keep the Nyquist bin, which
    // the G-BL gate must then catch.
    bool testKeepNyquist = false;
   #endif

private:
    void inverseLevel (int level);

    juce::dsp::FFT fft;
    std::vector<float> spec, work;

    JUCE_DECLARE_NON_COPYABLE (MipmapBuilder)
};

// 2.4 seam (import / restore): numFrames consecutive 2048-sample cycles ->
// a new immutable bank. No re-normalisation (Normalise::none). Returns
// nullptr for a null source or numFrames outside 1..256.
std::unique_ptr<WavetableBank> buildFromLevel0 (const float* level0, int numFrames, const juce::String& name);
