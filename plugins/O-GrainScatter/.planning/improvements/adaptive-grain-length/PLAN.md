---
milestone: adaptive-grain-length
domain: mixed
execute_agent: general-purpose
version_bump: minor
base_version: 3.4.0
target_version: 3.5.0
created: 2026-10-28
---

# Plan: Signal-adaptive grain length in freeze (extrema density)

**Plugin:** O-GrainScatter
**Milestone:** adaptive-grain-length
**Created:** 2026-10-28
**Phase:** Plan

---

## Summary

**Improvement:** a new `grain_adapt` knob ("Adapt", 0–100 %, default 0). While Freeze is held, each new grain's length follows the local extrema density of the snapshot (Nielsen DAFx26 span L). Grain Size stays the mean length. Adapt 0 renders bit-exact against v3.4.0.

**Approach:** `ExtremaMap` (new header) analyses the mid channel of the held loop with a B-spline derivative, ε = 0.001 hysteresis on vₘ, and reverse-linear sub-sample positions. The analysis is chunked at R = 32 work units per output sample and finishes at a deterministic `readyAbs`. K0 is fixed per snapshot (time-median span = 100 ms). Each block solves a calibration constant c so that the clamped mean equals GS. Each spawn then sets `lenA = clamp(GS·exp(a·(lnL − lnMed) + c), 10 ms, 500 ms)` through an explicit `grainAdapt > 0 && frozen && ready` branch.

**Version:** 3.4.0 → 3.5.0 (MINOR: one new parameter, AU version hint 5, no breaking change)

**Binding decisions from RESEARCH (do not re-litigate during execute):**
- **Store:** one `ExtremaMap` owned by `FreezeManager` for the current capture, not one per slot.
- **Source:** read ring level 0 via `delayBuffer.getRing().copyLevel(0, ch, origin + b, n, scratch)`. No grain-engine module change.
- **Scatter:** stays scaled by the Grain Size knob, not the adapted length.
- **Size Random:** multiplies the adapted length, using the same draw (`CounterRng::SlotSize`) under the same condition.
- **Length units:** output samples. Do not divide by the playback rate.
- **Pre-roll guard:** unchanged, still computed with the fixed length.
- **Reverse grains:** anchor at the fixed-length start `p_rev = frozenBase + fixedLen − 1`. Measure the span backward. Then set `frozenBase = p_rev − (len − 1)`.
- **Tilt:** move `tiltPeakFor` after the adapt block. It is a pure function, so Adapt 0 is unchanged.
- **Analysis runs on every engage, even at Adapt 0.** It writes only private state.
- **Fallback to fixed length** when M < 8, when the loop is shorter than 0.2 s, or when `absSample < readyAbs`.
- **Bit-exactness at Adapt 0:** never compute `pow`/`exp`/`log` unconditionally.

---

## Task Breakdown

### Wave 1: DSP (dsp-agent writes; orchestrator builds and runs gates)

> **Executed 2026-10-28** by the orchestrator inline (Tasks 1–6 done; see SUMMARY.md for deviations: V1 at Scan 30 with 16 × 8 s, R1 probe, 58 px / 3 px cells, FR "Inclin.").

#### Task 1: `Source/dsp/ExtremaMap.h` — analyser, calibration, lookup

**Outcome:** a self-contained, JUCE-free, allocation-free class, ported from `research-scripts/extrema_proto.cpp` with the wrap fix from RESEARCH Q1.6.

- `prepare(maxLoopSamples, chunk)` sizes `float positions[maxLoopSamples]` (hard bound M ≤ N). It also sizes a 2 × (chunk + 4) stereo scratch and P = 256 doubles for `lr` and the prefix sums. This is the only place that allocates.
- `begin(origin, length, xfade, loopLength, engageAbs)` is O(1). It sets the state to `Analysing`, computes `W = loopLength + 5·4096` and `readyAbs = engageAbs + ceil(W / 32)`, and bumps `generation`.
- `step(ring, blockEndAbs)` runs work units until the cumulative quota `(blockEndAbs − engageAbs)·32` is used up.
  - Phases: warm-up over the last 2048 loop samples (not stored); main pass `i = 1 .. loopLength` with circular neighbours; then 5 calibration units of 4096 each (4 K0 fixed-point steps plus the final sort of `lr`).
  - Reads go through `copyLevel` in chunks. The seam crossfade is applied exactly as `FreezeManager::locate` does, including the second copy for the `b − loopLength` half.
- `cancel()` and `clear()` set the state to `Idle`.
- `ready(abs)` is true when the state is `Done`, `abs ≥ readyAbs`, M ≥ 8 and loopLength ≥ 0.2·fs.
- `solveC(gs, a, minLen, maxLen)` runs a bisection on the clamped mean over the 256 sorted `lr` values, using prefix sums.
- `spanForward(p)` and `spanBackward(p)` do a binary search, compute the fractional-K0 span, and wrap modulo M with `loopLength·⌊j/M⌋`.
- `loopCoord(pos)` uses the same mapping as `locate`: `fmod(pos − xfade, loopLength)`, wrapped to ≥ 0.

**Verification:**
- [x] Compiles header-only and is included by FreezeManager.
- [x] The class has no `new`, `std::vector::push_back`, `resize` or `std::sort` on a dynamic container outside `prepare`. `std::nth_element` and `std::sort` on the pre-sized `lr[256]` are fine.
- [x] The extremum counts M for sine 55, noise and silence, and K0 for sine 55 (≈ 11) and noise (≈ 2366), match `proto_output.txt` to within ±1 %. This is checked in the Task 4 gate as a unit row (U1).

**Dependencies:** None
**Estimated effort:** Medium

---

#### Task 2: `FreezeManager` integration

**Outcome:** `FreezeManager.h` owns one `ExtremaMap`.

- `prepare` sizes it to `maxFreezeSamples`.
- `clear()`, release and `reset` cancel it. `prepareToPlay` and `reset()` must cancel the analysis (RESEARCH Q2).
- Engage calls `beginAnalysis(...)` with the capture's `origin`, length, xfade, loopLength and the engage absolute sample. Re-engaging restarts it.
- New public API:
  - `analysisStep(const DelayBuffer&, int64 blockEndAbs)`;
  - `adaptReady(int64 abs)`;
  - `adaptGeneration()`;
  - `solveAdaptC(gs, a)`, which forwards to `solveC` with the clamps 10 ms and 500 ms in samples;
  - `adaptiveLength(anchor, reverse, c, gs, a)`, which returns the clamped length in output samples as a double.

**Verification:**
- [x] An engage → release → re-engage mid-analysis sequence leaves no stale `Done` state from the previous capture: the generation bumps and `readyAbs` is recomputed.
- [x] `FreezeManager::locate` behaviour is unchanged (the freeze rows A4–A8 in Task 4 are bit-exact).

**Dependencies:** Task 1
**Estimated effort:** Medium

---

#### Task 3: Parameter, per-block hooks and the spawn branch in `PluginProcessor`

**Outcome:**

- **Parameter:** `juce::ParameterID { "grain_adapt", 5 }`, "Grain Adapt", `NormalisableRange<float>(0.0f, 100.0f, 0.1f)`, default 0, label "%". Add it to `coreGroup` after `shape_tilt` (`PluginProcessor.cpp:~206`). Cache `grainAdaptParam` next to `shapeTiltParam` (`:66`).
- **Per block:**
  - Read `grainAdapt = jlimit(0, 1, v / 100)` beside the shapeTilt read.
  - Call `freezeManager.analysisStep(delayBuffer, blockEndAbs)` right after the engage/release handling (`:~646–655`).
  - When `grainAdapt > 0` and the analysis is ready, refresh the cached `adaptC`, keyed on (grainSizeSamples, grainAdapt, generation). Skip the refresh entirely at Adapt 0.
- **Spawn (`:786–866`):**
  - Steps up to the detune stay unchanged, except that the `tiltPeak` assignment moves.
  - Insert the RESEARCH Q5 block after the frozen-base assignment (`:841–846`), before the spatial and detune steps. It uses `gp.grainLengthSamples` as `fixedLen`, the same `SlotSize` draw and condition, and re-derives the reverse `frozenBase`.
  - Then `gp.tiltPeak = GrainPool::tiltPeakFor(shapeTilt, gp.grainLengthSamples, tiltMinRamp)`.
  - The pre-roll guard keeps reading the fixed length. It already runs before the frozen base, so its order is unchanged.
- **Test hook:** under `#if GS_TEST_SPAWN_LOG`, add a fixed ring of 8192 `{absSample, grainLengthSamples, loopPos, reverse, adapted}` written at spawn, with a getter. It is compiled out of shipping builds.
- **Negative-control hook:** under `#if GS_TEST_ADAPT_FIXED_K`, use CONTEXT's K = GS·M/len with no c solve. This exists for gate row N1 only.

**Verification:**
- [x] Release VST3 + AU build clean, with no new warnings.
- [x] A shipping build does not define `GS_TEST_SPAWN_LOG` or `GS_TEST_ADAPT_FIXED_K` (grep the target's compile definitions).
- [x] The existing gates `blocksize-null`, `density-gain`, `alias-gate` and `skip-tilt` pass unchanged.

**Dependencies:** Task 2
**Estimated effort:** Medium

---

#### Task 4: `tests/adapt-gate` — new gate plus the v3.4.0 twin

**Outcome:**

- **Targets:**
  - `O-GrainScatter-adapt-gate` with `GS_TEST_SPAWN_LOG=1`.
  - `O-GrainScatter-adapt-gate-v340`, extracted from **144a73f3** with the `git archive` and `list(TRANSFORM … SOURCES)` pattern at `CMakeLists.txt:153–178`.
  - `O-GrainScatter-adapt-gate-fixedk`, with `GS_TEST_ADAPT_FIXED_K=1` and `GS_TEST_SPAWN_LOG=1`.
- **Shared code:** reuse `skip-tilt/main.cpp`'s `render`, `fixedSplit`, `variableSplit`, `setParams`, `makeInstance` and `kChangePoint = 98304`.
- **Rows** (thresholds exactly as in RESEARCH Q9):

| # | Row | Pass |
|---|---|---|
| U1 | ExtremaMap unit: M for sine 55 / noise / silence, and K0, vs prototype | ±1 % (silence M = 0) |
| A1–A8 | Adapt 0 vs v3.4.0: free, sync (repeats, Euclid, stutter gate), spatial (trajectory, Doppler), freeze @ kChangePoint, freeze + rev 50 + SizeRnd 50 + Tilt −80, freeze +24 st, freeze spatial | peak diff == 0 |
| A9 | v3.4.0 session loaded into an instance with Adapt pre-set to 50 | param reads 0, render bit-exact |
| L1 | Equal-RMS sparse/dense snapshot, a 1, GS 100: sine-half / noise-half mean length | ≥ 3.0; Adapt 0 control 1.00 ± 0.02 |
| L2 | Same at a 0.5 | ≥ 1.8 |
| M1 | Mean adapted length (pre Size Rnd), ≥ 2000 spawns, GS 50/100/200, a 1, sparse/dense + bass+snare | ±5 % of GS |
| M2 | Reverse 100 %, same | ±5 % |
| P1 | bass+snare, a 1, GS 50 → 100, same seed | per-grain ratio 2.00 ± 0.01 (unclamped) |
| F1 | Silent snapshot, −80 dB noise, loop < 0.2 s, spawns before readyAbs | bit-exact vs Adapt 0 |
| D1 | Block-size null at Adapt 100 in freeze, plus Adapt automated at kChangePoint; 32 / variable vs 512 | ≤ −120 dBFS |
| D2 | First adapted spawn's absSample across 3 splits | identical; = engageAbs + ceil(W/32) |
| R1 | operator-new counter over every processBlock after the first (engage, analysis, re-engage mid-analysis, release, Adapt 0↔100) | 0; liveness: a deliberate `new` counts 1 |
| V1 | Level: equal-RMS sparse/dense, Density 100, GS 200, a 0.5, Spread 100, 4 hash-mixed seeds, 4 s from readyAbs + 0.5 s | \|Δ\| ≤ 0.5 dB; old-vs-old ≤ 0.2 dB |
| V2 | Dense noise cloud, Adapt 100 vs 0 | ≤ 0.5 dB |
| C1 | Analysis step cost at 48 k / 96 k, 2 s noise: total µs + max per 32-sample block | report; FAIL if one block > 50 µs |
| N1 | fixed-K twin: M1 | must FAIL (expected ≈ +136 % at GS 100) |

**Verification:**
- [x] Every row prints PASS, or the expected FAIL for N1, with measured values. The gate exits 0 only when all rows hold.
- [x] The run output is saved to `tests/adapt-gate/RESULTS.txt`.

**Dependencies:** Task 3
**Estimated effort:** Large

---

### Wave 2: GUI (gui-agent writes; orchestrator runs the UI gates)

#### Task 5: Adapt knob, layout, i18n

**Outcome:**

- **`index.html`:**
  - Add a knob-container `data-param="grain_adapt"` with caption `data-i18n="label.adapt"`, in Core row 1 directly after Grain Size.
  - Add a Core-only override, scoped as `.core-group .knob-container` or the existing Core group selector, that sets the width to 57 px and the gap to 4 px. The 48 px dial is unchanged.
- **`js/app.js`:** `setupKnob('grain_adapt', …)` with the percent formatter, beside `:~429`. Double-click resets to 0, like the other knobs.
- **`js/i18n.js`:**
  - `label.adapt`: en "Adapt", fr "Adapt." with `reviewed: false`, zh "自适应" with `'mt'`.
  - `tip.adapt` in all three languages. It must say that the knob **acts only while Freeze is held**, that Grain Size stays the average, and that it takes effect about 75 ms after engaging. Glossary terms: fr *Geler/gel*, zh *冻结*.
  - Add the tooltip binding row beside `:~1653`.
- **`PluginEditor.h/.cpp`:** `grainAdaptRelay`, `.withOptionsFrom`, and a `WebSliderParameterAttachment`, following the pattern at `PluginEditor.cpp:53, 220, 294`. The relay is declared before the attachment.
- **Tests:** add the knob to `tests/i18n-states.json` if that file enumerates knobs.

**Verification:**
- [x] `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint` and `check-ui-labels` all PASS at EN, FR and zh.
- [x] Core row 1 fits within 429 px, and the row 2 captions, including FR "Inclinaison", don't overflow at 57 px. If one does, use an FR abbreviation such as "Inclin." or take ≤ 8 px from the right column's slack.
- [x] The frame stays 900 × 800 and the lower-panel budget is unchanged (`boot-all-uis` or a headless capture shows no wrap or overlap).
- [x] In Standalone, the knob moves and its readout shows % (`getScaledValue`).

**Dependencies:** Task 3 (parameter exists). Task 5 can run in parallel with Task 4.
**Estimated effort:** Medium

---

### Wave 3: Integration

#### Task 6: Full validation, version, CHANGELOG, install

**Outcome:**

- **Version:** set `CMakeLists.txt` to `VERSION 3.5.0`.
- **CHANGELOG:** add a `## [3.5.0] - 2026-10-28` entry. It documents:
  - the ~70–76 ms of fixed-length grains after each engage;
  - that adaptation fades near 10 and 500 ms (the clamp with the mean pinned);
  - the never-boost thinning at low density;
  - that a −60 dBFS ε reads quiet material as sparse;
  - the anti-phase stereo blind spot.
- **Gates:** run every gate (adapt-gate, blocksize-null, density-gain, alias-gate, skip-tilt).
- **Validation:** pluginval strictness 10 and `auval`.
- **Install:** `./scripts/build-and-install.sh O-GrainScatter`.
- **Bookkeeping:** fill in SUMMARY.md and mark the PLAN tasks done.

**Verification:**
- [x] All gates PASS, pluginval 10 PASS, and `auval -a | grep -i grainscatter` lists the AU.
- [x] The installed bundle is the dev variant, and the alternate-variant sweep is clean.
- [x] SUMMARY.md lists the measured values: C1 µs, L1/L2 ratios, M1 errors, V1 dB.

**Dependencies:** Tasks 4, 5
**Estimated effort:** Small

---

## Dependency Graph

```
Task 1 ──> Task 2 ──> Task 3 ──┬──> Task 4 (gate) ──┐
                               │                     ├──> Task 6
                               └──> Task 5 (UI)  ────┘
```

**Parallelizable:** Task 4 and Task 5, once Task 3 builds.

## Execution Order

| Order | Task | Agent | Dependencies | Parallel with |
|---|---|---|---|---|
| 1 | 1 ExtremaMap | dsp-agent | — | — |
| 2 | 2 FreezeManager | dsp-agent | 1 | — |
| 3 | 3 Processor | dsp-agent → orchestrator build | 2 | — |
| 4 | 4 adapt-gate | general-purpose (needs Bash) | 3 | 5 |
| 4 | 5 UI | gui-agent → orchestrator gates | 3 | 4 |
| 5 | 6 Integration | orchestrator | 4, 5 | — |

**Note on agents:** dsp-agent and gui-agent have no Bash, so they write code only. The orchestrator, or a general-purpose agent, builds, runs gates and iterates. That is why `execute_agent` is `general-purpose`.

---

## Risk Notes

1. **Adapt 0 bit-exactness lost to the tilt move or an unconditional transcendental.**
   - Impact: rows A1–A9 fail, which breaks the milestone's #1 priority.
   - Mitigation: the explicit `> 0.0f` branch (Task 3). Tilt keeps the same arguments. Rows A1–A9 against the 144a73f3 twin.
2. **Analysis state survives a re-engage or reset, so grains use a stale map.**
   - Impact: wrong lengths, or reads of positions from the old capture.
   - Mitigation: a generation bump on `begin`, and cancel in `clear`, `reset` and `prepareToPlay` (Task 2). Re-engage mid-analysis is in R1/D1.
3. **Block-size dependence through the analysis quota.**
   - Impact: the D1 null breaks.
   - Mitigation: the quota is absolute (`blockEndAbs − engageAbs`), so `readyAbs` is deterministic. D2 checks it exactly.
4. **Caption overflow at 57 px.**
   - Impact: check-ui-labels FAIL.
   - Mitigation: FR abbreviation, or borrow from the right column's slack (Task 5).
5. **Long sparse-region grains with scatter on the knob may smear.**
   - Impact: heavy overlap on sparse passages.
   - Mitigation: the ±6 ct detune and random offsets, plus a listening check in verify. The lever, rescaling the same draw by `max(1, len/GS)`, is deferred to verify.
6. **CPU on older x86 is 2–3× the M4 numbers.**
   - Mitigation: the C1 report. R = 32 keeps the 96 k/32 block under ~25 µs at 3×.

---

## Domain Agent Instructions

**Execute agent:** general-purpose orchestration. dsp-agent writes Tasks 1–3 and gui-agent writes Task 5.

**Domain rules:**
- No allocation in processBlock, engage, `analysisStep` or spawn. Everything is sized in `prepare`.
- No new RNG draws. Re-reading `draw.nextFloat(CounterRng::SlotSize)` is keyed, so it returns the same value.
- Keep the Adapt 0 path textually identical where possible. Gate any new math behind `grainAdapt > 0.0f`.
- WebView: the relay is declared before the attachment. Readouts use `getScaledValue()`.
- Path-scope commits (`git commit -- plugins/O-GrainScatter`). Never `git add -A`. Do not tag.

**Files to read first:**
- `.planning/improvements/adaptive-grain-length/RESEARCH.md`: every decision and number.
- `.planning/improvements/adaptive-grain-length/research-scripts/extrema_proto.cpp`: the reference analyser and calibration to port.
- `Source/dsp/FreezeManager.h`: `locate` (`:168`), the capture/engage path and `getSpawnBase`.
- `Source/PluginProcessor.cpp`: params `:195–215`, per-block reads `:~568`, engage `:646–655`, spawn `:786–866`.
- `Source/dsp/GrainPool.h`: reverse `readPosition` init `:302`, frozen read `:377`.
- `modules/synthesis/grain-engine/cpp/BandlimitedReader.h`: `MipRing::copyLevel` `:377–392`, `MipSnapshot::captureFrom` `:494`.
- `tests/skip-tilt/main.cpp` and `CMakeLists.txt:153–179`: the gate harness and the twin-target pattern.

---

## Success Criteria

1. [ ] Adapt 0 is bit-exact vs v3.4.0 on all paths, and a v3.4.0 session loads at Adapt 0 (A1–A9).
2. [ ] Content-following lengths: sparse/dense ratio ≥ 3.0 at a 1 and ≥ 1.8 at a 0.5 (L1/L2).
3. [ ] Grain Size is the mean within ±5 % (M1/M2), lengths scale proportionally where unclamped (P1), and the negative control fails (N1).
4. [ ] Level flat within 0.5 dB, with the control at ≤ 0.2 dB (V1/V2).
5. [ ] 0 allocations (R1), block-size null bit-exact (D1/D2), engage cost reported (C1).
6. [ ] UI gates PASS in EN/FR/zh, and the frame is unchanged.
7. [ ] Existing gates, pluginval 10 and auval PASS.

---

*Generated by improve-milestone plan phase*
