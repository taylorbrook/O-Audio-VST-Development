/*
  ==============================================================================

    state_migration_check.cpp
    O-MicrotonalSampler — v1.14.0 ValueTree state-migration tests.

    Manual run: ninja O-MicrotonalSampler_StateMigrationCheck && ./...
    Exit code = number of failed assertions (0 = all pass).

    What this exercises
    -------------------
    The v1.14.0 plan specifies a strict back-compat contract for state:

      "State migration: missing fields → defaults (technique_count=1,
       ks_enabled=false). Audio bit-identical to v1.13.0 reference."

    This test does NOT spin up the audio processor (that requires a host).
    It verifies the lower-level data model the migration relies on:

      1. A v1.13.0-shaped <SampleFolders> tree with NO <TechniqueNames>
         child decodes to empty / default vocabulary — the processor
         constructor must seed the curated vocab in this case.
      2. A v1.14.0 <TechniqueNames> tree with sparse slots (only some
         entries) decodes only the present slots and leaves the rest at
         the curated default.
      3. SampleCell::technique defaults to 0 — so any v1.13.0
         deserialised cell (no technique attr) plays as "ord" without
         further work.
      4. The applyMergeRrCell helper, when run against a v1.13.0-shaped
         input cell vector (technique=0 throughout) and a v1.13.0-shaped
         new cell, behaves identically to v1.13.0: same-(midi, vel)
         collisions merge into one cell.

    The intent is to catch regressions in the migration contract that
    don't show up until the very first project reopen — by which time
    the user has lost work.

  ==============================================================================
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../SampleMap.h"
#include "../TechniqueDefaults.h"   // canonical default vocabulary (IN-03)

#include <iostream>
#include <string>

namespace
{
    constexpr const char* kTechniqueNamesTag = "TechniqueNames";
    constexpr const char* kTechniqueSlotTag  = "Slot";

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

    // The curated default seeded by resetTechniqueNames — pulled from the
    // shared source of truth so this fixture can never drift from the real
    // vocabulary again (it previously claimed the stale "sv"/"mart" names).
    juce::StringArray defaultVocab()
    {
        return OMtsTechnique::defaultTechniqueVocabulary();
    }

    // Mimics restoreStateValueTree's <TechniqueNames> branch — applies the
    // tree on top of the seeded default. We do NOT need to spin up the
    // processor; the migration logic is a tiny data transformation.
    void applyTechniqueNamesTree (juce::StringArray& names, const juce::ValueTree& tree)
    {
        if (! tree.isValid()) return;
        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            const auto slot = tree.getChild (i);
            if (! slot.hasType (kTechniqueSlotTag)) continue;
            const int  idx  = juce::jlimit (0, 7,
                                  static_cast<int> (slot.getProperty ("index", 0)));
            const auto name = slot.getProperty ("name").toString();
            if (name.isNotEmpty())
            {
                while (names.size() <= idx)
                    names.add ("ord");
                names.set (idx, name);
            }
        }
    }
}

int main()
{
    std::cout << "== state_migration_check ==\n";

    // 1. v1.13.0 shape — no <TechniqueNames> child. Default vocab survives.
    {
        auto names = defaultVocab();
        juce::ValueTree empty;
        applyTechniqueNamesTree (names, empty);
        check (names == defaultVocab(),
               "Missing <TechniqueNames> → curated default vocab unchanged");
    }

    // 2. v1.14.0 sparse tree — only renames slot 1.
    {
        auto names = defaultVocab();
        juce::ValueTree tree (kTechniqueNamesTag);
        juce::ValueTree slot (kTechniqueSlotTag);
        slot.setProperty ("index", 1, nullptr);
        slot.setProperty ("name",  "sul pont", nullptr);
        tree.appendChild (slot, nullptr);

        applyTechniqueNamesTree (names, tree);

        check (names[0] == "ord", "Slot 0 untouched by sparse rename");
        check (names[1] == "sul pont", "Slot 1 renamed to 'sul pont'");
        check (names[7] == "trem", "Slot 7 untouched by sparse rename");
    }

    // 3. SampleCell::technique defaults to 0 (POD default-init verified)
    {
        SampleCell c;
        check (c.technique == 0, "SampleCell::technique default == 0");
    }

    // 4. v1.13.0-shaped merge: all cells at technique=0 → identical to v1.13.0.
    {
        std::vector<SampleCell> target;
        target.push_back (makeCell (60, 0, 0, "C3_a.wav"));

        SampleCell newCell = makeCell (60, 0, 0, "C3_b.wav");
        juce::StringArray skipped;
        const bool ok = applyMergeRrCell (target, newCell, /*cap*/ 64, skipped);

        check (ok, "v1.13.0-shape merge returns true");
        check (target.size() == 1, "v1.13.0-shape merge produces exactly one cell");
        check (target[0].variants.size() == 2,
               "v1.13.0-shape merge appends second variant onto first cell");
        check (target[0].technique == 0,
               "v1.13.0-shape merge preserves technique=0");
    }

    // 5. SampleMap::findCell two-arg back-compat is bit-identical to a
    //    v1.13.0 single-technique library — the old call site behaviour
    //    survives.
    {
        SampleMap map;
        map.cells.push_back (makeCell (60, 0, 0, "ord.wav"));
        map.cells.push_back (makeCell (61, 0, 0, "ord2.wav"));

        const auto* a = map.findCell (60, 0);     // legacy two-arg
        const auto* b = map.findCell (60, 0, 0);  // explicit tech=0
        check (a != nullptr && a == b,
               "Two-arg findCell (60,0) == three-arg findCell (60,0,0)");
    }

    std::cout << "== state_migration_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
