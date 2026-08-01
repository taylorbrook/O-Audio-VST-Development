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

    ks_default_check.cpp
    O-MicrotonalSampler — v1.23.3 keyswitch-default regression tests
    (review WR-03 / IN-03 / IN-04, REVIEW-processor-state.md).

    Manual run: ninja O-MicrotonalSampler_KsDefaultCheck && ./...
    Exit code = number of failed assertions (0 = all pass).

    What this pins
    --------------
    v1.23.2 shipped a fresh-instance default of ks_enabled=true over MIDI
    0..9, so PluginProcessor::processBlock silently absorbed every note-on in
    0..9 as a keyswitch and never forwarded it to the synth. v1.23.3 makes
    keyswitches OPT-IN and narrows the default range to exactly kMaxTech slots.

    The defaults and the KS→technique mapping now live once in
    TechniqueDefaults.h, consumed by createParameterLayout, resetTechniqueNames,
    and processBlock. This executable guards them:

      1. Shipped defaults — ks_enabled=false, range 0..7, count=8. A red test
         here means a future edit re-flipped the default that eats low notes
         (exactly what the review warned maintainers not to "fix" back).
      2. keyswitchTechnique over the default range — notes 0..7 map to their
         own technique; notes 8/9 return -1 (forwarded, not absorbed) so no two
         notes collapse onto the last slot (IN-04).
      3. The clamp is still deliberate for narrower configs (count < range
         width still saturates high notes onto the last active slot).
      4. Canonical Dorico-aligned vocabulary (IN-03): stacc/trem, not sv/mart.

  ==============================================================================
*/

#include <juce_core/juce_core.h>

#include "../TechniqueDefaults.h"
#include "../TriggerMapping.h"

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
}

int main()
{
    using namespace OMtsTechnique;

    std::cout << "== ks_default_check ==\n";

    // ----------------------------------------------------------------
    // 1. Shipped fresh-instance defaults (WR-03 / IN-04).
    //    These are the values createParameterLayout hands to APVTS.
    //    Keyswitches MUST be opt-in so a new instance never eats low notes.
    // ----------------------------------------------------------------
    {
        check (kDefaultKsEnabled == false,
               "ks_enabled default is FALSE (keyswitches opt-in) — new "
               "instance forwards all notes, never absorbs MIDI 0..9");
        check (kDefaultKsLowNote == 0,
               "ks_low_note default is 0");
        check (kDefaultKsHighNote == 7,
               "ks_high_note default is 7 (ks_low + kMaxTech - 1)");
        check (kDefaultTechniqueCount == 8,
               "technique_count default is 8 (kMaxTech)");
        // The default range must span exactly kMaxTech slots — no slot short,
        // no slot over (the old 0..9 range advertised 10 slots for 8 techs).
        check (kDefaultKsHighNote - kDefaultKsLowNote + 1 == OMtsTrigger::kMaxTech,
               "default KS range width == kMaxTech (one note per technique)");
    }

    // ----------------------------------------------------------------
    // 2. keyswitchTechnique over the DEFAULT range (low=0, high=7, count=8).
    //    Every in-range note maps to its own technique; out-of-range → -1.
    // ----------------------------------------------------------------
    {
        for (int n = 0; n <= 7; ++n)
            check (keyswitchTechnique (n, kDefaultKsLowNote, kDefaultKsHighNote,
                                       kDefaultTechniqueCount) == n,
                   "note " + std::to_string (n) + " → technique "
                       + std::to_string (n));

        // Notes just past the default range are NOT absorbed (forwarded to
        // the synth). This is the IN-04 fix: 8 and 9 no longer saturate.
        check (keyswitchTechnique (8, kDefaultKsLowNote, kDefaultKsHighNote,
                                   kDefaultTechniqueCount) == -1,
               "note 8 → -1 (outside default range → forwarded, not absorbed)");
        check (keyswitchTechnique (9, kDefaultKsLowNote, kDefaultKsHighNote,
                                   kDefaultTechniqueCount) == -1,
               "note 9 → -1 (outside default range → forwarded, not absorbed)");
        check (keyswitchTechnique (-1, kDefaultKsLowNote, kDefaultKsHighNote,
                                   kDefaultTechniqueCount) == -1,
               "note -1 → -1 (below range)");

        // No two distinct in-range notes share a technique at the default
        // config — the concrete "no collapse" guarantee behind IN-04.
        bool seen[OMtsTrigger::kMaxTech] = { false };
        bool anyCollision = false;
        for (int n = kDefaultKsLowNote; n <= kDefaultKsHighNote; ++n)
        {
            const int t = keyswitchTechnique (n, kDefaultKsLowNote,
                                              kDefaultKsHighNote,
                                              kDefaultTechniqueCount);
            if (t < 0 || t >= OMtsTrigger::kMaxTech || seen[t])
                anyCollision = true;
            else
                seen[t] = true;
        }
        check (! anyCollision,
               "default range 0..7 maps to 8 distinct techniques (no collapse)");
    }

    // ----------------------------------------------------------------
    // 3. Clamp still saturates deliberately for narrower technique counts.
    //    With count=4 over a 0..7 range, notes >= 3 pin to the last slot (3).
    //    (Documented, intentional — the user narrowed the count.)
    // ----------------------------------------------------------------
    {
        check (keyswitchTechnique (2, 0, 7, 4) == 2, "count=4 — note 2 → tech 2");
        check (keyswitchTechnique (3, 0, 7, 4) == 3, "count=4 — note 3 → tech 3 (last)");
        check (keyswitchTechnique (6, 0, 7, 4) == 3, "count=4 — note 6 → tech 3 (saturates)");
        check (keyswitchTechnique (7, 0, 7, 4) == 3, "count=4 — note 7 → tech 3 (saturates)");
        // count is also clamped from below: count<=1 collapses everything to 0.
        check (keyswitchTechnique (5, 0, 7, 1) == 0, "count=1 — every note → tech 0");
    }

    // ----------------------------------------------------------------
    // 4. Canonical Dorico-aligned vocabulary (IN-03).
    //    slots 3 and 7 are the ones that had drifted (sv/mart → stacc/trem).
    // ----------------------------------------------------------------
    {
        const auto v = defaultTechniqueVocabulary();
        check (v.size() == 8, "default vocabulary has 8 slots");
        check (v == juce::StringArray { "ord", "sp", "st", "stacc",
                                        "cs", "pizz", "harm", "trem" },
               "vocabulary is the Dorico-aligned {ord,sp,st,stacc,cs,pizz,harm,trem}");
        check (v[3] == "stacc", "slot 3 is 'stacc' (not the stale 'sv')");
        check (v[7] == "trem",  "slot 7 is 'trem' (not the stale 'mart')");
    }

    std::cout << "== ks_default_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
