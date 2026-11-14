---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.4 Summary"
date: 2026-11-14
phase: "2.4"
status: executed
version: "0.4.0"
designer_harness: "114 PASS, 0 FAIL, 16 INFO (default and --long)"
render_harness: "100 PASS, 0 FAIL (default and --long)"
---

# O-ParametricEQ — Stage 2 / Phase 2.4 Summary (structural crossfades and bypass)

## Outcome

**Complete. Designer harness 114 PASS / 0 FAIL / 16 INFO; render harness 100 PASS / 0 FAIL; both also with `--long`.**

- **Crossfades.** Type, slope, placement, enable and band bypass changes crossfade over 20 ms (raised cosine, incoming slot from rest). A request that arrives during a fade is latched.
- **Sleep.** A gain-bearing band held at exactly 0 dB sleeps after 50 ms.
- **Global bypass.** Bypass is a 20 ms fade to the dry input through one path for the parameter and for `processBlockBypassed`. While fully bypassed the engine is idle. Un-bypass restarts every band settled on its current parameters.
- **Build and validators.** Version 0.4.0 is built and installed. pluginval strictness 10 passes (VST3 ×2, AU), `auval` passes, and the Standalone stays alive.

Three decisions were taken in the session, two of them Taylor's (TK, XF-1p), and three harness defects were fixed. The engine and the processor were not changed after their first build. See "Deviations".

No tag.

## Tasks

| Task | Result |
|---|---|
| T1 pre-flight | Contract checksums equal; branch `main`; plugin tree clean. Gate 1→2 PASSED (`--skip-review`, as in 2.1–2.3). Baselines 88 / 0 / 14 and 83 / 0; `check_spec.py` OK. |
| T2 / T3 engine | New `Source/dsp/Fade.h` (`FadeTable`, `BypassFade`, constants) and `Source/dsp/SlotBand.h`. No other `Source/dsp` file was touched (P1). |
| T4 designer rows | 26 rows and 2 INFO lines under "2.4 / structural crossfades and bypass (double)"; `--neg fade-alloc`. |
| T5 processor | `std::array<eq::SlotBand, 24>`; one `processAudio (buffer, viaBypassed)` for both entry points; P8 steps 1–9; lane union per P7. |
| T6 version | `OPEQ_VERSION 0.4.0`; `CHANGELOG.md` has `## [0.4.0] - 2026-11-14`. |
| T7 render rows | 19 rows and X-cpu3 (INFO). P18 amendments: H1 (ii), M3, U2, A1, S5; M6n retired. TK amended (Taylor). |
| T8 gates | Build rc 0, no warning in this plugin's files. Both harnesses 0 FAIL, default and `--long`. Ten `--neg` controls rc 0. `check_spec.py` OK; `params.tsv` unchanged. Static greps clean; `Source/dsp` diff is the two new headers only. `dynOffsetDb`: the declaration and its reads only. No complementary-form mix. |
| T9 install + validators | Install rc 0; exactly the two `-dev` bundles, both 0.4.0. pluginval 10: VST3 `SUCCESS` ×2, AU `SUCCESS`. `auval -v aufx OuPQ OuDv`: `AU VALIDATION SUCCEEDED`, capabilities `[1, 1] [2, 2]`. Standalone rebuilt, `ALIVE`. |
| T10 | This file, STATUS, PLUGINS.md, one path-scoped commit. |

## What was built

- **`Fade.h`**
  - Constants and lengths: `fadeSeconds` 0.020, `fadePrerollSeconds` 0, `sleepSeconds` 0.050; `fadeLengthSamples` (clamped to 7680), `sleepLengthTicks`.
  - `FadeTable`: N mixing weights `0.5 − 0.5 cos (π (n + 1) / (N + 1))`; pre-roll as a count of zeros.
  - `BypassFade`: four states, never reverses, a latched opposite fade at the end, `jump` for no running band.
- **`SlotBand.h`**
  - Two unmodified `eq::Band` slots, each with a placement and an off flag.
  - `startSettled`, `request`, `controlTick` (with the sleep test), `process` (64-sample pieces, mix `a + w (b − a)`, a fade end continues inside the call), `goIdle`, `clearState`.
  - `touchesLane` per P7; read-only access and counters for the harness.
  - The latch is derived (`fading && in ≠ desired`), not stored.
- **Processor**
  - P6 single entry; P8 per-block order.
  - The running list is fixed at the block start; it is rebuilt after a latched un-bypass inside the block (step 7), with a `jassert` that the restarted bands' lanes were converted.
  - Idle reached mid-block: the rest of that chunk is written back as converted, and later chunks are not touched.

## Results

### G3 step rows (XF-g3 / XR-g3; P13)

| Reading | Double (XF), 968 / 484 readings | Float (XR), 1028 / 514 readings | Bench |
|---|---|---|---|
| (a) max second difference / louder held, limit 2.0x | 50 Hz **1.525x** (Low Cut 48 ↔ 96, block 512); 1 kHz 1.021x | **1.522x** | 1.53 / 1.02 |
| (b) 1 kHz local κ / held, limit 2.5x | **2.20x** (Notch ↔ Tilt, block 32) | **2.197x** | 2.20 |
| 50 Hz κ, printed | worst 20.49x; 202 of 484 above 2.0x | 20.47x; 212 of 514 (band bypass included) | 20.5x; 202 |
| (c) hard control ≥ 10x at block 512 | 482 of 482 fire; lowest 273.6x at 50 Hz, 17.9x at 1 kHz | 511 of 511; weakest 17.92x | 274 / 17.9 |
| Void (printed, not counted) | Bell 10 Hz Q 40 +12 at 1 kHz: enable, global bypass (1.00x) | the same, plus band bypass | 1.00x |

- **XR-g3b**, the Global bypass group through `processBlockBypassed`: (a) worst 1.334x; (b) worst 1.648x.
- **R2** (XF-g3k margin): the worst reading is 2.20x, the same as the bench, so no reading falls in 2.2x–2.5x.

### Fade reference, rest, latch

- **XF-ref**: 60 of 60 readings within 1e-9 of the input peak; worst 1.78e-15.
- **XF-refn** fires:
  - Linear weights: 60 of 60, least 0.0248.
  - Incoming chain one sample late: 54 of 54, least 4.2e-6.
  - The 6 disable readings are void for the late start, since an off incoming slot has no chain to start late.
- **XR-ref**: worst −145.4 dB re the output peak (limit −120).
- **XF-rest / XR-rest**: bit-identical from the fade's end, 23 040 samples; during the fade the output equals neither neighbour (0 of 960 samples).
  - XF-rest's input is zero after the burst, so the incoming slot outputs zeros there; the row's live content is that the ring does not leak into the incoming slot.
- **XF-lat**: 20 readings, worst 1.170x; all count checks hold. XF-cont, XF-lane (0 uncovered lanes; side −5988 dB) and XF-idle are exact.
- **XF-byp**: worst 1.245x; case (a) mixes 2 × 960 samples, with one fade to idle and one latched restart. XF-bypn reads 48.4x and 39.1x (bench 48.5x).

### Transparency, sleep, wake (G4 (d), (e), (f))

| Row | Reading | Bench / limit |
|---|---|---|
| XF-ident | 100 000 of 100 000 identity designs at 5 rates | 100 000 |
| XF-g4 (d0) | equal from the first sample | — |
| XF-g4 (d1) 6 → 0 dB | 85.83 – 88.18 ms | 85.7 – 88.3; ≤ 100 |
| XF-g4 (e) gain scale 0 % | 86.94 – 88.92 ms | 87.0 – 89.1; ≤ 100 |
| XF-g4 (f) bypass | exactly N (20.00 ms) | 19.95 – 20.00 |
| XF-g4n (never sleeps) | 60 of 60 differ | — |
| XR-g4d / XR-g4e (float) | equal from **19.98 ms** (20.08 ms on the random block sequence) | ≤ 200 ms |
| XR-g4f | equal from exactly N after the block that sees it, both routes | — |

The float rows equal the input as soon as the 20 ms ramp lands on 0 dB, before the band sleeps. The 0 dB design is the identity to about 1e-16, which is below float resolution. The sleep itself is gated in double (XF-g4 at 86–89 ms, XF-g4n).

- **XF-slp**: range 0.10x – 2.10x (bench 0.10 – 2.10; limit 3.0x); a sleep and a wake in every render.
- **XF-slpc**: no sleep on a ramp through 0, at 0.0001 dB, or on a Low Cut; the vacuity case sleeps once.
- **XF-wake**: exact. The fade starts at position 0; the slot is the 0 dB design; the gain arrives at tick 60; at tick 61 the slot equals the static +12 dB design. N and the sleep ticks match the plan at 8 rates.

### Live slopes (G2)

- **XF-g2**: 336 cases. Corner worst 0.0194 dB. Slope measured on 308 cases (worst 0.545 dB/oct) and by G2s-short on 21 (worst 0.641). 7 cases were unmeasurable, with a span under one octave. The sections equal the static design in all cases.
- **XR-g2**: 16 of 16. Corner worst 0.00122 dB; slope worst 0.375 dB/oct; 1 cell by G2s-short.

### Changed while bypassed, block identity, re-prepare

- **XR-byp**: 4 of 4 variants are `memcmp`-equal to a fresh instance from un-bypass + N, by both routes. The variants are a structural edit, a continuous edit only (SUMMARY-2.3 deviation 4), and bypass engaged mid-ramp (VERIFICATION-2.3 Issue 3). **XR-bypn** fires on 4 of 4.
- **XF-blk**: bit-identical at blocks 1 / 32 / 64 / 512 / 2048 at four rates, with counts equal across block sizes.
  - Fades 274 / 189 / 240 / 259; wakes 11 / 10 / 9 / 7.
  - Latched requests 0 / 0 / 0 / 180: only 192 kHz can latch with this script (see deviation 4).
- **XR-blk**: 20 / 20. **XR-big** and **XR-prep**: bit-identical.

### VERIFICATION-2.3 issues carried

| Issue | Row | Reading |
|---|---|---|
| 1 Mid / Side ramping path | XF-ms | 16 of 16 within 1e-12 (worst 2.7e-16) |
| 2 one-pole on the ramping path | XF-1p | κ 1.00x (limit 4.0x); distance worst 2.59e-4 (limit 1e-3); Gc-held control 116x / 39x (deviation 2) |
| 3 bypass mid-move; oversize block; re-prepare | XR-byp, XR-big, XR-prep | exact |
| 4 state rule of a structural change | XF-rest, XR-rest | exact |
| 5 G5 peak clause | XF-g5, S5 | see below |

### G5 with structure and bypass (XF-g5, S5; P14)

- **XF-g5**, double, 60 s and `--long` 600 s per rate plus a 10 s every-block run:
  - 0 non-finite samples.
  - 0 of 6.5 million checks (default run) above stack + 12 dB; worst peak − stack −8.4 dB.
  - All bands off: equal after at most 39.99 ms (bench 40).
- **S5**, float, `--long`:
  - 600 s per rate, 0 non-finite; peaks +196 to +253 dB re the input.
  - Largest stack 519.9 – 554.8 dB (guard +600); seed attempt 1 at 176.4 and 192 kHz.
  - All bands off: equal after 10.7 – 48.0 ms.
  - Default run: 16 of 16; largest stack 584.9 dB; one seed change.
- **XF-nan**: finite from the next chunk; the fade ends on schedule (sample 7104 with and without the NaN).

### Allocation (G6)

- **XF-alloc**: 0 / 0 over 1e4 blocks (19 200 fades, 100 fades to idle, 100 restarts).
- **A3**: 0 / 0 in 222 834 windows (56 043 through `processBlockBypassed`). A3n, A1 (amended) and `--neg fade-alloc` fire or pass as expected.

### CPU (INFO; G10 binds in Stage 4), 48 kHz stereo, block 512, % of one core

| Case | XF-cpu (double) settled / always fading / 1 per s / asleep | X-cpu3 (plugin) | Bench |
|---|---|---|---|
| 24 cuts × 96 dB/oct | 2.87 / 5.62 / 2.92 / — | 3.06 / 5.82 / 3.01 / — | 2.65 / 5.17 / 2.71 |
| 24 shelves × 96 dB/oct | 6.15 / 11.66 / 5.88 / 0.002 | 5.79 / 11.2 / 5.8 / 0.001 | 4.93 / 9.88 / 5.06 / 0.04 |
| 24 bells ↔ 12 dB/oct shelves | 0.73 / 1.86 / 0.83 / 0.002 | 0.90 / 1.85 / 0.94 / 0.001 | 0.67 / 1.74 / 0.87 / 0.04 |
| 24 shelves 96 ↔ 72 | 5.63 / 9.58 / 4.80 / 0.002 | 5.77 / 9.44 / 4.79 / 0.001 | 4.97 / 8.39 / 4.26 / 0.04 |

X-cpu2's settled 24-cut reading moved 2.74 → 3.08 %. That is the per-band slot bookkeeping, and it is INFO.

## Existing rows (P18)

**Designer harness:** all 88 baseline PASS rows and 14 INFO rows print as in the baseline. The only differences are timing figures: the seed time INFO, the RM-g7 block-time ratio and RM-cpu.

**Render harness:** 82 of the 83 baseline rows are present (M6n retired). Rows not named in P18 print as before, except:

- S2's `pluginVersion` 0.3.0 → 0.4.0.
- The timing figures of N7 and X-cpu / X-cpu2.
- TK, below.

| Row | As amended | Reading |
|---|---|---|
| H1 (ii) | from the toggle + N; from un-bypass + N equal to a fresh instance started at the un-bypass sample; both routes | 7 / 7 |
| M3 | on: input from the toggle + N; off: from the toggle + N equal to a fresh instance started at the toggle | 6 / 6 |
| U2 | disabled for 2 blocks (≥ N); from the re-enable + N equal to a fresh instance | PASS |
| A1 | the bypassed block checked after the fade; the fade blocks stay inside the window | 217 windows (214 before) |
| M6n | retired; XR-rest and XR-g3h replace it | — |
| S5 | P14 draw plus the every-block run | 16 / 16 |
| **TK** (Taylor) | the on-tick case expects 1026 | 9 / 9 |

**Existing rows with structural edits mid-render, not amended:**

- **A2**: allocation count only; unchanged.
- **U3**: inert edits compare identical and start no fade; its control, a slope edit on a Low Cut, now fades and still differs. PASS as before.
- **M2**: its 0 dB bell now sleeps and reads exactly 0 dB, as the plan noted.

## Deviations

1. **TK amended (Taylor's call in the session).**
   - TK's bell starts at exactly 0 dB, so under 2.4 it sleeps, and the +24 dB edit wakes it through a fade.
   - The fade starts at 1024 (gated exactly in double by XF-wake). The first two weights times one ramp step fall below float resolution, so the float output first differs at 1026.
   - TK's on-tick case now expects 1026. A separate `kTickExpectedPlugin` carries it, because `kTickCases` also feeds M1's reference, which never sleeps.
2. **XF-1p gated in the sample domain (Taylor's call).**
   - On G3 motion 5, local κ cannot see a held one-pole coefficient. The Gc-held mutant reads 3.8x (6 dB/oct) and 5.0x (18 dB/oct), and 2.3's `holdRender` reads the same. That is under the plan's 10x control and, at 6 dB/oct, under the 4.0x verdict.
   - The κ clause stays as planned. Added: max |engine − refRender| ≤ 1e-3 on the 0.5-peak tone at all 8 readings (worst 2.59e-4), with the Gc-held render's distance ≥ 10x the engine's as the control (116x, 39x).
   - XF-1p was rerun on the default run only. The `--long` run read the same row before the clause, and the row has no length dependence.
3. **S5's stack in the render harness follows the fade schedule.**
   - The first form took the larger of the two latest requests on every band at every block. No seed then stayed under the +600 dB guard at 176.4 kHz in 32 attempts; 2.3's own `--long` already sat at 589.2 dB.
   - The new form applies "the larger of two slots" to **fading** bands only, per RESEARCH-2.4 C10, which P14 adopts. A structural change starts an N-sample fade whose outgoing slot is the request before it. A change during a fade is latched and starts one more fade at that fade's end.
   - An intermediate form (hold the largest pre-change peak for 2 N) over-counted at blocks of 1 and 7 and was replaced.
   - Seeds now pass at attempt 0 or 1.
   - XF-g5 in the designer harness holds the request from before the latest structural change for 2 N. Its stack only loosens the peak clause, and XF-g5 has no overflow guard.
4. **XF-blk's latched count is gated on the row total.** With events on multiples of 2048, a band can latch only where the fade is longer than 2048 samples: 192 kHz (3840). At 44.1 / 48 / 96 kHz (882 / 960 / 1920) no request can meet a fade. The bench's figures are row totals. Fades and wakes stay gated per rate.
5. **XF-lat (ii) boundary fixed.** When the first fade ends on a block end, `SlotBand::process` starts the latched fade at the end of that call, at position 0. The check required `blockEnd > event + N` and missed that case; it is now `>=`.
6. **Plan ambiguities resolved by the row writers (stated in the row text):**
   - XF-lat (iii) is unscriptable at block 512, since only one block start falls inside a 960-sample fade. It is VOID there and gated at block 32.
   - The step rows run `gain_scale` 200 % with the raw gain halved, because some bench settings need Tilt ±36 dB.
   - XR-g4n (e) uses `gain_scale` to leave the 12 dB bands at 0.001 dB.
   - XR-blk draws the `gain_scale` events with probability 0.3.
   - XR-ref's 30 pairs were chosen by the writer around 1 kHz.
   - XF-1p's control is the Gc-held render, which is VERIFICATION-2.3's SE2 mutant, not a variant of `holdRender`.
7. **Run note.** The first harness-writing agents stopped on an account spend limit. The work was resumed from their partial edits, which were kept and checked: the T7 header, A1, H1 (ii), U2.

## Carry-forward

- **2.5**: `output_gain` and `auto_gain` are still read by nothing. The bypass fade stays the last stage before the float conversion; 2.5 inserts its stages in front of it.
- **2.6**: G6 with the analyzer on; the listen crossfade.
- **Verify**:
  - Hands-on listening:
    - A 48 dB/oct or steeper Low Cut switched in under sustained bass (U2, no pre-roll).
    - Whether 20 ms feels immediate on enable and bypass.
    - Several bands changing type at once, as in a preset change.
  - The `--long` runs once more.
  - A mutation run over `SlotBand.h`, `Fade.h` and the bypass path.
  - The XF-1p distance clause's margin, 3.9x below its limit.
  - Whether the float G4 rows should also see the sleep. They pass on the ramp alone; XF-g4 is the only sleep-timing gate.
- **Requirements**: no status edited; they close at verify.
