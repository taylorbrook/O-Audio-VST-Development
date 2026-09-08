---
plugin: O-Strata
stage: 0
stage_name: ideation
phase: replan
status: replanned_live_terrain_awaiting_stage_0
last_updated: 2026-09-08
workflow_mode: manual
complexity_score: null
staged_implementation: true
orchestration_mode: true
next_action: /plan O-Strata
next_stage: 0
ready_for_implementation: false
contract_checksums: {}
mockup_latest_version: 0
ui_design_phase_complete: false
mockup_finalized: false
brief_updated_from_mockup: false
ui_scaffolding_phase_complete: false
fork_exists: true
fork_verified: "stages/1-foundation/VERIFICATION.md (baked parameter set, 2026-09-07)"
superseded_design: superseded-baked-v1/
---

# O-Strata Status

## Current Position

Stage: 0 — **re-planned 2026-09-08** around the live wave-terrain oscillator (research §7.2); the baked-geometry design is superseded (`superseded-baked-v1/README.md`) and its tables ship as an O-Prism factory bank (`plugins/O-Prism/.planning/improvements/geometry-wavetables.md`).
Status: BRIEF.md, REQUIREMENTS.md (v2.0.0, 28 IDs) and parameter-spec-draft.md (v2: 171 inherited + 34 new = 205; 46 mod destinations) rewritten. Stage 0 (ARCHITECTURE + ROADMAP) not yet run for the new design.
Progress: [####................] 20% (fork + rename + strip verified; parameters and DSP to redo)

## Why (evidence)

- `evidence/critique-check-2026-09-08.md` — measured on the prototype: centred orbits over symmetric fields play an octave / a twelfth up; mesh sweeps are spectrally static; fBm is the only rich baked source.
- `evidence/replan-proposal-2026-09-08.md` — the sequence below and what carries over.
- Listening (Taylor, 2026-09-08): the baked tables sound nice but do not warrant a new plugin.

## What carries over from the built fork (`Source/`, verified 2026-09-07)

Full `Strata` rename, wavetable library/editor removed, `JUCE_WEB_BROWSER`-guarded editor, `juce_cryptography` linked, UI gate fixtures, headless smoke harness pattern (`stages/1-foundation/smoke/`), CHANGELOG/NOTES/PLUGINS.md plumbing. **The 48 baked-geometry parameters in `Source/` are stale** and are replaced in the Stage 1 re-parameterise pass; `params.tsv` (219 rows) describes the *current binary*, not the target.

## Phase Progress

### Stage 0: Research & Planning (live terrain design)
| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| re-plan brief | ✓ | 2026-09-08 | BRIEF / REQUIREMENTS v2 / parameter-spec-draft v2 |
| research + plan (`/plan`) | | | ARCHITECTURE.md + ROADMAP.md |
| mockup v2 | | | `design UI for O-Strata`; lock parameter-spec.md v2 |

### Stage 1: Foundation — first pass ✓ (baked params, 2026-09-07); **second pass pending** (re-parameterise: 205 params, 46 mod destinations, COMPAT-01 re-verify)

## Next Steps

1. (Recommended) Play Aaron Anderson's *Terrain* for an hour — confirm the live sound is the one wanted before planning it.
2. `/plan O-Strata` — Stage 0 for the live design. Research items the old plan never needed: feedback stability + damping, pitch-tracked spatial frequency vs Chebyshev per-pitch truncation, PNG → Chebyshev projection, per-osc halfband decimator + latency, `WavetableOscillator` interface parity, mod-destination append, Bandlimited-mode coefficient swap cadence, Quality default.
3. UI mockup v2 → `parameter-spec.md` v2 (locked).
4. Stage 1 second pass: set `stage: 1`, `phase: discuss` here and run `/plugin-discuss O-Strata 1-foundation`.
5. Stage 2: live oscillator DSP with the aliasing / CPU / symmetry harness gates.

## Context to Preserve

- Research: `research/wavetable-synthesis-3d-geometry.md` §1, §7.2, §7.3; benchmark `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/`; view prototype `webgl-3d/terrain-proto.html`
- Superseded design (for the v1.1 Baked source type): `superseded-baked-v1/` (ARCHITECTURE, ROADMAP, spec v1, mockup v1, Stage 0 + Stage 2 CONTEXT)
- Fork history: `stages/1-foundation/` (CONTEXT D1–D6, RESEARCH, PLAN, SUMMARY, VERIFICATION, smoke harness)
- Symmetry rule and harness gate: REQUIREMENTS DSP-06
- Out of scope v1.0: baked sources (v1.1), wavetable mode (never), RGB terrains (v1.1), dual-orbit stereo (v1.x)

## Files
- plugins/O-Strata/Source/** (fork, baked parameter set — to be re-parameterised), CMakeLists.txt, tests/**, CHANGELOG.md, .planning/params.tsv (current binary)
- plugins/O-Strata/.planning/{BRIEF.md, REQUIREMENTS.md, parameter-spec-draft.md} (v2, 2026-09-08)
- plugins/O-Strata/.planning/superseded-baked-v1/** ; .planning/evidence/*.md (WAVs gitignored)
- PLUGINS.md (row → 🚧 Stage 0)
