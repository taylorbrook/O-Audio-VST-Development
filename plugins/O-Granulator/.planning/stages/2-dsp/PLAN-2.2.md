# Stage 2 — DSP · Phase 2.2 (Four streams, randomisation, pitch/scale, window morph, normalisation) — Plan

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.2 of 6
**GSD phase:** plan
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `88e3af98`
**Inputs:**
- `CONTEXT-2.2.md` (D4, D5 locked, carry-ins, execution defaults)
- `RESEARCH-2.2.md` (R5–R12, Q1–Q4, §7 gate recipes). **Where it differs from ARCHITECTURE.md or CONTEXT-2.2, RESEARCH-2.2 wins.**
- `SUMMARY-2.1.md`, `VERIFICATION-2.1.md` (carried gap and errata), `RESEARCH-2.1.md` §9 (gate recipes reused)
- `research/ARCHITECTURE.md` §4–§11, Algorithm §5; `ROADMAP.md` Phase 2.2; `parameter-spec.md` (LOCKED)
- The 2.1 engine in `Source/dsp/`, `PluginProcessor.*`, `tests/render-harness/`

---

## Entry Check — contract checksums

Re-hashed this session. All six match STATUS.md frontmatter.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `c74b5ebc…` | ✅ |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `79d69576…` | ✅ |

No drift.

---

## Goal

Turn the one-stream skeleton into the full grain parameter set: four streams sharing one pool. The phase adds:
- stream-fair stealing over parameter-derived, water-filled quotas;
- the 65-step WindowMorphBank with skew;
- the D4 scale quantiser over pitch-collections 1.0.0, with Custom read from an audio-side copy;
- stereo spread and gain rand;
- the R5 partial-coherence output law, which replaces the 2.1 incoherent law and the ARCHITECTURE κ blend;
- a stream mixer with level, pan, mute and solo, each on a 10 ms ramp.

At the end of 2.2, every ROADMAP 2.2 gate passes in the render harness, together with the steal-free bit-identity row carried from VERIFICATION-2.1 and the R12 2.1-equivalence null. The plugin installs and passes pluginval 10 and auval. There is no human listening checkpoint (CONTEXT D1–D3).

---

## Plan-phase decisions

- **P9 — The steal-free row lands first, on the unmodified 2.1 tree** (RESEARCH §7). T1 is a separate commit, made before any spawner, pool or law change. Every later wave must keep it green.
- **P10 — The output law is R5, not ARCHITECTURE §10.** There is no dB κ-blend and no `getForwardNorm`. The law is the derived partial-coherence form `P = max(c·S2, (c·S1)²) + (1 − c)·S2`, with c from (reverse, spray, |pitch|, pitch_rand / scale unison).
  - Constants: W₀ = 5 ms, p₀ = r₀ = 0.5 st. They are **not** retuned against a single source (RESEARCH §1.4).
  - Errata go into SUMMARY-2.2: ARCHITECTURE §10's κ variables are superseded, and the E0 worst case moves from P/μ₂ 43.02 to **333**.
- **P11 — Custom table: non-blocking try-lock, no new double buffer** (RESEARCH §4 recommendation). At the grid, when `customVersion` changes, the audio thread does `ScopedTryLockType` on the existing `customLock` and copies the table (≈ 264 B). If the try fails, it retries at the next grid.
  - This supersedes CONTEXT's "double buffer + version". It reuses state that already exists, and it never waits.
  - The engine reaches the processor's masks, lock, table and version through a `ScaleSource` pointer bundle that is bound once in `bind()`.
- **P12 — Mixer ramps are 10 ms.** Level, pan and the mute/solo audibility gain move from 20 ms to 10 ms. Add `rampCellsFor(ms)`; at 48 k, 10 ms is 15 cells.
  - Input gain, output gain, mix and g_out stay on the 20 ms base ramp.
  - Consequence: level and pan automation renders differ from 2.1. Static renders, and R12, are unaffected. This is a deliberate gate change, not a regression.
- **P13 — Grain layout:** add `float tiltT, tiltA, tiltB` to the window-morph section of `Grain`, and latch `makeTilt(0.5 + 0.45·skew)` at spawn. `windowSkew` stays as telemetry. Add `float pitchCents` (the latched scale or random offset) for the record-based DSP-01 row. The POD stays trivially copyable. This is a layout addition, not a renumbering.
- **P14 — Hann fast path.** When the latched `windowStep == 0 && skew == 0`, the renderer keeps `hannWindow()` and the law keeps `hannCumW2`. This keeps E0 exact and makes R12 possible (R6). Every other step uses table lerp + warp.
- **P15 — Per-stream probes go through solo.** Muted and non-soloed streams keep spawning and keep their quota. A solo-s render therefore isolates stream s inside the same shared-pool run, so the FUNC-03 starvation rows need no extra bus tap. A dedicated row proves that solo changes no pool state (the spawn logs are identical with and without solo).
- **P16 — New gate rows go in `tests/render-harness/engine_gates_22.h`.** `engine_gates.h` is already 963 lines. The 2.1 rows stay where they are, unchanged, except E0. E0 gains rows and keeps its old ones (the 2.1 incoherent function is kept in the harness as the reference for the bitwise reduction).

---

## Source layout (deltas against 2.1)

| File | Change |
|---|---|
| `Source/dsp/EngineConstants.h` | `kMixerRampMs = 10`, `kCoherW0Ms = 5`, `kCoherP0St = kCoherR0St = 0.5`, `kGainRandDb = 24`, `kWindowSteps = 65`, `kWindowPoints = 2048`, `kMaxActive = 512`. Slots stay append-only (Pan and Gain are already reserved, so nothing is renumbered) |
| `Source/dsp/Grain.h` | P13 fields |
| `Source/dsp/ControlGrid.h` | `rampCellsFor(fs, ms)`. `ParamPointers` / `GridParams::Stream` gain mute, solo, scale (choice int), window, windowSkew (→ [−1, 1]), spread, gainRand |
| `Source/dsp/WindowMorphBank.h` **(new)** | 65 × 2048 float tables, plus `C1/C2[65][2049]` cumulatives (trapezoid-integrated in double from the float table, stored as float) and per-step μ₁/μ₂. `read(step, tilt, phase)`; `cum(k, step, tilt, x)`, two-piece warped (R6). Built in the constructor on the message thread |
| `Source/dsp/OutputLaw.h` | `Coherence` (c from latched params), `streamPower(N, q, step, tilt, c, spreadS)` → P per R5 §1.2, `streamOutputGain(...)` → g_out. Hann closed form when P14 applies. The 2.1 functions stay, for E0 and the reduction row |
| `Source/dsp/Quotas.h` **(new)** | Closed-form water-fill over 4 demands (`N_s = density·len̄`, 0 when disabled) → `double q[4]` and `int qCeil[4]` |
| `Source/dsp/ScaleQuantiser.h` **(new)** | Per-stream `ActiveSet` (sorted `float cents[kMaxActive]`, count, rebuild key). `rebuildIfNeeded(scale, mask, spanCents, customVersion, const Table&)`: tile the degrees across periods inside [−span, span], test the mask bit with the **original** degree index, then insertion-sort (R9). `window(pitch)` → [lo, hi) via two `lower_bound`s. `pick(draw, lo, hi)` |
| `Source/dsp/Spawner.h` | `plan()`: D4 pitch path, spread (Slot::Pan), gain rand (Slot::Gain), window step and tilt latch. The fixed draw order is Size, Pitch, Reverse, PosSpray, Pan, Gain (R10), and every draw happens before `acquire()`. `acquire()`: count `live[4]` in the existing scan; progress = `2·overQuota[g.stream] + (g.stream == spawning) + age·invLen`, with `overQuota = live_s > qCeil_s` (R8); progress′ keeps the 2.1 kill term |
| `Source/dsp/Engine.h/.cpp` | Remove the P5 `s == 0` mask. Per grid: quotas → coherence → g_out per stream; audibility mask; mixer ramps (P12); custom-table try-lock copy (P11); active-set rebuilds. The renderer switches between the bank and Hann (P14). New test hooks and diag fields (T2, T13) |
| `Source/PluginProcessor.*` | Cache the new parameter pointers. Bind `ScaleSource {degreeMasks.data(), &customLock, &customTable, &customVersion}`. No new params, and no state-codec change |
| `tests/render-harness/engine_gates_22.h` **(new)** | The 2.2 rows (Wave 5) |
| `tests/r12/r12_render.cpp` **(new)** | The R12 out-of-tree driver (T14) |

---

## Tasks

### Wave 0 — the carried gap, on the 2.1 tree (P9)

**T1. Steal-free bit-identity row (S0)**
- Add `TestHooks::nativeAcquire`. When it is set, `Engine::spawn` bypasses `Spawner::acquire` and takes `pool.tryObtain() ?: &pool.obtain(age·invLen)`.
- Row S0:
  - Settings: `grain_limit` 128, s1 only, density 100 × size 1 s (N = 100), jitter 20 %, 4 s render, seeded.
  - Vacuity check: `steals == 0 && kills == 0` on both runs, else the row FAILs as vacuous.
  - Pass condition: engine path vs native hook, max|Δ| == 0.
- Row S0n (negative control): the same pair at N = 300 (density 150 × 2 s) must **differ**.
- Build the harness: 79/79 PASS. Commit on its own (path-scoped) **before Wave 1 starts**.
- Depends on: none.

### Wave 1 — primitives (independent)

**T2. Constants, Grain, ControlGrid, parameter plumbing**
- Constants and Grain fields per the layout table (P13).
- `rampCellsFor(fs, ms)`, rounded up to whole cells. Keep `rampCellsFor(fs)` as the 20 ms base.
- New `ParamPointers` / `GridParams` fields:
  - `sN_scale` is a raw choice index 0…31.
  - `sN_window_skew` −100…100 % is read with divisor 100.
  - `sN_stereo_spread` and `sN_gain_rand` use `readPct`.
  - mute and solo are bools (> 0.5).
- Diag gains `planCalls[4]` and `schedCalls[4]` (the FUNC-03 zero-draw proof). `SpawnRecord` gains gainL, gainR, windowStep, tilt t, pitchCents and quota.
- Processor: cache the pointers. Bind `ScaleSource`.
- Depends on: T1.

**T3. `WindowMorphBank.h`** (R6, ARCHITECTURE §8 segments)
- Segment A, m ∈ [0, ⅓]: Tukey α 1 → 0.25 (α = 1 is Hann).
- Segment B, m ∈ [⅓, ⅔]: lerp Tukey 0.25 → Trapezoid (12.5 % linear ramps).
- Segment C, m ∈ [⅔, 1]: lerp Trapezoid → WindowLut ExpoDecay (2 % raised-cosine attack, e^(−5u)). Build it **from WindowLut's shape function**, not a re-derivation.
- `step = round(m·64)`. Tables are 2048 points, read by linear interpolation at `phase·2047` (the same convention as WindowLut).
- Cumulatives: `C1/C2[k][i]`, 2049 entries, trapezoid-integrated in double over the float table the renderer reads. `C(1) = μ`.
- Warped cumulative (R6): `q = tilt.warp(x)`, `Ch = C(0.5)`; `x ≤ t ? 2t·C(q) : 2t·Ch + 2(1−t)·(C(q) − Ch)`.
- Memory: ≈ 1.6 MB. It is a member of `Engine`, built in the `Engine` constructor (message thread), and never rebuilt in `prepare`.
- Depends on: T2.

**T4. `OutputLaw.h` — R5 law**
- `S1, S2`:
  - N ≤ q: `N·μ₁`, `N·μ₂`.
  - N > q, with x = q/N: `q·C1w(x)/x`, `q·C2w(x)/x` (warped).
  - The P14 path uses `hannCumW2` and the Hann μ₁ = 0.5 closed form. Add `hannCumW(x) = x/2 − sin(2πx)/(4π)`.
- `S1 *= E[g_spread] = (4/(π·s))·sin(π·s/4)`, exactly 1 at s = 0 (branch, no division).
- `P = max(c·S2, (c·S1)²) + (1 − c)·S2`; `g_out = 1/sqrt(max(1, P/μ₂_Hann))`.
- `Coherence::compute(stream params, fs, span, nInWindow, unisonActive)`, per RESEARCH §1.2:
  - `c = (1 − reverse_prob) · c_spray · c_pitch`
  - `c_spray = (1 − clamp(spray·span / (W₀·fs)))²`
  - Three `c_pitch` branches: scale Off; scale on with pitch_rand > 0; empty set or no degree in span.
- Everything is double, at the grid, from latched parameters only.
- Depends on: T3.

**T5. `Quotas.h` — water-fill** (RESEARCH §3)
- Demand `N_s = density_s · clamp(size_s) ms / 1000` for enabled streams, and 0 for disabled ones. Mute and solo do **not** change demand.
- If `ΣN ≤ Q`, then `q_s = N_s`. Otherwise sort the ≤ 4 demands, find λ with `Σ min(N_s, λ) = Q`, and set `q_s = min(N_s, λ)`.
- `qCeil_s = (int) ceil(q_s)`.
- **Single-stream capped case:** λ must come out as exactly `Q` (no accumulated subtraction), so that R12 holds bitwise. Unit-row it.
- Depends on: T2.

**T6. `ScaleQuantiser.h`** (D4, R9)
- Index mapping: `sN_scale` 0 = Off. The table comes from `GranulatorParamIds::scaleChoiceToId`; choice 31 = Custom reads the **engine's** copy.
- Mask: `masks[maskIndex(s + 1, choice)]`, loaded relaxed at the grid. Streams are 0-based in the engine and 1-based in the mask index (R9 trap).
- Rebuild key: (choice, mask, spanCents = pitch_rand·100, customVersion copy).
  - Tile `deg + k·period` for every k that lands inside [−span, +span]. Degree 0 at k = 0 is the unison.
  - Test the mask bit with the source index d. Insertion-sort the active array.
  - Bound the array at `kMaxActive`, and truncate safely if the bound is hit. Worst case: 64 degrees × 5 periods.
  - Record `unisonActive` (mask bit 0 set).
- `window(pitch)`: `[max(−span, −2400 − 100p), min(span, 2400 − 100p)]`, using lower_bound / upper_bound. This yields `nInWindow` for T4 and the pick range. Base pitch is never a rebuild key.
- `pick`: `cents = active[lo + draw.nextInt(Slot::Pitch, hi − lo)]`. If `hi == lo`, the offset is 0 and **no draw is made**. With pitch_rand = 0, there is no draw and the offset is 0.
- Depends on: T2.

### Wave 2 — spawner

**T7. `Spawner::plan()`**
- Pitch:
  - Off: the 2.1 path, unchanged (bitwise).
  - On: `st = pitch + pick/100`, then clamp ±24 (a safety no-op). `rate = exp2(st/12)` in double. Latch `pitchCents`.
- Spread: if spread > 0, `x = spread·(2u − 1)` (Slot::Pan) and `(gL, gR) = panGains(x)`. At spread 0 there is no draw and the gains stay (1, 1) bitwise.
- Gain rand: if gain_rand > 0, `g = (float) exp10(−gain_rand·u·24/20)` (Slot::Gain). Multiply into gL and gR. The output law never sees it (D5).
- Window: `step = (int) lround(window·64)`, `tilt = makeTilt(0.5f + 0.45f·skew)`.
- The draw order is fixed. Every draw comes before `acquire()`. A refused or stolen spawn consumes identical draws.
- Depends on: T2, T6.

**T8. `Spawner::acquire()` — fair progress** (R8)
- The existing 128-slot scan also fills `live[4]` (live, non-killing grains). A first pass computes `overQuota[s] = live[s] > qCeil[s]`, and a second pass takes the argmax over `2·overQuota + (stream == spawning) + age·invLen`. Keep the ties-on-lowest-slot rule.
- progress′ (native-steal fallback) = `(killing ? 3 : 0) + progress`.
- Proof obligation (unit row, T13): with one stream, the victim equals the 2.1 victim for every candidate set.
- Test hook `nativeProgress`: progress = `age·invLen` only. This is the FUNC-03 negative control.
- Depends on: T5, T7.

### Wave 3 — engine

**T9. Grid tick: four streams, quotas, law, mixer, scale state**
- Remove the P5 mask: `scheds[s].setEnabled(gp.s[s].enable)` for all 4. Each stream keeps its R4 epoch and edge reset.
- Order per grid tick:
  1. Latch the parameters.
  2. **Custom-table try-lock copy** (P11), when the version changed.
  3. Per-stream active-set rebuild (T6).
  4. Quotas (T5).
  5. Per stream: `nInWindow`, then c (T4), then the g_out target on the 20 ms ramp.
- Audibility: `audible_s = !mute_s && (!anySolo || solo_s)`. The audibility ramp goes 0 ↔ 1 at 10 ms. Level and pan ramps are also 10 ms (P12).
- `streamGainL/R = g_out · level · audible · panGains(pan)`, pushed to the existing `CellLerp`.
- Disabled streams: no `scheduleStreams` call and no `plan` call. Diag counters prove both.
- Depends on: T4, T5, T6, T8.

**T10. Renderer**
- Per grain: if `windowStep == 0 && tiltT == 0.5f`, use `hannWindow(phase)`. Otherwise use `bank.read(step, tilt, phase)`.
- The window is still applied LAST. Still multiply `killGain · fade · gainL/R` per channel.
- `readGrain()` stays the single read site (R1).
- Depends on: T3, T9.

### Wave 4 — integration build

**T11. Build + quick regression**
- Build the harness and plugin with 0 new warnings.
- The 2.1 rows (G, D, S, P, B, E0–E4, A1, L, N, O, M, R3, S0) stay ALL PASS, run on 2.1-equivalent settings where they depend on defaults.
- **Watch item:** `sN_stereo_spread` defaults to 30 %. Every 2.1 row that relied on unity grain gains must now set spread 0 explicitly. Record each row touched in SUMMARY. Setting spread 0 is restoring a precondition, not weakening a gate.
- Depends on: T10.

### Wave 5 — gates (`engine_gates_22.h`)

All rows print PASS/FAIL and count toward the exit code. They are seeded, and stimuli are position-deterministic. A level probe uses a stationary source, discards a warm-up of 2 × size, and averages seeds as stated.

**T12. FUNC-03 streams + pool**
- **F1 — each stream alone:** s_k enabled alone, k = 1…4. Each is audible (> −40 dBFS wet), and its spawn log holds only stream k.
- **F2 — disabled = nothing:** s2–s4 disabled across a 4 s render with toggles on s1. `planCalls[1..3] == 0`, `schedCalls[1..3] == 0`, and there are 0 onsets. Then enable s3 mid-render: its first onset lands on the next sample (R4 edge).
- **F3 — cap:** 4 streams at 200 g/s × 2 s, `grain_limit` {32, 64, 96, 128}. Per sample after each spawn, `maxAlive ≤ cap`, with kills > 0 (liveness).
- **F4 — starvation:** RESEARCH §3 case "N 400, 50, 10, 1 @ 64". The probe uses P15 solo renders.
  - Pass: each stream's RMS is within ±1.5 dB of its quota-law target.
  - Negative control F4n (`nativeProgress`): the smallest stream reads < −10 dB.
- **F5 — fairness case table:** the six RESEARCH §3 cases, within ±1.0 dB. Include "60, 60, 0.1, 0.1 @ 96" (the R8 ceil row) together with its no-ceil negative-control hook, which must read below −0.5 dB.
- **F6 — solo isolates without perturbing:** the spawn logs (slot, onset, delay) are identical across no-solo / solo-s2 / mute-s1 runs.
- **F7 — single-stream victim equivalence** (T8 proof, unit row).
- Depends on: T11.

**T13. DSP-01 scale quantiser** (D4, R9)
- **Q1 — record-based, all 31 collections:** includes Bohlen-Pierce (tritave) and Custom (a non-default custom table installed through `setCustomScale`).
  - Settings: pitch_rand 7 st, base pitch {0, +5.3, −20} st. A random mask with ≥ 3 bits set, including one without bit 0.
  - Pass: for every record, `1200·log2(rate) − 100·pitch` is within 1 ¢ of an active member inside the window.
- **Q2 — empty mask, or no degree in span:** `rate == exp2(pitch/12)` **exactly**, and no Pitch draw was made. Check the draw by equality: under an empty mask, renders with pitch_rand 0 and 7 are bitwise equal.
- **Q3 — monotone:** the 7-Limit Just active array is sorted, and its mask-bit alignment is right. Mask only 7/4 (source index of 969 ¢) and every pick reads 969 ± 0 ¢, never 933.
- **Q4 — spec index ≠ gs-Id:** the Chromatic (spec 1) mask ≠ the Major (gs-Id 1) mask, and each picks from its own set.
- **Q5 — audio:** a 1 kHz sine through 5-Limit Just with only 5/4 active, pitch_rand 5 st, spray 10 %. The FFT peak (zero-padded, parabolic interpolation) is at 1250 Hz ± 1 ¢.
- **Q6 — Custom swap:** change the custom table mid-render. Within ≤ 2 grids every new pick is from the new table, and every grain spawned before the change carries an old-table pitch.
- **Q7 — base pitch is unsnapped:** pitch +0.37 st with pitch_rand 0 and scale on gives `rate == exp2(0.37/12)` exactly.
- Depends on: T11.

**T14. DSP-03 / output-law level gates** (R5, R6, R7, R11)
- **E0 additions (units):**
  - bank `C(1) = μ` to 1e-6 per step;
  - the warped cumulative vs direct double integration, ≤ 1e-5 over 65 steps × 9 skews;
  - the R5 law at c = 0 equals the 2.1 `streamOutputGain` **bitwise** across a grid of (density, size, q);
  - continuity at N = q;
  - worst capped P/μ₂H = 333 ± 0.5 % at (step 22, skew −1, q = 128, N = 400);
  - water-fill single-stream λ == Q exactly.
- **W1 — morph × skew ±1 dB:** N = 40 (200 g/s × 200 ms), uncapped, spray 10 % (c = 0), band-limited noise, averaged over 4 seeds. 65 steps × skew {−1, −0.5, 0, 0.5, 1}. Add one capped corner row: N = 400, q = 128, step 22, skew −1. Reference: the same render at step 0 / skew 0.
- **W2 — density flatness ±1.5 dB** (R7 domain):
  - size 1000 ms; density {1, 2, 5, 10, 20, 50, 100, 128, 160, 200}.
  - Rows: c = 1 (spray 0), c = 0 (spray 10 %), c = 0.5 (reverse 50 %).
  - Sources: band-limited noise (4 seeds) and sine 440 (≥ 8 seeds, R11); jitter 20 %, pitch 0.
  - Plus an old-vs-old control row: the same configuration, two seed sets, must agree within ±0.5 dB.
- **W2n — hot spot gone (negative control):** at c = 1, density 200, the 2.1 law (harness reference) reads > +15 dB and the 2.2 law reads within ±1.5 dB.
- **W3 — sub-floor:** at the 100 ms default size, density 1, Expo step 64, skew −1, the peak is ≤ window peak × level (no boost).
- **R11 guard:** no sine row at jitter 0 with pitch ≠ 0. Assert this in the row builder.
- **Band documentation:** print one informational row inside the transition band (spray 0.03 %, noise vs sine 110 Hz). Non-gating, recorded in SUMMARY (RESEARCH §1.4).
- Depends on: T11.

**T15. DSP-04 randomisation**
- **D1 reverse:** 0 % → no reverse records; 100 % → all reverse; 50 % over ≥ 2000 spawns stays within ±4σ of the binomial mean.
- **D2 deterministic:** spray 0 → every record's delay equals `margin + pos·span` (for D at or above the reach floor). Spread 0 → `gainL == gainR == 1.0f` bitwise.
- **D3 spread:** at 100 %, `gL² + gR²` = 2 per grain (constant power, ±1e-6). Mean power L = R within 0.2 dB over 4000 grains, at stream pan 0 and at ±1 (no edge pile-up; R10).
- **D4 gain rand:** at 0, the render is bitwise equal to the no-draw path; at 100 %, every latched gain is in [10^(−24/20), 1].
- **D5 draw-before-obtain:** a refused or stolen spawn at cap 32 leaves the CounterRng sequence of later grains unchanged. Compare spawn records k > n between caps 32 and 128 on the draw-derived fields (rate, dir, gains, step).
- Depends on: T11.

**T16. QUAL-01, mixer, PERF**
- **K1 — steal click:** tonal source, ~0.001 threshold, 4 streams overflowing cap 32 (RESEARCH-2.1 §9 recipe). Non-vacuous: steals + kills > 0 inside the analysis window.
- **X1 — mute/solo/level/pan ramps:** per-event step ratio vs the same-motion reference (the E2 method), < 1.5×. An injected hard switch must read > 10× (negative control). Ramp length is 15 cells at 48 k (P12).
- **N5 — block-size null, 4 streams:** blocks 441, 512, 4096 and the variable pattern vs 32, with everything on:
  - spray, size_rand, jitter, pitch_rand with scale + mask, reverse 50 %, window 0.6, skew −0.4, spread 70 %, gain_rand 50 %;
  - cap 32 with fair stealing;
  - mute/solo toggles and an enable toggle at shared boundaries;
  - a custom swap.
  - Pass: max|Δ| == 0. Different-seed control N5c must differ.
- **M2 — allocations:** M1's coverage plus 4 streams, scale / mask / pitch_rand sweeps (active-set rebuilds), a custom-table swap, window and skew sweeps, and mute/solo. 0 allocations, liveness 1. `setCustomScale` runs on the harness thread with the alloc flag off (message-thread work), and the audio-side copy runs with it on.
- **R12 — 2.1-equivalence null** (RESEARCH §7):
  - Build `tests/r12/r12_render.cpp` against `git archive 6508c635 plugins/O-Granulator` (extracted under the scratchpad) and against the working tree. Same driver source, processor API, params by ID, fixed seed.
  - Settings: s1 only, spray 10 %, window 0, skew 0, spread 0, gain_rand 0, scale Off, static params.
  - Cases: steal-free (N 100, cap 128) and capped (N 400, cap 32).
  - Pass: max|Δ| == 0.
  - This runs as a script (`tests/r12/run.sh`), not inside the harness binary. Its result is recorded in SUMMARY and re-run at verify.
- **C2 — CPU (non-blocking):** 4 streams, 128 grains, non-Hann window, scale on, 48 k/128. Record against the 2.1 C1 baseline of 8.28 %.
- Depends on: T11 (R12 also needs T14's E0 bitwise row green first).

### Wave 6 — validate + docs

**T17. Build, install, validate**
- Harness ALL PASS, with 0 new warnings.
- `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep and AU cache clear).
- `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10 on VST3 and AU.
- `tests/r12/run.sh` PASS.
- Depends on: T12–T16.

**T18. Docs + state**
- SUMMARY-2.2.md:
  - the gate table;
  - errata: ARCHITECTURE §10 κ superseded by R5, E0 worst case 333, ARCHITECTURE §5 nearest-degree snap superseded by D4;
  - P11 and P12 deviations from CONTEXT;
  - the rows that T11 set to spread 0;
  - the CPU figure;
  - the documented transition band.
- STATUS.md → `stage_2_phase_2_2_execute_complete`. REQUIREMENTS stays untouched until verify.
- Path-scoped commit: `git commit -- plugins/O-Granulator`, after re-checking the branch and `git status --short`.
- Depends on: T17.

---

## Dependency graph

```
T1 ─ T2 ─┬─ T3 ─ T4 ─┐
         ├─ T5 ──────┼─ T8 ─ T9 ─ T10 ─ T11 ─┬─ T12 ─┐
         └─ T6 ─ T7 ─┘                       ├─ T13 ─┤
                                              ├─ T14 ─┼─ T17 ─ T18
                                              ├─ T15 ─┤
                                              └─ T16 ─┘
```

(T7 also feeds T8; T3 also feeds T10.)

---

## Success criteria (gate for verify)

| # | Criterion | Req | Task |
|---|---|---|---|
| 1 | S0 steal-free bit-identity (cap 128, N 100, 0 steals/kills) vs native pool; S0n differs | PERF-02 | T1 |
| 2 | Each stream audible alone; disabled streams: 0 plan calls, 0 scheduler calls, 0 onsets; enable edge fires next sample | FUNC-03 | T12 F1/F2 |
| 3 | Total grains ≤ cap at {32, 64, 96, 128}; no starvation (±1.5 dB vs quota law), native steal-oldest control < −10 dB; six-case table ±1.0 dB; R8 ceil control | FUNC-03, PERF-02 | T12 F3–F5 |
| 4 | Solo/mute leave pool behaviour unchanged (identical spawn logs) | FUNC-03 | T12 F6 |
| 5 | Quantised picks within 1 ¢ of active members across all 31 collections (BP, Custom); empty mask → base pitch exactly; 7-Limit Just sorted and mask-aligned; spec ≠ gs-Id; audio 1250 Hz ± 1 ¢; custom swap; base pitch unsnapped | DSP-01 | T13 |
| 6 | Morph × skew within ±1 dB (N = 40, plus capped corner); E0 bank and law units incl. c = 0 bitwise reduction and worst case 333 | DSP-03 | T14 |
| 7 | Density 1→200 flat within ±1.5 dB (c = 1 / 0 / 0.5, noise + sine), old-vs-old control; 2.1 hot spot gone (negative control); sub-floor no boost | DSP-03 | T14 |
| 8 | Reverse 0/100 exact, 50 % within binomial bounds; spray 0 and spread 0 deterministic; spread constant power, no edge pile-up; gain rand 0 bitwise and 100 % in [−24, 0] dB; draw-before-obtain | DSP-04 | T15 |
| 9 | Steal click gate with overflow verified non-vacuous | QUAL-01 | T16 K1 |
| 10 | Mute/solo/level/pan ramps: no step vs same-motion reference; hard-switch control fails | FUNC-03 | T16 X1 |
| 11 | Block-size null with 4 streams and all randomisation; different-seed control differs | PERF-03 | T16 N5 |
| 12 | 0 allocations incl. active-set rebuilds and custom swap; liveness 1 | PERF-01 | T16 M2 |
| 13 | R12 2.1-equivalence out-of-tree null (steal-free and capped) | COMPAT-01 | T16 R12 |
| 14 | All 2.1 rows still PASS; pluginval 10 VST3 + AU; auval | COMPAT-01 | T11, T17 |
| 15 | CPU at 4 streams recorded (non-blocking) | PERF-02 | T16 C2 |

---

## Risks / watch items

- **Spread defaults to 30 %.** Any 2.1 row that silently assumed unity grain gains turns red, or worse, stays green for the wrong reason. T11 audits every row and records it.
- **R12 is fragile by design.** Any reordering of float operations in the law, the water-fill or the renderer's Hann path breaks it. If R12 fails, bisect before touching the gate. It is the net that proves the refactor.
- **W1 runtime:** 65 × 5 × 4 seeds is 1300 short renders. Budget it. If the harness wall time exceeds ~90 s, parallelise the renders across threads (each one owns its own processor). Do not thin the grid.
- **Try-lock starvation (P11):** if the message thread holds `customLock` at every grid (it cannot in practice), the copy is deferred indefinitely. Q6 bounds this at ≤ 2 grids in the harness.
- **Transition band (RESEARCH §1.4):** level inside 0 < spray < W₀ or 0 < |pitch| < 0.5 st can be off by up to the coherent excess. That is documented, not gated, and the constants are not refitted.
- **Cross-stream coherence:** two streams with identical coherent settings sum +6 dB. This is accepted and documented (it is the same as duplicating a track).
- **Do not touch grain-engine 1.1.0 or pitch-collections 1.0.0** (D1). Sorting happens on the plugin's active copy only.
