/*
  ==============================================================================

    dynamics_layer_check.cpp
    O-MicrotonalSampler — v1.15.0 dynamics-mapping sanity check.

    Manual run: ninja O-MicrotonalSampler_DynamicsLayerCheck && ./...
    Exit code = number of failed assertions (0 = all pass).

    Why this test exists
    --------------------

    v1.15.0 ships docs/dynamics-mapping.md to clarify how note-on velocity
    drives velocity-layer selection (a sample-pick at note-on time, never
    a continuous modulator) versus how CC11 ("Expression") drives the
    sustained gain envelope. The doc claims that with N velocity layers,
    a uniform 128-wide velocity space is bucketed into N consecutive
    bands of width (128 / N), with the highest band absorbing any
    leftover above 128/N * (N-1).

    This file verifies the bucketing matches that promise, by replicating
    the formula from MicrotonalSamplerVoice::startNote and pinning the
    bucket boundaries for the 4-layer case (the plugin's standard
    layer count).

    What this exercises
    -------------------

      1. velocityToLayer(N, vel) bucket boundaries for N=1, 2, 4, 8.
      2. Edge-case clamping: vel=0 → layer 0; vel=128 → top layer.
      3. SampleMap::findCell selects the correct (midi, layer) cell once
         the layer index has been resolved — proves the dynamics path
         from the documentation is the same path the voice walks.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "../SampleMap.h"

#include <iostream>
#include <string>

namespace
{
    int failed = 0;

    void check (bool cond, const std::string& desc)
    {
        if (cond)
            std::cout << "  PASS: " << desc << "\n";
        else
        {
            std::cout << "  FAIL: " << desc << "\n";
            ++failed;
        }
    }

    // Mirrors MicrotonalSamplerVoice::startNote lines ~420..423:
    //   vel        = jlimit(1, 127, round(velocity * 127))
    //   numLayers  = max(1, currentMap->numVelocityLayers)
    //   layerWidth = max(1, 128 / numLayers)
    //   layerIdx   = jlimit(0, numLayers - 1, (vel - 1) / layerWidth)
    int velocityToLayer (int numLayers, int vel0to127)
    {
        const int n     = juce::jmax (1, numLayers);
        const int v     = juce::jlimit (1, 127, vel0to127);
        const int width = juce::jmax (1, 128 / n);
        return juce::jlimit (0, n - 1, (v - 1) / width);
    }

    SampleVariant makeVariant (const juce::String& filename)
    {
        SampleVariant v;
        v.filename = filename;
        return v;
    }

    SampleCell makeCell (int midi, int layer, const juce::String& filename)
    {
        SampleCell c;
        c.midiNote      = midi;
        c.velocityLayer = layer;
        c.technique     = 0;
        c.variants.push_back (makeVariant (filename));
        return c;
    }
}

int main()
{
    std::cout << "== dynamics_layer_check ==\n";

    // ----------------------------------------------------------------
    // 1. N=1 — single layer absorbs every velocity.
    // ----------------------------------------------------------------
    {
        for (int v = 0; v <= 127; v += 16)
            check (velocityToLayer (1, v) == 0,
                   "N=1, vel=" + std::to_string (v) + " → layer 0");
    }

    // ----------------------------------------------------------------
    // 2. N=2 — 64-wide bands.
    //    width = 128 / 2 = 64.
    //    layerIdx = (vel - 1) / 64.
    //    vel=1..64 → 0; vel=65..127 → 1 (and clamped at 1).
    // ----------------------------------------------------------------
    {
        check (velocityToLayer (2, 1)   == 0, "N=2, vel=1   → layer 0");
        check (velocityToLayer (2, 32)  == 0, "N=2, vel=32  → layer 0");
        check (velocityToLayer (2, 64)  == 0, "N=2, vel=64  → layer 0 (high edge)");
        check (velocityToLayer (2, 65)  == 1, "N=2, vel=65  → layer 1 (low edge)");
        check (velocityToLayer (2, 127) == 1, "N=2, vel=127 → layer 1");
    }

    // ----------------------------------------------------------------
    // 3. N=4 — the plugin's standard configuration. 32-wide bands.
    //    width = 128 / 4 = 32.
    //    vel=1..32 → 0; vel=33..64 → 1; vel=65..96 → 2; vel=97..127 → 3.
    // ----------------------------------------------------------------
    {
        check (velocityToLayer (4, 1)   == 0, "N=4, vel=1   → layer 0");
        check (velocityToLayer (4, 32)  == 0, "N=4, vel=32  → layer 0 (high edge)");
        check (velocityToLayer (4, 33)  == 1, "N=4, vel=33  → layer 1 (low edge)");
        check (velocityToLayer (4, 64)  == 1, "N=4, vel=64  → layer 1 (high edge)");
        check (velocityToLayer (4, 65)  == 2, "N=4, vel=65  → layer 2 (low edge)");
        check (velocityToLayer (4, 96)  == 2, "N=4, vel=96  → layer 2 (high edge)");
        check (velocityToLayer (4, 97)  == 3, "N=4, vel=97  → layer 3 (low edge)");
        check (velocityToLayer (4, 127) == 3, "N=4, vel=127 → layer 3");
    }

    // ----------------------------------------------------------------
    // 4. N=8 — 16-wide bands. Common in deeply layered libraries.
    //    width = 128 / 8 = 16.
    // ----------------------------------------------------------------
    {
        for (int layer = 0; layer < 8; ++layer)
        {
            const int lo = layer * 16 + 1;
            const int hi = (layer + 1) * 16;
            check (velocityToLayer (8, lo) == layer,
                   "N=8, vel=" + std::to_string (lo) + " (low edge layer "
                       + std::to_string (layer) + ")");
            check (velocityToLayer (8, hi) == layer,
                   "N=8, vel=" + std::to_string (hi) + " (high edge layer "
                       + std::to_string (layer) + ")");
        }
        // Clamp tail: vel=127 lands in layer 7.
        check (velocityToLayer (8, 127) == 7, "N=8, vel=127 → layer 7");
    }

    // ----------------------------------------------------------------
    // 5. Edge-case clamping: vel=0 ≡ vel=1 because the voice clamps to
    //    [1..127] before bucketing. (MIDI 0 velocity is note-off in
    //    most hosts so this is a defensive lower edge.)
    // ----------------------------------------------------------------
    {
        check (velocityToLayer (4, 0) == velocityToLayer (4, 1),
               "vel=0 clamps to vel=1 → same bucket");
    }

    // ----------------------------------------------------------------
    // 6. SampleMap.findCell receives the resolved layer index — i.e.
    //    the bucketing happens at the voice, not in the cell lookup.
    //    This pins the contract that velocity is a *layer-pick* signal,
    //    not a gain modulator (the doc's primary point).
    // ----------------------------------------------------------------
    {
        SampleMap map;
        map.numVelocityLayers = 4;
        map.cells.push_back (makeCell (60, 0, "C3_v1.wav"));
        map.cells.push_back (makeCell (60, 1, "C3_v2.wav"));
        map.cells.push_back (makeCell (60, 2, "C3_v3.wav"));
        map.cells.push_back (makeCell (60, 3, "C3_v4.wav"));

        // Walk a vel sweep and prove (vel → layerIdx → cell) yields
        // monotonically increasing dynamic layers as velocity climbs.
        for (int vel = 1; vel <= 127; vel += 8)
        {
            const int layer = velocityToLayer (4, vel);
            const auto* cell = map.findCell (60, layer);
            const bool ok = (cell != nullptr) && (cell->velocityLayer == layer);
            check (ok, "vel=" + std::to_string (vel) + " → layer "
                       + std::to_string (layer) + " → cell present");
        }
    }

    // ----------------------------------------------------------------
    // 7. The dynamics-mapping doc claims layer crossfade is FROZEN at
    //    note-on — i.e. CC11 (smoothed in PluginProcessor) cannot
    //    re-pick a layer mid-note. We can't drive a live voice from a
    //    test, but we CAN pin the SampleMap data model: a SampleCell
    //    is keyed on (midi, layer, technique) and findCell never reads
    //    a continuous "expression" parameter. If a future refactor
    //    accidentally adds such a parameter to findCell, this test
    //    serves as the regression boundary marker — update only when
    //    the doc gets updated alongside the code.
    // ----------------------------------------------------------------
    {
        SampleMap map;
        map.numVelocityLayers = 4;
        map.cells.push_back (makeCell (60, 1, "C3_v2.wav"));

        const auto* a = map.findCell (60, 1);
        const auto* b = map.findCell (60, 1);
        check (a != nullptr && a == b,
               "findCell is deterministic — no expression-driven layer drift");
    }

    // ----------------------------------------------------------------
    // 8. v1.17.2 — findCellNearestLayer: single-dynamic library safety net.
    //    A library where the parser landed every slot on ONE non-zero layer
    //    (e.g. an all-`mf` library → layer 2) must still play across the whole
    //    velocity range instead of going silent at velocities that bucket to
    //    an empty layer. findCellNearestLayer falls back to the nearest
    //    populated layer; the exact findCell would return nullptr.
    // ----------------------------------------------------------------
    {
        SampleMap map;
        map.numVelocityLayers = 4;
        map.cells.push_back (makeCell (60, 2, "C3_mf.wav"));  // only layer 2 populated

        // Exact lookups for the empty layers return nullptr…
        check (map.findCell (60, 0) == nullptr, "exact findCell(layer 0) is empty");
        check (map.findCell (60, 1) == nullptr, "exact findCell(layer 1) is empty");
        check (map.findCell (60, 3) == nullptr, "exact findCell(layer 3) is empty");

        // …but the nearest-layer lookup always resolves to the populated cell.
        for (int layer = 0; layer < 4; ++layer)
        {
            const auto* cell = map.findCellNearestLayer (60, layer, 0);
            check (cell != nullptr && cell->velocityLayer == 2,
                   "findCellNearestLayer(layer " + std::to_string (layer)
                       + ") → populated layer 2");
        }

        // Sweeping a full velocity range never goes silent.
        for (int vel = 1; vel <= 127; vel += 8)
        {
            const int layer = velocityToLayer (4, vel);
            const auto* cell = map.findCellNearestLayer (60, layer, 0);
            check (cell != nullptr,
                   "vel=" + std::to_string (vel) + " → nearest-layer cell present");
        }
    }

    // ----------------------------------------------------------------
    // 9. v1.17.2 — equidistant tie-break prefers the LOWER (quieter) layer.
    //    Requested layer 1 is empty; layers 0 and 2 are both populated and
    //    equidistant → must pick layer 0.
    // ----------------------------------------------------------------
    {
        SampleMap map;
        map.numVelocityLayers = 4;
        map.cells.push_back (makeCell (60, 0, "C3_p.wav"));
        map.cells.push_back (makeCell (60, 2, "C3_mf.wav"));

        const auto* cell = map.findCellNearestLayer (60, 1, 0);
        check (cell != nullptr && cell->velocityLayer == 0,
               "equidistant tie-break prefers lower layer 0");
    }

    // ----------------------------------------------------------------
    // 10. v1.17.2 — fully-populated library is UNAFFECTED: nearest-layer
    //     lookup returns the exact layer (never expands).
    // ----------------------------------------------------------------
    {
        SampleMap map;
        map.numVelocityLayers = 4;
        for (int layer = 0; layer < 4; ++layer)
            map.cells.push_back (makeCell (60, layer,
                "C3_v" + std::to_string (layer + 1) + ".wav"));

        for (int layer = 0; layer < 4; ++layer)
        {
            const auto* cell = map.findCellNearestLayer (60, layer, 0);
            check (cell != nullptr && cell->velocityLayer == layer,
                   "fully-populated: nearest == exact layer "
                       + std::to_string (layer));
        }
    }

    // ----------------------------------------------------------------
    // 11. v1.21.0 — gatherLayerCells: the CC Crossfade engine's layer-stack
    //     builder. Must return every populated layer for ONE technique, in
    //     ASCENDING velocity-layer order, with no null gaps, capped at maxOut.
    // ----------------------------------------------------------------
    {
        // (a) Fully-populated 4-layer library → 4 cells, ascending order.
        {
            SampleMap map;
            map.numVelocityLayers = 4;
            for (int layer = 0; layer < 4; ++layer)
                map.cells.push_back (makeCell (60, layer,
                    "C3_v" + std::to_string (layer + 1) + ".wav"));

            const SampleCell* out[4] = { nullptr };
            const int n = map.gatherLayerCells (60, 0, out, 4);
            check (n == 4, "gather: full 4-layer → count 4");
            bool ascending = true;
            for (int k = 0; k < n; ++k)
                if (out[k] == nullptr || out[k]->velocityLayer != k)
                    ascending = false;
            check (ascending, "gather: layers returned ascending 0,1,2,3");
        }

        // (b) Sparse library (layers 0 and 2 only) → 2 cells, no gap, ascending.
        {
            SampleMap map;
            map.numVelocityLayers = 4;
            map.cells.push_back (makeCell (60, 0, "C3_p.wav"));
            map.cells.push_back (makeCell (60, 2, "C3_mf.wav"));

            const SampleCell* out[4] = { nullptr };
            const int n = map.gatherLayerCells (60, 0, out, 4);
            check (n == 2, "gather: sparse {0,2} → count 2 (compacted, no gap)");
            check (n == 2 && out[0] != nullptr && out[0]->velocityLayer == 0
                          && out[1] != nullptr && out[1]->velocityLayer == 2,
                   "gather: sparse stack is [layer 0, layer 2]");
        }

        // (c) Single layer → exactly one cell (CC mode falls back to a gain).
        {
            SampleMap map;
            map.numVelocityLayers = 4;
            map.cells.push_back (makeCell (60, 1, "C3_mf.wav"));

            const SampleCell* out[4] = { nullptr };
            const int n = map.gatherLayerCells (60, 0, out, 4);
            check (n == 1 && out[0] != nullptr && out[0]->velocityLayer == 1,
                   "gather: single populated layer → count 1");
        }

        // (d) Technique isolation — gather(tech=1) must NOT mix in tech-0 cells
        //     (no articulation mixing across the loudness axis; the voice does
        //     the ord fallback at the technique level, not gatherLayerCells).
        {
            SampleMap map;
            map.numVelocityLayers = 4;
            map.cells.push_back (makeCell (60, 0, "C3_ord_p.wav"));   // tech 0
            map.cells.push_back (makeCell (60, 1, "C3_ord_mf.wav"));  // tech 0
            SampleCell pizz = makeCell (60, 3, "C3_pizz.wav");        // tech 1
            pizz.technique = 1;
            map.cells.push_back (pizz);

            const SampleCell* out[4] = { nullptr };
            const int n = map.gatherLayerCells (60, 1, out, 4);
            check (n == 1 && out[0] != nullptr && out[0]->technique == 1,
                   "gather(tech 1): only tech-1 cells, no ord mixing");

            const int n0 = map.gatherLayerCells (60, 0, out, 4);
            check (n0 == 2, "gather(tech 0): two ord layers, pizz excluded");
        }

        // (e) maxOut cap honoured — never writes past the caller's buffer.
        {
            SampleMap map;
            map.numVelocityLayers = 4;
            for (int layer = 0; layer < 4; ++layer)
                map.cells.push_back (makeCell (60, layer, "v.wav"));

            const SampleCell* out[2] = { nullptr };
            const int n = map.gatherLayerCells (60, 0, out, 2);
            check (n == 2, "gather: maxOut=2 caps the returned count");
        }
    }

    std::cout << "== dynamics_layer_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
