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

    embedded_technique_check.cpp
    O-MicrotonalSampler — v1.23.1 embedded-audio technique round-trip regression.

    Manual run: ninja O-MicrotonalSampler_EmbeddedTechniqueCheck && ./...
    Exit code = number of failed assertions (0 = all pass).

    What this pins (CR-01 / review C2, 2026-06-30)
    ----------------------------------------------
    The embedded-library serializer (`buildEmbeddedAudioTree` /
    `decodeEmbeddedAudioTree`, PluginProcessor.cpp anon namespace ~2628/2660)
    round-trips each SampleCell through a <cell> ValueTree. Before v1.23.1 it
    persisted only `midi` + `layer`. Because `technique` is part of the cell key
    `(midi, layer, technique)` (SampleMap.h), every embedded cell on a non-zero
    technique slot reloaded collapsed onto technique 0 — silent sample-map
    corruption and collisions with real "ord" cells on project reopen.

    Those two codec functions are file-local (anonymous namespace inside
    PluginProcessor.cpp) and depend on WAV/base64 helpers, so — following the
    convention of state_migration_check.cpp — this test MIRRORS the cell-key
    property contract (`midi`/`layer`/`tech`) and drives it against the REAL
    SampleMap key + findCell logic. The audio blob itself is out of scope for a
    key-integrity test; only the addressing axis matters here.

    The `serializeCellKey`/`deserializeCellKey` mirrors below are byte-for-byte
    the property lines in the shipped codec — keep them in sync if that codec
    changes. Case 3 is an explicit reproduction of the pre-v1.23.1 corruption.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../SampleMap.h"

#include <iostream>
#include <string>

namespace
{
    constexpr const char* kCellTag = "cell";

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

    SampleCell makeCell (int midi, int layer, int tech, const juce::String& filename)
    {
        SampleCell c;
        c.midiNote      = midi;
        c.velocityLayer = layer;
        c.technique     = tech;
        SampleVariant v;
        v.filename = filename;
        c.variants.push_back (v);
        return c;
    }

    // Mirrors buildEmbeddedAudioTree's per-cell key write (PluginProcessor.cpp).
    // `writeTech=false` reproduces the pre-v1.23.1 codec that dropped technique.
    juce::ValueTree serializeCellKey (const SampleCell& cell, bool writeTech)
    {
        juce::ValueTree cellTree (kCellTag);
        cellTree.setProperty ("midi",  cell.midiNote,      nullptr);
        cellTree.setProperty ("layer", cell.velocityLayer, nullptr);
        if (writeTech)
            cellTree.setProperty ("tech", cell.technique,  nullptr);   // v1.23.1 fix
        // filename carried only so the test can identify the cell after decode.
        cellTree.setProperty ("filename", cell.variants.front().filename, nullptr);
        return cellTree;
    }

    // Mirrors decodeEmbeddedAudioTree's per-cell key read (PluginProcessor.cpp).
    SampleCell deserializeCellKey (const juce::ValueTree& cellTree)
    {
        SampleCell cell;
        cell.midiNote      = juce::jlimit (0, 127, static_cast<int> (cellTree.getProperty ("midi", 0)));
        cell.velocityLayer = juce::jlimit (0, 3,   static_cast<int> (cellTree.getProperty ("layer", 0)));
        cell.technique     = juce::jlimit (0, kMaxTechniques - 1,
                                           static_cast<int> (cellTree.getProperty ("tech", 0)));
        SampleVariant v;
        v.filename = cellTree.getProperty ("filename").toString();
        cell.variants.push_back (v);
        return cell;
    }

    SampleMap roundTrip (const SampleMap& in, bool writeTech)
    {
        juce::ValueTree audioTree ("embeddedAudio");
        for (const auto& c : in.cells)
            audioTree.appendChild (serializeCellKey (c, writeTech), nullptr);

        SampleMap out;
        for (int i = 0; i < audioTree.getNumChildren(); ++i)
            out.cells.push_back (deserializeCellKey (audioTree.getChild (i)));
        return out;
    }
}

int main()
{
    std::cout << "== embedded_technique_check ==\n";

    // A library where the SAME (midi, layer) is populated on two technique
    // slots — exactly the shape C2 corrupted: "ord" on slot 0, "pizz" on slot 5.
    SampleMap src;
    src.cells.push_back (makeCell (60, 0, 0, "ord.wav"));
    src.cells.push_back (makeCell (60, 0, 5, "pizz.wav"));

    // 1. Fixed codec (writeTech=true): technique survives the round-trip.
    {
        const SampleMap out = roundTrip (src, /*writeTech*/ true);
        check (out.cells.size() == 2, "round-trip preserves both cells");

        const auto* ord  = out.findCell (60, 0, 0);
        const auto* pizz = out.findCell (60, 0, 5);
        check (ord  != nullptr && ord->variants.front().filename  == "ord.wav",
               "findCell(60,0,tech=0) resolves to ord.wav after round-trip");
        check (pizz != nullptr && pizz->variants.front().filename == "pizz.wav",
               "findCell(60,0,tech=5) resolves to pizz.wav after round-trip");
        check (ord != pizz,
               "technique axis keeps the two same-(midi,layer) cells distinct");
    }

    // 2. Every technique value round-trips unchanged (0..kMaxTechniques-1).
    {
        SampleMap wide;
        for (int t = 0; t < kMaxTechniques; ++t)
            wide.cells.push_back (makeCell (72, 0, t, "t" + juce::String (t) + ".wav"));

        const SampleMap out = roundTrip (wide, /*writeTech*/ true);
        bool allMatch = (out.cells.size() == (size_t) kMaxTechniques);
        for (int t = 0; t < kMaxTechniques && allMatch; ++t)
        {
            const auto* c = out.findCell (72, 0, t);
            allMatch = (c != nullptr && c->technique == t
                        && c->variants.front().filename == "t" + juce::String (t) + ".wav");
        }
        check (allMatch, "all kMaxTechniques slots survive the round-trip exactly");
    }

    // 3. BUG REPRODUCTION — the pre-v1.23.1 codec (writeTech=false) collapses
    //    every cell onto technique 0, so the "pizz" identity is lost: both cells
    //    key to (60,0,0) and findCell(60,0,5) falls back to whatever landed on
    //    slot 0 first. This asserts the OLD behaviour to document exactly what
    //    the fix repairs — if a future change made writeTech=false behave like
    //    the fix, this guard would (correctly) need revisiting.
    {
        const SampleMap out = roundTrip (src, /*writeTech*/ false);
        const auto* pizz = out.findCell (60, 0, 5);
        check (pizz != nullptr && pizz->variants.front().filename == "ord.wav",
               "pre-fix codec (no 'tech'): tech-5 lookup falls back to slot-0 'ord' (corruption reproduced)");
        check (out.cells[0].technique == 0 && out.cells[1].technique == 0,
               "pre-fix codec (no 'tech'): every decoded cell collapses to technique 0");
    }

    std::cout << "== embedded_technique_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
