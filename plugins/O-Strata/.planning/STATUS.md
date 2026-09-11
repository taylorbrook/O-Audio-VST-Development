---
plugin: O-Strata
stage: 2
stage_name: dsp
phase: research
status: stage_2_research_complete
round: A
last_updated: 2026-09-11
workflow_mode: manual
complexity_score: 5.0
complexity_raw: 25.0
staged_implementation: true
orchestration_mode: true
next_action: "/plugin-plan O-Strata 2-dsp (Round A PLAN.md — Phases 2.1–2.3; RESEARCH §4 decisions 1–10)"
next_stage: 2
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

Stage: 2 — **DSP (live wave-terrain oscillator) — discuss ✓ (2026-09-10), research ✓ (2026-09-11); Round A = Phases 2.1–2.3, Round B = Phases 2.4–2.5.** Stage 1 second pass complete (discuss ✓ research ✓ plan ✓ execute ✓ verify ✓, 2026-09-10). Stage 0 v2 is complete (ARCHITECTURE / ROADMAP v2, mockup v2 finalised + scaffolded, `parameter-spec.md` v2 locked); the baked-geometry design is superseded (`superseded-baked-v1/README.md`) and its tables ship as an O-Prism factory bank.
Status: `stages/1-foundation/VERIFICATION.md` (second pass) — **✅ VERIFIED**, COMPAT-01 complete; every SUMMARY figure re-measured (param-dump byte-identical, ID diff −2 +34 +20 / 151 identical, pluginval ×2 SUCCESS, auval SUCCEEDED, smoke 62/0, all UI gates at baseline). Harness note: `strata-smoke` must run from the repo root (fixture path is cwd-relative). The binary exposes **205** parameters (params.tsv diff vs O-Prism v1.24.0: −2 `osc?Table`, +34, 20 rows changed in place), 46 mod destinations, `terrainImports` state child, 8 combo relays, factory bank = `Init` only (stale 192-preset bank removed from `~/Library/O-Strata/Presets`); pluginval strictness 10 SUCCESS VST3 + AU, auval SUCCEEDED (COMPAT-01); smoke harness 62/62 (checks [1]–[6], incl. the D3 route check with `Pitch` positive control); UI gates equal the first-pass baseline (page byte-identical). Harness finding: the placeholder's random start phase is seeded from the oscillator address, so sample-identical comparisons need `osc?Phase > 0` (pinned in the harness).
Progress: [############........] 58% (fork + rename + strip verified; architecture + plan done; mockup v2 + spec locked; Stage 1 second pass verified; Stage 2 discuss + research done → plan Round A)

## Why (evidence)

- `evidence/critique-check-2026-09-08.md` — measured on the prototype: centred orbits over symmetric fields play an octave / a twelfth up; mesh sweeps are spectrally static; fBm is the only rich baked source.
- `evidence/replan-proposal-2026-09-08.md` — the sequence below and what carries over.
- Listening (Taylor, 2026-09-08): the baked tables sound nice but do not warrant a new plugin.

## What carries over from the built fork (`Source/`, verified 2026-09-07; re-parameterised 2026-09-10)

Full `Strata` rename, wavetable library/editor removed, `JUCE_WEB_BROWSER`-guarded editor, `juce_cryptography` linked, UI gate fixtures, headless smoke harness (`stages/1-foundation/smoke/`, checks [1]–[6]), CHANGELOG/NOTES/PLUGINS.md plumbing. The 48 baked-geometry parameters were replaced by the 34 live-oscillator parameters in the Stage 1 second pass; `params.tsv` (205 rows) is the current binary **and** the spec v2 target.

## Phase Progress

### Stage 0: Research & Planning (live terrain design)
| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| re-plan brief | ✓ | 2026-09-08 | BRIEF / REQUIREMENTS v2 / parameter-spec-draft v2 |
| research + plan (`/plan`) | ✓ | 2026-09-08 | `research/ARCHITECTURE.md` v2 (Core 1–11, Algorithms, Decisions D1–D12, harness H1–H11), `ROADMAP.md` v2 (complexity 5.0), `stages/0-ideation/CONTEXT.md` |
| mockup v2 | ✓ finalised + scaffolded | 2026-09-10 | `mockups/v2-ui.yaml` + `v2-ui-test.html` on the O-Prism v1.26.0 card shell; brief §UI Concept synced; brief checksum re-anchored (only §UI Concept changed). Scaffolding: `mockups/v2-ui.html`, `v2-PluginEditor-TEMPLATE.h/.cpp`, `v2-CMakeLists-SNIPPET.txt`, `v2-integration-checklist.md`; **`parameter-spec.md` v2 LOCKED** (205 params, 46 mod destinations; 166 slider + 8 comboBox + 30 toggle relays) |

### Stage 1: Foundation — first pass ✓ (baked params, 2026-09-07 — `stages/1-foundation/first-pass-baked/`); **second pass ✓ verified 2026-09-10** (re-parameterise: 205 params, 46 mod destinations, COMPAT-01 re-verify — ROADMAP "Stage 1: Foundation — second pass")
| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| discuss | ✓ | 2026-09-10 | `stages/1-foundation/CONTEXT.md` — D1 relays only, page untouched (UI criteria → Phase 3.1); D2 factory bank = `Init` only, O-Prism presets dropped, stale on-disk bank must be removed (version stays 1.0.0); D3 `StrataVoice` untouched; D4 params.tsv diff = −2 +34 + 20 in-place rows |
| research | ✓ | 2026-09-10 | `stages/1-foundation/RESEARCH.md` — 7 items resolved (§2.1–2.7); corrections: `~/Library/O-Strata/Presets` path + 192-preset stale bank, no migration hook in fork, 3-arg remap lambdas + 7-decimal text, `params.tsv` baseline = `a774d6d4^`; smoke checks [4] D3 route (with positive control) and [5] on-disk `Init` specified |
| plan | ✓ | 2026-09-10 | `stages/1-foundation/PLAN.md` — 14 tasks / 5 waves; Decisions 1–12 close RESEARCH §5 (preset path + `rm` timing, default TerFreq text, 3-arg lambdas, self-describing `Init`, `Pitch` positive control, harness literals asserted against live values, `a774d6d4^` baseline, host names, 0.001 steps, comment-only count refresh, gates regression-only) |
| execute | ✓ | 2026-09-10 | `stages/1-foundation/SUMMARY.md` — Tasks 1–14 of PLAN.md; 205 params / 46 dests / `terrainImports` / 8 combo relays / `Init` bank; pluginval ×2 + auval green; smoke 62/62; UI gates at baseline; commit `feat(O-Strata): Stage 1 second pass …` |
| verify | ✓ | 2026-09-10 | `stages/1-foundation/VERIFICATION.md` — ✅ VERIFIED; COMPAT-01 → complete in REQUIREMENTS.md; FUNC-09 / FUNC-05 / FUNC-10 Stage 1 evidence recorded, formal verification stays Stage 2; issue: smoke harness cwd-sensitive (carry to the Stage 2 render harness) |

### Stage 2: DSP — two rounds (CONTEXT D1); Round A = 2.1 core + 2.2 feedback/tracking + 2.3 oversampling (H6/H7 decision point), Round B = 2.4 Bandlimited + 2.5 PNG path
| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| discuss | ✓ | 2026-09-10 | `stages/2-dsp/CONTEXT.md` — D1 two rounds split at 2.3 (Round A artifacts move to `round-a/` after verify); D2 PERF-02 amended to oscillator delta ≤ 12 % (REQUIREMENTS.md edited); D3 no *Terrain* listening, ARCH Decisions 1 / 3 stand; D4 F-lattice v1.1. Findings: smoke check [4] inverts in 2.1 (fold smoke into `render-harness --smoke`), bench prototype at repo-root `research/…/terrain-bench/`, harness must exclude the editor TU (not O-Bowed's CMake), M4 Max = §7.2 machine. 12 research items |
| research | ✓ | 2026-09-11 | `stages/2-dsp/RESEARCH.md` — 12 items resolved for both rounds; 16 corrections (harness via `ouaricon_add_processor_console`, 2× decimator = 5 sections / latency 1.26, 4× = 1.74 → +1 vs +2 decision, H6 rows A2/A4/A6 exact-cycle, ramps 22 per voice, `getActiveOscFrame` only page caller, HeapBlock gap in H8, blur 113 ms → 3-pass box, Clenshaw 43–91 ns); §4 = 10 plan decisions; §5 = harness skeleton |
| plan (Round A) | ○ | | Phases 2.1–2.3 |
| execute (Round A) | ○ | | |
| verify (Round A) | ○ | | H1–H9; H6 / H7 fallback outcome recorded → input to Round B |
| plan / execute / verify (Round B) | ○ | | Phases 2.4–2.5; H1–H11 `--all` ≤ 3 min |

## Stage 0 decisions (summary — full table in `stages/0-ideation/CONTEXT.md`)

- Terrain formulas use the **πF convention**; defaults c = (0.13, 0.21), aspect 0.7, r = 0.5, F = 1 measured h1 = 0 dB (symmetry gate passes); the gate is not monotone in F, so every factory preset is gated individually.
- Pitch tracking F_eff = F · min(1, C4/f_note)^track (scales down only, block-rate).
- Bandlimited mode: degree-16 triangle, Terrain Freq clamped ≤ 2, Pitch Track inert, truncation is the mip, terrain destinations block-rate (global sources) / bypassed (per-voice); F-lattice is v1.1.
- Hand-rolled per-sample polyphase-IIR decimator (JUCE `FilterDesign` coefficients in `prepare`), not `juce::dsp::Oversampling`; constant +1 sample latency report.
- `WavetableOscillator` / `WavetableData` / mipmaps **deleted in Phase 2.1**; `TerrainOscillator` replaces in place; reaper generalised.
- Quality default 2×. Feedback: two-sample average + leaky integrator, clamped, DC blocker on the output.
- PERF-02 measured as **oscillator delta** via a harness baseline (open question: amend the requirement wording); full-path model ≈ 15 % → ≈ 11.4 % after the two planned Phase 2.3 savings.

## Next Steps

1. ~~Play Aaron Anderson's *Terrain* before Stage 2~~ **skipped by decision (Stage 2 CONTEXT D3)** — the Phase 2.3 WAV grid and the Stage 4 QUAL-04 listening pass are the listening material; the raw-feedback blend stays an internal fallback.
2. ~~**UI mockup v2:** `design UI for O-Strata` (ui-mockup skill) from BRIEF §UI Concept and ARCHITECTURE "Parameter Mapping" → finalise → `parameter-spec.md` v2 locked; then set `mockup_finalized: true`, `ready_for_implementation: true` here.~~ **Done 2026-09-10** — mockup v2 finalised and scaffolded; `parameter-spec.md` v2 locked (sha256 in `contract_checksums`); `ready_for_implementation: true`.
3. ~~Stage 1 second pass: `/plugin-discuss O-Strata 1-foundation`~~ **verified 2026-09-10** (`stages/1-foundation/VERIFICATION.md`).
4. ~~`/plugin-discuss O-Strata 2-dsp`~~ **done 2026-09-10**; ~~`/plugin-research O-Strata 2-dsp`~~ **done 2026-09-11** (`stages/2-dsp/RESEARCH.md`, both rounds). **Next:** `/plugin-plan O-Strata 2-dsp` — Round A PLAN.md (Phases 2.1–2.3), taking RESEARCH §4 decisions 1–10 (latency +1 kept, H6 A-notes at fs = 440·65536/600, pre-filter tap, H8 coverage statement, absolute-index shared ramps, stubs, phase seed, theta_reference.h copied before the deletion).
5. ~~Answer the four open questions in `stages/0-ideation/CONTEXT.md`~~ **all closed 2026-09-10**: PERF-02 → oscillator delta (REQUIREMENTS amended); *Terrain* listening skipped (Decisions 1 / 3 stand); F-lattice v1.1; host strings closed by the mockup v2 lock.

## Context to Preserve

- Research: `research/wavetable-synthesis-3d-geometry.md` §1, §7.2, §7.3; benchmark `research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/`; view prototype `webgl-3d/terrain-proto.html`
- Contracts (v2): `research/ARCHITECTURE.md`, `ROADMAP.md`, `REQUIREMENTS.md`, `BRIEF.md`, **`parameter-spec.md` (locked 2026-09-10)**, `parameter-spec-draft.md` (superseded by the lock); UI: `mockups/v2-ui.yaml`, `v2-ui.html`, `v2-integration-checklist.md`
- Superseded design (for the v1.1 Baked source type): `superseded-baked-v1/` (ARCHITECTURE, ROADMAP, spec v1, mockup v1, Stage 0 + Stage 2 CONTEXT) — its Core 5 / Core 8 / State Persistence / 3D view sections are cited by the v2 ARCHITECTURE
- Fork history: `stages/1-foundation/first-pass-baked/` (CONTEXT D1–D6, RESEARCH, PLAN, SUMMARY, VERIFICATION); `stages/1-foundation/smoke/` (shared harness); second pass: `stages/1-foundation/CONTEXT.md`, `RESEARCH.md`, `PLAN.md`
- Symmetry rule and harness gate: REQUIREMENTS DSP-06, ARCHITECTURE "Symmetry rule", harness H2
- Out of scope v1.0: baked sources (v1.1), wavetable mode (never), RGB terrains (v1.1), dual-orbit stereo (v1.x), Bandlimited F-lattice (v1.1)

## Files
- plugins/O-Strata/Source/** (fork, spec v2 parameter set — 205 params, sine placeholder oscillator), CMakeLists.txt, tests/**, CHANGELOG.md, .planning/params.tsv (205 rows, current binary)
- plugins/O-Strata/.planning/{BRIEF.md, REQUIREMENTS.md, parameter-spec-draft.md} (v2, 2026-09-08); parameter-spec.md (v2 locked, 2026-09-10); mockups/{v2-ui.yaml, v2-ui-test.html, v2-ui.html, v2-PluginEditor-TEMPLATE.h, v2-PluginEditor-TEMPLATE.cpp, v2-CMakeLists-SNIPPET.txt, v2-integration-checklist.md, img/}
- plugins/O-Strata/.planning/research/ARCHITECTURE.md, ROADMAP.md, stages/0-ideation/CONTEXT.md (v2, 2026-09-08); stages/2-dsp/CONTEXT.md (2026-09-10), RESEARCH.md (2026-09-11)
- plugins/O-Strata/.planning/superseded-baked-v1/** ; .planning/evidence/*.md (WAVs gitignored)
- PLUGINS.md (row 🚧 Stage 1, 2026-09-10)
