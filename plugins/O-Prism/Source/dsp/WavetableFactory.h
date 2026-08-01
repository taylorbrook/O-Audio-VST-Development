/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    WavetableFactory.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    Factory library: 28 procedural wavetables across 5 categories.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "WavetableData.h"
#include "WavetableGenerator.h"
#include <memory>
#include <vector>

struct TableInfo
{
    juce::String name;
    juce::String category;
    int numFrames;
};

struct FactoryEntry
{
    std::unique_ptr<WavetableData> table;
    TableInfo info;
};

class WavetableFactory
{
public:
    /** Create the complete factory library (28 tables). */
    static std::vector<FactoryEntry> createFactoryLibrary();

    /** Table count (compile-time constant). */
    static constexpr int kNumFactoryTables = 28;

    /** Get metadata for all tables (no audio data). */
    static std::vector<TableInfo> getTableInfoList();

    // ─── Helpers (public for use by formant generation helpers) ───
    static void normalizeFrame (float* buffer, int size);
    static void addHarmonic (float* buffer, int size, int harmonic, double amplitude);
    static void addPartial (float* buffer, int size, double freqRatio, double amplitude);

private:
    // ─── Analog ───
    static std::unique_ptr<WavetableData> generatePWMSweep (int numFrames);
    static std::unique_ptr<WavetableData> generateSupersaw (int numFrames);
    static std::unique_ptr<WavetableData> generateSyncSweep (int numFrames);

    // ─── Digital ───
    static std::unique_ptr<WavetableData> generateFM (int numFrames, double cmRatio,
                                                       double minIndex, double maxIndex);
    static std::unique_ptr<WavetableData> generateWavefold (int numFrames);
    static std::unique_ptr<WavetableData> generateBitcrush (int numFrames);

    // ─── Formant ───
    static std::unique_ptr<WavetableData> generateVowelMorph (int numFrames);
    static std::unique_ptr<WavetableData> generateChoirPad (int numFrames);
    static std::unique_ptr<WavetableData> generateVocalLead (int numFrames);
    static std::unique_ptr<WavetableData> generateFormantFilter (int numFrames);

    // ─── Spectral ───
    static std::unique_ptr<WavetableData> generateHarmonicSeries (int numFrames);
    static std::unique_ptr<WavetableData> generateSpectralTilt (int numFrames);
    static std::unique_ptr<WavetableData> generateOddHarmonics (int numFrames);
    static std::unique_ptr<WavetableData> generateHarmonicStretch (int numFrames);
    static std::unique_ptr<WavetableData> generateCombSweep (int numFrames);
    static std::unique_ptr<WavetableData> generatePrismSpectrum (int numFrames);

    // ─── Organic ───
    static std::unique_ptr<WavetableData> generateBreath (int numFrames);
    static std::unique_ptr<WavetableData> generatePluckedString (int numFrames);
    static std::unique_ptr<WavetableData> generateChurchBell (int numFrames);
    static std::unique_ptr<WavetableData> generateOrganSweep (int numFrames);
    static std::unique_ptr<WavetableData> generateWind (int numFrames);
    static std::unique_ptr<WavetableData> generateFilteredNoise (int numFrames);
};
