---
plugin: O-GranulatorInstrument
stage: ideation
status: creative_brief_complete
last_updated: 2026-11-06 00:00:00
---

# Resume Point

## Current State: Creative Brief Complete

The creative brief for O-GranulatorInstrument is finalized. Next is Stage 0 planning or a UI mockup.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined: a polyphonic, MIDI-driven, sample-based sibling of O-Granulator
- Parameter deltas vs O-Granulator specified
- UI direction captured (inherits the O-Granulator v2 language)
- Use cases identified
- Requirements extracted with acceptance criteria

## Next Steps

1. Stage 0 planning (`/plan O-GranulatorInstrument`). Key decision: share the O-Granulator core as a module, or fork it.
2. UI mockup (`/start O-GranulatorInstrument` → option 3)

## Context to Preserve

**Key Decisions:**
- Plugin type: Synth (instrument)
- MIDI model: polyphonic clouds (per-note voice, key-tracked from the per-stream root)
- Sources: one file per stream (4 slots), with a small factory set
- Kept from O-Granulator: feedback (into working copies, Freeze/Reset), per-grain FX, reverb, scales, mod matrix
- Replaced: the input followers become amp/mod ADSRs, velocity, key, mod wheel, aftertouch/MPE and note-random
- Added: record-in from the sidechain into a slot (hybrid)

**Files Created:**
- plugins/O-GranulatorInstrument/.planning/BRIEF.md
- plugins/O-GranulatorInstrument/.planning/REQUIREMENTS.md
