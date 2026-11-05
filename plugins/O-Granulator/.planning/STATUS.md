---
plugin: O-Granulator
stage: 2
status: stage_2_phase_2_1_research_complete
current_phase: research
current_dsp_phase: 2.1
stage_0_status: ui_design_complete
last_updated: 2026-11-04
latest_mockup_version: 2
mockup_finalized: true
finalized_version: 2
brief_updated_from_mockup: true
complexity_score: 5.0
complexity_tier: 6
staged_implementation: true
orchestration_mode: true
next_action: stage_2_phase_2_1_plan
next_stage: 2
ready_for_implementation: true
blocked_on: null
contract_checksums:
  brief: sha256:f9c748a8a256de9d2fb2bcbe1aff58b12999a640cc8d36fbdbf96e6deb5559d7
  requirements: sha256:b9ba9106585001342afa9818ab895602eb351c23bf7ec82874208dcba536495d   # COMPAT-01 → complete (Stage 1 verify)
  parameter_spec_draft: sha256:d0b1457ea7c1624ff757fd12c1acb7bd62c9acf7bebbdd3460c7f1d30cdbbce3
  parameter_spec: sha256:f5ce35954c2c18e46faa914d95898ddb718d6b77ec4be8582be23b60a26b3688   # P3 uiLanguage amendment (Stage 1)
  architecture: sha256:f0cf753b4de2cb6099ad327b0887a0c254f013746b373000a28f01ad7949d9d9
  roadmap: sha256:79d69576f70776e92ce498be990e1f3cbdc09e03a081563443545f9650c14b20
---

# O-Granulator Status

## Current Position

Stage: 2 of 4 (DSP) — Phase 2.1 (Ring + pass engine + one stream) discuss ✓ · research ✓ 2026-11-04 (RESEARCH-2.1.md: R1 read-fraction quantisation for the bitwise null, R2 0 dB snap + mix-0 bypass, R3 plugin-side grain_limit cap, R4 onset epoch across re-enable)
Status: O-Granulator 0.1.0 shell builds VST3 + AU + Standalone; 220 params, pass-through, state codec; pluginval 10 + auval PASS. pitch-collections 1.0.0 module and O-GrainScatter 3.6.0 landed with it.
Progress: [#####...............] 25% (Stage 1 verified — VERIFICATION.md)

## Completed So Far

**Ideation:** ✓ Complete. Brief, 30 requirements, and a draft param spec (214).

**UI mockup:** ✓ v2 FINALIZED (2026-11-04). `.planning/mockups/v2-ui.yaml` is the UI source of truth (`finalized: true`); BRIEF.md UI Concept synced from it (Phase 5.6). It covers all 220 params, incl. full microtonal scales modelled on O-GrainScatter v3.0.0: `sN_scale` has 32 choices (Off, GrainScatter's collections without Free, Custom last), a grouped dropdown, per-stream degree chips and ONE custom scale shared by all streams (state, not a param).

**Implementation scaffolding (Phase B):** ✓ `mockups/v2-ui.html` (production page: Family A knobs, canonical i18n en/fr/zh-Hans, hover-help switch), `v2-i18n.js`, `v2-PluginEditor.h/.cpp` (220 relays → webView → 220 attachments, 21 native fns, Windows WebView2 block), `v2-CMakeLists.txt` (complete plugin CMakeLists), `v2-integration-checklist.md`. Pre-verified on a scratch tree: check-i18n ALL PASS, fr lint clean, zh lint 0 findings, check-ui-labels 0 failures (all pages/popovers forced visible), boot-all-uis --strict-tips clean.

**Parameter contract:** ✓ `.planning/parameter-spec.md` LOCKED — 220 params (140 Float / 59 Choice / 21 Bool), 113-entry append-only mod-destination table, non-parameter state (shared custom scale, per-stream × per-collection uint64 degree masks, seed, table version, uiLanguage), PitchCollections → shared module `modules/tuning/pitch-collections` (provisional).

**Stage 0:** ✓ Research & Planning complete. ARCHITECTURE.md and ROADMAP.md are documented (Complexity 5.0, raw 22.0, tier 6, DEEP).
- Plugin type: Audio Effect (live-input granular, 4 streams, shared pool)
- Professional references: Clouds/Beads, Granulator III, Efx Fragments, Output Portal, Bitwig/Ableton modulators
- JUCE: juce_audio_utils, juce_dsp (FFT for MPM, ReverbUnit), WebView; grain-engine v1.1.0 (MipRing<2>, GrainPool, GrainScheduler, CounterRng, WindowLut); reverb-unit v1.1.0; preset-manager
- Strategy: phased. Stage 2 has 6 phases, Stage 3 has 3.

## Next Steps

1. Stage 2: DSP (`/plugin-discuss O-Granulator 2-dsp`) — 6 phases per ROADMAP. Carry-ins from Stage 1: sort the active pitch table (7-Limit Just unsorted at source); audio thread owns its own custom-table copy; never derive unity gain from `get()` on skewed ranges.
2. Stage 3 integrates the UI with `mockups/v2-integration-checklist.md` (+ check-ui-labels, deferred from Stage 1).

## Context to Preserve

- Architecture: `plugins/O-Granulator/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-Granulator/.planning/ROADMAP.md`
- Stage 0 decisions: `plugins/O-Granulator/.planning/stages/0-ideation/CONTEXT.md`
- Key decisions:
  - Absolute 32-sample pass grid with a 32-sample feedback return (write-before-read).
  - Freeze is a head-stop and wins over feedback (no overdub in v1.0).
  - Coherent feedback-tap law plus 0.95 cap plus tanh plus AGC plus wet safety.
  - MPM pitch follower on the audio thread.
  - Append-only 113-destination matrix.
  - Reported latency is 0.
  - CPU budget: ≤ 30 % of one core @ 48 k/128 worst case.
- Module gaps (optional): G1 WindowMorphBank, G2 GrainPool::fadeOut, G3 GrainScheduler::rescheduleIn, G4 ReverbUnit raw-atomic bind (critical path; fallback is a local copy)

## Files Created
- plugins/O-Granulator/.planning/parameter-spec.md (LOCKED)
- plugins/O-Granulator/.planning/mockups/v2-ui.html, v2-i18n.js, v2-PluginEditor.h, v2-PluginEditor.cpp, v2-CMakeLists.txt, v2-integration-checklist.md
- plugins/O-Granulator/.planning/research/ARCHITECTURE.md
- plugins/O-Granulator/.planning/ROADMAP.md
- plugins/O-Granulator/.planning/stages/0-ideation/CONTEXT.md

## Phase Progress

### Stage 2: DSP — Phase 2.1 (Ring + pass engine + one stream)
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-11-04 | |
| research | → | | |
| plan | | | |
| execute | | | |
| verify | | | |

Decisions: grain-engine 1.1.0 unmodified (G2/G3 local workarounds, promote after ship); ring 41 s at all rates (no ≥96 k clamp); human listening at end of 2.1 and 2.4 only. Stage 1 harness B1–B3 rescope to mix 0 % (mix default 50). See `stages/2-dsp/CONTEXT-2.1.md`.

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-11-04 | |
| research | ✓ | 2026-11-04 | |
| plan | ✓ | 2026-11-04 | |
| execute | ✓ | 2026-11-04 | |
| verify | → | | |

Decisions: PitchCollections extract + migrate O-GrainScatter (→3.6.0) in Stage 1; GenericAudioProcessorEditor shell; all three bus layouts. See `stages/1-foundation/CONTEXT.md`.

Execute (2026-11-04): see `stages/1-foundation/SUMMARY.md` — W1 pitch-collections 1.0.0 (229/229), W2 GS 3.6.0 (pitch-null 415/415, 190 renders byte-identical), W3 shell (render harness 47/47 + golden negative control), W4 pluginval 10 VST3 ×3 + AU, auval PASS.
