# Stage 2 — DSP · Phase 2.1 (Ring + pass engine + one stream) — Plan

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.1 of 6 (Ring + pass engine + one stream)
**GSD phase:** plan
**Date:** 2026-11-04
**Branch:** `main` (trunk-based) @ `4f5e29d2`
**Inputs:** `CONTEXT-2.1.md` (D1–D3 + execution defaults), `RESEARCH-2.1.md` (R1–R4, Q1–Q3, §8 pins, §9 gate recipes; **authoritative over ARCHITECTURE.md where they differ**), `research/ARCHITECTURE.md` §1–7, §10, §14, §16, Processing Chain, Algorithm Details §1–2, `ROADMAP.md` Phase 2.1, `parameter-spec.md` (LOCKED)

---

## Entry Check — contract checksums

Re-hashed in this session. All six byte-exact against STATUS.md frontmatter:

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…eb5559d7` | ✅ |
| `REQUIREMENTS.md` | `b9ba9106…a536495d` | ✅ |
| `parameter-spec-draft.md` | `d0b1457e…0cdbbce3` | ✅ |
| `parameter-spec.md` | `f5ce3595…a26b3688` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…7949d9d9` | ✅ |
| `ROADMAP.md` | `79d69576…50c14b20` | ✅ |

No drift.

---

## Goal

Replace the Stage 1 pass-through with the load-bearing engine skeleton: an absolute 32-sample pass grid
with write-before-read, a 41 s `MipRing<2>`, **one** live stream (Free timing, telescoping jitter),
the spawner with reach clamp and double read positions, `GrainPool<Grain,128,16>` with a plugin-side
`grain_limit` cap, a Hann-windowed renderer, the cap-aware incoherent output law, equal-power dry/wet and
output gain on a control grid with exact-arrival ramps.

The engine is shaped for four streams from day one (arrays `[4]`, scheduler table `[4]`), but only
stream 1 is honoured. The feedback return buffer `fbDelay` (z⁻³²) exists and is all zero. No freeze, FX,
window morph, scale quantiser, matrix, sources or tempo.

At the end of 2.1, every ROADMAP 2.1 gate passes in the render harness, the plugin installs and passes
pluginval 10 + auval, and Taylor runs the D3 listening checkpoint at verify.

---

## Plan-phase decisions

- **P1 — Reach floor wins over span** (RESEARCH §2). Span bounds the *distribution* of start positions;
  the reach floor bounds *validity*. `D = max(floorFwd/Rev(r, len), margin + reflect(...))`. No clamp of the
  live span to `writeCount` (startup silence is honest).
- **P2 — Read-fraction quantisation is unconditional** (R1). Every grain read forms its delay through the
  `ri / fq` split. No `#if`, no fast path. A test hook (P6) can disable it for the negative control only.
- **P3 — dB snap** (R2): `snapDb(v) = |v| < 1e-3 ? 0 : v`, then `juce::Decibels::decibelsToGain`. Applies to
  `input_gain`, `output_gain`, `s1_level`. `s1_level ≤ −59.95 dB → 0.0f` exactly (spec: −60 = −∞).
- **P4 — Mix-0 bypass** (R2): when the mix ramp is *settled* at exactly 0 **and** the output-gain ramp is
  settled at exactly 1.0f for the whole piece, `out = dry` by copy; the wet sum is computed (engine stays
  warm, R4/draw determinism unchanged) but never added. Mono→mono copies the input channel directly.
- **P5 — `sync_mode`, `sN_density_sync`, `sN_size_sync` are ignored in 2.1** (Free only). Tempo is Phase 2.5.
  `s2–s4_enable` are masked off inside the engine (no scheduler run, no draws).
- **P6 — Test hooks** behind `OGRAN_TEST_HOOKS` (defined only on the render-harness target):
  `forceLevel0` (alias negative control), `disableFracQuant` (R1 negative control), and read-only
  `Engine::Diag` (`reachViolations`, `kills`, `maxAlive`, `spawns`, `steals`). `Diag` is plain ints updated on
  the audio thread and read by the harness between blocks; it never feeds a gain law.
- **P7 — `getTailLengthSeconds()` stays 0 in 2.1.** The real tail (grains ≤ 2 s + feedback + reverb) is set in
  2.6 when every contributor exists. Recorded so 2.6 doesn't miss it.
- **P8 — Deliberate gate change:** Stage 1 harness B2 "defaults" rows (bitwise pass-through) are **replaced**,
  not weakened. Mix defaults to 50 %, so defaults are wet. B2 becomes: mix 0 % bitwise across 3 layouts × 6
  block patterns **with params set through the host normalised path** (`setValueNotifyingHost`, exercises R2),
  plus a "defaults: wet present" row. B1 and B3 are unchanged. This is a gate change, not a regression.

---

## Source layout

All new code is header-first under `Source/dsp/`, namespace `ogran`. Only `Engine.cpp` is a TU (keeps the
hot render loop in one place for the 2.3 CPU work). Add `Source/dsp/Engine.cpp` to `target_sources`.

| File | Contents |
|---|---|
| `Source/dsp/EngineConstants.h` | `kCell = 32`, `kFbDelay = 32`, `kMaxStreams = 4`, `kPoolLive = 128`, `kPoolFade = 16`, ring derivation (`kMaxSpanS = 30`, `kMaxReachS = 10`, `kGuardS = 1`, `kRingSeconds = 41.0`) + `static_assert (kCell <= kFbDelay)`, `static_assert (kMaxSpanS + kMaxReachS + kGuardS <= kRingSeconds)`; `kMaxSemis = 24`; CounterRng stream bases (`kSchedStream`, `kGrainStream`) and the slot enum (`Jitter, PosSpray, Size, Pitch, Reverse`, then reserved `Pan, Gain, FxCutoff, FxReso, FxDrive, FxCrush, FxCrushProb, FxRing, Window, Skew` so 2.2/2.3 append without renumbering) |
| `Source/dsp/Grain.h` | Grain POD at **final layout** (CONTEXT default): stream, dir, level, set, rate, readAbs (double), age, frac, len, invLen, gainL/R, killGain/killStep, plus FX state fields value-initialised and unused |
| `Source/dsp/ControlGrid.h` | `ExactRamp` (linear per cell, arrival assigns `current = target`), `snapDb`, equal-power `mixGains(m)` with snapped ends, `GridParams` latch struct (all stream-1 + global values read at a grid point) |
| `Source/dsp/CaptureWriter.h` | I/O adapter: stereo engine input (mono L=R), input-gain ramp, per-sample `ring.push (inL·g + fb[n−32].L, inR·g + fb[n−32].R)` (fb all zero in 2.1); freeze hook stub absent (2.4) |
| `Source/dsp/StreamScheduler.h` | Wrapper over `GrainScheduler` per stream: telescoping jitter (§8 pin, `u_0 = ½`, J = 0 draws nothing), persistent `epoch` (R4), enable/disable edges, `[4]` table |
| `Source/dsp/Spawner.h` | Draw-before-obtain, len/pitch/reverse/position formulas, reflect, reach clamp (P1), `latchLevel` + `SincBank::setFor`, R3 cap logic |
| `Source/dsp/OutputLaw.h` | Hann closed form `∫₀ˣ w²` in double, `g_out(N, q)` (RESEARCH §4 single-normalisation form) |
| `Source/dsp/Engine.h/.cpp` | Owns ring, pool, schedulers, `fbDelay[2][kFbDelay]`, stream buses `[4][2][kCell]`; implements the per-pass order; renderer; dry/wet + output gain |

`PluginProcessor` owns one `ogran::Engine` and caches raw parameter pointers in the constructor.

---

## Tasks

### Wave 1 — primitives (independent; any order)

**T1. `EngineConstants.h` + `Grain.h`**
- Constants, derivation comment, both `static_assert`s, slot enum with reserved entries.
- `kRingSamples(fs) = (int) std::ceil (kRingSeconds · fs)` — the value passed to `ring.prepare()` and the
  bound in the reverse reach clamp (RESEARCH §1: write the limit as `≤ kRingSamples`, not `capacity − 64`).
- Grain POD final layout; `static_assert (std::is_trivially_copyable_v<Grain>)` (pool copies to fade slots).
- Depends on: none.

**T2. `ControlGrid.h`**
- `ExactRamp`: `setTarget(t, cells)`; per-sample `value(i)` lerps prev→cur inside the cell; on arrival
  `current = target` bit-exact. **No `SmoothedValue`** (reset copies a NaN target). Non-finite target → ignored
  (hold last value).
- Base ramp time 20 ms, rounded **up** to whole cells from fs (parameter-derived, block-independent).
- `snapDb` (P3); `mixGains(m)`: `m == 0 → (1, 0)`, `m == 1 → (0, 1)`, else `(cos ½πm, sin ½πm)` computed at the grid.
- `GridParams`: denormalised values via cached `std::atomic<float>*` (no `get()` on skewed ranges for unity —
  carry-in), `grain_limit` choice → {32, 64, 96, 128}.
- Depends on: none.

**T3. `OutputLaw.h`**
- `hannCumW2(x) = 0.375·x − sin(2πx)/(4π) + sin(4πx)/(32π)` (double). `M(x) = hannCumW2(x)/x`.
- `N = density · len̄ / fs` with `len̄ = clamp(size, 5, 2000) ms · fs` (size_rand ignored — symmetric).
- `P = N ≤ q ? N·μ₂ : q·M(q/N)`; `g_out = 1/sqrt(max(1, P/μ₂))`, μ₂ = 3/8. Hann ratio is exactly 1.0f.
- Unit rows in the harness: `x = 1 → 0.375` exact to 1e-15; continuity at N = q; worst case 200 g/s × 2 s,
  q = 128 → P/μ₂ ≈ 61 (−3.2 dB vs naive) within 0.5 %.
- **Erratum for SUMMARY-2.1:** ARCHITECTURE §10 double-counts μ₂; 2.2/2.3 use `M_m(x)` from the morph bank's
  cumulative-w² table in this single-normalisation form.
- Depends on: none.

**T4. `StreamScheduler.h`**
- `interval(k) = I·(1 + J·(u_{k+1} − u_k))`, `u_k = Draw(seed, kSchedStream + s, epoch + k).nextFloat(Jitter)`,
  `u_0 := ½`. I read from the grid value at the onset sample. `GrainScheduler::jittered` not used.
- Disabled: not run, no draws. Rising enable edge: `epoch += getOnsetCount(); reset();` → first onset next
  sample (R4). `epoch` zeroed only in `prepare()`.
- Emits onsets `(offset, frac, globalIndex = epoch + index, stream)` for a piece; merge order (offset, stream).
- Depends on: T1.

**T5. `CaptureWriter.h`**
- Engine input from the **bus** channel count (Stage 1 `monoToStereo` / layout flags), per-sample push.
- `ring.clear()` only from `Engine::prepare()` (message thread). Never on the audio thread (27.6 MB memset).
- Depends on: T1, T2.

### Wave 2 — spawner + engine

**T6. `Spawner.h`** (RESEARCH §2, §3, §8; ARCHITECTURE §5)
- Draws first, keyed `Draw(seed, kGrainStream + s, globalIndex)`; each randomisation at 0 draws nothing;
  `reverseProb` gated to (0, 1) before drawing (0 % / 100 % exact).
- `len = size·(1 + sizeRand·(2u−1)·0.5)` → clamp [5, 2000] ms·fs. `st = clamp(pitch + pitchRand·(2u−1), ±24)`,
  `rate = exp2(st/12)` in double. Level `latchLevel(rate, 2)`, set `SincBank::setFor(rate / 2^level)`.
- `D0 = margin(level) + reflect(pos·span + spray·span·(u − ½), [0, span])`, `span = buffer_length·fs`.
- Reach floor (P1): forward `D ≥ margin + max(0, (r−1)·len) + 1`; reverse start `D ≥ margin + 1` and
  `D + (r+1)·len ≤ kRingSamples` (backstop — never binds under D2). Debug `jassert`, and `Diag.reachViolations`
  counts any read with `delay > kRingSamples` or `delay < margin(level)`.
- `readAbs = (W_at_onset − D) + dir·rate·frac` (double); `age = 0`, phase uses `(age + frac)·invLen`.
- **R3 cap:** exactly the RESEARCH §3 pseudocode. `nAlive` counts live slots with `killStep == 0`; victim =
  argmax progress among non-killing live grains; `killStep = 1/(5 ms·fs)`; fallback `obtain(progress')` with
  `progress' = (killing ? 3 : 0) + age·invLen`. At `grain_limit = 128` this reduces to native pool behaviour.
- Depends on: T1, T4.

**T7. `Engine.h/.cpp`** — per-pass order (ARCHITECTURE Processing Chain, normative; steps 2, 6-feedback,
7, reverb and wet safety are no-ops or absent in 2.1)
- `prepare(fs, maxBlock)` (message thread): `ring.prepare(kRingSamples(fs))` (27.6 MB @ 48 k, 110 MB @ 192 k —
  D2), `pool.prepare(fs, 5.0)`, zero `fbDelay`, zero epochs, ramps snapped to current params. Nothing in
  `process` scales with the host block — every per-pass array is `kCell`.
- `process(in, out, n)`: cut the host block at every absolute multiple of 32 (`absCount`, int64).
  Per piece `[a, b)`: (1) grid tick if `a % 32 == 0` → latch `GridParams`, retarget ramps, compute `g_out`;
  (3) write piece into ring; (4) schedule stream 1 → onsets; (5) render grain-major, split at onset offsets,
  spawn at each split; (8) dry/wet + output gain → out.
- Renderer per grain per sample:
  ```cpp
  const double ri = std::floor (g.readAbs);
  const double fq = std::round ((g.readAbs - ri) * 0x1p28) * 0x1p-28;          // R1
  const double delay = (double) (W - (std::int64_t) ri) - fq;
  ring.readStereo (g.level, delay, g.set, l, r);
  w = hann ((g.age + g.frac) * g.invLen) * g.killGain * fade.next();          // window LAST
  bus[s][0][i] += w * l * g.gainL;  bus[s][1][i] += w * r * g.gainR;
  g.readAbs += g.dir * g.rate;  ++g.age;  if (killStep) killGain = max(0, killGain - killStep);
  return (g.age + g.frac) < g.len && g.killGain > 0;
  ```
  W is the ring head at render time (= b after step 3). Hann via `0.5 − 0.5·cos(2πφ)` in float from the
  double phase (2.2 replaces it with the morph bank).
- Stream mix: bus × `g_out` × `s1_level` ramp × constant-power pan ramp → wet. `s2–s4` buses stay zero.
- Output: P4 bypass or `out = dry·dG + wet·wG` with both ramps lerped per sample, then × output-gain ramp.
  Mono out = `(L+R)/2` of the final mix.
- `Diag` updates (P6). Test hooks behind `OGRAN_TEST_HOOKS`.
- Depends on: T2, T3, T5, T6.

### Wave 3 — processor integration

**T8. Wire `PluginProcessor`**
- Cache the 17 parameter pointers listed in CONTEXT "Stream-1 parameters honoured" (+ `s2–s4_enable` for the
  mask, read but ignored). `prepareToPlay` → `engine.prepare` and pass the layout flags. `processBlock` keeps
  `ScopedNoDenormals`, hands the seed (`getSeed()` latched at prepare and on change) and calls `engine.process`.
- `CMakeLists.txt`: add `Source/dsp/Engine.cpp`; harness target gets `OGRAN_TEST_HOOKS=1`.
- No APVTS writes from the audio thread or any listener. No new parameters, no state-codec change.
- Depends on: T7.

### Wave 4 — gates (`tests/render-harness/main.cpp`, new section **E.** engine rows; one helper file
`tests/render-harness/engine_gates.h` if `main.cpp` passes ~900 lines)

All rows print PASS/FAIL and count toward the exit code. Seeded via `setSeed`. Stimuli are
position-deterministic (hash of the absolute sample index), never a sequential RNG.

**T9. Rescope B1–B3** (P8). Replace the B2 defaults rows; add "defaults: wet present" (RMS(out − dry) >
−40 dBFS after 1 s). B2 mix 0 % sets every param through `setValueNotifyingHost` (host normalised path).
- Depends on: T8.

**T10. FUNC-01 + FUNC-07 rows**
- E1 audible at position 0: output energy (out − dry at mix 100) within `interval + len` samples of start.
- E2 `buffer_length` change (RESEARCH §2 three-part): (a) every post-change spawn satisfies
  `D ≤ max(margin + span_new, floor(r, len))` (via `Diag`-logged spawn D's, test hook), (b)
  `reachViolations == 0`, (c) step-click verdict vs a same-motion reference with no `buffer_length` change
  (tonal source, spray 0, jitter 0).
- E3 mix 0 % bitwise (covered by T9) + mix 100 %: |corr(out, dry)| < 0.05 at position ≥ 1 s.
- E4 `output_gain`/`input_gain` default after a normalised round-trip → gain exactly 1.0f (R2 regression row).
- Depends on: T8.

**T11. DSP-02 alias** — port `plugins/O-GrainScatter/tests/alias-gate` row 1e: processor end-to-end, 44.1 k,
+24 st via the param, swept sine, out-of-band spurious ≥ 60 dB below the in-band tone. **Negative control:**
`forceLevel0` must FAIL the same threshold.
- Depends on: T8.

**T12. DSP-01 long-buffer** — `plugins/O-TextureForge/tests/scheduler-harness/longcorpus.cpp` pattern:
periodic corpus `i % P`, two grains P-multiples apart null; plus a double reference `start + n·rate` read at
25 s within 1e-6 of the engine's `readAbs` (Diag hook) for rates {0.5, 1.5, 4}.
- Depends on: T8.

**T13. PERF-03 block-size null + R4 + oversize**
- Blocks 32 / 441 / 512 / 4096 / variable (seeded pattern incl. 1 and 8192); every s1 randomisation ON
  (spray, size_rand, jitter, pitch_rand, reverse_prob 50 %), `grain_limit` 32 (R3 kill path inside the null),
  a param change at a shared boundary, and an `s1_enable` off→on toggle. `max|Δ| == 0` vs block 32.
- Different-seed control must differ.
- **R1 negative control:** `disableFracQuant` over a 20 s, 128-grain render at 441 vs 32 must show ≥ 1 diff
  (non-vacuous proof the gate catches the bug). If it shows 0, the row FAILs as "vacuous gate".
- **R4 row:** grain start D set of the second enable burst ≠ the first; null across the toggle bit-identical.
- **Oversize:** `prepareToPlay(48k, 512)` then `processBlock(8192)` and 4096 at position 0: finite output,
  `reachViolations == 0`, guard canaries around engine arrays intact (no ASan on macOS 26 — poison-guard pattern).
- Depends on: T8.

**T14. PERF-01 alloc + R3 cap**
- `--alloc-check` mode (O-Bells `tests/render-harness/main.cpp` pattern): `malloc_logger`, `volatile` flag and
  counter, a liveness malloc that must count exactly 1, scoped to the render thread. Covers spawn, native steal
  (`grain_limit` 128, N ≫ 128), R3 kill path (`grain_limit` 32), enable toggles, `buffer_length` sweeps.
  0 allocations.
- R3 row: `grain_limit` 32, density 200, size 1 s → per-sample `maxAlive ≤ 32` **and** `kills > 0` (liveness).
- Output-law unit rows from T3.
- Depends on: T8.

**T15. CPU baseline (non-blocking, recorded)** — 128 grains, Hann, no FX, 48 k/128, ratio vs engine-bypass;
print % of one core. Recorded in SUMMARY-2.1 for 2.3 (datum: 128 reads ≈ 6.5 % in isolation).
- Depends on: T8.

### Wave 5 — build, install, validate

**T16.** Build `O-Granulator-render-harness` (tests ON) → ALL PASS, 0 new warnings. Then
`./scripts/build-and-install.sh O-Granulator` (dual-variant sweep, AU cache clear), `auval` PASS, pluginval
strictness 10 VST3 + AU. Standalone launches with the Generic editor and produces grains on live input.
- Depends on: T9–T15.

**T17. Docs + state** — SUMMARY-2.1.md (gate table, CPU baseline, §10 erratum, P7 tail note, P8 gate change),
STATUS.md (`stage_2_phase_2_1_execute_complete`), REQUIREMENTS untouched until verify. Path-scoped commit:
`git commit -- plugins/O-Granulator` after re-checking branch and staging.
- Depends on: T16.

---

## Dependency graph

```
T1 ─┬─ T4 ─┬─ T6 ─┐
    ├─ T5 ─┤      ├─ T7 ─ T8 ─┬─ T9  ─┐
T2 ─┴──────┘      │           ├─ T10 ─┤
T3 ───────────────┘           ├─ T11 ─┤
                              ├─ T12 ─┼─ T16 ─ T17
                              ├─ T13 ─┤
                              ├─ T14 ─┤
                              └─ T15 ─┘
```

---

## Success criteria (gate for verify)

| # | Criterion | Req | Task |
|---|---|---|---|
| 1 | Input audible within one onset at position 0 | FUNC-01 | T10 E1 |
| 2 | `buffer_length` change: span bound, 0 reach violations, no step vs same-motion reference | FUNC-01 | T10 E2 |
| 3 | Mix 0 % bitwise dry (3 layouts × 6 blocks, host-normalised params); mix 100 % no dry | FUNC-07 | T9, T10 E3/E4 |
| 4 | +24 st swept sine aliases ≥ 60 dB down; `forceLevel0` control fails | DSP-02 | T11 |
| 5 | Periodic-corpus null + double reference at 25 s | DSP-01 | T12 |
| 6 | Block-size null 32/441/512/4096/variable, all randomisation ON, cap 32, toggle; different-seed control differs; R1 negative control non-vacuous | PERF-03 | T13 |
| 7 | Oversized block (8192 after prepare 512) and 4096 at position 0: no overrun | PERF-03 | T13 |
| 8 | 0 allocations across spawn / steal / kill / toggles; liveness malloc = 1 | PERF-01 | T14 |
| 9 | `grain_limit` 32 enforced (`maxAlive ≤ 32`, `kills > 0`) | PERF-02 | T14 |
| 10 | Stage 1 rows (G, D, S, P, rescoped B) still ALL PASS | COMPAT-01 | T9, T16 |
| 11 | pluginval 10 VST3 + AU, auval PASS | COMPAT-01 | T16 |
| 12 | CPU baseline recorded (non-blocking) | PERF-02 | T15 |
| 13 | **D3 human listening checkpoint** (at verify): position scans the buffer, ±24 st clean, reverse prob works, `buffer_length` moves without clicks, mix 0/50/100 as expected. **Judge level only at defaults** (spray 10 %, jitter 20 %) — at spray 0 + jitter 0 + pitch 0 the incoherent law is ≈ +7 dB hot by design until the κ blend lands in 2.2 | — | verify |

---

## Risks / watch items

- **R1 regressions are intermittent.** Any new read site (2.2+) must go through the same `ri / fq` helper;
  put it in one inline function `readGrain()` so there is exactly one place.
- **Kill path vs native steal at cap 128** must be bit-identical to the pool alone — T13 at `grain_limit` 128
  with N < 128 is the steal-free bit-identity row.
- **Memory:** 110 MB per instance at 192 k (D2). 2.6 turns the 96 k / 192 k smoke into an allocation-size check.
- **Do not touch grain-engine 1.1.0** (D1). If a module bug surfaces, stop and report — don't patch in place.
