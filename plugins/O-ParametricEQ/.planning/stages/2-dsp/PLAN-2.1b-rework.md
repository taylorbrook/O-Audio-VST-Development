---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.1b Rework Plan (polish continuity)"
created: 2026-11-13
juce_version: "8.0.15"
phase: "2.1b"
supersedes: "PLAN-2.1b.md T7 (blocked at C1) and T8; everything else in PLAN-2.1b.md stands unless amended here"
summary: "Six waves, serial: (W0) pre-flight on the known uncommitted 2.1b tree, baseline 47 / 1 / 11 reproduced; (W1) shelf re-centre in the Python model, designs golden regenerated and audited against the verified one; (W2) the same rule in Prototype.h, step limit and rate gate in the polish; (W3) polished model and golden; (W4) harness rows (C1 widened, never-worse off the grid, runaway, rate gate, re-centre boundaries, first-build negative control), build, gates; (W5) summaries, STATUS, one path-scoped commit. No processor code, no plugin binary change, no install, no tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 2 / Phase 2.1b Rework Plan (polish continuity)

## Goal

The polished designer is continuous everywhere it runs and is never far worse than its seed anywhere in the parameter range, with every accuracy row of `PLAN-2.1b.md` still inside its tolerance. Row C1 passes on a wider line set, and the first Phase 2.1b build fails the new rows.

Three changes, all decided (STATUS step 13, `RESEARCH-2.1b-continuity.md`):

1. **Shelf re-centre on the seed side** (a Phase 2.1 seed change).
2. **Step limit 0.35** on every polish step.
3. **No polish below 44.1 kHz**, decided once in `Designer::prepare`.

Out of scope: processor code, the correction hold, polishing the first-order shelf section, any constant not named here.

**Starting state:** the blocked 2.1b execute is in the working tree, uncommitted (`SUMMARY-2.1b.md` "Uncommitted files"). This plan builds on it and commits it together with the rework.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/RESEARCH-2.1b-continuity.md` | Q4 (the two rules, exactly), Q5 / Q6 (expected figures), Q7, Pitfalls, "Consequences for the plan". |
| `stages/2-dsp/research-2.1b/cont_bench.cpp` | Working code of both rules: `recentrePair` and the `trust` block. Port from here. |
| `stages/2-dsp/SUMMARY-2.1b.md` | The baseline tally and the two C1 lines that failed. |
| `stages/2-dsp/PLAN-2.1b.md` | P1–P12 and the row table. Still binding where not amended below. |
| `research/ARCHITECTURE.md` | "Frequency policy near and above Nyquist" (Shelves, Tilt row), "Accuracy polish" (limit, rate, Continuity), gate G1. Amended by this plan phase. |
| `Source/dsp/Prototype.h`, `Polish.h`, `PolishConstants.h`, `Designer.h`; `tests/model/*.py`; `tests/designer-harness/main.cpp` | The code this phase changes. |

---

## Plan-phase decisions

- **RP1 The re-centre rule, verbatim from RESEARCH Q4 (a).** For each pair `(w0, Q)` (zeros, then poles, independently) of a `gain2` section built by `detail::shelfSections` (Low Shelf, High Shelf, Tilt):
  ```
  wL = shelfRecentre · π                                   shelfRecentre = 0.95
  if not (w0 > wL): unchanged
  β  = sstep ((ln Q − ln Qlo) / (ln Qhi − ln Qlo))         Qlo = 1.5, Qhi = 4
  if not (β > 0): unchanged
  wT = exp (ln w0 + β (ln wL − ln w0))
  x  = wT / w0
  Qe = 1 / sqrt ((1 − x²)² + x² / Q²), clamped to [recentreQMin, recentreQMax]
  designSide pair = (wT, Qe)
  ```
  `trueSide` is never changed (the polish target and `BandDesign::trueProto` stay the true section). The `gain1` section and bells are not touched. The two early returns are what keeps every design outside the region bit-identical to Phase 2.1; they are value-continuous (`Qe = Q` at `w0 = wL`, the move is 0 at `β = 0`).
- **RP2 Constants.** `shelfRecentre`, `shelfRecentreQLo`, `shelfRecentreQHi` live in `Prototype.h` beside `bellRecentre` (the seed's constants). `polish::stepLimit = 0.35` and `polish::minSampleRateHz = 44100.0` live in `PolishConstants.h`. PLAN-2.1b P10 holds for all five: a change is an escalation, not a tuning step.
- **RP3 Step limit, RESEARCH Q4 (b).** In `polishStep`, after the back-substitution and before the update: `δ ← δ · R / sqrt (R² + |δ|²)`, Euclidean norm over the six free parameters. No branch in the shipped path.
- **RP4 Rate gate.** `Designer::prepare` sets one flag: `sampleRateHz >= polish::minSampleRateHz`. With the flag off, `design` and `designWith` return exactly what `designSeed` returns, weights 0. The model's `design_band_polished` does the same.
- **RP5 C1 limits.** Polished neighbour-step change ≤ **4x** the seed's at both step counts; polished 32 000-step figure ≤ 0.5x its 8000-step figure. The second is the test that detects a jump.
- **RP6 C1 line set.** Block A is the existing 341 lines, same generator seed and order, so the two lines that failed stay in the row and are printed by name. Block B is appended from a second generator seed (`0xC1C1C1C2`): shelves at 18, 24, 72, 96 dB/oct (slope indices 2, 3, 6, 7) and Tilt at 48 dB/oct (index 5), 60 lines per family and rate; plus 20 more per rate for each of the four original families. 760 lines, 1101 in the row. **No reseeding to make a row pass.**
- **RP7 C1-hi is new evidence.** 216 lines (9 families × 88.2 / 96 / 192 kHz × 8), same limits, binding. Research ran these rates at step limit 0.5 only (0 of 1080).
- **RP8 Never-worse off the grid (NW2) has a ceiling of +1.0 dB.** Research measured +0.67 dB worst over 2.8 million sections at 44.1 kHz. This number is the plan's call; ARCHITECTURE G1 carries it.
- **RP9 The negative control is the first build, rebuilt inside the harness.** Seed `Designer::designSeedSection (trueProto)` (no re-centre), polished by `detail::polishSectionWith` with a harness-local policy whose step limit is off. No runtime switch and no hook in `Prototype.h`. The polish's existing compile-time `Extension` parameter becomes a policy that also carries the step limit.
- **RP10 C1n scores only lines the shipped design passes** (SUMMARY-2.1b caveat). It fires when the binary gate fails at least one of those.
- **RP11 I-weak stays INFO, with WARN ceilings** at 44.1 kHz, 10–20 k: 1.0 / 1.5 / 1.0 dB at ±6 / ±12 / ±30 (bench: at most 0.75 / 1.03 / 0.45). The harness grid differs from the bench's and that difference is unresolved; the binding protection in this region is NW2, RA1 and C1.
- **RP12 Phase 2.1 artefacts are redone, and the change is audited.** `designs.tsv` is regenerated; every changed line must be explained by the rule (T3). The verified Phase 2.1 result stands for everything the audit shows unchanged.
- **RP13 One commit.** The blocked execute's files and the rework go in together. A second 30 MB `designs.tsv` blob enters history (most lines are identical, so it packs as a delta).

---

## Tasks

Roles as in `PLAN-2.1.md`: **[agent]** = dsp-agent writes files (no Bash); **[run]** = the execute session. **BG** = `run_in_background`, log to `$S/<name>.log`; `$S` = the session scratchpad. Paths are relative to `plugins/O-ParametricEQ/` unless they start with `build/`. `$H` = the designer harness binary (PLAN-2.1 T8.4).

### Wave 0 — pre-flight

**T1. Pre-flight and baseline** [run]
- Done when (any mismatch → STOP):
  - `shasum -a 256` of `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` equal STATUS `contract_checksums`.
  - `git branch --show-current` = `main`. `git status --short plugins/O-ParametricEQ` is exactly: modified `Source/dsp/Designer.h`, `Source/dsp/EqTypes.h`, `tests/designer-harness/main.cpp`, `.planning/stages/1-foundation/gate-report.json`; untracked `Source/dsp/Polish.h`, `Source/dsp/PolishConstants.h`, `tests/model/polish_model.py`, `tests/model/dump_polish.py`, `tests/golden/designs-polished.tsv`, `.planning/stages/2-dsp/SUMMARY-2.1b.md`. Save the full `git status --short` to `$S/git-status-T1.txt`.
  - (BG) build both harnesses; `$H` → **47 PASS, 1 FAIL, 11 INFO** with C1 the only FAIL (the blocked state, reproduced). Render harness `36 PASS, 0 FAIL`; `check_spec.py` → `OK:`.
  - Reference copies in `$S`: from the repo root, `git show HEAD:plugins/O-ParametricEQ/tests/golden/designs.tsv > $S/designs-old.tsv` and `shasum` it against the working file (equal); `cp` the uncommitted `tests/golden/designs-polished.tsv` to `$S/designs-polished-first.tsv`; `cp $S/designer.log $S/designer-first.log`.

### Wave 1 — the seed change in the model

**T2. `tests/model/eqmodel.py`: shelf re-centre** [agent]
- `SHELF_RECENTRE`, `SHELF_RECENTRE_Q_LO`, `SHELF_RECENTRE_Q_HI` beside `BELL_RECENTRE`; `recentre_pair (w0, Q)` exactly as RP1; `_shelf_sections` designs `mlr` from the re-centred pairs and keeps the true tuple as `true` (the section dict already has a `design` slot, as the bell uses).
- The header comment's D10 line names the shelf rule.
- Depends on: T1.

**T3. Regenerate `designs.tsv` and audit the change** [run]
- `python3 plugins/O-ParametricEQ/tests/model/dump_designs.py > plugins/O-ParametricEQ/tests/golden/designs.tsv`.
- Audit script in `$S`, importing `tests/model/eqmodel.py`, comparing against `$S/designs-old.tsv` line by line:
  - same line count, same key columns on every line;
  - every changed line is `lowShelf` / `highShelf` / `tilt`, class `gain2`, and its true section has a pair with `w0 > 0.95 π` and `Q > 1.5`;
  - every `gain2` shelf / tilt line with such a pair is changed; print both counts;
  - 0 changed lines for bell, lowCut, highCut, notch, bandPass and for `gain1`;
  - changed-line count > 0 (vacuity).
- Model self-check as PLAN-2.1 T3 (RESEARCH-2.1 Q1 eleven rows within 0.01 dB; Q2 cells within 0.02 dB). A Q1 row that now moves must be a row the audit attributes to the rule: record old → new; anything else → STOP.
- Done when: audit clean; no `\r`, `nan`, `inf`; a second run is byte-identical.
- Depends on: T2.

### Wave 2 — C++

**T4. `Source/dsp/Prototype.h`: shelf re-centre** [agent]
- The three constants (RP2), `detail::recentrePair` (RP1), applied in `shelfSections` to the zero pair and the pole pair of each `gain2` section; `addSection (…, designSide, trueSide, …)` with the two now different.
- Expression order as in the model (`exp (ln w0 + β (ln wL − ln w0))`), so D1 holds at 1e-9.
- One `smoothStep` definition serves `Prototype.h` and `Polish.h` (move it; do not copy it).
- The file's D10 comment and `AnalogBand`'s field comments say that a shelf's design side can differ.
- Depends on: T3 (ordering only).

**T5. `Source/dsp/PolishConstants.h`, `Polish.h`, `Designer.h`: step limit and rate gate** [agent]
- `polish::stepLimit`, `polish::minSampleRateHz` (RP2), with the comment that they came from `RESEARCH-2.1b-continuity.md`.
- `polishStep` applies RP3. The limit comes from the policy type: `detail::SmoothExtension` (rename to a policy name if clearer, harness updated to match) carries `polish::stepLimit`; a policy with the limit off compiles the scaling out. The shipped `polishSection` has no runtime switch.
- `Designer::prepare` sets the flag (RP4); `designWith` skips the polish loop when it is off. `designSeed` unchanged.
- Header comments of `Polish.h` and `Designer.h` updated to say all three things.
- Depends on: T4.

### Wave 3 — the polished model and golden

**T6. `tests/model/polish_model.py`** [agent]
- `STEP_LIMIT`, `MIN_FS` mirroring the header; `_step` applies the limit; `design_band_polished` returns the seed with weight 0 below `MIN_FS`. The seed it polishes is now the re-centred one (it calls `eqmodel`); the target stays the true section.
- Depends on: T5 (ordering only).

**T7. Regenerate `designs-polished.tsv`** [run]
- `python3 …/dump_polish.py > plugins/O-ParametricEQ/tests/golden/designs-polished.tsv`.
- Self-check as PLAN-2.1b T3: bells at 44.1 kHz 0.043 / 0.081 / 0.193 within 0.01 dB (bells are untouched by the re-centre; the step limit may move them slightly: research Q6 says "the same"); 12 dB/oct shelves Q ≤ 1.41, 10–20 k within 0.02 dB of 0.043 / 0.120 / 0.184.
- Compare with `$S/designs-polished-first.tsv`: print the count of changed lines per type (information for SUMMARY; bells are expected to change little or not at all).
- Done when: both self-checks pass; no `\r`, `nan`, `inf`; a second run is byte-identical.
- Depends on: T6.

### Wave 4 — harness, build, gates

**T8. `tests/designer-harness/main.cpp`** [agent]
- Changed rows:

  | Row | Change |
  |---|---|
  | C1 | Limits per RP5. Lines per RP6 (block A unchanged, block B appended). The two lines that failed in the first build (`random shelf 48 dB/oct #27` at 48 kHz, `#2` at 44.1 kHz) are printed with their figures whatever the result. Also printed: the count of lines above 2x of the seed and the worst ratio (research expects about 1 in 1000, at most 2.4x). |
  | C1n | RP10. |
  | I-weak | RP11: ceilings printed, WARN line above them. Adds the row "12 dB/oct, Q 2–40, fc 20–30 k" (bench 0.58 / 0.22 / 0.38). The "stubborn cell" line now reads against the bench's 0.45 dB. |
  | I-new | The 22.05 / 32 kHz bell line is replaced by PR1's statement (no polish there). |
  | P1 | Unchanged rows stay; add P1s below. |

- New rows:

  | Row | Check | Threshold |
  |---|---|---|
  | C1-hi | RP7 | as C1 |
  | NW2 | 250 000 seeded random bands per rate at 44.1 and 48 kHz: Bell, Low Shelf, High Shelf, Tilt, every slope, fc 10 Hz – 30 kHz log, gain 0 – 60 dB, Q 0.025 – 40 log. Per `gain2` section, against its own true prototype on 400 log points over the match band plus the evaluation grid's tied points: polished max error − seed max error | ≤ +1.0 dB on every section (RP8); at least 1 000 000 sections per rate or the row is void. Printed: worst excess, count above +0.01 dB, split "Q ≥ 4 and upper pair above 0.9 Nyquist" / rest. Same at 96 kHz as INFO |
  | RA1 | On NW2's draw: sections with weight > 0 whose `g` or `k` is more than 10x (either way) from the seed's | 0 |
  | PR1 | 10 000 mixed settings at 22.05 and at 32 kHz: `design` vs `designSeed` | `memcmp` equal, every weight 0. Vacuity: the same settings at 44.1 kHz give at least 1000 bands with a weight > 0 |
  | P1s | Shelf re-centre, on `designSeed` and `buildPrototype`: (i) a pair crossing `0.95 π (1 ± 1e-9)`, High Shelf pole pair and Low Shelf zero pair, 12 dB/oct, Q 4 and 40; (ii) section Q crossing `1.5 (1 ± 1e-9)` and `4 (1 ± 1e-9)` with the pair above `0.95 π`; (iii) 10 000 random shelf / tilt settings: a pair with `w0 ≤ 0.95 π` or `Q ≤ 1.5` has `designSide == trueSide` bit for bit, and `trueSide` equals the Phase 2.1 formula for every pair; (iv) a moved pair has `0.95 π ≤ wT ≤ w0`, `Qe` inside the clamp, and its magnitude at `wT` equals the true pair's within 1e-12 relative where the clamp is not active | (i), (ii) ≤ 1e-6 relative parameter change; (iii), (iv) exact; at least 1000 moved pairs or the row is void. The boundary values are harness literals, not the header's constants |

- `--neg first-build` (RP9): NW2, RA1 and C1 block A computed on the first-build variant; exit 0 only if **all three** fail. Expected: NW2 worst about +54 dB at 44.1 kHz, RA1 about 1000, C1 the 637x line with a non-shrinking jump.
- `--neg binary-gate` follows RP10. The other controls are unchanged.
- The constants block printed at the top gains the five new constants.
- Depends on: T5, T7.

**T9. Build, run, negative controls** [run]
- Done when, in order:
  1. (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness > $S/build.log 2>&1` → rc 0, no warnings in this plugin's files. No CMake change is expected.
  2. (BG) `$H > $S/designer.log 2>&1` → **0 FAIL**; every row ID of PLAN-2.1, PLAN-2.1b and this plan appears. C1, C1-hi and NW2 dominate the run time.
  3. D1 green against the regenerated `designs.tsv`; D2 green against the regenerated `designs-polished.tsv`.
  4. Phase 2.1 rows against `$S/designer-first.log`: G1-bell, G1-sh12, G1-sh6, T1, R1, N1, G2s, CS1, CS2, NB1–NB4 print the same figures (the rule does not reach them). INFO rows for resonant and steep shelves may move (research: up to 0.4 dB at ±30 dB); copy old → new into the summary.
  5. `$H --neg first-build`, `--neg binary-gate`, `--neg seed`, `--neg rbj-b`, `--neg rbj`, `--neg f1`, `--neg alloc` → rc 0 each.
  6. Golden negative controls on copies in `$S`: one cell of `designs.tsv` changed in the 6th significant figure → D1 FAIL only; one cell of `designs-polished.tsv` in the 4th → D2 FAIL only.
  7. Constants: `grep -rnE "0\.35\b|44100" plugins/O-ParametricEQ/Source/dsp --include=*.h` hits `PolishConstants.h` only; `grep -rnE "\b1\.5\b|\b4\.0\b" plugins/O-ParametricEQ/Source/dsp/Prototype.h` hits the constant definitions only (or explain each other hit). PLAN-2.1b T7.5 re-run.
  8. Static gates of PLAN-2.1 T8.8; render harness `36 PASS, 0 FAIL`; `check_spec.py` → `OK:`.
- **On a binding failure:**
  - D1, D2 or the T3 audit red → a port bug. Fix it.
  - D1 and D2 green and any of C1 (shrink test), C1-hi, NW2, RA1, a G1b cell, NW1 red → research and the harness disagree. **STOP and escalate with the cells.** No constant, threshold, ceiling, line seed or line count is changed in this session (RP2, RP6).
  - C1's 4x test red with the shrink test green → report the line and its ratio; this is the case research flagged (2.4x measured). STOP; the limit is the user's call.
  - `--neg first-build` does not fire on one of its three rows → that row is vacuous. Fix the row, not the control.
- Depends on: T8.

### Wave 5 — bookkeeping + commit

**T10. Summaries, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/2-dsp/SUMMARY-2.1b-rework.md`: tally; the T3 audit counts; C1 / C1-hi worst ratios, the count above 2x, the two former failing lines; NW2 worst excess per rate and region; RA1; PR1; P1s; every G1b cell at five rates beside the first build's (the 72 dB/oct ±30 dB cell at 44.1 kHz is expected near 0.247 against 0.410); I-weak old → new; H1; I-cost old → new; the constants; negative controls; deviations.
- `SUMMARY-2.1b.md`: one line under the title: superseded by the rework summary; kept as the record of the blocked run.
- `SUMMARY-2.1.md`: a dated addendum: the seed changed for shelf / tilt sections with a pair above 0.95 π and Q above 1.5; the audit counts; the 2.1 INFO figures that moved.
- Carry-forward for verify 2.1b: re-verify the seed change (the audit, P1s) as well as the polish; `VERIFICATION-2.1.md` covers the unchanged lines only. For 2.2: no polish below 44.1 kHz; `Designer::prepare` must run before the first `design` at every rate change.
- `REQUIREMENTS.md`: no edit (DSP-01 closes at verify).
- STATUS: phase rows, `next_action: stage_2_verify_phase_2_1b`, `blocked_on` cleared, files list.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` the new paths by name (`Source/dsp/Polish.h`, `Source/dsp/PolishConstants.h`, `tests/model/polish_model.py`, `tests/model/dump_polish.py`, `tests/golden/designs-polished.tsv`, `.planning/stages/2-dsp/SUMMARY-2.1b.md`, `SUMMARY-2.1b-rework.md`), then `git commit -- plugins/O-ParametricEQ`. Never `-A` / `-a`. `git log -1 --stat` lists only this plugin's paths. No tag.
- Depends on: T9.

---

## Dependency graph

```
T1 → T2 → T3 (audit) → T4 → T5 → T6 → T7 → T8 → T9 (build, gates, controls) → T10
                                                  ↑   │
                                                  └───┘ port bug → fix, rebuild, rerun T9.2–9.8 in full
                                                  research disagreement → STOP
```

## Success criteria

| Criterion (ROADMAP Phase 2.1b as amended; ARCHITECTURE G1) | Proven by |
|---|---|
| Continuous on random lines at 44.1 / 48 kHz across nine families, and at 88.2 / 96 / 192 kHz | C1, C1-hi; C1n |
| The first build's two failing lines pass | C1 block A, printed by name |
| Never far worse than the seed off the gated grid; no runaway fit | NW2, RA1 |
| The new rows are not vacuous | `--neg first-build` (three rows), `--neg binary-gate` |
| Below 44.1 kHz the design is the closed form | PR1; F2 still covers 22.05 and 32 kHz |
| The re-centre is continuous and touches only its region | P1s; T3 audit against the verified golden |
| G1 at the 2.1b thresholds at five rates; seed and RBJ fail | G1b rows, G1bn-seed, G1bn-rbj (unchanged rows, re-run) |
| Per section never worse on the G1 grid; reciprocity; finite; skip exact; tilt identity; allocation-free; thread-independent | NW1, R2, F2, S1, TL2, A2, B2 (unchanged rows, re-run) |
| The C++ is the model, seed and polish | D1, D2 + golden negative controls |
| Phase 2.1's gated rows unchanged | T9.4 |
| Constants in their two homes only | T9.7 |
| No processor code; Stage 1 rows green | T9.8 |

## Risks

- **R1 The harness's lines are not the bench's.** Research's 0 jumps is on its own 5340 lines. A non-shrinking jump on a harness line is a finding: STOP (T9).
- **R2 C1-hi and step limit 0.35 at 88.2 kHz and above were not run together in research.** Expected clean (nothing changes at 96 kHz in the accuracy table). Binding; STOP if not.
- **R3 NW2's ceiling has 1.5x margin over one measured figure** (+0.67 against +1.0 dB) on a different draw. A reading between the two is a pass; above it is a STOP, not a new ceiling.
- **R4 The Python model has never run these two rules.** T3's audit and D1 / D2 are what tie it to the bench-derived C++. If the model and the C++ agree and both differ from research's figures, that is R1, not a port bug.
- **R5 The 72 dB/oct ±30 dB cell loses margin** (0.161 → 0.247 against 0.410 in the bench). Inside tolerance; report it.
- **R6 Run time.** C1 grows from 341 to 1317 lines and NW2 scores over 2 million sections. Run BG; if the whole harness passes about five minutes, thread NW2 as C1 is threaded. Do not cut line counts.
- **R7 Repository size.** A second 30 MB `designs.tsv` version enters history (RP13). `designs-polished.tsv` was never committed, so only its final version does.
