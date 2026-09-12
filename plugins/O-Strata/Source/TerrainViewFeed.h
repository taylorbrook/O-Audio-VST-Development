/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    TerrainViewFeed.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Stage 3 (ARCHITECTURE Core 10; plan Decisions 11, 12, 15): the message-thread
    side of the view pushes. Reads the processor's CycleCapture rings and readout
    atomics, extracts the last COMPLETE orbit cycle, and boxes the `terrainCycle`
    and `terrainStatus` events the editor pushes through
    emitEventIfBrowserIsVisible. Round B adds the playhead (`terrainState`) and
    the heightmap (`terrainHeightmap`) here.

    Depends on the processor only — never on the editor — so the offline render
    harness (JUCE_WEB_BROWSER=0, editor TU excluded) still compiles this file.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <cstdint>

namespace TerrainViewFeed
{
    /** Points per pushed cycle (the page draws y[i] against i, i < 512). */
    constexpr int kCyclePoints = 512;

    /** Per-oscillator scratch the editor owns: a full ring copy (128 KB) plus the
        writeIndex the last successful extraction saw (a non-advancing index = idle). */
    struct CycleScratch
    {
        std::array<float, CycleCapture::kPoints * 4> ring {};
        uint32_t lastWriteIndex = 0;
    };

    /** Message thread. Copies the ring twice-fenced (RESEARCH §2.3), walks back over
        the θ wraps to the last complete cycle (FM-warp fallback: the newest
        fs · OS / f_display slots), resamples it to 512 points by linear interpolation
        on slot index and writes {px, py, y} triples to `out` (512 × 3 floats), with
        y saturated by the oscillator's own law g = 1 + 4 · osc?TerSat (raw value).
        Returns kCyclePoints, or 0 when the ring is idle, torn, or holds no complete
        cycle yet (the caller keeps the last drawn cycle). */
    int copyCycle (OStrataAudioProcessor& processor, int osc, CycleScratch& scratch, float* out);

    /** Little-endian Float32 raw bytes → base64 (juce::Base64, no line breaks);
        non-finite values are scrubbed to 0 first. */
    juce::String encodeFloat32Base64 (const float* data, size_t count);

    /** {"osc":"A","n":512,"data":"<base64>"} */
    juce::var makeCycleEvent (int osc, const float* out512x3);

    /** {"osc":"A","quality":1,"terrain":0,"partials":16,"fit":100,"approx":false,"topNote":-1,"sourceMissing":false} */
    juce::var makeStatusEvent (int osc, const OStrataAudioProcessor::TerrainStatus& status);
}
