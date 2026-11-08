# Stage 2 — DSP · Phase 2.3 (Per-grain FX + CPU budget) — Verification

## Verification Date

2026-11-08

I re-ran every automated check below myself in the verify session, against `9fc9289c`. The plugin tree was clean beforehand.
- I touched `Engine.cpp` and `PluginProcessor.cpp` and rebuilt the harness and param-dump.
- I re-extracted and re-built the parity reference from the pin into a fresh work dir (`tests/parity/run.sh`).
- I reinstalled with `build-and-install.sh` and relinked the Standalone explicitly.

No automated result here is copied from SUMMARY-2.3.md.

HANDS-ON-2.1 (13 rows), HANDS-ON-2.2 (10 rows) and HANDS-ON-2.3 (9 rows) are human gates. On 2026-11-08 the user deferred all three until after Stage 3 (D14 override). They are **not** self-certified here.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.3 / ROADMAP Phase 2.3)

1. **DSP-03 null (D18).** FX null rows vs the parent @ `82406fce` reach ≤ −90 dB, for each FX alone and chained, with crush_prob 0 / 100. Each family's control must fail the null.
2. **Law rows (D18).** The parent's GrainFx unit / law rows are ported and green, and every stage is block-size invariant.
3. **Statistics (D18).** filter_rand, ring_rand and crush_prob match the parent within the CI, and the wrong-distribution controls fail.
4. **No state bleed.** Slot reuse is bitwise equal to a fresh pool, and the skip-reset control fails.
5. **PERF-01 arm.** 0 allocations with all FX on.
6. **QUAL-01 arm.** No NaN / Inf / subnormal under stress at 44.1 / 48 / 96 k, and the NaN kill fires on an injected NaN.
7. **True bypass.** FX-off renders are byte-identical to renders-2.2, param-dump is identical and fork drift is 0.
8. **D19.** The CPU worst case is ≤ 36 % with a valid witness, measured against a same-session FX-off reference.
9. **D20.** The typical proxy is ≤ 12 %.
10. **Validators.** auval and pluginval 10 (VST3 + AU) pass, and parent code stays out of our binaries.

### Deliverables (SUMMARY-2.3.md)

- **Engine:**
  - `Source/dsp/GiFxLatch.h` (new): the parent `planFx` / `assignFxLatch` with renames only, `svfMake` latched.
  - `GiRenderer::renderGrain` is now gather → FX → scatter, with the SVF NaN kill before the scatter.
  - `ControlGrid.h` binds the 10 FX rows × 4 streams (ADAPTED, re-hashed).
- **Gates:** `fx_units_23.h` (FU), `engine_gates_23.h` (FN / FR / FV / FB / FZ / FA3 + `--cpu23`), `parity_gates_23.h` (PF / PFc / PS5–7), `cpu_calib.h`, plus the frozen `tests/golden/cpu-calib-2.3.txt`.
- `HANDS-ON-2.3.md` (9 rows, deferred).
- Unchanged: `GrainFx.h`, which is byte-identical to the pin.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. DSP-03 null | ✅ Achieved | Fresh parent build from `git archive 82406fce`: `run.sh` rc 0, 0 parent self-check failures. `--parity` gives **99 PASS / 0 FAIL**. **PF0** is −149.3 dB with g₀ 0.9977000 (threshold −130). The 48 PF rows are all ≤ −90 dB; worst first: LP −126.2 (PFL3), BP −126.1 (PFB3), HP −131.2, drive −148.8, crush −148.8, ring −149.1, chained −151.7. Every row has equal grain counts, 0 bad `fxActive` and the same raws. The gain band reaches **1e-7 at most** (band 1e-4). PFc1–5 fire (−21.0 / −46.3 / −0.2 / −1.0 / −39.8 dB). PFc6 (`legacySvfMake`) has its null pass at −129.5 dB and its band fail at −0.684, so the band catches a latched-scalar error that alignment hides. |
| 2. Law rows + block size | ✅ Achieved | **FU (15 rows):**<br>• BP at fc: worst −0.0025 dB.<br>• Oracle: 249 cases, worst +0.0003 dB; the inline control without × make reads −10.0 dB.<br>• 0.45·fs clamp: peak 1.35.<br>• Drive: 0.5 ulp.<br>• Crush: lattice / hold / silence exact; the FU3n control fires.<br>• Ring: drift −141.0 dB; the FU4n control reads −115.6.<br>**FN1:** 33/33 bitwise across chunks 1 / 3 / 17 vs 32; the per-call renorm control gives 174 230 differences.<br>**FN2:** blocks 441 / 512 / 4096 / 8192 / var give 0 differing samples with every FX and a filter_type switch; the seed + 1 control gives 127 998. |
| 3. Statistics | ✅ Achieved | n = 4000 per side for each row.<br>• **PS5:** D2 0.017 (≤ 0.060).<br>• **PS6:** D2 0.028.<br>• **PS7:** p̂ 0.3035 vs 0.2993, and the crush bit equals `crushed` on every grain on both sides.<br>Each row's control fires. Engine-side census rows FV1a–d, FV2 (ρ 0.999), FV3 (58/58) and FV4 (78/78) pass; their `fxSharedLatch` / wrong-table controls fire. |
| 4. No bleed | ✅ Achieved | **FB1:** max\|Δ\| exactly 0 over 12 811 spawns (6616 kills and 6209 steals); poison + `fxNoReset` gives 0.0558. **FB2:** B on silence is exactly 0.0 in the same slot; the control gives 6.7e-4. **FB3:** 2146 steals, 0 mismatches. **FB4:** 0 non-default FX state at spawn; the control gives 12 811. |
| 5. Allocations | ✅ Achieved | **FA3:** 0 audio-thread allocations over 116 blocks, with every FX on, type switches, 19 steals, cap 256 → 32 and 1 NaN kill. The liveness malloc control gives 1. |
| 6. Stress / NaN | ✅ Achieved | **FZ1** at 44.1 / 48 / 96 k (res 100, drive / crush / ring 100, +48 / −24 st, full scale): 0 non-finite values, 0 subnormal FX state and 0 NaN kills. The controls fire: `injectNan` + `nanKillOff` gives 4703 non-finite values, and `denorm_min` gives 1. **FZ2:** 13/13 injected NaN / ±Inf cases each give 1 kill, the slot is retired, and the output equals the `forceRetire` render. Code: `GiRenderer.h:247-262`. The kill returns before the scatter, and the stage order is SVF → drive → crush → ring (`:275-281`; the swapped branch is only the `OGI_TEST_HOOKS` PFc5 control). |
| 7. Bypass / drift | ✅ Achieved | **FR1** is 14/14 = renders-2.2. A fresh `--dump-render-hashes` is **byte-identical** to `tests/golden/renders-2.2.sha256`. param-dump is byte-identical (`cmp`) to `.planning/params.tsv`. The PN01–26 residuals are unchanged at **−110.9 … −120.2 dB**. Drift is **0 / 13**. `GrainFx.h` is `d92940e5…` = manifest (VERBATIM) and `ControlGrid.h` is `ba86d4d3…` = manifest. NC-a and NC-b were re-made independently. |
| 8. D19 worst case | ✅ Achieved | **C23w 26.01 %** (25.88 … 26.04), live 243.2, duty ≥ 0.995, witness valid. That is **≤ 36 %**, a margin of 9.99 points. |
| 9. D20 typical | ✅ Achieved | **C23t 5.38 %** (5.35 … 5.44), live 72.0. That is **≤ 12 %**. |
| 10. Validators / exclusion | ✅ Achieved | See Automated Checks. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.3 of 6). As in 2.1 and 2.2, a requirement closes only when **all** of its acceptance criteria are covered, including the human rows.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-03: Per-grain FX | must | ⚠️ Partial (was pending) | The criterion is "Each FX audibly applies per grain; no per-grain allocation". **Automated arm complete:**<br>• Per grain: FV2–FV4, PS5–7 and PF nulls vs the parent.<br>• No allocation: FA3.<br>• No bleed: FB1–4.<br>• NaN safety: FZ1–2.<br>**Open:** "audibly", HANDS-ON-2.3 rows 1–7. |
| PERF-02: Pool + CPU budget | must | ⚠️ Partial | **Now covered:** D19 at 26.01 % ≤ 36 % (FX worst case) and D20 at 5.38 % ≤ 12 % (proxy).<br>**Open:**<br>• The final 40 % verdict with 4 feedback chains + reverb (2.4).<br>• The typical verdict on factory presets (Stage 4). |
| PERF-01: RT safety | must | ⚠️ Partial | **Now covered:** FA3 with all FX on.<br>**Open:** reset / freeze (2.4), matrix (2.5), file swap (2.6). |
| QUAL-01: No artifacts | must | ⚠️ Partial | **Now covered:** the FZ1 / FZ2 stress and NaN arm.<br>**Open:** reset / freeze click gates (2.4), plus the HANDS-ON rows. |
| DSP-01 | must | ✅ Complete (unchanged) | The PN residuals are identical at HEAD. |
| FUNC-01–04, FUNC-09, DSP-02, DSP-07, PERF-03 | must | ⚠️ Partial | Unchanged from 2.2. Their HANDS-ON-2.1 / 2.2 rows are pending. |

Still pending (later phases): FUNC-05, FUNC-06, FUNC-07, DSP-04, DSP-05 and DSP-06.

**Requirements Summary (after this phase):**
- ✅ Complete: 1 (DSP-01)
- ⚠️ Partial: 12 (FUNC-01–04, FUNC-09, DSP-02, DSP-03, DSP-07, PERF-01–03, QUAL-01)
- ⏸️ Deferred (later phase or stage): 15
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness + param-dump) | ✅ Pass | rc 0 after the touch-rebuild; 0 warnings. |
| build-and-install.sh | ✅ Pass | rc 0, 0 plugin warnings. Only `O-GranulatorInstrument-dev.{vst3,component}` are on disk. |
| Standalone | ✅ Pass | Relinked explicitly (`OuariconGranulatorInstrument_Standalone`, rc 0). The binary (16:35:10) is newer than the harness (16:33:12). Alive after 6 s. |
| Render harness (default) | ✅ Pass | **150 PASS / 0 FAIL**, rc 0. The 33 Phase 2.3 rows (FU 15, FN 3, FR 1, FV 7, FB 4, FZ 2, FA 1) all pass. **70** `ctrl FIRED` and **0** `NOT FIRED`. |
| Parity (`--parity`, fresh parent build) | ✅ Pass | `run.sh` rc 0 (0 self-check failures, 32 s). `--parity` **99 / 0**; `--rows GV` **2 / 0**. Breakdown in Goal 1. |
| Render goldens | ✅ Pass | The re-dump is byte-identical to `renders-2.2.sha256`, and its control fires (N1 seed + 1 → different hash). |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv`. |
| Fork drift | ✅ Pass | 0 / 13. Unmodified parent root from the pin archive: rc 0. **NC-a** (byte appended to `Grain.h` in that root) → rc 1. **NC-b** (byte appended to `GrainFx.h` in a local-root copy) → rc 1. The parent `Source/dsp` and grain-engine are unchanged since the pin. |
| X1 compdb exclusion | ✅ Pass | 151 O-GranulatorInstrument entries → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. Control: the parity compdb has 91 `O-Granulator/Source` entries. |
| X2 symbol exclusion | ✅ Pass | `nm -C` finds **0** `ogran::` in the installed VST3, the installed AU and the harness. Control: `proj-parity` has 76. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. The `ZZZZ` control gives rc 2. |
| pluginval 10 VST3 (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, `Main bus num input channels: 2`, 0 `!!!` lines. |
| pluginval 10 AU (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, 2 ch in. The only `!!!` line is the informational "Current program is -1". |
| `--cpu23` (run alone) | ✅ Pass | **2 PASS / 0 FAIL**, rc 0. Witness `ref_ns` 40 523 708 (frozen). Every config's calib-before is between 0.988 and 0.993 × ref, after / before is ≤ 1.002, and duty is ≥ 0.990, so no NOVERDICT.<br>• Bypass 0.31 %.<br>• **C22r 14.07 %** (FX off, same session).<br>• **C23w 26.01 %** ≤ 36.<br>• **C23t 5.38 %** ≤ 12 (live 72.0).<br>• C23s −0.30 points (the restructure costs nothing).<br>• C23w96 51.06 % (INFO).<br>Δ FX = 11.94 points = **0.049 %/grain** (parent 0.064). These agree with SUMMARY (26.08 / 5.41) within 0.07 points. |
| Git | ✅ Pass | No uncommitted plugin changes before verify. No `O-GranulatorInstrument*` tag. |

## Human Verification

HANDS-ON-2.1, 2.2 and 2.3 are **pending**. The user deferred them on 2026-11-08 until the Stage 3 UI is in place, and they are run together then:
- [ ] HANDS-ON-2.1 rows 1–13
- [ ] HANDS-ON-2.2 rows 1–10, including row 6 (voice-gain ear trim)
- [ ] HANDS-ON-2.3 rows 1–9. These include the two listening notes: the R22 crush silence threshold (row 4) and the R19 LP / HP resonance vs the wet safety (row 6).

## Issues Found

- **None blocking** in the automated scope.
- **Stress output exceeds full scale, as expected for now.** In FZ1 the bus peaks at 1.53 / 1.71 / 1.73 (≈ +3.7 … +4.8 dBFS) at 44.1 / 48 / 96 k, under res 100 + drive / crush / ring 100 on full-scale material with +48 st. It is finite and printed as INFO; nothing gates the level yet. The output limiter / ceiling arrives with feedback in 2.4 (DSP-04: "never exceeds the limiter ceiling"). Carry this stimulus into 2.4's ceiling row.
- **CPU-run environment.** Logic itself was closed. Only its QuickLook / Thumbnail extensions were resident (idle). The duty witness (≥ 0.990) and the calib brackets confirm the run was uncontended.
- **Carried, by design:**
  - The final 40 % worst case with chains + reverb: 2.4.
  - FX destination modulation (b+17…b+25): 2.5.
  - The typical verdict on factory presets: Stage 4.
  - The FX UI: Stage 3.
  - `DevLoaderEditor` is removed in 3.1.
- **Plan deviations reviewed and accepted.** All of them are listed in SUMMARY-2.3 §Deviations 1–8. These are the FB1 / FB4 stimulus fixes (cap drop, `thinningOff`), the FZ1 control retargeting, FV1d's 10th config, the extra test hooks and the parity plumbing. Each keeps a firing control and no threshold was loosened. X2 = 0 confirms the hooks are absent from the shipping binaries.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, O-GrainScatter) are not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green, including both CPU verdicts. HANDS-ON-2.1, 2.2 and 2.3 (human) are pending, deferred by the user until after Stage 3.

**Ready for next phase:** Yes, under the user's D14 override. Phase 2.4 covers working-copy feedback, Freeze, Reset, reverb and the final 40 % PERF-02 worst case (13.99 points of headroom today). The hands-on sheets run after Stage 3, and any ❌ there goes back to `/plugin-execute O-GranulatorInstrument 2-dsp` for the owning phase.

**Blockers:** none for 2.4. Open human gates: HANDS-ON-2.1, 2.2 and 2.3 (deferred).
