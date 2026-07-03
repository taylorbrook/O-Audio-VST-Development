/*
  ==============================================================================

    FilenameParser.cpp
    Microtonal Sample Engine - Tolerant filename parser implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "FilenameParser.h"
#include <cctype>
#include <optional>

namespace FilenameParser
{
namespace
{
    //==============================================================================
    // Note-letter → semitone offset within an octave.
    //   C=0, D=2, E=4, F=5, G=7, A=9, B=11
    // Sharps add 1, flats subtract 1.
    inline std::optional<int> letterToSemitone (juce::juce_wchar c) noexcept
    {
        switch (juce::CharacterFunctions::toUpperCase (c))
        {
            case 'C': return 0;
            case 'D': return 2;
            case 'E': return 4;
            case 'F': return 5;
            case 'G': return 7;
            case 'A': return 9;
            case 'B': return 11;
            default:  return std::nullopt;
        }
    }

    //==============================================================================
    // Try to parse `token` as a scientific-pitch note: [A-G][#b]?(-?[0-9]).
    // Returns the resulting MIDI number (clamped 0..127) or nullopt.
    //
    // Notation: middle C = C3 = MIDI 60 (Ableton/Cubase/FL/Logic/Pro Tools/
    // Reaper default). v1.11.1 switched from C4=60 (Yamaha/JUCE-native) to
    // C3=60 to align filename parsing with the dominant DAW labelling — the
    // previous convention caused samples named in DAW-native form to load
    // one octave low, audibly transposing playback up an octave.
    // Therefore: MIDI = (octave + 2) * 12 + semitoneOffset, so:
    //   C-2 → 0, A-2 → 9, C-1 → 12, C3 → 60, A3 → 69, G8 → 127.
    std::optional<int> parseAsScientificPitch (const juce::String& token)
    {
        if (token.isEmpty())
            return std::nullopt;

        const auto* s = token.toRawUTF8();
        const int   len = (int) token.getNumBytesAsUTF8();

        // First char: note letter
        auto baseSemi = letterToSemitone ((juce::juce_wchar) s[0]);
        if (! baseSemi.has_value())
            return std::nullopt;

        int idx = 1;
        int accidental = 0;
        if (idx < len && (s[idx] == '#' || s[idx] == 'b' || s[idx] == 'B'))
        {
            // Lower-case 'b' = flat; '#' = sharp. Note: upper-case 'B' is a
            // note letter, not a flat marker — we already consumed the first
            // letter, so an upper-case 'B' here is ambiguous. Reject by
            // treating as flat ONLY if it's lower-case 'b'. Keep '#' for sharp.
            if (s[idx] == '#')
                accidental = +1;
            else if (s[idx] == 'b')
                accidental = -1;
            else
                return std::nullopt;  // upper-case 'B' after a letter is invalid
            ++idx;
        }

        // Remaining chars: signed integer octave.
        if (idx >= len)
            return std::nullopt;

        const juce::String octStr = token.substring (idx);
        if (octStr.isEmpty())
            return std::nullopt;

        // Validate that octStr is a signed integer ([+/-]?[0-9]+).
        auto* octChars = octStr.toRawUTF8();
        const int octLen = (int) octStr.getNumBytesAsUTF8();
        int oi = 0;
        if (octChars[0] == '+' || octChars[0] == '-')
            oi = 1;
        if (oi >= octLen)
            return std::nullopt;
        for (int k = oi; k < octLen; ++k)
            if (octChars[k] < '0' || octChars[k] > '9')
                return std::nullopt;

        const int octave = octStr.getIntValue();
        const int semiOffset = *baseSemi + accidental;
        const int midi = (octave + 2) * 12 + semiOffset;

        if (midi < 0 || midi > 127)
            return std::nullopt;
        return midi;
    }

    //==============================================================================
    // Try to parse `token` as a MIDI form: ^[Mm][Ii][Dd][Ii]?[0-9]{1,3}$
    // Returns the MIDI number (clamped 0..127) or nullopt.
    // v1.17.1 (PARSE-S1): `lc` is the already-lower-cased token. parse()
    // lower-cases each token once into lcTokens and passes it here, so the
    // case-insensitive helpers no longer re-allocate a lower-cased copy per
    // call. parseAsScientificPitch is the deliberate exception — it is
    // case-SENSITIVE ('b'=flat vs 'B'=note letter) and still takes the original.
    std::optional<int> parseAsMidiForm (const juce::String& lc)
    {
        if (! lc.startsWith ("midi"))
            return std::nullopt;

        const auto rest = lc.substring (4);
        if (rest.isEmpty())
            return std::nullopt;

        auto* s = rest.toRawUTF8();
        const int len = (int) rest.getNumBytesAsUTF8();
        for (int k = 0; k < len; ++k)
            if (s[k] < '0' || s[k] > '9')
                return std::nullopt;

        const int n = rest.getIntValue();
        if (n < 0 || n > 127)
            return std::nullopt;
        return n;
    }

    //==============================================================================
    // Try to parse `token` as a bare integer in [0..127].
    std::optional<int> parseAsBareInteger (const juce::String& token)
    {
        if (token.isEmpty())
            return std::nullopt;

        auto* s = token.toRawUTF8();
        const int len = (int) token.getNumBytesAsUTF8();
        for (int k = 0; k < len; ++k)
            if (s[k] < '0' || s[k] > '9')
                return std::nullopt;

        const int n = token.getIntValue();
        if (n < 0 || n > 127)
            return std::nullopt;
        return n;
    }

    //==============================================================================
    // Parse a velocity token. Recognised:
    //   v1, v2, v3, v4         → 0, 1, 2, 3
    //   vel1, vel2, vel3, vel4 → 0, 1, 2, 3
    //   p, mp, mf, f           → 0, 1, 2, 3
    //   layer1..4, L1..4, Lyr1..4 → 0..3
    // Returns nullopt if not recognised.
    std::optional<int> parseAsVelocity (const juce::String& lc)
    {
        // Dynamics first (exact-match).
        if (lc == "p")  return 0;
        if (lc == "mp") return 1;
        if (lc == "mf") return 2;
        if (lc == "f")  return 3;

        // v[1-4] / vel[1-4]
        if (lc.startsWith ("vel"))
        {
            const auto rest = lc.substring (3);
            if (rest.length() == 1 && rest[0] >= '1' && rest[0] <= '9')
            {
                const int n = rest.getIntValue();
                if (n >= 1 && n <= 4)
                    return n - 1;
            }
            return std::nullopt;
        }
        if (lc.startsWith ("v") && lc.length() >= 2)
        {
            const auto rest = lc.substring (1);
            // Reject tokens like "vox" or "viola" that just happen to start with v.
            // Require rest is a single digit 1-4.
            if (rest.length() == 1 && rest[0] >= '1' && rest[0] <= '9')
            {
                const int n = rest.getIntValue();
                if (n >= 1 && n <= 4)
                    return n - 1;
            }
            return std::nullopt;
        }

        // layer[N]
        if (lc.startsWith ("layer"))
        {
            const auto rest = lc.substring (5);
            if (rest.length() == 1 && rest[0] >= '1' && rest[0] <= '9')
            {
                const int n = rest.getIntValue();
                if (n >= 1 && n <= 4)
                    return n - 1;
            }
            return std::nullopt;
        }

        // Lyr[N]
        if (lc.startsWith ("lyr"))
        {
            const auto rest = lc.substring (3);
            if (rest.length() == 1 && rest[0] >= '1' && rest[0] <= '9')
            {
                const int n = rest.getIntValue();
                if (n >= 1 && n <= 4)
                    return n - 1;
            }
            return std::nullopt;
        }

        // L[N]  — must be EXACTLY two chars: 'L' + digit, to avoid eating
        // prefixes like "Lab" or "London".
        if (lc.length() == 2 && lc[0] == (juce::juce_wchar) 'l'
                             && lc[1] >= (juce::juce_wchar) '1'
                             && lc[1] <= (juce::juce_wchar) '9')
        {
            const int n = (int) (lc[1] - (juce::juce_wchar) '0');
            if (n >= 1 && n <= 4)
                return n - 1;
        }

        return std::nullopt;
    }

    //==============================================================================
    // v1.8.0: parse a round-robin index token. Recognised:
    //   rr[N]    — 1-based, common in commercial libraries (Spitfire, OT)
    //   take[N]  — common in indie / sound-design libraries
    //   tk[N]    — short form
    // [N] may be 1..2 digits. Returns the 0-based index (rr1 → 0, take7 → 6),
    // capped at 63. Returns nullopt for unrecognised tokens.
    //
    // Case-insensitive. Forms like "round1" / "var1" intentionally NOT recognised
    // — the user signalled rr/take/tk specifically; we don't widen the surface.
    std::optional<int> parseAsRrIndex (const juce::String& lc)
    {
        auto extractTrailingDigits = [] (const juce::String& s) -> std::optional<int>
        {
            if (s.isEmpty())
                return std::nullopt;
            auto* p = s.toRawUTF8();
            const int len = (int) s.getNumBytesAsUTF8();
            if (len < 1 || len > 2)
                return std::nullopt;
            for (int k = 0; k < len; ++k)
                if (p[k] < '0' || p[k] > '9')
                    return std::nullopt;
            const int n = s.getIntValue();
            if (n < 1 || n > 64)
                return std::nullopt;
            return n - 1;   // 0-based index
        };

        if (lc.startsWith ("rr"))
            return extractTrailingDigits (lc.substring (2));
        if (lc.startsWith ("take"))
            return extractTrailingDigits (lc.substring (4));
        if (lc.startsWith ("tk"))
            return extractTrailingDigits (lc.substring (2));

        return std::nullopt;
    }

    //==============================================================================
    // v1.14.0: parse a technique token. Recognised tokens (case-insensitive,
    // must be the WHOLE token after the [_\-\s.]+ tokeniser splits — never a
    // substring match):
    //   slot 0 — ord, ordinario, ordinary
    //   slot 1 — sp, sulpont, sulponticello
    //   slot 2 — st, sultasto
    //   slot 3 — sv, senzavib, senzavibrato, nonvib, nonvibrato
    //   slot 4 — cs, consord, consordino, muted, mute
    //   slot 5 — pizz, pizzicato
    //   slot 6 — harm, harmonic, harmonics
    //   slot 7 — mart, martele, martellato
    //   slot 8 — trem, tremolo
    //   slot 9 — flaut, flautando, flautato      (outside default KS range)
    //
    // We deliberately do NOT use prefix-startsWith matching here — `sp` would
    // collide with `Spitfire`, `st` with `Stradivari`, `cs` with `csharp`, etc.
    // Each token must match exactly to be accepted.
    //
    // The two-token forms (sul pont / sul tasto / senza vib / non vib /
    // con sord) are recognised separately by parseAsTechniquePair, since the
    // [_\-\s.]+ tokeniser splits "sul_pont" into ["sul", "pont"].
    std::optional<int> parseAsTechnique (const juce::String& lc)
    {
        // Slot 0 — ordinario / normale. "norm"/"normale"/"normal" are the
        // common orchestral-library spelling for the plain/un-effected
        // articulation (v1.17.2) — they map onto the same slot as "ord".
        if (lc == "ord" || lc == "ordinario" || lc == "ordinary"
                        || lc == "norm" || lc == "normale" || lc == "normal") return 0;
        // Slot 1 — sul ponticello
        if (lc == "sp" || lc == "sulpont" || lc == "sulponticello")   return 1;
        // Slot 2 — sul tasto
        if (lc == "st" || lc == "sultasto")                           return 2;
        // Slot 3 — senza vibrato / non vibrato
        if (lc == "sv" || lc == "senzavib" || lc == "senzavibrato"
                       || lc == "nonvib" || lc == "nonvibrato")       return 3;
        // Slot 4 — con sordino / muted
        if (lc == "cs" || lc == "consord" || lc == "consordino"
                       || lc == "muted" || lc == "mute")              return 4;
        // Slot 5 — pizzicato
        if (lc == "pizz" || lc == "pizzicato")                        return 5;
        // Slot 6 — natural harmonic
        if (lc == "harm" || lc == "harmonic" || lc == "harmonics")    return 6;
        // Slot 7 — martelé
        if (lc == "mart" || lc == "martele" || lc == "martellato")    return 7;
        // Slot 8 — tremolo
        if (lc == "trem" || lc == "tremolo")                          return 8;
        // Slot 9 — flautando
        if (lc == "flaut" || lc == "flautando" || lc == "flautato")   return 9;
        return std::nullopt;
    }

    //==============================================================================
    // v1.14.0: adjacent-pair technique recognition. The tokeniser splits on
    // `[_\-\s.]+`, so filenames like `Violin_sul_pont_C3.wav` produce
    // ["Violin", "sul", "pont", "C3"] — the canonical "sul ponticello"
    // form is two tokens. We detect this by scanning adjacent pairs after
    // the single-token pass fails.
    //
    // Recognised pairs (case-insensitive on both halves):
    //   sul + pont        → slot 1
    //   sul + ponticello  → slot 1
    //   sul + tasto       → slot 2
    //   senza + vib       → slot 3
    //   senza + vibrato   → slot 3
    //   non + vib         → slot 3
    //   non + vibrato     → slot 3
    //   con + sord        → slot 4
    //   con + sordino     → slot 4
    //
    // The first-token leads (sul / senza / non / con) are NEVER accepted as
    // standalone single-token forms in parseAsTechnique above — they're too
    // generic and would over-match (e.g. a file accidentally named
    // `con_a_capo` would land on slot 4 if "con" alone were recognised).
    std::optional<int> parseAsTechniquePair (const juce::String& lca,
                                             const juce::String& lcb)
    {
        if (lca == "sul")
        {
            if (lcb == "pont" || lcb == "ponticello") return 1;
            if (lcb == "tasto")                       return 2;
        }
        if (lca == "senza" || lca == "non")
        {
            if (lcb == "vib" || lcb == "vibrato")     return 3;
        }
        if (lca == "con")
        {
            if (lcb == "sord" || lcb == "sordino")    return 4;
        }
        return std::nullopt;
    }

} // namespace

//==============================================================================
std::optional<ParsedName> parse (const juce::String& filenameNoExtension)
{
    if (filenameNoExtension.isEmpty())
        return std::nullopt;

    juce::StringArray tokens;
    tokens.addTokens (filenameNoExtension, "_-. \t", "");
    tokens.removeEmptyStrings (true);

    if (tokens.isEmpty())
        return std::nullopt;

    // v1.17.1 (PARSE-S1): lower-case every token ONCE. The case-insensitive
    // helpers (MIDI form / velocity / RR / technique) take these directly
    // instead of re-allocating a lower-cased copy on each call. The note scan
    // keeps using the ORIGINAL tokens for parseAsScientificPitch (case-sensitive
    // 'b'=flat vs 'B'=note letter) and parseAsBareInteger (digits only).
    juce::StringArray lcTokens;
    lcTokens.ensureStorageAllocated (tokens.size());
    for (const auto& t : tokens)
        lcTokens.add (t.toLowerCase());

    // First pass: try to find a NOTE token (scientific pitch first, then MIDI form).
    // We do NOT accept bare integer in this first pass — the bare-integer fallback
    // only fires if NO scientific-pitch / MIDI-form token is present (RESEARCH R4).
    std::optional<int> midiNote;
    int  noteTokenIndex = -1;
    for (int i = 0; i < tokens.size(); ++i)
    {
        if (auto m = parseAsScientificPitch (tokens[i]))   // original case (case-sensitive)
        {
            midiNote = *m;
            noteTokenIndex = i;
            break;
        }
        if (auto m = parseAsMidiForm (lcTokens[i]))
        {
            midiNote = *m;
            noteTokenIndex = i;
            break;
        }
    }

    // Bare-integer fallback (only if no scientific/MIDI-form note was found).
    if (! midiNote.has_value())
    {
        for (int i = 0; i < tokens.size(); ++i)
        {
            if (auto m = parseAsBareInteger (tokens[i]))
            {
                midiNote = *m;
                noteTokenIndex = i;
                break;
            }
        }
    }

    if (! midiNote.has_value())
        return std::nullopt;

    // Velocity scan — two-tier to suppress false positives from dynamics
    // letters appearing inside instrument-name substrings.
    //
    // Tier 1 — POST-note (any velocity form): the first token AFTER the note
    //   that parses as a velocity wins. Both explicit forms (v[1-4]/vel[1-4]/
    //   layer[N]/L[N]/lyr[N]) and dynamics (p/mp/mf/f) are accepted here
    //   because the post-note region is by convention where velocity tags
    //   live.
    //
    // Tier 2 — PRE-note (any velocity form): if no post-note match was found,
    //   fall back to scanning tokens BEFORE the note for ANY velocity form,
    //   including dynamics (p/mp/mf/f). This is the dominant orchestral naming
    //   convention — `vln_norm_mf-A#2-V127-…` puts the dynamic immediately
    //   before the pitch — so the dynamic MUST be honoured here.
    //
    // v1.17.2 (PARSE-DYN): this used to skip dynamics pre-note
    // (`parseAsExplicitVelocity`) to avoid assigning a single-dynamic library
    // (e.g. all `mp`) to a non-zero layer and silencing the bottom of the
    // velocity range — the voice bailed to silence when the resolved layer's
    // cell was empty. That guard now lives where it belongs: the voice's
    // primary cell lookup falls back to the NEAREST populated layer
    // (SampleMap::findCellNearestLayer), so honouring pre-note dynamics can no
    // longer cause silence. Pre/post-note dynamics are therefore symmetric.
    //
    // Default if nothing matches: velLayer = 0.
    int velLayer = 0;

    bool foundVel = false;
    for (int i = noteTokenIndex + 1; i < tokens.size(); ++i)
    {
        if (auto v = parseAsVelocity (lcTokens[i]))
        {
            velLayer = *v;
            foundVel = true;
            break;
        }
    }

    if (! foundVel)
    {
        for (int i = 0; i < noteTokenIndex; ++i)
        {
            // v1.23.6 (IN-03): reject BARE single-letter dynamics ("p"/"f") in the
            // PRE-note tier. Pre-note, a single delimited letter is far more likely
            // an instrument abbreviation than a dynamic — `F-C3.wav` (Flute) was
            // silently read as forte→layer 3, `P_C3.wav` (Piano) as piano→layer 0.
            // Two-char dynamics (mp/mf) and explicit forms (v/vel/L/layer/lyr) stay
            // valid here (the dominant `vln_norm_mf-A#2-…` convention still works);
            // bare p/f remain valid POST-note (handled in the loop above).
            const auto& lc = lcTokens[i];
            if (lc == "p" || lc == "f")
                continue;

            if (auto v = parseAsVelocity (lc))
            {
                velLayer = *v;
                break;
            }
        }
    }

    // v1.8.0: round-robin index scan. Independent of position relative to
    // the note token (rr/take/tk has no token-name collision risk like
    // dynamics letters). First token that matches wins; -1 means no token.
    //
    // v1.12.3 (HG-02): the v1.8.0 form required the index to be glued to
    // the prefix (e.g. "take1"). DAW-export conventions like
    //   Piano_C3_take_1.wav  /  Trumpet_F#3_rr_2.aif  /  Bowed_E2_tk_3.flac
    // tokenise to ["…", "take", "1"] — leaving the prefix bare and
    // silently dropping RR semantics. We now ALSO detect the split form:
    // a bare "rr" / "take" / "tk" token whose IMMEDIATE NEXT token is a
    // 1-2 digit integer in 1..64.
    //
    // v1.23.6 (WR-04): the split form must NOT re-consume the token already
    // claimed as the bare-integer note. For `take_60.wav` — tokens ["take","60"]
    // with "60" chosen as the MIDI note — the old scan evaluated
    // parseAsRrIndex("take"+"60") → rrIndex 59, using "60" as BOTH the pitch and
    // the RR index. That fabricates a spurious explicit RR that suppresses the
    // ambiguous-duplicate modal and skews variant ordering. Fix: skip the note
    // token itself, and in the split form require the digit token != noteTokenIndex.
    int rrIndex = -1;
    for (int i = 0; i < tokens.size(); ++i)
    {
        if (i == noteTokenIndex)
            continue;   // never re-read the token already claimed as the note

        // Glued form (v1.8.0): "rr1", "take7", "tk2", etc.
        if (auto rr = parseAsRrIndex (lcTokens[i]))
        {
            rrIndex = *rr;
            break;
        }

        // v1.12.3 (HG-02): split form. Match a bare prefix followed by a
        // numeric next token. Prefix match is exact (case-insensitive) so
        // "taken" / "tkr" / "rrm" don't accidentally trip this branch — they
        // would have already failed parseAsRrIndex above.
        const auto& lc = lcTokens[i];
        if (lc == "rr" || lc == "take" || lc == "tk")
        {
            if (i + 1 < tokens.size() && i + 1 != noteTokenIndex)   // digit token must not be the note
            {
                if (auto rr = parseAsRrIndex (lc + lcTokens[i + 1]))
                {
                    rrIndex = *rr;
                    break;
                }
            }
        }
    }

    // v1.14.0: technique scan. Independent of position relative to the note
    // token (technique tokens are exact-match per parseAsTechnique — no
    // substring collisions). First match wins; -1 means no token.
    //
    // Two passes per token-position: first the single-token form, then the
    // adjacent-pair form (sul + pont, senza + vib, con + sord, non + vib).
    // The pair form is necessary because the [_\-\s.]+ tokeniser splits
    // canonical orchestral filenames like `Violin_sul_pont_C3.wav` into
    // separate tokens.
    int techniqueIndex = -1;
    for (int i = 0; i < tokens.size(); ++i)
    {
        if (auto t = parseAsTechnique (lcTokens[i]))
        {
            techniqueIndex = *t;
            break;
        }
        if (i + 1 < tokens.size())
        {
            if (auto t = parseAsTechniquePair (lcTokens[i], lcTokens[i + 1]))
            {
                techniqueIndex = *t;
                break;
            }
        }
    }

    return ParsedName { *midiNote, velLayer, rrIndex, techniqueIndex };
}

} // namespace FilenameParser

//==============================================================================
// Inline unit tests (gated by OMTS_UNIT_TESTS). Compiled out by default.
// To run: build with `-DOMTS_UNIT_TESTS=1`, then call FilenameParser::runTests().
#ifdef OMTS_UNIT_TESTS

#include <iostream>
#include <vector>

namespace FilenameParser
{
    namespace
    {
        struct TestCase
        {
            const char* input;
            std::optional<int> expectedMidi;     // nullopt = expect parse failure
            std::optional<int> expectedVelLayer; // ignored if expectedMidi == nullopt
            int                expectedRrIndex = -1;  // v1.8.0
        };
    }

    int runTests()
    {
        // v1.11.1 — convention switched to C3=60 (Ableton/Cubase/FL/Logic/
        // Pro Tools/Reaper default). C4 now maps to MIDI 72; C3 maps to 60.
        // Test inputs that previously expected midi=60 for "C4" updated to
        // expect midi=72; a new "C3" case anchors the new middle-C target.
        const std::vector<TestCase> cases = {
            // Scientific pitch + velocity (case-insensitive)
            { "C3_v1",         60,        0,        -1 },     // middle C — new anchor
            { "C4_v1",         72,        0,        -1 },
            { "c4_v1",         72,        0,        -1 },
            { "F#3_v2",        66,        1,        -1 },
            { "Gb5_vel3",      90,        2,        -1 },
            { "A-1_v1",        21,        0,        -1 },
            // MIDI form (raw MIDI numbers — convention-independent)
            { "MIDI60_v1",     60,        0,        -1 },
            { "midi72_layer2", 72,        1,        -1 },
            // Bare-integer fallback (raw MIDI — convention-independent)
            { "60_v1",         60,        0,        -1 },
            // Unparseable
            { "weird-name",    std::nullopt, std::nullopt, -1 },
            // Default vel (no velocity token)
            { "C3",            60,        0,        -1 },
            // Dynamics tokens
            { "C3_p",          60,        0,        -1 },
            { "C3_mp",         60,        1,        -1 },
            { "C3_mf",         60,        2,        -1 },
            { "C3_f",          60,        3,        -1 },
            // Order-independent parse
            { "Lyr3_C3",       60,        2,        -1 },
            { "C3_L4",         60,        3,        -1 },
            // Extra leading token (filler)
            { "Sample_C3_v1",  60,        0,        -1 },
            // Mixed separators
            { "C3-v2.something", 60,      1,        -1 },

            // v1.17.2 (PARSE-DYN) — pre-note dynamics ARE now honoured
            // (reversed from the Phase 2.5 suppression). Pre/post-note dynamics
            // are symmetric; the empty-layer silence that the old suppression
            // guarded against is now handled by SampleMap::findCellNearestLayer
            // at the voice. (D#3 is MIDI 63 under C3=60; C3 is MIDI 60.)
            { "vln_long_mp-D#3-V127-T6N6", 63, 1,   -1 },   // pre-note "mp" → layer 1
            { "vln_long_mp-C3-V127-EHGV",  60, 1,   -1 },
            { "vln_norm_mf-A#2-V127-JXRO", 58, 2,   -1 },   // user's library: pre-note "mf" → layer 2
            { "Auto Sampled Instrument-D#3-V127-RJHU", 63, 0, -1 },  // no dynamics token → default 0
            { "mp_C3",     60, 1,   -1 },                    // pre-note "mp" → layer 1
            { "C3_mp",     60, 1,   -1 },
            { "L3_C3",     60, 2,   -1 },
            { "vel2_C3",   60, 1,   -1 },

            // v1.17.2 — "norm"/"normale" recognised as technique slot 0 (ord).
            // (techniqueIndex isn't asserted by this harness, but these pin
            //  that the dynamic still parses correctly alongside a norm token.)
            { "Cello_normale_mf_C3",       60, 2,   -1 },
            { "Cello_norm_p_C3",           60, 0,   -1 },

            // v1.8.0 — round-robin tokens.
            { "C3_v1_rr1",       60, 0, 0 },
            { "C3_v1_rr2",       60, 0, 1 },
            { "C3_rr03",         60, 0, 2 },
            { "C3_take1",        60, 0, 0 },
            { "C3_take7",        60, 0, 6 },
            { "C3_tk1",          60, 0, 0 },
            { "RR2_C3_v1",       60, 0, 1 },     // pre-note RR also fine
            { "C3_v1_rr0",       60, 0, -1 },    // 0 rejected (1-based input)
            { "C3_rr",           60, 0, -1 },    // bare prefix rejected
            { "C3_round2",       60, 0, -1 },    // not in recognised set
            { "C3_var3",         60, 0, -1 },    // not in recognised set

            // v1.12.3 (HG-02) — separator-tokenised RR forms (the DAW-export
            // convention). Tokeniser splits "Piano_C3_take_1" into
            // ["Piano","C3","take","1"]; the v1.8.0 path lost RR here.
            { "Piano_C3_take_1.wav",     60, 0, 0 },
            { "Piano_C3_take_2",         60, 0, 1 },
            { "Trumpet_F#3_rr_2.aif",    66, 0, 1 },
            { "Bowed_E2_tk_3.flac",      40, 0, 2 },
            { "C3_v1_take_5",            60, 0, 4 },
            { "C3-take-1",               60, 0, 0 },   // dash separator
            { "C3 take 12",              60, 0, 11 },  // space separator
            { "C3_take_99",              60, 0, -1 },  // out of range (>64) → no RR
            { "C3_take_0",               60, 0, -1 },  // 1-based: 0 invalid
            { "C3_take_abc",             60, 0, -1 },  // non-numeric next token
            { "C3_taken_1",              60, 0, -1 },  // "taken" is not a bare prefix
        };

        int passed = 0, failed = 0;
        for (const auto& tc : cases)
        {
            auto result = parse (juce::String (tc.input));
            const bool ok =
                (! tc.expectedMidi.has_value() && ! result.has_value())
                || ( tc.expectedMidi.has_value() &&  result.has_value()
                    && result->midiNote == *tc.expectedMidi
                    && result->velLayer == *tc.expectedVelLayer
                    && result->rrIndex  == tc.expectedRrIndex);

            if (ok) { ++passed; }
            else
            {
                ++failed;
                std::cout << "FAIL: '" << tc.input << "' -> ";
                if (result.has_value())
                    std::cout << "(midi=" << result->midiNote
                              << ", vel=" << result->velLayer
                              << ", rr=" << result->rrIndex << ")";
                else
                    std::cout << "<none>";
                std::cout << "  expected ";
                if (tc.expectedMidi.has_value())
                    std::cout << "(midi=" << *tc.expectedMidi
                              << ", vel=" << *tc.expectedVelLayer
                              << ", rr=" << tc.expectedRrIndex << ")";
                else
                    std::cout << "<none>";
                std::cout << "\n";
            }
        }
        std::cout << "FilenameParser tests: " << passed << " passed, "
                  << failed << " failed\n";
        return failed;
    }
}

#endif  // OMTS_UNIT_TESTS
