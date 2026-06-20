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

    Velocity scan ordering (v1.17.2 — pre-note dynamics honoured):
      The velocity scan is two-tier by POSITION only — both tiers accept any
      velocity form, including dynamics (p/mp/mf/f).

        Tier 1 — POST-note: first velocity token AFTER the note wins.
        Tier 2 — PRE-note:  if no post-note match, first velocity token BEFORE
                 the note wins (dynamics included).

      Why pre-note dynamics matter: the dominant orchestral naming convention
      places the dynamic immediately before the pitch, e.g.
      `vln_norm_mf-A#2-V127-JXRO.aif` → velLayer=2 (mf).

      History: v1.14.0–v1.17.1 suppressed dynamics in the PRE-note tier to
      stop a single-dynamic library (e.g. all `mp`) from landing every slot on
      a non-zero layer and silencing the bottom of the velocity range — the
      voice bailed to silence on an empty layer. That guard now lives at the
      voice instead: SampleMap::findCellNearestLayer falls back to the nearest
      populated layer, so honouring pre-note dynamics can no longer silence a
      library. Pre/post-note dynamics are therefore symmetric again.

    On parse failure: returns std::nullopt; caller (SampleLoader::run()) is
    responsible for logging the skip via `juce::StringArray skippedFiles`.

  ==============================================================================
*/

#pragma once
// Specific juce_core include (instead of the build-system-generated
// <JuceHeader.h>) so this header is consumable by standalone test
// executables that don't go through juce_add_plugin (e.g.
// technique_parse_check.cpp). Pattern matches SampleMap.h.
#include <juce_core/juce_core.h>
#include <optional>

namespace FilenameParser
{
    struct ParsedName
    {
        int midiNote;   // 0..127
        int velLayer;   // 0..3 (0 = pp/v1, 3 = ff/v4); default 0 if no velocity token
        // v1.8.0: round-robin index from explicit rr[N] / take[N] / tk[N]
        // tokens. -1 = no explicit RR token (caller may treat duplicates as
        // ambiguous and surface the modal-confirm flow). 0-based: rr1 → 0,
        // take2 → 1, tk03 → 2, etc. Capped at 0..63 (way over any realistic
        // RR set size; defends against malicious filenames).
        int rrIndex;
        // v1.14.0: technique slot inferred from filename. -1 = no technique
        // token recognised (caller defaults to 0 / "ord"). 0-based, capped
        // at kMaxTechniques-1.
        //
        // Recognised tokens (case-insensitive, must appear as their own
        // delimited token):
        //   _ord_      → 0 (ordinario)
        //   _sp_       → 1 (sul ponticello)
        //   _st_       → 2 (sul tasto)
        //   _sv_       → 3 (senza vibrato)
        //   _cs_       → 4 (con sordino)
        //   _pizz_     → 5 (pizzicato)
        //   _harm_     → 6 (natural harmonic)
        //   _mart_     → 7 (martelé / martellato)
        //   _trem_     → reserved (8) — outside the 8-slot KS range; only
        //                surfaces when the user grows the technique count
        //   _flaut_    → reserved (9) — same caveat as trem
        // The two reserved tokens map onto slots that exceed the default
        // KS range; the parser still returns them so a user that has grown
        // technique_count past 8 can pull them in via filename auto-detect.
        int techniqueIndex;
    };

    // Parse a filename (without extension). Returns nullopt if no MIDI note
    // can be recovered from the tokens. Velocity defaults to 0 if no velocity
    // token is found. RR index defaults to -1 if no rr/take/tk token is found.
    // Technique index defaults to -1 if no technique token is found.
    std::optional<ParsedName> parse (const juce::String& filenameNoExtension);
}
