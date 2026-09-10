---
plugin: O-Strata
stage: 1
stage_name: foundation
phase: research
status: stage_1_second_pass_research_complete
last_updated: 2026-09-10
workflow_mode: manual
complexity_score: 5.0
complexity_raw: 25.0
staged_implementation: true
orchestration_mode: true
next_action: "/plugin-plan O-Strata 1-foundation (second pass — RESEARCH.md §2.1–2.7, four CONTEXT corrections in §1)"
next_stage: 1
ready_for_implementation: true
contract_checksums:
  brief: sha256:955f20b31a5f99a45b97a989e5c7d1333b8d23b359dd1f8b66ec90af189c4671
  parameter_spec_draft: sha256:cc3631cba8a098103cc88f2b0bd87cffa6fdf1f93ea8f7ea39b7e55cb1f0cc7d
  parameter_spec: sha256:238863d7dcd9f59045e9532e79afbbe9e8b531c7aaab29fc27317c9efb702f61
  architecture: sha256:165ba8e24b43a78e7ea5c5a8e690ca4b056e2f275a73660e5676b5551a2af211
  roadmap: sha256:9d77da57b61937b532148a1df2376e314ed5ce3f373293a6e813211cba343aa9
mockup_latest_version: 2
ui_design_phase_complete: true
mockup_finalized: true
finalized_version: 2
brief_updated_from_mockup: true
mockup_version_synced: 2
brief_update_timestamp: "2026-09-10T15:15:37Z"
ui_scaffolding_phase_complete: true
fork_exists: true
fork_verified: "stages/1-foundation/first-pass-baked/VERIFICATION.md (baked parameter set, 2026-09-07)"
superseded_design: superseded-baked-v1/
stage_0_status: ui_design_complete
---

# O-Strata Status

## Current Position

Stage: 1 — **Foundation, second pass (re-parameterise the verified fork) — discuss ✓ research ✓ (2026-09-10)**. Stage 0 v2 is complete (ARCHITECTURE / ROADMAP v2, mockup v2 finalised + scaffolded, `parameter-spec.md` v2 locked); the baked-geometry design is superseded (`superseded-baked-v1/README.md`) and its tables ship as an O-Prism factory bank.
Status: `stages/1-foundation/RESEARCH.md` (second pass) written — seven CONTEXT items resolved; four corrections carried to the plan: preset folder is `~/Library/O-Strata/Presets/` (initializer never deletes; 192 stale JSON on disk, fork bank is 192 not ≈ 20), the fork's preset manager has NO v1.0.6 migration hook, `ValueRemapFunction` is `(start, end, v)` and continuous-range text prints 7 decimals, O-Prism `params.tsv` is the v1.25.0 registry (diff against `a774d6d4^`). CONTEXT: D1 relays only / page untouched, D2 `Init` only, D3 voice untouched, D4 −2 +34 + 20 in-place.
Progress: [########............] 40% (fork + rename + strip verified; architecture + plan done; mockup v2 + spec locked; Stage 1 second pass: discuss ✓ research ✓ → plan)

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
| mockup v2 | ✓ finalised + scaffolded | 2026-09-10 | `mockups/v2-ui.yaml` + `v2-ui-test.html` on the O-Prism v1.26.0 card shell; brief §UI Concept synced; brief checksum re-anchored (only §UI Concept changed). Scaffolding: `mockups/v2-ui.html`, `v2-PluginEditor-TEMPLATE.h/.cpp`, `v2-CMakeLists-SNIPPET.txt`, `v2-integration-checklist.md`; **`parameter-spec.md` v2 LOCKED** (205 params, 46 mod destinations; 166 slider + 8 comboBox + 30 toggle relays) |

### Stage 1: Foundation — first pass ✓ (baked params, 2026-09-07 — `stages/1-foundation/first-pass-baked/`); **second pass in progress** (re-parameterise: 205 params, 46 mod destinations, COMPAT-01 re-verify — ROADMAP "Stage 1: Foundation — second pass")
| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| discuss | ✓ | 2026-09-10 | `stages/1-foundation/CONTEXT.md` — D1 relays only, page untouched (UI criteria → Phase 3.1); D2 factory bank = `Init` only, O-Prism presets dropped, stale on-disk bank must be removed (version stays 1.0.0); D3 `StrataVoice` untouched; D4 params.tsv diff = −2 +34 + 20 in-place rows |
| research | ✓ | 2026-09-10 | `stages/1-foundation/RESEARCH.md` — 7 items resolved (§2.1–2.7); corrections: `~/Library/O-Strata/Presets` path + 192-preset stale bank, no migration hook in fork, 3-arg remap lambdas + 7-decimal text, `params.tsv` baseline = `a774d6d4^`; smoke checks [4] D3 route (with positive control) and [5] on-disk `Init` specified |
| plan | ○ | | |
| execute | ○ | | |
| verify | ○ | | COMPAT-01 |

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
2. ~~**UI mockup v2:** `design UI for O-Strata` (ui-mockup skill) from BRIEF §UI Concept and ARCHITECTURE "Parameter Mapping" → finalise → `parameter-spec.md` v2 locked; then set `mockup_finalized: true`, `ready_for_implementation: true` here.~~ **Done 2026-09-10** — mockup v2 finalised and scaffolded; `parameter-spec.md` v2 locked (sha256 in `contract_checksums`); `ready_for_implementation: true`.
3. ~~Stage 1 second pass: `/plugin-discuss O-Strata 1-foundation`~~ **discuss + research done 2026-09-10** → `/plugin-plan O-Strata 1-foundation`, then execute → verify (ROADMAP "Stage 1: Foundation — second pass").
4. Stage 2: live oscillator DSP, Phases 2.1–2.5 with harness gates H1–H11 (Phase 2.3 is the CPU / aliasing decision point).
5. Answer the four open questions in `stages/0-ideation/CONTEXT.md` (PERF-02 wording, *Terrain* listening, F-lattice v1.0/v1.1, mod-destination host strings).

## Context to Preserve

- Research: `research/wavetable-synthesis-3d-geometry.md` §1, §7.2, §7.3; benchmark `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/`; view prototype `webgl-3d/terrain-proto.html`
- Contracts (v2): `research/ARCHITECTURE.md`, `ROADMAP.md`, `REQUIREMENTS.md`, `BRIEF.md`, **`parameter-spec.md` (locked 2026-09-10)**, `parameter-spec-draft.md` (superseded by the lock); UI: `mockups/v2-ui.yaml`, `v2-ui.html`, `v2-integration-checklist.md`
- Superseded design (for the v1.1 Baked source type): `superseded-baked-v1/` (ARCHITECTURE, ROADMAP, spec v1, mockup v1, Stage 0 + Stage 2 CONTEXT) — its Core 5 / Core 8 / State Persistence / 3D view sections are cited by the v2 ARCHITECTURE
- Fork history: `stages/1-foundation/first-pass-baked/` (CONTEXT D1–D6, RESEARCH, PLAN, SUMMARY, VERIFICATION); `stages/1-foundation/smoke/` (shared harness); second pass: `stages/1-foundation/CONTEXT.md`, `RESEARCH.md`
- Symmetry rule and harness gate: REQUIREMENTS DSP-06, ARCHITECTURE "Symmetry rule", harness H2
- Out of scope v1.0: baked sources (v1.1), wavetable mode (never), RGB terrains (v1.1), dual-orbit stereo (v1.x), Bandlimited F-lattice (v1.1)

## Files
- plugins/O-Strata/Source/** (fork, baked parameter set — to be re-parameterised), CMakeLists.txt, tests/**, CHANGELOG.md, .planning/params.tsv (current binary)
- plugins/O-Strata/.planning/{BRIEF.md, REQUIREMENTS.md, parameter-spec-draft.md} (v2, 2026-09-08); parameter-spec.md (v2 locked, 2026-09-10); mockups/{v2-ui.yaml, v2-ui-test.html, v2-ui.html, v2-PluginEditor-TEMPLATE.h, v2-PluginEditor-TEMPLATE.cpp, v2-CMakeLists-SNIPPET.txt, v2-integration-checklist.md, img/}
- plugins/O-Strata/.planning/research/ARCHITECTURE.md, ROADMAP.md, stages/0-ideation/CONTEXT.md (v2, 2026-09-08)
- plugins/O-Strata/.planning/superseded-baked-v1/** ; .planning/evidence/*.md (WAVs gitignored)
- PLUGINS.md (row 🚧 Stage 0, 2026-09-08)
