# Stage 2 — DSP · Phase 2.5a (Tempo sync, LFO/env/macro sources, modulation matrix) — Research

**Plugin:** O-Granulator
**Stage / phase:** 2-dsp / 2.5a of 7
**GSD phase:** research
**Date:** 2026-11-05
**Inputs:**
- `CONTEXT-2.5a.md` (D13–D16 locked; seven open questions)
- ARCHITECTURE §4, §13, §15–§17, Telemetry, Parameter Mapping, Algorithm §6
- `parameter-spec.md` (destination table, tempo divisions, LFO/env/macro/mod rows)
- the 2.4 engine: `Engine.cpp` (`process`, `gridTick`, `scheduleStreams`, `spawn`, `writeReverbAtomics`), `ControlGrid.h` (`CellLerp`, `ExactRamp`, `GridParams::latch`), `StreamScheduler.h`, `EngineConstants.h` (RNG streams / slots)
- `GranulatorParamIds.h` (`kModDestIds`, contract rows), `tests/golden/mod-dests.tsv`, render harness `MockPlayHead` (`main.cpp:132`)
- O-GrainScatter 3.6.0 `dsp/TempoTracker.h`, `dsp/GrainScheduler.h::processBlockSync`
- grain-engine 1.1.0 `GrainScheduler.h`, `CounterRng.h`; reverb-unit 1.2.0 `ReverbUnit.h`
- JUCE 8.0.15 `juce_NormalisableRange.h` (`convertTo0to1` / `convertFrom0to1`)

**Scratch experiments:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/79afa22a-79b0-486d-ab4c-94a097a80eac/scratchpad/r25a/`

| Script | What it does |
|---|---|
| `rt.cpp` | float32 `convertFrom0to1 (convertTo0to1 (x))` identity over 2²⁰ values per destination range (JUCE formulas, exact skews); ±10 % normalised swing asymmetry at defaults |
| `anchor.py` | ppq at every 32-sample grid point: per-block host origin (GrainScatter port) vs a held anchor, across block sizes 441/512/4096/variable at 120/133/97.3 bpm; float LFO differences; `ceil (ppq/div)` flips |
| `ramp.py` | tempo-ramp ppq prediction error per block: constant-pps vs trapezoid |

All seven open questions are answered. There are **twenty findings** (R39–R58). Six would make a 2.5a gate fail, or pass vacuously, if missed: **R39, R40, R46, R50, R56** and **R41** (enable-edge timing).

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| **R39** | The GrainScatter port re-origins ppq at every host block (`ppqOrigin = host ppq`). `origin + k·pps` and `origin' + k'·pps` round differently: **30–52 % of grid-point ppq values differ between block sizes**, and the float LFO differs at ~0.01 % of cells. That is enough to break the PERF-03 null under Tempo | **PERF-03 fails** | **Held anchor:** `ppq(abs) = anchorPpq + (abs − anchorAbs)·pps`. Re-anchor only on a discontinuity or a bpm change (R42). Measured: 0 % differ at every block size |
| **R40** | `ceil (ppq/div)` evaluated per tick flips when a grid point lands on a cell start (1/16 at 120 bpm / 48 k = 6000 samples, half of them on a cell boundary): **1 flip per 20 k cells** with the per-block origin → a dropped or doubled onset | **FUNC-04a / PERF-03** | Track the pending **slot index** per stream (`slot + 1` per onset). Derive it from ppq only at re-anchor, as `ceil ((ppq − origin)/div − 1e-9)` |
| **R41** | `GrainScheduler::rescheduleNext` is a no-op before the first onset, and `reset()` fires on the next sample. A Tempo-mode enable edge (or Free→Tempo switch) therefore fires **off-grid immediately**. This is ARCH gap G3 | **FUNC-04a** | Tempo mode uses a plugin-local **absolute-time grid clock** in `StreamScheduler` (onset k at `t(slot_k) + J·(u_k − ½)·I`). Free mode keeps `GrainScheduler` untouched (N2 bitwise). No module change |
| R42 | Discontinuity detection and transport states (Q1). Prediction with the constant old pps misses a fast host tempo ramp by up to **161 samples** (60→180 bpm over 2 s, 4096 block); the **trapezoid** predictor stays below 4e-12 | Q1 closed | Predict `ppq_prev + n·(pps_prev + pps_new)/2`. Error > 0.5 sample or bpm changed → re-anchor + re-time. Error > 1 ms → also a **re-lock** (LFO, slot). While stopped, ignore host ppq and run a virtual ppq at host bpm |
| R43 | GrainScatter port deltas (Q1): no loop/locate detection at all (stateless crossings); pre-roll `divIndex < 0` skipped (silent count-in); `bpm ≤ 0` or no `ppq` → 120 manual counter with `isPlaying = true`. **By reading, not reproduced:** GS never checks `isPlaying`, so a stopped transport parked on a beat (ppq 0) re-fires a sync grain every block | Port note | Port `SyncInfo::ppqAt` and the WR-09 `bpm > 0` guard only. Use the R39–R42 design for the rest. File the GS stopped-transport issue separately (out of 2.5a scope) |
| R44 | "1 bar" is undefined outside 4/4. The spec says divisions are quarter-note-relative, and `ceil (ppq/4)·4` misplaces bars after a 3/4 or 7/8 change | **Needs sign-off** | Recommend: bar = `4·num/den` quarters from the host time signature (4/4 fallback), grid origin `fmod (ppqOfLastBarStart, barLen)` latched at each anchor. Alternative: fixed 4/4 grid from ppq 0, documented |
| R45 | Tempo-mode size is capped by `kMaxSizeMs = 2000` (reach `kMaxReachS = 10 s`). **"1 bar" clamps below 120 bpm and "2 bars" clamps below 240 bpm.** A density multiplier of `2^(4·offset)` on 1/64 T reaches 768 g/s | **Needs sign-off** (size) | Clamp size to [5, 2000] ms, and say so in the UI copy (Stage 3). Clamp the density multiplier in log2 steps to the [0.5, 200] g/s Free range. Quotas and coherence read the **effective** Tempo density and size |
| **R46** | The normalised round-trip is **not identity**: 18–72 % of float values change on skewed ranges, s_pan 18 %, **s_level 25 %** (max rel err 0.25 near 0 dB). Only input_gain, mix, pitch and window are exact | **N2 fails** if missed | Evaluate the matrix **only for routed destinations** (sparse). Unrouted, or an offset of exactly 0 → the raw 2.4 path. The N2 zero-depth positive control then holds by construction |
| R47 | Ramped destinations already ramp in their own domain (gain, log2 Hz, pan, ×0.95, 1 + 7·sat). Modulating the unramped base would lerp knob jumps over 0.67 ms instead of 10–20 ms | Design | Apply modulation **after** the base ramp: map `ramp.current()` → raw → base01, add the offset, clamp, map back, then `CellLerp` per sample. There is a per-destination map table (§4) |
| R48 | Spawn-latched reads: `Spawner::plan` reads `gp.s[k]` latched at the tick. ARCH wants the value interpolated at the spawn sample. Tick consumers (quotas, R5 coherence, scheduler interval) also read density, size, pitch, spray | Design | At spawn, lerp the **offset** (normalised) `prev → cur` at `offset/32`, then `convertFrom0to1`, into a per-spawn copy of `GridParams::Stream`. Tick consumers read the modulated `cur` |
| R49 | A per-slot live shadow overflows when a route changes again within 20 ms. Host automation of `modM_dest` stepping through indices does exactly this, and dropping the busy shadow is a step | Design | Use a **per-destination residual** instead: on a route change, add the slot's current contribution to `residual[dest]` and ramp the residual linearly to 0 over 20 ms from its new value. Fixed memory (113), any change rate, value-continuous |
| **R50** | D15 refinement: the Free-mode direct formula `frac (φ0 + n·inc)` jumps under **ordinary rate automation**, not only under matrix modulation. Δphase = n·Δinc, which is 0.6 cycles for 0.5 → 0.51 Hz after 60 s | **QUAL-03 / D15** | **Free LFOs always use the accumulator.** Direct is used only for Tempo (playing, or the virtual ppq while stopped). Predicate: offset ≠ 0 on 109–112 flips Direct → Accum; Accum → Direct only at a re-lock. `lfoN_phase` is added outside the accumulator and ramped on the shortest arc. Rate offsets use the **previous** cell's matrix (no source cycle) |
| R51 | LFO shapes and draws. S&H and smooth random need a cycle counter. Negative cycles occur in pre-roll. Square, saw and S&H steps are lerped over one cell (0.67 ms) | Design / gate scope | New RNG stream base `kLfoStream`, counter = cycle (`int64` → `uint64` cast, deterministic). Smooth random uses cosine interpolation. Use `frac (x) = x − floor (x)`, never `fmod`. The zipper gate runs on sine/triangle; steps are by design |
| R52 | Envelope follower: a per-sample one-pole on the conditioned mono input, with coefficients from attack/release at ticks. dB is taken at the grid point only | Design | `a = 1 − exp (−1/(τ·fs))` in double → float. The source is `clamp ((20·log10 (env) + 60)/60, 0, 1)` at the tick. NaN guard → state 0 |
| R53 | Skew asymmetry (Q4): ±10 % normalised on `s_size` 100 ms gives **×2.04 / ×0.415**, which is near log-symmetric. Cutoff at its default 20 kHz (base01 = 1) can only go **down**, so a bipolar LFO is half-wave rectified. Spawn-latched aliasing (Q5) is a per-grain sample of the source, by design | Q4/Q5 closed | Accept. Note for 3.3: rings draw in normalised space (symmetric). The QUAL-03 gate excludes latched-destination aliasing |
| R54 | Reverb destinations: `reverb_mix` modulated to ≤ 0.001 for longer than the 50 ms fade **clears the tank** (`ReverbUnit.h:182–188`), so the result is gated-reverb behaviour. `reverb_size` is smoothed over 100 ms inside the unit, so fast modulation is attenuated | Accept + DAW item | The matrix writes the atomics per cell (closes the 2.4 carry). Add "LFO → reverb_mix across 0" to the D16 checklist |
| R55 | Choice-backed destinations (Q7): **none**. All 112 entries are Float (`GranulatorParamIds.h:282`). The table is already pinned by the Stage 1 golden `tests/golden/mod-dests.tsv` | Q7 closed | No rounding rule needed. Reuse the existing golden; don't write a second one |
| **R56** | The harness `MockPlayHead` computes `ppq = t/fs·bpm/60`, so a bpm change **is a ppq jump**, not a tempo change. It has no loop, locate, pre-roll or time signature. Real hosts report a position only at block starts, so a loop wrap mid-block can't be expressed | **FUNC-04a / PERF-03 vacuous** | Upgrade the mock: a piecewise tempo map (integrated ppq), loop [a, b), locate, negative start, time signature, and an event list. The harness **splits host blocks at transport events**. A tempo **ramp** is block-size dependent by nature (per-block bpm) → DAW only (D16) |
| R57 | QUAL-03 / FUNC-06 measurement: an audio click probe alone is confounded by the source | Gate design | Gate the **destination trajectory** through a test hook: max\|Δv\| per sample ≤ 1.05 × the analytic LFO slope bound, and no step at route removal. The audio step-click gate against a same-motion reference with a tonal source is secondary |
| R58 | Cost (Q6): 16 slots + sparse conversions + 4 LFOs + env < 1 % of a core. Telemetry of all 113 values through `convertTo0to1` costs ≈ 170 k pow/s ≈ 0.3 % | Q6 closed, INFO | Use the sparse form. Telemetry for unrouted destinations is dirty-checked on the raw value. **C5 ≈ C4 + < 1 % ≈ 19 %** |

---

## 1. Tempo tracker (R39–R43)

### 1.1 Port delta: GrainScatter `TempoTracker` (Q1)

`TempoTracker::update` returns per block `{bpm, ppqOrigin = host ppq, ppqPerSample, isPlaying}`. Without a usable position (no playhead, `bpm ≤ 0` (WR-09), or no ppq) it falls back to 120 bpm on a `manualSamples` counter with `isPlaying = true`. `GrainScheduler::processBlockSync` then fires on crossings `triggerPpq ∈ (ppqAt (i − 1), ppqAt (i)]`. It is stateless, so:

- **Loop / locate:** there is nothing to detect. A crossing either happens in the new block or it doesn't. There is no threshold.
- **Pre-roll (negative ppq):** `if (divIndex < 0) continue;`, so the count-in is silent.
- **Stopped transport:** `isPlaying` is never read by the scheduler. With a host that keeps reporting the same stopped ppq (common in Logic), `ppqAt (0) == stoppedPpq` every block. If a division lies at `stoppedPpq` (stopped at bar 1 is ppq 0), `0 > −pps && 0 ≤ 0` fires at i = 0 **every block**. This is from reading the code and was not reproduced. It is a GrainScatter issue to file separately.

**Port only** `SyncInfo::ppqAt` (integer samples × pps, never an accumulated ppq) and the WR-09 `bpm > 0` guard. The Granulator scheduler is a pending-onset clock (ARCH §17), not a crossing detector, so it needs the anchor design below.

### 1.2 R39: the held anchor

`anchor.py` renders the ppq at every absolute grid point (20 k cells) under the harness formula `ppq = t/fs·bpm/60`:

| bpm | Per-block origin: ppq differs vs block 32 | Float LFO differs | `ceil (ppq/0.25)` flips | Held anchor |
|---|---|---|---|---|
| 120 | 29.7–32.6 % | 0.01 % | 1 (441, 512, var) | **0 %** |
| 133 | 45.6–51.9 % | ≤ 0.005 % | 0 | **0 %** |
| 97.3 | 47.2–49.7 % | ≤ 0.01 % | 0 | **0 %** |

A one-ulp ppq difference occasionally crosses a float rounding boundary of the LFO value. That changes the matrix offset, then a latched grain field, then the audio, so the PERF-03 null fails under Tempo.

**Design:** keep `anchor = {abs, ppq, pps, barOrigin}`. Every grid-point ppq is `anchor.ppq + (abs − anchor.abs)·anchor.pps`, a pure function of the absolute counter. The host position is read once per host block (`getPosition()` returns `Optional<PositionInfo>` by value, with no allocation). It is compared with the prediction (R42) and adopted only when it disagrees. Transport events in the harness fall on block starts (R56), so every block split re-anchors at the same absolute samples.

### 1.3 R40: slot tracking

Grid points coincide with cell starts whenever `div·60/bpm·fs` is a multiple of 32 (or a half-multiple, as for 1/16 at 120 bpm / 48 k = 6000 = 187.5 cells, so every second slot). `ceil (ppq/div)` on a computed ppq that should be exactly `k·div` returns k or k + 1 depending on the last ulp.

Each stream therefore keeps `slotNext` (int64) and advances it by 1 per onset. It is derived from ppq **only at re-anchor**: `slotNext = ceil ((ppq_now − barOrigin)/div_eff − 1e-9)`. The ε admits a slot sitting exactly at `now`.

### 1.4 R41: Tempo clock (and ARCH gap G3)

`GrainScheduler::rescheduleNext` returns early when `nextIndex == 0`, and `reset()` arms the clock to fire on the next sample (that is the R4 contract for Free). On a Tempo-mode enable edge or a Free → Tempo switch, the first grain would fire **immediately, off-grid**, and only the second could be re-timed.

**Recommendation:** add a Tempo branch to `StreamScheduler` that computes onset times directly:

```
t_k   = anchor.abs + ((barOrigin + slot_k · div_eff) − anchor.ppq) / anchor.pps      (double, samples)
        + J · (u_k − ½) · I,        I = div_eff / anchor.pps,  u_0 := ½,  u_k keyed on epoch + k (unchanged)
emit k when t_k < pieceEnd:  sample = max (pieceStart, ceil (t_k)),  frac = sample − t_k  (frac ∈ [0, 1) except a late re-anchor → 0)
```

- **Telescoping jitter is kept:** the displacement of onset k is the same `J·(u_k − ½)·I` as in Free, so the mean rate is exact and jitter 0 is exactly on the grid (Q2).
- **Ordering:** `t_{k+1} − t_k ≥ I·(1 − J) ≥ 0`. Clamp it to ≥ 1 sample (the Free `kMinInterval`) at J = 1.
- **Re-time is automatic:** a new anchor changes `t_k` for the pending k. "> 0.5 sample off" (ARCH §17) becomes "recompute every piece", which is exact with no threshold.
- **Epochs:** Free ↔ Tempo switches fold the onset count into `epoch` exactly like the R4 enable edge, so draws never replay.
- **Enable edge in Tempo:** the first onset is the next slot ≥ now, not "now".
- **Free mode:** untouched. It keeps `GrainScheduler`, so the N2 null holds bitwise.

This needs no grain-engine change. G3 stays optional.

### 1.5 R42: discontinuities and transport states

`ramp.py`: the worst prediction error of a block's ppq from the previous block's `(ppq, bpm)` is shown below. A trapezoid assumes the host reports the instantaneous bpm at block start and the tempo is linear within the block.

| Ramp | Block | Constant old pps | Trapezoid |
|---|---|---|---|
| 60 → 180 bpm / 2 s | 512 / 4096 | 2.7 / **161** samples | < 4e-12 |
| 90 → 180 bpm / 8 s | 512 / 4096 | 0.34 / 21.6 | < 2e-11 |
| 120 → 121 bpm / 4 s | 512 / 4096 | 0.006 / 0.36 | < 3e-11 |

**Rules, evaluated per host block** (the playhead is read once per block):

| Condition | Action |
|---|---|
| No playhead, no ppq, or `bpm ≤ 0` | bpm 120, not playing (virtual ppq) |
| Not playing | **Ignore host ppq**; keep the anchor and let the virtual ppq run at host bpm (re-anchor on a bpm change). LFOs and the grid free-run seamlessly |
| `isPlaying` rising edge | Re-anchor to host ppq and **re-lock** (LFO accumulators, slots). This is the D15 / spec "restarts at transport start" |
| Playing, `\|host − trapezoid\| > 1 ms` | Discontinuity (loop, locate): re-anchor and **re-lock** |
| Playing, `\|err\| > 0.5 sample` or bpm changed | Re-anchor only (tempo change: the grid re-times, LFO direct stays continuous) |
| Otherwise | Keep the anchor (the constant-tempo harness never re-anchors) |

**Negative ppq (pre-roll):** all grid maths uses `floor`/`ceil` in double and `frac (x) = x − floor (x)`, never `fmod` or an int cast. Grains run through a count-in. Whether Logic reports `isPlaying` during count-in is a D16 DAW check item: it is not verified here.

---

## 2. Tempo divisions (R44, R45)

**R44, bars.** `sN_size_sync` has 1–2 bars, `sN_density_sync` 1 bar and `lfoN_rate_sync` 1–8 bars, while the spec defines divisions as quarter-note-relative. The harness mock hard-codes 4/4.

- **Recommended:** `barLen = 4·num/den` quarters from `getTimeSignature()` (4/4 if absent). `barOrigin = fmod (getPpqPositionOfLastBarStart(), barLen)` (floor-based), latched at each anchor. All grid positions are computed relative to `barOrigin`. Under constant 4/4 this is identical to the plain `ppq/div` grid.
- **Alternative:** fixed 4/4 from ppq 0, documented in the tooltip.
- The choice changes the D16 checklist (a 3/4 project). **Sign-off at plan.**

**R45, ranges.**

| Quantity | Tempo value | Bound | Consequence |
|---|---|---|---|
| Size | `divBeats·60000/bpm · 2^(4·off)` (continuous) | [5, 2000] ms (`kMinSizeMs`/`kMaxSizeMs`; reach `kMaxReachS` = 10 s at rate 4 × 2 s = 8 s) | "1 bar" = 2000 ms at 120 bpm. Below that, 1 bar and 2 bars **clamp to 2 s**. Raising the cap breaks the reach/ring budget. **Recommend: clamp + UI note (sign-off)** |
| Density | `1/divSeconds · 2^round(4·off)` | [0.5, 200] g/s | Clamp the exponent: `e = clamp (round_tiesUp (4·off), ceil (log2 (0.5/d0)), floor (log2 (200/d0)))`, so the result stays a power of two and on the grid |

`computeStreamTargets` (demand, quotas, R5 coherence) currently reads `st.density` / `st.sizeMs`. In Tempo mode it must read the **effective** values. Otherwise quotas are computed for the Free knobs while the clock runs the synced division.

---

## 3. Matrix evaluation and the N2 null (R46)

`rt.cpp` reproduces JUCE's float formulas exactly (`pow (p, skew)` / `exp (log (p)/skew)`):

| Destination range | Round-trip non-identity | Max rel err |
|---|---|---|
| input_gain, mix, pitch, window (linear, exact) | 0 % | 0 |
| s_pan (−100..100) | 18.0 % | 1.4e-7 |
| **s_level (−60..6)** | **25.0 %** | 0.25 (values near 0 dB) |
| buffer_length (0.5) | 40.4 % | 2.3e-7 |
| size / density / lfo rate (0.3) | 59–66 % | ≤ 6.5e-7 |
| filter_cutoff / ring_freq / fb cuts (centre skews) | 47–72 % | ≤ 6.3e-7 |

If every destination went through the matrix, the N2 null would fail on the first cell, and the zero-depth positive control would differ.

**Rule:** a destination is *routed* in a cell iff its summed offset (slots + residual, R49) is non-zero. **Unrouted → the raw 2.4 value, untouched.** A zero-depth slot, a source sitting at exactly 0, or a route whose depth has ramped to 0 all fall back to the raw path. The switch between raw and round-trip is ≤ 6.5e-7 relative (≤ 0.25 at s_level near 0 dB, i.e. ~1e-6 dB), which is inaudible and deterministic.

Evaluation per routed destination d:

```
off_d   = Σ_slots depth_ramped · src  +  residual_d          (sum BEFORE the single clamp)
v01_d   = clamp (base01_d + off_d, 0, 1)
raw_d   = range_d.convertFrom0to1 (v01_d)                     (skew respected; float, JUCE formula)
```

`range_d` is built from the `GranulatorParamIds` contract rows (start, end, skew, or `setSkewForCentre`). This is the same source as `createParameterLayout`, so the base01 equals the host's normalised value. Interval is 0 everywhere and there are no lambda ranges (`PluginProcessor.cpp:149`), so there is no snapping trap.

---

## 4. Ramped vs spawn-latched destinations (R47, R48)

**R47, ramped destinations.** The modulation goes **after** each existing base ramp, in that ramp's own domain:

| Destination | 2.4 ramp domain | engine → raw | raw → engine |
|---|---|---|---|
| input_gain, output_gain | gain (`dbToGain`) | `20·log10 (g)` | `dbToGain` |
| sN_level | gain (`levelDbToGain`) | `g ≤ 0 → −60`, else `20·log10 (g)` | `levelDbToGain` (−60 → exactly 0) |
| mix | 0..1 | ×100 | /100 → `mixGains` (snapped ends) |
| sN_pan | −1..1 | ×100 | /100 → `panGains` |
| feedback | `0.95·x` | /0.95 ×100 | /100 × 0.95 |
| fb_saturation | `1 + 7·x` | (d − 1)/7 ×100 | inverse |
| fb_diffusion | 0..1 | ×100 | /100 |
| fb_low_cut / fb_high_cut | log2 Hz | `exp2` | `log2` → `updateChainCutoffs` |
| reverb_mix/size/damp | none (atomics; ReverbUnit smooths) | — | write the modulated value into `revMix/revSize/revDamp` |
| lfoN_rate | none (control) | — | feeds the LFO increment (R50) |

Per routed ramped destination per cell: `engine → raw → base01 → +off → raw → engine`. This costs one log/exp pair plus one pow. Per sample, `CellLerp (prevMod, curMod)` stands in for `ramp.value (i)`. Level and pan already enter as the per-cell `streamGainL/R` product, so they only substitute values. input_gain needs its own `CellLerp` inside `CaptureWriter`. Base knob jumps keep their 10/20 ms ramps, and the modulation is never low-passed by a re-targeted ramp. Re-targeting `ExactRamp` every cell with a 30-cell length would act as a ~20 ms one-pole: 0.37× amplitude at 20 Hz.

**R48, spawn-latched destinations** (the 22 per-stream fields other than level/pan, plus buffer_length):

- Per tick, routed destinations keep `offPrev, offCur` and `base01` (from the latched raw `gp` value).
- At spawn, at piece offset o, use `off = offPrev + (offCur − offPrev)·(o/32)`, then `raw = convertFrom0to1 (clamp (base01 + off))`, then engineering units. Percentages are /100, pan and skew are /100, and `spanSamples = raw·fs` for buffer_length. The results go into a **local copy** of `GridParams::Stream`, which is passed to `Spawner::plan`. Unrouted fields are copied bitwise.
- Tick-rate consumers read the modulated `cur` value: `computeStreamTargets` (density, size, pitch, pitch_rand, spray → quotas, R5 coherence → g_out) and the Free scheduler interval `fs / density`. `GrainScheduler` latches the interval per onset, so density modulation reaches the gap after the next onset (the ARCH L-class rule).

---

## 5. Slot ramps and route changes (R49)

| Event at a tick | Action |
|---|---|
| depth target changes, route unchanged | the slot's `ExactRamp` (20 ms, `rampCellsFor (fs)`) |
| source or destination changes | `residual[oldDest] += depth_cur · src_cur(old source)` then `residualRamp[oldDest].setTarget (0, 20 ms)` from the new start; the slot restarts at depth 0 and ramps to target |
| either change on a slot whose old dest is None / source None | no residual |

The residual uses the **frozen** contribution at the change instant rather than a live source. The difference over 20 ms is inaudible, and it buys bounded memory: 113 floats + 113 ramps, any number of changes per cell. `ExactRamp::setTarget` early-outs on an identical target, so re-arming needs `reset (newStart)` followed by `setTarget (0, cells)`.

Route changes are detected by comparing the latched choice indices at ticks (no listeners). Removing a route goes `depth·src → residual → 0` linearly, so it is value-continuous. Once both are 0, the offset is exactly 0 → raw path (R46).

---

## 6. LFOs (R50, R51)

**R50 — D15 refinements (Q3):**

1. **Free mode always uses the accumulator.** The direct formula needs a constant increment. Automating `lfoN_rate` from 0.5 to 0.51 Hz after 60 s jumps the phase by `n·Δinc` = 60·48000·(0.01/48000) = **0.6 cycles**. That applies to host automation and knob moves too, not just to matrix modulation. The accumulator is block-size invariant anyway (one deterministic advance per absolute cell), and Free has no external reference to lock to.
2. **Tempo uses Direct while unmodulated:** `φ = frac ((ppq − barOrigin)/divBeats)` on the R39 anchor, whether playing or on the virtual ppq while stopped. A change of `rate_sync` jumps to the new grid phase. That is intended, because a synced LFO stays locked.
3. **Predicate (Q3):** an offset ≠ 0 on destination 108 + N in this cell (slots + residual) flips Direct → Accum, with the accumulator seeded from the direct phase on every Direct cell. **Accum → Direct happens only at a re-lock** (R42: transport start, loop, locate, prepare). The predicate therefore only matters on its rising edge, and removing a route mid-ramp needs no special case.
4. **The phase parameter stays outside the accumulator:** `out = shape (frac (base + φ_ramped/360))`. `φ` is ramped over 20 ms on the shortest arc, so 359° → 1° moves +2°, not −358°.
5. **No source cycles:** rate offsets come from the **previous** cell's matrix. LFO1 → LFO1 rate, or LFO1 ↔ LFO2, are then well defined, with a one-cell lag.
6. **Accumulator** `{int64 cycle; double frac}`. Free: `frac += 32·rate_Hz/fs`. Tempo: `frac += 32·(pps/divBeats)·2^(4·off)` (continuous, D15). Carry into `cycle` on wrap.

**R51 — shapes (bipolar):** sine `sin (2πφ)` · triangle `1 − 4·|φ − ½|` · saw `2φ − 1` · square `φ < ½ ? 1 : −1` · S&H `2·h(c) − 1` · smooth random `2·lerp_cos (h(c), h(c+1), φ) − 1`, where `h(c) = Draw (seed, kLfoStream + N, uint64 (c)).nextFloat (0)`.

- `kLfoStream` is a **new stream base** (for example `0x0A1F'0000`). No `Slot` entry moves, so the N2 null holds.
- c is the cycle index: `floor` of the direct phase, or the accumulator's `cycle`. Negative c in pre-roll casts to a large uint64, which is deterministic. In Tempo-direct mode, S&H repeats on every loop pass, which is musically useful and comes for free.
- Values are sampled once per cell, and ramped destinations lerp inside the cell. Square, saw and S&H edges become 0.67 ms ramps. That is by design and is **excluded** from the zipper gate, which uses sine and triangle.

---

## 7. Envelope follower and macros (R52)

- **Tap:** the conditioned input, `0.5·(L + R)·inGain(i)`, taken at the `CaptureWriter` input (post input_gain, pre-ring). It sees neither the feedback return nor the dry path, and it runs while frozen too.
- **Per sample:** `x = |s·envGain|`, `e += (x > e ? aA : aR)·(x − e)`, with `a = 1 − exp (−1/(τ·fs))` computed in double at ticks from `env_attack`/`env_release`/`env_gain` (raw ms and dB).
- **NaN guard:** `if (! std::isfinite (e)) e = 0`, once per piece.
- **Source at the tick:** `clamp ((20·log10 (max (e, 1e-30)) + 60)/60, 0, 1)`. Silence gives exactly 0, i.e. unrouted (R46).
- **Macros:** `macroN` raw/100 through an `ExactRamp` (20 ms). Unipolar.

---

## 8. Destination semantics (R53–R55)

**R53 — skew (Q4)**, measured with ±10 % normalised around the defaults:

| Destination @ base | +10 % | −10 % |
|---|---|---|
| s_size @ 100 ms | 204 ms (×2.04) | 41.5 ms (×0.415) |
| s_density @ 10 g/s | ×2.04 | ×0.415 |
| filter_cutoff @ 1 kHz | ×2.29 | ×0.377 |
| filter_cutoff @ **20 kHz (default)** | **×1.00 (clamped)** | ×0.589 |
| lfo_rate @ 0.5 Hz | ×2.40 | ×0.313 |
| buffer_length @ 8 s | ×1.39 | ×0.680 |

The swings are near-symmetric in log terms, which is the musically right behaviour. A destination at a range end is rectified: a bipolar LFO onto the default cutoff only closes the filter. This is acceptable as specified (ARCH: "skew respected"). **3.3 note:** rings draw `v01`, so they are symmetric on screen.

**Q5 — spawn-latched aliasing:** a latched destination samples the source once per grain. A 20 Hz LFO at 10 g/s produces a beat pattern. That is the defined behaviour (ARCH "latched at spawn"), not zipper, and it has no within-grain step by construction. QUAL-03 judges latched destinations on the trajectory of the per-grain values, not on a smoothness bound.

**R54 — reverb:**
- `ReverbUnit::process` clears the tank when it re-activates. A modulated `reverb_mix` that sits ≤ 0.001 for longer than the 50 ms `activeGain` fade therefore loses its tail every LFO cycle, which sounds like gated reverb.
- `reverb_size` and `predelay` are smoothed over 0.1 s inside the unit, so fast size modulation is attenuated (and pitch-warbles the tank, as size modulation always does).
- Accept both. Add them to the D16 listening list.

**R55 — Choice-backed destinations (Q7):** none. `GranulatorParamIds.h:282` states that every entry other than None is Float, and the contract confirms it. `tests/golden/mod-dests.tsv` (Stage 1) already pins index ↔ ID ↔ host string, so the plan reuses it.

---

## 9. Test infrastructure and gates (R56, R57)

**R56 — the mock playhead must be rebuilt before any FUNC-04 or PERF-03 row counts:**

| Need | Current `MockPlayHead` | Required |
|---|---|---|
| Tempo change | `ppq = t/fs·bpm/60` → a bpm change **jumps ppq** | piecewise tempo map, ppq integrated |
| Loop / locate / pre-roll | none | event list: `loop [a, b)`, `locate (t, ppq)`, start at negative ppq |
| Time signature | 4/4 constant, no last-bar-start | `num/den` + `ppqPositionOfLastBarStart` (R44) |
| Transport stop/start | `playing` flag, ppq frozen | as now, plus start/stop events |
| Block splitting | none | the harness **cuts host blocks at every event sample** (hosts report a position only at block start), so all block-size variants see events at identical absolute samples |

A tempo **ramp** cannot be block-size invariant, because the host reports one bpm per block. PERF-03 uses tempo **steps** at split points, and the ramp is a D16 DAW item.

**FUNC-04a oracle:** the exact onset instant `sample − frac` vs the analytic grid time from the mock's tempo map. That gives |Δ| ≤ 1e-6 sample at jitter 0, which is far tighter than "within 1 sample", and catches R40-class off-by-one slots. Run every density division × {constant, tempo step, loop, locate, pre-roll, 3/4 if R44 is approved}.

**R57 — QUAL-03 / FUNC-06:**
- Add a test hook that records the per-sample engine value of the routed destination (mix gain, level·pan product) and the per-spawn latched values.
- Ramped: max |Δv| per sample ≤ 1.05 × `depth·2π·f_LFO/fs` (normalised, sine) mapped through the destination slope.
- Route removal: max |Δv| at and after removal ≤ the same bound (residual ramp slope `|c|/(20 ms·fs)` included).
- The audio step-click gate (memory: same-motion control, tonal source) is the secondary check, because the trajectory gate cannot be confounded by the source.

**N2 null:** reuse `tests/n1/run.sh` as `tests/n2/` with `OLD_SHA = 42d30b01`. The four controls from CONTEXT apply. The zero-depth control passes by construction under R46, so assert it is **identical**.

---

## 10. Cost and telemetry (R58, Q6)

| Item | Per second @ 48 k | Estimate |
|---|---|---|
| 16 slots × accumulate, per cell | 24 k mult-adds | ≈ 0 |
| Routed conversions (≤ 16 + residuals), per cell | ≤ 24 k pow/log | < 0.1 % |
| Spawn-latched conversions (≤ 22 per spawn, ≤ 800 spawns/s) | ≤ 18 k pow | < 0.05 % |
| 4 LFOs per cell (sin / hash) | 6 k | ≈ 0 |
| Env follower per sample | 48 k × ~4 flops | < 0.05 % |
| Telemetry, all 113 via `convertTo0to1` every cell | 170 k pow | **≈ 0.3 %**, so dirty-check unrouted on the raw value instead |

**Sparse form** (Q6): rebuild an active-destination list (a 113-bit set) per cell from the slots plus the non-zero residuals. Only those go through §3 and §4. **C5 ≈ C4 (17.75 %) + < 1 % ≈ 19 %**, against a 30 % budget.

Telemetry layout follows CONTEXT:
- 113 destination atomics (relaxed, written per cell: routed = `v01`, unrouted = `base01` on change)
- 11 source atomics: LFO 1–4 ∈ [−1, 1], Env/Macros ∈ [0, 1], Pitch/Onset = 0 in 2.5a

No APVTS writes anywhere. The host-automation gate stays the CONTEXT row (pedalboard + CFRunLoop pump). Nothing in 2.5a writes a parameter, so it should pass trivially, and it is still run.

---

## Decisions needed at plan (sign-off)

1. **R44:** use the host time signature for bars (recommended), or fixed 4/4.
2. **R45:** clamp Tempo size at 2000 ms, so 1–2 bars clamp below 120/240 bpm (recommended), or extend the size cap and re-budget reach.
3. **R50:** Free LFOs always use the accumulator. This amends D15 (Direct only in Tempo).
4. **R49:** per-destination residual instead of a per-slot live shadow. This amends the ARCH §16 wording.

## Plan inputs (ordering)

1. N2 neutral null vs `42d30b01` first (tests/n2), before any Source change counts.
2. Mock playhead + block splitting at events (R56). It is a prerequisite for every Tempo row.
3. Tempo tracker (anchor, R42 rules) → Tempo clock in `StreamScheduler` (R41) → effective density/size (R45).
4. Sources: LFO (R50/R51), env (R52), macros. Source telemetry.
5. Matrix: range table from contract rows, sparse eval (R46), residuals (R49), ramped hooks (R47), latched spawn copy (R48), reverb atomics (R54), destination telemetry (R58).
6. Gates: FUNC-04a/b, FUNC-06, D15, QUAL-03 (R57), PERF-03 (steps, splits), PERF-01, QUAL-02 stress, telemetry, C5, host automation.
7. Update the stage comments in `Engine.h` / `StreamScheduler.h` (carry-in).
