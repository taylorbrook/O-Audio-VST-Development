---
plugin: O-GranulatorInstrument
stage: 2
status: stage_2_phase_2_1_discuss_complete
current_phase: 2-dsp/2.1
gsd_phase: discuss_complete
last_updated: 2026-11-07
complexity_score: 5.0
complexity_tier: 6
staged_implementation: true
orchestration_mode: true
next_action: stage_2_research
next_stage: 2
ready_for_implementation: true
blocked_on: none (fork pin 82406fce, D11)
latest_mockup_version: 1
mockup_finalized: true
finalized_version: 1
stage_0_status: ui_design_complete
ui_scaffolding_phase_complete: true
ui_scaffolding_timestamp: "2026-11-07"
parameter_spec_locked: true
brief_updated_from_mockup: true
mockup_version_synced: 1
brief_update_timestamp: "2026-11-07T19:40:17Z"
contract_checksums:
  parameter_spec: sha256:b6bc70e3a9dcd2611f241452b893f8318956cb946b1ac0694fe3957837ca751b
  parameter_spec_body: sha256:c5553e0ccda2c7f5320a1891ba20c5a92913e089ab1fa1e47b0e33fa7c543191
  brief: sha256:74e7f8f24468ed9658aa8533733f45da24c61ad116d6ae47cd5c2af0e8524722
  requirements: sha256:c0a89b3d12fa334a732e15ba788713941f933aa32416be4cddf53dc988be281e   # Stage 1 verify: COMPAT-01 complete
  parameter_spec_draft: sha256:3c4918748a4375940684a22ed6850a3184bdd96df53d152a8900d5f0e4260796
  architecture: sha256:8c44e54f86994b1b84e1f8a516a95215d654d552aaa9e4b406eff8a8ed241f79
  roadmap: sha256:ca57fdb8f1fbeaa8abad8d40be8e37a84e8a96491e31cb786c1fcc54d738caf0
---

# O-GranulatorInstrument Status

## Current Position

Stage: 2 of 4 (DSP) — Phase 2.1 discuss complete (`stages/2-dsp/CONTEXT-2.1.md`: D11 fork pin O-Granulator 82406fce, zero drift, D43 + D47a included; D12 water-fill + onset thinning on the voice axis in 2.1; D13 `OGI_DEV_LOADER` dev-loader editor until 3.1). Next: research 2.1.
Stage 1: VERIFIED
Status: Stage 1 executed (74606bb3): 0.1.0 VST3 + AU + Standalone built; 261 params (176/70/15), dest 126 / src 18 goldens; harness 55 PASS / 0 FAIL + golden and spec negative controls; pluginval 10 VST3 ×3 + AU SUCCESS with SC active (2 ch) and gate variant inactive (0 ch), auval PASS [1, 2] [2, 2] (frozen); D9 contingency did not fire; shipping build restored + installed. See stages/1-foundation/SUMMARY.md. Verified 2026-11-07 (stages/1-foundation/VERIFICATION.md): independent re-run on the installed shipping build — goldens + param-dump byte-identical, harness 55/0, auval PASS [1, 2] [2, 2], pluginval 10 VST3 + AU SUCCESS (2 ch in); COMPAT-01 complete. Next: Stage 2 discuss (Phase 2.1 fork).
Progress: [####................] 20%

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

1. ✓ UI mockup v1 finalized 2026-11-07 (Phase 5.5); Phase B scaffolding generated: `v1-ui.html`, `v1-i18n.js`, `v1-PluginEditor.{h,cpp}` (261 relays, 34 native fns, sidechain-aware Record-in telemetry), `v1-CMakeLists.txt`, `v1-integration-checklist.md`, `v1-i18n-states.json`. UI gates pre-verified on a scratch tree (check-i18n / fr / zh / check-ui-labels / boot-all-uis all clean).
2. ✓ `parameter-spec.md` LOCKED (Stage 1 contract): 261 params, Design Sync S1–S11 applied, 126-entry destination order and 18-entry source order frozen (append-only). One description-level draft conflict recorded in its "Draft vs mockup" section (`polyphony` steal wording) — resolved Stage 1 discuss D10: keep mockup wording.
3. Stage 1: Foundation — discuss ✓ (`stages/1-foundation/CONTEXT.md`), research ✓ (`stages/1-foundation/RESEARCH.md`), plan ✓ (`stages/1-foundation/PLAN.md`); execute ✓ (`SUMMARY.md`, 74606bb3), verify ✓ (`VERIFICATION.md`). Next: `/plugin-discuss O-GranulatorInstrument 2-dsp` (Phase 2.1 fork pin).
4. Stage 2 Phase 2.1: discuss ✓ (`stages/2-dsp/CONTEXT-2.1.md`). Fork pin = O-Granulator `82406fce` (D11, supersedes the 3.3-close pin). Next: `/plugin-research O-GranulatorInstrument 2-dsp`.

## Context to Preserve

- Architecture: `plugins/O-GranulatorInstrument/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-GranulatorInstrument/.planning/ROADMAP.md`
- Stage 0 context: `plugins/O-GranulatorInstrument/.planning/stages/0-ideation/CONTEXT.md`
- Stage 1 context: `plugins/O-GranulatorInstrument/.planning/stages/1-foundation/CONTEXT.md`
- Stage 1 plan: `plugins/O-GranulatorInstrument/.planning/stages/1-foundation/PLAN.md` (authoritative for execute)
- Stage 1 research: `plugins/O-GranulatorInstrument/.planning/stages/1-foundation/RESEARCH.md` (open: uiLanguage on state root not <Engine>; skip D10 header note; note-expression registry 1.1.1 vs disk 1.1.2)
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
- plugins/O-GranulatorInstrument/.planning/mockups/v1-ui-test.html (UI mockup v1 browser test, 2026-11-07; gitignored)
- plugins/O-GranulatorInstrument/.planning/parameter-spec.md (LOCKED, 2026-11-07)
- plugins/O-GranulatorInstrument/.planning/mockups/v1-ui.html, v1-i18n.js, v1-PluginEditor.h, v1-PluginEditor.cpp, v1-CMakeLists.txt, v1-integration-checklist.md, v1-i18n-states.json (ui-finalization, 2026-11-07)
