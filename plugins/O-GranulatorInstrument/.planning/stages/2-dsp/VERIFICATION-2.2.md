# Stage 2 — DSP · Phase 2.2 (Four streams, randomisation, windows, scale, quotas + thinning) — Verification

## Verification Date

2026-11-08

I re-ran every automated check below myself in the verify session, against `e90ce25a`. The plugin tree was clean beforehand. I touched `Engine.cpp` and `PluginProcessor.cpp`, rebuilt the harness and param-dump, re-extracted and re-built the parity reference from the pin (`tests/parity/run.sh`), and reinstalled with `build-and-install.sh`. No automated result here is copied from SUMMARY-2.2.md.

HANDS-ON-2.1 (13 rows) and HANDS-ON-2.2 (10 rows) are human gates. On 2026-11-08 the user deferred both until after Stage 3 (D14 override, recorded in SUMMARY-2.2). They are **not** self-certified here.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.2 / ROADMAP Phase 2.2)

1. Full grain-parameter parity with O-Granulator. Two arms:
   - D15 null: a derandomised sample null at ≤ −90 dB vs the parent @ `82406fce`.
   - Statistics: the random controls match the parent's latched distributions.
2. 4 streams × N voices with up to 80 schedulers, telescoping jitter and a deterministic (offset, serial, stream) merge. The block-size null must hold with every randomisation on.
3. Forked `WindowMorphBank`. Window 0 must be bitwise ≡ the 2.1 Hann render and law.
4. FUNC-09 scales: the quantiser ≡ the parent leaf, with degree masks per stream and per collection and the custom scale. D17 state: a lock-free publish, plus `<Engine>`-adjacent save/restore covering bit 63, absent → default and two reopens.
5. PERF-02 quotas on ≤ 80 pairs. Live ≤ cap at every tick, no starvation on either axis, and a lowered cap converges.
6. Output law with coherence (b): level flat ±1.5 dB.
7. A mixer with click-free 10 ms enable / mute / solo ramps.
8. D16: `kVoiceGainDb` from a measured RMS match (±0.5 dB), plus an ear row.
9. Goldens re-baselined once. The Stage 1 rows and param-dump stay byte-identical.
10. Fork drift 0, auval, pluginval 10, and parent code kept out of our binaries. CPU recorded as INFO only.

### Deliverables (SUMMARY-2.2.md)

- **New code:** `Source/dsp/GiScale.h`. `Engine.{h,cpp}` extended to 4 streams × 20 voices, 80 schedulers, the morph bank + P14 Hann fast path, ActiveSet + masks + the custom try-lock, the coherence-(b) law and the 4-stream mixer.
- **Parity reference:** `tests/parity/{run.sh, parent_render.cpp, ParityRows.h}`, built out of tree.
- **Gates:** `engine_gates_22.h` (29 rows) and `parity_gates_22.h` (PN / PS / SC / GV).
- **Goldens:** `tests/golden/renders-2.{1,2}.sha256`.
- **Constant:** `kVoiceGainMeasuredDb` changed from −6.0 to 0.02.
- `HANDS-ON-2.2.md`.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. DSP-01 parity | ✅ Achieved | Parent rebuilt from `git archive 82406fce` → `run.sh` rc 0, 0 parent self-check failures. `--parity`: **41 PASS / 0 FAIL**. PN01–PN26 residual **−110.9 … −120.2 dB** (relative to the reference power, threshold −90 dB, worst margin 20.9 dB); grain counts equal, `p0` off 0, raw size / density same on every row. PNc1 / PNc2 / PNc3 fire (−0.2 / −54.5 / −3.6 dB). PS1a–e, PS2, PS3a/b and PS4 pass, and every ×0.5 control fires; PS3b's engine `jitterScale 0.5` control fires too. |
| 2. 4 streams, determinism | ✅ Achieved | NB1: blocks 32 / 441 / 512 / 4096 / 8192 / var are bit-identical with all randomisation on, and the seed + 1 control differs. V3s: slot permutation 0 differ. N2s: 148 unread IDs → 0 differ. FS1: 16/16 rates exact, grain slot = stream, s3-disabled control silent. FS2: release spawns in every stream. |
| 3. Window morph | ✅ Achieved | W0a / W0c bitwise ≡ 2.1 (step 1 control differs). W0b: `forceBankPath` hash ≠ 2.1 (proves the fast path is the one under test). |
| 4. Scales + D17 state | ✅ Achieved (automated) | SC1: 0/864 ActiveSet cases differ from the parent (mask-bit control → 864). SC3: 0/5 custom parses differ. SC2: per-stream / per-collection masks. ST1–ST6: round trip incl. bit 63 (decimal / `getLargeIntValue` control clamps), absent → default, second reopen wins with 0 stale children, malformed hex ignored, the try-lock never blocks. A2 counts 0 audio-thread allocations across mask / custom swaps. Code: `GiScale.h:93-113`; the audio thread takes only `ScopedTryLockType`, and the blocking lock is limited to bind / prepare. |
| 5. Quotas | ✅ Achieved | QS1–QS7 pass with every control firing. R21 Q0–Q3 are unchanged inside 117/0. |
| 6. Level / coherence | ✅ Achieved | LF1, CO1 and CO2 pass; their `lawOff`, `coherenceVerbatim` and `coherenceOff` controls fire. |
| 7. Mixer clicks | ✅ Achieved (automated) | MX1 (`mixerRampZero` control fires) and V2s (2 steals, `stealRampZero` control fires). Ear rows: HANDS-ON-2.2 rows 2 and 8. |
| 8. D16 voice gain | ⚠️ Partial | GV0 target **+0.021 dB** (seed spread 0.021, old-vs-old 0). GV1 error **0.001 dB** ≤ 0.5; the −6 dB override control gives 6.020 dB. The ear confirmation (HANDS-ON-2.2 row 6) is pending. |
| 9. Re-baseline | ✅ Achieved | A fresh `--dump-render-hashes` is **byte-identical** to `tests/golden/renders-2.2.sha256`. Its `-pinned` lines match `renders-2.1.sha256` 7/7, and all 7 `-default` lines differ. RB1 passes in the harness. param-dump is byte-identical to `.planning/params.tsv`. |
| 10. Drift / validators / exclusion / CPU | ✅ Achieved | Drift 0 / 13 leaves. NC-a (byte appended to `Grain.h` in a parent-root built from the pin archive) → rc 1. NC-b (byte appended to `Quotas.h` in a local-root copy) → rc 1. The unmodified parent-root gives 0 drift. X1, X2, auval and pluginval are in Automated Checks. C22 INFO 14.45 %. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.2 of 6). As in 2.1, a requirement is closed only when **all** of its acceptance criteria are covered, including the human rows.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-01: Engine parity | must | ✅ Complete | Its single criterion is "1 voice, fixed key, scan 0 renders equivalently to the parent stream on frozen material (stated tolerance)". PN01–26 ≤ −110.9 dB vs −90 dB, with the shift / window / mip-phase controls firing. PS1–PS4 add the randomised-parameter statistics. HANDS-ON-2.2 rows 3–4 are by-ear confirmation, not part of the criterion. |
| FUNC-09: Scales | must | ⚠️ Partial | Automated: SC1 / SC3 match the parent bitwise, SC2 covers masks, PS4 the picks, ST1–ST6 the state. Open: HANDS-ON-2.2 row 5, and the custom-scale / mask editing surface (native functions and WebView, Stage 3). |
| FUNC-01: Polyphonic clouds | must | ⚠️ Partial | Now covered: the stream arm (FS1: 4-note chord × 4 streams, independently pitched, 16/16 exact rates) and steals with 4 streams (V2s). Open: HANDS-ON-2.1 rows 2 and 4, HANDS-ON-2.2 rows 1 and 8. |
| FUNC-02: File slots | must | ⚠️ Partial | Now covered: slots 2–4 audible (FS1). Open: drag-drop / browser (3.2), HANDS-ON-2.1 rows 1 and 8, HANDS-ON-2.2 row 1. |
| DSP-02: Amp envelope | must | ⚠️ Partial | Now covered: grains continue during release in every stream (FS2). Open: HANDS-ON-2.1 row 5, HANDS-ON-2.2 row 9. |
| PERF-01: RT safety | must | ⚠️ Partial | Now covered: A2 gives 0 allocations with 4 streams, scale / mask / custom swaps, enable / mute / solo and a cap change. Open: reset / freeze (2.4), matrix (2.5). |
| PERF-02: Pool + CPU budget | must | ⚠️ Partial | Now covered: quotas on 80 pairs (QS1–QS7). C22 INFO is recorded (14.45 %, no verdict). Open: the budget verdict with FX (2.3). |
| QUAL-01: No artifacts | must | ⚠️ Partial | Now covered: MX1 and V2s. Open: reset / freeze (2.4), plus the HANDS-ON rows. |
| FUNC-03, FUNC-04, DSP-07, PERF-03 | must | ⚠️ Partial | Unchanged from 2.1 (their HANDS-ON-2.1 rows are pending). |

Still pending (later phases): FUNC-05, FUNC-06, FUNC-07, DSP-03, DSP-04, DSP-05 and DSP-06.

**Requirements Summary (after this phase):**
- ✅ Complete: 1 (DSP-01)
- ⚠️ Partial: 11 (FUNC-01–04, FUNC-09, DSP-02, DSP-07, PERF-01–03, QUAL-01)
- ⏸️ Deferred (later phase or stage): 16
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness + param-dump) | ✅ Pass | rc 0 after the touch-rebuild; 0 warnings in the build log. |
| build-and-install.sh | ✅ Pass | rc 0, 19 s. Only `O-GranulatorInstrument-dev.{vst3,component}` are on disk; no alternate variant. |
| Standalone | ✅ Pass | Alive after 6 s, **after a rebuild**. The `OuariconGranulatorInstrument_Standalone` artefact on disk was dated 2026-11-07 22:23, before 2.2; `build-and-install.sh` does not rebuild it. I relinked it (rc 0) and re-ran the check. |
| Render harness (default) | ✅ Pass | **117 PASS / 0 FAIL**, rc 0: 55 Stage 1 + 33 Phase 2.1 + 29 eg22 rows, all 29 eg22 IDs present. 51 `ctrl FIRED` lines and 0 `NOT FIRED`. |
| Parity harness (`--parity`, fresh parent build) | ✅ Pass | `run.sh` rc 0 (0 self-check failures, 864 ActiveSet cases, 5 custom texts). `--parity` **41 / 0**; `--rows GV` **2 / 0** (GV0 + GV1). |
| Render goldens | ✅ Pass | The re-dump is byte-identical to `renders-2.2.sha256`; pinned = 2.1 on 7/7. |
| param-dump | ✅ Pass | Byte-identical (`cmp`) to `.planning/params.tsv`. |
| Fork drift check | ✅ Pass | 0 drift / 13 leaves. NC-a and NC-b were re-made independently, and both return rc 1. |
| X1 compdb exclusion | ✅ Pass | 151 O-GranulatorInstrument entries: 0 match `O-Granulator/Source\|ogi-parity\|82406fce`, and 0 dev-loader references in the console TUs. Control: the parity project's compdb has 92 `O-Granulator/Source` entries. |
| X2 symbol exclusion | ✅ Pass | `nm -C` finds 0 `ogran::` in the installed VST3, the installed AU and the harness. Control: `proj-parity` has 76. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. The `ZZZZ` control gives rc 2. |
| pluginval 10 VST3 (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0; `Main bus num input channels: 2`; 0 `!!!` lines. |
| pluginval 10 AU (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0; 2 ch in. The only `!!!` line is the informational "Current program is -1". |
| C22 CPU (INFO, run alone) | ℹ️ INFO | Median **14.45 %** (14.42–14.48) of real time over 7 × 10 s at 48 k / 128, 16 notes × 4 streams × 4 slots, d 200, cap 256, no FX. Mean live 243.2 (saturated); duty witness 0.997. Bypass 0.306 % → 47.3×. Below the 28 % flag, and it agrees with SUMMARY (14.36 %). |
| Git | ✅ Pass | No uncommitted plugin changes before verify. No `O-GranulatorInstrument*` tag. |

## Human Verification

HANDS-ON-2.1 (13 rows) and HANDS-ON-2.2 (10 rows) are **pending**. The user deferred them on 2026-11-08 until the Stage 3 UI is in place, and they are run together then:
- [ ] HANDS-ON-2.1 rows 1–13 (Standalone + DAW; row 10 Standalone only)
- [ ] HANDS-ON-2.2 rows 1–10 (Standalone + DAW)
- [ ] HANDS-ON-2.2 row 6: confirm the voice gain or give a trim. A trim goes to `kVoiceGainEarTrimDb` and re-runs GV1 + RB2.

## Issues Found

- **None blocking** in the automated scope.
- **SUMMARY-2.2 PN residual figures are stale.** SUMMARY records PN −122.9 … −132.0 dB and a fitted gain of 1.995. Those come from T11, run with the −6 dB placeholder before T12 set `kVoiceGainMeasuredDb = 0.02`. At HEAD the fitted gain is ≈ 0.998 and the residuals are **−110.9 … −120.2 dB**: still ≤ −90 dB, with a 20.9 dB worst margin.
  - The ~6 dB gain change alone does not account for a ~12 dB residual shift. I did not isolate the cause; doing so would need a temporary source edit, which verify does not make.
  - Not a failure. It is recorded so later phases compare against the HEAD figures, not SUMMARY's.
- **The Standalone is not rebuilt by `build-and-install.sh`.** The on-disk app predated 2.2 until verify relinked it. Any future Standalone check (including HANDS-ON) must build `OuariconGranulatorInstrument_Standalone` first.
- **Carried, by design:**
  - The FX and the CPU verdict: 2.3.
  - Feedback / freeze / reset / reverb: 2.4.
  - Modulation / MPE / sync: 2.5.
  - Record-in / `<Slots>`: 2.6.
  - Scale / mask UI: Stage 3.
  - `DevLoaderEditor` is removed in 3.1.
  - Module-gap G5 is unchanged.
  - PD4's sibling `<CustomScale>` / `<DegreeMasks>` refines D17's "keys on `<Engine>`". `parameter-spec.md` was not edited.
- **Plan deviations reviewed and accepted.** Every one is listed in SUMMARY-2.2 §Deviations.
  - The parity geometry (8.25 s buffer) and the PS3 length of 5.02 s are stimulus fixes: the anchor self-check caught the first and n < 4000 caught the second. The closed form is unchanged.
  - The gate-stimulus changes (MX1 / V2s spray 30 %, LF1 spray 10 %, QS2 N = 4 arm, FS1 mute-difference) keep a firing control each, and no threshold was loosened.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`, O-GrainScatter) are not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green. HANDS-ON-2.1 and HANDS-ON-2.2 (human) are pending, deferred by the user until after Stage 3.

**Ready for next phase:** Yes, under the user's D14 override. Phase 2.3 is per-grain FX (SVF, drive, crush, ring) and the PERF-02 CPU verdict. The hands-on sheets run after Stage 3, and any ❌ there goes back to `/plugin-execute O-GranulatorInstrument 2-dsp` for the owning phase.

**Blockers:** none for 2.3. Open human gates: HANDS-ON-2.1 and HANDS-ON-2.2 (deferred).
