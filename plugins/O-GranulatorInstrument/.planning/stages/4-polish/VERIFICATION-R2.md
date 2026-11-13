# Stage 4 — Polish / Validation — Verification, Rework R2 (feedback holds the lane level)

## Verification Date

2026-11-13

Follows `VERIFICATION-R1.md` (PARTIAL, finding F3). Every automated check below was re-run in this session on `main` @ `5e0f5b95` (plugin sources = C2 `b7d025f7`; plugin tree clean before and after except the untracked `stages/3-gui/gate-report.json`, not from this session).
- Every plugin and harness `.cpp` was touched and rebuilt: harness, param-dump, VST3, AU, Standalone, rc 0.
- The parity reference was re-extracted from the pin (O-Granulator `82406fce`) and rebuilt in a scratch directory.
- Logic and Live were closed throughout.

No automated result is copied from SUMMARY-R2. The human retest (45 rows of `HANDS-ON-SESSION-4.md`) is **not** self-certified and is open. The Windows CI arm (D54) was not run.

Not re-done: `--egr2-long` (LH11, 600 s, INFO only); the LH8i INFO comparison of latch thresholds beyond what `--egr2` prints.

## Goal-Backward Analysis

### Original Goals (CONTEXT-R2 D70–D77, PLAN-R2 success criteria)

1. A lane under feedback holds its level for 60 s at Amount 100 / 50 / 25 %, Post and Pre, and the same rows fail with the makeup off.
2. No overshoot, no lift of a silent or near-silent lane, the reference follows the loop region, the cap still lets a lost lane fade.
3. After note-off the lane is left within 3 dB; a releasing voice does not re-open the head; the pedal keeps it writing.
4. Loop bound unchanged; return clamping ≤ 1 % on the presets.
5. Block-size invariant, deterministic, 44.1 and 96 kHz in band.
6. Feedback 0, the switch Off and Freeze bitwise; render goldens byte-identical; parity 168 / 0; makeup off == the R1 engine.
7. CK1, CK4, CK7n pass with controls that fire, on the shipping path.
8. 51 presets bitwise; level outliers trimmed with `sN_level` only; FP2 spread ≤ 9 dB.
9. C25m ≤ 40 %; every preset T ≤ 12 % / W ≤ 40 %.
10. 293 parameters, 150 destinations, page, strings, `params.tsv`, forked leaves untouched.
11. Build, install, auval (4 LFO-rate warnings exactly), pluginval 10 sidechain on / off, X2.
12. Docs and release metadata; VERSION 1.0.0, no tag, no push.

### Deliverables (SUMMARY-R2)

C1 `16fde35c` (planning + baseline), C2 `b7d025f7` (DSP, rows, trims, evidence), C3 `57cbcadc` + `5e0f5b95` (docs, sheet, summary).

Code read in verify (`git diff 4c89247c..HEAD -- Source`):
- **Scope:** six source files, all inside the fence (`GiRecordHead.h`, `SlotTape.h`, `SlotLoader.cpp`, `Engine.h`, `Engine.cpp`, `FactoryPresets.h`). `git diff 4c89247c -- Source/dsp/forked` is empty. Nothing under `Source/ui`, `PluginProcessor.*`, `CMakeLists.txt`, `modules/` or `scripts/` moved.
- **No transcendental call at the tick.** Every `exp` / `pow` of the makeup is in `MakeupCoef::prepare`, called once from `Engine::prepare`. `writeCell` uses arithmetic and `sqrt`. The one `pow` in `writeHeads` and the `log10` in the statistics sit inside `#if OGI_TEST_HOOKS`.
- **Makeup-off path keeps the R1 shape.** With `f.makeup` false the gain block is skipped, `gPrev = gNew = 1`, the per-sample multiply and the three sums are not executed, and no running value is updated.
- **Bound.** The gain is applied before the ±0.7 writer clamp and the convex-envelope clamp, so the written sample stays inside `max (|wc|, 0.7)` for any finite gain; a non-finite gain becomes 1.
- **Latch** (`Engine::updateHeads`): evaluated at every tick, not behind a dirty check. Cleared only by an enabled, audible stream with a gated voice (key down or pedal-held); set only with the makeup on and `shortRun ≥ latchCells`. A parked head resets the makeup state, so the run count cannot carry over.
- **Reference table:** `buildRef` has one caller, `SlotLoader::buildTape` (the only `make_unique<SlotTape>`), on the worker thread. `refMeanSq` is O(1) and returns 0 for an empty table, which gives gain 1.
- **Bank:** the `FactoryPresets.h` diff is 16 changed lines; with the `sN_level` values masked the old and new lines are identical.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Lane holds | ✅ Achieved (automated) | LH1 min −2.1 / −2.1 / −2.9, max +1.3 / +1.3 / +1.5, margin 0.15 dB; LH1p, LH2, LH3 PASS; controls LH1n / LH1pn / LH2n / LH3n fire. The lane is measured from the working copy directly, not from the makeup's own statistics. Listening = rows 2.4-1 – 2.4-7, R2-1. |
| 2. Overshoot, silence, region, cap | ✅ Achieved | LH4, LH5, LH6 (end −1.7, min −3.6, max +1.1), LH6u, LH7 (end −155.6 dB, capped 100 %); controls fire. |
| 3. Note-off | ✅ Achieved (automated) | LH8 −0.3 / −0.6 dB, pedal arm writes 19.0 s; LH14 latch 1658 ms after note-off, 0 writes in 1 s of release, resume 0.98 ms after a new note; LH8n, LH14n fire. |
| 4. Bound, clamp pressure | ✅ Achieved | FL1 / FL1p / FL1s / FL1sp / FL2 / FL2p / CL1 PASS in the 543; LH10 worst 0.548 % (Fifth Bloom s3). |
| 5. Invariance, rates | ✅ Achieved | LH9 PASS; LH13 44.1 kHz −1.8 … +1.0, 96 kHz −3.2 … +0.9, end −0.6. |
| 6. Nothing else moved | ✅ Achieved | Render hashes 14 / 14 identical, 7 pinned lines == `renders-2.1`, `ctrl FIRED`. Parity 168 / 0 incl. GV; 34 PB / PR lines == `rb-parity.txt`. Makeup-off run: see Issue 1. |
| 7. Click rows | ✅ Achieved | CK1, CK4, CK7, CK7n PASS in the full run, text equal to `R2.full.txt`. |
| 8. Presets | ✅ Achieved (automated) | PT0r 51 / 51 bitwise, PT3 13 / 13, PT3n fires. 64 `$RB` raws == `rb-preset-renders.sha256`; fresh dump == `r2-preset-renders.sha256` (64 / 64). Level table == `r2-levels.txt`. FP2 PASS. Audition = Pass 2. |
| 9. CPU | ✅ Achieved | Run alone: C25w 28.79 %, C25t 5.77 %, C25m 29.65 %, C25tm 6.69 %, C26rec 29.64 %, C25mPre 30.93 % (INFO). `--cpu4` 66 / 0: T median 4.23 %, p90 6.48 %, max 8.97 % (Fifth Bloom), W max 10.11 %. |
| 10. Surface | ✅ Achieved | param-dump == `params.tsv` (293); 7 / 7 contract hashes == STATUS before the REQUIREMENTS edit; fork drift 0 / 13; UI chain green with no file changed; three v2 pairs `cmp` clean. |
| 11. Validators | ✅ Achieved | See Automated Checks. |
| 12. Docs / release | ✅ Achieved | One `## [1.0.0] - 2026-11-13` heading with the level-hold sentence; CMake and installed `Info.plist` 1.0.0; PLUGINS.md `🚧 Stage 4 \| 1.0.0 \| 2026-11-13`, no duplicate row; 0 `*GranulatorInstrument*` tags; sheet has section 1e (R2-1 – R2-12) and the reset rows. |
| Human retest (D51) | ⏳ Pending | 45 rows open on the R2 build. |
| Windows (D54) | ⚠️ Partial (by decision) | Grep gate PASS. No Windows compile until `/publish`. |

## Requirements Verification

**Stage:** stage-4. Three requirements edited (PD16); checksum re-pinned in STATUS.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-04: feedback into per-stream working copies | must | ⚠️ Partial | R2 evidence line added (LH rows, lower bound, controls). Automated arm green; closes on the retest of 2.4-1 – 2.4-7, section 1d, R2-1 – R2-12. |
| FUNC-08: factory content | should | ⚠️ Partial | R2 evidence line added (PT0r / PT3, ten trims, FP2 7.00 dB, Factory folder). Closes on the re-audition of the 14 feedback presets. |
| PERF-02: CPU budget | must | ✅ Complete (was partial) | The `--cpu4` re-measure it waited on is done and inside budget; the Logic CPU row passed on R1 and was not reset for R2. |
| FUNC-02, UI-01, COMPAT-02 | — | ⚠️ Partial (unchanged) | Windows rows (D54), MPE / Dorico (D57). |

Totals: **20 complete, 5 partial**, 0 pending, 0 failed.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (5 targets) | ✅ Pass | rc 0; 0 warnings in `Source/`; 14 in harness headers (`parity_gates_22 / 23.h`, `engine_gates_25a / b / d.h`), the rest SDK headers. |
| Render harness | ✅ Pass | **543 PASS / 0 FAIL**, rc 0, 0 NOT FIRED, 0 INVALID. The 512 ids of `B.ids.txt` all present and PASS; the 31 extra ids are the LH rows by name. Text differs from `R2.full.txt` on the wall-clock rows only (CM1 H4 SR1 UCM6b UCM6bn USV2 USV3 USV3n). |
| `--egr2` | ✅ Pass | 31 / 0; 14 of 14 control rows print FIRED. |
| Makeup-off run | ✅ Pass (one flake) | 510 / 33. Carried rows equal `B.full.txt` except CK1 / CK4 / CK7 / CK7n, FP2 / FP6n, the wall-clock rows — the list SUMMARY-R2 gives — and USV1 (Issue 1). The LH rows fail, as they must. |
| Render goldens | ✅ Pass | 2.4, legacy 2.2, 7 pinned 2.1 lines identical. |
| Parity (fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures; 168 / 0. |
| Preset renders | ✅ Pass | `--cmp-preset-renders $RB --r2`: 3 / 0. |
| param-dump, fork drift, overrides | ✅ Pass | Identical; 0 / 13; two dumps identical and == committed. |
| CPU (alone) | ✅ Pass | Values in goal 9. |
| UI chain (no-change proof) | ✅ Pass | `ui_frontend_check.js` ALL PASS, `--nc` 4 / 4; `ui_binding_probe.js` ALL PASS (293 × 2, value entry 15 / 15, lanes 12 / 12, DM2 161 / 149), `--nc` live; check-i18n · fr-lint · zh-lint 0 · 0 · 0; check-ui-labels `param-dump (293 rows) + overrides`, `default + 48`, ALL CHECKS PASSED; boot-all-uis `--strict-tips` rc 0, 0 DEAD; PN1 ALL PASS; Windows grep gate PASS. |
| build-and-install.sh | ✅ Pass | rc 0; installed VST3 + AU binaries `cmp`-identical to the build; only the `-dev` variant on disk. |
| auval `aumu OuGI OuDv` | ✅ Pass | SUCCEEDED before the gate variant and after the restore; exactly 4 "did not retain default value" warnings. |
| pluginval 10, sidechain on | ✅ Pass | VST3 ×3 seeds rc 0, 2 ch in, 0 `!!!`; AU ×3 rc 0, the one carried `!!!` line. Seeds `0x1a2b3c`, `0x51ced`, `0x7e57`. No `rec_arm` line. |
| pluginval 10, sidechain off | ✅ Pass | `OGI_GATE_SC_DEFAULT_OFF=ON`: VST3 ×3 rc 0 with 0 ch in; AU ×3 rc 0. |
| Restore | ✅ Pass | Cache variable `OFF`; rebuilt and reinstalled, installed == built; pluginval 10 VST3 rc 0 with 2 ch in; `auval -a` lists one line. |
| X2 | ✅ Pass | Shipping VST3 / AU / Standalone: 0 hook symbols, 0 `stub_` strings, 0 `makeupOff` strings; harness control 9 / 9. |
| Factory folder | ✅ Pass | 64 `.json` + 2 sentinels, `.bank-hash` 1a994b77e891b91b, byte-identical to `--dump-factory-presets` from this build; 0 files with `"output_gain"`. |
| Real home | ✅ Pass | Listings of `~/Library/O-GranulatorInstrument` and `~/Music/Ouaricon` identical before and after the harness, parity, CPU and param-dump runs. |
| Git | ✅ Pass | `main`; no tag. |

## Human Verification (D51 — `HANDS-ON-SESSION-4.md`)

**Not run.** 255 run rows: 210 ✅ carried from 2026-11-12, **45 to run on the R2 build**.

- [ ] Pass 1: 2.4-1 – 2.4-7, 3.2-10, 3.2-11, 3.2-24, section 1d (8 rows), section 1e R2-1 – R2-12 (30 rows)
- [ ] Pass 2: the 14 presets with a Feedback Amount above 0 (14 rows)
- [ ] Pass 3: 2.4-9 (1 row)

Four rows ask for a decision, not only a mark:
- **R2-3** — latch threshold. 6 dB ships; execute measured −4.9 dB at note-off in Pre / Mix 100 / Size 100 against −1.4 dB at 3 dB.
- **R2-5** — brightness walk on Dotted Echo, Feedback Loop, Tape Halo, Low Orbit.
- **R2-9** — a sparse lane becomes a steady cloud.
- **R2-10 (b)** — a filtered drone no longer fades; it becomes the filter's residue at level.

R2-6 is a listening call on the seven presets whose tone moved most.

**Decisions recorded 2026-11-13 (Taylor):** R2-3 keep the 6 dB latch threshold; R2-5 brightness walk accepted, cuts not re-voiced; R2-9 accepted; R2-10 (b) accepted. No source change follows from any of them. These are the decisions only: the ✅ / ❌ marks of the 45 rows are still open.

## Issues Found

None blocking.

1. **USV1 failed once in the makeup-off run; it is a timing flake in the row, not a neutrality break.** The failing clause was the "tape swapped while the copy runs" arm: `a new take lands after chunk 0/9: ''` instead of `'changed'`; every other clause equalled the makeup-on text. That arm has no head running. Re-run in its own group (`--ug32`): 5 / 5 PASS with `--makeup-off` (chunk 2, 1, 1, 2, 2) and 3 / 3 with the makeup on. The chunk index varies run to run, so the arm depends on thread timing; under load the swap can miss. Pre-existing (PLAN-3.2), not R2. Worth hardening before it costs a CI run.
2. **LH1's margin is 0.15 dB** (Saturate 100, lowest lap −2.9 against −3). It reproduced to the digit here, and the render is deterministic, but any later change to the feedback path will trip it first.
3. **LH6 and LH13 pass on the two-tier band** (end inside −3 / +2, no lap under −4), not the literal band: single laps reach −3.6 dB on a 2 s loop and −3.2 dB at 96 kHz. Accepted as SUMMARY-R2 deviation 2; the hold rows LH1 – LH3 keep the literal band.
4. **The lane rows read from 8 s.** The first two laps are covered by LH4 (overshoot) only; an early dip between 0 and 8 s would not be seen by LH1 – LH3.
5. Residual limits from SUMMARY-R2 stand and are in the sheet, not gated: hot lanes plateau lower; the Pre long-tail dip at note-off; the brightness walk; a filter can no longer empty a lane.
6. Deviations 1–9 in SUMMARY-R2 reviewed and accepted. Deviation 4 (the UI overrides fixture is dumped with the makeup off) means that fixture no longer shows a shipping-path overview; it is a UI data stub, so no gate depends on it.
7. CPU was measured while another session had work in the shared checkout; the figures are within 0.5 point of execute's.
8. Foreign working-tree changes (`modules/registry.yaml`, agent memory, `.planning/STATE.md`, O-ParametricEQ, O-GrainScatter) not touched.

## Stage Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is verified green on the R2 tree and F3 is fixed in the engine; the 45-row human retest has not been run.

**Ready for next stage:** No.

**Blockers:**
- The human retest on the R2 build (45 rows). The four decisions are taken; the marks are not.
- D54 Windows validate run at `/publish` (unchanged, by decision).
