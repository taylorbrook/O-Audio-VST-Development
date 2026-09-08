---
plugin: O-Strata
stage: 0
stage_name: ideation
phase: mockup_v2
status: stage_0_complete_awaiting_mockup_v2
last_updated: 2026-09-08
workflow_mode: manual
complexity_score: 5.0
complexity_raw: 25.0
staged_implementation: true
orchestration_mode: true
next_action: "design UI for O-Strata (ui-mockup skill) → lock parameter-spec.md v2 → set stage: 1 / phase: discuss → /plugin-discuss O-Strata 1-foundation (second pass)"
next_stage: 1
ready_for_implementation: false
contract_checksums:
  brief: sha256:34b2c5bcaf3d86da18a172bbb73d870c369054717b929e5181f3320c70a897ca
  parameter_spec_draft: sha256:cc3631cba8a098103cc88f2b0bd87cffa6fdf1f93ea8f7ea39b7e55cb1f0cc7d
  architecture: sha256:165ba8e24b43a78e7ea5c5a8e690ca4b056e2f275a73660e5676b5551a2af211
  roadmap: sha256:9d77da57b61937b532148a1df2376e314ed5ce3f373293a6e813211cba343aa9
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

Stage: 0 — **research + plan complete for the live wave-terrain design (2026-09-08)**; the baked-geometry design is superseded (`superseded-baked-v1/README.md`) and its tables ship as an O-Prism factory bank (`plugins/O-Prism/.planning/improvements/geometry-wavetables.md`).
Status: `research/ARCHITECTURE.md` v2 and `ROADMAP.md` v2 written (complexity 5.0, raw 25.0, phased: Stage 1 second pass, Stage 2 × 5, Stage 3 × 3, Stage 4 × 2). **`ready_for_implementation` stays `false` until UI mockup v2 is finalised and `parameter-spec.md` v2 is locked** — the Stage 1 second pass re-parameterises from the locked spec, not from the draft.
Progress: [#####...............] 25% (fork + rename + strip verified; architecture + plan done; mockup, parameters and DSP to do)

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
| research + plan (`/plan`) | ✓ | 2026-09-08 | `research/ARCHITECTURE.md` v2 (Core 1–11, Algorithms, Decisions D1–D12, harness H1–H11), `ROADMAP.md` v2 (complexity 5.0), `stages/0-ideation/CONTEXT.md` |
| mockup v2 | | | `design UI for O-Strata`; lock parameter-spec.md v2 → `ready_for_implementation: true` |

### Stage 1: Foundation — first pass ✓ (baked params, 2026-09-07); **second pass pending** (re-parameterise: 205 params, 46 mod destinations, COMPAT-01 re-verify — ROADMAP "Stage 1: Foundation — second pass")

## Stage 0 decisions (summary — full table in `stages/0-ideation/CONTEXT.md`)

- Terrain formulas use the **πF convention**; defaults c = (0.13, 0.21), aspect 0.7, r = 0.5, F = 1 measured h1 = 0 dB (symmetry gate passes); the gate is not monotone in F, so every factory preset is gated individually.
- Pitch tracking F_eff = F · min(1, C4/f_note)^track (scales down only, block-rate).
- Bandlimited mode: degree-16 triangle, Terrain Freq clamped ≤ 2, Pitch Track inert, truncation is the mip, terrain destinations block-rate (global sources) / bypassed (per-voice); F-lattice is v1.1.
- Hand-rolled per-sample polyphase-IIR decimator (JUCE `FilterDesign` coefficients in `prepare`), not `juce::dsp::Oversampling`; constant +1 sample latency report.
- `WavetableOscillator` / `WavetableData` / mipmaps **deleted in Phase 2.1**; `TerrainOscillator` replaces in place; reaper generalised.
- Quality default 2×. Feedback: two-sample average + leaky integrator, clamped, DC blocker on the output.
- PERF-02 measured as **oscillator delta** via a harness baseline (open question: amend the requirement wording); full-path model ≈ 15 % → ≈ 11.4 % after the two planned Phase 2.3 savings.

## Next Steps

1. (Recommended, 1 hour) Play Aaron Anderson's *Terrain* — the feedback damp law (D1) and the Bandlimited limits (D3) are the decisions a listening result could change before Stage 2.
2. **UI mockup v2:** `design UI for O-Strata` (ui-mockup skill) from BRIEF §UI Concept and ARCHITECTURE "Parameter Mapping" → finalise → `parameter-spec.md` v2 locked; then set `mockup_finalized: true`, `ready_for_implementation: true` here.
3. Stage 1 second pass: set `stage: 1`, `phase: discuss` here and run `/plugin-discuss O-Strata 1-foundation` (ROADMAP "Stage 1: Foundation — second pass").
4. Stage 2: live oscillator DSP, Phases 2.1–2.5 with harness gates H1–H11 (Phase 2.3 is the CPU / aliasing decision point).
5. Answer the four open questions in `stages/0-ideation/CONTEXT.md` (PERF-02 wording, *Terrain* listening, F-lattice v1.0/v1.1, mod-destination host strings).

## Context to Preserve

- Research: `research/wavetable-synthesis-3d-geometry.md` §1, §7.2, §7.3; benchmark `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/`; view prototype `webgl-3d/terrain-proto.html`
- Contracts (v2): `research/ARCHITECTURE.md`, `ROADMAP.md`, `REQUIREMENTS.md`, `BRIEF.md`, `parameter-spec-draft.md` (→ `parameter-spec.md` after mockup v2)
- Superseded design (for the v1.1 Baked source type): `superseded-baked-v1/` (ARCHITECTURE, ROADMAP, spec v1, mockup v1, Stage 0 + Stage 2 CONTEXT) — its Core 5 / Core 8 / State Persistence / 3D view sections are cited by the v2 ARCHITECTURE
- Fork history: `stages/1-foundation/` (CONTEXT D1–D6, RESEARCH, PLAN, SUMMARY, VERIFICATION, smoke harness)
- Symmetry rule and harness gate: REQUIREMENTS DSP-06, ARCHITECTURE "Symmetry rule", harness H2
- Out of scope v1.0: baked sources (v1.1), wavetable mode (never), RGB terrains (v1.1), dual-orbit stereo (v1.x), Bandlimited F-lattice (v1.1)

## Files
- plugins/O-Strata/Source/** (fork, baked parameter set — to be re-parameterised), CMakeLists.txt, tests/**, CHANGELOG.md, .planning/params.tsv (current binary)
- plugins/O-Strata/.planning/{BRIEF.md, REQUIREMENTS.md, parameter-spec-draft.md} (v2, 2026-09-08)
- plugins/O-Strata/.planning/research/ARCHITECTURE.md, ROADMAP.md, stages/0-ideation/CONTEXT.md (v2, 2026-09-08)
- plugins/O-Strata/.planning/superseded-baked-v1/** ; .planning/evidence/*.md (WAVs gitignored)
- PLUGINS.md (row 🚧 Stage 0, 2026-09-08)
