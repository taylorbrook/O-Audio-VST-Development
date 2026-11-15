# O-ParametricEQ Stage 2 (DSP), Phase 2.5: Research (output stage and auto gain)

**Researched:** 2026-11-14
**Scope:** ROADMAP Phase 2.5: `eq::OutputStage` (output gain, auto gain, seeded ramps), `eq::AutoGain` (the curve-derived estimator), peak atomics for the output meter. Gates G12, G3 for the gains, G4 (a) with the stage in place, offline / real-time bit identity of the auto gain. Carried in from VERIFICATION-2.4: Issue 2 (a render row for the un-bypass latched during the fade to bypass, PX14 / PX9) and Issue 1 (the stop-band burst, still waiting on the listen). No analyzer, display model or listen (2.6).
**Method:** a scratch bench over the real `Source/dsp` headers, `stages/2-dsp/research-2.5/gain_bench.cpp` (output: `research-2.5/bench-output.txt`). It renders gain trajectories on a tone and scores them with G3's own measures, and it runs the auto gain estimator on `eq::Designer::design` output against a dense pink-noise truth. Numbers marked [BENCH] are from that bench on this machine (Apple M4 Max, clang -O3, scalar, double). Statements about the processor are from reading `Source/PluginProcessor.cpp` at `42847130` and are marked [ARGUED]. Nothing in `Source/`, the harnesses, the model or the goldens was changed.
**Contracts read:** ARCHITECTURE.md § 6, § 7, "Processing Chain", "Auto gain", G3, G4, G12; REQUIREMENTS FUNC-05, FUNC-06; Stage 2 CONTEXT; VERIFICATION-2.4; `modules/metering/vu-meter` 1.0.0.

## Summary

1. **The gain ramp shape is decided by G3 at 50 Hz, and a linear ramp fails it.** A 20 ms output-gain step from +36 to −36 dB reads 25x the louder held render on the largest second difference with a linear ramp, 188x with the band pattern (dB on the control grid, amplitude interpolated per sample). The corner of a linear ramp multiplies the tone by a slope step that a 50 Hz sine cannot hide. Raised-cosine and S-shaped trajectories read 1.0x to 1.6x. [BENCH]
2. **Recommended: a cascade of four one-pole smoothers in amplitude** (call it P4), time constant set so a step is within 1e-3 of its size at 20 ms (100 ms for auto gain), snapped to the target when all four stages are within 1e-9. Every output-gain step pair ≤ 1.56x at 50 Hz, ≤ 1.00x at 1 kHz; a second change inside the ramp ≤ 1.58x; it can never leave the range of its targets or reach zero (its impulse response is positive, so the output is a weighted average of past targets); automation sweeps track within 0.05 dB. Exact at the target 45 – 52 ms after an output-gain step and 222 – 232 ms after an auto-gain step. [BENCH]
3. **The three alternatives that pass a single step fail elsewhere.** A raised cosine restarted on a retarget kinks (up to 38x); latched, it lags up to 40 ms and turns a sweep into 20 ms stairs. A cubic Hermite that carries its slope overshoots on dense automation and drives the gain through zero (141 064 non-positive samples in 200 s of random automation); clamping its slope puts the kinks back (17x). [BENCH]
4. **The estimator grid must be denser than 64 points.** As specified (64 log points, inclusive ends), the G12 case is 0.07 dB off and passes, but a narrow boost swept in frequency makes the auto gain wobble as it passes between grid points: 0.88 dB peak-to-peak for a +12 dB Q 40 bell, 8.7 dB for +30 dB Q 10. With **512 points at cell midpoints** the +12 dB rows fall to ≤ 0.03 dB and the G12 case is exact to 0.000 dB; only +30 dB bells at Q ≥ 20 keep a ripple (1.1 – 2.9 dB p-p). [BENCH] This changes ARCHITECTURE "Auto gain" (call C1).
5. **Cost is small at 512.** Per band whose target changed: design + |H|² product over the grid 1.5 µs (bell) to 12 µs (96 dB/oct shelf). Per refresh, the product of 24 cached vectors for two lanes, the mean and the log: 1.7 µs. Worst case, 24 shelves at 96 dB/oct all moving, refreshed every 1024 samples at 48 kHz: about 290 µs per 21 ms, 1.4 % of a core. [BENCH]
6. **Linear power vectors, not log-power.** ARCHITECTURE caches ln |H|² per band and sums. Caching |H|² and multiplying needs no `log` / `exp` per point, gives the same number, and underflows only where the true power is below 1e-300 (where it contributes nothing to the mean). [BENCH, ARGUED]
7. **The stage changes the processor's fast path in four places.** With output gain or auto gain off unity, "no running band" no longer means "do not touch the buffer"; every lane must be converted; a bypass toggle must fade; and the stage must snap at the first block and at un-bypass like the bands do. With output gain 0 dB and auto gain off (or on with nothing to compensate) the stage is skipped and G4 (a) holds unchanged. [ARGUED]
8. **The vu-meter bridge keeps only the last block.** `VUMeterBridge::storeLevels` overwrites a dB value per block; a 30 Hz poll of 32-sample blocks reads one block in about 46 and misses the peaks between. Keep a per-channel max in the processor, reset by the reader (Stage 3), and feed the bridge from the message thread. [ARGUED]
9. **Two harness rows are owed from 2.4**, and both now pass through code 2.5 touches: the un-bypass latched during the fade to bypass (PX14) and the late start of a latched opposite fade (PX9). The stop-band G3 row (Issue 1) waits for the listen. [ARGUED]

One call is Taylor's (C1). The rest are the plan's.

---

## Q1. Which gain trajectory is click-free? (G3 for the gains; FUNC-05, FUNC-06)

`gain_bench steps | retarget | sweep | stress`. 48 kHz, a tone at −6 dBFS (0.5), 50 Hz and 1 kHz, the gain applied in double and rounded to float as the plugin's output is. Edits sit on the 16-sample grid, eight tone phases per row, worst reported. Scores are G3's step measures: (a) largest second difference over the change ± 50 ms, divided by that of the louder held render (limit 2.0); (b) local κ at 1 kHz over the larger held κ (limit 2.5). HARD (the target at once) is the negative control.

Shapes: LIN linear in amplitude; DBG linear in dB on the control grid with amplitude interpolated per sample (the Phase 2.3 band-ramp pattern); RC raised cosine in amplitude (the `Fade.h` weight), restarted on a retarget; RCL the same, a retarget latched until the ramp ends; HER cubic Hermite in amplitude, carrying the current slope into a retarget; HERM the same with the carried slope limited to 3Δ / N (monotone); HERG HERM plus a limit on a falling slope carried into a rise; HDB cubic Hermite in dB; P2 / P3 / P4 a cascade of 2 / 3 / 4 one-poles.

### A single step

| Shape | Output gain 20 ms, 20 pairs of −36 / −12 / 0 / +12 / +36 dB: 50 Hz (a) | 1 kHz (a) | 1 kHz κ | Auto gain 100 ms, 0 ↔ −18 / −6 / +6 / +12 dB: 50 Hz (a) |
|---|---|---|---|---|
| LIN | **25.1** (+36 → −36) | 1.06 | **239** | **5.21** |
| DBG | **188** (+36 → −36) | 1.47 | **2.97** | **11.0** |
| RC | 1.24 | 1.00 | 2.17 | 1.00 |
| HER | 1.15 | 1.00 | 2.13 | 1.01 |
| HDB | **2.24** | 1.00 | 2.42 | 1.01 |
| P2 | **3.12** | 1.01 | **20.1** | 1.07 |
| P3 | 1.74 | 1.00 | 2.05 | 1.02 |
| **P4** | **1.56** | **1.00** | **2.22** | **1.01** |
| HARD | 23 175 | 59.4 | 232 611 | 20 239 |

Why linear fails: the output is g(n) x(n), so its second difference carries g''x + 2g'x' + gx''. A linear ramp has g'' infinite at its two corners: the step in slope times the tone's level. At +36 → −36 dB in 20 ms that slope is 0.066 per sample; the held render's own second difference at 50 Hz is 63 × 0.5 × (2π 50 / 48 000)² ≈ 1.4e-3. Any trajectory with bounded g'' passes. The auto-gain toggle fails with LIN for the same reason at a fifth of the slope.

### A second change inside the ramp

Largest second difference over the louder of the three held renders, 50 Hz, worst over eight phases, the second change at 25 / 50 / 75 / 90 % of the first ramp (eight cases; the 1 kHz rows are all ≤ 1.01x except DBG and LIN):

| Case | RC | RCL | HERM | HERG | P2 | P3 | **P4** |
|---|---|---|---|---|---|---|---|
| 0 → +36 → −36 | **37.8** | 1.30 | 1.17 | 1.17 | **3.15** | 1.75 | **1.58** |
| +36 → −36 → +36 | **37.4** | 1.24 | 1.18 | **15.9** | **3.12** | 1.74 | **1.56** |
| +36 → −36 → 0 | **37.3** | 1.12 | **17.0** | **17.0** | **3.12** | 1.74 | **1.56** |
| 0 → +12 → +18 | **14.5** | 1.11 | 1.10 | 1.10 | 1.05 | 1.05 | **1.05** |
| worst of the 32 rows | 38.3 | 1.30 | 17.0 | 17.0 | 3.15 | 1.75 | **1.58** |

### Automation, delivered at block starts

The automation (dB, linear in time) is sampled at each host block start and handed over on the next tick; the reference applies it per sample. (a) is against the reference's own second difference. "Lag" is how far the gain is from the automation 20 ms after the sweep ends.

| Shape | −30 → +30 → −30 dB in 20 ms: (a) 50 Hz, block 32 / 512 | lag at block 512 | in 1 s: (a) 50 Hz, block 512 | 200 s random automation, any block 1 – 2048: range of the gain |
|---|---|---|---|---|
| LIN | 0.13 / 0.43 | 1.19 dB | **2.64** | −36 … +36 |
| RC | 0.05 / 0.20 | 0.37 dB | **4.06** | −36 … +36 |
| RCL | 0.02 / 0.02 | **24.1 dB** | 0.77 | −36 … +36 |
| HER | 0.02 / 0.02 | 0.40 dB | 0.77 | **−105 … +36.6, 141 064 samples ≤ 0** |
| HERM | 0.02 / 0.02 | 0.40 dB | 0.77 | **−105 … +36.3, 30 321 samples ≤ 0** |
| HDB | 0.02 / 0.02 | 1.10 dB | 0.77 | −41.8 … **+40.7** |
| **P4** | **0.03 / 0.03** | **0.03 dB** | **0.78** | **−36 … +36** |

At 50 ms per sweep HER overshoots by 66.8 dB at block 32. A slope carried into many short retargets extrapolates: each new Hermite starts at the previous slope over a 20 ms span, so the gain swings past its target and through zero. The monotone clamp stops the same-direction overshoot but not the dip of a reversal near the bottom.

### Recommendation (P1, P2 for the plan)

**P1. Both gains use a four-stage one-pole cascade in linear amplitude.** Per sample, with a = 1 − exp(−X / N), X = 13.0622 (the value at which a four-pole step response is within 1e-3 at N):

```
y1 += a (T - y1);  y2 += a (y1 - y2);  y3 += a (y2 - y3);  y4 += a (y3 - y4);  gain = y4
```

N = round (0.020 fs) for output gain, round (0.100 fs) for auto gain. When all four stages are within 1e-9 of T, all four are set to T exactly and the smoother is settled: from then on the gain is T, and when T is exactly 1.0 the stage does nothing. Why this one:

- **Steps and reversals pass G3 with room** (1.56x, 1.58x at 50 Hz; 1.00x at 1 kHz; κ 2.22x).
- **It cannot overshoot or reach zero.** Its impulse response is positive and sums to 1, so the gain is a weighted average of past targets and stays inside their range whatever the automation does. Every other S-shape with carried state failed this in the stress run.
- **No latch, no restart rule.** A retarget only changes T; the state carries everything. Sweeps lag 0.03 dB.
- **Exact at the target in finite time:** 45 – 52 ms after an output-gain step, 33 ms for a 0.01 dB move, 222 – 232 ms after an auto-gain step, at every rate (the time constant is set in samples from N). [BENCH]
- **Cost:** 4 multiply-adds per smoother per sample, shared by the lanes; nothing while settled.

Wording consequence: ARCHITECTURE says "20 ms ramp" and "100 ms ramp". With P4 the gain is within 1e-3 of the step at 20 / 100 ms and exact at about 50 / 230 ms. The plan should amend § 6 and "Auto gain" to say so; it is not a user-visible change.

**P2. The two gains are separate smoothers, applied as one multiply.** `g = agSmoothed * ogSmoothed` per sample, then `lane[i] *= g` for each converted lane. The order of the product is fixed, so the result is deterministic; one rounding instead of two.

Targets: `output_gain` raw → sanitised (non-finite → 0 dB) and clamped to ±36 dB → `std::pow (10.0, dB / 20.0)`, which is exactly 1.0 at 0 dB. The auto-gain target is 1.0 exactly with `auto_gain` off. Both are read at the block start with the band snapshots and handed to the smoother on the next control tick, as a band target is (block-size independence, Phase 2.3 rule).

---

## Q2. Where the stage sits, and what changes in the processor (FUNC-05, QUAL-01)

ARCHITECTURE's chain: bands → (listen, 2.6) → auto gain → output gain → global soft bypass → float. The bypass mix is the last stage today and stays so; the gain goes between the bands and the bypass mix, inside each segment of step 6. [ARGUED]

Four places in `processAudio` assume "no running band ⇒ buffer untouched", and each must change:

1. **Step 5, the empty list.** Today: no running band → `bypassFade.jump` and return. With the stage, the return is allowed only when the stage is also **idle**: both smoothers settled, output target exactly 1.0, auto-gain target exactly 1.0, and nothing pending that could change a target before the next block start (an output-gain edit seen at this block start, or the auto-gain refresh with a dirty estimate and `auto_gain` on). Otherwise the block runs with an empty band list.
2. **Step 6, the lanes.** When the stage is not idle, every lane of the layout is converted, not only the lanes a band touches. A lane converted for gain loses its denormal samples under flush-to-zero, which is correct: its samples are multiplied.
3. **The bypass toggle with no band running.** Today it is a flag (nothing differs between processed and dry). With a gain off unity it differs, so the toggle must fade. This falls out of (1): the "nothing running" jump applies only when the stage is idle too.
4. **Seeding.** On the first block after a prepare and on the restart at un-bypass (step 4 restart, and the step 7 restart inside a block) the bands start settled; the stage must too: output smoother snapped to its target, auto-gain estimate computed for every band at once and its smoother snapped. Without this, a session that opens at +12 dB output fades in from 0 dB (the suite lesson that gave ARCHITECTURE § 6 its "not `juce::dsp::Gain`" line), and an edit made while bypassed ramps after un-bypass instead of being in effect.

The non-finite check (step 8) needs no change: the stage holds no audio state, and its targets are sanitised. The stacked-gain overflow of the float output (bands + auto gain + output gain) is still possible and still not a non-finite double; G5's stack guard has to include both gains (Q5).

The segment loop keeps its shape: per segment, after the bands and before the bypass mix, the stage runs over the segment's samples. The auto-gain refresh is a control-tick event (Q3), so it falls at a segment start like the band ticks.

**P3.** The stage is a plugin-local `eq::OutputStage` header with: `prepare (fs)`, `snap (ogTarget, agTarget)`, `setOutputTarget`, `setAutoTarget` (both taking effect at the call, which the processor makes on a tick), `isIdle()`, `process (double* L, double* R, int n)` (R may be null). No JUCE type, no allocation, so the designer harness can drive it as it drives `SlotBand`.

---

## Q3. The auto gain estimator (FUNC-06)

### What it computes

As ARCHITECTURE "Auto gain", with two changes (grid, linear domain):

```
grid:   f_i = exp (ln 40 + (ln fTop − ln 40)(i + 0.5) / M),  i = 0 … M−1,  M = 512,  fTop = min (16 kHz, 0.45 fs)
        t2_i = tan² (π f_i / fs)                         (once per prepare)
band b: P_b[i] = Π over its sections of sectionMag2 (t2_i, section)      (ResponseEval.h; its target design)
lanes:  PL[i] = Π P_b[i] over Stereo, Mid, Left bands;  PR[i] = Π P_b[i] over Stereo, Mid, Right bands;  mono: all bands
AG_dB = −10 log10 ( mean_i 0.5 (PL[i] + PR[i]) ),  clamped to [−18, +12]
```

- **Target design.** A band's contribution is `designer.design (band.getDesired().settings)` of its latest request, not the ramping design: the estimate does not chase the 20 ms band ramp, and the 100 ms smoother follows it anyway. A band whose desired input is inactive, or a gain type whose effective gain is exactly 0 dB (the sleeping case), contributes nothing and is not evaluated. With no contributing band the target is exactly 1.0 and nothing is evaluated. The dynamic offset (v1.x) is not in the target; a dynamic EQ should not move the auto gain.
- **Guards.** A mean that is not finite, or ≤ 1e-30, gives the +12 dB clamp (everything cut). A non-finite design cannot occur (DSP-06), but the clamp of a NaN is defined: treat as 0 dB.

### Accuracy against pink noise

`gain_bench ag`. Truth: the band-limited pink power ratio of the same digital design, a 40 000-point midpoint mean over 40 Hz – 16 kHz. Error of the estimate in dB (est − truth), identical to three decimals at 44.1, 48, 96 and 192 kHz:

| Band | Truth AG | 64 inclusive (as specified) | 64 midpoint | 128 | 256 | **512** |
|---|---|---|---|---|---|---|
| **Low Shelf 200 Hz +12 dB, 12 dB/oct (G12)** | −6.167 | −0.069 | 0.000 | 0.000 | 0.000 | **0.000** |
| Low Shelf 200 Hz +12 dB, 96 dB/oct | −6.886 | −0.049 | −0.001 | 0.000 | 0.000 | 0.000 |
| High Shelf 8 kHz +12 dB, 12 dB/oct | −3.009 | −0.167 | +0.001 | 0.000 | 0.000 | 0.000 |
| High Shelf 15 kHz +12 dB Q 4, 96 dB/oct | −1.458 | **−1.344** | **+0.662** | −0.181 | −0.011 | −0.003 |
| Bell 1 kHz +30 dB Q 10 | −7.506 | −0.021 | **−1.350** | **+0.852** | **−0.653** | +0.055 |
| Band Pass 1 kHz Q 10 | +15.950 (clamped to +12) | −0.111 | −0.227 | +0.001 | 0.000 | 0.000 |
| Bells Q ≤ 2, cuts, notch, tilt (9 rows) | | ≤ 0.050 | ≤ 0.001 | 0.000 | 0.000 | 0.000 |

G12's ±0.5 dB passes on every grid. The single-setting errors are luck: what matters is how the estimate moves when the user moves a narrow band.

### Ripple under a frequency sweep

`gain_bench ripple`. A bell swept 200 Hz → 8 kHz in 4000 log steps at 48 kHz, midpoint grids; peak-to-peak of (estimate − truth) in dB:

| Bell | 64 | 128 | 256 | **512** | 1024 |
|---|---|---|---|---|---|
| Q 40, +30 dB | 11.9 | 8.8 | 5.7 | **2.92** | 0.86 |
| Q 40, +12 dB | **0.88** | 0.42 | 0.16 | **0.028** | 0.001 |
| Q 40, +6 dB | 0.19 | 0.08 | 0.02 | **0.002** | 0.000 |
| Q 20, +30 dB | 11.0 | 7.3 | 3.8 | **1.12** | 0.10 |
| Q 10, +30 dB | **8.66** | 4.47 | 1.33 | **0.12** | 0.001 |
| Q 10, +12 dB | 0.54 | 0.10 | 0.003 | **0.000** | 0.000 |
| Q 4, +30 dB | 3.72 | 0.83 | 0.04 | **0.000** | 0.000 |
| Q 1 (any gain), cuts at any Q (−30 dB rows) | ≤ 0.05 | 0 | 0 | 0 | 0 |

The wobble is the grid aliasing a peak narrower than its spacing: 64 points over 8.6 octaves are 7.4 per octave, and a Q 40 bell is about 0.036 octave wide. When the peak sits on a grid point the estimate sees all of it, between points none. Sweeping a narrow boost with auto gain on then pumps the level by the table's figures. Cuts do not show it (a narrow dip removes almost no pink power).

**P4. M = 512, midpoint cells.** It brings every ±12 dB row to ≤ 0.03 dB and every Q ≤ 10 row to ≤ 0.12 dB. The residual is +30 dB at Q 20 – 40 (1.1 – 2.9 dB p-p), an extreme setting; 1024 points take it to 0.1 – 0.9 dB at about twice the cost (not benched; the per-band cost scales with the grid). The midpoint rule is also what makes the G12 case exact (the inclusive grid over-weights its two ends by half a cell).

### Schedule and determinism

- **Refresh** on control ticks where `tickIndex mod R == 0`, R = max (1, round (1024 / 48 000 · fs / K)) ticks: 59 at 44.1 / 88.2 / 176.4 kHz (944, 1888, 3776 samples), 64 at 48 / 96 / 192 kHz (1024, 2048, 4096), 29 at 22.05 kHz, 43 at 32 kHz. Per refresh: re-evaluate P_b for each band whose desired input changed since its last evaluation (compared by value), recompute PL / PR and the mean, retarget the smoother. Nothing at a refresh with nothing dirty.
- **Why bit-identical between offline and real-time blocks:** band requests are made only at block starts and the refresh sits on the tick clock, so a change seen at the same sample is evaluated at the same tick at any block size. The B1 rule (events on multiples of 2048) carries over unchanged. Only the audio thread runs it; there is no message-thread round trip.
- **`auto_gain` off:** nothing is evaluated; bands that change are only marked dirty. Turning it on evaluates the dirty bands at the next refresh tick (up to one refresh period, 21 ms), then the 100 ms smoother runs. Seeding (Q2.4) evaluates every contributing band at once: worst case 24 × 12 µs ≈ 290 µs inside one block, at prepare-time and at un-bypass only.
- **Memory:** 24 band vectors + 2 lane vectors + the t2 table, 512 doubles each, about 220 KB, sized in `prepareToPlay` (or fixed arrays in the processor object). No allocation on the audio thread.

### Cost

`gain_bench cost`, 48 kHz, -O3, scalar. Per band re-evaluation (design + product over the grid) and per refresh (two lanes × 24 cached vectors, mean, log):

| Grid | Bell | 96 dB/oct Low Cut | 96 dB/oct shelf (8 sections) | Refresh |
|---|---|---|---|---|
| 64 | 0.44 µs | 0.90 µs | 3.2 µs | 0.21 µs |
| 256 | 0.92 µs | 3.1 µs | 7.0 µs | 0.86 µs |
| **512** | **1.5 µs** | **6.1 µs** | **12.1 µs** | **1.7 µs** |

Worst case at 512: 24 shelves at 96 dB/oct all moving, every refresh: about 290 µs per 21.3 ms, **1.4 % of a core**, on top of X-cpu3's 11.2 % (24 shelves × 96 dB/oct always fading). The design call dominates; the band engine already does the same design on its ticks, but sharing it would tie the estimate to the ramp (rejected above).

### Approximations kept from ARCHITECTURE (not re-opened)

Centre-panned programme (Side bands ignored, Left / Right bands at half weight), equal weights on a log grid (pink), and the 100 ms smoother. The last one means a large structural change is uncompensated for its first tens of milliseconds: a +12 dB shelf enabled with auto gain on is loud for about the time the smoother needs, then pulled back (the band fades in 20 ms; the auto gain reaches −6 dB in about 100 ms). That is what a 100 ms auto gain is; it stays a hands-on item.

---

## Q4. The output meter (ARCHITECTURE § 6, Stage 3 mounts it)

`VUMeterBridge` 1.0.0 stores a dB value per call to `storeLevels` and the editor polls at 30 Hz. Called per block, the poll sees one block in every 1600 / blockSize (46 blocks at 32 samples, 3 at 512): peaks in the others are lost, and the bar flickers with the block size. The module's JS side applies its own attack / release to whatever it is given, so it needs a peak over the poll interval, not over the last block. [ARGUED]

**P5.** Two `std::atomic<float>` per processor, one per output channel (mono: one), holding the largest |sample| since the last read. The audio thread, at the very end of `processBlock` and `processBlockBypassed` (after `processAudio` returns, on every path including the fully bypassed and untouched ones): `peak = buffer.getMagnitude (ch, 0, n)`; `if (peak > slot.load (relaxed)) slot.store (peak, relaxed)`. The reader (Stage 3 timer) takes `slot.exchange (0)`. One writer, so the race between the writer's load and the reader's exchange only keeps a peak for one more poll. The bridge is then fed from the message thread. The meter reads the float output, so it shows the bypass fade and the dry signal as the host gets them. Cost: one pass over the block per channel, read-only, no `memcmp` row affected. The harness can read the atomics directly (a row: a +6 dBFS spike of one sample in a block of 32 between two reads is reported).

---

## Q5. Gates and rows for the plan

| Row | What | Expected [from the bench where marked] |
|---|---|---|
| G12 output gain | −36, −12, 0, +12, +36 dB, 1 kHz sine, settled: RMS ratio | ≤ 1e-6 dB from target (the gain is `pow (10, dB/20)` exactly); limit ±0.05 dB |
| G12 gain scale | 200 % on ±6 dB bells | ±12 dB within G1 tolerance (2.2 / 2.3 path, unchanged) |
| G12 auto gain | +12 dB Low Shelf 200 Hz, 12 dB/oct, band-limited pink 40 Hz – 16 kHz (FFT-shaped, ≥ 10 s): output RMS / input RMS | estimate −6.167 dB, error 0.000 dB [BENCH]; the measurement's own spread from finite noise is what uses the ±0.5 dB |
| G12 determinism | two renders of that row | bit-identical |
| G3 gain steps | output gain, the 20 pairs of −36 / −12 / 0 / +12 / +36; auto gain on / off with targets −18, −6, +6, +12 dB; 50 Hz / 1 kHz; blocks 32 / 512 | (a) ≤ 2.0x: P4 1.56x worst [BENCH]; (b) 1 kHz κ ≤ 2.5x: 2.22x [BENCH]; hard control (smoothing off) ≥ 10x: 59x at 1 kHz, 23 000x at 50 Hz [BENCH] |
| G3 gain retarget | the eight reversal cases at 25 – 90 % | ≤ 2.0x: P4 1.58x [BENCH] |
| G3 gain sweep | output gain −30 → +30 dB in 20 ms and back (ARCHITECTURE's sweep), and auto gain following a Bell gain sweep −12 → +12 dB in 20 ms with `auto_gain` on: against a per-sample reference | ≤ 4.0x: 0.03x [BENCH, output gain]; the auto-gain row is new and has no bench figure |
| G4 (a) with the stage | no band, output 0 dB, `auto_gain` **off** and **on**: `memcmp` equal, denormal tail included | the stage is idle in both |
| G4 (a) negative | output gain +0.001 dB: must differ | |
| Stage idle again | output gain 0 → +6 → 0 dB, no band: `memcmp` equal from about 50 ms after the second edit | 45 – 52 ms [BENCH, settle to snap] |
| Seeding | prepare with output +12 dB and `auto_gain` on with a +12 dB shelf: the first block is at the settled gain (equal to a render that started there); same after un-bypass with both changed while bypassed, by both routes | |
| Bypass with gain only | no band, output +12 dB, bypass toggled: the toggle fades (N samples), it is not a flag | |
| Block identity | B1 extended: `output_gain` and `auto_gain` events on multiples of 2048, auto-gain refreshes in flight, blocks 32 / 64 / 512 / 2048 at four rates | bit-identical |
| G5 | S5 / XF-g5 with `output_gain` and `auto_gain` randomised; the stack guard becomes stack + auto gain + output gain + 12 dB | 0 non-finite |
| G6 | A2 / XF-alloc / A3 with the estimator refreshing every refresh tick (24 bands moving) | 0 allocations |
| Meter | the max-hold atomics see a one-sample spike between reads at block 32; reset by the read | |
| **Issue 2 (owed from 2.4)** | bypass on, un-bypass inside the 20 ms fade, by the parameter and by `processBlockBypassed`: equal to a fresh instance from the end of the fade back (PX14); the opposite fade starts on the sample after the first one ends (PX9) | must fail with PX14 and PX9 planted |
| CPU (INFO) | X-cpu with `auto_gain` on and 24 shelves × 96 dB/oct sweeping | about +1.4 % [BENCH] |

Issue 4 from VERIFICATION-2.4 applies to the seeding and Issue 2 rows: after a multi-band restart the output equals one fresh instance only after the chain settles (about 237 ms with 24 bands), so those rows either use one band or compare against a chain of single-band instances.

The P4 smoother, the estimator and the meter accumulator are plain C++; the designer harness can gate them in double (steps, reversal, stress range, settle-to-snap, estimator vs truth, ripple at the P4 / M = 512 figures above) and the render harness gates them through `processBlock`.

### Issue 1 (stop-band burst), status

Not touched by 2.5 and not blocking it. The options stand as VERIFICATION-2.4 left them (pre-roll through `fadePrerollSeconds`, or a warm start for slope-only changes on a cut), each with a stop-band G3 row. If the hands-on listen happens before the 2.5 plan, the plan can carry the fix and the row; otherwise it stays open into 2.6.

---

## Calls

**C1 (Taylor's): the auto-gain grid goes from 64 points to 512.** The ARCHITECTURE figure (64) makes auto gain pump by up to 0.9 dB when a +12 dB Q 40 bell is swept, and by up to 8.7 dB for +30 dB Q 10. 512 points take that to 0.03 dB and 0.12 dB for 1.4 % of a core in the worst case. Recommendation: 512, midpoint cells. Alternative: 1024 (2.8 %), which also tames the +30 dB Q 20 – 40 rows.

For the plan (no call needed):

- P1 four-pole smoother for both gains; amend the "20 ms / 100 ms ramp" wording.
- P2 one multiply for both gains; targets sanitised, set on the tick.
- P3 `eq::OutputStage` header, harness-drivable.
- P4 estimator: linear power vectors, midpoint grid, target designs, refresh on the tick clock, dirty bands only, guards.
- P5 max-hold peak atomics per channel, read-and-reset by the editor; bridge fed from the message thread.
- The four processor changes of Q2 (idle predicate in step 5, all lanes when not idle, bypass fades when only the gain differs, seeding at first block and un-bypass including the step 7 restart).
- The Q5 rows, including Issue 2.
- Contract amendments: ARCHITECTURE § 6, § 7, "Auto gain" (grid, linear domain, smoother wording), G12 row text if the auto-gain sweep row is added; refresh the checksums in STATUS.
