/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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

    InstrumentPresets.h
    O-Wind - Instrument Preset Parameter Sets
    Ouaricon Audio
    Developer: Taylor Brook

    Defines internal DSP coefficients for each instrument type.
    Presets tune the physics model; user APVTS params sit on top.
    These are NOT exposed as APVTS parameters -- they configure the
    internal jet gain, radiation/end-reflection filters, jet geometry, etc.

  ==============================================================================
*/

#pragma once
#include <array>

struct InstrumentPreset
{
    const char* name;
    float jetGain;            // tanh saturation gain (1.0-3.0)
    float radiationCutoff;    // radiation highpass cutoff (Hz)
    float endReflCutoff;      // end reflection lowpass cutoff (Hz)
    float jetAmplification;   // mu factor: jet spatial amplification (Verge 1995)
    float overblowEase;       // how easily instrument overblows (0=hard, 1=easy)
    float jetDiameter;        // jet opening diameter in meters (for Strouhal noise)
    float formantCenterHz;    // headjoint formant resonance center frequency (Hz)
    float inharmonicityBase;  // allpass inharmonicity base (APVTS param multiplies this)
};

namespace InstrumentPresets
{
    // Core presets (ship at launch)
    // jetAmplification (mu) scaled to keep small-signal loop gain in 3-7x range.
    // Previous values (12-35) caused ~26x loop gain → deep tanh saturation,
    // octave mode-locking, and hard-clip distortion.
    //                                    jetGain radCut endRefl  mu   overblow jetDiam formant inharm
    static constexpr InstrumentPreset concertFlute {
        "Concert Flute",
        2.8f, 150.0f, 3000.0f, 5.0f, 0.6f, 0.010f, 2500.0f, 0.15f
    };

    static constexpr InstrumentPreset shakuhachi {
        "Shakuhachi",
        2.1f, 100.0f, 2000.0f, 3.5f, 0.3f, 0.015f, 1800.0f, 0.5f
    };

    static constexpr InstrumentPreset bansuri {
        "Bansuri",
        2.5f, 120.0f, 2500.0f, 4.5f, 0.5f, 0.012f, 2000.0f, 0.35f
    };

    static constexpr InstrumentPreset nativeAmericanFlute {
        "Native Am. Flute",
        1.8f, 90.0f, 1500.0f, 3.0f, 0.4f, 0.014f, 1500.0f, 0.4f
    };

    // Expansion presets (Phase 3.4)
    static constexpr InstrumentPreset recorder {
        "Recorder",
        3.2f, 200.0f, 4000.0f, 7.0f, 1.0f, 0.008f, 3000.0f, 0.3f
    };

    static constexpr InstrumentPreset panFlute {
        "Pan Flute",
        1.5f, 175.0f, 2500.0f, 4.0f, 0.4f, 0.013f, 2200.0f, 0.35f
    };

    static constexpr InstrumentPreset piccolo {
        "Piccolo",
        3.0f, 250.0f, 5000.0f, 6.0f, 0.7f, 0.007f, 4000.0f, 0.2f
    };

    static constexpr InstrumentPreset ocarina {
        "Ocarina",
        2.2f, 120.0f, 2000.0f, 4.0f, 0.2f, 0.010f, 2000.0f, 0.4f
    };

    // All presets in order
    static constexpr std::array<InstrumentPreset, 8> allPresets {{
        concertFlute,
        shakuhachi,
        bansuri,
        nativeAmericanFlute,
        recorder,
        panFlute,
        piccolo,
        ocarina
    }};

    static constexpr int numCorePresets = 4;
    static constexpr int numTotalPresets = 8;

    inline const InstrumentPreset& getPreset (int index)
    {
        index = (index < 0 || index >= numTotalPresets) ? 0 : index;
        return allPresets[static_cast<size_t> (index)];
    }
}
