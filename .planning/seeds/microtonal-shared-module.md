---
name: Extract microtonal support into shared Ouaricon module
description: After O-Lyrica proves the VST3 Note Expression pattern works end-to-end in Dorico, extract into a shared module so every pitched plugin inherits microtonal support cheaply
type: seed
planted_date: 2026-04-22
trigger_condition: O-Lyrica microtonal spike verified working in Dorico
priority: medium
related_notes: [dorico-microtonal-vst-research.md]
---

# Seed: Microtonal shared module

Once the VST3 Note Expression (`kTuningTypeID`) pattern is proven working on O-Lyrica, extract the reusable pieces into a shared Ouaricon module — the repo already has a `module-system` for this kind of cross-plugin reuse.

## Trigger

O-Lyrica spike passes — specifically:
- JUCE patch applied and stable
- `INoteExpressionController` advertised correctly (Dorico discovers it on plugin load)
- Custom accidentals in Dorico produce correct pitch offsets on playback
- No pitch glides/zippers at note attack

## What to extract

Candidate module name: `OuariconMicrotonal` (or similar)

| Piece | Goes in module? |
|---|---|
| `TuningNoteExpressionController` class (INE controller boilerplate) | Yes — identical across plugins |
| `VST3ClientExtensions` subclass with `queryIEditController` wiring | Yes — identical across plugins |
| The JUCE patch to `juce_VST3Common.h` | **No** — lives in local JUCE fork / as a patch file in the repo, applied at build time |
| `NoteExpressionBridge` SysEx encode/decode (if SysEx tunneling approach) | Yes — the protocol between patch and plugin |
| Voice-side tuning storage (`vst3NoteId`, `tuningSemitones` on a base voice class?) | **Depends** — each synth's voice struct is different. Might expose as a mixin/helper rather than a base class. |
| NE event → voice routing helper | Yes — generic logic (match noteId, apply semitones) |

## Target plugins

Every pitched plugin in the repo should eventually inherit this:
- **Monophonic / melodic**: O-Lyrica (spike), O-Wind, O-Reed, O-Bowed, O-Formant
- **Polyphonic**: O-Bells, and anything else with chord capability (audit during extraction)
- **Skip**: percussive/noise-based plugins without definite pitch

## Maintenance consideration

The JUCE patch is the ongoing cost. Track it as a named patch file in `scripts/` or similar, re-apply whenever JUCE updates. Consider a CI check that verifies the patched JUCE is in use.

## Why it's a seed (not a todo)

Premature to create tasks until the spike validates the approach. If the spike reveals JUCE complications we didn't anticipate (e.g., thread-safety issues with the side-channel queue, or Dorico sending NE events at unexpected sample offsets), the extractable pattern may look different. Plant now, act after the spike.
