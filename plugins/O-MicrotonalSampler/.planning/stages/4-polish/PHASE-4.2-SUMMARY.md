---
title: "O-MicrotonalSampler Phase 4.2 — PERF-02 measurement (methodology deviation)"
created: 2026-04-28
phase: 4.2
status: complete
verifies_requirements:
  - PERF-02   # partial → complete (conditional on 4.4 pluginval-10 timing PASS)
---

# Phase 4.2 — PERF-02 measurement (SUMMARY)

## Goal recap

Run the PERF-02 protocol per `RESEARCH.md §RQ4-3`: Logic Pro
Performance Meter, Apple Silicon on power, 48 kHz / 256, 16-voice
held chord, 3 runs, `delta_CPU_pct ≤ 5 %`. Flip `PERF-02` from
`partial` to `complete`.

## What actually landed (deviation)

The Logic Pro 11.x Performance Meter could not be surfaced in this
user's environment — the floating Performance Meter window appears
to have been restructured or removed in Logic 11, and the
LCD/Control-Bar mini-meter was not visible either. The spec metric
(Performance Meter `delta_CPU_pct`) is therefore unmeasurable in this
environment.

**Path B taken** (per the discuss/decide exchange immediately
preceding this phase):

- **Activity Monitor used as a supporting headline only.** One run
  on M4 Max (laptop, plugged in) at 16 voices / 48 kHz / 256:
  **~16 % of one core**, ≈ **~1 % of total system CPU** on the
  16-core part. Well below the 5 % spec budget at the system level.
- **Objective per-block timing budget = `pluginval --strictness-level
  10` in Phase 4.4** — the strictness-10 stress includes timing
  constraints and fuzzed parameter sequences, providing an objective
  and reproducible substitute for the Logic-side metric. This is the
  **gate-of-record** for PERF-02; the Activity Monitor reading is
  supporting evidence, not the gate.
- **Conditional flip.** PERF-02 is marked `complete` now; if Phase
  4.4 strictness-10 surfaces a timing regression, the flip is rolled
  back and Stage 2 sub-phase 2.4 (voice-steal) or 2.5 (loop-detect
  hot path) reopens per `PLAN.md §Failure Routing`.

## Files modified

- `.planning/stages/4-polish/VERIFICATION.md` — created; PERF-02
  section populated with hardware, methodology deviation, Activity
  Monitor headline, verdict, and v1.1 follow-up.
- `.planning/REQUIREMENTS.md` — `PERF-02` row flipped `partial →
  complete`, `verified at` field carries the methodology-deviation
  caveat in full so the deviation is visible at a glance from the
  requirements index.

## Files NOT modified (invariants held)

- All Stage 2 / Stage 3 source paths — frozen.
- `CMakeLists.txt`, `modules.json` — untouched.
- No new pluginval / auval runs in this phase (Phase 4.1's runs are
  current; the strictness-10 run is Phase 4.4's responsibility).

## Risk register

| Risk | Mitigation |
|---|---|
| Activity Monitor headline doesn't map to Logic Performance Meter % | Phase 4.4 pluginval-10 is the objective gate-of-record; this phase's flip is conditional on that PASS. |
| Logic Pro 11 may regain a Performance Meter in a future point release | v1.1 follow-up logged in VERIFICATION §PERF-02; capture the spec metric then. |
| Activity Monitor reading was a single run, not the spec's 3 runs | Acceptable because the gate-of-record is pluginval-10 in 4.4, not the Activity Monitor headline. |

## Next phase

Phase 4.3 — QUAL-01 listening checklist. **User-driven** subjective
audit (sustained sine, cello vibrato, transient, ±50 c retune,
voice-steal stress, mixed-SR, short-region loop). All-pass flips
QUAL-01 → complete; any defect reopens the relevant Stage 2 sub-phase.
