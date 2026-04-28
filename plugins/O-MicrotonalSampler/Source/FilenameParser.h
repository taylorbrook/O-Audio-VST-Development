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
