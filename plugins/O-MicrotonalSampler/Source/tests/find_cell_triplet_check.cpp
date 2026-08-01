/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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

    find_cell_triplet_check.cpp
    O-MicrotonalSampler — v1.14.0 SampleMap::findCell triplet-key tests.

    Manual run: ninja O-MicrotonalSampler_FindCellTripletCheck && ./...
    Exit code = number of failed assertions (0 = all pass).

    What this exercises
    -------------------
    The expanded cell key — cells are now uniquely identified by
    (midiNote, velocityLayer, technique), with technique=0 acting as
    the "ord" fallback when the requested technique is empty.

    Invariants:

      1. Exact-match: requesting a present (midi, vel, tech) returns
         that cell.
      2. Fallback: requesting tech > 0 when only tech=0 has cells →
         returns the tech=0 cell.
      3. Strict miss: tech=0 lookup with no tech=0 cell → nullptr,
         even if higher-tech cells exist (no upward fallback).
      4. Disjoint techniques: two cells at the same (midi, vel) but
         different techniques are distinct — looking up either tech
         returns the matching cell, not the other.
      5. Closest-note lookup respects technique: tech=1 lookup at midi=64
         when tech=1 has a cell at midi=62 returns that cell, not a
         closer tech=0 cell at midi=63.
      6. Two-arg back-compat overload routes to tech=0.

    SampleMap::applyMergeRrCell triplet-key invariants are also covered:

      7. Cells with same (midi, vel) but different techniques do NOT
         collide — the helper appends the new cell rather than
         merging.

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

    SampleVariant makeVariant (const juce::String& filename)
    {
        SampleVariant v;
        v.filename = filename;
        return v;
    }

    SampleCell makeCell (int midi, int layer, int tech, const juce::String& filename)
    {
        SampleCell c;
        c.midiNote      = midi;
        c.velocityLayer = layer;
        c.technique     = tech;
        c.variants.push_back (makeVariant (filename));
        return c;
    }
}

int main()
{
    std::cout << "== find_cell_triplet_check ==\n";

    SampleMap map;
    map.cells.push_back (makeCell (60, 0, 0, "C3_ord.wav"));
    map.cells.push_back (makeCell (60, 0, 1, "C3_sp.wav"));
    map.cells.push_back (makeCell (62, 0, 1, "D3_sp.wav"));
    map.cells.push_back (makeCell (63, 0, 0, "D#3_ord.wav"));
    map.cells.push_back (makeCell (60, 1, 5, "C3_v2_pizz.wav"));

    // 1. Exact match
    {
        const auto* c = map.findCell (60, 0, 0);
        check (c != nullptr && c->variants.front().filename == "C3_ord.wav",
               "Exact match (60, 0, ord)");
    }
    {
        const auto* c = map.findCell (60, 0, 1);
        check (c != nullptr && c->variants.front().filename == "C3_sp.wav",
               "Exact match (60, 0, sp)");
    }

    // 2. Fallback to technique=0 when requested slot empty
    {
        // tech=2 has nothing at midi=60,layer=0 → falls back to tech=0 (C3_ord)
        const auto* c = map.findCell (60, 0, 2);
        check (c != nullptr && c->technique == 0
                            && c->variants.front().filename == "C3_ord.wav",
               "Fallback: (60, 0, tech=2) → tech=0 cell");
    }

    // 3. Strict miss when tech=0 also empty
    {
        // layer=2 has no cells in any technique
        const auto* c = map.findCell (60, 2, 0);
        check (c == nullptr,
               "Strict miss: (60, layer=2, tech=0) returns nullptr");
    }

    // 4. Disjoint techniques — tech=1 at (60,0) is NOT the tech=0 cell
    {
        const auto* tech0 = map.findCell (60, 0, 0);
        const auto* tech1 = map.findCell (60, 0, 1);
        check (tech0 != nullptr && tech1 != nullptr && tech0 != tech1,
               "Disjoint cells: tech 0 vs tech 1 at same (midi, vel)");
    }

    // 5. Closest-note lookup respects technique
    {
        // Requesting tech=1 at midi=64: tech=1 only has cells at 60 and 62.
        // Distance(60,64)=4, Distance(62,64)=2 → expect midi=62.
        // A naïve impl that ignores technique would pick midi=63 (tech=0).
        const auto* c = map.findCell (64, 0, 1);
        check (c != nullptr && c->midiNote == 62 && c->technique == 1,
               "Closest-note within technique slot wins over closer foreign-tech");
    }

    // 6. Two-arg back-compat overload routes to tech=0
    {
        const auto* tech0 = map.findCell (60, 0, 0);
        const auto* legacy = map.findCell (60, 0);
        check (tech0 == legacy, "Two-arg findCell == three-arg with tech=0");
    }

    // 7. applyMergeRrCell triplet-key — same (midi, vel) different tech ≠ collision
    {
        std::vector<SampleCell> target;
        target.push_back (makeCell (60, 0, 0, "ord_a.wav"));

        SampleCell newCell = makeCell (60, 0, 1, "sp_a.wav");
        juce::StringArray skipped;
        const bool ok = applyMergeRrCell (target, newCell, /*cap*/ 64, skipped);

        check (ok, "applyMergeRrCell returns true on technique-mismatched insert");
        check (target.size() == 2,
               "applyMergeRrCell appends instead of merging across technique boundary");
        check (target[0].technique == 0 && target[1].technique == 1,
               "applyMergeRrCell preserves technique field on insert");
    }

    // 8. applyMergeRrCell triplet-key — same triplet WILL merge variants
    {
        std::vector<SampleCell> target;
        target.push_back (makeCell (60, 0, 0, "ord_a.wav"));

        SampleCell newCell = makeCell (60, 0, 0, "ord_b.wav");
        juce::StringArray skipped;
        const bool ok = applyMergeRrCell (target, newCell, /*cap*/ 64, skipped);

        check (ok && target.size() == 1 && target[0].variants.size() == 2,
               "applyMergeRrCell merges variants when triplet matches");
    }

    // 9. findCellNearestLayer — requested technique on a DIFFERENT velocity
    //    layer beats an ord cell sitting on the requested layer (v1.18.3
    //    regression guard for the pizz-reverts-to-ord bug).
    {
        // pizz (tech=5) exists only at layer 1; ord (tech=0) exists at layer 0.
        // Requesting (60, layer=0, tech=5) must expand to layer 1 and return the
        // pizz cell — NOT fall back to the ord cell on layer 0.
        const auto* c = map.findCellNearestLayer (60, 0, 5);
        check (c != nullptr && c->technique == 5
                            && c->variants.front().filename == "C3_v2_pizz.wav",
               "findCellNearestLayer: technique on other layer beats ord on requested layer");
    }

    // 10. findCellNearestLayer — genuinely empty technique slot still falls back
    //     to ord (the "empty slot plays ord" contract is preserved).
    {
        // tech=3 has no cells on any layer → Phase 2 falls back to ord (tech=0).
        const auto* c = map.findCellNearestLayer (60, 0, 3);
        check (c != nullptr && c->technique == 0,
               "findCellNearestLayer: empty technique slot falls back to ord");
    }

    // 11. findCellNearestLayer — exact (layer, technique) still hits directly,
    //     no behavioural change for fully-populated lookups.
    {
        const auto* c = map.findCellNearestLayer (60, 0, 1);
        check (c != nullptr && c->technique == 1
                            && c->variants.front().filename == "C3_sp.wav",
               "findCellNearestLayer: exact layer+technique hit unchanged");
    }

    std::cout << "== find_cell_triplet_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
