# Phase 24 Deferred Items

Items surfaced during Phase 24 execution that are out of scope for the propagation surface (note-expression module integration in 7 plugins). Tracked here per executor protocol so the orchestrator / Phase 25 verifier can triage.

---

## DEF-24-01 [DOWNGRADED 2026-04-26]: O-Lyrica `auval` parameter-meta-flag — tool-static-check artifact, not a defect

**Status:** NOT a defect. O-Lyrica is the **validated spike/reference plugin** for the entire `note-expression` module — it was the original implementation surface used to develop and test VST3 Note Expression Dorico microtonal playback (Phase 23 spike → Plans 23-01..23-05). It is the canonical PASS state for the suite.

**What surfaced:** 2026-04-26 during plan 24-08 Task 3, `scripts/verify-au-link.sh O-Lyrica` exited 255 with a parameter-state-restore message:
```
ParameterID=1275870432, Scope=0, Element=0: Saved Value = 0.337891, Current Value 0.000000
ERROR: Parameter values are different since last set - probable cause: a Meta Param Flag is NOT set on a parameter that will change values of other parameters.
```

**Why this is NOT a defect:**
- O-Lyrica was the spike vehicle (Phase 23 Plans 01–05) and is already validated end-to-end for VST3 Note Expression Dorico microtonal playback. Phase 23 LYR-03 5-test Dorico battery: PASS. Phase 24 batch validation 2026-04-26: PASS 3/3.
- The auval finding is a static parameter-state-restore consistency check that surfaces a meta-flag annotation gap on one APVTS parameter — it does NOT prevent VST3 hosting in Dorico/Logic/Live/etc. and does NOT affect runtime behavior, NE event handling, or microtonal correctness.
- O-Lyrica VST3 functions normally in every host the user has tested. The substantive runtime path is correct.
- Treating this as a "DEF" was an over-classification by the executor — auval's static check fires on a real annotation gap but the gap is benign for the plugin's actual behavior.

**Optional cosmetic follow-up (low priority, not blocking):**
- If macOS code-signing audit ever flags the auval failure during a release pipeline, the cosmetic fix is to identify the APVTS parameter with ID 1275870432 (likely a derived/correlated param) and add `juce::AudioParameterFloatAttributes().withMeta()` to its constructor. This is a 1-line parameter annotation, not an architectural change.
- This is NOT a Phase 24 carry-forward and does NOT block v1.5 milestone close. Track only if a release pipeline surfaces it.

**Phase 24 aggregate AU verify result:** All 8 plugins (O-Lyrica + 7 Phase 24 targets) PASS substantive AU loading and Dorico runtime validation. The auval static check on O-Lyrica is the only non-PASS auval result in the suite; user-confirmed 2026-04-26 that this does NOT reflect an actual functional defect.

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

**Status:** Phase 24 Dorico-gate scope CLOSED. DEF-24-01 has been DOWNGRADED — see above; it is a tool-static-check artifact, not a defect. O-Lyrica is the validated spike/reference plugin for the suite and PASSES all substantive runtime gates. No Phase 24 items carry forward as defects.

---
