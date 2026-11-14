---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.2 Summary"
date: 2026-11-13
phase: "2.2"
status: executed
version: "0.2.0"
designer_harness: "67 PASS, 0 FAIL, 12 INFO"
render_harness: "64 PASS, 0 FAIL"
---

# O-ParametricEQ — Stage 2 / Phase 2.2 Summary (static engine, placement, transparency)

## Outcome

**Complete. Designer harness 67 PASS / 0 FAIL / 12 INFO (39 s); render harness 64 PASS / 0 FAIL (6 s).** Audio now passes through up to 24 bands with fixed parameters. Version 0.2.0 is built and installed (VST3 + AU dev bundles); pluginval strictness 10 passes (VST3 twice, AU once), `auval` passes, the Standalone stays alive.

One thing did not go as planned and was decided by Taylor during the run: designer row **EN-g8 failed as specified** (deviation 1). The engine was not changed; ARCHITECTURE gate G8's double-engine clause and the row were amended.

No tag.

## Tasks

| Task | Result |
|---|---|
| Gate 1 → 2 | PASSED (build, pluginval; `--skip-review`, non-interactive session). |
| T1 pre-flight | Checksums equal; `main`; plugin tree clean; baseline 54 / 0 / 12 and 36 / 0; `check_spec.py` OK. |
| T2 / T3 engine headers | `Source/dsp/BandParams.h`, `SectionEngine.h`, `Band.h`, `Router.h` (std-only, `noexcept`, no allocation). |
| T4 designer rows | 13 rows under "2.2 / engine (double)" + `--neg engine-alloc`. The 54 + 12 earlier lines print as in the baseline (one timing figure differs). |
| T5 processor | Engine wired: change detection in two levels, lane conversion only where an active band touches, chunks of the prepared block size, hard bypass, tail 2.0 s. |
| T6 version | `OPEQ_VERSION 0.2.0`; `CHANGELOG.md` `## [0.2.0] - 2026-11-13`. |
| T7 render rows | 28 rows added; L4 and P1 amended by name. |
| T8 gates | Build rc 0, no warning in this plugin's files; both harnesses 0 FAIL; eight `--neg` controls rc 0; `check_spec.py` OK; `params.tsv` unchanged; static greps clean. |
| T9 install + validators | Install rc 0, exactly the two `-dev` bundles. pluginval 10: VST3 `SUCCESS` ×2, AU `SUCCESS`. `auval -v aufx OuPQ OuDv`: `AU VALIDATION SUCCEEDED`, capabilities `[1, 1] [2, 2]`. Standalone `ALIVE`. |
| T10 | This file, STATUS, PLUGINS.md, one path-scoped commit. |

## Results

### Engine in double (designer harness)

EN-ir, worst error in dB against `eq::bandMagDb` above -80 dB (limit 1e-6); every render 1 s, tail at least 886 dB below the peak:

| Type | 44.1 k | 48 k | 88.2 k | 96 k | 192 k |
|---|---|---|---|---|---|
| Bell 1 kHz +12 dB Q 1 | 5.5e-14 | 4.1e-14 | 6.4e-14 | 6.3e-14 | 1.3e-13 |
| Low Shelf 200 Hz +9 dB 24 dB/oct | 1.0e-13 | 1.4e-13 | 1.6e-13 | 1.7e-13 | 3.1e-13 |
| High Shelf 8 kHz −12 dB 12 dB/oct | 1.1e-14 | 8.0e-15 | 9.2e-15 | 8.5e-15 | 1.4e-14 |
| Low Cut 80 Hz 48 dB/oct | 5.2e-11 | 5.7e-11 | 9.5e-11 | 1.6e-10 | 2.7e-10 |
| High Cut 6 kHz 96 dB/oct | 2.8e-12 | 1.2e-12 | 2.0e-12 | 4.1e-12 | 2.5e-12 |
| Notch 1 kHz Q 10 | 1.5e-12 | 5.2e-13 | 2.4e-13 | 7.9e-13 | 1.0e-12 |
| Band Pass 500 Hz Q 2 | 5.0e-14 | 6.0e-14 | 1.1e-13 | 2.1e-13 | 1.7e-13 |
| Tilt 1 kHz +6 dB 12 dB/oct | 2.0e-14 | 2.9e-14 | 3.6e-14 | 3.2e-14 | 5.6e-14 |

- **EN-irn:** the 1 % detuned bell misses by 0.157 dB at every rate.
- **EN-ms:** untouched component worst **-264.9 dB** re input peak (24 bells +12 dB; limit -250); 1 bell ±30 dB -313 / -325 dB. **EN-msn:** on Left the "untouched" component moves by -7 dB or more.
- **EN-lr:** other lane `memcmp` equal, touched lane differs in 65 536 of 65 536 samples.
- **EN-g8 (as amended):** 10 000 configurations, 124 906 bands. 0 start late with a non-zero prediction; worst error **2.5e-14** of the chain's largest magnitude (limit 1e-12). Forced High Cuts start at index 0 with y[0] 2.0e-47, 1.2e-57, 1.2e-41 (relative error below 1e-15). Not gated, printed: 46 configurations with y[0] and its prediction both exactly 0; 116 above 1e-12 relative to the prediction.
- **EN-blk:** 0 of 8 block sizes differ, both settings.
- **EN-fin:** (i) 3894 settings, 15.9 million samples; (ii) 8 rates × 10 s with a re-draw every 64 samples: 0 non-finite. Peaks reach +272 dB re the noise (double only; P12).
- **EN-alloc:** 0 allocations, 0 frees over 1e4 blocks; hook live.
- **SN1** 89 checks; **SN2** 2 000 000 gains bit-equal; **SN3** 26 000 inert edits identical, 4000 controls fire; **EN-dyn** bit-equal.

### Plugin in float (render harness)

- **E1:** (i) 1.33e-7 dB; (ii) 24 of 24, worst 3.27e-7 dB. **E1n:** smallest miss 3 dB.
- **E2:** worst error in dB (limit 0.01); mono and stereo give the same figures:

  | Type | 44.1 k | 96 k |
  |---|---|---|
  | Bell | 3.2e-7 | 1.4e-7 |
  | Low Shelf | 1.1e-7 | 3.5e-7 |
  | High Shelf | 2.9e-7 | 3.9e-7 |
  | Low Cut 48 dB/oct | 2.4e-4 | 5.2e-5 |
  | High Cut 96 dB/oct | **1.35e-3** | 1.7e-4 |
  | Notch | 3.1e-7 | 3.4e-7 |
  | Band Pass | 2.4e-6 | 1.5e-6 |
  | Tilt | 5.7e-7 | 5.9e-7 |

  The bench predicted 5.8e-4 dB for the High Cut; the plugin reads 1.35e-3 dB, still 7× inside the limit.
- **N1:** 32 of 32, worst 7.5e-4 dB. **N2:** finite; peak -58.5 / -39.2 dB re input.
- **T1a** 8 / 8 mono and stereo; **T1b** 15 / 15; **T1bn** 14 / 14 fire.
- **T1c:** untouched component -146.5 dB (24 Mid bells), -149.5 dB (1 Mid bell +30 dB), -146.6 and -147.8 dB on Side, re the larger peak (limit -130). **T1cn:** on Left -6 to -7 dB.
- **T1g** 6 / 6; **T1m** 5 / 5; **K1**, **H1** (i) 4 / 4, (ii) 7 / 7; **U1** -9 dB after, +6 dB before; **U2**; **U3** 6 / 6; **V1** 48 / 48.
- **Z1:** 126 of 126; 168 gains compared. Forced High Cuts, impulse at 37, first non-zero float sample: 10 Hz at 44.1 kHz index 52 (stereo R 53); 10 Hz at 192 kHz index 131 (R 136); 100 Hz at 192 kHz index 40 (R 41). This is the float underflow G8 was amended for at plan time.
- **A1:** 0 allocations, 0 frees in 214 windows; probe 1 / 1. **A1n:** 1 / 1 counted.
- **X-cpu (INFO):** 0 bands 0.0009 %; 24 cuts × 96 dB/oct **2.44 %** (bench 2.4, budget 10); 24 shelves × 96 dB/oct **4.94 %** (bench 4.7, budget 14); 24 bells 0.68 % (bench 0.70); 1 band 0.045 %.

### Controls

`--neg` rbj, f1, alloc, seed, rbj-b, binary-gate, first-build, engine-alloc: rc 0 each. In-harness: EN-irn, EN-msn, E1n, E2n, T1bn, T1cn, A1n, P1n all fire.

### Static gates

`Source/dsp` token grep, CMake flag grep and processor token grep: no lines. `dynOffsetDb`: one declaration (`Band.h`), two reads (`PluginProcessor.cpp`), the `makeBandInput` argument; no write. `makeBandInput`: one definition, one call in the processor, both harnesses call it.

## Amended Stage 1 rows

- **L4** "tail 0" → "tail 2.0".
- **P1 `processBlock`** (mono and stereo lines): "…x Off/On/all random, every bNN_enabled off, bypass off".
- **S2** is not amended but prints `pluginVersion 0.2.0` in its detail (the version bump). The remaining 32 lines are identical to the baseline.

## Deviations

1. **EN-g8 and G8 amended (Taylor's call, 2026-11-13).** As planned (first non-zero at the impulse index in every configuration; y[0] within 1e-12 relative) the row failed: 46 of 10 000 started one sample late and 116 missed 1e-12 relative. Cause: rounding in the contracted algebra. Mid / Side difference injection `L += F(m) − m` cancels when a band's first-sample gain is tiny (a Mid High Cut at 1e-18 on L = R gives exactly 0, and the prediction gives 0 too), and the SVF high-pass term `x − k v1 − v2` cancels at high Q (without any Mid / Side band: 5 of 26 000, worst 3.4e-12, measured in a scratch run). Against the chain's largest magnitude the worst error was 9.8e-15. Amended: start at the impulse index wherever the predicted y[0] is non-zero; error ≤ 1e-12 of the chain's largest magnitude; forced single-band cases keep 1e-12 relative. ARCHITECTURE G8 edited, checksum refreshed; PLAN-2.2 row annotated.
2. **`-Wfloat-equal`** on three comparisons in `sameBandInput`: replaced by `sameValue (a, b)` (`! (a < b) && ! (a > b)`), same result for finite values.
3. **Inactive bands are not designed** on a continuous change (P3 says "design"). Activation is structural and designs from the current input, so the output is the same.
4. **Gain is not canonicalised on cuts, Notch, Band Pass.** P2 step 5 lists slope and Q only, so a gain or scale edit on a cut redesigns (identical design, state kept, no audible effect). ARCHITECTURE "Parameter Interactions" asks for no design work there. One line if wanted.
5. **Three Stage 1 lines changed name, not two** (P1 prints a mono and a stereo line), plus S2's detail.
6. **Row choices where the table was silent:** EN-g8 runs one configuration in four on mono and the impulse on both lanes or the left alone; EN-fin (ii) keeps type / slope / placement on half the re-draws so both the cleared and kept state paths run; N1 runs both gain signs; Z1 draws frequency and Q over the full range.
7. **Agents were interrupted once** by a usage limit and resumed; no file was lost.

## Carry-forward

- **Verify 2.2:** hands-on DAW listen (the Stage 1 smoke items are still open). The human code review was skipped at the gate. `BandParams.h` comment on the lowest raw Q is wrong (`0.025f` is just above 0.025, not below; SN1 tests the real behaviour). N2's peak is 39–58 dB below the input: the +40 dB bound is far from binding. EN-g8's scaled metric says nothing about a tiny first sample except in the three forced cases.
- **2.3:** G5's "peak ≤ +72 dB re input" against stacked gain (P12; EN-fin reaches +272 dB in double); parameter moves step; ramp-path shelf cost not timed; `dynOffsetDb` is not in the change snapshot (a writer must add it).
- **2.4:** G4 (d), (e), (f); structural changes click; hard bypass.
- **2.5:** P1's random draw includes `output_gain` and `auto_gain`; both are read by nothing.

## Files

New: `Source/dsp/BandParams.h`, `SectionEngine.h`, `Band.h`, `Router.h`; this summary.
Changed: `Source/PluginProcessor.h/.cpp`, `CMakeLists.txt`, `CHANGELOG.md`, `tests/designer-harness/main.cpp`, `tests/render-harness/main.cpp`, `.planning/research/ARCHITECTURE.md` (G8), `.planning/stages/2-dsp/PLAN-2.2.md` (EN-g8 row), `.planning/STATUS.md`, `.planning/stages/1-foundation/gate-report.json`; `PLUGINS.md` (0.2.0).
