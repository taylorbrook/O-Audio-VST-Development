/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    ArpabetFormants.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    v1.33.0: direct formant targets for lyric-mode phonemes.

    Lyric vowels used to go through the XY inverse-distance morpher, whose
    only anchors are A E I O U R L. Every vowel between anchors came out as a
    blend — IH landed 0.08 from the R anchor (F2 1093 Hz, F3 1682 Hz: heard as
    "er"), AE sat at F2 1110 Hz (heard as "ah"). Lyric notes now read their
    formants straight from this table; the XY pad and manual mode are unchanged.

    F1-F3: Hillenbrand et al. 1995, adult male means. Diphthong nuclei and
    offglides after Peterson & Barney 1952 / Holbrook & Fairbanks 1962.
    F4/F5: Klatt 1980 neutral-tract defaults. Glide onsets (W Y L R): Stevens
    1998 approximant targets; L and R match the VowelData.h anchors.

    Index order is shared with the page (main.js ARPABET_FORMANT_ID).

  ==============================================================================
*/

#pragma once
#include <cmath>

namespace ArpabetFormants
{
    struct Entry
    {
        float freq[5];       // F1-F5 Hz
        float bandwidth[5];  // B1-B5 Hz
        float gainDb[5];     // parallel/hybrid branch levels (cascade ignores)
    };

    enum Id
    {
        IY, IH, EY, EH, AE, AA, AH, AO, OW, UH, UW, ER,
        AW_NUC, AY_NUC, OY_NUC,
        GLIDE_W, GLIDE_Y, GLIDE_L, GLIDE_R,
        kNumEntries
    };

    static constexpr Entry entries[kNumEntries] =
    {
        // IY /i/
        { { 342.f, 2322.f, 3000.f, 3500.f, 4000.f }, { 60.f, 100.f, 120.f, 180.f, 220.f }, { 0.f, -24.f, -16.f, -22.f, -28.f } },
        // IH /ɪ/
        { { 427.f, 2034.f, 2684.f, 3300.f, 3850.f }, { 60.f,  90.f, 120.f, 180.f, 220.f }, { 0.f, -16.f, -18.f, -24.f, -30.f } },
        // EY /e/ (nucleus)
        { { 476.f, 2089.f, 2691.f, 3300.f, 3850.f }, { 60.f,  90.f, 120.f, 180.f, 220.f }, { 0.f, -14.f, -14.f, -20.f, -26.f } },
        // EH /ɛ/
        { { 580.f, 1799.f, 2605.f, 3300.f, 3850.f }, { 70.f,  90.f, 120.f, 180.f, 220.f }, { 0.f, -10.f, -14.f, -20.f, -26.f } },
        // AE /æ/
        { { 588.f, 1952.f, 2601.f, 3300.f, 3850.f }, { 80.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -8.f, -14.f, -20.f, -26.f } },
        // AA /ɑ/
        { { 768.f, 1333.f, 2522.f, 3300.f, 3850.f }, { 80.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -7.f, -12.f, -18.f, -26.f } },
        // AH /ʌ/
        { { 623.f, 1200.f, 2550.f, 3300.f, 3850.f }, { 70.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -7.f, -16.f, -22.f, -30.f } },
        // AO /ɔ/
        { { 652.f,  997.f, 2538.f, 3300.f, 3850.f }, { 80.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -6.f, -20.f, -24.f, -34.f } },
        // OW /o/ (nucleus)
        { { 497.f,  910.f, 2459.f, 3300.f, 3850.f }, { 60.f,  90.f, 120.f, 180.f, 220.f }, { 0.f, -10.f, -22.f, -26.f, -38.f } },
        // UH /ʊ/
        { { 469.f, 1122.f, 2434.f, 3300.f, 3850.f }, { 60.f,  90.f, 120.f, 180.f, 220.f }, { 0.f, -10.f, -24.f, -28.f, -36.f } },
        // UW /u/
        { { 378.f,  997.f, 2343.f, 3300.f, 3850.f }, { 60.f,  90.f, 120.f, 180.f, 220.f }, { 0.f, -16.f, -30.f, -30.f, -38.f } },
        // ER /ɝ/ (F3 ~1700 Hz = r-colouring)
        { { 474.f, 1379.f, 1710.f, 3300.f, 3850.f }, { 60.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -8.f, -12.f, -24.f, -30.f } },
        // AW nucleus /a/
        { { 760.f, 1400.f, 2500.f, 3300.f, 3850.f }, { 80.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -7.f, -12.f, -18.f, -26.f } },
        // AY nucleus /a/
        { { 750.f, 1300.f, 2500.f, 3300.f, 3850.f }, { 80.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -7.f, -12.f, -18.f, -26.f } },
        // OY nucleus /ɔ/
        { { 600.f,  900.f, 2500.f, 3300.f, 3850.f }, { 80.f,  90.f, 120.f, 180.f, 220.f }, { 0.f,  -6.f, -20.f, -24.f, -34.f } },
        // W onset (rounded, u-like, very low F2)
        { { 300.f,  650.f, 2200.f, 3300.f, 3850.f }, { 60.f,  90.f, 150.f, 200.f, 250.f }, { 0.f, -18.f, -32.f, -32.f, -40.f } },
        // Y onset (i-like, high F2)
        { { 280.f, 2250.f, 3000.f, 3500.f, 4000.f }, { 60.f, 100.f, 150.f, 200.f, 250.f }, { 0.f, -26.f, -16.f, -22.f, -28.f } },
        // L onset (VowelData.h anchor)
        { { 400.f,  900.f, 2600.f, 3400.f, 4200.f }, { 80.f, 120.f, 150.f, 250.f, 280.f }, { 0.f,  -6.f, -16.f, -22.f, -28.f } },
        // R onset (VowelData.h anchor — low F3)
        { { 340.f, 1050.f, 1600.f, 3500.f, 4300.f }, { 60.f,  90.f, 130.f, 250.f, 280.f }, { 0.f,  -8.f, -14.f, -24.f, -30.f } },
    };

    inline bool isValid (int id) noexcept { return id >= 0 && id < kNumEntries; }

    // Blend a (weight 1-t) toward b (weight t): frequencies in the log domain,
    // bandwidths and gains linear — same rule as VowelMorpher.
    inline void blend (int a, int b, float t,
                       float outFreq[5], float outBW[5], float outGain[5]) noexcept
    {
        const Entry& ea = entries[a];
        const Entry& eb = entries[b];
        for (int f = 0; f < 5; ++f)
        {
            outFreq[f] = std::exp ((1.0f - t) * std::log (ea.freq[f]) + t * std::log (eb.freq[f]));
            outBW[f]   = (1.0f - t) * ea.bandwidth[f] + t * eb.bandwidth[f];
            const float gDb = (1.0f - t) * ea.gainDb[f] + t * eb.gainDb[f];
            outGain[f] = std::pow (10.0f, gDb / 20.0f);
        }
    }
} // namespace ArpabetFormants
