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

    Stage 3 (ARCHITECTURE Core 10; plan Decisions 11, 12, 15, 31, 32): the
    message-thread side of the view pushes. Reads the processor's CycleCapture
    rings, readout atomics and published terrain objects; extracts the last
    COMPLETE orbit cycle (`terrainCycle`), the ring's newest slot (`terrainState`,
    the playhead), the active surface sampled on the view grid
    (`terrainHeightmap`) and the readout status (`terrainStatus`), boxed as the
    events the editor pushes through emitEventIfBrowserIsVisible.

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

    // ─── Round B (plan Decision 32): the playhead ───

    /** The ring's newest slot: θ (radians), the DISPLACED point {x, y} (after feedback
        and the ±1 clamp) and its terrain value saturated by the oscillator's law
        g = 1 + 4 · osc?TerSat — the same code path copyCycle's third column takes. */
    struct Playhead { float theta = 0.0f, x = 0.0f, y = 0.0f, h = 0.0f; };

    /** Message thread. Reads writeIndex (acquire); returns false when it has not advanced
        since `lastWriteIndex` (idle — the caller pushes nothing and the page keeps its
        last values) or the slot was overwritten under the read. Else fills `out` from
        slot (w − 1), every field rounded to 1e-4, and sets lastWriteIndex = w. */
    bool getPlayhead (OStrataAudioProcessor& processor, int osc, uint32_t& lastWriteIndex, Playhead& out);

    /** {"osc":"A","theta":1.2345,"x":0.1,"y":0.2,"h":0.3} */
    juce::var makeStateEvent (int osc, const Playhead& ph);

    // ─── Round B (plan Decision 31): the heightmap ───

    /** Grid = the page's corner nodes x_i = −1 + 2 i / 63, row j = 0 ⇒ y = −1, row-major
        out[j · 64 + i] (matches the page's sampleH and the R32F upload). */
    constexpr int kHeightmapN = 64;
    constexpr int kHeightmapSize = kHeightmapN * kHeightmapN;

    /** FNV-1a over {quality, terrain, F, mx, my, edge} quantised to 1/1024 (the scheduler's
        rule) + chebGeneration, imageGeneration, importRevision and stateGeneration — the
        editor's change gate for the heightmap push. */
    uint64_t heightmapKey (OStrataAudioProcessor& processor, int osc);

    /** The ACTIVE surface sampled on the grid, with the oscillator's own source rule:
        Bandlimited → clenshaw2D (untapered) over the published set when its key names
        the terrain (chebPtr for analytic terrains, the image's embedded set for
        Imported…), else the 1× fallback the voice takes; 2× / 4× analytic → terrain()
        at raw F / Mod X / Mod Y (at C4 the track ratio is 1); Imported… → the published
        image's sample (x, y, F, edge), zeros while no image is published (the voice is
        silent then). Base values are the unmodulated raw parameter reads; non-finite → 0. */
    void copyHeightmap (OStrataAudioProcessor& processor, int osc, float* out4096);

    /** {"osc":"A","n":64,"data":"<base64 LE Float32 × 4096>"} */
    juce::var makeHeightmapEvent (int osc, const float* out4096);
}
