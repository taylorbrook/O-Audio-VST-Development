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

    loader_robustness_check.cpp
    O-MicrotonalSampler — v1.23.6 loader/parser robustness regression.

    Manual run: ninja O-MicrotonalSampler_LoaderRobustnessCheck && \
                ./build/.../O-MicrotonalSampler_LoaderRobustnessCheck
    Exit code = number of failed assertions (0 = all pass).

    Pins the pure, header-extracted logic from the 2026-06-30 loading-parsing
    review (REVIEW-loading-parsing.md):

      WR-02  oms::isAcceptableSampleLength — int64 header length is validated
             against kMaxSamplesPerFile BEFORE the int narrowing that used to
             wrap a corrupt/huge length to a silent under-read or false reject.
      WR-03/ oms::resampleOutLength — ceil output length (the source buffer is
      IN-05  guard-padded by one sample in the loader) is always >= 1, so the
             removed `outNumSamples < 1` clamp really was dead.
      WR-04  FilenameParser split-form RR no longer double-consumes the bare-
             integer note token (`take_60.wav` → note 60, RR -1, not RR 59).
      IN-02  oms::isAmbiguousRrGroup — mixed explicit/no-token groups and
             duplicate explicit RR indices now flag the confirmation modal.
      IN-03  FilenameParser rejects BARE single-letter dynamics ("p"/"f") in
             the PRE-note tier (`F-C3` = Flute is no longer forte→layer 3),
             while keeping mp/mf pre-note and bare p/f post-note.

    WR-01 (per-file try/catch + ceiling guard) and the CR-01 cooperative
    cancellation are integration-level (need a real/corrupt reader) and are
    covered by build + auval + pluginval editor cycles, not this console test.

  ==============================================================================
*/

#include <juce_core/juce_core.h>

#include "../FilenameParser.h"
#include "../LoaderSupport.h"

#include <iostream>
#include <limits>
#include <string>
#include <vector>

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

    // Assert the parser resolves (midiNote, velLayer, rrIndex) as expected.
    void checkParse (const char* input, int expMidi, int expVel, int expRr)
    {
        auto r = FilenameParser::parse (juce::String (input));
        if (! r.has_value())
        {
            std::cout << "  FAIL: '" << input << "' did not parse (expected midi="
                      << expMidi << ")\n";
            ++failed;
            return;
        }

        const bool ok = r->midiNote == expMidi
                     && r->velLayer == expVel
                     && r->rrIndex  == expRr;
        if (ok)
            std::cout << "  PASS: '" << input << "' → midi=" << expMidi
                      << " vel=" << expVel << " rr=" << expRr << "\n";
        else
        {
            std::cout << "  FAIL: '" << input << "' → got midi=" << r->midiNote
                      << " vel=" << r->velLayer << " rr=" << r->rrIndex
                      << "  expected midi=" << expMidi << " vel=" << expVel
                      << " rr=" << expRr << "\n";
            ++failed;
        }
    }
}

int main()
{
    std::cout << "== loader_robustness_check ==\n";

    // ------------------------------------------------------------------
    // WR-02 — untrusted header length validation (isAcceptableSampleLength).
    // ------------------------------------------------------------------
    std::cout << "-- WR-02: header length ceiling --\n";
    check (! oms::isAcceptableSampleLength (0),  "len 0 rejected");
    check (! oms::isAcceptableSampleLength (-1), "negative len rejected (int64→int wrap case)");
    check (  oms::isAcceptableSampleLength (1),  "len 1 accepted");
    check (  oms::isAcceptableSampleLength (48000 * 60),        "1 min @48k accepted");
    check (  oms::isAcceptableSampleLength (oms::kMaxSamplesPerFile), "exactly the ceiling accepted");
    check (! oms::isAcceptableSampleLength (oms::kMaxSamplesPerFile + 1), "ceiling+1 rejected");
    check (! oms::isAcceptableSampleLength ((juce::int64) 1 << 40), "1<<40 (huge/corrupt) rejected");
    check (! oms::isAcceptableSampleLength (std::numeric_limits<juce::int64>::max()),
           "int64 max rejected (would wrap negative on (int) cast)");
    // The ceiling is comfortably below INT_MAX so the post-validation (int) cast is lossless.
    check (oms::kMaxSamplesPerFile < (juce::int64) std::numeric_limits<int>::max(),
           "ceiling < INT_MAX → narrowing after validation is safe");

    // ------------------------------------------------------------------
    // WR-03 / IN-05 — resampler output length is ceil and always >= 1.
    // ------------------------------------------------------------------
    std::cout << "-- WR-03/IN-05: resample output length --\n";
    check (oms::resampleOutLength (100, 1.0) == 100, "ratio 1.0 → identity length");
    check (oms::resampleOutLength (3, 2.0)   == 2,   "downsample 3/2.0 → ceil 1.5 = 2");
    check (oms::resampleOutLength (1, 2.0)   == 1,   "tiny 1-sample downsample → 1 (never 0 — clamp was dead)");
    check (oms::resampleOutLength (1, 4.0)   == 1,   "even tinier ratio still → 1");
    // 48 kHz → 44.1 kHz: ratio 48000/44100; 48000 input frames → 44100 output frames exactly.
    check (oms::resampleOutLength (48000, 48000.0 / 44100.0) == 44100,
           "48k→44.1k length maps 48000 → 44100");
    // 44.1 kHz → 48 kHz (upsample): more output than input.
    check (oms::resampleOutLength (44100, 44100.0 / 48000.0) == 48000,
           "44.1k→48k length maps 44100 → 48000");

    // ------------------------------------------------------------------
    // WR-04 — split-form RR must not re-consume the bare-integer note token.
    // ------------------------------------------------------------------
    std::cout << "-- WR-04: RR split-form note-token exclusion --\n";
    // THE BUG: "take_60" tokenises to ["take","60"]; "60" is the MIDI note, and the
    // old scan also read parseAsRrIndex("take60") → rr 59. Now RR stays -1.
    checkParse ("take_60",       60, 0, -1);
    checkParse ("take_60.wav",   60, 0, -1);   // extension already stripped by caller, but harmless
    checkParse ("rr_60",         60, 0, -1);   // same double-consume via "rr"
    checkParse ("tk_60",         60, 0, -1);
    // A DISTINCT note token keeps the explicit take index (regression guard).
    checkParse ("C3_take_60",    60, 0, 59);   // C3 is the note; take 60 → rr 59
    checkParse ("60_take_2",     60, 0,  1);   // bare-int note 60 + explicit take2
    // Established RR forms must all still resolve (no regression).
    checkParse ("Piano_C3_take_1", 60, 0, 0);
    checkParse ("Piano_C3_take_2", 60, 0, 1);
    checkParse ("Trumpet_F#3_rr_2", 66, 0, 1);
    checkParse ("RR2_C3_v1",       60, 0, 1);   // pre-note glued RR
    checkParse ("C3 take 12",      60, 0, 11);  // space-separated split RR
    checkParse ("C3_v1_rr1",       60, 0, 0);

    // ------------------------------------------------------------------
    // IN-03 — bare single-letter dynamics rejected PRE-note, kept POST-note.
    // ------------------------------------------------------------------
    std::cout << "-- IN-03: pre-note single-letter dynamics --\n";
    checkParse ("F-C3",  60, 0, -1);   // Flute abbreviation — NOT forte→layer 3
    checkParse ("f_C3",  60, 0, -1);   // lower-case pre-note bare f rejected
    checkParse ("P_C3",  60, 0, -1);   // Piano abbreviation — NOT piano→layer 0 (default anyway)
    // Two-char dynamics pre-note STILL honoured (the dominant orchestral convention).
    checkParse ("mp_C3", 60, 1, -1);
    checkParse ("mf-C3", 60, 2, -1);
    checkParse ("vln_norm_mf-A#2-V127-JXRO", 58, 2, -1);  // user's real library filename
    // Explicit velocity forms pre-note still win over the skipped letter.
    checkParse ("f_v2_C3", 60, 1, -1);   // bare f skipped, explicit v2 → layer 1
    checkParse ("L3_C3",   60, 2, -1);
    // Bare single-letter dynamics remain valid POST-note.
    checkParse ("C3_f", 60, 3, -1);
    checkParse ("C3_p", 60, 0, -1);
    checkParse ("C3_mf", 60, 2, -1);

    // ------------------------------------------------------------------
    // IN-02 — round-robin group ambiguity classification.
    // ------------------------------------------------------------------
    std::cout << "-- IN-02: RR group ambiguity --\n";
    check (! oms::isAmbiguousRrGroup ({}),          "empty group → not ambiguous");
    check (! oms::isAmbiguousRrGroup ({ -1 }),      "single no-token file → not ambiguous");
    check (! oms::isAmbiguousRrGroup ({ 0 }),       "single explicit-RR file → not ambiguous");
    check (! oms::isAmbiguousRrGroup ({ 0, 1, 2 }), "distinct explicit RR set → intentional, not ambiguous");
    check (  oms::isAmbiguousRrGroup ({ -1, -1 }),  "two no-token files → ambiguous (original rule)");
    check (  oms::isAmbiguousRrGroup ({ 0, -1 }),   "mixed explicit + no-token → ambiguous (IN-02)");
    check (  oms::isAmbiguousRrGroup ({ 0, 1, -1 }),"explicit set + stray no-token → ambiguous (IN-02)");
    check (  oms::isAmbiguousRrGroup ({ 0, 0 }),    "duplicate explicit RR index → ambiguous (IN-02)");
    check (  oms::isAmbiguousRrGroup ({ 2, 5, 5 }), "duplicate among distinct explicit → ambiguous (IN-02)");

    std::cout << "== loader_robustness_check: "
              << (failed == 0 ? "ALL PASS" : "FAIL")
              << " (" << failed << " failures) ==\n";
    return failed;
}
