# O-ParametricEQ Stage 2 (DSP), Phase 2.3: Research (continuous modulation)

**Researched:** 2026-11-13
**Scope:** ROADMAP Phase 2.3: the global control grid, `ControlRamp` (log frequency, dB gain, log Q), target sanitising, the ramping path of the section engine, the `dynOffsetDb` hook. Gates G3 (sweep rows), G5, G6 under stress, G7, and the block-size row with events on multiples of 2048. No second slot, crossfade, sleep or soft bypass (2.4), no output stage (2.5).
**Method:** a scratch bench over the real `Source/dsp` headers, `stages/2-dsp/research-2.3/ramp_bench.cpp`: a prototype of the control grid, the ramps and the ramping path, fed by `eq::Designer::design`. Every number marked [BENCH] is from that bench on this machine (Apple M4 Max, clang -O3, scalar, double engine). Nothing in `Source/`, the harnesses, the model or the goldens was changed.
**Read in full this time:** Wishnick, "Time-Varying Filters for Musical Applications", DAFx-14 (the stability proof that Stage 0 marked unread). Marked [READ].
**Not in the bench:** the JUCE processor, the float output, the render harness. Statements about those are from reading `Source/PluginProcessor.cpp` and are marked [ARGUED].

## Summary

1. **The architecture's smoothing works as specified.** 20 ms control ramps on a 16-sample grid with per-sample linear interpolation of the section parameters give the same click reading as a reference that redesigns the filter on every sample: within 3 % on 49 of 52 sweep rows, and 1.9x to 2.5x on three (grid kinks on rows whose reference is itself nearly click-free). No change to K, the ramp length or the interpolation is needed.
2. **Gate G3's sweep threshold cannot be met as written, by any engine.** "Local κ ≤ 2.0x the held references" reads 3x to 2250x on a 50 Hz tone and fails 8 of 26 rows at 1 kHz, and the per-sample reference reads the same figures. A full-range sweep in 20 to 50 ms modulates a 50 Hz tone inside one cycle; that curvature is the filter doing its job, not a click. The rows need a same-motion reference as the verdict (call C1, Taylor's).
3. **Gate G5's peak bound cannot stand.** "Peak ≤ +72 dB re input" with every parameter random: the double engine reads +199 to +311 dB re input, all finite, over 10 minutes at each of 8 rates. Stacked gain is legitimate output (call C2, Taylor's).
4. **Gate G7's ring-down number is wrong for its own band.** A 10 Hz, Q 40, +12 dB bell has a pole Q of 80 and decays at 3.42 dB/s, not 6.8. After an abrupt stop of the -6 dBFS sine the 60th second reads -280.9 dBFS against "≤ -300". The noise-floor half passes with room: -235.4 dBFS in double (limit -180, model -234), -162.4 dBFS in float (limit -140). Call C3, Taylor's.
5. **Time-varying stability is no longer an unread citation.** The paper's proof was read and covers this engine's parameter box; a shorter argument was also derived and checked numerically: the SVF state matrix has Euclidean norm exactly 1 for every `g, k > 0`, so the state cannot grow under any parameter motion. It holds only if `a1, a2, a3` are recomputed from the interpolated `g, k` each sample, which is what ARCHITECTURE specifies.
6. **CPU.** PERF-02 "all sweeping" (24 cuts x 96 dB/oct): 4.35 % of a core (budget 35 %). The INFO case (24 shelves x 96 dB/oct all sweeping): 23.0 % (line 35 %). The per-sample division costs nothing measurable. The correction-hold fallback is not needed on this evidence.
7. **Block-size identity holds by construction:** events on multiples of 2048 render bit-identically at block sizes 1, 32, 64, 512 and 2048 at four rates.

Three calls are Taylor's (C1 to C3, contract wording). Eight are the plan's.

---

## Q1. Does the specified smoothing pass a click gate? (G3 sweep rows, DSP-04)

`ramp_bench g3`. 48 kHz, one band, sine at -6 dBFS (50 Hz, then 1 kHz), host blocks of 32 and 512; the parameter is read once per block, as in the processor. Timeline: 0.5 s held at A, sweep to B, 0.4 s held, sweep back, 0.4 s held ("p0"); frequency rows also run there-and-back inside the sweep time with no hold ("p1"). Local κ as G3 defines it (5 ms frames, frames below -80 dBFS skipped), first 0.3 s excluded.

Four renders per row:
- **engine**: the specified smoothing (K = 16, 20 ms linear ramps in log f / dB / log Q, per-sample linear interpolation of `g, k, mL, mB, mH, G, cL, cH`, `a1` by division).
- **reference**: the same control ramp, but the filter is designed on every sample (K = 1, no interpolation). It shares nothing with the grid or the interpolation.
- **hold**: K = 16 with the per-sample interpolation off.
- **hard**: no smoothing at all (the Phase 2.2 behaviour: redesign at the block start, state kept).

| Row | Tone | engine / held (blocks 32, 512) | engine / reference | hold / reference (worst) | hard / reference (32, 512) |
|---|---|---|---|---|---|
| Bell +18 dB Q 1, 10 Hz ↔ 30 kHz in 50 ms, p0 | 50 Hz | 19.2, 17.4 | 1.00, 0.99 | 29 | 35, 252 |
| same, p1 | 50 Hz | 32.6, 32.5 | 1.00, 1.00 | 1.07 | 71, 343 |
| Bell at the tone, -30 ↔ +30 dB in 20 ms | 50 Hz | 41, 195 | 0.94, 0.98 | 322 | 211, 211 |
| Bell +18 dB at 2x the tone, Q 0.025 ↔ 40 in 20 ms | 50 Hz | 39, 60 | 0.99, 0.89 | 62 | 200, 1489 |
| Low Cut 96 dB/oct, 10 Hz ↔ 30 kHz in 50 ms, p0 | 50 Hz | 2251, 1534 | 1.00, 1.00 | 2.5 | 2.8, **9.2** |
| same, p1 | 50 Hz | 603, 1591 | 1.00, 1.00 | 9.5 | 43, 17.6 |
| Low Cut 96 dB/oct at 30 Hz, Q 0.025 ↔ 40 in 20 ms | 50 Hz | 81, 199 | **2.17**, 0.96 | 541 | 585, 307 |
| High Shelf 12 dB/oct +18 dB, 10 Hz ↔ 30 kHz, p0 | 50 Hz | 7.7, 9.6 | **2.54**, 1.06 | 143 | 270, 914 |
| same, p1 | 50 Hz | 3.4, 3.5 | 1.01, 1.02 | 16 | 649, 1754 |
| High Shelf 12 dB/oct at tone / 2, -30 ↔ +30 dB | 50 Hz | 159, 86 | 0.97, 0.96 | 43 | 236, 395 |
| High Shelf 12 dB/oct +18 dB at the tone, Q 0.025 ↔ 40 | 50 Hz | 54, 99 | **1.93**, 0.88 | 415 | 419, 1624 |
| High Shelf 96 dB/oct +18 dB, 10 Hz ↔ 30 kHz, p0 | 50 Hz | 21.0, 22.1 | 1.00, 1.00 | 14 | 21, 147 |
| same, p1 | 50 Hz | 45.6, 52.1 | 1.00, 1.00 | 1.6 | 41, 122 |
| Bell frequency, p0 / p1 | 1 kHz | 1.58, 2.48 / 1.51, 2.62 | 1.00 | 1.3 | 3.8, 20 / 4.8, 75 |
| Bell gain | 1 kHz | 1.90, 1.58 | 0.98, 1.00 | 34 | 43, 192 |
| Bell Q | 1 kHz | 1.43, 1.45 | 1.00 | 4.5 | 14, 39 |
| Low Cut frequency, p0 / p1 | 1 kHz | 6.9, 28.3 / 1.35, 9.3 | 1.00, 1.03 / 1.00 | 30 | 93, 27 / 308, 119 |
| Low Cut Q at 30 Hz | 1 kHz | 1.03, 1.02 | 1.00 | 1.4 | 3.6, **8.2** |
| High Shelf 12 frequency, p0 / p1 | 1 kHz | 1.58, 1.54 / 1.32, 1.81 | 1.00 | 1.6 | 2.6, 32 / 4.5, 33 |
| High Shelf 12 gain | 1 kHz | 1.77, 1.54 | 0.99, 1.00 | 5.3 | 14, 87 |
| High Shelf 12 Q | 1 kHz | 1.08, 1.10 | 1.00, 1.01 | 8.5 | 25, 117 |
| High Shelf 96 frequency, p0 / p1 | 1 kHz | 2.11, 2.57 / 1.23, 2.93 | 1.00 | 1.4 | 1.9, 34 / 5.5, 22 |

What the table says:

- **Against the held references the gate is unpassable.** At 50 Hz no row is under 2.0x (smallest 3.4x), and the per-sample reference reads the same (the "engine / reference" column is 1.00). At 1 kHz 8 of 26 readings exceed 2.0x, again with the reference. The cause is the stimulus, not the engine: 60 dB of gain in 20 ms on a tone whose period is 20 ms, or a 96 dB/oct cut passing through the tone in a few milliseconds, bends the waveform inside one cycle. The largest readings (Low Cut, 600x to 2250x) are frames in which the tone is dying toward -80 dBFS: the level-change confound the suite already knows (memory: local κ, same-motion control).
- **Against the same-motion reference the engine is clean.** 49 of 52 readings are within 0.88 to 1.06. Three read 2.17, 2.54 and 1.93, all on the 50 Hz tone at block 32, and all on rows whose reference is itself quiet (the High Shelf frequency row's reference is 3.0x a held sine, the lowest in the table; the engine reads 7.7x, the curvature of a held 140 Hz tone). They are the slope change of the piecewise-linear interpolation at each tick.
- **The controls fire.** With interpolation off, the three rows above read 143x to 541x. With no smoothing at block 512 every row reads at least 17.6x, except two that are void: Low Cut frequency p0 at 50 Hz (9.2x; the reference is already 1500x a held sine because the tone dies) and Low Cut Q at 30 Hz under a 1 kHz tone (8.2x; the Q of a 30 Hz cut barely moves a 1 kHz tone). At block 32 the hard control is weak on frequency rows (1.9x to 5.5x at 1 kHz): a 0.67 ms staircase is already a fine grid. The hard control has to be taken at block 512.

`ramp_bench g3var`, the same rows on other grids and interpolation forms, worst over both block sizes, as a ratio to the reference:

| Variant | 50 Hz rows, worst three | everything else |
|---|---|---|
| K = 16, linear on `g, k, m…` (specified) | 2.54, 2.17, 1.93 | ≤ 1.03 |
| K = 8 | 1.05, 1.04, 0.94 | ≤ 1.01 |
| K = 4 | 1.01, 0.99, 0.98 | ≤ 1.00 |
| K = 64 | 9.79, 6.72, 8.49 | ≤ 2.02 |
| K = 16, linear on `a1, a2, a3` (no division) | 2.54, 2.17, 1.93 | ≤ 1.03 |
| K = 16, one-pole interpolates `Gc` | 2.54, 2.17, 1.93 | ≤ 1.03 |
| K = 16, `g` and `k` geometric, rest linear | 2.54, 1.00, 2.00 | ≤ 1.00 |

The interpolation form does not change the reading; the grid does. K = 8 would bring every row to the reference at twice the design cost (Q5). K = 16 stays the recommendation: the three rows are 2x a quiet reference, and their absolute level is far below every row the reference itself reads loud on.

Two things the same-motion ratio cannot see, and what covers them:
- **A jump in the design itself** (polish basin, re-centre boundary, gain sign change) is in both renders and cancels. Covered by the designer's continuity gate (C1, C1-hi, 2.1b) and by `ramp_bench zero`: across the gain sign change (boost ↔ exact inverse) the section parameters are continuous. 200 000 random bell / shelf / tilt settings per rate: a ±0.02 dB step across 0 moves a parameter by at most 8.9 %, a ±0.0002 dB step by at most 0.075 % (100x smaller step, 100x smaller change), and no more than 2.1x the one-sided step of the same size. The 1 kHz rows against the held references (all ≤ 2.93x except the dying-tone Low Cut rows) are a second, absolute check worth printing as INFO with a ceiling.
- **Whether 20 ms is the right ramp length** is a listening question. The reference shares the ramp.

Also measured: 0 changes of section count or of a section's SVF / one-pole flags during any continuous move (the ramping path can assume a fixed structure between structural changes).

**Recommended (call C1):** G3 sweep rows are scored against a same-motion per-sample reference built in the harness (independent code: design every sample, static recurrence). Threshold 4.0x (measured worst 2.54x). Controls, each on the rows where it is meaningful: interpolation off must read ≥ 10x on the three grid-sensitive rows; no smoothing must read ≥ 10x at block 512, or the row is void (two rows are, and are listed). The reading against the held references is printed for every row, not gated, with a WARN ceiling of 4x on the 1 kHz rows that are not Low Cut frequency rows.

## Q2. Is the time-varying engine stable? (G5, DSP-04; CONTEXT open question 11)

**The paper [READ].** Wishnick's state transition matrix (his eq. 19) is `P = 1/(g² + 2Rg + 1) · [[1 − g² − 2Rg, −2g], [2g, 1 − g² + 2Rg]]`. With `k = 2R` it is this engine's matrix for `(ic1, ic2)`, entry for entry. He notes `‖P‖ = 1` (so the plain norm criterion, which needs a bound below 1, fails), and then proves with a change of basis `T = [[1, c], [0, 1]]` that `‖T P T⁻¹‖ < 1` holds over any box `0 < g_min ≤ g ≤ g_max`, `0 < R_min ≤ R ≤ R_max` for a suitable `c > 0`. That is BIBO stability for arbitrary per-sample variation of `g` and `R` inside a bounded box. The proof was done with computer algebra and omits intermediate results; the statement and its conditions are clear.

**This engine's box [BENCH `lyap`].** Over 400 000 random settings per rate, range ends included, gain to ±60 dB: `g` from 2.9e-5 to 257, `k` from 7.9e-4 to 1256, one-pole `G` from 5.2e-6 to 21. A bounded box with `g, k > 0`: the proof applies.

**A shorter argument, derived here and checked.** With `D = 1 + gk + g²` the matrix satisfies `I − PᵀP = (4gk / D²) · v vᵀ`, `v = (1, −g)`. So `‖P s‖² = ‖s‖² − (4gk / D²)(s₁ − g s₂)²`: with zero input the Euclidean norm of the state never increases, whatever `g, k > 0` do from sample to sample. The only direction that is not shrunk, `(g, 1)`, maps to `(−g, 1)`, which the next step shrinks for any positive `g`. Numerically: largest one-step norm over a 60 x 60 grid (`g` 1e-4 to 1e4, `k` 1e-4 to 1e2) 1 + 1.5e-9 (rounding of the test); a greedy adversary choosing the worst of 8 random parameter sets at each of 2000 steps, 2000 trials: the state norm never exceeded its start. The TPT one-pole is `s ← (1 − 2Gc) s + 2Gc x` with `|1 − 2Gc| < 1` for `G > 0`.

**What the argument requires of the code.** Every per-sample matrix must be of that family: `a1, a2, a3` computed from one `(g, k)` pair. Linear interpolation of `g` and `k` between two ticks stays inside the box (it is convex), so the specified ramping path qualifies. Interpolating `a1, a2, a3` independently leaves the family and is not covered, though it reads the same on G3 and costs the same (Q5). The mix parameters `mL, mB, mH, cL, cH` are memoryless and bounded (largest magnitude designed: 6.5e3), so their interpolation needs no argument.

**Stress [BENCH `g5`].** 24 bands, stereo, noise at -12 dBFS with one second of silence in five; frequency, gain, Q and gain scale redrawn every host block (10 % of draws on each range end), type, slope and enable redrawn every 50th block (a hard reset, as 2.3 has no crossfade); block sizes 4096, 512, 64, 7, 1 cycling every 2 s; 8 rates. Double engine.

| fs | Non-finite samples | Peak re input peak | Largest static stack drawn | Peak minus stack (worst block) | Speed |
|---|---|---|---|---|---|
| 22.05 k | 0 | +199 dB | 482 dB | -140 dB | 77x real time |
| 32 k | 0 | +274 | 550 | -248 | 52x |
| 44.1 k | 0 | +238 | 513 | -143 | 20x |
| 48 k | 0 | +262 | 588 | -206 | 18x |
| 88.2 k | 0 | +311 | 553 | -198 | 15x |
| 96 k | 0 | +295 | 577 | -227 | 14x |
| 176.4 k | 0 | +258 | 623 | -242 | 9.3x |
| 192 k | 0 | +297 | 632 | -220 | 8.6x |

10 minutes of audio per rate (the contract's length). "Static stack": the sum over active bands of the peak gain of each band's target design (floor 0 dB per band), as a running maximum over the blocks of 512 and 4096 samples; an upper bound on the gain of any settled configuration drawn so far. 0 samples above the float range; 0 changes of section structure during a continuous move.

- Finite everywhere. The peak is 127 to 239 dB above G5's "+72 dB re input": that bound reads like one band's ±60 dB plus headroom and does not survive 24 bands, of which about 15 carry gain. The output stayed at least 140 dB below the largest static stack the run had drawn: parameter motion adds no gain of its own.
- The float output is safe on this draw (largest peak +299 dBFS; float overflows at +770), but not in principle: 24 Low Shelves at +60 dB are +1440 dB, finite in the double engine and `inf` on the float output. PLAN-2.2 R3 already ruled that nothing is inserted in the signal path to hide this.
- Throughput: the full 10 minutes per rate took 301 s here for the engine alone. Through the plugin, with a second render for any reference, this is minutes per harness run (call C9).

**Recommended (call C2):** G5's peak clause becomes "peak re input peak ≤ the largest static stack drawn so far + 12 dB" (measured: 140 dB inside) (the stack is the sum over active bands of each target's peak gain, floor 0 dB each; the harness computes it from the designs it requests), on the double engine with the full random draw. The plugin row asserts finite float output on a seeded draw and prints the peak. "Every sample finite" and "no sticky state" stay as written.

**Sticky state.** The double state stays finite when the float output overflows, so a stacked-gain `inf` is not sticky. A non-finite *input* sample is: it makes every later output NaN until the next reset. A check at the end of each processed chunk (the last working sample of each touched lane; if not finite, clear every band's state) costs two comparisons per block and changes no finite signal. Recommended (call C10), with a G5 row: one block of NaN input, then noise; the output is finite again from the next block.

## Q3. Noise floor and ring-down (G7, DSP-05)

`ramp_bench g7`, 192 kHz, bell at 10 Hz, Q 40, static, flush-to-zero on.

| Gain | Residual after removing 1 kHz, last 3 s: double / float | Silence after an abrupt stop: 1st s / 10th s / 60th s | Decay |
|---|---|---|---|
| +12 dB (the G7 band) | **-235.4** / **-162.4** dBFS RMS | -79.2 / -110.0 / **-280.9** dBFS | 3.42 dB/s |
| -12 dB | -255.0 / -162.2 | -82.7 / -205.2 / -885.7 | 13.6 dB/s |
| +30 dB | -215.7 / -161.9 | -66.9 / -77.9 / -138.5 | 1.21 dB/s |

- **Noise floor: passes.** -235.4 dBFS in double against ≤ -180 (the Stage 0 model said -234); -162.4 dBFS on the float image against ≤ -140. The float figure is float quantisation of a -6 dBFS sine, not the filter.
- **Ring-down: "≤ -300 dBFS at 60 s" fails as written, for physical reasons.** The boost's poles have Q = 40 · 10^(12/40) = 79.8, so the mode decays at `8.686 π f / Q` = 3.42 dB/s (measured 3.42). ARCHITECTURE's "about 6.8 dB/s" is the figure for a pole Q of 40. Stopping a -6 dBFS 1 kHz sine abruptly leaves the mode at -79 dBFS, and 59 s later it is at -281 dBFS. With the sine faded out over 2 s instead the mode is barely excited (-227 dBFS in the first second, -429 in the 60th), which passes but tests nothing about the ring.
- Monotone in 1 s windows in all three cases.
- **Silence costs no more than signal:** 24 bells at 10 to 33 Hz, Q 40, 192 kHz stereo: 3.8 % of real time with noise (noise generator included), 3.7 % in silence, 4.1 % in a later silent stretch (run-to-run spread). No denormal slowdown: the state is at 1e-14 after a minute, and reaches the denormal range of a double only after about 30 minutes, where flush-to-zero ends it.

**Recommended (call C3):** G7's silence clause becomes: after an abrupt stop, the level in 1 s windows falls monotonically at the pole's rate (3.42 dB/s ± 2 % between the 10th and 60th second) and is ≤ -270 dBFS in the 60th second; block time in silence ≤ 1.2x block time with signal. The two noise-floor thresholds stay. ARCHITECTURE "Denormal Protection" gets the corrected decay figure.

## Q4. Block-size identity with moving parameters

`ramp_bench blk`: 4 bands (bell, 96 dB/oct High Shelf, 48 dB/oct Low Cut, 24 dB/oct Tilt), stereo noise, frequency / gain / Q of all four redrawn at every event, 96 events.

| Events on multiples of | 44.1 k | 48 k | 96 k | 192 k |
|---|---|---|---|---|
| 2048 samples: blocks 1, 32, 64, 512, 2048 | bit-identical | bit-identical | bit-identical | bit-identical |
| 2048 samples: blocks 7 and 4096 | differ | differ | differ | differ |
| 1000 samples (not a block multiple) | differ | differ | differ where the block start rounds differently | same |

The ROADMAP criterion (events on multiples of 2048, blocks 32 / 64 / 512 / 2048) holds by construction, given three rules:
1. the tick counter is global, counts from `prepareToPlay`, and advances by the number of samples of **every** block, including a block that returns early (hard bypass, no active band);
2. a new target seen at a block start takes effect at the next tick (at once if the block starts on a tick);
3. within a tick the per-sample values depend only on the sample's position in the tick.

Blocks of 7 and 4096 differ because the processor reads parameters once per block: the event is seen later. That is the defined behaviour, not a defect, and G5 covers those sizes for stability only.

## Q5. CPU on the ramping path

`ramp_bench cpu`, 48 kHz stereo, 512-sample blocks, 20 s, noise, 24 bands. "Sweeping": every band's frequency follows a 2 s log triangle 40 Hz to 12 kHz (gain-bearing types also move their gain), so every band redesigns at every tick (3000 per second). Both lanes of a band run in one sample loop on the ramping path.

| Case | Settled | Sweeping, specified | one-pole `Gc` linear | `a1 a2 a3` linear | Design alone |
|---|---|---|---|---|---|
| 24 cuts x 96 dB/oct (PERF-02) | 2.48 % | **4.35 %** | 4.29 | 4.41 | 1.69 |
| 24 shelves x 96 dB/oct (G10 INFO) | 5.10 | **23.0** | 23.0 | 24.0 | 18.1 |
| 24 tilts x 96 dB/oct | 5.11 | 23.0 | 22.9 | 24.0 | 18.1 |
| 24 shelves x 48 dB/oct | 1.99 | 12.9 | 12.5 | 12.4 | 10.4 |
| 24 shelves x 12 dB/oct | 0.70 | 3.78 | 3.78 | 3.73 | 3.18 |
| 24 bells | 0.71 | 4.46 | 4.49 | 4.43 | 3.87 |

- PERF-02 "all sweeping" is at 4.35 % against 35 % (Stage 0 model: about 22 %).
- The INFO case is at 23 % against the 35 % line. RESEARCH-2.2 projected 33.6 % of design cost from one setting (4.67 µs per band); over this sweep the average design is 2.5 µs, because the polish keeps the seed over part of the range. The correction hold stays a documented fallback and is not needed here. The real reading is the plugin's, in execute.
- Sweeping cost is design plus settled cost: the per-sample interpolation and the division add nothing measurable once both lanes share the loop. There is no CPU reason to prefer a form outside the stability argument.
- A preset load that moves all 24 bands ramps them for 20 ms: at most 24 x 4.7 µs x 60 ticks = 6.7 ms of design inside those 20 ms (a third of a core for 20 ms), then nothing.

## Q6. What the processor and the band have to do [ARGUED, from the bench prototype]

**Control ramp (per band, three values: ln f, effective gain in dB, ln Q).**
- Ramp length in ticks: `round (0.020 · fs / K)`: 55 at 44.1 / 88.2 / 176.4 kHz (19.95 ms), 60 at 48 / 96 / 192 kHz, 28 at 22.05 kHz, 40 at 32 kHz.
- A new target restarts the ramp from the current value over the full length. Under block-rate automation this is a 20 ms lag, which is what the G3 rows were measured with.
- Arrival is counted, not compared: after the last step the value is the target itself and the design is made **from the target's own settings**, not from `exp (log f)`. The settled design is then bit-identical to a static design of the same parameters, which every `memcmp` row and G9 depend on. (Suite lesson: a float ramp with an arrival test lands a grid apart from its double twin; a counter has no such case.)
- One ramp for the effective gain `clamp (gain · scale / 100)`, not one for the band gain and one for the scale: fewer states, the scale-100 bit-exactness of `makeBandInput` is kept, and a `gain_scale` move is a gain move on every gain-bearing band. `dynOffsetDb` is added **after** the ramp at each tick (a detector's output must not be smoothed a second time); a band counts as moving while its ramp runs or its offset differs from the last tick's. With the offset at 0 the sum is skipped and EN-dyn stays bit-equal.

**Tick (every K samples, global).** For each active band: if the last interval interpolated, set the section parameters to the tick's design exactly; if the ramp is running, advance it, design, and store per-parameter increments `(target − current) / K`. A band whose ramp has ended and whose parameters have arrived runs the settled path (`toRuntime` once, no division).

**Ramping path.** Per sample, per section: add the increments, then `a1 = 1 / (1 + g (g + k))`, `a2 = g a1`, `a3 = g a2`, `Gc = G / (1 + G)`, then the same two recurrences as the settled path, for both lanes. The parameter is advanced before use, so the last sample of an interval runs on the tick's design.

**Structural changes in 2.3** stay as in 2.2 (design from the target, state cleared, no ramp). Any path that clears a band's state also snaps its ramps to the targets: activation, un-bypass, re-prepare.

**Change detection.**
- `dynOffsetDb` joins the snapshot (SUMMARY-2.2 carry-forward), or a future writer is never seen.
- Canonical gain: `gainDbEff = 0` for Low Cut, High Cut, Notch and Band Pass in `makeBandInput` step 5 (VERIFICATION-2.2 issue 6). Without it one `gain_scale` move starts a 60-tick ramp, with a design per tick, on every active cut. The designer does not read the gain for these types.

**Sanitising** is already in `makeBandInput`: a non-finite raw value becomes the default before it reaches a ramp. The ramp type is the engine's own and is set explicitly in `prepareToPlay`.

**Files.** The ramp and the ramping path belong in the std-only headers (`Band.h`, `SectionEngine.h`, a small `ControlRamp.h`), so the designer harness can drive them in double. The processor owns the tick counter and calls a per-tick function on each active band.

## Q7. Harness shape

| Level | Rows |
|---|---|
| Engine in double (`tests/designer-harness`) | G3 sweep rows against the in-harness per-sample reference, 50 Hz and 1 kHz, blocks 32 and 512, with the two controls; G5 in full (8 rates, the amended peak clause, the NaN-input recovery row); G7 in double; block-size identity with events on 2048 (blocks 1, 32, 64, 512, 2048); settled-after-ramp design equals the static design bit for bit; the kept-state row below |
| Plugin, float (`tests/render-harness`) | G3 sweep rows through `processBlock` against the same reference; G5 on a seeded draw, finite; G6 (`malloc_logger`) around the stress; G7 float; block-size identity at 32 / 64 / 512 / 2048; a `gain_scale` edit mid-render and a `bNN_bypass` row (VERIFICATION-2.2 gap 2); re-prepare at another rate (gap 4); sweeping CPU as INFO |

- **Kept state (VERIFICATION-2.2 gap 1)** [BENCH `keep`]: a bell at 40 Hz, Q 20, +24 dB, rung by a short burst; the gain is edited to +18 dB during the ring. Measure `|y[n] − (2 y[n−1] − y[n−2])|` at the edit sample against the ring's own largest second difference just before it. Ramp with the state kept: 1.7x. State cleared at the edit: 3985x. The 2.2 step: 1353x. A limit of 10x separates all three, so the row also fails a build that classifies the edit as structural.
- The reference must not share code with the ramp: design every sample with `Designer::design`, convert with `toRuntime`, run `svfTick` / `onePoleTick`. Only the control-ramp trajectory is re-stated in the harness.
- G3 at 48 kHz exercises K = 16 only. One row at 96 kHz (K = 32) and one at 192 kHz (K = 64) are cheap and cover the other two grid sizes.
- Start each sweep render with the parameter at its start value and exclude the head (0.3 s here): a 96 dB/oct Low Cut at 30 Hz rings for about 100 ms after the tone's onset.

## Pitfalls carried

- The hard control must be shown to fire before a row counts, and at block 32 it does not on frequency rows: take it at block 512.
- A click row whose tone dies during the sweep reads its largest κ in the last frames above -80 dBFS. That is the reference's reading too; it is why the verdict is a ratio to the reference.
- No `juce::SmoothedValue` (its `reset` keeps a NaN and its arrival is value-tested); no `juce::dsp` filter classes.
- A "moving" flag must be derived at each tick from the ramp counter and the offset, not cached past the change check (suite lesson on run flags that latch).
- No audio golden across platforms: `std::exp` / `std::log` in the ramp and compiler contraction differ between macOS and Windows in the last bits. Bit-identity rows compare renders of one binary.
- Stage 1's 36 rows, the designer's 67 and the render harness's 64 stay green; the parameter golden is untouched.

## Calls for the plan

| # | Call | Recommendation | Whose |
|---|---|---|---|
| C1 | G3 sweep-row verdict | Score against a same-motion per-sample reference at ≤ 4.0x (measured worst 2.54x). Controls: interpolation off ≥ 10x on the three grid-sensitive rows; no smoothing ≥ 10x at block 512 or the row is void. Held-reference readings printed, WARN above 4x on the 1 kHz non-cut rows. Amends ARCHITECTURE G3 and the ROADMAP 2.3 criterion. Alternative: keep 2.0x and halve the grid to K = 8 at ≤ 48 kHz (every row ≤ 1.05x; design cost doubles, the INFO case goes to about 41 %). | Taylor (contract) |
| C2 | G5 peak clause | Double engine: peak ≤ the largest static stack drawn so far + 12 dB. Plugin: finite on a seeded draw, peak printed. Amends ARCHITECTURE G5. | Taylor (contract) |
| C3 | G7 silence clause | Decay 3.42 dB/s ± 2 % from the 10th to the 60th second, monotone, ≤ -270 dBFS in the 60th second; corrected decay figure in "Denormal Protection". Amends ARCHITECTURE G7. | Taylor (contract) |
| C4 | Interpolation form | As specified: linear on `g, k, m…`, `a1` by division each sample. Same click reading and cost as the alternatives, and the only form inside the stability argument. | Plan |
| C5 | Ramp semantics | Restart on retarget; target takes effect at the next tick; counted arrival; final design from the target's own settings; length `round (0.020 fs / K)` ticks. | Plan |
| C6 | Which values ramp | ln f, effective gain, ln Q. No separate `gain_scale` ramp. `dynOffsetDb` added after the ramp, and added to the change snapshot. | Plan |
| C7 | Canonical gain | `gainDbEff = 0` for cuts, Notch and Band Pass in `makeBandInput`. | Plan |
| C8 | Tick counter and snaps | The counter advances on every block, early returns included; any state clear snaps the band's ramps. | Plan |
| C9 | G5 length in the harness | Default 60 s per rate; `--long` runs the contract's 10 minutes, once in execute and once in verify. | Plan |
| C10 | Non-finite input | End-of-chunk check on the touched lanes; a non-finite sample clears every band's state. One G5 row. | Plan |
| C11 | Row placement | Q7's two-level table; the reference lives in the harness, not in `Source/`. | Plan |

## Not answered here

- The float output's own G3 readings (the bench is double). Float rounding is about 1 % of a held 50 Hz tone's second difference at -6 dBFS, so the ratios should carry over; the render harness measures it.
- G3 with several bands moving at once, and with `gain_scale` as the moved parameter (it is a gain move on each band by construction).
- Whether 20 ms sounds right on real material: a hands-on item for verify.
- A velocity-continuous interpolation (quadratic per tick) that would remove the three 2x rows at K = 16 without more designs. Not needed for the recommended gate.
