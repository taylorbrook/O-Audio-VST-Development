# O-ParametricEQ Stage 2 (DSP), Phase 2.2: Research (static engine, placement, transparency)

**Researched:** 2026-11-13
**Scope:** ROADMAP Phase 2.2: `eq::SvfSection`, `eq::OnePole`, `eq::Band` (one slot), `eq::Router`, working buffers, fast path, render-harness rows. Gates G4 (a), (b), (c), (g), G6 static, G8, FUNC-01, FUNC-02, DSP-06 above Nyquist. No ramps (2.3), no crossfades, sleep or soft bypass (2.4), no output stage (2.5).
**Method:** a scratch bench over the real `Source/dsp` headers, `stages/2-dsp/research-2.2/engine_bench.cpp`: a prototype of the section engine (the ARCHITECTURE recurrences, double state) fed by `eq::Designer::design`. Every number below marked [BENCH] is from that bench on this machine (Apple M4 Max, clang -O3, scalar). Nothing in `Source/`, the harnesses, the model or the goldens was changed.
**Not in the bench:** the JUCE processor, APVTS reads, the render harness, pluginval. Statements about those are from reading `Source/PluginProcessor.cpp`, `tests/render-harness/main.cpp` and JUCE 8.0.15 headers, and are marked [READ] or [ARGUED].

## Summary

1. **The recurrences realise the designer's closed form.** Rendered impulse response against `eq::bandMagDb`, 22 settings covering the 8 types at 44.1 and 96 kHz: at most 2.8e-8 dB in double and 5.8e-4 dB after conversion to float, wherever the response is above -80 dB. The 0.01 dB criterion (FUNC-02) has a margin of 17x in float. It cannot hold in a deep stop band on the float output (first miss at -103 dB), so the row needs the same "above -80 dB" scope as G9.
2. **Three gate definitions do not survive the float output as written** (each needs a contract amendment in the plan):
   - **G4 (c)**, Mid bands leave the side within -130 dB *re input peak*: the float output's rounding is about -147 dB re *output* peak, so the row reads -120.5 dB with 24 Mid bells at +6 dB and -91 dB at +12 dB. The engine itself is at -265 dB or better.
   - **G8**, "first non-zero output sample index equals the impulse index": a 96 dB/oct High Cut at 10 Hz has a first-sample gain of 2.0e-47 (44.1 kHz) and 1.2e-57 (192 kHz), below the smallest float. The first non-zero float sample is index 2 and 27.
   - **G4 (b)** and ARCHITECTURE "Float → double → float is exact": false for denormal float samples under `ScopedNoDenormals`. On arm64 with FZ set, a denormal float converts to 0.0. An untouched channel must not go through the conversion.
3. **A 0 dB gain band is not bit-transparent.** Its parameters are exactly neutral, but the SVF output differs from the input by an ulp of the double state, which shows in float as soon as small samples follow large ones (60 378 of 262 144 samples differ for one bell). G4 (d) therefore needs the Phase 2.4 sleep, as planned; 2.2 does not claim it.
4. **CPU, early smoke reading.** 24 cuts x 96 dB/oct settled (the PERF-02 case): 4.6 % of a core in the section-major loop the model assumed, **2.4 % in a sample-major loop** (budget 10 %). 24 shelves x 96 dB/oct settled: 4.7 to 8.6 % (budget 14 %). Design cost with the shipped polish: 4.67 µs per 96 dB/oct shelf band, 33.6 % of a core for 24 of them at 3000 ticks/s (model 46 %).
5. **Recommended structure:** the engine as std-only headers in `Source/dsp` (same rules as the designer), gated twice: in double by the designer harness (tight thresholds, no float confound) and through the plugin by the render harness.

Six calls for the plan are listed at the end. Two are the user's (C1, C2).

---

## Q1. Does the engine realise the closed form? (FUNC-02)

`engine_bench ir`: impulse through one band, rendered until the tail is 300 dB below the peak (cap 120 s), single-bin DFT at 160 log-spaced frequencies 20 Hz to min(20 kHz, 0.499 fs), compared with `eq::bandMagDb` of the same `BandDesign`. "float" converts each output sample to float first, as the plugin does.

| Case | max err, double | max err, float (above -80 dB) |
|---|---|---|
| Bells (1 kHz +12; 18 kHz -18 Q 5; 25 kHz +12; 100 Hz -30 Q 0.025) | ≤ 8e-11 dB | ≤ 3.8e-6 dB |
| Bell 10 Hz, Q 40, +30 dB (120 s render, tail not finished) | 2.8e-8 | 4.7e-7 |
| Low / High Shelf, 12, 18, 48, 96 dB/oct, to +30 dB | ≤ 4.5e-13 | ≤ 8.1e-6 |
| Low Cut 6, 24, 96 dB/oct | ≤ 1.2e-9 | ≤ 8.1e-5 |
| High Cut 12, 48 dB/oct (one above Nyquist) | ≤ 1.8e-14 | ≤ 3.3e-7 |
| High Cut 6 kHz, 96 dB/oct | 4.1e-12 | **5.8e-4** (above -80 dB); 12.4 dB over the whole band |
| Notch (1 kHz Q 10; 50 Hz Q 40) | ≤ 2.7e-10 | ≤ 3.2e-7 |
| Band Pass (500 Hz Q 2; 10 Hz Q 40) | ≤ 5e-13 | ≤ 9.6e-6 |
| Tilt 12 and 48 dB/oct | ≤ 8.4e-14 | ≤ 6.3e-7 |

Worst over both rates. The float output departs from the closed form by more than 0.01 dB only where the response is below about -103 dB (44.1 kHz) and -107 dB (96 kHz): that is float rounding of the impulse response, not the filter.

**For the plan:**
- Score the FUNC-02 row where the closed form is above -80 dB (as G9 does). Margin 17x.
- **Render length is part of the row.** Needed for a -300 dB tail [BENCH]: 0.1 to 0.4 s for ordinary settings, 1.7 s for a 30 Hz 96 dB/oct Low Cut, 6.7 s for a 50 Hz Q 40 notch, 9.4 s for a Q 0.025 bell, 44 s for a 10 Hz Q 40 band pass, more than 120 s for a 10 Hz Q 40 +30 dB bell (its pole Q is about 225). A fixed short render fails the slow cases by truncation. The row should render until the tail is at least 140 dB below the peak, assert that it got there (so a truncated render cannot pass), and keep its eight type cases at settings that settle within a few seconds. The 10 Hz / Q 40 extremes belong to G7 and G9.
- The reference must be designed from the values the processor used: the raw parameter floats, through one shared function (Q6).

## Q2. Mid / Side difference injection (G4 (c), FUNC-03)

`engine_bench ms`: stereo noise at -12 dBFS (independent L and R, float), 48 kHz, bells Q 1 spread 40 Hz to 16 kHz on Mid (or Side), ARCHITECTURE's `d = F(m) − m; L += d; R ± d`. Error of the untouched component (side for Mid bands) against the input's:

| Config | Double engine output | Float output, re input peak | Float output, re output peak |
|---|---|---|---|
| 1 Mid bell +6 dB | -325 dB | -149.5 | -151.7 |
| 1 Mid bell +30 dB | -313 | -133.6 | -148.9 |
| 1 Mid bell -30 dB | -325 | -149.5 | -152.7 |
| 24 Mid bells +6 dB | -289 | **-120.5** | -147.4 |
| 24 Mid bells +12 dB | -265 | **-91.2** | -146.8 |
| 1 Side bell +30 dB | -313 | -133.0 | -148.2 |
| 24 Side bells +12 dB | -265 | **-91.1** | -146.9 |

The difference injection is sound: the leak in the engine is double rounding. What the float row measures is `0.5 (L' − R')` of two independently rounded floats, an error of about half a float ulp of the *output*. Once the Mid boost raises the output above the input, the gate's "-130 dB re input peak" is a statement about float precision and fails.

**Recommended (call C1):** gate the engine in double at ≤ -250 dB re input peak (measured worst -265), and the plugin's float output at ≤ -130 dB re the **larger of the input and output peak** (measured -147 to -153, 17 dB of margin). Negative control: the same row with one band on Left must fail.

## Q3. Latency and the first sample (G8, DSP-02)

`engine_bench g8`, unit impulse, Q 0.025:

| Band | fs | y[0] (double) | first non-zero index: double / float | first normal float |
|---|---|---|---|---|
| High Cut 10 Hz, 96 dB/oct | 44.1 k | 2.0e-47 | 0 / **2** | 15 |
| High Cut 10 Hz, 96 dB/oct | 192 k | 1.2e-57 | 0 / **27** | 94 |
| High Cut 100 Hz, 96 dB/oct | 192 k | 1.2e-41 (a float denormal) | 0 / 0 | 3 |
| High Cut 10 Hz, 48 dB/oct | 192 k | 3.5e-29 | 0 / 0 | 0 |
| every other case tried (cuts to 48 dB/oct, Low Cut 30 kHz, Band Pass, Notch, Bell at 10 Hz) | both | ≥ 5.9e-15 | 0 / 0 | 0 |

The engine has a direct path in every section (`a3 > 0`), so in double the output starts at the impulse index in every configuration: the latency is 0. The float output of a steep, very low High Cut starts later only because its first samples underflow; with flush-to-zero on (the plugin's case) the "first normal float" column applies.

**Recommended (call C2):** G8 becomes: `getLatencySamples() == 0`; no non-zero output sample before the impulse index (always); the output at the impulse index equals the predicted first-sample gain `Π (mL a3 + mB a2 + mH (1 − k a2 − a3)) · Π (cL Gc + cH (1 − Gc))` within 1e-6 relative wherever that gain is at least 1e-30; and the double engine starts at the impulse index in every configuration (designer harness). The predicted-gain check is stronger than "non-zero".

## Q4. Bit transparency (G4 (a), (b), (g); QUAL-01)

- **Block-size invariance** [BENCH `blk`]: a 96 dB/oct High Shelf into a 36 dB/oct Low Cut rendered at block sizes 1, 7, 16, 32, 64, 512, 2048, 4096 and one block: bit-identical. With static parameters nothing in the engine depends on the block boundary, provided state persists across calls and the oversized-block chunk loop (below) splits only the sample range.
- **A 0 dB gain band** [BENCH `neutral`, `tiny`]: the designer returns exactly neutral parameters for a Bell, shelf or Tilt at 0 dB (`mL = mH = 1`, `mB = k`, `cL = cH = 1`; seven settings checked). The section output still differs from its input by up to 2.5e-16 (the SVF computes `v2 + k v1 + (x − k v1 − v2)`). On -12 dBFS noise that never reaches the float output (0 of 65 536 samples differ). On 0 dBFS bursts alternating with -180 dBFS noise, 60 378 of 262 144 float samples differ for one bell and 99 766 for 24. So "24 bells at 0 dB" is bit-exact only with the Phase 2.4 sleep. In 2.2 a 0 dB band is an ordinary active band; G4 (d) and (e) are not 2.2 rows.
- **Denormal samples** [measured, a 20-line arm64 test in the session scratchpad]: with FPCR.FZ set (what `juce::ScopedNoDenormals` does on arm64, `juce_FloatVectorOperations.cpp`), a denormal float (1e-41) converted to double reads 0.0, and a double whose float image is denormal (7.5e-39) converts to 0.0f. ARCHITECTURE "Float I/O, double engine" says the round trip is exact for untouched samples. It is, except for denormal samples. Consequences:
  - A channel no active band touches must not be converted at all. With only Left bands, the right channel is never copied to the working buffer and never written back. This also saves the copy.
  - The fast path (no active band) does not touch the buffer, as specified.
  - G4 (a) and (b) need a stimulus that contains denormal samples (a tail scaled to 1e-41 … 1e-39 after the noise), or the gate cannot see the defect: -12 dBFS white noise has none.
- **Compiler contraction** [ARGUED]: clang on arm64 contracts `a * b + c` within one expression. That is deterministic per build, so every bit-identity row holds within one binary, but macOS and Windows renders will differ in the last bits. No audio golden may be committed for byte comparison across platforms (the designer goldens are safe: the designer keeps products in separate statements).

## Q5. CPU (early smoke reading; PERF-02 is a Stage 4 gate)

`engine_bench cpu`, 48 kHz stereo, 512-sample blocks, 20 s, noise input, two runs agreeing within 3 %:

| Settled, % of one core | Section-major, lane by lane | Section-major, both lanes in one loop | Sample-major, lane by lane | Budget |
|---|---|---|---|---|
| 24 cuts x 96 dB/oct (384 SVF) | 4.6 (2.5 ns per SVF-sample) | 2.8 | **2.4** | ≤ 10 |
| 24 shelves x 96 dB/oct (384 SVF + 384 one-poles) | 8.6 | 6.8 | **4.7** | ≤ 14 |
| 24 bells | 1.07 | 0.87 | **0.70** | |
| 24 cuts x 12 dB/oct | 0.59 | 0.36 | 0.64 | |

- The Stage 0 cost model (2.5 ns per settled SVF, 4.6 %) is exact for the loop it assumed: one section over the whole block. That loop is bound by the latency of one section's state recurrence. Putting every section of a band inside one sample loop lets the independent recurrences overlap and halves the cost with no SIMD. Recommended loop: **sample-major per band and lane** (call C5). Its bit-identity with the section-major loop was not tested; block-size invariance holds for either by construction.
- Float → double → float for two channels: 0.001 % of a core.
- Ramping path as specified (per-sample linear interpolation of the five SVF parameters, one division per sample), 24 cuts x 96 dB/oct in 16-sample segments: 3.2 % (both lanes alternate per segment, so their recurrences overlap; the division is off the critical path).
- Design cost, shipped `Designer::design`, per band: bell 0.78 µs; 96 dB/oct High Cut 0.18 µs; High Shelf 12 / 48 / 96 dB/oct 0.75 / 2.68 / 4.67 µs; Tilt 96 dB/oct 4.68 µs. At 3000 ticks/s with 24 bands sweeping: 5.6 %, 1.3 %, 5.4 %, 19.3 %, 33.6 %, 33.7 % of a core. The architecture table said 5.8 / 23 / 46 % for bells / 48 / 96 dB/oct shelves; the shipped polish is cheaper than the research port (0.58 µs per polished section against 0.77 to 0.90).

Projection for Phase 2.3 from these parts: PERF-02 "all sweeping" (cuts) about 1.3 + 3.2 = 4.5 % against 35 %. The INFO case (24 shelves x 96 dB/oct sweeping) about 33.6 % design plus roughly 5 to 9 % audio, so near or just over the 35 % line: the correction-hold fallback stays a 2.3 decision, read on the real plugin. In 2.2 the engine is static, so the plugin can only confirm the settled rows; the sweeping figures above are this bench's.

Worst one-off design burst in 2.2: a preset load that changes all 24 bands in one block costs at most 24 x 4.7 µs = 0.11 ms, 17 % of a 32-sample block at 48 kHz. Acceptable; no deferral needed.

## Q6. What the processor has to do around the engine [READ / ARGUED]

**Files.** `Source/dsp/Engine.h` (or `SvfSection.h`, `OnePole.h`, `Band.h`, `Router.h`): std-only C++17, `noexcept`, no allocation, same rules as the four designer headers. The 2.1 plan's grep gate over `Source/dsp` (no `juce`, `std::vector`, `new`, `malloc`) then keeps covering it, and the designer harness can drive the engine in double with no framework. Working buffers are owned by the processor (`std::vector<double>` or `juce::HeapBlock`, sized in `prepareToPlay`) and passed in as pointers.

**Runtime coefficients.** Convert each `SectionParams` once per design: `a1 = 1 / (1 + g (g + k))`, `a2 = g a1`, `a3 = g a2`, and `Gc = G / (1 + G)` for the one-pole (ARCHITECTURE writes `v = (x − s) G / (1 + G)`). A section runs the halves its `hasSvf` / `hasOnePole` flags name; `numSections` is at most 8.

**Per block:**
1. `ScopedNoDenormals`. Clear output channels beyond the inputs (the Stage 1 line).
2. Read the 4 + 192 raw atomics once. Per band, compare the eight raw floats and `gain_scale` with the last snapshot **by bit pattern** (`memcmp`), so a NaN compares equal to itself and a change is never missed or repeated.
3. Sanitise, then build `BandSettings` through **one shared function** (the display model in 2.6 and the harness reference use the same one): non-finite → the parameter default; clamp frequency 10 to 30 000 Hz, gain ±30 dB, Q 0.025 to 40, scale 0 to 200; choice values rounded and clamped to their index range; `gEff = clamp (gain · scale / 100, −60, +60)`. `(g · s) / 100`, `g · (s / 100)` and `g · (s · 0.01)` all return the float gain exactly at scale 100 (2 million random gains, 0 inexact [BENCH `scale`]); the form is free. `Band::dynOffsetDb` (0) is added to `gEff` here: that is DSP-07's engine half, and it needs its coverage row (CONTEXT).
4. Active list: enabled and not bypassed. No sleep in 2.2.
5. A band whose snapshot changed is redesigned at the block start. Continuous change (frequency, gain, Q, scale): new coefficients, state kept; the step is audible and is 2.3's job. Structural change (type, slope, placement) or activation: state cleared. A band leaving the active list has its state cleared, so re-enabling never plays stale state.
6. No active band: return without touching the buffer.
7. Otherwise, in chunks of at most the prepared block size (a host block can exceed `samplesPerBlock`; suite lesson): convert **only the lanes an active band touches**, run the bands in index order, write those lanes back.

**Placement.** The stereo decision is a flag set in `prepareToPlay` from the negotiated output set, not from `buffer.getNumChannels()`. Mono: every band runs on lane 0 whatever its placement. Stereo, Mid, Side touch both lanes; Left and Right one. Mid and Side need one scratch lane.

**`prepareToPlay`.** `Designer::prepare (fs)` before any design (it decides the no-polish rule below 44.1 kHz); size the two working lanes and the scratch lane to `samplesPerBlock`; clear all state; invalidate every snapshot so the first block designs every active band; `setLatencySamples (0)`. It can be called repeatedly and at a new rate: nothing may be cached across it.

**Global `bypass` in 2.2.** The soft bypass is Phase 2.4. Until then the parameter should act as a hard switch (pass-through, engine state cleared while bypassed), so pluginval and hosts see a working bypass. `processBlockBypassed` stays routed to pass-through. `output_gain` and `auto_gain` are not applied until 2.5; G4 (a) is stated at output 0 dB.

**Thread note.** A state restore on another thread can change a band's parameters between two of the block's reads, so one block may design a mixed setting. It is sanitised and finite, and the next block corrects it. No lock.

**Tail.** ARCHITECTURE says `getTailLengthSeconds()` returns 2.0; the Stage 1 shell returns 0 and render-harness row L4 asserts "tail 0". Phase 2.2 is where the engine starts to ring, so the return value and row L4 change together here (call C4).

## Q7. Harness shape

Two levels, because three of the gates are confounded by the float output:

| Level | Where | Rows |
|---|---|---|
| Engine in double | `tests/designer-harness` (std-only, already has the `malloc_logger` row A1 and the golden plumbing) | Impulse response vs closed form ≤ 1e-6 dB above -80 dB, eight types, five rates; M/S untouched component ≤ -250 dB; first output sample at the impulse index in every configuration; block-size bit-identity; finite output for frequencies above Nyquist at 44.1 kHz and for random settings at 22.05 to 192 kHz (static, re-drawn per block) |
| Plugin, float | `tests/render-harness` (36 Stage 1 rows stay; L4 changes) | FUNC-01 independence; FUNC-02 ≤ 0.01 dB above -80 dB; G4 (a), (b) with denormal samples in the stimulus, (c) re the larger peak, (g) at 32 / 64 / 512 / 2048; mono: every placement changes the channel and equals the Stereo render; G8 as amended; G6 static with the suite `malloc_logger` recipe (volatile flag, thread scope, liveness probe), first block and a block after a parameter change included; an oversized block (prepare 256, feed 1024) equal to four blocks of 256; the settled CPU readings as INFO |

Negative controls worth having (suite practice: a row that cannot fail proves nothing): G4 (b) with the band moved to Stereo must fail; G4 (c) with the band moved to Left must fail; FUNC-01 with one band disabled must show that band's contribution missing; the allocation row must count a planted `malloc`; the FUNC-02 row must fail against the closed form of a neighbouring setting (for example the same band 1 % higher in frequency), which shows the 0.01 dB row can see a wrong design.

FUNC-01 measure: 24 bands enabled together at distinct frequencies; for each band, the render with that band disabled differs from the full render, and the rendered response equals the product of the 24 closed forms within the FUNC-02 tolerance.

## Pitfalls carried

- `Designer::prepare` before the first `design` at every rate change; `design()` does not guard its inputs (VERIFICATION-2.1). Sanitising is the caller's job (Q6 step 3).
- Skewed defaults are inexact (999.99988 Hz, 0.99999976): no fast path may test for an exact frequency or Q. Gain 0 dB is exact.
- No always-on stage: nothing between the float input and the float output except the bands.
- No `juce::dsp::IIR::Filter`, no `StateVariableTPTFilter`, no `juce::dsp::Gain`.
- Stimulus level above every threshold in the gate; byte-compared harness dumps written with `"\n"`.
- `scripts/resolve-target.sh` greps comments in `CMakeLists.txt` for the plugin-declaration command; header-only engine files need no CMake change.

## Calls for the plan

| # | Call | Recommendation | Whose |
|---|---|---|---|
| C1 | G4 (c) reference level | Double engine ≤ -250 dB re input peak; plugin float output ≤ -130 dB re the larger of input and output peak. Amends ARCHITECTURE G4 (c). | Taylor (contract) |
| C2 | G8 wording | No output before the impulse; predicted first-sample gain matched where it is ≥ 1e-30; double engine starts at the impulse index always. Amends ARCHITECTURE G8. | Taylor (contract) |
| C3 | FUNC-02 row scope and length | Scored above -80 dB; render to a -140 dB tail, asserted; type cases that settle in seconds. Amends the ROADMAP 2.2 criterion's wording. | Plan |
| C4 | Tail length | `getTailLengthSeconds()` → 2.0 in 2.2 as ARCHITECTURE says; row L4 updated in the same commit. | Plan |
| C5 | Inner loop | Sample-major per band and lane (2.4 % against 4.6 % on the PERF-02 case). Lane-pair and SIMD stay deferred. | Plan |
| C6 | 2.2 behaviour that later phases replace | Parameter change = redesign at the block start, state kept (steps until 2.3); structural change = state cleared (clicks until 2.4); global bypass = hard switch (until 2.4); a 0 dB band is active (until 2.4's sleep). ARCHITECTURE "Float I/O, double engine" gets a line: untouched lanes are not converted (denormals). | Plan |

## Not answered here

- Bit-identity between the sample-major and section-major loops (not needed by any gate).
- Whether pluginval at strictness 10 and `auval` still pass with the engine in: they are run in execute, as in Stage 1.
- Time-varying stability (G3, G5 full) and the noise floor (G7): Phase 2.3.
- Shelf audio cost on the ramping path (only cuts were timed).
