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

    WavetableImporter.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

    FFT-based wavetable import from audio files (Serum-style FFT 2048).

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include "WavetableGenerator.h"
#include <JuceHeader.h>
#include <memory>

class WavetableImporter
{
public:
    struct ImportResult
    {
        std::unique_ptr<WavetableData> table;
        juce::String error;
        bool success = false;
    };

    /** Import from audio file using FFT analysis (Serum-style FFT 2048).
        Supports WAV, AIFF, FLAC via JUCE AudioFormatManager. */
    static ImportResult importFromFile (const juce::File& file);

    /** Import from memory block (for drag-and-drop via WebView). */
    static ImportResult importFromMemory (const void* data, size_t sizeInBytes);

private:
    static ImportResult processAudioBuffer (juce::AudioBuffer<float>& buffer);
};
