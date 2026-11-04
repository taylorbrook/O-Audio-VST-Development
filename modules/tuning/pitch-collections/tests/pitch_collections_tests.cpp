/*
   This file is part of the Ouaricon pitch-collections module.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
/*
  ==============================================================================

    pitch-collections tests. Exit 0 iff every check passes.

      U1  builtIns structure + digests equal to v3.5.1 (all 32 tables)
      U2  edo equivalences, bitwise (edo 19 == Edo19, edo 13 3/1 == BP, ...)
      U3  edo errors and their order (badEdo, badPitch + token, badPeriod)
      U4  offending token; 3-arg and 4-arg overloads agree
      U5  v3.5.1 regression corpus (text + Scala), captured before the move
      U6  toScala round-trip: tableFromScala (toScala (n, t)) == tableFromText (t)

    Build: see tests/CMakeLists.txt.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <cstdio>
#include <cstring>

#if defined (__clang__)
 #pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined (__GNUC__)
 #pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include "PitchCollections.h"

namespace PC = ouaricon::tuning::PitchCollections;

struct ExpectedText  { bool ok; const char* error; int count; uint64_t digest; };
struct ExpectedScala { bool ok; const char* error; int count; uint64_t digest; const char* asText; const char* description; };

#include "corpus.inc"
#include "expected_v351.inc"

static int failures = 0, checks = 0;

static void check (bool cond, const juce::String& what)
{
    ++checks;
    if (! cond) { ++failures; std::printf ("  FAIL  %s\n", what.toRawUTF8()); }
}

static uint64_t digest (const PC::Table& t)
{
    uint64_t h = 1469598103934665603ull;
    auto mix = [&] (uint32_t v) { for (int b = 0; b < 4; ++b) { h ^= (v >> (8 * b)) & 0xffu; h *= 1099511628211ull; } };
    mix ((uint32_t) t.count);
    uint32_t pb; std::memcpy (&pb, &t.period, 4); mix (pb);
    for (int i = 0; i < t.count; ++i) { uint32_t cb; std::memcpy (&cb, &t.cents[(size_t) i], 4); mix (cb); }
    return h;
}

static bool bitwiseEqual (const PC::Table& a, const PC::Table& b)
{
    if (a.count != b.count || std::memcmp (&a.period, &b.period, sizeof (float)) != 0) return false;
    return std::memcmp (a.cents.data(), b.cents.data(), sizeof (float) * (size_t) a.count) == 0;
}

static bool parse (const juce::String& text, PC::Table& t, juce::String& err, juce::String& tok)
{
    err = {}; tok = {};
    return PC::tableFromText (text, t, err, tok);
}

static void u1()
{
    std::printf ("U1 builtIns\n");
    const auto& b = PC::builtIns();
    check (b.size() == 32 && PC::kCount == 32 && PC::Custom == 31 && PC::Free == 5, "32 Ids, Free 5, Custom 31");
    check (PC::names().size() == PC::kCount, "names() has kCount entries");
    for (int i = 0; i < PC::kCount; ++i)
    {
        const auto& t = b[(size_t) i];
        // Not "ascending": v3.5.1's 7-Limit Just lists 12/7 after 7/4 (frozen data;
        // the GS engine sorts its active table itself). Degree 0 = 0, all inside.
        bool inside = t.cents[0] == 0.0f && t.count >= 1 && t.count <= PC::kMaxPitches;
        for (int k = 1; k < t.count; ++k) inside = inside && t.cents[(size_t) k] > 0.0f && t.cents[(size_t) k] < t.period;
        check (inside, "table " + juce::String (i) + " starts at 0, degrees inside the period");
        check (digest (t) == kBuiltInDigests[i], "table " + juce::String (i) + " digest == v3.5.1");
    }
    check (b[PC::BohlenPierce].period == PC::ratioToCents (3, 1), "BP period == ratioToCents (3, 1)");
}

static void u2()
{
    std::printf ("U2 edo equivalences\n");
    const auto& b = PC::builtIns();
    PC::Table t; juce::String e, tok;
    for (auto* s : { "edo 19", "19-edo", "EDO 19", "edo19", "19 edo", "  Edo 19  ", "19edo" })
        check (parse (s, t, e, tok) && bitwiseEqual (t, b[PC::Edo19]), juce::String ("'") + s + "' == Edo19");
    check (parse ("edo 12", t, e, tok) && bitwiseEqual (t, b[PC::Chromatic]), "edo 12 == Chromatic");
    PC::Table ref;
    check (parse ("100 200 300 400 500 600 700 800 900 1000 1100 1200", ref, e, tok) && bitwiseEqual (ref, b[PC::Chromatic]),
           "typed 12-EDO cents == Chromatic");
    check (parse ("edo 13 3/1", t, e, tok) && bitwiseEqual (t, b[PC::BohlenPierce]), "edo 13 3/1 == BohlenPierce");
    check (parse ("edo 53", t, e, tok) && bitwiseEqual (t, b[PC::Edo53]), "edo 53 == Edo53");
    check (parse ("edo 64", t, e, tok) && t.count == 64, "edo 64 -> 64 degrees");
    check (parse ("edo 1", t, e, tok) && t.count == 1 && t.period == 1200.0f, "edo 1 -> one degree");
    check (parse ("edo 5 1200", t, e, tok) && bitwiseEqual (t, b[PC::Slendro]), "edo 5 1200 == Slendro");
    juce::String err = "stale";
    PC::tableFromText ("edo 19", t, err);
    check (err == "stale", "edo success leaves error untouched (token-path parity)");
}

static void expectError (const char* text, const char* code, const char* token)
{
    PC::Table t; juce::String e, tok;
    const bool ok = parse (text, t, e, tok);
    check (! ok && e == code && tok == token,
           juce::String ("'") + text + "' -> " + code + " [" + token + "], got " + e + " [" + tok + "]");
}

static void u3()
{
    std::printf ("U3 edo errors\n");
    expectError ("edo 0", "badEdo", "");
    expectError ("edo 65", "badEdo", "");
    expectError ("edo 99999999999", "badEdo", "");
    expectError ("edo 0019", "badEdo", "");
    expectError ("edo 19 abc", "badPitch", "abc");
    expectError ("edo 19 5000", "badPeriod", "");
    expectError ("edo 0 5000", "badPeriod", "");     // order: period before N
    expectError ("edo 0 x", "badPitch", "x");        // order: period token first
    expectError ("edo", "badPitch", "edo");
    expectError ("edo 12.5", "badPitch", "edo");
    expectError ("edo 19, 1200", "badPitch", "edo");
}

static void u4()
{
    std::printf ("U4 offending token\n");
    expectError ("100 2x0 1200", "badPitch", "2x0");
    expectError ("5/4 0/1 2/1", "badPitch", "0/1");
    for (auto* s : kTextCorpus)
    {
        PC::Table a, b; juce::String ea, eb, tok;
        const bool oa = PC::tableFromText (s, a, ea);
        const bool ob = PC::tableFromText (s, b, eb, tok);
        check (oa == ob && ea == eb && (! oa || bitwiseEqual (a, b)), juce::String ("3-arg == 4-arg: '") + s + "'");
    }
}

static bool isEdoShaped (const juce::String& s)
{
    PC::Table t; juce::String e, b;
    return PC::detail::tryEdo (s, t, e, b) >= 0;
}

static void u5()
{
    std::printf ("U5 v3.5.1 corpus\n");
    constexpr auto nText = sizeof (kTextCorpus) / sizeof (kTextCorpus[0]);
    static_assert (nText == sizeof (kTextExpected) / sizeof (kTextExpected[0]), "corpus / expected size");
    int edoShaped = 0;
    for (size_t i = 0; i < nText; ++i)
    {
        const auto s = juce::String (juce::CharPointer_UTF8 (kTextCorpus[i]));
        const auto& x = kTextExpected[i];
        PC::Table t; juce::String e;
        const bool ok = PC::tableFromText (s, t, e);
        if (isEdoShaped (s))
        {
            // The only inputs whose result may change: v3.5.1 rejected all of them.
            ++edoShaped;
            check (! x.ok && juce::String (x.error) == "badPitch", "edo-shaped input failed in v3.5.1: '" + s + "'");
            continue;
        }
        check (ok == x.ok && e == x.error && (! ok || (t.count == x.count && digest (t) == x.digest)),
               "text[" + juce::String ((int) i) + "] '" + s.replace ("\n", "\\n") + "' == v3.5.1");
    }
    check (edoShaped == 2, "exactly 2 corpus inputs are edo-shaped (edo 19, 19-edo)");

    constexpr auto nScl = sizeof (kScalaCorpus) / sizeof (kScalaCorpus[0]);
    static_assert (nScl == sizeof (kScalaExpected) / sizeof (kScalaExpected[0]), "scala corpus / expected size");
    for (size_t i = 0; i < nScl; ++i)
    {
        const auto& x = kScalaExpected[i];
        PC::Table t; juce::String e, asText, desc;
        const bool ok = PC::tableFromScala (juce::String (juce::CharPointer_UTF8 (kScalaCorpus[i])), t, asText, desc, e);
        check (ok == x.ok && e == x.error
               && (! ok || (t.count == x.count && digest (t) == x.digest && asText == x.asText && desc == x.description)),
               "scala[" + juce::String ((int) i) + "] == v3.5.1");
    }
}

static void roundTrip (const juce::String& name, const juce::String& text)
{
    PC::Table a, b; juce::String e, asText, desc;
    const bool okA = PC::tableFromText (text, a, e);
    const auto scl = PC::toScala (name, text);
    if (! okA) { check (scl.isEmpty(), "toScala of unparsable text is empty: '" + text + "'"); return; }
    const bool okB = PC::tableFromScala (scl, b, asText, desc, e);
    check (okB && bitwiseEqual (a, b) && desc == PC::scalaDescription (name),
           "round-trip '" + text.substring (0, 40).replace ("\n", "\\n") + "' name '" + name.replace ("\n", "\\n") + "'"
           + (okB ? juce::String() : " (" + e + ")"));
}

static void u6()
{
    std::printf ("U6 toScala round-trip\n");
    for (auto* s : kTextCorpus)
        roundTrip ("Corpus", juce::String (juce::CharPointer_UTF8 (s)));
    for (auto* s : { "100 1300 2400", "-100 1200", "5/2 3/2 2/1", "100 100 100.0 1200", "386.3137 701.955 1200",
                     "0.015 1199.99 1200", "1/1 9/8 2/1", "edo 19", "edo 13 3/1", "edo 7 1200", "edo 31 1901.955",
                     "7/4 3/2 1901.955", "edo 64", "33.33333333 1200", "1 2 3 4800", "4799.999 4800" })
        roundTrip ("x", s);
    roundTrip ("", "5/4 3/2 2/1");
    roundTrip ("!bang\nnew line", "5/4 3/2 2/1");
    roundTrip ("  ! ! ", "100 1200");
    roundTrip (juce::String (juce::CharPointer_UTF8 ("Échelle 音阶 / a:b")), "edo 22");

    // Spot-check the text: bare-integer cents never become ratios.
    const auto scl = PC::toScala ("t", "100 1200");
    check (scl.contains ("\n 100.0\n") && scl.contains ("\n 1200.0\n") && ! scl.contains ("/1\n"), "bare integers written as N.0");
    check (PC::toScala ("t", "edo 12").contains ("\n 2/1\n"), "edo default period written as 2/1");
    check (PC::toScala ("t", "abc").isEmpty(), "unparsable -> empty");
    check (PC::scalaDescription ("") == "Ouaricon custom scale", "empty name -> default description");
}

int main()
{
    u1(); u2(); u3(); u4(); u5(); u6();
    std::printf ("\n%d / %d checks passed%s\n", checks - failures, checks, failures ? "  -- FAILURES" : "  -- ALL PASS");
    return failures == 0 ? 0 : 1;
}
