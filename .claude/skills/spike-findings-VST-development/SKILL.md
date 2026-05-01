---
name: spike-findings-VST-development
description: Validated patterns, constraints, and implementation knowledge from spike experiments on the Ouaricon VST plugin suite. Auto-loaded during implementation work. Currently covers VST3 Note Expression for Dorico microtonal playback (applicable to O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant — any pitched plugin).
---

<context>
## Project: VST-development

Ouaricon Audio plugin repository. Collection of JUCE 8-based VST3 / AU / Standalone plugins sharing a common module system (aesthetic templates, preset manager, licensing, etc.).

Spike sessions wrapped: 2026-04-22 → 2026-04-23 (Dorico microtonal playback research).
</context>

<findings_index>
## Feature Areas

| Area | Reference | Key Finding |
|------|-----------|-------------|
| VST3 Note Expression for Dorico microtonal playback | [references/vst3-note-expression-dorico.md](references/vst3-note-expression-dorico.md) | Dorico's tonality system drives per-note tuning via `kTuningTypeID` NE events. JUCE 8.0.4 drops them — a local JUCE patch (2 files, Approach 2: side-channel queue on `VST3ClientExtensions`) + plugin-side NEC wiring + voice-side `exchange` consume in `startNote` produces sample-accurate microtones end-to-end. **Non-obvious UX trap:** Dorico's "Auto" microtonality picks pitch bend / VST2 detune for non-Steinberg VST3s — users must create a custom expression map with Microtonality = "VST3 Note Expression". |

## Source Files

Original spike READMEs and extracted source snippets are preserved in `sources/` for complete reference. Key entry points:

- `sources/shared-code/juce-patch.md` — exact hunks to reapply to local JUCE after upgrades
- `sources/shared-code/NoteExpressionSupport.spike.h` — NEC + extensions class (strip diagnostic trace before production merge)
- `sources/shared-code/processor-drain.cpp` — drain + NE→pitch correlation for processBlock
- `sources/shared-code/voice-startNote.cpp` — voice-side tuning application before DSP trigger

</findings_index>

<usage_guidance>
Consult this skill when:
- Adding microtonal support to any Ouaricon pitched plugin (the pattern generalizes — extraction into a shared module is the likely next step).
- Reapplying the JUCE patch after a JUCE upgrade (`grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/` should return 4 hits).
- Debugging "Dorico sends microtones but plugin plays 12-TET" — 90% chance the user's expression map Microtonality isn't set to "VST3 Note Expression".
- Adding new VST3-only host features that require the `VST3ClientExtensions::queryIEditController` pattern.

Do NOT use this skill for:
- MTS-ESP integration (orthogonal path, different protocol, not suitable for Dorico's per-note deltas — see upstream research doc).
- VST2 detune (we don't build VST2).
- Pitch-bend-based microtuning fallback for AU builds.
</usage_guidance>

<metadata>
## Processed Spikes

- 001-patch-build-load
- 002-quarter-sharp-end-to-end
- 003-attack-transient-check
</metadata>
