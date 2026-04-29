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
    // Notation: middle C = C4 = MIDI 60 (Yamaha convention, JUCE's
    // MidiMessage::getMidiNoteName uses the same: octave-1 means MIDI 0..11).
    // Therefore: MIDI = (octave + 1) * 12 + semitoneOffset, so:
    //   C-1 → 0, A-1 → 9, C0 → 12, C4 → 60, A4 → 69, G9 → 127.
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
        const int midi = (octave + 1) * 12 + semiOffset;

        if (midi < 0 || midi > 127)
            return std::nullopt;
        return midi;
    }

    //==============================================================================
    // Try to parse `token` as a MIDI form: ^[Mm][Ii][Dd][Ii]?[0-9]{1,3}$
    // Returns the MIDI number (clamped 0..127) or nullopt.
    std::optional<int> parseAsMidiForm (const juce::String& token)
    {
        const auto lc = token.toLowerCase();
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
    std::optional<int> parseAsVelocity (const juce::String& token)
    {
        const auto lc = token.toLowerCase();

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
    // Same as parseAsVelocity but rejects bare dynamics tokens (p/mp/mf/f).
    // Used for the PRE-note pass of the velocity scan in `parse()` below.
    //
    // Why: dynamics letters collide with instrument-name substrings — the most
    // common collision is sample libraries with names like "vln_long_mp-D#3-…"
    // where "mp" is part of the library name, not a velocity tag. The unguarded
    // pre-note scan would silently match "mp" and assign velLayer=1 to every
    // file in the library, putting all slots on a layer that's only reachable
    // at MIDI velocity ≥ 65. Explicit forms (v[1-4]/vel[1-4]/layer[N]/L[N]/
    // lyr[N]) don't have this collision risk because they require a digit
    // immediately after the prefix.
    std::optional<int> parseAsExplicitVelocity (const juce::String& token)
    {
        const auto lc = token.toLowerCase();
        if (lc == "p" || lc == "mp" || lc == "mf" || lc == "f")
            return std::nullopt;
        return parseAsVelocity (token);
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

    // First pass: try to find a NOTE token (scientific pitch first, then MIDI form).
    // We do NOT accept bare integer in this first pass — the bare-integer fallback
    // only fires if NO scientific-pitch / MIDI-form token is present (RESEARCH R4).
    std::optional<int> midiNote;
    int  noteTokenIndex = -1;
    for (int i = 0; i < tokens.size(); ++i)
    {
        if (auto m = parseAsScientificPitch (tokens[i]))
        {
            midiNote = *m;
            noteTokenIndex = i;
            break;
        }
        if (auto m = parseAsMidiForm (tokens[i]))
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
    // Tier 2 — PRE-note (explicit forms only): if no post-note match was
    //   found, fall back to scanning tokens BEFORE the note for explicit
    //   velocity forms. Dynamics tokens are deliberately skipped because they
    //   collide with instrument-name fragments — `vln_long_mp-D#3-V127-T6N6`
    //   would otherwise match "mp" and assign velLayer=1 to every slot,
    //   silencing the library at MIDI velocities < 65.
    //
    // Default if nothing matches: velLayer = 0.
    int velLayer = 0;

    bool foundVel = false;
    for (int i = noteTokenIndex + 1; i < tokens.size(); ++i)
    {
        if (auto v = parseAsVelocity (tokens[i]))
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
            if (auto v = parseAsExplicitVelocity (tokens[i]))
            {
                velLayer = *v;
                break;
            }
        }
    }

    return ParsedName { *midiNote, velLayer };
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
        };
    }

    int runTests()
    {
        const std::vector<TestCase> cases = {
            // Scientific pitch + velocity (case-insensitive)
            { "C4_v1",         60,        0      },
            { "c4_v1",         60,        0      },
            { "F#3_v2",        54,        1      },
            { "Gb5_vel3",      78,        2      },
            { "A-1_v1",         9,        0      },
            // MIDI form
            { "MIDI60_v1",     60,        0      },
            { "midi72_layer2", 72,        1      },
            // Bare-integer fallback
            { "60_v1",         60,        0      },
            // Unparseable
            { "weird-name",    std::nullopt, std::nullopt },
            // Default vel (no velocity token)
            { "C4",            60,        0      },
            // Dynamics tokens
            { "C4_p",          60,        0      },
            { "C4_mp",         60,        1      },
            { "C4_mf",         60,        2      },
            { "C4_f",          60,        3      },
            // Order-independent parse
            { "Lyr3_C4",       60,        2      },
            { "C4_L4",         60,        3      },
            // Extra leading token (filler)
            { "Sample_C4_v1",  60,        0      },
            // Mixed separators
            { "C4-v2.something", 60,      1      },

            // Phase 2.5 reopen — pre-note dynamics must NOT match.
            // Real-world filename from the user's `vln_long_mp` library —
            // previously matched "mp" → velLayer=1 (silenced the library
            // below MIDI vel 65). Now resolves to velLayer=0.
            { "vln_long_mp-D#3-V127-T6N6", 51, 0 },
            { "vln_long_mp-C4-V127-EHGV",  60, 0 },
            // ...and the new-style "Auto Sampled Instrument" form (parser-clean).
            { "Auto Sampled Instrument-D#3-V127-RJHU", 51, 0 },
            // Dynamics token in PRE-note position is now skipped.
            { "mp_C4",     60, 0 },
            // Dynamics token in POST-note position still works.
            { "C4_mp",     60, 1 },
            // Explicit forms in PRE-note position still work (regression guard
            // for the existing `Lyr3_C4` / `L4_C4` conventions).
            { "L3_C4",     60, 2 },
            { "vel2_C4",   60, 1 },
        };

        int passed = 0, failed = 0;
        for (const auto& tc : cases)
        {
            auto result = parse (juce::String (tc.input));
            const bool ok =
                (! tc.expectedMidi.has_value() && ! result.has_value())
                || ( tc.expectedMidi.has_value() &&  result.has_value()
                    && result->midiNote == *tc.expectedMidi
                    && result->velLayer == *tc.expectedVelLayer);

            if (ok) { ++passed; }
            else
            {
                ++failed;
                std::cout << "FAIL: '" << tc.input << "' -> ";
                if (result.has_value())
                    std::cout << "(midi=" << result->midiNote
                              << ", vel=" << result->velLayer << ")";
                else
                    std::cout << "<none>";
                std::cout << "  expected ";
                if (tc.expectedMidi.has_value())
                    std::cout << "(midi=" << *tc.expectedMidi
                              << ", vel=" << *tc.expectedVelLayer << ")";
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
