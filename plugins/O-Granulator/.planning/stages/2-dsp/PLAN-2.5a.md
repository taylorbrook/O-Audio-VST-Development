# Stage 2 — DSP · Phase 2.5a (Tempo sync, LFO/env/macro sources, modulation matrix) — Plan

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.5a of 7
**GSD phase:** plan
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `95c5d54b`
**Inputs:**
- `CONTEXT-2.5a.md` (D13–D16 locked, carry-ins, execution defaults)
- `RESEARCH-2.5a.md` (R39–R58). **Where it differs from ARCHITECTURE.md, ROADMAP.md or CONTEXT-2.5a, RESEARCH-2.5a wins**, as amended by the sign-offs below.
- `SUMMARY-2.4.md`, `VERIFICATION-2.4.md` (carries: matrix → reverb atomics; QUAL-01 / PERF-01 / PERF-03 under modulation; W2o margin)
- `research/ARCHITECTURE.md` §4, §15–§17, Telemetry, Parameter Mapping, Algorithm §6; `ROADMAP.md` Phase 2.5a; `parameter-spec.md` (LOCKED)
- The 2.4 engine in `Source/dsp/`, `PluginProcessor.*`, `GranulatorParamIds.h`, `tests/render-harness/`, `tests/n0/`, `tests/n1/`, `tests/r12/`, `tests/golden/mod-dests.tsv`

---

## Sign-offs (Taylor, 2026-11-05, at plan)

| Item | Decision |
|---|---|
| **R44** — bars outside 4/4 | **Host time signature.** `barLen = 4·num/den` quarters from `getTimeSignature()` (4/4 if absent). `barOrigin = floorMod (ppqPositionOfLastBarStart, barLen)`, latched at each anchor. Every grid position is relative to `barOrigin`. Under 4/4 this equals the plain `ppq/div` grid. FUNC-04a gains 3/4 and 7/8 rows; D16 gains a 3/4 project item. |
| **R45** — Tempo size above the 2 s cap | **Clamp.** Synced size is clamped to [5, 2000] ms (1 bar clamps below 120 bpm, 2 bars below 240 bpm). The reach / ring budget is unchanged. The Stage 3 tooltip states the clamp (carry to 3.x). The density multiplier is clamped in power-of-two steps to [0.5, 200] g/s. |
| **R50** — Free LFO phase (amends D15) | **Free LFOs always use the accumulator.** Direct phase is used only in Tempo (playing, or on the virtual ppq while stopped) while the rate is unmodulated. Accum → Direct happens only at a re-lock. |
| **R49** — route-change crossfade (amends ARCH §16) | **Per-destination residual** replaces the per-slot shadow. On a source/dest change the slot's frozen contribution is added to `residual[oldDest]`, which ramps linearly to 0 over 20 ms. 113 floats + 113 ramps; any change rate; value-continuous. |

---

## Entry Check — contract checksums

Re-hash all six at the start of execute (`shasum -a 256`) against the STATUS.md frontmatter (ROADMAP is `1060dae0…` after D13). The executor confirms, before T1:
- `git diff --stat 42d30b01 HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt` is empty (checked at plan: empty), so `Source/` at HEAD is the verified 2.4 tree byte for byte.
- `git diff --quiet 42d30b01 -- modules scripts/param-dump` is clean. **2.5a changes no module** (R41: the Tempo clock is plugin-local; G3 stays optional).

---

## Goal

Make the engine modulatable and tempo-aware:
- **Tempo:** a held-anchor `TempoTracker` (R39, R42, R44), a plugin-local absolute-time Tempo clock per stream with slot tracking (R40, R41), sync divisions for size / density / LFO rate, and the clamped Tempo-mode size/density modulation (R45).
- **Sources:** LFO ×4 (six shapes, R51; phase per amended D15 / R50), the envelope follower on the conditioned input (R52), and macros ×4. Pitch and Onset are present and output exactly 0.
- **Matrix:** 16 slots in normalised space, evaluated **sparsely** (only routed destinations; unrouted = the raw 2.4 path, R46), slot depth ramps of 20 ms, per-destination residuals (R49), modulation applied after each base ramp (R47), spawn-latched values lerped at the spawn sample (R48), the reverb atomics (R54, closes the 2.4 carry), and telemetry (R58).

At neutral settings (every slot without a live route, `sync_mode` Free) the engine stays **bit-identical to the verified 2.4 tree** (N2 vs `42d30b01`). N1, N0 and R12 keep passing. The 225 carried rows do not change.

At the end of 2.5a:
- FUNC-04, QUAL-03 are gated objectively; FUNC-06 is gated for LFO / Env / Macro (Pitch / Onset in 2.5b).
- QUAL-01 (route edits, sweeps), QUAL-02 (matrix stress), PERF-01 (matrix / transport edits) and PERF-03 (modulation + tempo) advance.
- The D16 DAW tempo checklist (2.5a items) is ready to run at verify.
- The plugin installs and passes auval and pluginval 10 on VST3 and AU, and the host-automation cache gate.

---

## Plan-phase decisions

- **P46 — N2, the neutral null, lands first on the unmodified tree** (CONTEXT gate 1). T1 is a separate commit, made before any `Source/` change.
  - `tests/n2/` is a sibling of `tests/n1/` (same `git archive` machinery), `OLD_SHA = 42d30b01`. The refusal check covers `modules/core/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump` (all byte-identical; no module moves in 2.5a).
  - The driver extends the N1 driver (48 k, 512 blocks, 6 s hash noise, R12 seed, exact `setP`, 2.3 FX non-peaking, 2.4 dials non-default but neutral) with **non-default but neutral 2.5a dials:**
    - `sync_mode` Free with `sN_size_sync` / `sN_density_sync` non-default (inert in Free);
    - all four LFOs at non-default shape / rate / rate_sync / phase; env attack / release / gain non-default; all four macros at non-zero values;
    - slots 1–4 with a source and dest None; slots 5–8 with source None and a dest; slots 9–16 None/None; every depth non-zero.
    - None of these is a route: the null must hold.
  - Cases `free` and `capped` as N1. A `MockPlayHead` (constant 120 bpm, playing) is attached on **both** sides, so the Tempo control can run on the new side.
  - Controls (new side only vs the old side's same case):
    - `N2c`: seed + 1, must differ.
    - **Positive `N2p`:** slot 1 LFO1 → s1_level at depth **0**, plus slot 2 Env → mix at depth 0. Must be **IDENTICAL** at every task (R46: offset exactly 0 → raw path).
    - Negative, each IDENTICAL until its stage lands, then must DIFFER; strict (`--strict`) from T9:
      - `N2n-TMP`: `sync_mode` Tempo (flips at T5);
      - `N2n-LFO`: LFO1 → s1_level depth 1 % (flips at T8);
      - `N2n-MAC`: macro1 50 % → mix depth 1 % (flips at T8);
      - `N2n-ENV`: Env → s1_pitch depth 1 % (flips at T9, spawn-latched).
  - N1 (`6aed8017`), N0 (`c10d0f2c`) and R12 (`6508c635`) keep running unchanged.
- **P47 — The mock playhead is rebuilt before any Tempo row counts** (R56). `MockPlayHead` gains a v2 mode: a piecewise **tempo map** (ppq integrated across steps), a **time signature** map with `ppqPositionOfLastBarStart`, an **event list** (start, stop, loop `[a, b)`, locate `(t, ppq)`, negative start ppq) and `isLooping` / loop points. The harness host-block loop **cuts blocks at every event sample** (hosts report position only at block start), so every block-size variant sees events at identical absolute samples. The legacy formula (`ppq = t/fs·bpm/60`, `hasBpm`, `playing`) stays the default, so B1/B2 and every carried row are untouched. Tempo **ramps** are not harness rows (block-size dependent by nature) — they are D16 DAW items.
- **P48 — `TempoTracker.h` (new, header-only, plugin-local).**
  - The processor reads `getPlayHead()->getPosition()` **once per host block** (by value, no allocation) into a POD `TransportSnapshot {bool valid, playing; double bpm, ppq, lastBarPpq; int num, den;}` passed to `engine.process`. The engine calls `tracker.update (snap, absBlockStart, blockLen)` before the first piece.
  - Port from O-GrainScatter only `ppqAt`-from-integer-samples and the WR-09 `bpm > 0` guard (R43). Everything else is the R39 / R42 design:
    - `anchor = {abs, ppq, pps, barOrigin, barLen}`; every ppq is `anchor.ppq + (abs − anchor.abs)·anchor.pps` (no accumulated ppq anywhere).
    - Rules per block (R42 table): invalid / `bpm ≤ 0` → 120 bpm virtual; stopped → ignore host ppq, virtual ppq at host bpm (re-anchor on bpm change only); `isPlaying` rising edge → re-anchor + **re-lock**; playing and `|host − trapezoid| > 1 ms` → re-anchor + re-lock; `> 0.5 sample` or bpm or time-sig changed → re-anchor only; else keep.
    - Trapezoid prediction: `ppq_prev + n·(pps_prev + pps_new)/2`.
    - `relockEpoch` (int64) increments on every re-lock; consumers compare it at ticks.
  - All grid maths in double with `floor`/`ceil`; `frac (x) = x − floor (x)`; never `fmod` or an int cast (negative ppq in pre-roll).
- **P49 — Tempo clock in `StreamScheduler`; Free path untouched** (R40, R41).
  - A `TempoClock` branch, selected per stream from the latched `sync_mode`: onset k at `t_k = anchor.abs + ((barOrigin + slot_k·div_eff) − anchor.ppq)/anchor.pps + J·(u_k − ½)·I`, `I = div_eff/anchor.pps`, `u_0 := ½`, u keyed on `epoch + k` with the existing `kSchedStream` / `Slot::Jitter` (same telescoping law as Free, R41).
  - Emit k when `t_k < pieceEnd`: `sample = max (pieceStart, ceil (t_k))`, `frac = sample − t_k` (clamped to [0, 1) with `frac = 0` for a late re-anchor). Ordering: consecutive onsets ≥ 1 sample apart (Free `kMinInterval`).
  - `slotNext` (int64) advances by 1 per onset. It is derived from ppq **only at re-anchor / enable edge / mode switch / division change**: `slotNext = ceil ((ppq_now − barOrigin)/div_eff − 1e-9)`.
  - Mode switches and the Tempo enable edge fold the onset count into `epoch` exactly like R4. The first Tempo onset is the next slot ≥ now (never "now").
  - Free keeps `GrainScheduler` verbatim; `scheduleStreams` keeps `fs / density` for Free.
- **P50 — Effective Tempo values** (R45). At each tick, per stream in Tempo mode:
  - `divBeats` from a 22-entry table (`1/64` = 1/16 beat … `1/2` = 2 beats; T ×2/3, D ×3/2; bars × `barLen`), defined once in `EngineConstants.h` and checked against `tempoDivisions()` by a unit row.
  - Density: `d0 = pps·fs / divBeats` → g/s; `e = clamp (round_tiesUp (4·off), ceil (log2 (0.5/d0)), floor (log2 (200/d0)))`; `div_eff = divBeats·2^(−e)`. Size: `divBeats·60000/bpm·2^(4·off)`, clamped to [5, 2000] ms.
  - `computeStreamTargets` (demand, quotas, R5 coherence) and the spawn-time size read the **effective** values in Tempo mode.
- **P51 — `ModSources.h` (new, header-only):** `Lfo`, `EnvFollower`, macro ramps.
  - **LFO phase (D15 as amended by R50):** `mode ∈ {Direct, Accum}`. Free → always Accum. Tempo → Direct `φ = frac ((ppq − barOrigin)/divBeats)` while no offset on dest 108 + N; the accumulator `{int64 cycle; double frac}` is re-seeded from Direct every Direct cell. Offset ≠ 0 → Accum. Accum → Direct only when `tracker.relockEpoch` changed (and in `prepare`). Increment per cell: Free `32·rate_Hz/fs` (rate_Hz = the matrix-modulated rate from the **previous** cell, R50.5); Tempo `32·(pps/divBeats)·2^(4·off)` (continuous).
  - `lfoN_phase` is outside the accumulator: `out = shape (frac (base + φ_ramped/360))`, φ ramped over 20 ms on the shortest arc.
  - Shapes (R51), bipolar: sine, triangle `1 − 4·|φ − ½|`, saw `2φ − 1`, square, S&H `2·h(c) − 1`, smooth random `2·lerp_cos (h(c), h(c+1), φ) − 1`; `h(c) = Draw (seed, kLfoStream + N, uint64 (c)).nextFloat (0)`. **`kLfoStream = 0x0A1F'0000ull`** is a new stream base; no `Slot` entry moves.
  - Sources are sampled **once per cell** at the tick (block-size invariant by construction).
  - **Env (R52):** tap = `0.5·(L + R)·inGain(i)` at the `CaptureWriter` input (post input_gain, pre-ring; never the feedback return or dry; runs while frozen). Per sample `x = |s·envGain|`, `e += (x > e ? aA : aR)·(x − e)`, `a = 1 − exp (−1/(τ·fs))` in double → float at ticks. Once per piece: `! isfinite (e)` → `e = 0`. Source at the tick = `clamp ((20·log10 (max (e, 1e-30)) + 60)/60, 0, 1)`.
  - **Macros:** `macroN/100` through an `ExactRamp` (20 ms). Unipolar.
  - Pitch (6) and Onset (7) return exactly 0.0f.
- **P52 — `ModMatrix.h` (new, header-only).**
  - **Range table:** 113 `NormalisableRange<float>` built once (message thread) from the `GranulatorParamIds` contract rows (start, end, skew or `setSkewForCentre`) — the same source as `createParameterLayout`. A unit row asserts that `base01` equals the APVTS parameter's `convertTo0to1` for every destination at 64 values.
  - **Slots:** fixed `std::array<Slot, 16>` `{int src, dest; ExactRamp depth;}`. At each tick the latched `modM_source/dest/depth` are compared with the slot state (no listeners). Depth change only → `depth.setTarget (d, rampCells)`. Source or dest change → if the old route was live (`src ≠ None && dest ≠ None`), `residual[oldDest] += depth.current()·srcPrev(oldSrc)`, then `residualRamp[oldDest].reset (residual); setTarget (0, rampCells)`; the slot restarts at depth 0 and ramps to target (R49). `ExactRamp::setTarget` early-outs on an identical target, so re-arming always goes through `reset` first.
  - **Sparse eval:** a 113-bit active set rebuilt per cell from live slots + non-zero residuals. For each active dest: `off = Σ depth·src + residual` (sum **before** one clamp). A dest is *routed* iff `off ≠ 0.0f`. **Unrouted → the raw 2.4 value, untouched** (R46). Routed → `v01 = clamp (base01 + off, 0, 1)`, `raw = range.convertFrom0to1 (v01)`.
  - Each routed dest keeps `offPrev`, `offCur` for in-cell interpolation (P53, P54).
  - **Choice-backed destinations:** none (R55); no rounding rule.
- **P53 — Ramped destinations: modulation after the base ramp** (R47).
  - Per routed ramped dest per cell: `ramp.current()` → raw → base01 → `+off` → clamp → raw → engine units; the per-sample value is `CellLerp (prevMod, curMod)` in place of `ramp.value (i)`. The table of engine ↔ raw maps is the RESEARCH §4 table, one function pair per destination class in `ModMatrix.h`, each with a round-trip unit (U9e).
  - Wiring points: input_gain (`CaptureWriter`, its own `CellLerp`), output_gain, mix (`mixGains`), sN_level / sN_pan (the per-cell `streamGainL/R` product), feedback, fb_saturation, fb_diffusion, fb_low/high_cut (log2 Hz → `updateChainCutoffs`).
  - The base ramps keep their 10 / 20 ms; modulation is **never** routed through a re-targeted `ExactRamp`.
  - **Reverb (R54):** `writeReverbAtomics()` writes the modulated `revMix/revSize/revDamp` per cell (closes the 2.4 carry). `reverb_mix ≤ 0.001` gating and the 100 ms size smoothing are accepted behaviour (D16 items).
  - **LFO rate (109–112):** the modulated rate feeds the next cell's increment (P51); never a same-cell cycle.
- **P54 — Spawn-latched destinations** (R48).
  - At spawn at piece offset o: `off = offPrev + (offCur − offPrev)·(o/32)` (normalised), `raw = convertFrom0to1 (clamp (base01 + off))`, then engineering units, into a **local copy** of `GridParams::Stream` passed to `Spawner::plan`. Unrouted fields are copied bitwise. buffer_length → `spanSamples` for that spawn.
  - Tick consumers (`computeStreamTargets`: density, size, pitch, pitch_rand, spray → quotas, R5 coherence, g_out; the Free interval `fs / density`) read the modulated `cur` value. `GrainScheduler` latches the interval per onset (ARCH L-class rule).
  - Scale destinations follow the `ScaleQuantiser.h:47` note: the modulated pitch / pitch_rand go through the existing quantiser path unchanged.
- **P55 — Telemetry** (R58). `std::array<std::atomic<float>, 113> destTelemetry` (routed: `v01`; unrouted: `base01`, written only when the raw value changes — dirty-checked) and `std::array<std::atomic<float>, 11> srcTelemetry` (LFO 1–4 ∈ [−1, 1], Env and Macros ∈ [0, 1], Pitch / Onset 0), relaxed, written per cell. Public read-only accessors on the processor for Stage 3. **The engine never writes APVTS.**
- **P56 — QUAL-03 / FUNC-06 measurement is the destination trajectory** (R57). A test hook `traceDest {int dest; int stream;}` records the per-sample engine value of a ramped destination and the per-spawn latched value of a latched destination into a fixed ring (64 k entries, test builds only). Ramped bound: `max |Δv| per sample ≤ 1.05 × depth·2π·f_LFO/fs` (normalised, sine) mapped through the destination slope at the evaluated point, and the residual slope `|c|/(20 ms·fs)` at route removal. The audio step-click gate (same-motion reference, tonal source) is the secondary check. Square / saw / S&H edges (0.67 ms cell ramps) and latched-destination aliasing (R53 / Q5) are excluded by design.
- **P57 — Host-automation gate** is a script on the **installed** VST3 (memory: pedalboard via `uv`): `tests/hostcache/run.py` sets `modM_source/dest/depth`, `lfoN_*`, `sync_mode`, `macroN` through `raw_value`, pumps CFRunLoop, renders, and reads back — the host cache must equal what was set (decoded values, memory: raw_value round-trip on decoded values) and the plugin's state XML must agree. It runs at T13 after install. Nothing in 2.5a writes a parameter, so it should pass trivially; it is still run.
- **P58 — Row files.** New rows go in `tests/render-harness/engine_gates_25a.h` (units: TT, U9, LS, EV; tempo: TS) and `engine_gates_25ab.h` (MF, PH, ZQ, N8, M5, Z5, TL, C5). Carried row files are not edited. `main.cpp` gains `--only25a`. Expected total **≈ 290** (225 carried + ≈ 65 new); record the actual count.
- **P59 — D16 DAW checklist at verify.** T13 writes `stages/2-dsp/DAW-2.5a.md`: per item a session recipe (Logic and/or Reaper, generic editor, monitors at normal level) and what to listen/look for. Items: (1) synced grains vs the click at 1/16 and 1/8 T; (2) re-timing on start / loop / locate; (3) a host tempo **ramp** (60 → 180 bpm); (4) LFO sweeps of position / pitch / cutoff / mix for zipper; (5) a synced LFO across a loop point; (6) a 3/4 project with 1-bar density and a 1-bar LFO (R44); (7) LFO → reverb_mix crossing 0 (gated tail, R54) and fast LFO → reverb_size; (8) Logic count-in / pre-roll (does Logic report `isPlaying`, R42). Execute does not wait on the DAW.

---

## Source layout (deltas against 2.4)

| File | Change |
|---|---|
| `Source/dsp/EngineConstants.h` | `kLfoStream = 0x0A1F'0000ull`; `kModSlots = 16`; `kModSources = 12` (incl. None); `kModRampMs = 20.0`; `kTempoDivBeats[22]` (P50); `kTempoFallbackBpm = 120.0`; `kRetimeSamples = 0.5`; `kRelockSeconds = 0.001`; `kDensityMin = 0.5`, `kDensityMax = 200.0`; `kTraceCapacity = 65536` (test builds) |
| `Source/dsp/TempoTracker.h` **(new)** | P48 (`TransportSnapshot`, anchor, R42 rules, R44 bar origin, `relockEpoch`) |
| `Source/dsp/StreamScheduler.h` | `TempoClock` branch (P49); `setMode (Free/Tempo)` with the epoch fold; header comment "Free + Tempo (2.5a)" |
| `Source/dsp/ModSources.h` **(new)** | P51 |
| `Source/dsp/ModMatrix.h` **(new)** | P52, P53 maps, P55 telemetry arrays |
| `Source/dsp/ControlGrid.h` | `ParamPointers` + `GridParams` gain `syncMode`, `sN_sizeSync`, `sN_densitySync`, `lfo[4] {shape, rateHz, rateSync, phaseDeg}`, `envAttackMs/ReleaseMs/GainDb`, `macro[4]`, `mod[16] {src, dest, depth}` (choice indices read with `lround`, clamped). Defaults = spec defaults |
| `Source/dsp/CaptureWriter.h` | input_gain `CellLerp` substitution for a routed input_gain; the env tap (P51). The live push statement is verbatim |
| `Source/dsp/Engine.h/.cpp` | Tracker update per host block; `scheduleStreams` Tempo branch; effective Tempo values; sources + matrix per tick; ramped substitutions; latched spawn copy; modulated reverb atomics; telemetry. `Diag` gains `tempoReanchors`, `tempoRelocks`, `tempoOnsets`, `routeChanges`, `routedDests` (per-cell max), `envScrubs`. `TestHooks` gain `traceDest`, `forceLfoMode` (Direct/Accum, for PH controls), `noResidual` (drop the residual on route change — the MF removal negative control), `fullRoundTrip` (evaluate all 113 through the matrix — the R46 negative control), `perBlockOrigin` (GrainScatter-style per-block ppq origin — the R39 negative control), `ceilPerTick` (derive the slot from ppq every tick — the R40 negative control). Banner step 2 → "analysis — 2.5b"; matrix step documented |
| `Source/PluginProcessor.h/.cpp` | Cache the new pointers (sync, 8 sync choices, 16 LFO, 3 env, 4 macro, 48 mod); read the playhead once per block into `TransportSnapshot`; telemetry accessors; prepare the range table (message thread) |
| `tests/n2/run.sh`, `tests/n2/n2_render.cpp` **(new)** | P46 |
| `tests/render-harness/main.cpp` | `MockPlayHead` v2 + event block splitting (P47); `--only25a`; two `#include`s + section calls |
| `tests/render-harness/engine_gates_25a.h`, `engine_gates_25ab.h` **(new)** | P58 |
| `tests/hostcache/run.py` **(new)** | P57 |

---

## Tasks

Build commands (from `build/`, Release, `OUARICON_BUILD_TESTS=ON`): `ninja O-Granulator-render-harness`, then `ninja OuariconGranulator_VST3 OuariconGranulator_AU`. The harness binary is `build/plugins/O-Granulator/O-Granulator-render-harness_artefacts/Release/O-Granulator-render-harness`; exit 0 means ALL PASS.

**Run the harness, `tests/n2/run.sh`, `tests/n1/run.sh`, `tests/n0/run.sh` and `tests/r12/run.sh` in the background** (memory: 600 s watchdog). `--only25a` is the iteration aid; every acceptance uses the full run.

**The carried-row diff:** sorted `[PASS]` / `[FAIL]` lines, dropping the new prefixes (`TT`, `U9`, `LS`, `EV`, `TS`, `MF`, `PH`, `ZQ`, `N8`, `M5`, `Z5`, `TL`, `C5`) and the INFO timing rows. Every task compares against **baseline A** (T1). The diff must be empty at every task — 2.5a has **no planned re-baseline**. A non-empty diff is a regression: stop and bisect.

### Wave 0 — the precondition, on the 2.4 tree (P46)

**T1. N2 neutral null vs `42d30b01`; baseline A**
- Files: `tests/n2/run.sh` (new), `tests/n2/n2_render.cpp` (new). **No `Source/` change.**
- `run.sh` copies `tests/n1/run.sh` (P46 refusal list; brace-form `"${OLD_SHA}"` revpaths, memory: zsh `:` modifier; `--strict`; WORK `${TMPDIR:-/tmp}/ogran-n2`).
- `n2_render.cpp` extends `n1_render.cpp` per P46, keeping the N1 below-knee print (max|wet| < `kKneeT` − 1 dB both sides) and the "every parameter landed with identical raw values on both sides" check, now over the 2.5a parameters too.
- Capture **baseline A**: harness stdout → `${TMPDIR:-/tmp}/ogran-25a/harness-A.txt`. Run twice and diff (empty: determinism).
- Acceptance:
  - `tests/n2/run.sh` exit 0: both cases max|Δ| == 0, N2c differs, below knee, **N2p IDENTICAL**.
  - All four N2n print **IDENTICAL** (no 2.5a code yet).
  - Harness 225/225. N1 `--strict`, N0 `--strict`, R12 PASS.
- Commit **alone**: re-check `git branch --show-current` + `git status --short`, then `git commit -- plugins/O-Granulator/tests`.
- Depends on: none.

### Wave 1 — test infrastructure + plumbing (bitwise no-ops)

**T2. `MockPlayHead` v2 + event block splitting** (P47)
- Files: `tests/render-harness/main.cpp`, `engine_gates_25a.h` (new: the mock self-test rows).
- **TT0 — mock self-test:** the tempo map integrates ppq exactly (120 → 90 bpm step at a split: ppq continuous, |Δ| ≤ 1e-12); loop `[a, b)` wraps to `a` at the event sample; locate jumps; negative start ppq; 3/4 and 7/8 `ppqPositionOfLastBarStart` correct across a time-sig change; the block splitter yields identical event samples for blocks 32 / 441 / 512 / 4096 / variable.
- Acceptance: TT0 PASS; carried diff vs A empty (B1/B2 use the legacy mock mode); N2 PASS.
- Depends on: T1.

**T3. Plumbing: constants, ControlGrid, processor pointers + `TransportSnapshot`, Diag / TestHooks / telemetry declarations, banner + stage comments**
- Files: `EngineConstants.h`, `ControlGrid.h`, `TempoTracker.h` (new: `TransportSnapshot` only), `Engine.h` (declarations; `Diag::reset()` extended; banner step 2 "analysis — 2.5b"), `StreamScheduler.h` (comment), `PluginProcessor.h/.cpp` (pointers; playhead read into the snapshot, passed and ignored).
- **U9a:** `kTempoDivBeats` matches the `tempoDivisions (22)` strings (parsed: base, T, D, bars) for all 22 entries; the 20 / 19 / 22 prefixes match `sizeSyncs` / `densitySyncs` / `lfoRateSyncs`.
- Acceptance: 0 new warnings; carried diff vs A empty; N2 PASS (N2p IDENTICAL); the `Grain` static_assert holds.
- Commit T2–T3 (path-scoped).
- Depends on: T2.

### Wave 2 — Tempo (tracer: synced onsets end to end before any modulation)

**T4. `TempoTracker` + TT units**
- Files: `TempoTracker.h`, `Engine.h/.cpp` (`tracker.update` per host block; Diag counters), `engine_gates_25a.h`.
- Hook wired: `perBlockOrigin`.
- Write the units against a stub first and watch them fail.
  - **TT1 — held anchor (R39):** grid-point ppq over 20 k cells at 120 / 133 / 97.3 bpm is bitwise identical across blocks 32 / 441 / 512 / 4096 / variable. **TT1n:** `perBlockOrigin` differs at ≥ 1 % of grid points (RESEARCH: 30–52 %).
  - **TT2 — R42 rules:** constant tempo → 0 re-anchors over 60 s; tempo step → 1 re-anchor, 0 re-locks; loop and locate → 1 re-anchor + 1 re-lock each; transport start → re-lock; stopped → host ppq ignored (a stopped host parked at ppq 0 for 10 s: virtual ppq advances at host bpm, no re-anchor); no playhead / `bpm ≤ 0` / no ppq → 120 bpm virtual.
  - **TT3 — trapezoid:** a synthetic per-block linear ramp (60 → 180 bpm / 2 s, 4096 blocks) predicts within 1e-9 ppq of the integrated map → 0 re-locks (the constant-pps predictor would re-lock: printed as INFO).
  - **TT4 — R44 bars:** `barOrigin` / `barLen` correct in 4/4, 3/4, 7/8 and after a time-sig change at a split; 4/4 fallback when absent.
  - **TT5 — pre-roll:** start at ppq −4: grid ppq and `floor`/`ceil` correct across 0 (no `fmod`; a 1/16 grid has no missing or doubled slot at ppq 0).
- Acceptance: TT1–TT5 PASS (TT1n differs); carried diff vs A empty; N2 PASS (Free untouched; N2n-TMP still IDENTICAL because nothing consumes the tracker yet).
- Depends on: T3.

**T5. Tempo clock + effective density/size; TS (FUNC-04a); FUNC-04b re-run**
- Files: `StreamScheduler.h` (P49), `Engine.cpp` (`scheduleStreams` branch, P50 effective values into `computeStreamTargets` + the spawn size), `engine_gates_25a.h`.
- Hook wired: `ceilPerTick`.
- **TS1 — onsets on the grid (FUNC-04a oracle, R56):** exact onset instant `sample − frac` vs the analytic grid time from the mock's tempo map, jitter 0: **|Δ| ≤ 1e-6 sample**, every density division (19) × {constant 120, constant 133, tempo step 120 → 97.3, loop, locate, pre-roll, 3/4, 7/8}. No missing or extra slot (count == analytic count). **TS1n:** `ceilPerTick` at 120 bpm / 48 k / 1/16 produces ≥ 1 dropped or doubled slot over 20 k cells (R40).
- **TS2 — enable edge (R41):** Tempo enable at an off-grid sample: the first onset is the next slot ≥ now, not now. Free → Tempo switch mid-run: same, and the epoch fold makes draws differ from a fresh start (no replay).
- **TS3 — jitter keeps the mean (Q2):** jitter 50 %, 1/16, 60 s: mean interval within 1e-9 relative of the grid; each onset within `J·I/2` of its slot.
- **TS4 — effective values (R45):** 1 bar size at 60 bpm reads 2000 ms (clamped); density 1/64 T at 300 bpm × multiplier stays ≤ 200 g/s and a power of two times the base; quotas / coherence read the effective density (a Tempo 1/64 stream at 120 bpm gets the quota of 48 g/s, not the Free knob's).
- **TS5 — stopped transport:** stopped parked on a beat for 5 s: onsets continue at the virtual tempo, no burst at i = 0 per block (the GrainScatter defect, R43, absent here).
- **FUNC-04b:** the 2.1 Free-density row (within 2 % over 10 s, jitter 0) is carried; confirm it in the carried diff, and add **TS6** = the same at density 0.5 / 200 under the 2.5a tree with every LFO running unrouted.
- Acceptance: TS1–TS6 PASS (TS1n fires); **N2n-TMP DIFFERS**; N2 PASS; carried diff vs A empty; 0 new warnings.
- Commit T4–T5 (path-scoped).
- Depends on: T4.

### Wave 3 — sources (computed, unrouted: bitwise no-op on audio)

**T6. `ModSources.h`: LFO ×4, env, macros; LS / EV units; source telemetry**
- Files: `ModSources.h` (new), `CaptureWriter.h` (env tap), `Engine.h/.cpp` (sources per tick; `srcTelemetry`), `engine_gates_25a.h`.
- Hook wired: `forceLfoMode`.
- **LS1 — shapes:** each of the six shapes at φ on a 1/1024 grid matches the R51 formula bitwise (reference computed in the row); range [−1, 1]; S&H / smooth random deterministic for a seed and differ for seed + 1; smooth random continuous across cycles (|Δ| per cell ≤ π/2·max |h(c+1) − h(c)|·inc).
- **LS2 — Free accumulator:** rate 0.5 → 0.51 Hz automated at 60 s: max |Δφ| per cell ≤ the increment bound (no 0.6-cycle jump). **LS2n:** `forceLfoMode = Direct` in Free jumps ≥ 0.5 cycle at the automation point (R50).
- **LS3 — Tempo direct lock:** an unmodulated synced LFO (1 bar, 1/16 T, 1/8 D) over **64 bars** at 120 and 133 bpm: phase **bitwise equal** to `frac ((ppq − barOrigin)/divBeats)` at every cell; after a loop and a locate it re-locks to the direct formula at the re-lock cell.
- **LS4 — phase param:** `lfo_phase` 359° → 1° moves +2° over 20 ms (shortest arc).
- **LS5 — pre-roll:** negative cycles in S&H are deterministic (same sequence for both block variants) and differ from positive cycles.
- **EV1 — follower law:** a −20 dBFS sine burst: the source rises to (−20 + 60)/60 within the attack τ (63 % at τ ± 1 sample), releases with τ_r; silence → exactly 0.0f; env_gain +24 dB maps correctly; `env_gain` / attack / release changes take effect at the next tick.
- **EV2 — tap point:** feedback 100 % with the input removed: env falls to 0 (the return is not tapped); input_gain −24 dB lowers env by 24/60; freeze on: env still tracks the input.
- **EV3 — NaN:** a NaN injected at the tap → `envScrubs == 1`, the source returns to 0 at the next tick, the audio is unaffected.
- **MC1 — macros:** 0 → 100 % ramps linearly over 20 ms; unipolar.
- Acceptance: LS / EV / MC PASS (LS2n differs); `srcTelemetry` in range on every row; carried diff vs A empty; **N2 PASS with every N2n other than TMP still IDENTICAL** (sources exist but nothing is routed).
- Commit (path-scoped).
- Depends on: T5.

### Wave 4 — matrix

**T7. `ModMatrix.h`: range table, slots, residuals, sparse eval; U9 units**
- Files: `ModMatrix.h` (new), `Engine.h` (member; prepared on the message thread), `PluginProcessor.cpp` (range table build), `engine_gates_25a.h`.
- Hooks wired: `noResidual`, `fullRoundTrip` (declared; consumed at T8).
- Units, against stubs first:
  - **U9b — destination table:** `kModDestIds` ↔ `tests/golden/mod-dests.tsv` (reuse; no second golden, R55); every entry ≠ None is Float.
  - **U9c — range parity:** for every destination at 64 values, the matrix `base01` == APVTS `convertTo0to1` bitwise, and `convertFrom0to1` matches.
  - **U9d — sum before clamp:** two slots on one dest at +80 % and −80 % → offset 0 → **unrouted** (raw path); +80 % and +80 % → clamped at 1.
  - **U9e — residual (R49):** a source change mid-ramp is value-continuous (|Δoff| per cell ≤ slope bound); 10 dest changes inside 20 ms (host stepping `modM_dest`) stay continuous on every touched dest and every residual reaches exactly 0 after 20 ms; then the dest is unrouted. **U9en:** `noResidual` produces a step ≥ 10× the bound.
  - **U9f — engine ↔ raw maps:** each ramped class (gain, level with −60 → 0, mix, pan, feedback ×0.95, sat 1 + 7·x, diffusion, log2 Hz) round-trips within 1 ulp-of-engine over its range.
  - **U9g — sparse vs dense:** `fullRoundTrip` changes the raw value on ≥ 1 destination class with a skew (RESEARCH: s_level 25 %) — the R46 control; the sparse form is identical to raw for unrouted.
- Acceptance: U9 PASS; carried diff vs A empty; N2 PASS (matrix built, nothing wired).
- Depends on: T6.

**T8. Ramped wiring + reverb atomics + LFO rate destinations**
- Files: `Engine.cpp`, `CaptureWriter.h` (input_gain substitution), `ModMatrix.h` (maps), `engine_gates_25ab.h` (new: first MF rows).
- Implement P53 at every wiring point; `writeReverbAtomics()` writes modulated values; LFO rate dests feed the next cell's increment; the D15 predicate switches Tempo LFOs to Accum on a rate offset.
- **MF1 — ramped classes:** LFO1 sine 2 Hz at ±50 % onto input_gain, output_gain, mix, s1_level, s1_pan, feedback (at 50 %), fb_saturation, fb_diffusion, fb_low_cut, fb_high_cut, reverb_mix / size / damp: the traced engine value follows `convertFrom0to1 (clamp (base01 + d·lfo))` mapped to engine units within 1e-5 relative at every cell boundary, both depth signs; reverb atomics carry the modulated value.
- **MF2 — base ramp preserved:** a knob jump of mix 20 → 80 % under a routed LFO still takes the 20 ms base ramp (the modulated trajectory = base ramp + offset, not a 0.67 ms jump).
- Acceptance: MF1–MF2 PASS; **N2n-LFO and N2n-MAC DIFFER**; **N2p IDENTICAL**; N2 PASS; carried diff vs A empty; N1 `--strict` / N0 `--strict` / R12 PASS.
- Depends on: T7.

**T9. Spawn-latched wiring + tick consumers + destination telemetry**
- Files: `Engine.cpp` (`spawn` local copy, `computeStreamTargets` + Free interval reads), `Spawner.h` (no change expected — confirm `plan` takes the stream by const ref), `ModMatrix.h` (telemetry), `engine_gates_25ab.h`.
- **MF3 — latched classes:** LFO onto s1_position, spray, size, size_rand, density (Free), jitter, pitch, pitch_rand, reverse_prob, window, window_skew, stereo_spread, gain_rand, filter_cutoff, filter_res, drive, crush, crush_prob, ring_freq, ring_mix, buffer_length: the per-spawn latched value equals `convertFrom0to1 (clamp (base01 + lerp (offPrev, offCur, o/32)))` within 1e-6 relative (traced), both signs.
- **MF4 — tick consumers:** density modulated in Free changes the onset gaps from the next onset (L-class); quotas and coherence g_out follow the modulated density / size / spray (compare against `computeStreamTargets` on hand-built params).
- **MF5 — Tempo-mode density/size modulation:** a +0.3 offset on density in Tempo → ×2^round(1.2) = ×2, onsets stay on the grid (TS1 oracle ≤ 1e-6); size continuous ×2^(4·off) with the 2 s clamp.
- **TL1 — telemetry:** for a routed dest, `destTelemetry` == the engine's `v01` at the cell; unrouted == `base01`; every `srcTelemetry` in range; no APVTS write (the processor's parameter values are bitwise unchanged across a 10 s modulated render).
- Acceptance: MF3–MF5, TL1 PASS; **N2n-ENV DIFFERS → N2 `--strict` from here on** (all four N2n differ, N2p identical); carried diff vs A empty.
- Commit T7–T9 (path-scoped).
- Depends on: T8.

### Wave 5 — gates (`engine_gates_25ab.h`; no `Source/` change)

**T10. FUNC-06 matrix coverage, D15 phase, QUAL-03 zipper**
- **MF6 — every 2.5a source × class:** {LFO 1–4, Env, Macro 1–4} × {one ramped dest, one latched dest, one skewed dest (s_size), one reverb dest, one LFO-rate dest} × depth {+100 %, −100 %}: the destination moves in the expected direction (monotone check on a ramped source) and stays inside its range. Pitch / Onset routed → identical to unrouted (inert, 2.5b).
- **MF7 — route removal without a step (FUNC-06):** for each source class, remove the route mid-cycle (source → None, dest → None, depth → 0, and a dest swap): traced |Δv| per sample ≤ the P56 bound through removal and the 20 ms tail; then exactly the raw path. **MF7n:** `noResidual` fails it. Secondary audio check: step-click κ vs a same-motion reference (route held), tonal source, ≤ 2.0.
- **PH1 — D15 rate modulation:** LFO2 → LFO1 rate at ±100 % (Free and Tempo): max |Δφ| per cell ≤ the modulated increment bound; no phase discontinuity at the Direct → Accum edge (Tempo). **PH1n:** `forceLfoMode = Direct` under the same modulation jumps.
- **PH2 — self / cross modulation:** LFO1 → LFO1 rate and LFO1 ↔ LFO2 rate: deterministic, finite, one-cell lag (no same-cell cycle).
- **PH3 — Accum → Direct only at re-lock:** removing the rate route keeps Accum; the next loop point re-locks to Direct (bitwise phase == direct formula from that cell).
- **ZQ1 — QUAL-03 trajectory (R57):** sine and triangle LFO at 0.1 / 2 / 20 Hz, depth ±100 %, onto position, pitch, filter_cutoff (s1 LP), mix: ramped (mix) `max |Δv|` per sample ≤ 1.05 × the analytic bound; latched (position, pitch, cutoff) each grain's value equals the source sampled at its spawn sample (no within-grain step by construction).
- **ZQ2 — QUAL-03 audio:** the same sweeps on a 440 Hz tonal source, step-click κ vs a same-motion reference ≤ 2.0 (memory: step click gates need a same-motion control; tonal source). **ZQ2n:** a hook-free control that steps mix per cell without `CellLerp` (a harness-side render with the offset applied at cell starts only) exceeds 2.0 — proves the gate is not vacuous at this stimulus.
- Depends on: T9.

**T11. PERF / QUAL rows: N8, M5, Z5, C5**
- **N8 — PERF-03 under modulation:** all six LFO shapes routed (ramped + latched + LFO-rate + reverb dests), env routed, macro ramps, 16 slots, route edits at shared boundaries (incl. a dest-stepping burst inside 20 ms); tempo synced on the v2 mock through a tempo step, a loop, a locate and a 3/4 change; feedback 80 % + freeze toggle; seeded. Blocks 32 / 441 / 512 / 4096 / variable: **value compare** max|Δ| == 0 (R35). **N8c:** different seed differs. **N8n:** `perBlockOrigin` breaks the null (R39 — proves the anchor matters on this recipe). Non-vacuity: re-anchors > 0, re-locks > 0, route changes > 0, routed dests > 0.
- **M5 — PERF-01 allocations:** M4 coverage + route edits, source / dest swaps, the dest-stepping burst, sync-mode toggles, rate_sync changes, transport start / stop / loop / locate / tempo / time-sig events. 0 allocations; liveness 1.
- **Z5 — QUAL-02 matrix stress:** all 16 slots at ±100 % onto the hottest destinations (s_level ×4, pitch, density, feedback, fb_saturation, filter_res, drive, ring_mix, reverb_mix, output_gain, mix), LFOs at 20 Hz S&H + square, env from a 0 dBFS noise input, 60 s at 48 k + 44.1 k + 192 k: every sample finite, no subnormal in pool / chain / source state at section ends, peak ≤ float(A) (+6 dBFS), `fbScrubs == 0`, `nanKills == 0`, `envScrubs == 0`.
- **C5 (INFO):** C4 + 16 slots active + 4 LFOs + env. Warm-up 4.5 s, median of 3 × 10 s. Print % of one core, Δ vs a C4 re-run, headroom to 30 %. RESEARCH projects ≈ 19 %. Over 25 %: record and flag for 2.6 (the budget gate is 2.6).
- Depends on: T10.

### Wave 6 — validate + docs

**T12. Static checks**
- Grep only non-comment lines, confirming the baseline at HEAD first:
  - `std::vector|new |malloc|make_unique|std::function` in `TempoTracker.h`, `ModSources.h`, `ModMatrix.h` → 0.
  - `fmod` in `TempoTracker.h`, `ModSources.h`, `StreamScheduler.h` → 0 (negative ppq).
  - `->setValue|setValueNotifyingHost|beginChangeGesture` in `Source/dsp/` → 0 (no APVTS writes).
  - `getPosition\|getPlayHead` in `Source/` → exactly one call site (processBlock).
  - `git diff --stat 42d30b01 -- modules` empty.
- Depends on: T11.

**T13. Build, install, validate; host-cache gate; DAW sheet**
- Harness ALL PASS (record the count, ≈ 290) with 0 new warnings; carried diff vs A empty.
- `tests/n2/run.sh --strict`, `tests/n1/run.sh --strict`, `tests/n0/run.sh --strict`, `tests/r12/run.sh` PASS.
- `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep, AU cache clear). `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10, VST3 + AU (randomised params now hit routes, sync toggles and LFO rates; memory: pluginval-10 catches latent NaN).
- **HA — host-automation gate (P57):** `uv run --with pedalboard tests/hostcache/run.py` on the installed VST3: all `modM_*`, `lfoN_*`, `sync_mode`, `macroN` round-trip on decoded values after the CFRunLoop pump; no nested writes. Exit 0.
- Write `stages/2-dsp/DAW-2.5a.md` (P59, items 1–8).
- Depends on: T12.

**T14. Docs + state**
- Write SUMMARY-2.5a.md: the gate table with measured values; N2 / N1 / N0 / R12; HA; C5; the errata list below; carries to 2.5b / 2.6 / 3.x; execution deviations.
- STATUS.md → `stage_2_phase_2_5a_execute_complete` (next: verify, with the D16 DAW check). REQUIREMENTS stays untouched until verify.
- Commit path-scoped after the re-check: `git commit -- plugins/O-Granulator`.
- Depends on: T13.

---

## Dependency graph

```
T1 ─ T2 ─ T3 ─ T4 ─ T5 ─ T6 ─ T7 ─ T8 ─ T9 ─ T10 ─ T11 ─ T12 ─ T13 ─ T14
(N2) └ infra ┘ └ tempo ┘ (src) └── matrix ──┘ └── gates ──┘ └ validate ┘
```

Strictly sequential: every task touches `Engine.cpp`, the harness or the N2 control set, and each N2n flips at its own task (TMP T5, LFO + MAC T8, ENV T9 → strict). T10–T11 author rows only and could overlap T12, but stay sequential for clean diffs.

---

## Gate criteria (new rows)

≈ 65 harness rows, 7 N2 script rows, the HA script and the T13 validation. INFO rows: TT3 constant-pps comparison, C5.

| Row | Measures | Pass | Negative control | Req | Task |
|---|---|---|---|---|---|
| N2 free / capped | out-of-tree null vs `42d30b01`, neutral non-default 2.5a dials | max\|Δ\| == 0; below knee | N2c | COMPAT-01 | T1 |
| N2p | zero-depth routes | IDENTICAL | — | FUNC-06 (R46) | T1–T9 |
| N2n-TMP / LFO / MAC / ENV | one 2.5a element minimally on | each differs (strict from T9) | — | — | T5, T8, T9 |
| TT0 | mock v2 self-test | exact | — | — | T2 |
| U9a–g | divisions, table, range parity, sum-before-clamp, residual, maps, sparse | see T3 / T7 | U9en, U9g | FUNC-06 | T3, T7 |
| TT1–TT5 | held anchor, R42 rules, trapezoid, bars, pre-roll | bitwise / counts | TT1n | FUNC-04, PERF-03 | T4 |
| TS1 | synced onsets vs analytic grid, 19 divisions × 8 transports | ≤ 1e-6 sample; exact count | TS1n | FUNC-04 | T5 |
| TS2–TS6 | enable edge, jitter mean, effective values, stopped, Free density | see T5 | — | FUNC-04 | T5 |
| LS1–LS5, EV1–EV3, MC1 | LFO shapes / phase, env law / tap / NaN, macros | see T6 | LS2n | FUNC-06 | T6 |
| MF1–MF5 | ramped / latched / tick consumers / Tempo modulation | see T8 / T9 | — | FUNC-06 | T8, T9 |
| MF6 | every 2.5a source × class × sign | direction + range | Pitch/Onset inert | FUNC-06 | T10 |
| MF7 | route removal without a step | ≤ bound; κ ≤ 2.0 | MF7n | FUNC-06, QUAL-01 | T10 |
| PH1–PH3 | D15 / R50 phase under rate modulation, re-lock | ≤ increment bound; bitwise after re-lock | PH1n | FUNC-06 (D15) | T10 |
| ZQ1 / ZQ2 | QUAL-03 trajectory + audio | ≤ 1.05 × bound; κ ≤ 2.0 | ZQ2n | QUAL-03 | T10 |
| TL1 | telemetry, no APVTS writes | equal; params unchanged | — | FUNC-06 | T9 |
| N8 / N8c / N8n | block-size null under modulation + tempo | max\|Δ\| == 0 | N8c, N8n | PERF-03 | T11 |
| M5 | allocations | 0; liveness 1 | liveness | PERF-01 | T11 |
| Z5 | matrix stress, 3 rates | finite, no subnormal, ≤ float(A) | — | QUAL-02 | T11 |
| C5 | CPU, INFO | report vs 30 % | C4 re-run | PERF-02 | T11 |
| HA | host-cache round-trip on installed VST3 | equal | — | FUNC-06 | T13 |

---

## Success criteria (gate for verify)

| # | Criterion | Req | Task |
|---|---|---|---|
| 1 | N2 neutral null vs `42d30b01` bitwise (free + capped), N2p identical, all four N2n differ (strict); N1 / N0 `--strict` + R12 PASS | COMPAT-01 | T1, T9 |
| 2 | Carried 225 rows unchanged at every task; W2o unchanged | COMPAT-01 | all |
| 3 | Held anchor bitwise across block sizes; R42 transport rules; R44 bars; pre-roll | FUNC-04, PERF-03 | T4 |
| 4 | Synced onsets ≤ 1e-6 sample of the analytic grid for every density division × transport scenario; enable edge on grid; jitter keeps the mean; Free density within 2 % | FUNC-04 | T5 |
| 5 | LFO shapes, Free accumulator, Tempo direct lock over 64 bars, env law + tap, macros | FUNC-06 | T6 |
| 6 | Every 2.5a source modulates every destination class, bipolar; removal has no step; Pitch / Onset inert | FUNC-06 | T8–T10 |
| 7 | LFO rate modulation phase-continuous; re-lock at loop / locate | FUNC-06 (D15) | T10 |
| 8 | LFO sweeps of position / pitch / cutoff / mix zipper-free (trajectory + κ) | QUAL-03, QUAL-01 | T10 |
| 9 | Block-size null with modulation + tempo events; seed control differs; anchor control breaks it | PERF-03 | T11 |
| 10 | 0 allocations across matrix / transport edits | PERF-01 | T11 |
| 11 | Matrix stress finite, no subnormal, ≤ +6 dBFS at 3 rates | QUAL-02 | T11 |
| 12 | Telemetry equals engine values; no APVTS writes; host-cache gate PASS | FUNC-06 | T9, T13 |
| 13 | Install; auval; pluginval 10 VST3 + AU; static checks; 0 new warnings | COMPAT-01 | T12, T13 |
| 14 | C5 reported (INFO); DAW-2.5a.md ready (items 1–8) | PERF-02, D16 | T11, T13 |

Requirement status expected at verify (after the DAW check):
- **Complete:** FUNC-04, QUAL-03.
- **FUNC-06** complete for LFO / Env / Macro; Pitch / Onset in 2.5b.
- **QUAL-01 / QUAL-02 / PERF-01 / PERF-03** advanced (onset / pitch in 2.5b; full hardening in 2.6).
- **PERF-02** C5 measured; the budget gate is 2.6.

---

## Errata for SUMMARY-2.5a (record at T14; ARCHITECTURE / ROADMAP not edited — checksums)

1. **R39** held anchor vs ARCH §17 / GrainScatter per-block `ppqOrigin`.
2. **R40** slot tracking vs ARCH "`ceil (ppq/div)·div` at every grid point".
3. **R41** plugin-local Tempo clock; ARCH §17 "re-time if > 0.5 sample off" becomes "recompute `t_k` every piece" (exact, no threshold). G3 stays optional.
4. **R42** trapezoid prediction; stopped transport ignores host ppq (virtual ppq at host bpm).
5. **R44** (signed off) bars from the host time signature.
6. **R45** (signed off) Tempo size clamped at 2 s; density multiplier clamped to [0.5, 200] g/s in power-of-two steps. Stage 3 tooltip note.
7. **R46** sparse matrix evaluation: unrouted destinations bypass `convertFrom0to1 (convertTo0to1 (x))` (not identity on skewed ranges).
8. **R47 / R48** modulation after the base ramp; spawn-latched offsets lerped at the spawn sample.
9. **R49** (signed off) per-destination residual vs ARCH §16 per-slot shadow.
10. **R50** (signed off) Free LFOs always accumulate; D15's Direct mode is Tempo-only.
11. **R51** `kLfoStream` new RNG stream base; cosine smooth random; S&H repeats per loop pass in Tempo-direct.
12. **R54** reverb_mix ≤ 0.001 clears the tank (gated tail); reverb_size smoothed 100 ms in the unit.
13. **R55** no Choice-backed destinations; the Stage 1 golden is reused.
14. **R56** mock playhead v2; tempo ramps are DAW-only.
15. Any execution deviation, with the measured reason, and the final harness count.

## Carried (not implemented in 2.5a)

- **2.5b:** onset detector + burst (D14), decimator + MPM, absolute pitch follow, Pitch / Onset sources live; N3 null vs the 2.5a tree.
- **2.6:** full PERF-02 budget gate; `getTailLengthSeconds()`; full QUAL-01 click sweep incl. route edits under all sources.
- **3.x:** R45 clamp note in the size-sync tooltip; mod rings draw `v01` (symmetric on screen, R53); telemetry accessors feed the UI.
- **Out of scope:** the O-GrainScatter stopped-transport re-fire (R43, from reading) — file as a separate GrainScatter issue.
- W2o rule unchanged: widen seeds, never the threshold.

---

## Risks / watch items

- **FP contraction / reordering breaks N2** (CaptureWriter env tap, mixer substitutions, `computeStreamTargets` reads). Unrouted code paths must execute the **2.4 statements verbatim**; substitutions only on routed destinations. Bisect to the statement; never loosen N2.
- **N2p fails** → the sparse predicate is wrong (an offset that should be 0.0f isn't, e.g. `depth.current()` at −0.0f or a residual stuck at a subnormal). Snap residual ramps to exactly 0 on completion; treat `−0.0f` as 0.
- **Vacuous negative controls** (memory: stimulus below threshold). TT1n, TS1n, LS2n, U9en, MF7n, PH1n, ZQ2n, N8n must each fail their gate **before** the subject row's PASS is trusted. If one doesn't, re-derive the stimulus and record it.
- **Event-split mock diverges from real hosts** (loop wrap mid-block). That is exactly why D16 exists; do not try to emulate mid-block wraps in the harness.
- **Tempo onset `frac` at a late re-anchor** could reorder with a Free stream's onset in `mergeOnsets`. The merge tie-break is (offset, stream) and already deterministic; TS2 / N8 cover it.
- **Spawn-local `GridParams::Stream` copy cost** at 800 spawns/s × 22 fields — < 0.05 % (R58). If C5 shows otherwise, copy only routed fields.
- **Harness wall time** grows (TS1 19 × 8 scenarios, LS3 64 bars, Z5 60 s × 3 rates, N8 × 5 variants). Use the render pool, run in the background, don't thin the grids.
- **Do not touch any module** (grain-engine 1.1.0, pitch-collections 1.0.0, reverb-unit 1.2.0). The N2 refusal enforces it.
- **Shared checkout:** path-scoped commits only. Re-check the branch and staging immediately before each commit. Other sessions' changes under `.claude/` and `plugins/O-GrainScatter/` are in the tree.
