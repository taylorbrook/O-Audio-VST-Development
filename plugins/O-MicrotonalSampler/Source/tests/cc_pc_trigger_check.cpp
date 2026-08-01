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

    cc_pc_trigger_check.cpp
    O-MicrotonalSampler — v1.15.0 CC + PC trigger routing tests.

    Manual run: ninja O-MicrotonalSampler_CcPcTriggerCheck && ./...
    Exit code = number of failed assertions (0 = all pass).

    What this exercises
    -------------------

    The v1.15.0 plan promotes three trigger mechanisms (KS / CC / PC) and
    binds their precedence to a single technique cursor in processBlock:

        KS > CC > PC > history > 0

    The tables themselves live in TriggerMapping.h as plain data + free
    helpers:

      * defaultCcMapping(count)   — equally split [0..127] across `count`
                                    techniques.
      * defaultPcMapping()        — pc[i] → tech[i].
      * resolveCcTechnique(value, slots)  — first containing slot wins.
      * resolvePcTechnique(pc#, slots)    — first matching slot wins.
      * resolveTriggerPrecedence(ks, cc, pc, history)
                                  — KS > CC > PC > history.

    Each helper is independently testable without instantiating the audio
    processor or any JUCE Component — the goal of this executable is to
    pin the routing contract that processBlock relies on.

  ==============================================================================
*/

#include <juce_core/juce_core.h>

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
    using namespace OMtsTrigger;

    std::cout << "== cc_pc_trigger_check ==\n";

    // ----------------------------------------------------------------
    // 1. defaultCcMapping equally splits 0..127 across `count` techs.
    // ----------------------------------------------------------------
    {
        // count=1 → entire range routes to tech 0.
        const auto m1 = defaultCcMapping (1);
        check (m1[0].rangeLow  == 0   && m1[0].rangeHigh == 127 && m1[0].technique == 0,
               "count=1 — slot 0 covers 0..127 → tech 0");

        // count=4 → 32-wide bands.
        const auto m4 = defaultCcMapping (4);
        check (m4[0].rangeLow == 0   && m4[0].rangeHigh == 31,
               "count=4 — slot 0 covers 0..31");
        check (m4[1].rangeLow == 32  && m4[1].rangeHigh == 63,
               "count=4 — slot 1 covers 32..63");
        check (m4[2].rangeLow == 64  && m4[2].rangeHigh == 95,
               "count=4 — slot 2 covers 64..95");
        check (m4[3].rangeLow == 96  && m4[3].rangeHigh == 127,
               "count=4 — slot 3 covers 96..127");
        check (m4[3].technique == 3,
               "count=4 — slot 3 routes to tech 3");

        // count=8 → 16-wide bands; every slot has its own technique.
        const auto m8 = defaultCcMapping (8);
        for (int i = 0; i < 8; ++i)
        {
            const int expectedLo = i * 16;
            const int expectedHi = (i + 1) * 16 - 1;
            check (m8[(size_t) i].rangeLow  == expectedLo
                && m8[(size_t) i].rangeHigh == expectedHi
                && m8[(size_t) i].technique == i,
                   "count=8 — slot " + std::to_string (i) + " routes correctly");
        }
    }

    // ----------------------------------------------------------------
    // 2. defaultPcMapping: pc i → tech i.
    // ----------------------------------------------------------------
    {
        const auto pc = defaultPcMapping();
        for (int i = 0; i < 8; ++i)
        {
            check (pc[(size_t) i].pc == i && pc[(size_t) i].technique == i,
                   "defaultPcMapping[" + std::to_string (i) + "] = (pc=i, tech=i)");
        }
    }

    // ----------------------------------------------------------------
    // 3. resolveCcTechnique routes inside band → expected tech.
    // ----------------------------------------------------------------
    {
        const auto m4 = defaultCcMapping (4);
        check (resolveCcTechnique (0,   m4) == 0, "CC value 0 → tech 0");
        check (resolveCcTechnique (15,  m4) == 0, "CC value 15 (mid-band 0) → tech 0");
        check (resolveCcTechnique (31,  m4) == 0, "CC value 31 (high edge band 0) → tech 0");
        check (resolveCcTechnique (32,  m4) == 1, "CC value 32 (low edge band 1) → tech 1");
        check (resolveCcTechnique (64,  m4) == 2, "CC value 64 (low edge band 2) → tech 2");
        check (resolveCcTechnique (95,  m4) == 2, "CC value 95 (high edge band 2) → tech 2");
        check (resolveCcTechnique (96,  m4) == 3, "CC value 96 (low edge band 3) → tech 3");
        check (resolveCcTechnique (127, m4) == 3, "CC value 127 → tech 3");
    }

    // ----------------------------------------------------------------
    // 4. resolveCcTechnique out-of-range input → -1.
    // ----------------------------------------------------------------
    {
        const auto m4 = defaultCcMapping (4);
        check (resolveCcTechnique (-1,   m4) == -1, "CC value -1  → -1 (no match)");
        check (resolveCcTechnique (128,  m4) == -1, "CC value 128 → -1 (no match)");
        check (resolveCcTechnique (1000, m4) == -1, "CC value 1000 → -1 (no match)");
    }

    // ----------------------------------------------------------------
    // 5. resolveCcTechnique with a custom mapping — first match wins.
    // ----------------------------------------------------------------
    {
        CcMapping m{};
        // Two slots both cover value 50; slot 0 wins.
        m[0] = CcSlot { 0,   100, 3 };
        m[1] = CcSlot { 40,  60,  5 };
        for (size_t i = 2; i < m.size(); ++i)
            m[i] = CcSlot { 200, 200, 0 };  // off-range, never match

        check (resolveCcTechnique (50, m) == 3,
               "Overlapping bands — first match wins");
    }

    // ----------------------------------------------------------------
    // 6. resolvePcTechnique on a sparse table.
    // ----------------------------------------------------------------
    {
        PcMapping m{};
        m[0] = PcSlot { 10, 7 };
        m[1] = PcSlot { 20, 3 };
        for (size_t i = 2; i < m.size(); ++i)
            m[i] = PcSlot { 200, 0 };  // out-of-range pc, never match

        check (resolvePcTechnique (10, m) == 7, "PC# 10 → tech 7");
        check (resolvePcTechnique (20, m) == 3, "PC# 20 → tech 3");
        check (resolvePcTechnique (5,  m) == -1, "PC# 5 (no slot) → -1");
        check (resolvePcTechnique (-1, m) == -1, "PC# -1 (out of range) → -1");
        // 200 is past the MIDI 0..127 PC range; resolvePcTechnique
        // rejects it on input regardless of what's in the table.
        check (resolvePcTechnique (200,m) == -1,
               "PC# 200 (out of MIDI range) → -1, even if a slot stores it");
    }

    // ----------------------------------------------------------------
    // 7. Precedence resolver — the core v1.15.0 contract.
    //    KS > CC > PC > history.
    // ----------------------------------------------------------------
    {
        // KS wins over everything when present.
        check (resolveTriggerPrecedence (3, 1, 7, 0) == 3,
               "KS=3, CC=1, PC=7, hist=0 → 3 (KS)");
        check (resolveTriggerPrecedence (5, -1, -1, 0) == 5,
               "KS=5 only → 5");

        // CC wins over PC when KS absent.
        check (resolveTriggerPrecedence (-1, 2, 7, 0) == 2,
               "CC=2, PC=7 (no KS) → 2 (CC)");
        check (resolveTriggerPrecedence (-1, 0, -1, 4) == 0,
               "CC=0 (explicit zero!) beats history=4 → 0");

        // PC wins when KS + CC absent.
        check (resolveTriggerPrecedence (-1, -1, 6, 0) == 6,
               "PC=6 only → 6");

        // History wins when nothing fired.
        check (resolveTriggerPrecedence (-1, -1, -1, 4) == 4,
               "Nothing fired, history=4 → 4");
        check (resolveTriggerPrecedence (-1, -1, -1, 0) == 0,
               "Nothing fired, history=0 → 0");

        // Out-of-range candidates clamp to 0..7.
        check (resolveTriggerPrecedence (99, -1, -1, 0) == 7,
               "KS=99 (oob) → clamped to 7");
        check (resolveTriggerPrecedence (-1, -1, -1, 99) == 7,
               "history=99 (oob) → clamped to 7");
    }

    // ----------------------------------------------------------------
    // 8. End-to-end scenario — three triggers fire in the same block,
    //    KS wins; second block fires only CC, CC wins; third block
    //    fires nothing, history (CC's value) persists.
    //    This mirrors the processBlock loop:
    //      candidates start at -1, get overwritten if the corresponding
    //      trigger fires, precedence resolver decides what to commit.
    // ----------------------------------------------------------------
    {
        const auto cc = defaultCcMapping (4);
        const auto pc = defaultPcMapping();

        int historyTech = 0;

        // Block 1: KS note absorbed → ksTech=2; CC value 96 → tech 3;
        // PC# 5 → no match → pcTech=-1.
        {
            const int ksTech = 2;
            const int ccTech = resolveCcTechnique (96, cc);     // = 3
            const int pcTech = resolvePcTechnique (5,  pc);     // = -1
            const int newTech = resolveTriggerPrecedence (ksTech, ccTech, pcTech, historyTech);
            check (newTech == 2, "Block 1: KS=2 wins over CC=3 and PC miss");
            historyTech = newTech;
        }
        // Block 2: only CC fires (no KS, no PC).
        {
            const int ksTech = -1;
            const int ccTech = resolveCcTechnique (16, cc);     // = 0
            const int pcTech = -1;
            const int newTech = resolveTriggerPrecedence (ksTech, ccTech, pcTech, historyTech);
            check (newTech == 0, "Block 2: CC=0 wins over history=2");
            historyTech = newTech;
        }
        // Block 3: nothing fires, history persists.
        {
            const int newTech = resolveTriggerPrecedence (-1, -1, -1, historyTech);
            check (newTech == 0, "Block 3: nothing fired, history=0 persists");
        }
        // Block 4: only PC fires.
        {
            const int pcTech = resolvePcTechnique (4, pc);      // = 4
            const int newTech = resolveTriggerPrecedence (-1, -1, pcTech, historyTech);
            check (newTech == 4, "Block 4: PC=4 wins over history=0");
        }
    }

    std::cout << "== cc_pc_trigger_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
