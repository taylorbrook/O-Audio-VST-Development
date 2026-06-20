/*
  ==============================================================================

    technique_parse_check.cpp
    O-MicrotonalSampler — v1.14.0 technique-token filename parse tests.

    Manual run: ninja O-MicrotonalSampler_TechniqueParseCheck && ./...
    Exit code = number of failed assertions (0 = all pass).

    What this exercises
    -------------------
    The new `parseAsTechnique` helper inside FilenameParser.cpp + the
    `techniqueIndex` field added to `ParsedName`. We must NOT regress any
    of the existing parse semantics (note / velocity / RR), so the cases
    below mix a technique token with the established conventions.

    Key invariants:

      1. Each of the 10 default tokens (`ord`, `sp`, `st`, `sv`, `cs`,
         `pizz`, `harm`, `mart`, `trem`, `flaut`) maps to its slot index.
      2. Tokens MUST appear as their own delimited token — substring
         matches like `chord_suspended` (would otherwise hit `ord` or
         `cs`) are rejected, returning techniqueIndex = -1.
      3. Case-insensitive: `ORD`, `Ord`, `oRd` all → 0.
      4. No technique token → techniqueIndex = -1 (caller defaults to 0).
      5. Coexists with note / velocity / RR tokens in any order.

  ==============================================================================
*/

#include <juce_core/juce_core.h>

#include "../FilenameParser.h"

#include <iostream>
#include <optional>

namespace
{
    int failed = 0;

    void assertParse (const char* input,
                      int expectedMidi,
                      int expectedVel,
                      int expectedRr,
                      int expectedTech)
    {
        auto result = FilenameParser::parse (juce::String (input));
        if (! result.has_value())
        {
            std::cout << "  FAIL: '" << input << "' did not parse — expected midi=" << expectedMidi << "\n";
            ++failed;
            return;
        }

        const bool ok = result->midiNote       == expectedMidi
                     && result->velLayer       == expectedVel
                     && result->rrIndex        == expectedRr
                     && result->techniqueIndex == expectedTech;

        if (ok)
        {
            std::cout << "  PASS: '" << input << "'  midi=" << expectedMidi
                      << " vel=" << expectedVel
                      << " rr="  << expectedRr
                      << " tech=" << expectedTech << "\n";
        }
        else
        {
            std::cout << "  FAIL: '" << input << "'  got midi=" << result->midiNote
                      << " vel=" << result->velLayer
                      << " rr="  << result->rrIndex
                      << " tech=" << result->techniqueIndex
                      << "  expected midi=" << expectedMidi
                      << " vel=" << expectedVel
                      << " rr="  << expectedRr
                      << " tech=" << expectedTech << "\n";
            ++failed;
        }
    }
} // namespace

int main()
{
    std::cout << "== technique_parse_check ==\n";

    // 1. Each token to its slot. Note tokens chosen so velocity+RR=defaults.
    assertParse ("C3_v1_ord",   60, 0, -1, 0);
    assertParse ("C3_v1_sp",    60, 0, -1, 1);
    assertParse ("C3_v1_st",    60, 0, -1, 2);
    assertParse ("C3_v1_sv",    60, 0, -1, 3);
    assertParse ("C3_v1_cs",    60, 0, -1, 4);
    assertParse ("C3_v1_pizz",  60, 0, -1, 5);
    assertParse ("C3_v1_harm",  60, 0, -1, 6);
    assertParse ("C3_v1_mart",  60, 0, -1, 7);
    assertParse ("C3_v1_trem",  60, 0, -1, 8);
    assertParse ("C3_v1_flaut", 60, 0, -1, 9);

    // 2. Substring rejection — these token boundaries would hit prefix forms
    //    in a startsWith-based scanner; exact-match parser must reject.
    assertParse ("chord_suspended_C3", 60, 0, -1, -1); // 'chord' / 'suspended' — no exact match
    assertParse ("Stradivari_C3",      60, 0, -1, -1); // 'Stradivari' contains 'st' but isn't 'st'
    assertParse ("Spitfire_C3",        60, 0, -1, -1); // 'Spitfire' contains 'sp' but isn't 'sp'
    assertParse ("orchestra_C3",       60, 0, -1, -1); // 'orchestra' contains 'orch'/etc. — none exact

    // 3. Case-insensitive
    assertParse ("C3_ORD",   60, 0, -1, 0);
    assertParse ("C3_Sp",    60, 0, -1, 1);
    assertParse ("C3_pIzZ",  60, 0, -1, 5);

    // 4. No technique token → -1
    assertParse ("C3_v2",          60, 1, -1, -1);
    assertParse ("Piano_C3_take_2", 60, 0,  1, -1);

    // 5. Coexists with velocity + RR
    assertParse ("C3_v2_pizz_rr3",  60, 1, 2, 5);
    assertParse ("Piano_C3_sp_take_2", 60, 0, 1, 1);
    assertParse ("D#3_mart_v3",     63, 2, -1, 7);

    // 6. Position-independent: technique can appear before or after the note.
    assertParse ("ord_C3_v1",  60, 0, -1, 0);
    assertParse ("sp-D3-v2",   62, 1, -1, 1);

    // 7. Long-form single-token aliases (the v1.14.0 follow-up). Real-world
    //    sample libraries rarely use the two-letter shorthand.
    assertParse ("Violin_C3_pizzicato",       60, 0, -1, 5);
    assertParse ("Violin_C3_v2_harmonic",     60, 1, -1, 6);
    assertParse ("Violin_C3_v3_harmonics",    60, 2, -1, 6);
    assertParse ("Trumpet_C3_muted",          60, 0, -1, 4);
    assertParse ("Strings_C3_tremolo_v2",     60, 1, -1, 8);
    assertParse ("Violin_C3_flautando",       60, 0, -1, 9);
    assertParse ("Violin_C3_flautato",        60, 0, -1, 9);
    assertParse ("Violin_C3_martele",         60, 0, -1, 7);
    assertParse ("Violin_C3_martellato",      60, 0, -1, 7);
    assertParse ("Violin_C3_ordinario",       60, 0, -1, 0);
    assertParse ("Violin_C3_ordinary",        60, 0, -1, 0);
    // v1.17.2: norm/normale/normal aliases for slot 0 (ord). Dynamic token
    // pre-note is honoured (v1.17.2 PARSE-DYN), so vel resolves correctly too.
    assertParse ("Violin_norm_mf_C3",         60, 2, -1, 0);
    assertParse ("Cello_normale_C3_v2",       60, 1, -1, 0);
    assertParse ("Viola_C3_normal",           60, 0, -1, 0);
    // The user's exact library filename: A#2 = midi 58, pre-note "mf" → layer 2,
    // "norm" → slot 0, and the "V127" recorded-velocity token is correctly
    // IGNORED (3 digits, not a v1–4 layer tag).
    assertParse ("vln_norm_mf-A#2-V127-JXRO", 58, 2, -1, 0);
    assertParse ("Violin_C3_sulpont",         60, 0, -1, 1);
    assertParse ("Violin_C3_sulponticello",   60, 0, -1, 1);
    assertParse ("Violin_C3_sultasto",        60, 0, -1, 2);
    assertParse ("Violin_C3_senzavib",        60, 0, -1, 3);
    assertParse ("Violin_C3_senzavibrato",    60, 0, -1, 3);
    assertParse ("Violin_C3_nonvib",          60, 0, -1, 3);
    assertParse ("Violin_C3_nonvibrato",      60, 0, -1, 3);
    assertParse ("Violin_C3_consord",         60, 0, -1, 4);
    assertParse ("Violin_C3_consordino",      60, 0, -1, 4);
    // Case-insensitive on long forms too.
    assertParse ("Violin_C3_PIZZICATO",       60, 0, -1, 5);
    assertParse ("Violin_C3_Sul_Ponticello",  60, 0, -1, 1);  // pair form, see below

    // 8. Adjacent-pair forms — the canonical orchestral filename style.
    //    Tokeniser splits "sul_pont" into ["sul","pont"]; the pair scan
    //    catches it.
    assertParse ("Violin_C3_sul_pont",          60, 0, -1, 1);
    assertParse ("Violin_C3_sul_ponticello",    60, 0, -1, 1);
    assertParse ("Violin_C3_sul_tasto",         60, 0, -1, 2);
    assertParse ("Violin_C3_senza_vib",         60, 0, -1, 3);
    assertParse ("Violin_C3_senza_vibrato",     60, 0, -1, 3);
    assertParse ("Violin_C3_non_vib",           60, 0, -1, 3);
    assertParse ("Violin_C3_non_vibrato",       60, 0, -1, 3);
    assertParse ("Violin_C3_con_sord",          60, 0, -1, 4);
    assertParse ("Violin_C3_con_sordino",       60, 0, -1, 4);
    // Pair form coexists with note / vel / RR.
    assertParse ("Spitfire_Violin_sul_pont_C3_v1_rr2", 60, 0, 1, 1);
    // Dash separator path — same tokeniser → same pair detection.
    assertParse ("Violin-C3-sul-pont",          60, 0, -1, 1);

    // 9. Pair-form leads (sul / senza / non / con) MUST NOT match alone.
    //    Without their suffix we get tech=-1 (caller defaults to slot 0).
    assertParse ("Violin_C3_sul_x",             60, 0, -1, -1);  // sul without pont/ponticello/tasto
    assertParse ("Violin_C3_con_C3_other",      60, 0, -1, -1);  // con without sord/sordino
    assertParse ("Violin_C3_non_extra",         60, 0, -1, -1);  // non without vib/vibrato
    // Lone "sul" / "senza" / "non" / "con" don't trigger the single-token
    // path either — we don't include them in parseAsTechnique.
    assertParse ("sul_C3_v1",                   60, 0, -1, -1);

    // 10. Long-form rejection — these should NOT collide with library names.
    //     "violin" alone (no suffix) doesn't match any technique.
    assertParse ("violin_strings_C3",           60, 0, -1, -1);

    std::cout << "== technique_parse_check: " << (failed == 0 ? "ALL PASS" : "FAIL") << " (" << failed << " failures) ==\n";
    return failed;
}
