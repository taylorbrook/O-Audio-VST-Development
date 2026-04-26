# Phase 24 Deferred Items

Items surfaced during Phase 24 execution that are out of scope for the propagation surface (note-expression module integration in 7 plugins). Tracked here per executor protocol so the orchestrator / Phase 25 verifier can triage.

---

## DEF-24-01: O-Lyrica `auval` parameter-meta-flag defect (Plan 24-08 Task 3)

**Surfaced:** 2026-04-26 during plan 24-08 Task 3 (AU verify gate sweep across all 8 plugins).

**Symptom:** `scripts/verify-au-link.sh O-Lyrica` exits 255. Direct `auval -v aumu OLyr OuDv` reproduces:
```
ParameterID=1275870432, Scope=0, Element=0: Saved Value = 0.337891, Current Value 0.000000
ERROR: Parameter values are different since last set - probable cause: a Meta Param Flag is NOT set on a parameter that will change values of other parameters.
Cannot perform Parameter Value check across initialization and reset
* * FAIL
```

**Failure stage:** auval's parameter-state-restore-after-reset test. NOT in NE event processing. NOT a Steinberg link error. The 7 Phase 24 plugins (O-Bells, O-Prism, O-Wind, O-IntonationPad, O-Reed, O-Bowed, O-Formant) all pass `verify-au-link.sh` cleanly — the failure is O-Lyrica-specific and unrelated to note-expression module adoption (which is a transparent passthrough for parameter-state restore).

**Out-of-scope rationale:**
- Phase 24 did NOT modify O-Lyrica sources (last O-Lyrica edits were Phase 23 commits `e695256`, `f667950`, `e89fdc9`, `fee09b6`).
- Phase 23 LYR-03 (`23-04-version-readme-dorico-smoketest-SUMMARY.md`) recorded `auval` PASS — this defect either (a) regressed after Phase 23 close due to a non-Phase-24 change, or (b) is a state-dependent auval check that depends on which parameter happens to be ID 1275870432 in the current build's APVTS hash ordering (auval's "Cold→Warm parameter restore" can surface latent meta-param-flag gaps when re-init paths change values of correlated params).
- Fix requires identifying the offending APVTS parameter (ID 1275870432, current value 0 vs saved 0.337891 — a fractional default suggests a normalized-range param rather than an enum) and adding `juce::AudioParameterFloatAttributes().withMeta()` (JUCE 8 `AudioParameterFloat` constructor flag) or equivalent for parameter dependency declaration. This is a parameter-architecture decision, not a propagation defect → Rule 4 (architectural).
- All 7 Phase 24 propagation targets pass `verify-au-link.sh` — propagation playbook is intact.

**Recommended action:**
- Defer to a separate fix-plan: identify which APVTS parameter has ID 1275870432 (likely a tuning-engine param or an FX-chain enable that drives multiple downstream parameter values during state restore); add the meta-flag to its constructor.
- Phase 25 plan owner should be informed before any release/installer work since `auval` failure may block code-signing audit on macOS.
- Until fixed, O-Lyrica should be flagged in the Phase 24 final SUMMARY's aggregate AU table as `FAIL (pre-existing, parameter meta-flag defect)` with a pointer to this DEF-24-01 entry.

**Build state:** O-Lyrica binary loads correctly via VST3 in DAWs (the failure is auval-specific parameter-restore consistency check, not a runtime load failure). VST3 plugin functions normally in Dorico/Logic/etc. The auval gate is a static QA check, not a runtime gate.

---

## Dorico Batch Validation Result (Plan 24-08 Task 5)

**Resolved:** 2026-04-26 — Phase 24 deferred-batch Dorico human-verify gate.

The Dorico C4 quarter-sharp 3-point smoke gates (D-07) recorded as `DEFERRED` in the per-plugin SUMMARYs for plans 24-02..24-07 (O-Prism, O-Wind, O-IntonationPad, O-Reed, O-Bowed, O-Formant) have been resolved as **PASS** via end-of-phase batch validation by the user on 2026-04-26.

**Per-plugin batch-validation result (3-point gate: ~269.29 Hz quarter-sharp C4 / no attack zipper / polyphonic correlation — only C4 detunes):**

| Plan | Plugin | Initial smoke | Batch validation (2026-04-26) | 3-point result |
|------|--------|---------------|-------------------------------|----------------|
| (Phase 23 23-04) | OLyrica | PASS (LYR-03 5-test) | PASS | 3/3 |
| 24-01 | O-Bells | PASS (canary, inline) | PASS | 3/3 |
| 24-02 | O-Prism | DEFERRED | PASS | 3/3 |
| 24-03 | O-Wind | DEFERRED | PASS | 3/3 |
| 24-04 | O-IntonationPad | DEFERRED | PASS | 3/3 |
| 24-05 | O-Reed | DEFERRED | PASS | 3/3 |
| 24-06 | O-Bowed | DEFERRED | PASS | 3/3 |
| 24-07 | O-Formant | DEFERRED | PASS | 3/3 |

**Aggregate:** 8/8 plugins PASS the 3-point Dorico gate. All three Phase 23 spike landmines (Pattern 1: noteId correlation; Pattern 2: apply-before-trigger; Pattern 3: 240-semitone full-scale conversion) defended on every Phase 24 propagation target.

**Canonical aggregate record:** `.planning/phases/24-propagate/24-08-final-sweep-SUMMARY.md` (this plan's SUMMARY) — the per-plugin SUMMARYs (24-02..24-07) intentionally retain their original `DEFERRED` status as historical record of the deferred-batch flow; they are NOT retroactively rewritten.

**Status:** Phase 24 Dorico-gate scope CLOSED. Only DEF-24-01 (O-Lyrica APVTS Meta-Flag, above) carries forward — that defect is unrelated to note-expression module adoption and is tracked in STATE.md pending-todos #2.

---
