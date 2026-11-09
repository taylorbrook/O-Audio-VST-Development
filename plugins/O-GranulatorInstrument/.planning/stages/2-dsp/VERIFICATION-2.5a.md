# Stage 2 — DSP · Phase 2.5a (Mod sources, 16-slot matrix with per-voice evaluation, newest-voice rule, Tempo) — Verification

## Verification Date

2026-11-09

I re-ran every automated check below myself in the verify session, against `98836951`. The plugin tree was clean beforehand.
- I touched `Engine.cpp` and `PluginProcessor.cpp` and rebuilt the harness and param-dump (rc 0, 0 warnings).
- I re-extracted and re-built the parity reference from the pin into a fresh work dir (`tests/parity/run.sh`, scratchpad, never in the repo).
- I reinstalled with `build-and-install.sh` and relinked the Standalone explicitly.
- `--cpu25a` ran alone, after pluginval finished.

No automated result here is copied from SUMMARY-2.5a.md.

HANDS-ON-2.1 (13 rows), 2.2 (10), 2.3 (9), 2.4 (11) and 2.5a (12) are human gates. On 2026-11-08 the user deferred all of them until after Stage 3 (D14 override). They are **not** self-certified here.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.5a / ROADMAP Phase 2.5a)

1. **D27 parity arm.** Global LFO 1–4 (shapes, Free + Tempo, phase, S&H), macros, slot ramps and the route-change shadow crossfade null vs the parent @ `82406fce` at ≤ −90 dB, gain band held, every family has a firing control.
2. **D27 per-voice independence (DSP-06).** Velocity / key / note random / per-voice LFO each drive their own voice; a shared-source control collapses the spread.
3. **D4 newest-voice rule.** Handover step ≤ the 20 ms ramp bound; held when silent; never a stolen spare; un-ramped control fails.
4. **Mod Env + per-voice LFO.** Closed-form golden, 2 ms floors, latched stage times, retrigger from current; phase reset at note-on; allocation-order invariance.
5. **Ranges + clamp.** Neutral controllers add exactly 0; 16 stacked routes clamp without NaN.
6. **QUAL-01 route edits.** Every MX3 edit kind and the `lfoN_mode` switch vs same-motion controls.
7. **Tempo.** Onsets within 1 sample of the grid at 44.1 / 48 / 96 k across tempo changes and loop jumps; power-of-two thinning only; Free-mode control does not lock; note-on burst then grid (user PD1).
8. **Determinism + RT.** Block-size null with every source live (different-seed control); 0 allocations across route edits / mode switches / tempo changes / steals with 16 routes.
9. **True bypass.** Unrouted = renders-2.4 byte-identical (legacy == renders-2.2); param-dump identical; drift 0 / 13; engine never writes APVTS.
10. **D28 CPU.** C25w + 16 per-voice routes ≤ 40 %; typical proxy + 4 routes ≤ 12 %.
11. **Validators.** auval and pluginval 10 (VST3 + AU) pass; parent code stays out of our binaries.

### Deliverables (SUMMARY-2.5a.md)

- **C1 `8a41d10b`:** `ControlGrid.h` (ADAPTED, 89 mod / sync rows, `struct Lfo`, `using GridParams = GiGridParams`); ModSources / TempoTracker compiled VERBATIM; `GiModMatrix.h` (parent ModMatrix port, 126 × 18); `GiVoiceMod.h`; global sources, G-dest substitutes, per-voice pass, pair mixer mode, newest-voice rule, mode crossfade, telemetry atomics; `parity_gates_25a.h`.
- **C2 `98836951`:** `GiTempoClock.h` (parent `processTempo` port, R45, thinning on the division, burst then grid); `engine_gates_25a.h` (19), `engine_gates_25b.h` (33) + `--cpu25a`; `HANDS-ON-2.5a.md` (12 rows, deferred).

Code read in verify: `Engine.cpp:1363-1424` `updateNewest()` picks the max serial among **counted** voices only, snapshots the current blend into `monoFrom` on a handover and restarts one shared ramp from 0 → 1 (never stacked); with no counted voice it holds `monoVal`. Global sources are pinned to `srcG`. This is the structural basis of NV1–NV4.

### Goal Achievement

| Goal | Status | Evidence (verify re-run) |
|------|--------|--------------------------|
| 1. Parity arm | ✅ Achieved | Fresh parent build: `run.sh` rc 0, 0 self-check failures, 332 outputs. `--parity` **168 PASS / 0 FAIL**; `--rows GV` **2 / 0**.<br>• **PL0** −146.0 dB, g_PL 0.9977000 (no route, every source busy). **PL0t** per-tick s1_level trace max diff **0.000** over 13 891 ticks.<br>• **PL1–10, PL14, PL15** −143.8 … −146.2 dB; **PL11–13** (Tempo) −147.0 … −148.7 dB.<br>• **PM1–4** −145.9 dB; **PE1–6** −143.0 … −145.7 dB.<br>• Band \|g/g_PL − 1\| ≤ 8.8e-8 on every row (thr 1e-4).<br>• **PLs1** per-grain latched len == parent grains.tsv, max rel err 0 (10 grains); control lfo1_phase + 1° → 0.014. Audio null −25.0 dB INFO (user S1).<br>• Controls fire: PLc1 −33.6, PLc2 −5.5, PLc3 −0.0, PMc1 −16.9, PEc1 −17.3, PEc2 −16.3, PEc3 −42.8 dB.<br>• Carried unchanged: PN01–26 −143.0 … −151.3; PF −126.1 … −152.1 (49 rows); PB −116.3 … −152.0; PR −96.8 … −112.0 dB. 39 parity controls FIRED, 0 NOT FIRED. |
| 2. Per-voice independence | ✅ Achieved | **PV1:** 108 grains of 3 voices, len err 0.00 ulp, cutoff rel err 0, 3 distinct lens (pvSharedSource control → 1). **SD1** 17 / 17, **SD2** 8 / 8, **DR1** 125 / 125 consumers move, MX0 0 mismatches. |
| 3. Newest voice | ✅ Achieved | **NV1** 1.000× bound, no overshoot. **NV2** 0 ticks moved over 2 s (held 0.708661). **NV3** 8 handovers, newestWasStolen 0, 1.000×. **NV4** 1.000×; newestStack control → 233.6× with overshoot. |
| 4. Mod Env + PV LFO | ✅ Achieved | **ME1** max err 5.96e-9 vs closed form. **ME2** coefficients bitwise, attack reaches 1 at n 97. **ME3** 0 / 4000 differ (envLiveTimes → 4000). **ME4** retrigger 0.500522401 exact. **VL1** first tick −0.5 bitwise on both voices. **VL2** 0 differ (slotKeyedSeed → 71 998). **VL3** 0 / 13 172 over 64 serials with 59 steals. |
| 5. Ranges + clamp | ✅ Achieved | **NZ1** 16 routes ±100 from neutral controllers == unrouted bitwise (macro 0.1 % control → 95 differ). **CP1** +6 dB exactly (1.995262384), then 0, 0 non-finite. RG1 / MD1 pass. |
| 6. Route edits | ✅ Achieved | **RTa–g** 1.00× the parent MX3 bound on every kind × {global LFO, PV LFO, Mod Env}, 0 violations, 0 cross-talk; RTn controls 29.9×. **REa–g** 0.24 … 1.07× vs the same-motion 20 ms level ramp (thr 2.0×, user S6); REn 25.51×. **LM1** κ 1.36×, trajectory 0.594; LM1n 115.11×. |
| 7. Tempo | ✅ Achieved | **TS1** 465 onsets / 37 runs, max \|onset − grid\| 0.9925 smp, 0 missing (TS1n ceilPerTick → 2 dropped). **TS2** 112 onsets, 0 off-lattice, 0 non-power-of-two (TS2n → 224 off-lattice). **TS3** Free 0 / 32 locked. **TS4** 0 / 4500 across a re-lock. **TS5** R45 bitwise. **TS6** burst 10 000, grid 12 000 (tempoGridOnly control fires). |
| 8. Determinism + RT | ✅ Achieved | **BN1** 0 differing samples at 441 / 512 / 4096 / var vs 32 (BN1n seed + 1 → 287 984). **FA5** 0 allocations / 246 blocks (route add / remove / swap, mode switches, bpm step + loop, steals, CC storms, macros). |
| 9. Bypass / drift | ✅ Achieved | `--dump-render-hashes` **byte-identical** to `renders-2.4.sha256`; `--dump-render-hashes-legacy` **byte-identical** to `renders-2.2.sha256`, its 7 `-pinned` lines == `renders-2.1.sha256`. **TB3** 14/14 + legacy 14/14; **TB4** 14/14. param-dump `cmp`-identical. Drift **0 / 13**; clean pin-archive parent root → 0 drift; **NC-a** (byte on `Grain.h`, parent root) → 1 drift; **NC-b** (byte on `FeedbackChain.h`, local root) → 1 drift. **AP1** 0 APVTS writes in 37 dsp files (control: processor → 1). |
| 10. D28 CPU | ✅ Achieved | **C25m 26.81 %** ≤ 40; **C25tm 6.37 %** ≤ 12. See Automated Checks. |
| 11. Validators | ✅ Achieved | See Automated Checks. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.5a of 2.1–2.6). A requirement closes only when **all** of its acceptance criteria are covered.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-06: Mod matrix | must | ⚠️ Partial (was pending) | **Now covered:** "each listed source routes to a continuous destination with bipolar depth" for every 2.5a source (SD1 17/17, DR1 125/125, PL / PM / PE nulls); "per-voice sources modulate each voice independently" (PV1, VL1–3).<br>**Open:** the requirement lists **MPE** pressure / slide / bend; 2.5a ships their non-MPE form only — the per-member-channel form is 2.5b. Feel = HANDS-ON-2.5a. |
| PERF-02: CPU budget | must | ⚠️ Partial | **Now covered:** final worst case with the matrix, C25m 26.81 % ≤ 40 %.<br>**Open:** typical verdict on factory presets (Stage 4, D20). |
| PERF-01: RT safety | must | ⚠️ Partial | **Now covered:** FA5 (matrix, mode switches, tempo, steals with 16 routes).<br>**Open:** 2.5b voice modes / MPE, 2.6 file swap / Record-in. |
| PERF-03: Off-thread + reaper | must | ⚠️ Partial | BN1 block-size invariance with every source live. Open: 2.6 file swap / Record-in; HANDS-ON-2.1. |
| QUAL-01: No artifacts | must | ⚠️ Partial | **Now covered:** route edits (RT / RE), `lfoN_mode` switch (LM1), newest-voice handover (NV).<br>**Open:** file swap (2.6), HANDS-ON-2.1–2.5a. |
| DSP-01, DSP-04, DSP-05 | must / should | ✅ Complete (unchanged) | PN / PB / PR residuals identical to 2.4 verify. |
| FUNC-01–04, FUNC-09, DSP-02, DSP-03, DSP-07 | must | ⚠️ Partial | Unchanged; their HANDS-ON rows are pending. |

Still pending (later phases): FUNC-05 (2.5b), FUNC-06, FUNC-07 (2.6).

**Requirements Summary (after this phase):**
- ✅ Complete: 4 (DSP-01, DSP-04, DSP-05; COMPAT-01 from Stage 1)
- ⚠️ Partial: 13 (FUNC-01–04, FUNC-09, DSP-02, DSP-03, DSP-06, DSP-07, PERF-01–03, QUAL-01)
- ⏸️ Pending (later phase or stage): 7 (FUNC-05–07 stage 2; UI-01–02 stage 3; FUNC-08, COMPAT-02 stage 4)
- ❌ Failed: 0

Total 24, matching REQUIREMENTS.md.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (harness + param-dump) | ✅ Pass | rc 0 after the touch-rebuild; 0 warnings. |
| build-and-install.sh | ✅ Pass | rc 0, 0 warnings. Only `O-GranulatorInstrument-dev.{vst3,component}` on disk. |
| Standalone | ✅ Pass | Relinked (`OuariconGranulatorInstrument_Standalone`, rc 0); binary 12:58:20 newer than the harness 12:05:04. Alive after 6 s. |
| Render harness (default) | ✅ Pass | **231 PASS / 0 FAIL**. **115** `ctrl FIRED`, **0** `NOT FIRED`. Duplicate-ID set (B1 G1–G4 H1 H2 M1 R1 R1v S2) == the 2.4 set; no new 2.5a ID collides. |
| Parity (`--parity`, fresh parent build) | ✅ Pass | `run.sh` rc 0, 0 self-check failures. **168 / 0**; `--rows GV` **2 / 0** (GV0 +0.020 dB, GV1 error 0.000 dB). |
| Render goldens | ✅ Pass | Shipping == `renders-2.4.sha256`; legacy == `renders-2.2.sha256`; legacy pinned == `renders-2.1.sha256` 7/7. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv`. |
| Fork drift | ✅ Pass | 0 / 13. NC-a / NC-b re-made independently → 1 drift each. |
| X1 compdb exclusion | ✅ Pass | `ninja -t compdb`: 158 entries referencing O-GranulatorInstrument → **0** matches of `O-Granulator/Source\|ogi-parity\|82406fce`. Control: the parity compdb has 98 `O-Granulator/Source` refs. |
| X2 symbol exclusion | ✅ Pass | `nm -C` finds **0** `ogran::` in the installed VST3, the installed AU and the harness. Control: `proj-parity` has 76. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. `ZZZZ` control rc 2. |
| pluginval 10 VST3 (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, 2 ch in, 0 `!!!` lines. |
| pluginval 10 AU (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0, 2 ch in. Only `!!!` is the informational "Current program is -1". |
| `--cpu25a` (run alone) | ✅ Pass | **2 PASS / 0 FAIL**, rc 0. Witness `ref_ns` 40 523 708; calib-before 0.931–0.981 × ref (≤ 1.15), after/before ≤ 1.061 (≤ 1.10), duty ≥ 0.987.<br>• C25w (same session) 26.38 %.<br>• **C25m 26.81 %** (26.48 … 27.87) ≤ 40, live 243.2 — margin 13.2 points.<br>• **C25tm 6.37 %** ≤ 12, live 80.2.<br>• C25t 5.51 %; Δ C25tm − C25t +0.86.<br>• Δ C25m − C25w **+0.43 points** (SUMMARY +1.43; projection +2.5 … 5.5). |
| Git | ✅ Pass | No uncommitted plugin changes before verify. No `O-GranulatorInstrument*` tag. |

## Human Verification

All pending; deferred by the user on 2026-11-08 until the Stage 3 UI is in place, run together then:
- [ ] HANDS-ON-2.1 rows 1–13
- [ ] HANDS-ON-2.2 rows 1–10
- [ ] HANDS-ON-2.3 rows 1–9
- [ ] HANDS-ON-2.4 rows 1–11 (rows 3–4 carry CK2 / CK3)
- [ ] HANDS-ON-2.5a rows 1–12. Listening calls: LFO feel (row 1), per-voice spread (row 2), Tempo note-on burst (row 9), reverb cold restarts when an LFO sweeps Reverb Mix through 0 (row 11, parent R54).

## Issues Found

- **None blocking** in the automated scope.
- **CPU lower than SUMMARY.** C25m 26.81 % here vs 28.18 % in execute; the matrix delta is +0.43 points (execute +1.43). Both well under 40 %. The calibration brackets and duty are valid on both runs, so this is run-to-run spread, not a regression; the matrix cost is far below the research projection.
- **DSP-06 stays partial by scope.** The requirement text names MPE pressure / slide / bend; their per-member-channel form is 2.5b (D26). 2.5b must leave every 2.5a golden made with `mpe_enable` off unchanged.
- **Plan deviations reviewed and accepted** (SUMMARY-2.5a §Deviations 1–12): FL7 re-scope (S8, user), PLs1 latch metric (S1, user), RE same-motion reference (S6, user), C1 scope T3–T11, PL11–13 after T12, PL0t on the PL1 stimulus, tempo thinning tolerance 1e-6, TB3 / TB4 excluding Bend, C25tm depth 20 → 5 (the one allowed stimulus fix), ST1 → AP1 rename, `rh::Overrides.midi`, per-voice pitch_rand with a scale (approximation, documented). Each keeps a firing control; no threshold was loosened. X2 = 0 confirms test hooks are absent from shipping binaries.
- **CPU-run environment.** Logic was closed; only its QuickLook / Thumbnail extensions were resident (idle).
- **Carried, by design:** Mono / Legato / glide, sustain, MPE parser, note expression (2.5b); Record-in, `<Slots>`, factory sources, file swap (2.6); matrix view, mod rings, LFO telemetry UI (Stage 3); typical CPU on factory presets (Stage 4); `DevLoaderEditor` removed in 3.1.
- Foreign working-tree changes (`modules/registry.yaml`, `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, `.planning/STATE.md`, O-GrainScatter) are not touched.

## Phase Verdict

**Status:** ⚠️ PARTIAL. Every automated gate is independently verified green, including the final PERF-02 worst case with the matrix (C25m 26.81 % ≤ 40 %). DSP-06 moves pending → partial (MPE form in 2.5b; feel in HANDS-ON-2.5a). HANDS-ON-2.1–2.5a (human) are pending, deferred by the user until after Stage 3.

**Ready for next phase:** Yes, under the user's D14 override. Phase 2.5b (Mono / Legato / glide, sustain, MPE + note expression) follows. Any ❌ in the hands-on sheets after Stage 3 goes back to `/plugin-execute O-GranulatorInstrument 2-dsp` for the owning phase.

**Blockers:** none for 2.5b. Open human gates: HANDS-ON-2.1, 2.2, 2.3, 2.4, 2.5a (deferred).
