---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.3 Execution Plan (continuous modulation)"
created: 2026-11-13
juce_version: "8.0.15"
phase: "2.3"
summary: "Six waves, serial: (W0) pre-flight and baselines; (W1) the control ramp, the per-band control tick and the ramping path as std-only code in Source/dsp, with their double-precision rows in the designer harness; (W2) the processor on the global control grid, version 0.3.0; (W3) render-harness rows (sweeps, stress, allocation under stress, noise floor, block identity, the four gaps carried from verify 2.2); (W4) build, both harnesses, controls, the long G5 run, install, pluginval 10, auval; (W5) summary, STATUS, one path-scoped commit. No second slot, no crossfade, no sleep, no soft bypass, no output stage. No tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 2 / Phase 2.3 Execution Plan (continuous modulation)

## Goal

Frequency, gain, Q and gain scale move without artifacts. A continuous edit ramps for 20 ms on a global control grid, the section parameters are interpolated per sample between grid points, and the filter state is kept. A settled band is bit-identical to a static design of the same settings and does no design work. Version 0.3.0 is built, installed and validated.

Proved at two levels, as in 2.2: the engine in double by the designer harness, the plugin's float output by the render harness.

**In scope:** `eq::ControlRamp`, the control grid, `Band`'s control tick and ramping path, the processor's tick clock and segment loop, the canonical gain on gainless types, the non-finite input check, the `dynOffsetDb` read at each tick. Gates G3 (sweep rows), G5, G6 under stress, G7, block-size identity with moving parameters. Four harness gaps from VERIFICATION-2.2 (issues 1 – 4).

**Out of scope (and what 2.3 does instead):**

| Later phase | 2.3 behaviour |
|---|---|
| 2.4 crossfades | A structural change (type, slope, placement, activation) designs from the target at the block start and clears the band's state, as in 2.2. Click. |
| 2.4 neutral sleep | A 0 dB band is an ordinary active band. G4 (d), (e) are not claimed. |
| 2.4 soft bypass | Global `bypass` stays a hard switch. G4 (f) is not claimed. |
| 2.4 | G3 step rows; G5 with structural fades. |
| 2.5 output stage | `output_gain` and `auto_gain` are read by nothing. |
| 2.6 | No taps, no display model, no listen. G6 "with the analyzer on" re-runs there. |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/RESEARCH-2.3.md` | All of it. Q6 is the recipe; Q7 the row list; "Pitfalls carried". |
| `stages/2-dsp/research-2.3/ramp_bench.cpp` | Working prototype of the grid, the ramp, the tick and the ramping path, and of every measurement mode (`g3`, `g3var`, `zero`, `lyap`, `g5`, `g7`, `blk`, `cpu`, `keep`). Port from here. |
| `stages/2-dsp/VERIFICATION-2.2.md` | "Issues Found" 1 – 7 and the mutation table: the gaps this plan closes. |
| `stages/2-dsp/CONTEXT.md` | "Constraints carried". Binding. |
| `research/ARCHITECTURE.md` | "Parameter smoothing and the control grid"; "Parameter Interactions"; gates G3, G5, G6, G7 (as amended by this plan phase); "Denormal Protection". |
| `Source/dsp/Band.h`, `SectionEngine.h`, `BandParams.h`, `Router.h`, `Designer.h`, `EqTypes.h` | The 2.2 engine this phase extends. |
| `Source/PluginProcessor.h/.cpp` | `processBlock` as 2.2 left it (snapshot compare, `classifyBandChange`, chunk loop). |
| `tests/designer-harness/main.cpp`, `tests/render-harness/main.cpp` | Row and tally conventions; EN-alloc / A1 (allocation window), `render` with its per-block callback, U1 / U2 / H1 (state rules), T1c. |
| Memory `index_dsp_gate_design.md`, `index_testing_render_harness_patterns.md`, `pattern_click_kappa_masked_by_level_change.md`, `pattern_run_flag_past_dirty_check_latches.md`, `pattern_gate_stimulus_below_threshold_is_vacuous.md` | Suite gate recipes this phase leans on. |

---

## Plan-phase decisions

User calls taken in this phase (2026-11-13), contracts amended and checksums refreshed:

- **U1 G3 sweep rows** as research C1: local κ ≤ 4.0x a same-motion per-sample reference; interpolation-off and no-smoothing controls ≥ 10x; held-reference readings printed with a WARN ceiling. K stays 16 / 32 / 64.
- **U2 G5 peak** as research C2: double engine, peak ≤ largest static stack drawn so far + 12 dB; plugin finite on a seeded draw. A non-finite input does not stick.
- **U3 G7 silence** as research C3: 3.42 dB/s ± 2 % from the 10th to the 60th second, monotone, ≤ -270 dBFS in the 60th second.
- **U4 Version 0.3.0**, CHANGELOG entry, dev build installed, pluginval 10 and `auval`.

The plan's calls:

- **P1 Files.** One new std-only header, three extended. Same rules as before (C++17, `noexcept`, no allocation, no framework, the gate tokens absent even from comments).

  | File | Change |
  |---|---|
  | `ControlRamp.h` (new) | `controlGridSize (fs)`: 16 for fs ≤ 48 kHz, 32 for ≤ 96 kHz, 64 above. `rampLengthTicks (fs)`: `round (0.020 fs / K)`, at least 1 (28, 40, 55, 60, 55, 60, 55, 60 at the eight rates). `eq::ControlRamp`: three values (ln f, effective gain dB, ln Q) on one counter. |
  | `SectionEngine.h` | The ramping tick: per sample and per section, add the increments to `g, k, mL, mB, mH, G, cL, cH`, then `a1 = 1 / (1 + g (g + k))`, `a2 = g a1`, `a3 = g a2`, `Gc = G / (1 + G)`, then the same two recurrences as the settled path. The parameter is advanced before use. |
  | `Band.h` | `setTarget`, `snapTo`, `controlTick`, `isMoving`; the lane processing picks the settled or the ramping path. `setDesign` and `reset` keep their 2.2 meaning, so the 2.2 rows that call them are untouched. |
  | `BandParams.h` | Canonical gain (P6); one helper for the offset sum (P5); the comment on the lowest raw Q corrected (VERIFICATION-2.2 issue 5). |

- **P2 `ControlRamp`, exactly (C5).**
  - `snap (settings)`: current = target = (ln f, gain, ln Q) of the settings; counter 0. This is the only way a ramp gets its first value; `prepareToPlay` calls it through `Band::snapTo` for every band.
  - `retarget (settings)`: per-tick increments `(target − current) / N`, counter `N = rampLengthTicks`. A new target while running restarts from the current value over the full length.
  - `advance()`: counter > 1 → add the increments, decrement; counter == 1 → counter 0 and the value **is** the target (assigned, not summed).
  - Arrival is the counter reaching 0, never a value comparison.
  - While running, the design settings are the target's settings with `freqHz = exp (ln f)`, `gainDb`, `q = exp (ln Q)`, each clamped to its range (an `exp (log x)` round trip can leave the range by one ulp). On arrival the design is made from **the target's own `BandSettings`**, not from the ramp values: the settled design is then bit-identical to a static one.
  - No `juce::SmoothedValue`, no framework type.
- **P3 `Band` control tick, exactly (Q6).** Called once per grid tick for each active band, with the designer:
  1. If the last interval interpolated: every section parameter is set to that interval's end design exactly (the stored tick design, not the accumulated sum), and the increments are zeroed.
  2. If a target is pending (`setTarget` since the last tick): `ramp.retarget`.
  3. `moving` is derived here and nowhere else: the ramp counter is non-zero, or `dynOffsetDb` differs from the value read at the last tick. It is never cached past a change check.
  4. If moving: `ramp.advance()`; build the settings (P2, P5); `designer.design`; store the design as the interval's end; increments `(end − current) / K` per parameter; the next K samples run the ramping path.
  5. If not moving and step 1 ran: `toRuntime` once from the arrived design; the settled path runs from this sample on.
  A band that neither interpolated nor moves does nothing in the tick: no design, no conversion.
  The section count and each section's `hasSvf` / `hasOnePole` flags cannot change inside a continuous move (measured: 0 in every bench run). The tick asserts it in debug and, in release, treats a mismatch as `snapTo` (design from the ramp's current settings, no interpolation for that interval; state kept).
- **P4 Coefficients advance once per sample, not once per lane.** A Stereo band's two lanes run on the same interpolated coefficients (a lane-pair loop as in the bench, or a per-interval coefficient table of at most 64 × `maxSections` entries on the stack; the executor's choice). Row RM-st fails a build that advances per lane.
- **P5 `dynOffsetDb` (C6).** The ramp carries `clamp (gain · scale / 100, −60, +60)`: no separate `gain_scale` ramp. The offset is added **after** the ramp at each tick: `gDesign = clamp (rampGain + offset, −60, +60)`, through one helper in `BandParams.h` that `makeBandInput` step 3 also uses; with the offset exactly 0 the sum is skipped. The processor passes `makeBandInput (…, 0.0)` targets to the band and the band reads its own `dynOffsetDb` at the tick. `makeBandInput`'s signature and its results for every existing SN / EN-dyn check are unchanged. The offset is ignored for gainless types (P6).
- **P6 Canonical gain (C7; VERIFICATION-2.2 issue 6).** `makeBandInput` step 5 also sets `gainDbEff = 0` for Low Cut, High Cut, Notch and Band Pass. A gain or `gain_scale` edit on those types is then "identical": no ramp, no design. If an existing SN1 or EN-dyn check asserts a non-zero `gainDbEff` on one of those four types, that check is amended to the canonical value, by name, and listed in the summary; no other existing row is edited.
- **P7 The tick clock (C8).** One `uint64` sample counter in the processor, zeroed in `prepareToPlay`, advanced by `buffer.getNumSamples()` on **every** call of `processBlock` and `processBlockBypassed`, every early return included (hard bypass, no active band, the channel-count guard). Ticks fall where `counter % K == 0`. K and the ramp length are set in `prepareToPlay` from the rate.
- **P8 Per block, in this order.**
  1. Snapshot compare and `classifyBandChange` as in 2.2, at the block start.
     - structural → `band.snapTo (target, designer)` (design from the target, ramps snapped, interpolation cancelled, settled coefficients) and `band.reset()`.
     - continuous → `band.setTarget (target)`. Nothing is designed here; the target takes effect at the next tick (at once when the block starts on a tick).
     - a band leaving the active list → state cleared, ramps snapped.
  2. No active band → advance the clock, return (buffer untouched).
  3. Chunks of at most the prepared block size (2.2 P6); inside a chunk, **segments that end at the next tick**. At a segment that starts on a tick: `controlTick` for each active band in index order. Then each active band processes the segment through the Router, in index order.
  4. End of each chunk: the non-finite check (P9).
  Any path that clears a band's state also snaps its ramp: activation, structural change, hard bypass, re-prepare, P9.
- **P9 Non-finite input (C10).** After the bands have run a chunk and before the write-back, the last working sample of each touched lane is tested with `std::isfinite`. If either is not finite: every band's state is cleared and its ramp snapped. The chunk's output is written as it is; nothing is replaced, limited or muted. Cost: two comparisons per chunk.
- **P10 Block-size identity (Q4).** Follows from P7, P8.1 and "per-sample values depend only on the position inside the tick". Bit-identity is claimed for block sizes that divide the event spacing (1, 32, 64, 512, 2048 with events on multiples of 2048). Blocks of 7 and 4096 see an event later: defined behaviour, covered for stability only.
- **P11 The G3 reference lives in the harness (C11).** `refRender`: the P2 trajectory re-stated in harness code (same restart rule, same lengths), then for every sample `Designer::design` → `toRuntime` → `svfTick` / `onePoleTick`. It calls no `ControlRamp`, no `Band`, no ramping tick. Both harnesses carry it (the render harness feeds it the same float input and compares in double). Controls are harness renders too: `holdRender` (grid designs, no interpolation) and `hardRender` (the 2.2 rule: design at the block start, state kept). The plugin has no test switch and gets none.
- **P12 G3 rows.** 48 kHz, sine at -6 dBFS, 50 Hz then 1 kHz, host blocks 32 and 512, parameter read once per block. Timeline p0: 0.5 s held at A, sweep to B, 0.4 s held, sweep back, 0.4 s held; frequency rows also p1 (there and back with no hold). The render starts with the parameter at A; the first 0.3 s is excluded from scoring. Thirteen motions:

  | # | Band | Motion |
  |---|---|---|
  | 1, 2 | Bell +18 dB, Q 1 | frequency 10 Hz ↔ 30 kHz in 50 ms, p0 / p1 |
  | 3 | Bell at the tone, Q 1 | gain −30 ↔ +30 dB in 20 ms |
  | 4 | Bell +18 dB at 2x the tone | Q 0.025 ↔ 40 in 20 ms |
  | 5, 6 | Low Cut 96 dB/oct, Q 1 | frequency 10 Hz ↔ 30 kHz in 50 ms, p0 / p1 |
  | 7 | Low Cut 96 dB/oct at 30 Hz | Q 0.025 ↔ 40 in 20 ms |
  | 8, 9 | High Shelf 12 dB/oct +18 dB | frequency 10 Hz ↔ 30 kHz in 50 ms, p0 / p1 |
  | 10 | High Shelf 12 dB/oct at tone / 2 | gain −30 ↔ +30 dB in 20 ms |
  | 11 | High Shelf 12 dB/oct +18 dB at the tone | Q 0.025 ↔ 40 in 20 ms |
  | 12, 13 | High Shelf 96 dB/oct +18 dB | frequency 10 Hz ↔ 30 kHz in 50 ms, p0 / p1 |

  13 motions × 2 tones × 2 block sizes = 52 readings.
  - **Verdict:** engine κ ≤ 4.0x reference κ, every reading.
  - **Interpolation-off control:** `holdRender` ≥ 10x the reference on motions 7, 8, 11 at the 50 Hz tone, block 32 (bench: 541, 143, 415).
  - **No-smoothing control:** `hardRender` at block 512 ≥ 10x the reference, per motion and tone. Two are void by construction and are listed as void in the output, not counted: motion 5 at 50 Hz (the tone dies; bench 9.2) and motion 7 at 1 kHz (a 30 Hz cut's Q barely moves a 1 kHz tone; bench 8.2). Any other reading below 10x fails the row.
  - **Held references:** engine κ against the larger of the two held renders, printed for all 52. WARN (not FAIL) above 4x on the 1 kHz readings of motions other than 5 and 6.
  - **Other grids:** motion 8 at 50 Hz, block 32, also at 96 kHz (K = 32) and 192 kHz (K = 64), same verdict.
- **P13 G5 draw.** 24 bands, stereo, noise at -12 dBFS with one second of silence in five. Frequency, gain, Q and gain scale redrawn every host block, 10 % of draws on each range end. Type, slope, placement, enable and band bypass redrawn every 50th block. Block sizes 4096, 512, 64, 7, 1 cycling every 2 s (prepared 4096). Rates 22.05, 32, 44.1, 48, 88.2, 96, 176.4, 192 kHz. **Length (C9): 60 s per rate by default; `--long` runs the contract's 10 minutes per rate.** The long run is made once in execute (T8) and once in verify, on both harnesses.
  - Static stack: the sum over active bands of the peak of `eq::bandMagDb` of each band's **target** design (floor 0 dB per band; 64 log-spaced points plus the band's own frequency), as a running maximum taken at the blocks of 512 and 4096 samples.
  - The plugin draw is the same generator with a fixed seed. Before rendering, the harness asserts that the largest stack of the whole draw is below +600 dB (float overflows at +770); if not, the row reports a stimulus failure and the seed is changed and recorded. The engine is never changed for it.
- **P14 G7.** 192 kHz, one bell at 10 Hz, Q 40, +12 dB, static.
  - Noise floor: 1 kHz sine at -6 dBFS, 2 s raised-cosine fade-in, 7 s; least-squares removal of the 1 kHz component from the last 3 s.
  - Ring-down (double only; float cannot represent it): the same sine stopped abruptly, 60 s of silence; RMS in 1 s windows.
  - Silence cost: 24 bells at 10 – 33 Hz, Q 40, 192 kHz stereo; median block time over the last 5 s of a 60 s silence against the median with noise. Timed three times; the row takes the best ratio.
- **P15 No audio golden** (2.2 P13 stands): `std::exp` / `std::log` and contraction differ across platforms. Bit-identity rows compare renders of one binary.
- **P16 Negative controls live inside the harness** as rows `…n`; the designer harness gains `--neg ramp-alloc` (a planted allocation inside the RM-alloc window).
- **P17 CPU is INFO.** G10 binds in Stage 4. The correction hold stays a documented fallback and is not built.
- **P18 Existing rows.** The designer harness's 67 rows and 12 INFO lines and the render harness's 64 rows print as in the T1 baselines, with two named exceptions: any SN1 / EN-dyn check amended under P6, and S2's detail (`pluginVersion 0.3.0`). A 2.2 row that edits a parameter mid-render (U1, U3, A1) keeps its verdict: each scores a steady state, an identity or a count, none the step itself. Any other change in an existing row → STOP.

---

## Tasks

Roles as in `PLAN-2.2.md`: **[agent]** = dsp-agent writes files (no Bash); **[run]** = the execute session. **BG** = `run_in_background`, log to `$S/<name>.log`; `$S` = the session scratchpad. Paths are relative to `plugins/O-ParametricEQ/` unless they start with `build/` or `scripts/`. `$H` = the designer harness binary, `$R` = the render harness binary.

### Wave 0 — pre-flight

**T1. Pre-flight and baselines** [run]
- Done when (any mismatch → STOP):
  - `shasum -a 256` of `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` equal STATUS `contract_checksums`.
  - `git branch --show-current` = `main`; `git status --short plugins/O-ParametricEQ` is empty. Save the full `git status --short` to `$S/git-status-T1.txt`.
  - (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness` → rc 0. `$H > $S/designer-base.log` → **67 PASS, 0 FAIL, 12 INFO**. `$R > $S/render-base.log` → **64 PASS, 0 FAIL**. `python3 tests/golden/check_spec.py` → `OK:`.

### Wave 1 — the modulation engine, in double

**T2. `Source/dsp/ControlRamp.h`; `BandParams.h` edits** [agent]
- P1, P2, P5, P6. File header in the style of `EqTypes.h`.
- `BandParams.h`: canonical gain in step 5 (the step list in the header comment updated); the offset helper; the lowest-Q comment corrected (`0.025f` lies just above 0.025, the clamp does not act).
- Depends on: T1.

**T3. `Source/dsp/SectionEngine.h`, `Band.h`: ramping path and control tick** [agent]
- P1, P3, P4. Port from `ramp_bench.cpp`. The settled functions (`toRuntime`, `svfTick`, `onePoleTick`) are not edited.
- `Band`: `snapTo (settings, designer)`, `setTarget (settings)`, `controlTick (designer)`, `isMoving()`. `reset()` clears the filter state only, as in 2.2; the caller pairs it with `snapTo` where P8 says so. `setDesign` stays for the static callers.
- `Router.h` is edited only if P4's lane-pair form needs it; its placement algebra does not change.
- Both header comments are brought up to date (what 2.3 added, what 2.4 will add).
- Depends on: T2.

**T4. `tests/designer-harness/main.cpp`: modulation rows** [agent]
- A new heading "2.3 / modulation (double)". A small harness driver (tick clock, P8.3 segment loop over `eq::Band` + `eq::Router`) and `refRender` / `holdRender` / `hardRender` (P11). No float anywhere unless stated. `--long` is parsed here (P13).

  | Row | Check | Threshold |
  |---|---|---|
  | RM-g3 | P12: 52 readings against `refRender` | each ≤ 4.0x |
  | RM-g3i | P12 interpolation-off control, 3 readings | each ≥ 10x (control fires) |
  | RM-g3h | P12 no-smoothing control at block 512, 26 readings | 24 ≥ 10x; the two void readings printed as void |
  | RM-g3k | Motion 8 at 96 and 192 kHz | ≤ 4.0x |
  | RM-g3r | INFO + WARN: the 52 readings against the held references | printed; WARN per P12 |
  | RM-arr | After a ramp of each of f, gain, Q and all three together (8 types, 5 rates), one interval past arrival: the band's runtime sections vs `toRuntime` of a static design of the target settings | `memcmp` equal; and `isMoving()` false, with a counting designer wrapper showing 0 designs over the next 100 ticks |
  | RM-arrn | The same compare taken one tick before arrival | must differ |
  | RM-keep | Kept state (Q7): bell 40 Hz, Q 20, +24 dB, rung by a burst; gain edited to +18 dB during the ring; `|y[n] − (2 y[n−1] − y[n−2])|` over the 20 ms after the edit against the ring's largest second difference just before it | ≤ 10x (bench 1.7x) |
  | RM-keepn | The same with the state cleared at the edit, and with the 2.2 step (`setDesign`) | each ≥ 100x (bench 3985x, 1353x) |
  | RM-blk | Q4: 4 bands (bell, 96 dB/oct High Shelf, 48 dB/oct Low Cut, 24 dB/oct Tilt), stereo noise, f / gain / Q of all four redrawn at each of 96 events on multiples of 2048; blocks 1, 32, 64, 512, 2048; 44.1, 48, 96, 192 kHz | bit-identical per rate |
  | RM-blkn | The same render with the events removed | must differ from RM-blk's (the events bite) |
  | RM-st | A Stereo band of each type with L = R input during a three-parameter ramp | L output `memcmp` equal to R output; and differs from the settled render |
  | RM-zero | Gain sign change: 200 000 random bell / shelf / tilt settings per rate, a ±0.0002 dB step across 0 | largest relative change of any section parameter ≤ 0.5 % (bench 0.075 %) and ≤ 3x the one-sided step of the same size (bench 2.1x) |
  | RM-norm | One bell, zero input, random start state; frequency, gain and Q retargeted at random (range ends included) every tick for 2000 ticks, 2000 trials, sampled inside the ramping path | the Euclidean norm of `(ic1, ic2)` never exceeds its previous value by more than 1e-9 relative |
  | RM-g5 | P13 on `eq::Band` + `eq::Router` in double, 8 rates | every sample finite; peak re input peak ≤ static stack + 12 dB at every check; 0 structure changes inside a continuous move; then all bands off → output `memcmp` equal to input from the next block |
  | RM-g5n | Peak clause control: the same check against the stack minus 200 dB | must fail (the stack is not a vacuous bound) |
  | RM-nan | One band, one NaN input sample, then noise, **without** the P9 check | output stays non-finite (the stimulus sticks; the plugin row S6 is then meaningful) |
  | RM-g7 | P14 in double | residual ≤ -180 dBFS RMS; 1 s windows monotone; slope 3.42 dB/s ± 2 % from the 10th to the 60th second; 60th second ≤ -270 dBFS |
  | RM-g7n | The ring-down slope check against 6.8 dB/s | must fail |
  | RM-dyn | `dynOffsetDb` written on a settled bell between ticks: (i) the band moves at the next tick and its design equals a static design at gain + offset, sections `memcmp` equal, with no 20 ms ramp (reached in one interval); (ii) offset back to 0 → sections equal the static design of the plain settings; (iii) offset on a Low Cut → no design | exact |
  | SN4 | Canonical gain: gain and `gain_scale` edits on Low Cut, High Cut, Notch, Band Pass | `BandInput` field-identical; control: the same edits on a Bell differ |
  | RM-alloc | 1e4 ticks with every band retargeted, plus the ramping path, inside the `malloc_logger` window | 0 allocations, 0 frees; SKIPPED line where the platform has no logger |
  | RM-cpu | INFO. 48 kHz stereo, 512-sample blocks, 20 s, 24 bands sweeping (Q5): 24 cuts × 96 dB/oct; 24 shelves × 96 dB/oct; 24 bells | printed beside the bench's 4.35 / 23.0 / 4.46 % and the 35 % line |

- `--neg ramp-alloc`: a planted allocation inside the RM-alloc window is counted; exit 0 only if it is.
- The 67 existing rows are not edited, except under P6.
- Depends on: T3.

### Wave 2 — the processor

**T5. `Source/PluginProcessor.h/.cpp`: the control grid** [agent]
- P7, P8, P9. Members added: the tick clock, K, nothing else beyond what the bands now hold.
- `prepareToPlay`: K and ramp length from the rate; clock 0; every band `snapTo` its current target on the first block (snapshots invalidated as in 2.2, so the first block is structural for every active band).
- `processBlockBypassed` and the `bypass` branch: state cleared, ramps snapped, clock advanced.
- `getBandDynOffsetDb` stays the only engine accessor. No test switch.
- Still no lock, no allocation, no listener, no `AsyncUpdater`, no `juce::SmoothedValue`, none of `juce::dsp::IIR`, `StateVariableTPTFilter`, `juce::dsp::Gain`.
- The file comments are brought up to date.
- Depends on: T3.

**T6. Version 0.3.0** [agent]
- `CMakeLists.txt`: `OPEQ_VERSION 0.3.0`. Nothing else in the file changes.
- `CHANGELOG.md`: `## [0.3.0] - <execute date>` (bracket form). Changed: frequency, gain, Q and gain scale moves are smoothed (20 ms) and no longer step; a gain or gain-scale edit on a cut, Notch or Band Pass does no work. Fixed: a non-finite input sample no longer leaves the filters stuck. Known in this build: type / slope / placement / enable changes click, global bypass is a hard switch, output gain and auto gain do nothing yet.
- Depends on: T5.

### Wave 3 — render-harness rows

**T7. `tests/render-harness/main.cpp`** [agent]
- New section headings for 2.3. `--long` is parsed here too. `refRender` / `hardRender` as P11, fed the plugin's own float input.

  | Row | Check | Threshold |
  |---|---|---|
  | M1 | G3 through `processBlock`: P12's 52 readings, the plugin's float output against `refRender` | each ≤ 4.0x |
  | M1h | `hardRender` at block 512 against `refRender` (the stimulus fires at the plugin's settings) | as RM-g3h |
  | M1k | Motion 8 at 96 and 192 kHz | ≤ 4.0x |
  | M2 | VERIFICATION-2.2 gap 2. Three Stereo bells at +24, +6 and 0 dB (three centres, three sines), `gain_scale` 100 → 25 mid-render: (i) steady-state gain of each 1 s later; (ii) local κ of the move against `refRender` | (i) +6 / +1.5 / 0 dB within 0.01 dB of the closed forms; (ii) ≤ 4.0x |
  | M2c | The same edit with a Low Cut and a Notch as the only bands | output bit-identical to the render without the edit (P6) |
  | M3 | `bNN_bypass` mid-render on the only active band: on, then off | on: output `memcmp` equal to input from that block; off: `memcmp` equal to a fresh instance fed the same remaining input |
  | M4 | Gap 3. One instance: prepare 44.1 kHz, render 1 s with 6 bands, prepare 96 kHz, render | `memcmp` equal to a fresh instance prepared at 96 kHz |
  | M5 | Gap 4. T1c's single Mid bell: gain of the Mid component at the bell centre; the converse on Side | equals the bell gain within 0.01 dB; the other component's gain there is 0 dB within 0.01 dB |
  | M6 | Gap 1 through the plugin: RM-keep's stimulus and metric on the float output | ≤ 10x |
  | M6n | A type change (Bell → Low Shelf) at the same sample, same metric | ≥ 100x (a structural change is not kept; fails if the two classes are confused the other way) |
  | B1 | Block identity: RM-blk's script plus `gain_scale` events, blocks 32, 64, 512, 2048 at 44.1, 48, 96, 192 kHz | bit-identical per rate; differs from the render without events |
  | TK | Tick clock (P7). 1000 samples idle, in turn by (a) `bypass` on, (b) no active band, (c) `processBlockBypassed`; then a bell active from sample 1000, noise input, block boundaries at 1000 and 1036, a 0 → +24 dB gain edit visible at the block starting at 1036. Compared with the same render without the edit | first differing sample index = 1040 at 48 kHz (the first tick at or after 1036, counted from `prepareToPlay`), for (a), (b), (c); 1056 with the boundary and the edit at 1041; 1024 with both at 1024 (a block that starts on a tick acts at once) |
  | S5 | G5, P13, seeded, through `processBlock`, 8 rates | stack guard holds; every sample finite; peak printed per rate; then all bands off → `memcmp` equal to input within 100 ms |
  | S6 | One block containing a NaN sample, then noise; and the same with +inf; 6 mixed bands | output finite from the next block on; and from 2 s on within 0.01 dB (steady-state, single-bin) of a fresh instance |
  | A2 | G6 under stress: the `malloc_logger` window around `processBlock` only, 20 s of the S5 draw at 48 kHz, first block included; parameter writes outside the window | 0 allocations, 0 frees; liveness probe |
  | A2n | A planted allocation inside the window | counted |
  | N7 | G7 float, P14: residual on the plugin's output; silence cost through `processBlock` | ≤ -140 dBFS RMS; silence ≤ 1.2x signal |
  | X-cpu2 | INFO. Sweeping (Q5) through `processBlock`: 24 cuts × 96 dB/oct; 24 shelves × 96 dB/oct; 24 bells; and settled, for comparison with 2.2's X-cpu | printed beside 4.35 / 23.0 / 4.46 % and the 35 % line |

- Depends on: T5.

### Wave 4 — build, gates, install, validators

**T8. Build and run** [run]
- Done when, in order:
  1. (BG) `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` (version bump), then `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness > $S/build.log 2>&1` → rc 0; `grep "O-ParametricEQ/.*warning:" $S/build.log` → no lines.
  2. (BG) `$H > $S/designer.log 2>&1` → **0 FAIL**. The 67 / 12 lines of `$S/designer-base.log` print the same figures (P18); every T4 row ID appears; RM-g3r's WARN count is recorded.
  3. `$H --neg ramp-alloc` → rc 0. The eight existing `--neg` controls → rc 0 each.
  4. (BG) `$R > $S/render.log 2>&1` → **0 FAIL**. The 64 existing rows print as in `$S/render-base.log` (P18); every T7 row ID appears; every `…n` row reports that its control fired.
  5. (BG, one at a time) `$H --long > $S/designer-long.log 2>&1` and `$R --long > $S/render-long.log 2>&1` → 0 FAIL each; RM-g5 and S5 print 600 s per rate.
  6. `python3 tests/golden/check_spec.py` → `OK:`; `$R --dump-params | diff - tests/golden/params.tsv` → empty.
  7. Static gates, from the repo root:
     - `! grep -rnE "juce|Juce|std::vector|std::function|new |malloc|throw " plugins/O-ParametricEQ/Source/dsp`
     - `! grep -rnE "fast-math|Ofast|fp:fast|ffp-contract" plugins/O-ParametricEQ/CMakeLists.txt`
     - `! grep -rnE "IIR::|StateVariableTPT|dsp::Gain|SmoothedValue|AsyncUpdater|callAsync|CriticalSection|std::mutex|addParameterListener" plugins/O-ParametricEQ/Source/PluginProcessor.h plugins/O-ParametricEQ/Source/PluginProcessor.cpp`
     - `grep -rn "dynOffsetDb" plugins/O-ParametricEQ/Source` → the declaration and its reads only; no write outside the initialiser.
     - `grep -rn "makeBandInput" plugins/O-ParametricEQ/Source plugins/O-ParametricEQ/tests` → one definition.
     - Read the bodies of `refRender`, `holdRender` and `hardRender` in both harnesses: none of `ControlRamp`, `controlTick`, `setTarget`, `snapTo` or the ramping tick appears in them (P11: the reference shares no code with the ramp).
- **On a failure:**
  - An RM row red → the engine. Fix in `Source/dsp`, rebuild, rerun 2 – 7 in full.
  - RM rows green and a render row red → the processor glue or the row's float scope. Fix the glue. A threshold, motion list or stimulus level is not changed in this session; if a row cannot pass as specified with the RM rows green, STOP and escalate with the figures.
  - A control that does not fire → the row is vacuous. Fix the row, not the control.
  - RM-g3h or M1h reads below 10x on a non-void reading → STOP with the figures (the void list is contract text in G3).
  - An existing row changes outside P18 → STOP.
- Depends on: T4, T6, T7.

**T9. Install and validate** [run]
- One validator at a time, each BG with its own log.
  1. `./scripts/build-and-install.sh O-ParametricEQ > $S/install.log 2>&1` → rc 0; exactly the two `-dev` bundles in the system folders, version 0.3.0.
  2. pluginval, strictness 10, `--timeout-ms 600000`, VST3: **2 runs** → `SUCCESS`. AU: 1 run → `SUCCESS`.
  3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`; channel capabilities still `[1, 1] [2, 2]`. Never `auval -a`.
  4. Standalone smoke as in 2.2 T9 (rebuild the Standalone target first) → `ALIVE`.
- Any failure: fix the root cause in `Source/`, then rerun T8 and T9 in full. A pluginval NaN or `inf` report is a finding about stacked gain (2.2 R3): record the configuration, insert nothing in the signal path.
- Depends on: T8.

### Wave 5 — bookkeeping + commit

**T10. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/2-dsp/SUMMARY-2.3.md`: tallies of both harnesses (default and `--long`); RM-g3 / M1 as the bench's table (engine / reference, hold / reference, hard / reference, engine / held) with the WARN count; RM-keep, RM-zero, RM-norm figures; RM-g5 / S5 per rate (peak, stack, margin); RM-g7 / N7 figures; A2 counts; X-cpu2 beside the bench; the float G3 readings beside the double ones (RESEARCH "Not answered here"); controls; validator results; any P6 amendment of an existing check; deviations.
- Carry-forward in the summary: for 2.4: G3 step rows (the held-reference threshold there deserves the same scrutiny C1 gave the sweeps), structural changes still click, G4 (d) (e) (f), G5 with structural fades, the tick-latched pending change; for 2.5: `output_gain` and `auto_gain`; for 2.6: G6 with the analyzer on; for verify: a hands-on listen for whether 20 ms sounds right on real material, G3 with several bands moving at once.
- `REQUIREMENTS.md`: no status edit (requirements close at verify).
- `PLUGINS.md`: version cell → 0.3.0. Check `git diff PLUGINS.md` first; if other sessions' rows are modified, use the temp-index recipe (memory `index_git_shared_checkout.md`) or leave it out and say so.
- STATUS: phase rows, `status: stage_2_phase_2_3_executed`, `next_action: stage_2_verify_phase_2_3`, files list.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` the new files by name (`Source/dsp/ControlRamp.h`, `.planning/stages/2-dsp/SUMMARY-2.3.md`), then `git commit -- plugins/O-ParametricEQ`. Never `-A` / `-a`. `git log -1 --stat` lists only this plugin's paths. No tag.
- Depends on: T9.

---

## Dependency graph

```
T1 → T2 → T3 ─┬→ T4 ───────────────┐
              └→ T5 → T6 ──┐        │
                   └→ T7 ──┴────────┴→ T8 (build, harnesses, controls, --long) → T9 (install, validators) → T10
                                        ↑   │
                                        └───┘ fix → rebuild → rerun T8 in full (and T9 if Source changed)
```

T4 and T5 / T7 touch different files and can be written in either order; one build covers all of them.

## Success criteria

| Criterion (ROADMAP Phase 2.3 as amended; ARCHITECTURE G3, G5, G6, G7) | Proven by |
|---|---|
| G3 sweep rows ≤ 4.0x the same-motion reference (DSP-04) | RM-g3, RM-g3k, M1, M1k, M2 (ii) |
| G3 controls fire (interpolation off, no smoothing) | RM-g3i, RM-g3h, M1h |
| What the same-motion ratio cannot see: design continuity across the gain sign change | RM-zero; RM-g3r WARN ceiling |
| G5: finite at 8 rates, peak within the static stack + 12 dB, no sticky state (DSP-04, DSP-06) | RM-g5, RM-g5n, S5; `--long` once |
| Non-finite input does not stick | S6, RM-nan |
| Time-varying stability rests on the argument's precondition | RM-norm; P1 (`a1` by division from one `g, k`), T8.7 read |
| G7 noise floor and ring-down (DSP-05) | RM-g7, RM-g7n, N7 |
| G6 under stress (PERF-01) | RM-alloc, A2, A2n, `--neg ramp-alloc` |
| Events on multiples of 2048 render bit-identically at 32, 64, 512, 2048 | RM-blk, RM-blkn, B1 |
| The grid is global and independent of the host block | TK, B1 |
| A settled band equals the static design and does no design work | RM-arr, RM-arrn; 2.2 rows unchanged (P18) |
| A continuous change keeps the state; a structural one does not (VERIFICATION-2.2 issue 1) | RM-keep, RM-keepn, M6, M6n |
| `gain_scale` and band bypass through the plugin (issue 2) | M2, M3 |
| Re-prepare at another rate (issue 3) | M4 |
| Mid / Side response through the plugin (issue 4) | M5 |
| Gain is inert on cuts, Notch, Band Pass (issue 6) | SN4, M2c |
| `dynOffsetDb` is read at the tick, after the ramp (issue 7; DSP-07) | RM-dyn; EN-dyn and V1 unchanged |
| Stage 1 and 2.2 rows, layout golden unchanged | T8.2, T8.4, T8.6 |
| 0.3.0 validates | T9 (pluginval 10 ×3, `auval`, Standalone) |

## Risks

- **R1 The bench is double and is not the plugin.** The float G3 readings are unmeasured (research estimates float rounding at about 1 % of a held 50 Hz tone's second difference). If M1 misses with RM-g3 green, that is a finding: STOP (T8), do not move 4.0x.
- **R2 The same-motion verdict hides anything both renders share.** It cannot see a design jump or a wrong ramp length. Covered by RM-zero, the 2.1b continuity rows, RM-g3r's WARN ceiling, RM-arr (the ramp lands on the target), and the hands-on listen at verify.
- **R3 The reference shares the trajectory by restatement.** If the harness copy of the ramp rule drifts from P2 (restart rule, tick lag), M1 reads a timing offset as a click ratio. `refRender` follows P2 and P3's one-interval lag to the sample; a first-difference index check between engine and reference at the sweep start belongs in RM-g3's stimulus guard.
- **R4 The tick clock on early returns.** A clock that stops while bypassed or idle is invisible to every block-identity row (2048 is a multiple of K). TK is the only row that sees it; do not drop it.
- **R5 A latched moving flag.** Deriving `moving` anywhere but P3 step 3 is the suite's run-flag failure. RM-arr's "0 designs over the next 100 ticks" and RM-dyn (ii) are the rows.
- **R6 Timing rows on a busy machine.** RM-g7's and N7's silence-cost ratio is a wall-clock measurement: medians, best of three, nothing else running. A miss is re-measured once on an idle machine before it counts.
- **R7 Run time.** Default G5 is 8 minutes of audio per harness, `--long` 80 minutes (bench: 301 s for the engine alone). G3 is 52 readings × 4 renders with a per-sample design in the reference. Everything BG; thread RM-g5 per rate as NW2 is threaded if the default run passes about ten minutes. Do not cut counts.
- **R8 pluginval fuzzes 196 parameters with ramps now running.** Expected to pass; a NaN / `inf` report is stacked gain, not a ramp defect, unless S5 is also red.
- **R9 Gate tokens in prose.** T8.7 greps `Source/dsp` comments too: no "new ", "throw ", "malloc", "juce" in the headers' comments. "SmoothedValue" may not appear in the processor even in a comment.
- **R10 Shared checkout.** Other sessions have uncommitted files in the tree. Path-scope the commit and re-check immediately before it (T10).
