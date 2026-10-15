/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>

// ============================================================================
// v3.0.0 — cents-based pitch collections (replaces the 12-TET ScaleQuantizer).
//
// A collection is a list of pitches in CENTS inside one period (1200 for an
// octave, 1901.955 for Bohlen-Pierce's tritave), degree 0 always being 0 ¢.
// Each degree can be switched off by a per-collection 64-bit mask, so no
// collection may exceed kMaxPitches degrees.
//
// The engine works on a sorted table of the ACTIVE pitches inside the span
// (Range x Pitch Rnd, sided by Direction), rebuilt on the audio thread only
// when an input changes. The table is a fixed array: no allocation, ever.
// ============================================================================

namespace PitchCollections
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

    /** A whitespace/comma separated pitch list, as typed in the UI. */
    inline bool tableFromText (const juce::String& text, Table& out, juce::String& error)
    {
        juce::StringArray tokens;
        tokens.addTokens (text.replaceCharacters (",;\n\r\t", "     "), " ", "");
        tokens.removeEmptyStrings();

        juce::Array<float> pitches;
        for (auto& t : tokens)
        {
            float c = 0.0f;
            if (! parsePitch (t, c)) { error = "badPitch"; return false; }
            pitches.add (c);
        }
        return tableFromPitches (pitches, out, error);
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
}

class PitchEngine
{
public:
    enum Mode      { Random = 0, LadderUp, LadderDown, Pendulum };
    enum Direction { Both = 0, Up, Down };

    struct Settings
    {
        int      collection = 0;
        int      rootNote   = 0;       // semitones above C
        int      mode       = Random;
        int      direction  = Both;
        float    amount     = 0.0f;    // Pitch Rnd, 0-1
        float    rangeSt    = 12.0f;   // 0-24
        float    transposeSt = 0.0f;
        float    fineCents  = 0.0f;
        uint64_t mask       = ~uint64_t (0);
        uint32_t customVersion = 0;    // bumps whenever the custom table changes
    };

    // Overall pitch is clamped to +-36 st (rate 0.125-8), the bound the delay
    // buffer and the spawn pre-roll are sized for.
    static constexpr float kMaxTotalSemitones = 36.0f;

    /** Audio thread. `custom` is the processor's audio-side copy. */
    float nextRate (const Settings& s, const PitchCollections::Table& custom, juce::Random& rng)
    {
        rebuildIfNeeded (s, custom);

        float offsetCents = 0.0f;
        const float spanCents = s.amount * s.rangeSt * 100.0f;

        if (spanCents > 0.5f)
        {
            if (s.collection == PitchCollections::Free && s.mode == Random)
            {
                const float lo = (s.direction == Up)   ? 0.0f : -spanCents;
                const float hi = (s.direction == Down) ? 0.0f :  spanCents;
                offsetCents = lo + rng.nextFloat() * (hi - lo);
            }
            else if (activeCount > 0)
            {
                if (s.mode == Random)
                    offsetCents = active[static_cast<size_t> (rng.nextInt (activeCount))];
                else
                {
                    offsetCents = active[static_cast<size_t> (ladderIndex)];
                    advanceLadder (s.mode);
                }
            }
        }

        const float totalSt = juce::jlimit (-kMaxTotalSemitones, kMaxTotalSemitones,
                                            (offsetCents + s.fineCents) / 100.0f + s.transposeSt);
        return std::exp2 (totalSt / 12.0f);
    }

    void resetLadder()
    {
        ladderIndex = startIndex;
        ladderDirection = 1;
    }

    /** Forces a table rebuild on the next nextRate() (prepare/reset). */
    void invalidate() { built = false; }

private:
    static constexpr int kMaxActive = 512;   // 53-EDO over +-24 st = 213; Free ladder 49

    std::array<float, kMaxActive> active {};
    int activeCount = 0;
    int startIndex = 0;      // the active pitch nearest 0 ¢
    int ladderIndex = 0;
    int ladderDirection = 1;

    // Rebuild key
    bool built = false;
    int  kCollection = -1, kRoot = -1, kDirection = -1, kMode = -1;
    float kSpan = -1.0f;
    uint64_t kMask = 0;
    uint32_t kCustomVersion = 0;

    void rebuildIfNeeded (const Settings& s, const PitchCollections::Table& custom)
    {
        const float span = s.amount * s.rangeSt * 100.0f;

        const bool collectionChanged = (s.collection != kCollection || s.mode != kMode);
        if (built && ! collectionChanged && s.rootNote == kRoot && s.direction == kDirection
            && span == kSpan && s.mask == kMask && s.customVersion == kCustomVersion)
            return;

        const auto& table = (s.collection == PitchCollections::Custom)
                                ? custom
                                : PitchCollections::builtIns()[static_cast<size_t> (
                                      juce::jlimit (0, PitchCollections::kCount - 1, s.collection))];

        const float lo = (s.direction == Up)   ? 0.0f : -span;
        const float hi = (s.direction == Down) ? 0.0f :  span;
        constexpr float eps = 0.01f;
        const float rootCents = static_cast<float> (s.rootNote) * 100.0f;
        const float period = table.period > 1.0f ? table.period : 1200.0f;

        activeCount = 0;
        // Pitches are rootCents + degree + k*period. Walk k from the period
        // below lo to the period above hi, ascending, so the table is sorted.
        const int kLo = static_cast<int> (std::floor ((lo - rootCents) / period)) - 1;
        const int kHi = static_cast<int> (std::ceil  ((hi - rootCents) / period)) + 1;
        for (int k = kLo; k <= kHi && activeCount < kMaxActive; ++k)
        {
            for (int d = 0; d < table.count && activeCount < kMaxActive; ++d)
            {
                if (d < 64 && ((s.mask >> d) & 1u) == 0)
                    continue;
                const float c = rootCents + table.cents[static_cast<size_t> (d)]
                              + static_cast<float> (k) * period;
                if (c >= lo - eps && c <= hi + eps)
                    active[static_cast<size_t> (activeCount++)] = c;
            }
        }

        // The pitch nearest 0 ¢ — where a ladder (re)starts.
        startIndex = 0;
        for (int i = 1; i < activeCount; ++i)
            if (std::abs (active[static_cast<size_t> (i)]) < std::abs (active[static_cast<size_t> (startIndex)]))
                startIndex = i;

        if (! built || collectionChanged)
            resetLadder();
        else
            ladderIndex = juce::jlimit (0, juce::jmax (0, activeCount - 1), ladderIndex);

        built = true;
        kCollection = s.collection; kRoot = s.rootNote; kDirection = s.direction; kMode = s.mode;
        kSpan = span; kMask = s.mask;
        kCustomVersion = s.customVersion;
    }

    void advanceLadder (int mode)
    {
        if (activeCount <= 1) { ladderIndex = 0; return; }

        if (mode == LadderUp)
            ladderIndex = (ladderIndex + 1) % activeCount;
        else if (mode == LadderDown)
            ladderIndex = (ladderIndex - 1 + activeCount) % activeCount;
        else  // Pendulum: bounce, without repeating the turning pitch
        {
            ladderIndex += ladderDirection;
            if (ladderIndex >= activeCount - 1) { ladderIndex = activeCount - 1; ladderDirection = -1; }
            else if (ladderIndex <= 0)          { ladderIndex = 0;               ladderDirection =  1; }
        }
    }
};
