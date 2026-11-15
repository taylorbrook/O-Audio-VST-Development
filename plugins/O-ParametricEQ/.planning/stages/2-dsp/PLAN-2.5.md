---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.5 Execution Plan (output stage and auto gain)"
created: 2026-11-14
juce_version: "8.0.15"
phase: "2.5"
summary: "Six waves, serial: (W0) pre-flight and baselines; (W1) eq::GainSmoother / eq::OutputStage and eq::AutoGain as std-only headers in Source/dsp, with their double-precision rows in the designer harness; (W2) the processor with the stage between the bands and the bypass mix, the four fast-path changes, seeding, the max-hold peak atomics, version 0.5.0; (W3) render-harness rows (G12, G3 gain rows, G4 (a) with the stage, seeding, block identity, G5 / G6 extended, meter, the VERIFICATION-2.4 Issue 2 row) and the amendment of S5; (W4) build, both harnesses, controls, --long, install, pluginval 10, auval; (W5) summary, STATUS, one path-scoped commit. No analyzer, display model, listen or pre-roll. No tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 2 / Phase 2.5 Execution Plan (output stage and auto gain)

## Goal

Output gain and auto gain work, click-free and seeded. Each gain is a four-pole one-pole cascade in linear amplitude; both are applied as one multiply between the band chain and the bypass mix. Auto gain is the negative pink-weighted mean power gain of the target curve on 512 log-spaced cell midpoints, refreshed on the tick clock on the audio thread, so offline and real-time renders are bit-identical. With output gain at 0 dB and nothing for auto gain to compensate, the stage is idle and G4 (a) holds exactly as before. The processor keeps a max-hold output peak per channel for the meter. Version 0.5.0 is built, installed and validated.

Proved at two levels, as in 2.2 – 2.4: the stage and the estimator in double by the designer harness, the plugin's float output by the render harness.

**In scope:** `eq::GainSmoother`, `eq::OutputStage`, `eq::AutoGain`; the processor's step 5 (idle predicate), step 6 (lanes, the stage in each segment, ticks), seeding at the first block and at both un-bypass restarts, the peak atomics. Gates G12, G3 for the gains, G4 (a) with the stage, block identity with gain events, G5 / G6 with the stage. VERIFICATION-2.4 Issue 2 (PX14, PX9).

**Out of scope (and what 2.5 does instead):**

| Later | 2.5 behaviour |
|---|---|
| 2.6 | No taps, no display model, no listen crossfade. G6 "with the analyzer on" re-runs there. The listen path will sit between the bands and this stage. |
| Stage 3 | The `vu-meter` module is not added. The atomics exist and are read only by the harness; the editor timer, `exchange (0)` and `VUMeterBridge` feeding are Stage 3. |
| VERIFICATION-2.4 Issue 1 | Open, waiting on the listen (Taylor, 2026-11-14). No pre-roll, no warm start, no stop-band row in 2.5. |
| Not planned | A 1024-point grid. Dynamic offset in the auto-gain target. Side-band or L / R weighting other than ARCHITECTURE's. |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/RESEARCH-2.5.md` | All of it. Q1 (smoother), Q2 (the four processor changes), Q3 (estimator), Q4 (meter), Q5 (rows). |
| `stages/2-dsp/research-2.5/gain_bench.cpp`, `bench-output.txt` | Working prototype and its figures. The P4 smoother, the estimator (grid, `sectionMag2` products, refresh schedule), the truth (40 000-point midpoint pink mean), and the measurements (`steps`, `retarget`, `sweep`, `stress`, `ag`, `ripple`, `cost`). |
| `stages/2-dsp/VERIFICATION-2.4.md` | Issues 2 and 4 (the row this plan owes; the multi-band settle rule). |
| `stages/2-dsp/SUMMARY-2.4.md`, `PLAN-2.4.md` | P8 (the per-block order this plan extends), P7 (lane rule), P10 (controls live in the harness), P18 style of amendments. |
| `stages/2-dsp/CONTEXT.md` | "Constraints carried". Binding. |
| `research/ARCHITECTURE.md` | § 6, § 7, "Processing Chain", "Auto gain", "Processing Order Requirements", "Thread Boundaries", G3, G4, G12 (as amended by this plan phase). |
| `Source/dsp/Fade.h`, `SlotBand.h`, `ResponseEval.h`, `Designer.h`, `BandParams.h` | The verified engine. **Not edited** (P1). `ResponseEval::sectionMag2`, `tanSquaredHalf` and `Designer::design` are what the estimator calls. |
| `Source/PluginProcessor.h/.cpp` | `processAudio` as 2.4 left it (steps 1 – 9). |
| `tests/designer-harness/main.cpp`, `tests/render-harness/main.cpp` | Row and tally conventions; `XfEngine`; `fadeRefRender`; `render` with its per-block callback; A1 – A3 allocation windows; the step-row readings `maxD2`, local κ; `refRender` (2.3 sweep reference). |
| Memory `index_dsp_gate_design.md`, `index_testing_render_harness_patterns.md`, `pattern_gate_stimulus_below_threshold_is_vacuous.md`, `pattern_engine_state_synced_only_in_processblock_lost_when_bypassed.md`, `pattern_run_flag_past_dirty_check_latches.md`, `pattern_smoothedvalue_reset_does_not_clear_a_nan.md` | Suite gate recipes and traps this phase leans on. |

---

## Plan-phase decisions

User call taken in this phase (2026-11-14), contracts amended and the ARCHITECTURE checksum refreshed:

- **C1 Auto-gain grid: 512 cell midpoints** (Taylor's, as recommended). ARCHITECTURE "Stage 0 Decisions" row 6, § 6, § 7, "Auto gain", the `output_gain` mapping row and the G12 row are amended (smoother wording, grid, linear power, the G3 forms of the gain rows).
- **Issue 1 stays open** (Taylor's): no hands-on listen yet; 2.5 does not carry a fix.

The plan's calls:

- **P1 Files.** Two new std-only headers. No other `Source/dsp` file is edited; if one must be, it is a deviation recorded with its reason, and every row that touches that file must print as in the baseline. Same rules as before (C++17, `noexcept`, no allocation, no framework, the gate tokens absent even from comments).

  | File | Content |
  |---|---|
  | `Source/dsp/OutputStage.h` (new) | `outputGainSeconds = 0.020`, `autoGainSeconds = 0.100`, `smootherSettleX = 13.0622`, `smootherSnapTolerance = 1e-9`, `outputGainMinDb / MaxDb = ∓36`. `eq::GainSmoother`, `eq::OutputStage`, `outputGainTarget (raw)`. |
  | `Source/dsp/AutoGain.h` (new) | `autoGainPoints = 512`, `autoGainLowHz = 40`, `autoGainHighHz = 16000`, `autoGainTopFraction = 0.45`, `autoGainMinDb = -18`, `autoGainMaxDb = +12`, `autoGainRefreshSamples48k = 1024`. `eq::AutoGain`, `autoGainRefreshTicks (fs, K)`. |

- **P2 `GainSmoother`, exactly (RESEARCH P1).**
  - State `y[4]`, target `T`, coefficient `a`, `settled`.
  - `prepare (N)`: `a = 1 − exp (−smootherSettleX / N)`, N ≥ 1.
  - `snap (T)`: all four stages = T, `settled = true`.
  - `setTarget (T)`: stores T; `settled = false` unless all four stages already equal T exactly. Nothing else; the state carries everything (no restart, no latch).
  - `next()`: when settled, returns T. Else `y1 += a (T − y1); y2 += a (y1 − y2); y3 += a (y2 − y3); y4 += a (y3 − y4)`; if all four `|y_k − T| ≤ 1e-9`: all set to T exactly, `settled = true`. Returns `y4`.
  - `isSettledAt (v)`: `settled && T == v` (exact compare).
  - Each `y_k` is a convex combination of its previous value and its input, so the output stays inside the range of the targets and is never ≤ 0 for positive targets. No clamp is added (a clamp would hide a defect, memory `pattern_downstream_guard_masks_defect_gate_the_clamp_target.md`).
- **P3 `OutputStage`, exactly (RESEARCH P2, P3).**
  - Members: `GainSmoother og, ag`; `pendingOg` (the output target read at the last block start).
  - `prepare (fs)`: `og.prepare (round (0.020 fs))`, `ag.prepare (round (0.100 fs))`; both snapped to 1.0; `pendingOg = 1.0`.
  - `snap (ogTarget, agTarget)`; `setOutputPending (T)` (block start); `applyOutputPending()` (on a tick: `og.setTarget (pendingOg)`); `setAutoTarget (T)` (on a refresh tick).
  - `isUnity()`: `og.isSettledAt (1.0) && pendingOg == 1.0 && ag.isSettledAt (1.0)`.
  - `process (double* L, double* R, int n)` (R may be null): when both are settled, `g = ag.T * og.T`; `g == 1.0` → return without touching a sample; else `lane[i] *= g`. Otherwise per sample `g = ag.next() * og.next()` (this order, fixed), `L[i] *= g`, `R[i] *= g`. One multiply per lane sample.
  - `outputGainTarget (raw)`: non-finite → 0 dB; clamp to ±36; `std::pow (10.0, dB / 20.0)` (exactly 1.0 at 0 dB).
- **P4 `AutoGain`, exactly (RESEARCH P4, C1).**
  - Fixed storage: `t2[512]`, `P[24][512]`, `PL[512]`, `PR[512]`, per band `BandInput lastEval` + `bool contributes` + `bool evaluated`. About 220 KB, a member of the processor (the processor is heap-allocated). No allocation anywhere.
  - `prepare (fs)`: `fTop = min (16000, 0.45 fs)`; `f_i = exp (ln 40 + (ln fTop − ln 40)(i + 0.5) / 512)`; `t2[i] = tanSquaredHalf (2π f_i / fs)` (`ResponseEval.h`); every band `evaluated = false`.
  - `contributes (input)`: `input.active` and not (a gain-bearing type with `settings.gainDbEff == 0.0` exactly). The same test as `SlotBand`'s sleep target: use the same predicate function if `SlotBand.h` exposes one; else restate it and say so in the header.
  - `estimate (const BandInput* desired[24] or the 24 bands, bool stereo, const Designer&)` → the linear target:
    1. Per band: if `! evaluated` or `desired ≠ lastEval` (compared by value, field by field, the `classifyBandChange` identity test): re-evaluate. Contributing → `designer.design (settings)`, `P[b][i] = Π sections sectionMag2 (t2[i], s)`; not contributing → mark only. `lastEval = desired`, `evaluated = true`.
    2. No contributing band → return exactly 1.0.
    3. `PL[i] = PR[i] = 1`; per contributing band in index order multiply into PL (Stereo, Mid, Left), PR (Stereo, Mid, Right); Side into neither; mono: every band into both.
    4. `mean = Σ 0.5 (PL[i] + PR[i]) / 512` in index order.
    5. `mean` NaN → 1.0; `mean = +inf` → `10^(−18/20)`; `mean ≤ 1e-30` → `10^(+12/20)`; else `AG_dB = clamp (−10 log10 (mean), −18, +12)`, return `pow (10, AG_dB / 20)`.
  - **Invariant (binding):** the value returned is a pure function of the 24 desired inputs, `stereo` and fs. The cache is only an optimisation. This is what lets the processor skip refreshes while idle without breaking block-size identity (P6).
  - `isDirty (desired…)`: any band whose desired input differs from `lastEval` (cheap compare, no design). Counters for the harness: evaluations, refreshes.
  - `evaluateAll (…)`: the same as `estimate` with every band forced dirty (seeding).
- **P5 Auto-gain schedule (RESEARCH Q3 "Schedule").**
  - `R = max (1, round (1024 / 48000 · fs / K))` ticks: 59 at 44.1 / 88.2 / 176.4 kHz, 64 at 48 / 96 / 192 kHz, 29 at 22.05 kHz, 43 at 32 kHz. A **refresh tick** is a tick whose index `clock / K` is a multiple of R.
  - `auto_gain` is read at the block start (`agWanted`); it takes effect at the next refresh tick (`agOn = agWanted`), in both directions.
  - At a refresh tick, after the band ticks: `agOn` → `stage.setAutoTarget (autoGain.estimate (…))`; off → `stage.setAutoTarget (1.0)`. With `agOn` off nothing is evaluated (bands stay dirty until it is turned on).
  - The output target: read and sanitised at the block start (`setOutputPending`), applied at the next tick (`applyOutputPending`); at a block start that is itself a tick, at once.
- **P6 Processor, per host block (PLAN-2.4 P8 extended; RESEARCH Q2).** Steps 1 – 4 and 7 – 9 as in 2.4 except where named.
  - **Step 4 (parameters):** also read `output_gain` (→ `stage.setOutputPending (outputGainTarget (raw))`) and `auto_gain` (→ `agWanted`), every processed block.
  - **Seeding (step 4 `restart` and the step 7 restart):** after every band's `startSettled`: `agOn = agWanted`; `stage.snap (pendingOg, agOn ? autoGain.evaluateAll (…) : 1.0)`. A session that opens at +12 dB, or an edit made while bypassed, is in effect from the first sample, not ramped in.
  - **Step 5 (the empty list):** the return with the buffer untouched (and `bypass.jump`) is allowed only when `numListed == 0` **and the stage is idle for the block**: `stage.isUnity()` **and** (`agOn == agWanted` **and** (`! agOn` or `! autoGain.isDirty (…)`)). With no running band every band's desired input contributes nothing (a band that is not running holds no latch and realises an off target), so a skipped refresh would have returned exactly 1.0: P4's invariant makes the skip invisible. Otherwise the block runs with an empty band list.
  - **Step 6 (lanes):** stage idle for the block (the same predicate, without `numListed == 0`) → the P7 union as in 2.4. Not idle → every lane of the layout is converted. The predicate is computed once at the block start; the stage cannot leave unity inside a block whose start found it idle (the pending output target is fixed for the block, and a refresh with nothing dirty returns the current target).
  - **Step 6 (per segment):** on a tick: band `controlTick`s (index order), then `stage.applyOutputPending()`, then on a refresh tick the auto-gain retarget (P5). Then the dry copy if a bypass fade is in flight; the bands; **`stage.process` over the segment's converted lanes**; the bypass mix. Order fixed: bands → auto gain × output gain → bypass mix (ARCHITECTURE "Processing Order Requirements" 4, 5).
  - **Bypass toggle with only the gain off unity:** falls out of step 5 — the jump is taken only when the stage is idle, so otherwise the toggle fades.
  - **Step 3 (fully bypassed)** and **step 8 (non-finite)** unchanged: the stage holds no audio state and its targets are sanitised.
- **P7 Peak atomics (RESEARCH P5).** `std::array<std::atomic<float>, 2> outputPeak` (mono: index 0 only). In `processBlock` and `processBlockBypassed`, **after** `processAudio` returns, on every path (prepared or not, bypassed, untouched): per output channel of the layout `p = buffer.getMagnitude (ch, 0, n)`; `if (p > slot.load (relaxed)) slot.store (p, relaxed)`. Public `float readAndResetOutputPeak (int ch) noexcept { return slot.exchange (0.0f); }`. Read-only on the buffer; no `memcmp` row can see it. A non-finite sample: `getMagnitude` of a NaN compares false and does not store; an `inf` stores `inf` (the meter shows the overflow). Stated in the header comment.
- **P8 No test switch in the engine or the plugin** (2.3 P11, 2.4 P10 stand). Controls are harness renders:
  - `hardGainRender`: the plugin's (or engine's) output with output gain 0 dB and auto gain off, multiplied in the harness by a gain that jumps to the target at the sample the stage would start moving. The hard control of the gain step rows.
  - `linearGainRender`: the same with a linear ramp of N samples — the RESEARCH Q1 LIN figure, printed (not a control).
  - `gainRefRender`: an independent per-sample restatement of the P2 recurrence (harness code; no `GainSmoother`) applied to the gain-free output, scored sample by sample (P9 XO-ref).
- **P9 No audio golden** (2.2 P13 stands). Bit-identity rows compare renders of one binary.
- **P10 Negative controls live inside the harness** as rows `…n`; the designer harness gains `--neg ag-alloc` (a planted allocation inside the XO-alloc window).
- **P11 CPU is INFO.** G10 binds in Stage 4.
- **P12 Existing rows.** Baselines: designer harness **114 PASS / 0 FAIL / 16 INFO**, render harness **100 PASS / 0 FAIL**.
  - **Designer harness: no existing row is edited and all print as in the baseline.** None of them instantiate the stage.
  - **Render harness: one amendment, by name.**

    | Row | 2.4 assertion | 2.5 form |
    |---|---|---|
    | S5 | the P14 draw (2.4) | the same, plus `output_gain` (uniform −36 … +36, range ends with p 0.05) and `auto_gain` (toggled p 1/100) per block; the stack guard is stack + 12 dB **+ the drawn output gain + 12 dB auto-gain allowance**; "all bands off → `memcmp` equal within 100 ms" becomes "all bands off, output gain 0 dB, auto gain off → `memcmp` equal within **300 ms**" (the auto-gain smoother snaps at about 232 ms) |

    Every other existing row runs with output gain 0 dB and auto gain off, where the stage is idle; any existing row whose verdict or figures change → STOP with the row and both readings. The executor lists in the summary every existing row that writes `output_gain` or `auto_gain` (expected: S5, the state round-trip rows, and pluginval-style sweeps) and what it read.
- **P13 Multi-band compares (VERIFICATION-2.4 Issue 4).** Any new row that compares a multi-band restart against a fresh instance uses one band, or compares against a chain of single-band instances, or reads after 300 ms. Rows below say which.

---

## Tasks

Roles as in `PLAN-2.2.md`: **[agent]** = dsp-agent writes files (no Bash); **[run]** = the execute session. **BG** = `run_in_background`, log to `$S/<name>.log`; `$S` = the session scratchpad. Paths are relative to `plugins/O-ParametricEQ/` unless they start with `build/` or `scripts/`. `$H` = the designer harness binary, `$R` = the render harness binary.

### Wave 0 — pre-flight

**T1. Pre-flight and baselines** [run]
- Done when (any mismatch → STOP):
  - `shasum -a 256` of `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md` equal STATUS `contract_checksums` (ARCHITECTURE refreshed by this plan phase).
  - `git branch --show-current` = `main`; `git status --short plugins/O-ParametricEQ` is empty. Save the full `git status --short` to `$S/git-status-T1.txt`.
  - (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness` → rc 0. `$H > $S/designer-base.log` → **114 PASS, 0 FAIL, 16 INFO**. `$R > $S/render-base.log` → **100 PASS, 0 FAIL**. `python3 tests/golden/check_spec.py` → `OK:`.

### Wave 1 — the stage and the estimator, in double

**T2. `Source/dsp/OutputStage.h`** [agent]
- P1, P2, P3. File header in the style of `Fade.h`: the recurrence, why four poles (G3 at 50 Hz: linear 25x, P2 3.1x, P4 1.56x), why no clamp (convex combination), the snap rule, the "within 1e-3 at N, exact at about 2.5 N" wording.
- Depends on: T1.

**T3. `Source/dsp/AutoGain.h`** [agent]
- P1, P4, P5 (`autoGainRefreshTicks`). Includes `Designer.h`, `ResponseEval.h`, `BandParams.h`; edits none. Header comment: the formula, the midpoint grid and why (C1, the ripple table's 64 / 512 rows), linear power and its underflow argument, the guards, the pure-function invariant.
- Depends on: T1.

**T4. `tests/designer-harness/main.cpp`: output-stage rows** [agent]
- A new heading "2.5 / output stage and auto gain (double)". Stimuli and scores as `gain_bench.cpp` (48 kHz, tone at 0.5, eight phases, edits on the 16-sample grid, worst reported; `maxD2`, local κ as in 2.4). Heavy rows threaded as XF-g3 is. `gainRefRender`, `hardGainRender`, `linearGainRender` (P8). An independent `agRef` (harness code: grid, `Designer::design`, `sectionMag2`, products, mean, clamp, re-stated; no `AutoGain`) and the 40 000-point pink truth.

  | Row | Check | Threshold |
  |---|---|---|
  | XO-step | Output gain 20 ms: the 20 ordered pairs of −36 / −12 / 0 / +12 / +36 dB; auto gain 100 ms: 0 ↔ −18 / −6 / +6 / +12 dB. 50 Hz and 1 kHz | (a) ≤ 2.0x the louder held (bench P4 1.56x / 1.00x; AG 1.01x); 1 kHz κ ≤ 2.5x (bench 2.22x) |
  | XO-steph | `hardGainRender` on the same pairs | ≥ 10x on (a) every pair (bench 59x at 1 kHz, 23 000x at 50 Hz); `linearGainRender` printed (bench 25.1x) |
  | XO-ret | The 32 reversal rows (Q1 table: four cases × second change at 25 / 50 / 75 / 90 %, both gains) | ≤ 2.0x (bench 1.58x) |
  | XO-sweep | −30 → +30 → −30 dB in 20 ms and in 1 s, automation sampled at block starts 32 / 512, against a per-sample reference | ≤ 4.0x the reference (bench 0.03x / 0.78x); lag 20 ms after the sweep ≤ 0.1 dB (bench 0.03 dB) |
  | XO-stress | 200 s of random automation, blocks 1 – 2048 random | gain within [min, max] of the targets at every sample; never ≤ 0 |
  | XO-snap | Settle-to-snap after a step, every pair, 8 rates | output gain 40 – 60 ms (bench 45 – 52); auto gain 200 – 260 ms (bench 222 – 232); after the snap the output equals the target exactly and `isSettledAt` is true |
  | XO-ref | `OutputStage` against `gainRefRender` over the XO-step and XO-ret renders | ≤ 1e-12 relative, sample by sample |
  | XO-refn | `gainRefRender` with X = 12.0, and with the product order reversed and the stages applied in float | each must exceed 1e-9 (the first) / differ (the second) |
  | XO-unity | `outputGainTarget (0)` = 1.0 exactly; ±36 dB and the clamp ends; NaN / ±inf raw → 1.0 / clamp ends; a settled unity stage leaves a buffer of denormals bit-identical | exact |
  | XO-ag | `AutoGain` against the pink truth, the RESEARCH Q3 table rows (G12 shelf, 96 dB/oct shelves, HS 15 kHz, Bell +30 Q 10, BP 1 kHz Q 10, 9 mild rows), at 44.1 / 48 / 96 / 192 kHz | G12 shelf within 0.001 dB of −6.167; every row within 0.06 dB of truth (bench worst 0.055) or at its clamp |
  | XO-agref | `AutoGain` against `agRef`, 2000 random 24-band configurations incl. Mid / Side / Left / Right placements, mono and stereo, 5 rates | ≤ 1e-12 dB |
  | XO-agrip | Ripple: bells swept 200 Hz → 8 kHz in 4000 log steps (the Q5 table rows) | p-p ≤ 1.2x the bench's 512 figures (Q 40 +12: 0.028 dB; Q 10 +30: 0.12 dB; Q 40 +30: 2.92 dB); the 64-point restatement in `agRef` reads ≥ 0.8 dB on Q 40 +12 (the control: the grid matters) |
  | XO-agid | Pure function: the estimate after any sequence of edits equals `evaluateAll` on the same inputs, bit for bit (1000 random edit sequences, refreshes skipped at random) | bit-identical |
  | XO-agz | No contributing band (all off; all gain types at exactly 0 dB; Side-only) → exactly 1.0, 0 evaluations of a design; a gain type at 0.0001 dB contributes | as stated |
  | XO-agg | Guards: 24 × −60 dB bells (mean tiny) → +12 dB; 24 × +60 dB shelves → −18 dB; clamp ends exact | as stated |
  | XO-agsch | `autoGainRefreshTicks` at the eight rates: 59 / 64 / 29 / 43 as P5 | exact |
  | XO-alloc | 1e4 blocks of a driver with the stage and every band dirty at every refresh tick, inside the `malloc_logger` window | 0 allocations, 0 frees; SKIPPED line without a logger |
  | XO-cpu | INFO. Per-band evaluation (bell, 96 dB/oct cut, 96 dB/oct shelf) and per-refresh cost at 512 | printed beside the bench (1.5 / 6.1 / 12.1 / 1.7 µs) |

- `--neg ag-alloc`: a planted allocation inside the XO-alloc window is counted; exit 0 only if it is.
- No existing row is edited (P12).
- Depends on: T2, T3.

### Wave 2 — the processor

**T5. `Source/PluginProcessor.h/.cpp`: the stage, seeding, the meter** [agent]
- P5, P6, P7. Members: `eq::OutputStage outputStage`; `eq::AutoGain autoGain`; `agWanted`, `agOn`; `refreshTicks`; `outputPeak`. `prepareToPlay`: `outputStage.prepare (fs)`, `autoGain.prepare (fs)`, `refreshTicks`, `agOn = false` (seeding sets it on the first block).
- `processAudio` per P6; the step-5 and step-6 predicate is one private `stageIdleForBlock()` so the two uses cannot drift (memory `pattern_run_flag_past_dirty_check_latches.md`: the predicate is computed, never latched).
- The auto-gain estimator reads `bands[b].getDesired()` and `stereoLayout`.
- `processBlock` / `processBlockBypassed`: the P7 peak update after `processAudio`.
- The raw-value comment "output_gain (1) and auto_gain (3) are cached and read by nothing until Phase 2.5" and the per-block comment above `processBlock` are rewritten for 2.5 (the "Nothing else sits between …" paragraph now names the stage).
- Still no lock, no allocation, no listener, no `AsyncUpdater`, none of `juce::SmoothedValue`, `juce::dsp::Gain`, `juce::dsp::IIR`, `StateVariableTPTFilter`.
- Depends on: T2, T3.

**T6. Version 0.5.0** [agent]
- `CMakeLists.txt`: `OPEQ_VERSION 0.5.0`. Nothing else in the file changes.
- `CHANGELOG.md`: `## [0.5.0] - <execute date>` (bracket form). Added: output gain (±36 dB) and auto gain, both smoothed without clicks and in effect from the first sample of a session; auto gain follows the curve, not the programme, so renders and null tests repeat exactly. Known in this build: a steep low cut switched in under sustained bass can thump at a slope change (VERIFICATION-2.4 Issue 1, under listening).
- Depends on: T5.

### Wave 3 — render-harness rows

**T7. `tests/render-harness/main.cpp`** [agent]
- New section heading for 2.5; the row index in the file header extended. `hardGainRender` and `gainRefRender` fed the plugin's own float output with the gains neutral, widened, compared in double. The S5 amendment of P12, with a comment naming this plan.

  | Row | Check | Threshold |
  |---|---|---|
  | XR-g12o | G12 output gain: −36, −12, 0, +12, +36 dB, 1 kHz sine, 48 kHz, read after 100 ms: RMS ratio | within 1e-4 dB of target (float output); limit ±0.05 dB |
  | XR-g12s | G12 gain scale: ±6 dB bell at 1 kHz, `gain_scale` 200 % | ±12 dB within the G1 tolerance |
  | XR-g12a | G12 auto gain: +12 dB Low Shelf 200 Hz, 12 dB/oct, `auto_gain` on; band-limited pink 40 Hz – 16 kHz, FFT-shaped, 12 s, seeded; RMS from 1 s on | output RMS / input RMS within ±0.5 dB; the printed estimate −6.167 dB ± 0.001 |
  | XR-g12d | Two renders of XR-g12a | bit-identical |
  | XR-g12n | XR-g12a with `auto_gain` off | outside ±0.5 dB (≈ +6.2 dB) |
  | XR-gstep | P11-style stimulus (2.4: L −6 dBFS, R −10.5 dBFS, 1.1 rad), 50 Hz and 1 kHz, blocks 32 / 512: the 20 output-gain pairs; `auto_gain` toggled with a static band set whose estimate is −18, −6, +6, +12 dB (the harness picks the bands and prints their estimate); one Bell +6 dB running in every row so the band path is live | (a) ≤ 2.0x; 1 kHz κ ≤ 2.5x |
  | XR-gsteph | `hardGainRender` on the same | ≥ 10x every row at block 512 |
  | XR-gret | The reversal cases through `processBlock` (output gain, blocks 32 / 512) | ≤ 2.0x |
  | XR-gsweep | Output gain −30 → +30 dB in 20 ms and back, blocks 32 / 512, against `gainRefRender`'s per-sample automation; auto gain on while a Bell 1 kHz Q 1 sweeps −12 → +12 dB in 20 ms, against the same render with the auto-gain trajectory replaced by `gainRefRender` driven by `agRef` targets at the same refresh ticks | ≤ 4.0x each; the auto-gain row has no bench figure: a reading above 4.0x is the STOP of T8, not a retune |
  | XR-ref | Every XR-gstep / XR-gret render against `gainRefRender` × the gain-neutral output | ≤ −120 dB re the output peak |
  | XR-g4a | G4 (a) with the stage: no band, output 0 dB, `auto_gain` **off** and **on**; blocks 1, 16, 64, 512, 1024, 4096 and the random sequence; mono and stereo; the denormal tail included | `memcmp` equal |
  | XR-g4an | Output gain +0.001 dB; and `auto_gain` on with one Left-only +1 dB bell | each must differ |
  | XR-idle | No band: output 0 → +6 → 0 dB | `memcmp` equal to the input from 60 ms after the second edit, denormal tail included (bench snap 45 – 52 ms) |
  | XR-seed | (i) prepare at output +12 dB with `auto_gain` on and one +12 dB Low Shelf: from the first sample, output = (render at 0 dB / auto off) × the constant `10^(12/20) · agRef`, sample by sample within float rounding; (ii) one band, bypass on, after the fade change output gain, toggle auto gain, edit the band; un-bypass by the parameter and by `processBlockBypassed`: from un-bypass + N equal to a fresh instance on the edited parameters started at the un-bypass sample | (i) ≤ −120 dB re peak; (ii) `memcmp` equal |
  | XR-seedn | (ii) against a fresh instance on the parameters from before the bypass | must differ |
  | XR-gbyp | No band, output +12 dB: bypass toggled on and off, both routes | the toggle fades: exactly N samples of mixing each way (equal to the harness mix of the gained and dry signals with the re-stated weights, ≤ −120 dB); then `memcmp` equal to the input while bypassed |
  | XR-unb | **VERIFICATION-2.4 Issue 2.** One band (P13). Bypass on; during the fade to bypass, a structural edit and a gain edit to the band; un-bypass 5 ms into the fade. By the parameter and by `processBlockBypassed`; blocks 32 / 512. (i) From the end of the fade back (e + N + N): equal to a fresh instance on the edited parameters started at e + N (PX14). (ii) Over [start, e + 2N]: equal to the harness mix of a held processed render and the dry input with the re-stated weights, the second fade starting at e + N exactly (PX9) | (i) `memcmp` equal; (ii) ≤ −120 dB re peak |
  | XR-unbn | (i) against the pre-edit parameters; (ii) with the second fade starting one sample late | each must differ |
  | XR-blk5 | B1 / XR-blk extended: 4 bands, 160 events on multiples of 2048 redrawing band parameters, `output_gain` (p 0.3) and `auto_gain` (p 0.1), refreshes in flight; blocks 32 / 64 / 512 / 2048 at 44.1 / 48 / 96 / 192 kHz | bit-identical per rate; refresh and evaluation counts printed; differs from the render without events |
  | XR-meter | `readAndResetOutputPeak`: a one-sample +6 dBFS spike in one 32-sample block among 50 between two reads is reported (2.0 ± 1 ulp); the read resets to 0; the bypassed and untouched paths update it; mono has channel 0 only | as stated |
  | A4 | G6 with the stage: the `malloc_logger` window around `processBlock` / `processBlockBypassed`, 20 s at 48 kHz, `auto_gain` on, 24 bands moving at every block (every refresh dirty), output gain redrawn per block, bypass toggled, first block included | 0 allocations, 0 frees; liveness probe |
  | A4n | A planted allocation inside the window | counted |
  | S5 | Amended (P12) | as amended |
  | X-cpu4 | INFO. X-cpu3's four cases with `auto_gain` on; plus 24 shelves × 96 dB/oct sweeping with auto gain on | printed beside X-cpu3 and the bench's +1.4 % |

- Depends on: T5.

### Wave 4 — build, gates, install, validators

**T8. Build and run** [run]
- Done when, in order:
  1. (BG) `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` (version bump), then `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness > $S/build.log 2>&1` → rc 0; `grep "O-ParametricEQ/.*warning:" $S/build.log` → no lines.
  2. (BG) `$H > $S/designer.log 2>&1` → **0 FAIL**. The 114 / 16 lines of `$S/designer-base.log` print the same figures (P12); every T4 row ID appears.
  3. `$H --neg ag-alloc` → rc 0. The ten existing `--neg` controls → rc 0 each.
  4. (BG) `$R > $S/render.log 2>&1` → **0 FAIL**. Against `$S/render-base.log`: all 100 rows present; rows other than S5 print as in the baseline; every T7 row ID appears; every `…n` row reports that its control fired.
  5. (BG, one at a time) `$H --long > $S/designer-long.log 2>&1` and `$R --long > $S/render-long.log 2>&1` → 0 FAIL each; S5 prints 600 s per rate.
  6. `python3 tests/golden/check_spec.py` → `OK:`; `$R --dump-params | diff - tests/golden/params.tsv` → empty.
  7. Static gates, from the repo root:
     - `! grep -rnE "juce|Juce|std::vector|std::function|new |malloc|throw " plugins/O-ParametricEQ/Source/dsp`
     - `! grep -rnE "fast-math|Ofast|fp:fast|ffp-contract" plugins/O-ParametricEQ/CMakeLists.txt`
     - `! grep -rnE "IIR::|StateVariableTPT|dsp::Gain|SmoothedValue|AsyncUpdater|callAsync|CriticalSection|std::mutex|addParameterListener" plugins/O-ParametricEQ/Source/PluginProcessor.h plugins/O-ParametricEQ/Source/PluginProcessor.cpp`
     - `git diff --stat HEAD -- plugins/O-ParametricEQ/Source/dsp` lists `OutputStage.h` and `AutoGain.h` only (P1), or the summary records the deviation.
     - `grep -n "std::log\|std::exp\|log10\|pow" plugins/O-ParametricEQ/Source/dsp/AutoGain.h`: only in `prepare`, the final dB conversion and the clamp guards — none inside the per-point product loop (linear power, RESEARCH P4).
     - `grep -n "stageIdleForBlock" plugins/O-ParametricEQ/Source/PluginProcessor.cpp`: the definition and exactly two calls (steps 5 and 6).
     - Read the bodies of `gainRefRender`, `hardGainRender` and `agRef` in both harnesses: none of `GainSmoother`, `OutputStage`, `AutoGain` appears in them.
- **On a failure:**
  - An XO row red → the stage or the estimator. Fix in `OutputStage.h` / `AutoGain.h`, rebuild, rerun 2 – 7 in full.
  - XO rows green and a render row red → the processor glue or the row's float scope. Fix the glue. A threshold, row list or stimulus level is not changed in this session; if a row cannot pass as specified with the XO rows green, STOP and escalate with the figures.
  - A control that does not fire → the row is vacuous. Fix the row, not the control.
  - An existing row changes outside P12 → STOP.
  - XR-g4a red → the idle predicate (P6). Do not loosen G4 (a); it is the contract this phase must not break.
- Depends on: T4, T6, T7.

**T9. Install and validate** [run]
- One validator at a time, each BG with its own log.
  1. `./scripts/build-and-install.sh O-ParametricEQ > $S/install.log 2>&1` → rc 0; exactly the two `-dev` bundles in the system folders, version 0.5.0.
  2. pluginval, strictness 10, `--timeout-ms 600000`, VST3: **2 runs** → `SUCCESS`. AU: 1 run → `SUCCESS`.
  3. `auval -v aufx OuPQ OuDv` → `AU VALIDATION SUCCEEDED`; channel capabilities still `[1, 1] [2, 2]`. Never `auval -a`.
  4. Standalone smoke as in 2.2 T9 (rebuild the Standalone target first) → `ALIVE`.
- Any failure: fix the root cause in `Source/`, then rerun T8 and T9 in full. A pluginval NaN or `inf` report is a finding about stacked gain (2.2 R3; now including +36 dB output and +12 dB auto gain): record the configuration, insert nothing in the signal path.
- Depends on: T8.

### Wave 5 — bookkeeping + commit

**T10. Summary, STATUS, commit** [agent writes the summary; run does the rest]
- `.planning/stages/2-dsp/SUMMARY-2.5.md`: tallies of both harnesses (default and `--long`); XO-step / XR-gstep per gain and tone beside the bench (P4 and the printed LIN); XO-ret / XR-gret; XO-sweep / XR-gsweep (the auto-gain sweep figure is new — state it); XO-snap per rate; XO-ag / XR-g12a errors; XO-agrip at 512 beside the 64-point control; XR-g4a / XR-idle / XR-seed / XR-gbyp / XR-unb results; XR-blk5 counts; S5 per rate; A4 counts; XO-cpu / X-cpu4; controls; validator results; the existing rows that write the gains and what they read; deviations.
- Carry-forward: for 2.6: the listen path goes between the bands and this stage; G6 with the analyzer on; for Stage 3: the meter reader (`readAndResetOutputPeak` on the editor timer, `VUMeterBridge` fed from the message thread); for verify: hands-on (auto gain on while enabling a +12 dB shelf — loud for about the smoother's time, then pulled back; sweeping a narrow +12 dB bell with auto gain on; output gain automation), the `--long` runs once more, a mutation run over `OutputStage.h`, `AutoGain.h` and the step-5 / step-6 predicate; VERIFICATION-2.4 Issue 1 still open.
- `REQUIREMENTS.md`: no status edit (requirements close at verify).
- `PLUGINS.md`: version cell → 0.5.0. Check `git diff PLUGINS.md` first; if other sessions' rows are modified, use the temp-index recipe (memory `index_git_shared_checkout.md`) or leave it out and say so.
- STATUS: phase rows, `status: stage_2_phase_2_5_executed`, `next_action: stage_2_verify_phase_2_5`, files list.
- Commit, path-scoped, on `main`. Immediately before: `git branch --show-current`, `git status --short`. `git add` the new files by name (`Source/dsp/OutputStage.h`, `Source/dsp/AutoGain.h`, `.planning/stages/2-dsp/SUMMARY-2.5.md`), then `git commit -- plugins/O-ParametricEQ`. Never `-A` / `-a`. `git log -1 --stat` lists only this plugin's paths. No tag.
- Depends on: T9.

---

## Dependency graph

```
T1 ─┬→ T2 ─┬→ T4 ────────────────────┐
    └→ T3 ─┤                          │
           └→ T5 → T6 ──┐             │
                └→ T7 ──┴─────────────┴→ T8 (build, harnesses, controls, --long) → T9 (install, validators) → T10
                                          ↑   │
                                          └───┘ fix → rebuild → rerun T8 in full (and T9 if Source changed)
```

T2 and T3 are independent. T4 and T5 / T7 touch different files; one build covers all of them.

## Success criteria

| Criterion (ROADMAP Phase 2.5; ARCHITECTURE G3, G4, G12 as amended) | Proven by |
|---|---|
| G12 output gain ±0.05 dB at −36 … +36 dB (FUNC-05) | XR-g12o, XO-unity |
| G12 gain scale 200 % → ±12 dB | XR-g12s |
| G12 auto gain within ±0.5 dB on band-limited pink; two renders bit-identical (FUNC-06) | XR-g12a, XR-g12d, XR-g12n, XO-ag, XO-agref |
| Auto gain does not pump on a swept narrow boost (C1) | XO-agrip |
| Output-gain steps and reversals, auto-gain toggle pass G3 step form | XO-step, XO-steph, XO-ret, XR-gstep, XR-gsteph, XR-gret |
| Output-gain sweep and auto gain following a gain sweep pass G3 sweep form | XO-sweep, XR-gsweep |
| The stage does the specified smoothing; it cannot overshoot or reach zero | XO-ref, XO-refn, XR-ref, XO-stress, XO-snap |
| Offline and real-time-sized blocks give bit-identical auto gain | XR-blk5, XO-agid |
| G4 (a) still passes with `auto_gain` off (and on) (QUAL-01) | XR-g4a, XR-g4an, XR-idle |
| Seeded: no fade-in at session open; edits made while bypassed are in effect at un-bypass | XR-seed, XR-seedn |
| Bypass with only the gain off unity fades | XR-gbyp |
| VERIFICATION-2.4 Issue 2 (PX14, PX9) | XR-unb, XR-unbn |
| Meter keeps the peak between reads | XR-meter |
| G5 with the gains randomised (DSP-04, DSP-06) | S5 (amended); `--long` once |
| G6 with the estimator refreshing (PERF-01) | XO-alloc, A4, A4n, `--neg ag-alloc` |
| Stage 1 – 2.4 rows, layout golden unchanged outside P12 | T8.2, T8.4, T8.6 |
| 0.5.0 validates | T9 (pluginval 10 ×3, `auval`, Standalone) |

## Risks

- **R1 The idle predicate is the whole of G4 (a).** A stage that runs a unity multiply on a lane it did not need flushes the denormal tail and fails XR-g4a; a predicate that skips when it should not leaves output gain silently unapplied. One function, used twice (T5), computed per block, never latched. XR-g4a / XR-g4an / XR-idle / XR-gbyp are its rows.
- **R2 The pure-function invariant (P4) carries block identity.** If the estimator ever depends on when a refresh was skipped (cache state leaking into the result, e.g. a running sum updated incrementally), XR-blk5 at block 32 against 2048 shows it. XO-agid gates it directly. Recompute the lane products from the cached vectors at each refresh; do not maintain them incrementally.
- **R3 Seeding at the step-7 restart (mid-block).** The latched un-bypass restarts inside a segment loop; the stage must be snapped there too, before the next sample. XR-unb at block 512 is the row that sees it.
- **R4 The auto-gain sweep row has no bench figure.** 4.0x is the G3 sweep-row limit. A miss with XO-ref and XR-ref green is a finding: STOP (T8), do not move the limit.
- **R5 Seeding cost.** `evaluateAll` designs up to 24 bands in one block (worst ≈ 290 µs, 24 × 96 dB/oct shelves) at session open and at un-bypass only. INFO in X-cpu4; not a G10 row.
- **R6 Stacked gain.** +36 dB output and +12 dB auto gain on top of 24 boosting bands can overflow the float output. That is legitimate output (G5 peak clause); S5's guard grows by the drawn gains. pluginval reports of `inf` are recorded, not limited.
- **R7 The meter's read-modify-write is not atomic.** One writer; a reader's `exchange (0)` between the writer's load and store keeps a peak for one more poll. Acceptable (RESEARCH P5); stated in the header comment, not "fixed" with a CAS loop.
- **R8 Gate tokens in prose.** T8.7 greps `Source/dsp` comments: no "new ", "throw ", "malloc", "juce" in the two new headers' comments. "SmoothedValue" may not appear in the processor even in a comment (write "no framework smoother").
- **R9 Run time.** XO-agref (2000 configurations × 5 rates), XO-agrip (4000 steps × 8 bells), XR-gstep, S5 `--long`. Everything BG, heavy rows threaded. Do not cut counts.
- **R10 Shared checkout.** Other sessions have uncommitted files in the tree. Path-scope the commit and re-check immediately before it (T10).
