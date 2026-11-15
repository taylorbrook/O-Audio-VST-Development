# Stage 2: DSP — Phase 2.5 (output stage and auto gain) - Verification

## Verification Date

2026-11-14 / 15 (verify session). Everything below was run against `7ec0d2ca`.

- **Independent re-run.**
  - The three harness objects were deleted and recompiled: the designer `main.cpp`, the render `main.cpp` and the render harness's `PluginProcessor.cpp`.
  - Both harnesses ran in default and `--long` form, with all eleven negative controls.
  - The figures were compared with SUMMARY-2.5.
- **Code read.** A full read of `OutputStage.h` and `AutoGain.h`, and the processor diff `75566f48..7ec0d2ca` (`stageIdleForBlock`, `seedOutputStage`, steps 4 – 7, refresh schedule, peaks).
- **Independent probe.** Written in this session (`verify-2.5/probe.cpp`, 17 gated rows + INFO). It drives the real processor through its public API only. Its oracles are restated from the plan, not taken from `Source/dsp`:
  - the four-pole smoother, the tick and the refresh schedule (P2 / P5), replayed sample by sample;
  - the auto-gain truth, computed from the plugin's own measured impulse response on the 512-midpoint pink grid.
- **Mutation run.** 51 mutants of `OutputStage.h`, `AutoGain.h` and the processor's output-stage path, plus an unmodified control. Every mutant ran through both harnesses and the probe.
- **Validators.** pluginval strictness 10 (VST3, AU) and `auval` on the installed 0.5.0 bundles.

The probe, its build script, the mutation driver and their outputs are in `stages/2-dsp/verify-2.5/`. Nothing in the repo source or in `build/` was modified; mutants and the probe were compiled in the session scratchpad.

Scope: Phase 2.5 only. Stage 2 has one more phase (2.6, analyzer), so this file does not close the stage.

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 2.5 as amended 2026-11-14; PLAN-2.5)

1. Output gain is accurate (±0.05 dB, FUNC-05) and click-free under steps, reversals and sweeps (G3 gain rows).
2. Auto gain meets G12: the negative pink-weighted mean power gain of the target curve on 512 cell midpoints (C1), clamped to [−18, +12] dB; toggling it and sweeping a band under it are click-free (G3).
3. The estimate is a pure function of the band requests and refreshes on the tick clock: offline equals real time, bit for bit, at every block size.
4. G4 (a) holds with the stage in place: no band, output 0 dB, auto gain off (and on) is `memcmp` equal to the input.
5. Nothing fades in: the stage is seeded at the first block and at both un-bypass restarts; VERIFICATION-2.4 Issue 2 gets its row (XR-unb).
6. G6 with the stage and the estimator; a max-hold output meter; 0.5.0 builds, installs and validates; existing rows unchanged outside the named amendments.

### Deliverables (from SUMMARY-2.5.md, confirmed in the tree)

1. `Source/dsp/OutputStage.h` (four-pole `GainSmoother`, `OutputStage`) and `Source/dsp/AutoGain.h` (512-midpoint linear-power estimator). The diff of `Source/dsp` against `75566f48` is these two new files only.
2. `PluginProcessor.h/.cpp`: `stageIdleForBlock()` used at steps 5 and 6 (definition + two calls), `seedOutputStage()` at the step-4 and step-7 restarts, refresh on ticks whose index is a multiple of R, max-hold peaks read by `readAndResetOutputPeak`.
3. Designer harness rows XO-*, `--neg ag-alloc`; render harness rows XR-g12*, XR-gstep / gret / gsweep, XR-g4a(n), XR-seed, XR-unb(n), XR-blk5, XR-meter, A4, X-cpu4. Named amendments: P1, S5.
4. `CMakeLists.txt` 0.5.0, `CHANGELOG.md` `## [0.5.0] - 2026-11-14`, `-dev` bundles installed.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Output gain | ✅ Achieved | XR-g12o 10 / 10, worst 1.59e-7 dB (limit ±0.05). XO-step 1.557x (50 Hz) / κ 2.220x; XR-gstep 1.565x; XO-ret / XR-gret 1.579x / 1.582x; hard controls ≥ 10x on every reading. Probe T1: 440 / 440 output-gain step pairs bit-exact to the restated smoother and tick; T3: 12 random-automation renders (8017 events, random blocks, 6 rates, mono and stereo) bit-exact; T1n's three controls (one tick late, N + 1, applied at the edit sample) all differ. |
| 2 Auto gain, G12 | ✅ Achieved | XO-ag worst 0.0551 dB; G12 shelf −6.1665 dB at all rates; XO-agrip 0.028 dB (Q 40 +12). Probe E1: 19 configurations (all types, placements, mono / stereo, 22.05 – 192 kHz) equal the IR-measured truth within 0.001 dB, worst 0.00000 dB; the inclusive grid misses by up to 0.013 dB, so the row tells the two apart. E2: a Side band on stereo leaves auto on == off. E3: the clamps read exactly +12.00000 / −18.00000 dB. T2: the toggle lands on the refresh tick and follows the 100 ms smoother, 12 / 12, worst −144.5 dB re peak; T2n's controls (one tick late, R − 1) 24 / 24 differ. XR-gsweep auto gain under a bell sweep 1x, −140.5 dB from its reference. |
| 3 Pure function, offline = real time | ✅ Achieved | XO-agref 20 000 estimates, worst 0 dB. XR-blk5 24 / 24 bit-identical, with evaluation / refresh counts equal at every block size. XR-g12d two renders bit-identical. Probe B1: gain, auto and band events at blocks 1 – 2048 and offline at 44.1 / 48 / 96 kHz all bit-identical. |
| 4 G4 (a) | ✅ Achieved | XR-g4a / XR-g4an 30 / 30 `memcmp` equal, auto off and on, mono and stereo, denormal tail; control 21 / 21. Probe I2: 68 / 68 equal, incl. auto on with 24 bands at gain scale 0. I1: after auto gain returns to unity the output is bit-exact again from 191.8 – 192.4 ms at 44.1 / 48 / 96 kHz. See Issue 1 for the subnormal case while a toggle is pending. |
| 5 Seeding, Issue 2 row | ✅ Achieved | XR-seed, XR-unb (i) / (ii) 4 / 4 + 4 / 4, worst −146.7 dB; controls fire. Probe S1: a session opening at +12 dB output with auto gain on is constant from sample 0, 8 / 8. S2 / S2b: gain edited while bypassed, by both routes: blend error −150.5 dB, bit-exact from u + N. S3 / S3b: latched un-bypass mid-block, both routes: −150.5 dB; controls (fade one sample late, unseeded ramp) read −58.3 dB. |
| 6 G6, meter, build | ✅ Achieved | A4 0 allocations / 0 frees in 222 834 windows; XO-alloc 0 / 0; `--neg ag-alloc` fires. XR-meter 6 / 6; probe M1 (max-hold, reset, both routes, mono). `params.tsv` unchanged, `check_spec.py` OK. Installed bundles identical to the artefacts, version 0.5.0. pluginval 10 and `auval` pass. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.5 of 7)
**Requirements touched by this phase:** 4 (3 must, 1 should)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-05: output gain and gain scale | must | ✅ Complete | Output gain ±0.05 dB met (XR-g12o, worst 1.59e-7 dB) and click-free (G3 gain rows, probe T1 / T3). Gain scale was met at 2.4. |
| FUNC-06: auto gain | should | ✅ Complete | G12 met (XO-ag, XR-g12a, probe E1 – E3); toggle and sweep click-free (XR-gstep auto rows, XR-gsweep, probe T2); offline = real time (XR-blk5, probe B1). |
| QUAL-01: transparency | must | ⚠️ Partial | G4 (a) met with the stage (XR-g4a / g4an, probe I2). Listening across factory presets is Stage 4. |
| PERF-01: real-time safety | must | ⚠️ Partial | G6 met with the stage and estimator (A4, XO-alloc, `--neg ag-alloc`); read: no lock, no allocation, fixed storage. G6 re-runs with the analyzer on in 2.6. |

**Requirements Summary (all 27):**
- ✅ Complete: 11 (COMPAT-01, FUNC-03, FUNC-04, FUNC-05, FUNC-06, DSP-01, DSP-02, DSP-03, DSP-04, DSP-05, DSP-07)
- ⚠️ Partial: 5 (FUNC-01, FUNC-02, DSP-06, PERF-01, QUAL-01)
- ⏸️ Pending / later phase: 11
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | BRIEF, parameter-spec, ARCHITECTURE and ROADMAP equal STATUS |
| Commit hygiene | ✅ Pass | `7ec0d2ca` lists this plugin's paths and `PLUGINS.md` only; plugin tree clean; no tag |
| Build (three objects deleted, recompiled) | ✅ Pass | rc 0; no warning from this plugin's files (the SDK's libc++ `availability.h` notice only) |
| Designer harness, default and `--long` | ✅ Pass | `131 PASS, 0 FAIL, 17 INFO` both times; every 2.5 figure equals SUMMARY-2.5 |
| Render harness, default and `--long` | ✅ Pass | `122 PASS, 0 FAIL` both times. X-cpu4 default run 25.4 % / 26.8 % (summary 25.5 / 26.9); the `--long` run's CPU INFO is inflated by the concurrent mutation run and is not a figure |
| Negative controls | ✅ Pass | `rbj`, `f1`, `alloc`, `seed`, `rbj-b`, `binary-gate`, `first-build`, `engine-alloc`, `ramp-alloc`, `fade-alloc`, `ag-alloc` rc 0; an unknown name rc 255 |
| Static gates (PLAN-2.5 T8) | ✅ Pass | `Source/dsp` diff is `OutputStage.h` and `AutoGain.h` only; `log` / `exp` / `pow` only in `prepare` and the final dB → linear conversion; `stageIdleForBlock` defined once, called at steps 5 and 6 |
| Layout golden, spec | ✅ Pass | `--dump-params` equals `params.tsv`; `OK: 196 params` |
| Installed bundles | ✅ Pass | Exactly the two `-dev` bundles, 0.5.0, `diff -rq` identical to the build artefacts, newer than every source file |
| pluginval 10, `auval` | ✅ Pass | VST3 `SUCCESS`, AU `SUCCESS`; `AU VALIDATION SUCCEEDED`, `[1, 1] [2, 2]`; no NaN / inf in the logs |
| Independent probe | ✅ 17 / 17 | `verify-2.5/probe-output.txt` |

### Mutation run

51 mutants plus an unmodified control (which reproduces 131 / 0 / 17, 122 / 0 and 17 / 0). The designer harness does not compile `PluginProcessor.cpp`, so the processor mutants (PX) are gated by the render harness and the probe. Full table: `verify-2.5/results-all.txt`.

**Caught by the harnesses (43 of 51):**
- **Smoother / stage:** OS1 – OS6, OS9 – OS11, OS15, OS16, OS19 by both harnesses; OS12 (non-finite reads +36 dB) and OS14 by the designer (XO-unity) and OS13 by the render harness only.
- **Estimator:** AG2, AG4 – AG6, AG19 by both; AG1, AG7 – AG10, AG12, AG13, AG15, AG16, AG18 by the designer alone (unit rows).
- **Processor:** PX1 – PX3, PX6 – PX10, PX13, PX14, PX16, PX18, PX20.

**Thinly gated (one row in one harness):** OS13 (XR-g12o / XR-idle), OS14 (XO-unity, XR-g4an), PX7 (XR-unb), PX13 / PX14 (XR-meter), PX16 / PX20 / PX9 / PX10 (XR-ref), AG8 / AG18 (XO-agz), AG9 / AG10 (XO-agid), AG15 / AG16 (XO-agref).

**Survive both harnesses (8):**

| Mutant | Kind | Probe |
|---|---|---|
| **PX17** step-7 restart on an empty band list jumps without seeding (the 2.4 form): a latched un-bypass with no band and a non-unity stage fades in the gain | **real** | **caught (S3, S3b)** |
| **PX11** pending output target applied at the block start instead of the next tick | **real** (block-size dependent) | **caught (T1, T1n, T3)** |
| **PX4** idle predicate ignores `agOn != agWanted`: an auto-gain toggle in an idle block waits for a non-idle block | **real** | **caught (T2, B1)** |
| **AG3** grid top 0.45 fs → 0.5 fs: changes the estimate only where 0.45 fs < 16 kHz (22.05 / 32 kHz) | **real** (low rates) | **caught (E1 at 22.05 kHz)** |
| **OS18** snap tests only the last stage: snaps early by up to the other stages' residue | real, small | caught (T3) |
| AG11 `isDirty` ignores an unevaluated band | equivalent: after the first estimate every band is evaluated | — |
| AG14 the +inf guard reads 0 dB | unreachable: the S5 stack guard (< +600 dB) keeps the mean finite | — |
| PX5 idle predicate ignores dirty bands | redundant by read: a dirty band is either listed (not idle) or has left the stage away from unity until a refresh | not caught |

The shipped code takes the correct branch in every surviving case (read: step 7 calls `seedOutputStage()` before the jump test; `applyOutputPending` runs on the tick; the predicate tests `agOn != agWanted`; `fTop = min (16 kHz, 0.45 fs)`; the snap tests all four stages).

## Human Verification

Not blocking. Carried to the hands-on listen:

- [ ] Auto gain on while enabling a +12 dB shelf: the level should settle over about 100 ms with no pump.
- [ ] A narrow +12 dB bell swept with auto gain on: no audible wobble (bench ≤ 0.03 dB).
- [ ] Output-gain automation, fast throws: no zipper, no click.
- [ ] The VERIFICATION-2.4 items: steep Low Cut under bass, the stop-band burst (2.4 Issue 1), 20 ms feel, several bands changing at once; the 2.3 drag items.

Expected in this build and not defects: no analyzer (2.6); the meter has no UI yet (Stage 3).

## Issues Found

None is a defect in the shipped code against its specification.

1. **Subnormal input is flushed while an auto-gain toggle is pending with no contributing band (INFO, probe I3).** With no band contributing, the auto target is exactly 1.0, but `agOn != agWanted` makes the block non-idle until the next refresh tick, so the lanes are converted and FTZ flushes subnormal samples: 2048 samples differ in a window of up to one refresh period (about 21 ms). G4 (a) is a static condition and is unaffected (XR-g4a, probe I2). Inaudible (float subnormals are below −750 dBFS). No change recommended; noted so a later "bit-exact while toggling" row does not trip on it.
2. **Gate gaps for the 2.6 plan.** Each of these is held by the probe only:
   - PX17: add a render row for a latched un-bypass with no band and a non-unity stage (output gain ≠ 0 dB).
   - PX11: output-gain edits off the tick boundary at varying block sizes (XR-blk5's events fall on multiples of 2048, so they are always on a tick).
   - PX4: an auto-gain toggle made while the block is otherwise idle (empty band list, stage at unity); the probe's T2 is the template.
   - AG3: one G12 row below 35.6 kHz (22.05 or 32 kHz), where the 0.45 fs top is active.
   - OS18: the bit-exact smoother replay (probe T3) is the only gate of the snap rule; an XO row comparing against a restated four-stage cascade would hold it.
3. **VERIFICATION-2.4 Issue 1 (stop-band burst) is still open**, waiting on the listen.
4. **Deviations 3 – 5 rechecked.** Deviation 3 (XR-gstep κ window) was Taylor's call; deviation 4 (the non-contributing edit is not dirty) is correct by read and by AG9 / AG10 being caught; deviation 5 (Side excluded on stereo) is output-equivalent, confirmed by probe E2 and AG8 being caught.
5. **CPU** (INFO; G10 binds in Stage 4). 24 × 96 dB/oct shelves with the gain swept every block: auto off 25.4 %, on 26.8 % (+1.4 %).

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.5)

**Ready for next phase:** Yes. Phase 2.6 (analyzer) has no research or plan yet; next is research for 2.6.

**Blockers (if any):** none. Carried forward, non-blocking:
- Issue 2, five gate gaps held by the probe only, for the 2.6 plan.
- VERIFICATION-2.4 Issue 1, the stop-band burst: hands-on item.
- The hands-on listen.
