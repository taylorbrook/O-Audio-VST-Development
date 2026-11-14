# Stage 2: DSP — Phase 2.2 (static engine, placement, transparency) - Verification

## Verification Date

2026-11-13 (verify session). What was run, against `d34505d2`:

- An independent re-run: the three harness objects (designer `main.cpp`, render `main.cpp`, the render harness's `PluginProcessor.cpp`) were deleted and recompiled, both harnesses and all eight negative controls were run and compared with SUMMARY-2.2.
- A full read of the four new headers (`BandParams.h`, `SectionEngine.h`, `Band.h`, `Router.h`) and of `PluginProcessor.h/.cpp`.
- A 35-mutant run of the engine headers and the processor, through both harnesses.
- Two probes written in this session, neither using the harness code:
  - `probe.cpp` drives the real processor and scores it against analog facts (19 rows).
  - `g1render.cpp` renders the double engine over the G1 grid and scores it against the true analog prototype.
- pluginval strictness 10 (VST3, AU) and `auval` on the installed 0.2.0 bundles.

The probes, the mutation driver and their logs are kept in `stages/2-dsp/verify-2.2/`.

Scope: Phase 2.2 only. Stage 2 has four more phases (2.3 – 2.6), so this file does not close the stage.

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 2.2 as amended; PLAN-2.2)

1. 24 bands enabled together, each changing the output independently (FUNC-01).
2. Each of the 8 types renders the designer's closed form: 0.01 dB in float, 1e-6 dB in the double engine (FUNC-02).
3. G4 (a): no band enabled is bit-identical, denormal samples included (QUAL-01).
4. G4 (b), (c): Left / Right leave the other channel bit-identical; Mid / Side leave the other component (FUNC-03). Mono: every placement processes the channel and placement is inert.
5. G4 (g): block-size invariance.
6. G8 as amended: latency 0, nothing before the impulse, predicted first-sample gain (DSP-02).
7. G6 with static parameters: no allocation in `processBlock` (PERF-01).
8. Frequencies above Nyquist at 44.1 kHz render finite and follow the frequency policy (DSP-06).
9. `Band::dynOffsetDb` is added to the effective gain and is 0 in v1 (DSP-07, engine half).
10. One raw → settings path, sanitised.
11. The 2.2 state rules: hard bypass, re-enable from rest, inert edits.
12. Stage 1 layout, state and bus rows unchanged; 0.2.0 builds, installs and validates.

### Deliverables (from SUMMARY-2.2.md, confirmed in the tree)

1. `Source/dsp/BandParams.h` (280 lines), `SectionEngine.h` (174), `Band.h` (226), `Router.h` (221): std-only, `noexcept`, no allocation.
2. `PluginProcessor.h/.cpp`: engine wired, two-level change detection, touched-lane conversion, chunks of the prepared block size, hard bypass, tail 2.0.
3. Designer harness: 13 engine rows and `--neg engine-alloc`. Render harness: 28 rows; L4 and P1 amended by name.
4. `CMakeLists.txt` at 0.2.0, `CHANGELOG.md` entry, dev bundles installed.
5. ARCHITECTURE G8 amended during execute (Taylor's call).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 24 independent bands | ✅ Achieved | E1 (i) 1.33e-7 dB, (ii) 24 / 24; E1n fires. Probe: each band alone as a bell of b/2 dB reads b/2 dB, 24 / 24, worst error 7e-6 dB. |
| 2 Eight types render the closed form | ✅ Achieved | EN-ir worst 2.69e-10 dB over 40 renders; E2 worst 1.35e-3 dB (High Cut 96 dB/oct, 44.1 kHz). Probe, against analog facts instead of the closed form: bell peak 12.0000 dB; shelf plateaus 8.9999 / −11.9985 dB; Low Cut and High Cut at all 8 slopes are −3.01 dB at the corner and Butterworth one octave away (worst error 0.005 / 0.020 dB); notch −112 dB; band pass 0 dB with the analog skirt; tilt −3 / 0 / +3 dB. |
| 3 G4 (a) | ✅ Achieved | T1a 8 / 8 mono and stereo, denormal tail asserted; P1 as amended. |
| 4 Placement | ✅ Achieved | EN-lr, T1b 15 / 15 (T1bn fires); EN-ms −264.9 dB, T1c −146.5 dB (controls fire); T1m 5 / 5, U3. Probe: a Mid bell gives +12 dB on an L = R input and leaves an L = −R input at 0.000000000 dB; Side the converse; a Left bell leaves R at exactly 0 dB. |
| 5 Block-size invariance | ✅ Achieved | EN-blk 0 of 8, T1g 6 / 6, K1. |
| 6 G8 | ✅ Achieved | EN-g8 0 late starts, worst 2.5e-14 of the chain's largest magnitude; Z1 126 / 126; L4 latency 0. Mutants of the first-sample formula, the latency value and the Mid algebra are all caught. |
| 7 No allocation (static) | ✅ Achieved | A1 0 / 0 in 214 windows with a live probe, A1n fires; EN-alloc 0 / 0; `--neg engine-alloc` rc 0. Read: no lock, listener or container growth in the audio path. |
| 8 Above Nyquist | ✅ Achieved | N1 32 / 32 (worst 7.5e-4 dB), N2 finite, EN-fin 0 non-finite over 15.9 M samples and 80 s of re-drawn settings. |
| 9 `dynOffsetDb` hook | ✅ Achieved | EN-dyn bit-equal, V1 48 / 48; grep: one declaration, two reads, no write. |
| 10 One raw → settings path | ✅ Achieved | SN1 89 checks, SN2 2 000 000 gains bit-equal, SN3. `makeBandInput`: one definition, one call in the processor, both harnesses call it. |
| 11 2.2 state rules | ✅ Achieved | H1 (i) 4 / 4, (ii) 7 / 7; U1, U2, U3 6 / 6. See Issues 1 and 2 for two rules no row asserts. |
| 12 Stage 1 rows, 0.2.0 validates | ✅ Achieved | The 36 Stage 1 lines pass (L4, P1 by their amended names); `params.tsv` unchanged; `check_spec.py` OK. Installed binaries equal the build artefacts (SHA-1) and are newer than every source file; pluginval 10 `SUCCESS` on VST3 and AU; `auval` succeeded with `[1, 1] [2, 2]`. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.2 of 7)
**Requirements touched by this phase:** 12 (11 must, 1 should)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: 24 bands | must | ⚠️ Partial | Criterion 1 met (E1, probe). Click-free enable is 2.4; preset recall is Stage 4. |
| FUNC-02: eight types | must | ⚠️ Partial | Criterion 1 met (EN-ir, E2, probe). Click-free type change is 2.4. |
| FUNC-03: placement | must | ✅ Complete | All three criteria met (T1b, EN-ms / T1c, T1m / U3, probe). |
| FUNC-04: bypass | must | ⚠️ Partial | Global bypass nulls (H1 (i)); per-band bypass is bit-exact (probe). Click-free toggling is 2.4. |
| FUNC-05: output gain, gain scale | must | ⚠️ Partial | Gain scale met: 0 % flat, 200 % doubles (SN1, probe: 24.0000 / 6.0000 / 0.0000000 dB). Output gain is 2.5. |
| DSP-01: analog-matched response | must | ✅ Complete | Criterion 1: design side at 2.1b; rendered, 2800 bells at 44.1 kHz stay within 47 % of the G1 tolerance against the true analog prototype and equal the closed form within 8e-11 dB (`g1render`). Criterion 2: no oversampling, latency 0. |
| DSP-02: zero latency, minimum phase | must | ✅ Complete | Latency 0 at every rate tested; every sample before the impulse is exactly 0 (Z1, EN-g8). |
| DSP-03: slopes | must | ⚠️ Partial | Criterion 1 met: design side G2s (2.1); rendered through the plugin, both cuts at all 8 slopes match Butterworth one octave into the stop band (probe). Click-free slope change is 2.4. |
| DSP-06: ranges | must | ⚠️ Partial | Above-Nyquist rendering met (N1, N2, EN-fin). Reachability from UI and typed entry is Stage 3. |
| DSP-07: room for dynamic EQ | should | ✅ Complete | Engine half met (EN-dyn, V1); the state-format half closed at Stage 1. |
| PERF-01: real-time safety | must | ⚠️ Partial | No locks; zero allocations with static parameters and after each kind of change (A1). "Under parameter stress" is G6 in 2.3. |
| QUAL-01: transparency | must | ⚠️ Partial | Criterion 1 met (T1a). Listening across factory presets is Stage 4. |

COMPAT-01 (complete since Stage 1) is confirmed again on 0.2.0.

**Requirements Summary (all 27):**
- ✅ Complete: 5 (COMPAT-01, FUNC-03, DSP-01, DSP-02, DSP-07)
- ⚠️ Partial: 8
- ⏸️ Pending / later phase: 14
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | brief, parameter-spec, ARCHITECTURE and ROADMAP equal STATUS |
| Commit hygiene | ✅ Pass | `d34505d2` lists this plugin's paths and `PLUGINS.md`; plugin tree clean; no tag on the commit |
| Build (three objects deleted, recompiled) | ✅ Pass | rc 0. The only warning is the SDK's libc++ `availability.h` notice, not from this plugin. |
| Designer harness | ✅ Pass | `67 PASS, 0 FAIL, 12 INFO` in 36 s. Every engine figure equals SUMMARY-2.2. |
| Render harness | ✅ Pass | `64 PASS, 0 FAIL` in 5 s. Every figure equals SUMMARY-2.2 except X-cpu (timing): 2.28 % and 4.47 % against 2.44 % and 4.94 %. |
| Negative controls | ✅ Pass | `rbj`, `f1`, `alloc`, `seed`, `rbj-b`, `binary-gate`, `first-build`, `engine-alloc` rc 0; an unknown name rc 255. In-harness: EN-irn, EN-msn, E1n, E2n, T1bn, T1cn, A1n, P1n fire. |
| Static gates | ✅ Pass | `Source/dsp` token grep, CMake flag grep and processor token grep print nothing; `dynOffsetDb` and `makeBandInput` as stated in goals 9 and 10. |
| Layout golden, spec | ✅ Pass | `--dump-params` equals `params.tsv`; `OK: 196 params` |
| Installed bundles | ✅ Pass | Exactly the two `-dev` bundles, version 0.2.0, binaries identical to the build artefacts |
| pluginval 10, `auval` | ✅ Pass | VST3 `SUCCESS`, AU `SUCCESS`, `AU VALIDATION SUCCEEDED` |
| Independent processor probe | ✅ Pass | 19 / 19 (`verify-2.2/probe.log`) |
| Rendered G1 | ✅ Pass | 35 / 35 rows (`verify-2.2/g1render.log`) |

### Rendered G1 (new in verify)

VERIFICATION-2.1b left "the rendered half of G1" to this phase, and PLAN-2.2 has no row for it. `g1render.cpp` renders the impulse response of `eq::Band` in double at 44.1 kHz and takes a 64-point DFT from 20 Hz to 20 kHz, on renders whose last 200 ms is at least 300 dB below the peak.

| Set | Rendered vs true analog prototype | Rendered vs closed form |
|---|---|---|
| Bells, 40 centres × ±3 / 6 / 12 / 18 / 30 dB × Q 0.025 – 40 (2800) | worst 0.019 / 0.043 / 0.077 / 0.109 / 0.193 dB against 0.086 / 0.122 / 0.194 / 0.266 / 0.410 | ≤ 7.9e-11 dB |
| Low Shelf, High Shelf, Tilt, 8 slopes, Q 1, ±6 / 12 / 30 dB (5760) | printed, equal to the design's own error to three decimals | ≤ 1.2e-7 dB |

The first two runs of this probe misread narrow low bells by up to 1e-4 dB. That was the probe truncating the render (a 200 dB tail still sums coherently at the resonance), not the engine.

### Mutation run (new in verify)

35 mutated copies of `Source/` were compiled against the unmodified harness sources in the session scratchpad; nothing in the repo or in `build/` was changed. The unmodified control gives 67 / 0 / 12 and 64 / 0.

| Mutation | Designer rows failing | Render rows failing |
|---|---|---|
| E1 SVF high-pass term drops `k` | EN-ir, EN-g8 | E1, E2 ×4, N1, N2, Z1, U1 |
| E2 `ic2` update wrong | EN-ir | E1, E2 ×4, N1, U1 |
| E3 one-pole state update wrong | EN-ir | E1, E2 ×2, N1 |
| E4 `a3` wrong | EN-ir | E1, E2 ×4, N1, U1 |
| E5 first-sample formula drops `mB` | EN-g8 | Z1 |
| B1 one-pole half never runs | EN-ir, EN-g8 | E1, E2 ×4, N1, Z1, U1, U3 |
| B2 `reset` leaves lane 1 `ic1` | EN-g8 | H1 (ii), U2 |
| B4 both lanes stored to state lane 0 | EN-blk | E1, E2 ×2, T1g |
| R1 Mid writes `R − d` | EN-ms, EN-g8 | T1c, Z1 |
| R6 Side writes `R + d` | EN-ms, EN-g8 | T1c, Z1 |
| R8 Mid uses `L + R` (router only) | EN-ms, EN-g8 | Z1 |
| R4 `touchesLane` Left includes lane 1 | EN-lr | T1b |
| P1 slope not canonical on a Bell | SN1, SN3 | U3 |
| P7 placement not mapped on mono | none | U3 |
| P8 Q not canonical at 6 dB/oct | SN3 | none |
| P2 effective gain clamp ±30 | SN1, EN-dyn | none |
| P3 band bypass ignored | SN1 | **none** |
| P4 toggle on above 0.5 only | SN1 | none |
| P6 choices truncate | SN1 | none |
| P9 gain scale ignored | EN-g8, SN1, EN-dyn | **none** |
| R7 Mid uses `L + R` (router and prediction) | EN-ms | **none** |
| X1 `bypass` does not clear state | – | H1 (ii) |
| X2 `processBlockBypassed` does not clear state | – | H1 (ii) |
| X4 tail 0 | – | L4 |
| X5 structural change does not clear state | – | U2 |
| X6 untouched lane converted too | – | T1b |
| X11 latency 1 | – | L4, Z1 |
| X12 designer prepared at a fixed 48 kHz | – | E2 ×4, N1, Z1 |
| X17 continuous change not designed | – | U1 |
| **B3 `setDesign` clears the state** | **none** | **none** |
| **P5 a type change classified continuous** | **none** | **none** |
| **X7 `gain_scale` missing from the change snapshot** | – | **none** |
| **X8 `prepareToPlay` does not invalidate the snapshots** | – | **none** |
| X9 `prepareToPlay` does not clear state | – | none (equivalent: the first block after a prepare is structural and clears it) |
| X18 no-active-band early return removed | – | none (equivalent: no lane is touched, so nothing is converted) |

29 of 33 live mutants are caught. Four survive both harnesses (Issues 1 – 3). Three more are caught only by the designer harness, so the plugin path for them rests on the read and on the probe (Issue 4). The probe catches X7 and X8 (run against both mutants: 18 / 19 each).

## Human Verification

Not blocking. This is the first build with an audible engine, so the Stage 1 smoke items now carry weight:

- [ ] Logic (AU): O-ParametricEQ-dev on a mono and a stereo track; enable a few bands in the generic editor and hear them act; with every band off, audio passes unchanged.
- [ ] One VST3 host: the same; the automation list shows the four globals first, then `B1 On` … `B24 Placement`.
- [ ] Save, close and reopen a session with bands set: the sound returns.

Expected in this build and not defects: parameter moves step, type / slope / placement / enable changes click, global bypass is a hard switch.

Execute skipped its code-review checkpoint (`--skip-review`). This session covers it for the engine and the processor: the four headers and `PluginProcessor.h/.cpp` were read in full, with no defect found. The two harnesses (6978 and 3241 lines) were not read; the mutation run, the controls and the two probes corroborate their scoring.

## Issues Found

1. **"State kept on a continuous change" has no row.** With `setDesign` clearing the state (B3), both harnesses pass: U1 reads the steady state a second later, and EN-fin only counts the changes. The same hole lets a type change classified as continuous (P5) through, since nothing distinguishes a kept state from a cleared one. Not a defect in the shipped code (read: `setDesign` keeps the state, `classifyBandChange` lists type). Phase 2.3 replaces this path with ramps; its plan should add a row that fails when a continuous change resets the filter (for example: a gain edit on a ringing low bell, output at the edit sample continuous with the block before).
2. **A `gain_scale` edit is not gated through the plugin.** With `gain_scale` left out of the change snapshot (X7) the render harness passes: A1 edits it but only counts allocations. The probe catches it (24 / 24 / 24 dB instead of 24 / 6 / 0). The same applies to band bypass (P3) and to the gain-scale arithmetic (P9), which only SN1 and EN-dyn see. Add to the render harness in 2.3 (where gain scale gets its ramp) or 2.5 (FUNC-05): a `gain_scale` edit mid-render and a `bNN_bypass` row.
3. **Re-preparing an instance at another rate is not gated.** With the snapshot invalidation removed from `prepareToPlay` (X8) the render harness passes, because every row uses a fresh instance per rate. The probe's re-prepare row catches it. A host that changes the sample rate without re-instantiating depends on this line. Add a row: prepare 44.1 kHz, render, prepare 96 kHz, compare with a fresh instance.
4. **The response of a Mid or Side band is not measured through the plugin.** With `m = L + R` in both the router and its first-sample prediction (R7), the render harness passes: T1c only requires the band's own component to change by more than −40 dB, and E1 / E2 use Stereo. EN-ms catches it in the designer harness, and the probe measures +12.0000 dB. A one-line addition to T1c (the Mid component's gain at the bell centre equals the bell gain) would close it.
5. **`BandParams.h` comment, lowest Q** (already in SUMMARY carry-forward). The header says the float nearest 0.025 lies just below it; it lies just above (0.025000000372…), so the clamp does not act and the lowest raw Q reads that value. Behaviour is correct and SN1 tests it; the comment is wrong.
6. **Deviation 4 of the SUMMARY stands:** a gain or gain-scale edit on a cut, Notch or Band Pass redesigns the band (identical design, state kept, no audible effect). ARCHITECTURE "Parameter Interactions" asks for no design work there. With `gain_scale` global, one move of it redesigns every active cut. Canonicalising `gainDbEff` to 0 for those types in `makeBandInput` is one line; decide in the 2.3 plan, where the ramp cost makes it matter.
7. **For Phase 2.3**, unchanged from the SUMMARY: G5's peak bound against stacked gain (EN-fin reaches +272 dB in double; float overflows); `dynOffsetDb` is read only when a raw value changes, so a future writer must add it to the snapshot; the X-cpu shelf figure is the settled path only.

None of these blocks the phase.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.2)

**Ready for next phase:** Yes. Phase 2.3 (continuous modulation) has no research or plan yet; next is research for 2.3.

**Blockers (if any):** none. Carried forward, non-blocking:
- harness rows for a kept state, a `gain_scale` / band-bypass edit through the plugin, a re-prepare at another rate, and the Mid / Side response (Issues 1 – 4);
- the gain canonical form on cuts (Issue 6) and the G5 peak bound (Issue 7), both for the 2.3 plan;
- the hands-on DAW listen.
