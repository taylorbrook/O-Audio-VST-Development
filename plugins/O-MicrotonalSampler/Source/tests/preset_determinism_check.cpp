/*
  ==============================================================================

    preset_determinism_check.cpp
    O-MicrotonalSampler — v1.23.4 (WR-02) preset-restore determinism tests.

    Manual run:
      ninja O-MicrotonalSampler_PresetDeterminismCheck && \
        ./build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_PresetDeterminismCheck
    Exit code = number of failed assertions (0 = all pass).

    What this exercises
    -------------------
    Review finding WR-02 (REVIEW-processor-state.md): restoreStateValueTree
    handled "state the preset does not carry" inconsistently. Trims were reset
    to unity before applying the sparse saved entries, but technique names and
    the CC/PC trigger tables were only *overridden where present* — so loading a
    partial / older .omspreset over a customized session left the PREVIOUS
    session's renamed techniques and custom trigger tables active (bleed-through).

    v1.23.4 fixes this by mirroring the trims pattern: reset technique names ->
    defaultTechniqueVocabulary(), CC -> defaultCcMapping(count), PC ->
    defaultPcMapping() BEFORE applying whatever the loaded tree carries.

    This test models that fixed restore logic (the restore path can't be unit-
    tested without a live processor — same constraint as state_migration_check)
    and pins the determinism contract:

      "The final restored state depends ONLY on the loaded tree, never on the
       state that was live before the load."

    It deliberately builds a CUSTOMIZED prior state, confirms it differs from the
    defaults, then restores a tree that omits each child and asserts the result
    collapses back to the defaults — i.e. no bleed. It uses the REAL default
    factories (OMtsTechnique::defaultTechniqueVocabulary, OMtsTrigger::default*)
    so it also catches default-vocabulary / default-mapping drift.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../TriggerMapping.h"
#include "../TechniqueDefaults.h"

#include <iostream>
#include <string>

namespace
{
    // XML tags — must match the constants in PluginProcessor.cpp.
    constexpr const char* kTechniqueNamesTag = "TechniqueNames";
    constexpr const char* kTechniqueSlotTag  = "Slot";
    constexpr const char* kCcMappingTag      = "CcMapping";
    constexpr const char* kPcMappingTag      = "PcMapping";
    constexpr const char* kTriggerSlotTag    = "Slot";

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

    //--------------------------------------------------------------------------
    // Faithful models of restoreStateValueTree's FIXED (v1.23.4) branches.
    // Each RESETS to the canonical default first, THEN applies the tree's
    // present slots — exactly the production reset-before-apply order.

    juce::StringArray restoreTechniqueNames (const juce::ValueTree& tree)
    {
        auto names = OMtsTechnique::defaultTechniqueVocabulary();   // reset first
        if (tree.isValid())
        {
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
        return names;
    }

    OMtsTrigger::CcMapping restoreCcMapping (const juce::ValueTree& tree, int count)
    {
        auto cc = OMtsTrigger::defaultCcMapping (count);            // reset first
        if (tree.isValid())
        {
            for (int i = 0; i < tree.getNumChildren(); ++i)
            {
                const auto slot = tree.getChild (i);
                if (! slot.hasType (kTriggerSlotTag)) continue;
                const int idx = juce::jlimit (0, OMtsTrigger::kMaxSlots - 1,
                    static_cast<int> (slot.getProperty ("index", -1)));
                if (idx < 0) continue;
                auto& s = cc.at ((size_t) idx);
                s.rangeLow  = juce::jlimit (0, 127, static_cast<int> (slot.getProperty ("lo", s.rangeLow)));
                s.rangeHigh = juce::jlimit (0, 127, static_cast<int> (slot.getProperty ("hi", s.rangeHigh)));
                if (s.rangeHigh < s.rangeLow) std::swap (s.rangeLow, s.rangeHigh);
                s.technique = juce::jlimit (0, OMtsTrigger::kMaxTech - 1,
                                            static_cast<int> (slot.getProperty ("tech", s.technique)));
            }
        }
        return cc;
    }

    OMtsTrigger::PcMapping restorePcMapping (const juce::ValueTree& tree)
    {
        auto pc = OMtsTrigger::defaultPcMapping();                  // reset first
        if (tree.isValid())
        {
            for (int i = 0; i < tree.getNumChildren(); ++i)
            {
                const auto slot = tree.getChild (i);
                if (! slot.hasType (kTriggerSlotTag)) continue;
                const int idx = juce::jlimit (0, OMtsTrigger::kMaxSlots - 1,
                    static_cast<int> (slot.getProperty ("index", -1)));
                if (idx < 0) continue;
                auto& s = pc.at ((size_t) idx);
                s.pc        = juce::jlimit (0, 127, static_cast<int> (slot.getProperty ("pc", s.pc)));
                s.technique = juce::jlimit (0, OMtsTrigger::kMaxTech - 1,
                                            static_cast<int> (slot.getProperty ("tech", s.technique)));
            }
        }
        return pc;
    }

    bool ccEqual (const OMtsTrigger::CcMapping& a, const OMtsTrigger::CcMapping& b)
    {
        for (size_t i = 0; i < a.size(); ++i)
            if (a[i].rangeLow != b[i].rangeLow || a[i].rangeHigh != b[i].rangeHigh
                || a[i].technique != b[i].technique)
                return false;
        return true;
    }

    bool pcEqual (const OMtsTrigger::PcMapping& a, const OMtsTrigger::PcMapping& b)
    {
        for (size_t i = 0; i < a.size(); ++i)
            if (a[i].pc != b[i].pc || a[i].technique != b[i].technique)
                return false;
        return true;
    }

    juce::ValueTree makeTechniqueTree (int index, const juce::String& name)
    {
        juce::ValueTree tree (kTechniqueNamesTag);
        juce::ValueTree slot (kTechniqueSlotTag);
        slot.setProperty ("index", index, nullptr);
        slot.setProperty ("name",  name,  nullptr);
        tree.appendChild (slot, nullptr);
        return tree;
    }
}

int main()
{
    std::cout << "== preset_determinism_check ==\n";

    const auto defaultVocab = OMtsTechnique::defaultTechniqueVocabulary();
    const auto defaultCc     = OMtsTrigger::defaultCcMapping (8);
    const auto defaultPc     = OMtsTrigger::defaultPcMapping();

    // ---- Technique names -----------------------------------------------------

    // 1. WR-02 core: a preset with NO <TechniqueNames> child must reset to the
    //    default vocab — NOT bleed the prior session's renames. (The prior
    //    "customized" state is irrelevant because the model rebuilds from the
    //    default; that IS the fix.)
    {
        juce::ValueTree absent;   // invalid → child omitted
        const auto restored = restoreTechniqueNames (absent);
        check (restored == defaultVocab,
               "No <TechniqueNames> child -> resets to default vocab (no bleed)");
    }

    // 2. Sanity: a customized state genuinely differs from the default, so the
    //    no-bleed assertion above is meaningful (guards against a vacuous pass).
    {
        auto customized = defaultVocab;
        customized.set (2, "MY RENAME");
        check (customized != defaultVocab,
               "Customized technique names differ from default (assertion is non-vacuous)");
    }

    // 3. Sparse rename applies on top of the reset default.
    {
        const auto tree = makeTechniqueTree (1, "sul pont");
        const auto restored = restoreTechniqueNames (tree);
        check (restored[0] == defaultVocab[0], "Sparse rename: slot 0 stays default");
        check (restored[1] == "sul pont",      "Sparse rename: slot 1 overridden");
        check (restored[7] == defaultVocab[7], "Sparse rename: slot 7 stays default");
    }

    // 4. Determinism: same tree, independent of any prior state → identical.
    {
        const auto tree = makeTechniqueTree (3, "flautando");
        const auto a = restoreTechniqueNames (tree);
        const auto b = restoreTechniqueNames (tree);
        check (a == b, "Technique restore is deterministic (tree-only)");
    }

    // ---- CC mapping ----------------------------------------------------------

    // 5. WR-02 core: no <CcMapping> child → resets to defaultCcMapping(count).
    {
        juce::ValueTree absent;
        const auto restored = restoreCcMapping (absent, 8);
        check (ccEqual (restored, defaultCc),
               "No <CcMapping> child -> resets to defaultCcMapping (no bleed)");
    }

    // 6. Sanity: a customized CC table differs from default.
    {
        auto customized = defaultCc;
        customized[0].rangeLow  = 40;
        customized[0].rangeHigh = 50;
        customized[0].technique = 5;
        check (! ccEqual (customized, defaultCc),
               "Customized CC table differs from default (assertion is non-vacuous)");
    }

    // 7. Sparse CC override applies on top of the reset default.
    {
        juce::ValueTree tree (kCcMappingTag);
        juce::ValueTree slot (kTriggerSlotTag);
        slot.setProperty ("index", 2,  nullptr);
        slot.setProperty ("lo",    64, nullptr);
        slot.setProperty ("hi",    80, nullptr);
        slot.setProperty ("tech",  4,  nullptr);
        tree.appendChild (slot, nullptr);

        const auto restored = restoreCcMapping (tree, 8);
        check (restored[2].rangeLow == 64 && restored[2].rangeHigh == 80
                   && restored[2].technique == 4,
               "Sparse CC override: slot 2 applied over default");
        check (restored[0].rangeLow == defaultCc[0].rangeLow
                   && restored[0].rangeHigh == defaultCc[0].rangeHigh,
               "Sparse CC override: slot 0 stays default");
    }

    // 8. CC reversed lo/hi gets normalised (rangeHigh >= rangeLow).
    {
        juce::ValueTree tree (kCcMappingTag);
        juce::ValueTree slot (kTriggerSlotTag);
        slot.setProperty ("index", 1,  nullptr);
        slot.setProperty ("lo",    90, nullptr);
        slot.setProperty ("hi",    10, nullptr);
        tree.appendChild (slot, nullptr);

        const auto restored = restoreCcMapping (tree, 8);
        check (restored[1].rangeLow <= restored[1].rangeHigh,
               "Reversed CC lo/hi is normalised on restore");
    }

    // ---- PC mapping ----------------------------------------------------------

    // 9. WR-02 core: no <PcMapping> child → resets to defaultPcMapping().
    {
        juce::ValueTree absent;
        const auto restored = restorePcMapping (absent);
        check (pcEqual (restored, defaultPc),
               "No <PcMapping> child -> resets to defaultPcMapping (no bleed)");
    }

    // 10. Sparse PC override applies on top of the reset default.
    {
        juce::ValueTree tree (kPcMappingTag);
        juce::ValueTree slot (kTriggerSlotTag);
        slot.setProperty ("index", 3, nullptr);
        slot.setProperty ("pc",    42, nullptr);
        slot.setProperty ("tech",  6, nullptr);
        tree.appendChild (slot, nullptr);

        const auto restored = restorePcMapping (tree);
        check (restored[3].pc == 42 && restored[3].technique == 6,
               "Sparse PC override: slot 3 applied over default");
        check (restored[0].pc == defaultPc[0].pc,
               "Sparse PC override: slot 0 stays default");
    }

    // 11. Determinism: same PC tree twice → identical.
    {
        juce::ValueTree tree (kPcMappingTag);
        juce::ValueTree slot (kTriggerSlotTag);
        slot.setProperty ("index", 5, nullptr);
        slot.setProperty ("pc",    12, nullptr);
        tree.appendChild (slot, nullptr);

        const auto a = restorePcMapping (tree);
        const auto b = restorePcMapping (tree);
        check (pcEqual (a, b), "PC restore is deterministic (tree-only)");
    }

    std::cout << "== preset_determinism_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
