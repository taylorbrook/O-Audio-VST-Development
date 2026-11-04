/*
   This file is part of the Ouaricon pitch-collections module.
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
#pragma once

// ============================================================================
// pitch-collections 1.0.0 — cents-based pitch collections, text / Scala
// parsing and Scala export. Header-only, juce_core only, C++17-clean.
//
// Moved VERBATIM from O-GrainScatter 3.5.1 Source/dsp/PitchEngine.h
// (namespace PitchCollections, lines 41-305). Every pre-existing body is
// byte-identical; GS 3.6.0 aliases `namespace PitchCollections =
// ouaricon::tuning::PitchCollections;` and its renders null against 3.5.1.
//
// 1.0.0 additions (appended, existing results unchanged):
//   - tableFromText (text, out, error, badToken): reports the offending token
//     (the 3-arg form is now a thin wrapper over it)
//   - "edo N [period]" shorthand in tableFromText (error "badEdo")
//   - toScala (name, text): a .scl body that tableFromScala reads back to a
//     bitwise-identical Table
//
// CONTRACT: the Id enum order and names() strings are FROZEN — GS stores the
// choice index in saved sessions and uses names() as its host strings.
// Message thread only, except builtIns() reads after a warm-up call.
// ============================================================================

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <regex>
#include <string>
#include <utility>

namespace ouaricon::tuning::PitchCollections
{
    static constexpr int kMaxPitches = 64;

    // Index order IS the "scale" AudioParameterChoice order. 0-4 are the
    // v1-v2 scales and must keep their indices (saved sessions store the
    // choice index). New entries are only ever appended before Custom.
    enum Id
    {
        Chromatic = 0, Major, Minor, Pentatonic, WholeTone,
        Free,
        Dorian, Phrygian, Lydian, Mixolydian, Locrian,
        HarmonicMinor, MelodicMinor, MinorPentatonic, Blues,
        Octatonic, Augmented, FifthsOctaves, Octaves,
        Edo19, Edo22, Edo24, Edo31, Edo53,
        JustFiveLimit, JustSevenLimit, Harmonics8to16,
        BohlenPierce, Slendro, Pelog, MaqamRast,
        Custom,
        kCount
    };

    // Host-facing names (AudioParameterChoice). The UI shows localized names.
    inline juce::StringArray names()
    {
        return { "Chromatic", "Major", "Minor", "Pentatonic", "Whole Tone",
                 "Free (unquantized)",
                 "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian",
                 "Harmonic Minor", "Melodic Minor", "Minor Pentatonic", "Blues",
                 "Octatonic", "Augmented", "Fifths + Octaves", "Octaves",
                 "19-EDO", "22-EDO", "24-EDO", "31-EDO", "53-EDO",
                 "5-Limit Just", "7-Limit Just", "Harmonics 8-16",
                 "Bohlen-Pierce", "Slendro", "Pelog", "Maqam Rast",
                 "Custom" };
    }

    struct Table
    {
        std::array<float, kMaxPitches> cents {};   // ascending, cents[0] == 0
        int   count  = 1;
        float period = 1200.0f;
    };

    inline float ratioToCents (double num, double den)
    {
        return static_cast<float> (1200.0 * std::log2 (num / den));
    }

    inline Table fromCents (std::initializer_list<float> c, float period = 1200.0f)
    {
        Table t;
        t.count = 0;
        for (float v : c)
            if (t.count < kMaxPitches)
                t.cents[static_cast<size_t> (t.count++)] = v;
        t.period = period;
        return t;
    }

    inline Table equalDivision (int n, float period = 1200.0f)
    {
        Table t;
        t.count = juce::jlimit (1, kMaxPitches, n);
        for (int k = 0; k < t.count; ++k)
            t.cents[static_cast<size_t> (k)] = period * static_cast<float> (k) / static_cast<float> (n);
        t.period = period;
        return t;
    }

    inline Table fromRatios (std::initializer_list<std::pair<int, int>> r)
    {
        Table t;
        t.count = 0;
        for (auto [n, d] : r)
            if (t.count < kMaxPitches)
                t.cents[static_cast<size_t> (t.count++)] = ratioToCents (n, d);
        return t;
    }

    /** Built-in tables. Built ONCE, off the audio thread (std::log2 is not
        constexpr). Custom gets a 12-EDO placeholder here; the live custom table
        is owned by the processor. */
    inline const std::array<Table, kCount>& builtIns()
    {
        static const std::array<Table, kCount> tables = []
        {
            std::array<Table, kCount> t {};
            t[Chromatic]       = equalDivision (12);
            t[Major]           = fromCents ({ 0, 200, 400, 500, 700, 900, 1100 });
            t[Minor]           = fromCents ({ 0, 200, 300, 500, 700, 800, 1000 });
            t[Pentatonic]      = fromCents ({ 0, 200, 400, 700, 900 });
            t[WholeTone]       = equalDivision (6);
            t[Free]            = equalDivision (12);   // ladders walk semitones; Random is continuous
            t[Dorian]          = fromCents ({ 0, 200, 300, 500, 700, 900, 1000 });
            t[Phrygian]        = fromCents ({ 0, 100, 300, 500, 700, 800, 1000 });
            t[Lydian]          = fromCents ({ 0, 200, 400, 600, 700, 900, 1100 });
            t[Mixolydian]      = fromCents ({ 0, 200, 400, 500, 700, 900, 1000 });
            t[Locrian]         = fromCents ({ 0, 100, 300, 500, 600, 800, 1000 });
            t[HarmonicMinor]   = fromCents ({ 0, 200, 300, 500, 700, 800, 1100 });
            t[MelodicMinor]    = fromCents ({ 0, 200, 300, 500, 700, 900, 1100 });
            t[MinorPentatonic] = fromCents ({ 0, 300, 500, 700, 1000 });
            t[Blues]           = fromCents ({ 0, 300, 500, 600, 700, 1000 });
            t[Octatonic]       = fromCents ({ 0, 100, 300, 400, 600, 700, 900, 1000 });
            t[Augmented]       = fromCents ({ 0, 300, 400, 700, 800, 1100 });
            t[FifthsOctaves]   = fromCents ({ 0, 700 });
            t[Octaves]         = fromCents ({ 0 });
            t[Edo19]           = equalDivision (19);
            t[Edo22]           = equalDivision (22);
            t[Edo24]           = equalDivision (24);
            t[Edo31]           = equalDivision (31);
            t[Edo53]           = equalDivision (53);
            t[JustFiveLimit]   = fromRatios ({ {1,1}, {16,15}, {9,8}, {6,5}, {5,4}, {4,3},
                                               {45,32}, {3,2}, {8,5}, {5,3}, {9,5}, {15,8} });
            t[JustSevenLimit]  = fromRatios ({ {1,1}, {8,7}, {7,6}, {6,5}, {5,4}, {4,3}, {7,5},
                                               {10,7}, {3,2}, {8,5}, {5,3}, {7,4}, {12,7} });
            t[Harmonics8to16]  = fromRatios ({ {8,8}, {9,8}, {10,8}, {11,8}, {12,8},
                                               {13,8}, {14,8}, {15,8} });
            t[BohlenPierce]    = equalDivision (13, ratioToCents (3, 1));
            t[Slendro]         = equalDivision (5);
            t[Pelog]           = fromCents ({ 0, 120, 270, 540, 670, 785, 950 });
            t[MaqamRast]       = fromCents ({ 0, 200, 350, 500, 700, 900, 1050 });
            t[Custom]          = equalDivision (12);
            return t;
        }();
        return tables;
    }

    // ------------------------------------------------------------------------
    // Pitch parsing (message thread only).
    //
    // A token with a '/' is a RATIO "a/b". Anything else is CENTS — including a
    // bare integer, because "100 200 … 1200" is what a person types into the
    // Custom field. v3.0.0 used the .scl rule here (bare integer = a/1), so
    // typed cents became ratios, the period 1200/1 (~12 274 ct) failed the
    // 4-octave check and Apply was rejected every time. The .scl rule is now
    // applied only by tableFromScala, which rewrites a bare integer as "a/1"
    // before it reaches this text form. Returns false for anything else,
    // including non-positive ratios.
    // ------------------------------------------------------------------------
    inline bool parsePitch (juce::String token, float& centsOut)
    {
        token = token.trim();
        if (token.isEmpty())
            return false;

        if (! token.containsChar ('/'))
        {
            if (! token.containsOnly ("+-.0123456789"))
                return false;
            centsOut = token.getFloatValue();
            return std::isfinite (centsOut);
        }

        const juce::String num = token.upToFirstOccurrenceOf ("/", false, false).trim();
        const juce::String den = token.fromFirstOccurrenceOf ("/", false, false).trim();
        if (num.isEmpty() || den.isEmpty() || ! num.containsOnly ("0123456789")
            || ! den.containsOnly ("0123456789"))
            return false;

        const double n = num.getDoubleValue(), d = den.getDoubleValue();
        if (n <= 0.0 || d <= 0.0)
            return false;
        centsOut = ratioToCents (n, d);
        return true;
    }

    /** Builds a table from Scala-convention pitches ABOVE 1/1: the last pitch
        is the period, the others are degrees 1..N-1 (degree 0 is implicit).
        Pitches are sorted, folded into (0, period) and de-duplicated. */
    inline bool tableFromPitches (const juce::Array<float>& pitches, Table& out, juce::String& error)
    {
        if (pitches.isEmpty())  { error = "noPitches"; return false; }

        const float period = pitches.getLast();
        if (! (period > 1.0f) || period > 4800.0f) { error = "badPeriod"; return false; }

        juce::Array<float> degrees;
        degrees.add (0.0f);
        for (int i = 0; i < pitches.size() - 1; ++i)
        {
            float c = std::fmod (pitches[i], period);
            if (c < 0.0f) c += period;
            if (c > 0.01f && c < period - 0.01f)
                degrees.addIfNotAlreadyThere (c);
        }
        degrees.sort();

        if (degrees.size() > kMaxPitches) { error = "tooMany"; return false; }

        out = Table {};
        out.count = degrees.size();
        for (int i = 0; i < out.count; ++i)
            out.cents[static_cast<size_t> (i)] = degrees[i];
        out.period = period;
        return true;
    }

    namespace detail
    {
        /** "edo N [period]" / "N-edo [period]" (case-insensitive) — JS parity
            with the O-Granulator page parser. Returns -1 when the text is NOT
            edo-shaped (the caller falls through to the token path), 0 on an
            error (error / badToken set), 1 on success (out set; error left
            untouched, like the token path). Error order: badPitch (period
            token), badPeriod, badEdo. Every edo-shaped text contains letters,
            which parsePitch rejects, so no text that parsed before 1.0.0
            reaches this branch. */
        inline int tryEdo (const juce::String& text, Table& out, juce::String& error,
                           juce::String& badToken, juce::String* periodToken = nullptr)
        {
            static const std::regex re (R"(^(?:edo\s*(\d+)|(\d+)\s*-?\s*edo)(?:\s+(\S+))?$)",
                                        std::regex::ECMAScript | std::regex::icase);
            const std::string s = text.trim().toStdString();
            std::smatch m;
            if (! std::regex_match (s, m, re))
                return -1;

            const juce::String nStr = juce::String::fromUTF8 ((m[1].matched ? m[1].str() : m[2].str()).c_str());
            float period = 1200.0f;
            if (m[3].matched)
            {
                const juce::String tok = juce::String::fromUTF8 (m[3].str().c_str());
                if (periodToken != nullptr)
                    *periodToken = tok;
                if (! parsePitch (tok, period)) { badToken = tok; error = "badPitch"; return 0; }
            }
            if (! (period > 1.0f) || period > 4800.0f) { error = "badPeriod"; return 0; }

            // More than 3 digits can only be out of range — and would overflow getIntValue.
            const int n = nStr.length() > 3 ? 0 : nStr.getIntValue();
            if (n < 1 || n > kMaxPitches) { error = "badEdo"; return 0; }

            out = equalDivision (n, period);   // "edo 19" == builtIns()[Edo19], bitwise
            return 1;
        }

        inline juce::StringArray textTokens (const juce::String& text)
        {
            juce::StringArray tokens;
            tokens.addTokens (text.replaceCharacters (",;\n\r\t", "     "), " ", "");
            tokens.removeEmptyStrings();
            return tokens;
        }

        /** Shortest fixed-point form that reads back (getFloatValue, the
            parsePitch path) to exactly the same float. Always contains '.',
            so the .scl rule never reads it as a ratio. */
        inline juce::String formatCents (float c)
        {
            for (int p = 1; p <= 10; ++p)
            {
                const juce::String s (static_cast<double> (c), p);
                if (juce::exactlyEqual (s.getFloatValue(), c))
                    return s;
            }
            return juce::String (static_cast<double> (c), 10);
        }
    }

    /** A whitespace/comma separated pitch list, as typed in the UI, or the
        "edo N [period]" shorthand. On a bad token, badToken holds it. */
    inline bool tableFromText (const juce::String& text, Table& out, juce::String& error, juce::String& badToken)
    {
        badToken = {};
        const int edo = detail::tryEdo (text, out, error, badToken);
        if (edo >= 0)
            return edo == 1;

        juce::StringArray tokens;
        tokens.addTokens (text.replaceCharacters (",;\n\r\t", "     "), " ", "");
        tokens.removeEmptyStrings();

        juce::Array<float> pitches;
        for (auto& t : tokens)
        {
            float c = 0.0f;
            if (! parsePitch (t, c)) { badToken = t; error = "badPitch"; return false; }
            pitches.add (c);
        }
        return tableFromPitches (pitches, out, error);
    }

    /** A whitespace/comma separated pitch list, as typed in the UI. */
    inline bool tableFromText (const juce::String& text, Table& out, juce::String& error)
    {
        juce::String unused;
        return tableFromText (text, out, error, unused);
    }

    /** A Scala .scl file body. Returns the pitch list re-expressed as the text
        form (so the UI field shows what was loaded) and the description line. */
    inline bool tableFromScala (const juce::String& scl, Table& out, juce::String& asText,
                                juce::String& description, juce::String& error)
    {
        juce::StringArray lines;
        lines.addLines (scl);

        int state = 0, expected = 0;
        juce::Array<float> pitches;
        juce::StringArray texts;

        for (auto line : lines)
        {
            if (line.trimStart().startsWithChar ('!'))
                continue;

            if (state == 0) { description = line.trim(); state = 1; continue; }
            if (state == 1)
            {
                expected = line.trim().getIntValue();
                if (expected < 1) { error = "badCount"; return false; }
                state = 2;
                continue;
            }

            // A pitch line: the first token is the pitch, the rest is a comment.
            auto token = line.trim().upToFirstOccurrenceOf (" ", false, false)
                                    .upToFirstOccurrenceOf ("\t", false, false);
            if (token.isEmpty())
                continue;
            // The .scl rule: no '.' means a ratio, and a bare integer "a" is
            // a/1. Written out as "a/1" so the text form (which reads a bare
            // integer as cents) round-trips the same pitch.
            if (! token.containsChar ('.') && ! token.containsChar ('/'))
                token << "/1";
            float c = 0.0f;
            if (! parsePitch (token, c)) { error = "badPitch"; return false; }
            pitches.add (c);
            texts.add (token);
            if (pitches.size() == expected)
                break;
        }

        if (state < 2 || pitches.size() != expected) { error = "badCount"; return false; }
        if (! tableFromPitches (pitches, out, error))
            return false;

        asText = texts.joinIntoString (" ");
        return true;
    }

    /** The .scl description line for a scale name: one line, no leading '!'
        (it would read as a comment), "Ouaricon custom scale" when empty.
        tableFromScala (toScala (name, text)) returns exactly this. */
    inline juce::String scalaDescription (const juce::String& name)
    {
        auto d = name.replaceCharacters ("\r\n\t", "   ").trim();
        while (d.startsWithChar ('!'))
            d = d.substring (1).trim();
        return d.isEmpty() ? juce::String ("Ouaricon custom scale") : d;
    }

    /** A Scala .scl body for a text-form scale. Returns {} if the text does
        not parse. Ratio tokens are kept as typed when they land on a degree
        unchanged; everything else is written as cents in the shortest exact
        form (always with a '.'). tableFromScala on the result gives a Table
        bitwise equal to tableFromText (text). */
    inline juce::String toScala (const juce::String& name, const juce::String& text)
    {
        Table t;
        juce::String error, badToken;
        if (! tableFromText (text, t, error, badToken))
            return {};

        juce::String periodToken;
        juce::StringArray ratioTokens;
        juce::Array<float> ratioValues;
        {
            Table unusedT;
            juce::String unusedE, unusedB;
            if (detail::tryEdo (text, unusedT, unusedE, unusedB, &periodToken) < 0)
            {
                const auto tokens = detail::textTokens (text);
                for (auto& tok : tokens)
                {
                    float c = 0.0f;
                    if (tok.containsChar ('/') && parsePitch (tok, c))
                    {
                        ratioTokens.add (tok.trim());
                        ratioValues.add (c);
                    }
                }
                periodToken = tokens[tokens.size() - 1].trim();
            }
            else if (periodToken.isEmpty())
            {
                periodToken = "2/1";
            }
        }

        auto lineFor = [&] (float c) -> juce::String
        {
            for (int i = 0; i < ratioValues.size(); ++i)
                if (juce::exactlyEqual (ratioValues[i], c))
                    return ratioTokens[i];
            return detail::formatCents (c);
        };

        const auto description = scalaDescription (name);
        juce::String scl;
        scl << "! " << juce::File::createLegalFileName (name.trim().isEmpty() ? juce::String ("custom") : description)
            << ".scl\n!\n" << description << "\n " << t.count << "\n!\n";
        for (int k = 1; k < t.count; ++k)
            scl << " " << lineFor (t.cents[static_cast<size_t> (k)]) << "\n";
        scl << " " << (periodToken.containsChar ('/') ? periodToken : detail::formatCents (t.period)) << "\n";
        return scl;
    }
}
