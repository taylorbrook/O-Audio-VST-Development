/*
  ==============================================================================

    FilenameParser.h
    Microtonal Sample Engine - Tolerant filename parser (Phase 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

    Pure-function parse() recognises the 6 filename conventions called out in
    BRIEF.md / D2-7 / RESEARCH RQ-5:

      * Scientific pitch: C4, c4, F#3, Bb5, A-1
      * MIDI form:        MIDI60, midi72
      * Bare integer:     60 (only when no other token parses as a note)
      * Velocity v[1-4]/vel[1-4]/V[1-4]/Vel[1-4]
      * Dynamics:         p / mp / mf / f → 0 / 1 / 2 / 3
      * Layer:            layer[N] / L[N] / Lyr[N]

    Tokenizer splits on `[_\-\s.]+`. Case-insensitive.

    Velocity scan ordering (Phase 2.5 reopen — bug fix):
      The velocity scan is two-tier to suppress false positives from dynamics
      letters appearing inside instrument-name substrings.

        Tier 1 — POST-note tokens accept any velocity form (explicit + dynamics).
        Tier 2 — PRE-note tokens accept ONLY explicit forms (v[1-4]/vel[1-4]/
                 layer[N]/L[N]/lyr[N]); dynamics letters (p/mp/mf/f) are skipped.

      Why: filenames like `vln_long_mp-D#3-V127-T6N6.aif` previously matched
      "mp" → velLayer=1 (a layer with no actual slots) and silenced the
      library below MIDI velocity 65. The new scan returns velLayer=0 for
      such filenames; conventional `[note]_[dyn]` filenames (e.g.
      `C4_mp.wav`) still resolve correctly via the post-note tier.

    On parse failure: returns std::nullopt; caller (SampleLoader::run()) is
    responsible for logging the skip via `juce::StringArray skippedFiles`.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <optional>

namespace FilenameParser
{
    struct ParsedName
    {
        int midiNote;   // 0..127
        int velLayer;   // 0..3 (0 = pp/v1, 3 = ff/v4); default 0 if no velocity token
    };

    // Parse a filename (without extension). Returns nullopt if no MIDI note
    // can be recovered from the tokens. Velocity defaults to 0 if no velocity
    // token is found.
    std::optional<ParsedName> parse (const juce::String& filenameNoExtension);
}
