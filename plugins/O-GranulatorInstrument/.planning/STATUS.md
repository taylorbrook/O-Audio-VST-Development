---
plugin: O-GranulatorInstrument
stage: 0
status: complete
last_updated: 2026-11-07
complexity_score: 5.0
complexity_tier: 6
staged_implementation: true
orchestration_mode: true
next_action: ui_mockup_then_parameter_spec
next_stage: 1
ready_for_implementation: false
blocked_on: mockup + LOCKED parameter-spec.md (Design Sync S1-S11); Phase 2.1 also waits for O-Granulator 3.3 to close (fork pin)
latest_mockup_version: 1
mockup_finalized: false
contract_checksums:
  brief: sha256:31c873f27b5eafaee2e1574406d0bea2d645a5fc85c72721a7876fdc5fb81c86
  requirements: sha256:ea9deda69106e02499b14601052eeefbe2e4d6742c7661e5240760f3dd56d54b
  parameter_spec_draft: sha256:3c4918748a4375940684a22ed6850a3184bdd96df53d152a8900d5f0e4260796
  architecture: sha256:8c44e54f86994b1b84e1f8a516a95215d654d552aaa9e4b406eff8a8ed241f79
  roadmap: sha256:ca57fdb8f1fbeaa8abad8d40be8e37a84e8a96491e31cb786c1fcc54d738caf0
---

# O-GranulatorInstrument Status

## Current Position

Stage: 0 of 4 (Research & Planning) — complete
Status: Research & Planning complete. Ready for implementation once the mockup and the LOCKED parameter-spec.md exist.
Progress: [##..................] 10%

## Completed So Far

**Ideation:** ✓ Complete (BRIEF, REQUIREMENTS, parameter-spec-draft with 261 params)

**Stage 0:** ✓ Complete. Research & Planning: ARCHITECTURE.md and ROADMAP.md written (complexity 5.0).
- Plugin type: Synth (sample-based polyphonic granular, MIDI/MPE/NE, sidechain Record-in)
- Professional examples researched: 5 (Granulator III, Pigments, Clouds/Beads, Thor, Logic EVOC/AU side-chain)
- JUCE / modules: juce_audio_formats, juce_dsp, juce_audio_utils, WebView; grain-engine 1.1.0, pitch-collections 1.0.0, reverb-unit 1.2.0, note-expression 1.1.1, webview-drop-streaming 1.0.0, preset-manager
- Decisions D1–D7 made (see ARCHITECTURE.md "Stage 0 Decisions"):
  - D1 fork the leaves, converge later
  - D2 30 s per slot / 2.88 M-frame cap
  - D3 grain_limit ≤ 256, 40 % budget, onset thinning
  - D4 newest-voice rule
  - D5 Dorico NE in v1
  - D6 Record-in decided by signal presence
  - D7 1× record head, convex write
- Strategy: Staged implementation (Stage 2 × 6 phases, Stage 3 × 3 phases)

## Next Steps

1. UI mockup (`/start O-GranulatorInstrument` → option 3). Apply ARCHITECTURE.md "Design Sync" S1–S11. **v1 created 2026-11-07** (`.planning/mockups/v1-ui.yaml` + `v1-ui-test.html`, 261 params, S1–S11 mapped in `design_sync_applied`); in design iteration, not finalized.
2. Write the LOCKED `parameter-spec.md` from the finalized mockup. Freeze the 126 destinations and 18 sources.
3. Stage 1: Foundation (`/implement O-GranulatorInstrument`).
4. Before Phase 2.1: confirm O-Granulator 3.3 has closed, then fork the leaves pinned to that commit.

## Context to Preserve

- Architecture: `plugins/O-GranulatorInstrument/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-GranulatorInstrument/.planning/ROADMAP.md`
- Stage 0 context: `plugins/O-GranulatorInstrument/.planning/stages/0-ideation/CONTEXT.md`
- Complexity 5.0 (raw 27.0), tier 6, phased
- Parent: O-Granulator @ 3043b72a (fork pin moves to the 3.3-closing commit)
- Design Sync changes (no param IDs added or removed; still 261):
  - grain_limit 32/64/128/256
  - pitch_bend_range = MPE master only (members use RPN 0)
  - feedback = convex write amount
  - ±48 st clamp
  - <Slots> state keys
  - factory sources embedded as BinaryData

## Files Created
- plugins/O-GranulatorInstrument/.planning/research/ARCHITECTURE.md
- plugins/O-GranulatorInstrument/.planning/ROADMAP.md
- plugins/O-GranulatorInstrument/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-GranulatorInstrument/.planning/BRIEF.md (ideation)
- plugins/O-GranulatorInstrument/.planning/REQUIREMENTS.md (ideation)
- plugins/O-GranulatorInstrument/.planning/parameter-spec-draft.md (ideation, 2026-11-07)
- plugins/O-GranulatorInstrument/.planning/mockups/v1-ui.yaml (UI mockup v1, 2026-11-07)
- plugins/O-GranulatorInstrument/.planning/mockups/v1-ui-test.html (UI mockup v1 browser test, 2026-11-07)
