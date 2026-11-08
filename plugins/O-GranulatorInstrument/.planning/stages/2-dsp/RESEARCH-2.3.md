# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.3 — RESEARCH

**Researched:** 2026-11-08
**Scope:** Phase 2.3 only. It covers wiring the forked `GrainFx.h` into the spawner and renderer, the D18 proof (parent null, law rows, statistical arm, no bleed), the D19/D20 CPU verdicts and the D21 lever ladder.
**Confidence:** HIGH on code facts. MEDIUM on the CPU projection and the crush-null risk (§2.4), which are derived, not run. Every claim is tagged **VERIFIED** (read this session: `path:line`, or `82406fce:<parent path>:line` from `git show` blobs, never the parent working tree) or **INFERRED** (reasoning, not run).
**Inputs read:**
- `CONTEXT-2.3.md` (D18–D21, Defaults, criteria, 6 open questions), `VERIFICATION-2.2.md`, `SUMMARY-2.2.md`, `RESEARCH-2.2.md`, `research/ARCHITECTURE.md` (CPU risk + levers)
- Plugin: `Source/dsp/{Engine.cpp,Engine.h,GiRenderer.h,GiSpawner.h,VoiceGrain.h}`, `forked/{GrainFx.h,Grain.h,EngineConstants.h,ControlGrid.h,FORK-MANIFEST.md}`, `Source/GiParamIds.h`, `tests/parity/{ParityRows.h,parent_render.cpp}`, `tests/render-harness/{main.cpp,engine_gates_22.h,parity_gates_22.h}`, `tests/golden/params.tsv`
- Parent blobs at `82406fce` (`plugins/O-Granulator/`): `.planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}-2.3.md`, `SUMMARY-2.6.md`, `Source/dsp/{Engine.cpp,Engine.h,Spawner.h}`, `tests/render-harness/{engine_gates_23.h,engine_gates_23b.h}`, `tests/golden/params.tsv`
- `modules/synthesis/grain-engine/cpp/GrainPool.h`

## User Constraints (CONTEXT-2.3, binding; not reopened)
- **D18:** DSP-03 has four proof arms.
  - (a) A null vs the parent at `82406fce` at ≤ −90 dB after gain alignment. Each FX alone and all four chained; crush_prob 0 % and 100 %; random controls at their deterministic values. Each row family has a negative control that must fail. The fitted gain is reported and must match the FX-off gain within a stated band.
  - (b) The parent's GrainFx law rows ported, plus block-size invariance per stage (32/441/512/4096/var).
  - (c) A statistical arm for filter_rand, ring_rand and crush_prob on latched fields, with CI-derived tolerances and wrong-distribution controls.
  - (d) No bleed: bitwise vs a fresh pool, with a skip-reset control.
- **D19:** CPU worst case ≤ 36 % as PASS/FAIL. The config is 16 × 4 streams × 4 slots saturating 256, all FX on (BP res 100 %, drive 100 %, crush 100 %/prob 100 %, ring mix 100 %), 48 k/128. The C22 method is used: run alone, warm-up + 7 × 10 s, live ≥ 240, duty ≥ 0.9, bypass ratio. C22 FX-off is re-run in the same session. Contended counts as FAIL.
- **D20:** A frozen typical proxy at ≤ 12 %. Research may refine it; it is then frozen in the plan.
- **D21:** Levers apply automatically in order: Hermite (timbre-gated) → SIMD ×4 → FX skip. The grain_limit 128 fallback stops execute and asks the user.
- **Defaults:** `GrainFx.h` stays byte-identical (drift 0/13). Latch math runs at spawn only; stages advance per sample. 0 allocations. Clamps sit at 0.45·fs (checked at 44.1/48/96 k). No new params or keys, and param-dump stays identical. FX-off renders are byte-identical to renders-2.2. HEAD PN residuals are −110.9 … −120.2 dB. The HANDS-ON-2.3 sheet is deferred. The Standalone is rebuilt explicitly.

## Project Constraints
- Trunk on `main`. Commits are path-scoped only: `git commit -- plugins/O-GranulatorInstrument`. Re-check `git branch --show-current && git status --short` immediately before every commit.
- Foreign dirty paths at research time (**VERIFIED**, `git status`): `.claude/agent-memory/{dsp,gui}-agent.md`, `modules/registry.yaml`, and untracked `.claude/compaction-snapshot.md` and `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`. Never sweep these in. Never tag.
- Build with `./scripts/build-and-install.sh O-GranulatorInstrument` (it does not rebuild the Standalone). Run long commands in the background (600 s watchdog). The parity reference is always built from `git archive 82406fce` (`tests/parity/run.sh`), never from the parent working tree.

---

## Summary (what the plan must absorb)

1. **The pin's `GrainFx.h` is the parent's FINAL FX, not its 2.3 FX.** It carries the parent's 2.4 **D10 LP/HP make-down** `svfMake = 1/√P(k)`, which is exactly 1.0f at res ≤ 30.83 % and about −10 dB at res 100 % (`forked/GrainFx.h:60-87`, `svf()` `:177-220`). The parent latches it in `planFx` (`82406fce:…/Spawner.h:350`). **Our latch must assign `svfMake`.** If it doesn't, the LP/HP res-100 null still passes, because one voice gives a constant per-grain scalar that the gain alignment absorbs. Only the fitted-gain band row (§2.5) catches it. **VERIFIED.**
2. **Q3, where the latch goes:** `Engine::spawn` (`Engine.cpp:1121-1314`) makes every draw before `acquire` and then writes the `Grain` fields one by one (`:1240-1263`). The comment at `:1263` says the FX fields come from value-initialisation. `VoiceGrain` wraps the forked `Grain`, which already holds the whole FX POD (`VoiceGrain.h:41`, `forked/Grain.h:69-82`, `crushHoldPhase = 1` default). `GrainPool::activate` sets `grains[i] = Grain {}` (`GrainPool.h:236-238`), so no-bleed holds by construction. **Missing:** the grid has no FX fields (`ControlGrid.h:304-333`), and there is no FX latch, no FX log fields and no FX stage in `GiRenderer::renderGrain` (`GiRenderer.h:130-212`). **VERIFIED.**
3. **Q2: the parent reference path needs no parent hooks.**
   - FX param IDs, ranges, skews and defaults are identical at the pin (`params.tsv` rows 30–39, both sides).
   - The parent `SpawnRecord` already logs `fxActive, svfMode, fcHz, ringHz, crushed, driveD` (`82406fce:…/Engine.h:350-356`).
   - At rand 0 / prob 0 or 100 % the parent draws nothing (`Spawner.h:334-392`), so `freezeNoMinSpray` + the 2.2 geometry stays derandomised.
   - Only our files change: `ParityRows.h` rows and `parent_render.cpp` columns. **VERIFIED.**
4. **The −40 dBFS parity material makes several D18 rows vacuous or dead** (§2.2).
   - Crush is silent below ½ LSB, so 0.01 peak gives all-zero output on both sides above crush ≈ 66 %.
   - BP res 100 % at 200 Hz falls below the −70 dBFS liveness floor.
   - Drive at d·x ≤ 0.16 hardly bends.
   - **Fix:** add an our-side test hook `wetSafetyOff` (bypass `kWetCeil·tanh`, `Engine.cpp:1397-1399`) and a per-row material peak. The parent knee is identity ≤ 0.501, so each row asserts a parent output peak ≤ 0.45, otherwise the row is INVALID. **INFERRED** **[flag]**.
5. **The two clamp rows D18 lists cannot be built in a derandomised 48 k null.** At 48 k, 0.45·fs = 21.6 kHz is above both the cutoff maximum (20 kHz) and the ring maximum (5 kHz). Both clamps only bind via rand or at 44.1 k. Clamp coverage moves to the ported unit rows and to 44.1 k census rows (§3). **VERIFIED** (ranges) **[flag: CONTEXT row not constructible]**.
6. **Crush nulls need bitwise pre-FX reads.** One quantiser flip in 48 000 samples costs about −78 dB, which is past −90. The 2.2 residuals (−110…−120 dB) are consistent with "reads bitwise, residual = our wet tanh", but that has not been proven. **Precondition row PF0:** FX off with `wetSafetyOff` must reach the float-rounding floor (≤ −130 dB) before any crush row counts. **INFERRED** **[decision-if-fires]** (§2.4).
7. **Q1: the parent law rows port mechanically.**
   - The units are U1a–d (SVF), U2a–c (drive), U3a/b/c + U3n (crush), U4a/b/c + U4n (ring), and INFO U1i/U2i/U3i (`82406fce:…/engine_gates_23.h:140-562`).
   - They touch only `ogran::fx::*`, `ogran::k*` constants, `juce::` and two helpers, `eg::hashNoise` / `eg::rmsDb`. Apply the fork rename and supply the two helpers.
   - **Our harness IDs already use V3, H3 and A2**, so the ported rows need a prefix (§3.1). **VERIFIED.**
8. **CPU projection for D19: about 29–32 % against 36 %**, a PASS with a ~4–7 point margin.
   - Parent measured FX cost: (C3w 17.33 − C2′ 9.11)/128 = **0.064 % per grain**, all four FX at 100 % (`82406fce:…/SUMMARY-2.3.md:98-100`).
   - × 243 live grains = +15.6 % on top of C22's 14.45 %, plus about 0.5 % for the gather/scatter kernel (parent C2 rose 2–4 %).
   - D21 is unlikely to fire. **INFERRED** (§6).
9. **The D20 proxy at defaults is vacuous.** Size 100 ms × density 10 g/s gives N ≈ 1 grain per pair, so 12 grains ≈ 1 %. Refined proxy: 6 notes × 2 streams at **density 40 g/s, size 150 ms** (≈ 72 live, asserted 60–90), s1 LP 1.2 kHz res 40 % + drive 30 %. Projected ≈ 5–6 % (§7). **INFERRED** **[refinement, freeze in plan]**.
10. **The duty witness cannot see a slowed core.** The parent C7 `--long` run read 38 % with duty passing, then 16.37 % alone (`82406fce:…/SUMMARY-2.6.md:82`). ARCHITECTURE already requires a **machine-speed calibration witness** (fixed-work loop timed before and after; `ARCHITECTURE.md` CPU risk). C22 does not have one yet; D19 needs it (§6.2). **VERIFIED.**
11. **No-bleed, bitwise vs a fresh pool:** a `poisonFreeSlots` hook writes non-default FX state into every inactive slot before each spawn. Value-init must erase it, so render(poison) ≡ render(clean) bitwise. The control `fxNoReset` restores the poison after `activate` and must differ (§4). **INFERRED**.
12. **The statistical arm reuses the 2.2 PS geometry** (4 streams × 200 g/s × 20 ms × 5 s ≈ 4000 grains per side). Two-sample KS on `log2(f/base)` has a critical D of 0.060 at α = 1e-6. The Bernoulli two-proportion bound at α = 1e-6 is |Δp| ≤ 0.050 at p = 0.3. Each wrong-distribution control is ≥ 5× past its bound (§5). **INFERRED** (math).

---

## 1. Spawn latch and kernel — our side (Open Q3)

### 1.1 Latch — re-derive the parent `planFx` Gi-side
- Parent `planFx` (`82406fce:…/Spawner.h:334-392`, **VERIFIED**) is called from `plan()` after the window latch (`:270`), before `acquire()`:
  - **SVF:** active iff `filterType ≠ Off`. Drawn (`Slot::FxCutoff`) only if `filterRand > 0`; `fc = randOct (cutoffHz, rand, u)`; `svfLatch (fc, res, fs)` → k, a1–a3, **make**, fcHz.
  - **Drive:** `driveLatch (drive)`, active iff `d ≥ 1e-4`. No draw.
  - **Crush:** only if `crush > 0`: `crushed = p ≥ 1 ? true : p ≤ 0 ? false : u(Slot::FxCrushProb) < p`. If crushed, `crushLatch` + bit.
  - **Ring:** only if `ringMix > 0`: `f = ringRand > 0 ? randOct (…, u(Slot::FxRing)) : ringFreqHz`; `ringLatch`; mix.
  - `assignFxLatch` (`:398-414`) writes the **latch fields only, never state**.
- `Slot::FxCutoff / FxCrushProb / FxRing` already exist in our forked `EngineConstants.h:246-263`, so nothing is appended. Our draw `Draw (seed, kGrainStream + s, (serial << 24) | idx)` is keyed by slot, so **no 2.2 draw moves**, and at FX default nothing new is drawn. **VERIFIED.**
- **Where:** a new Gi-side header `Source/dsp/GiFxLatch.h` holding `struct FxPlan` + `planFx (const GiGridParams::Stream&, double fs, Uni&&)` + `assignFxLatch (Grain&, const FxPlan&)`. It is called in `Engine::spawn` after the window latch (`Engine.cpp:1225`) and before `acquire` (`:1235`), and assigned after `g.pitchCents` (`:1262`). `GrainFx.h` is not touched.
- **Grid (ADAPTED leaf `ControlGrid.h`):** `GiGridParams::Stream` gains `filterType` (int, `lround` clamp 0–3), `filterCutoffHz` (raw Hz 20…20000), `filterRand`, `filterRes`, `drive`, `crush`, `crushProb` (default **1.0**), `ringFreqHz` (raw Hz 20…5000), `ringRand` and `ringMix`. `GiParamPointers` gains 4 × 10 binds. Skewed ranges are read as denormalised Hz and never inverted by hand.
  - This is an ADAPTED edit: update `FORK-MANIFEST.md` column 5 (sha256) and column 7 (edit note), then drift check = 0.
- **Readsets:** the FX IDs move from "unread" into the read set. `N2s` (`engine_gates_22.h:1453-1473`) asserts `touched == 148` and `bound == 113`. Both numbers change by 40, so update them in a new readSet23. Do not edit the 2.2 row (P16 precedent: rows are not edited; add an N2-23 variant and retire N2s with a recorded reason).
- **Log:** `GrainLogEntry` (`Engine.h:92-115`) gains `fxActive, svfMode, fcHz, ringHz, crushed, driveD`, the same names as the parent `SpawnRecord`.

### 1.2 Kernel — gather → FX → scatter inside `GiRenderer::renderGrain`
- Today one per-sample loop does read → (xfade blend) → `gk = w·killGain·fade` → `bl[c] += l·gk·gainL` → advance (`GiRenderer.h:160-209`). FX must sit between read and scatter, but the stages run over arrays (`fx::svf (g, l, r, n)`). So:
  1. **gather:** the existing loop body, unchanged, writes `L[n], R[n], GK[n]` (locals `float[kCell]`) and stops at the same `done` conditions;
  2. **FX:** `if (fxa & kFxSvf) { fx::svf; NaN check }`, then `drive`, `crush`, `ring`, with flags hoisted per segment;
  3. **scatter:** `bl[c0 + j] += L[j] * GK[j] * g.gainL;`, the **same single statement** as today.
- **Bitwise FX-off argument:** both `gk = w * g.killGain * fader.next()` and the scatter `l * gk * g.gainL` stay single statements with unchanged operand order. Apple clang contracts FMAs only within a statement, so the values are unchanged. The parent made the same restructure with a single kernel and proved N0 bitwise (`82406fce:…/Engine.cpp:1422-1578`; `SUMMARY-2.3.md:102`). **VERIFIED** (parent) / **INFERRED** (ours).
  - renders-2.2 is the gate.
  - **Fallback if a hash moves:** keep the verbatim 2.2 loop for `fxActive == 0` (parent P18 lever). Never re-baseline.
- **Segment boundaries:** our segments are cut at onsets and cells. The block-size invariance comes from the stages being per sample, which is the R13 rule the forked header states (`GrainFx.h:35-39`). The crush hold phase, SVF integrators and ring rotator live in `Grain` and carry across segments. CONTEXT's crush block-size worry is already handled by that design; the gate is the per-stage chunk row (§3.2) plus the engine block-size null.
- **NaN kill (parent P25):** after `fx::svf`, if `! fx::svfStateFinite (g)` → `return false` **before** the scatter. The segment contributes nothing and the grain retires. Fade slots go through the same lambda. Hooks (test builds only): `injectNan {slot, atSample, field, value}`, `forceRetire` (the reference), and a `nanKills` counter.
- **xfade path:** FX run on the blended `l, r`, after the dual-read blend and before the window, so one FX state per grain. Nothing is keyed to the take.
- **Fade copy:** `GiPool` copies the whole `VoiceGrain` (FX state included) into a fade slot (`VoiceGrain.h:28-33`), so a stolen grain keeps filtering. Gate it with the parent H2 row (steal copy bitwise).

## 2. Parent null with FX (Open Q2, D18a)

### 2.1 What exists — **VERIFIED**
- `tests/parity/run.sh` builds the parent from `git archive 82406fce`. `parent_render.cpp` sets `mix 100`, `output 0`, `fb 0`, `reverb 0`, freeze at `kTf`, and stream enable at `kTe` (`parent_render.cpp:109-126`). It writes `<row>.f32` + `<row>.grains.tsv` (16 columns, `:157`).
- `ParityRows.h` is the single source of truth for both drivers. Rows are `{id, kind, stream, sp (stream-suffix KV), oursOnly, deltaAdj, ourOnsetShift, parentOf, seed}` (`:85-99`), the geometry solver is at `:186-220`, and the material is `hashNoise` at `kMatPeak 0.01` (`:51, :74`).
- `parity_gates_22.h:182-261` applies least-squares gain alignment and checks `≤ −90 dB`, `refDb ≥ −70`, grain counts equal, `p0` exact and raw values same. Each control must exceed −90.

### 2.2 Material level — **[flag]**
- `kMatPeak 0.01` exists **only** because our `kWetCeil·tanh` deviates by x²/12 (RESEARCH-2.2 §2.3, P3). FX are level-dependent:

| FX row | at 0.01 peak | consequence |
|---|---|---|
| crush c | silent when 0.5/2^(15−14c) > 0.01 → c > 0.668 | both sides output 0 → null **vacuous** |
| BP res 100 % @ 200 Hz | BW ≈ k·fc = 20 Hz → −31 dB of −44.8 dBFS RMS ≈ −76 dBFS | fails the −70 dBFS liveness floor → **dead row** |
| drive 100 % | d·x ≤ 0.16 → tanh ≈ 0.9 % compression | weak but not vacuous |

- **Fix:**
  - Add a test hook `wetSafetyOff` (ours only; `Engine.cpp:1397-1399` → identity).
  - Add a per-row `matPeak` field (default 0.01, so 2.2 rows are unchanged).
  - FX rows use **0.25** (−12 dBFS, the D6 reference r, so drive is exercised at its unity point) for drive / crush / ring / chained, and **0.1** for the SVF rows (LP/HP resonance headroom).
  - Every FX row asserts `max|parent out| ≤ 0.45`. The parent knee is identity ≤ 0.501 (RESEARCH-2.2 Q5), so above that the row reports **INVALID** (counted as FAIL), never PASS.
  - Our side with `wetSafetyOff` is linear after the scatter.
- **INFERRED.**

### 2.3 Row set (derandomised, 1 voice, the 2.2 `nullBase()` + `freezeNoMinSpray` geometry, PN01 geometry unless noted)

| family | rows (stream suffix values) | notes |
|---|---|---|
| PF0 | FX off, `wetSafetyOff`, matPeak 0.25 | precondition (§2.4); also the FX-off reference gain g₀ |
| PF-L/B/H | type LP/BP/HP × cutoff {200, 2000, 12000} Hz × res {0, 50, 100} (27 rows) | the cutoff lands on the skewed range via the exact-raw `setP`; "raw same" is asserted |
| PF-D | drive {1, 25, 50, 100} | 1 % is d = 0.0016 ≥ guard (active) |
| PF-C | crush {10, 40, 70, 100} × prob {0, 100} | prob 0 ≡ PF0 (no draw, inactive); 100 % exercises the hold + quantiser |
| PF-R | ring_freq {20, 200, 5000} × mix {25, 100} | the carrier starts at (1, 0) |
| PF-X | LP 2 kHz res 80 + drive 50 + crush 40/100 % + ring 200 Hz mix 50; same with BP; same with HP | all four chained |

- **Clamp rows: not constructible here** (Summary 5). The 0.45·fs ceilings at 48 k are 21 600 Hz, above both maxima. They are covered by `FU1a` (fc = 0.45·fs at 44.1/48/96/192 k), `FU4a` (ring at 0.45·fs), and the census rows `FV1a/b` at 44.1 k (§5.3).
- **Negative controls (ours only, `oursOnly` or a hook):**
  - SVF: cutoff × 2^(1/12);
  - drive: drive + 1 %;
  - crush: crush + 1 %;
  - ring: ring_freq × 2^(1/12);
  - chain: hook `fxOrderSwap` (crush before drive);
  - make-down: hook `legacySvfMake` (`svfMake := 1`) on LP res 100 %. This one **passes the null** (it is a scalar) and must **fail the gain-band row**. That is the control for CONTEXT's "alignment hides a gain-law error" risk.
- Per-row count: about 27 + 4 + 8 + 6 + 6 + PF0 ≈ 52 null rows + 6 controls. At 1 s each per side, this adds roughly 1–2 min of parent render time. Run it in the background.

### 2.4 The crush row risk — **INFERRED** **[decision-if-fires]**
- A mid-tread quantiser amplifies any pre-FX difference. Take a one-ulp-scale read difference δ ≈ 3e-8 at x ≈ 0.25. It flips a decision with probability about δ/LSB per sample, and each flip is a full-LSB error.
  - At crush 40 % (LSB 1/2^9.4 ≈ 1.5e-3), one flip per second at 48 k with σ = 0.144 gives a residual of `√(1/48000)·1.5e-3/0.144 ≈ −87 dB`. That **fails −90**.
- The SVF, drive and ring are smooth or linear and tolerate the same δ (≤ −120 dB).
- So the crush and chained rows need **bitwise-equal pre-FX samples** on both sides.
  - The 2.2 evidence points that way: the residuals moved +12 dB with the +6 dB voice-gain change at T12. That is the x² signature of the wet tanh, so the reads contributed nothing visible.
  - It is still not proven.
- **PF0** (FX off, `wetSafetyOff`, 0.25 peak) is that proof. Bitwise reads leave only the output-scalar rounding, so the residual should sit at about −135 dB or lower.
  - Gate: **PF0 ≤ −130 dB, else no PF-C/PF-X row counts.**
  - If PF0 fails, execute stops and asks. Options:
    - (a) find the read difference;
    - (b) re-specify the crush rows as a flip-count metric. That would amend D18.

### 2.5 Fitted-gain band (CONTEXT risk)
- Per row, report `g_row` and require `|g_row / g₀ − 1| ≤ 1e-4` (0.001 dB), with g₀ from PF0.
- Two latched per-grain scalars would otherwise be invisible to a one-voice null: `svfMake` (−10 dB at res 100 %) and `driveNorm` (applied after the shaper).
- With bitwise pre-FX samples, the gain is FX-independent to float rounding (≈ 1e-6). **INFERRED.**

## 3. Law rows (Open Q1, D18b)

### 3.1 Port list — **VERIFIED** (`82406fce:…/engine_gates_23.h`, PLAN-2.3 table `:480-494`)

| parent | measures | pass |
|---|---|---|
| U1a | BP k·v1 at fc, res × fc {20, 200, 2 k, 10 k, 0.45·fs} × fs {44.1, 48, 96, 192} | \|dB\| ≤ 0.1 |
| U1b | LP/BP/HP vs the prewarped bilinear oracle, **with `svfMake`** (`:105`) | ±0.05 dB where \|H\| > −40 dB |
| U1c | k 0.1 at 0.45·fs, 10 s noise, 48/192 k | finite, \|y\| ≤ 100 |
| U1d | `svfStateFinite` × NaN/±Inf × 4 fields | 12 false, finite true |
| U2a / U2b / U2c | y(±r) == ±r (1 ulp); −12 dBFS sine sample peak ±0.1 dB; guard 1e-4 | as stated |
| U3a / U3b / U3c / U3n | silence → 0; lattice; hold runs + fresh hold; mid-rise control | exact |
| U4a / U4b / U4c / U4n | drift ≤ −135 dB (2 s, 5 f × 4 fs); carrier ≤ 0.01 Hz; mix law; per-32 control | as stated |
| U1i / U2i / U3i | LP/HP peak; RMS/fundamental Δ; R22 silence thresholds | INFO |

- Port rule: copy lines 140–562 with the fork rename (`ogran` → `ogi`), into `tests/render-harness/fx_units_23.h`. Supply `hashNoise` (reuse `parity::hashNoise`) and `rmsDb`.
- **Rename the IDs `U*` → `FU*`.** Our harness already has `V3` (slotPerm), `H3` (retireScan256) and `A2`. Without a prefix, ported `V3`/`H3` would collide in logs and in VERIFICATION greps.
- `ScopedNoDenormals` wraps U3a as in the parent. Ours is in `PluginProcessor.cpp:194`; the unit rows call the stages directly, so they need their own guard.
- The parent's R13 / R14 / R15 / R16 / R18 / R22 values are reference numbers for these rows. They are not re-derived here (parent RESEARCH-2.3 §2–§6).

### 3.2 Block-size invariance per stage (new)
- **Unit level:** each stage over 2 s of noise (with the stage at its extreme: res 100, drive 100, crush 100, ring 5 Hz/0.45·fs), fed in chunks {1, 3, 17, 32} vs 32, must be **bitwise** equal. A control applies the per-32 ring renorm variant (U4n's function) under chunks 17 vs 32 and must differ.
- **Engine level:** all FX on and randomised on 4 streams (LP/BP/HP across streams, filter_rand / ring_rand / crush_prob 50 %). Host blocks 441 / 512 / 4096 / variable vs 32 → 0 differing samples, with a different-seed control. This is the 2.2 block-null row with FX added.

## 4. No bleed (D18d)
- Our guarantee is the same as the parent's: `GrainPool::activate` → `Grain {}`, so FX state is zero (crush phase 1) at every spawn, and a fade copy carries the state. **VERIFIED.**
- **FB1, bitwise vs a fresh pool** (matches CONTEXT literally):
  - The hook `poisonFreeSlots` (test builds only) writes non-default FX **state** (`svfIc* = 0.7`, `crushHeld = 0.4`, `crushHoldPhase = 0.3`, `ringRe/Im = (0, 1)`) into every **inactive** slot at each grid tick.
  - Render a 2 s, 4-stream, all-FX, steal-heavy patch (cap 32) with and without poison → max|Δ| == 0.
  - **Control FB1n `fxNoReset`:** after `activate`, re-apply the poison to the new grain's FX state. It must differ.
  - This covers every spawn, both the R3 kill and the native-steal acquire paths, with no slot-targeting logic.
- **FB2 (parent H1 shape):** a resonant BP 100 % + crush + ring grain A, then grain B in the same pool slot on a silent tape region → B == 0.0f exactly. A must ring (|ic| > 1e-3).
- **FB3 (parent H2):** steal copy keeps latch + state bitwise (≥ 50 checked). **FB4 (parent H3):** FX state == `Grain {}` at every spawn.

## 5. Statistical arm (Open Q4, D18c)

### 5.1 Latched distributions — **VERIFIED** (formulas, `GrainFx.h:48-51`, parent `Spawner.h:338-392`)
- `log2 (fc / base) = rand · 4 · (u − ½)` → uniform on [−2·rand, +2·rand] when unclamped. The same holds for the ring.
- `crushed ~ Bernoulli (p)` for 0 < p < 1.
- Choose bases so that ±2 oct never clamps: cutoff 1 kHz (250 Hz–4 kHz) and ring 200 Hz (50–800 Hz).

### 5.2 Rows (the 2.2 `ps()` geometry: 4 streams × 200 g/s × 20 ms × 5 s ≈ 4000 grains per side)

| row | stream settings | statistic | bound (α = 1e-6) | control (ours only) |
|---|---|---|---|---|
| PS5 | LP 1 kHz, filter_rand 70 % | two-sample KS on log2(fc/base); range within ±1.4 oct (≥ 98 % covered) | D ≤ 0.060 | rand 35 % → D ≈ 0.5 |
| PS6 | ring 200 Hz, mix 50 %, ring_rand 70 % | same | D ≤ 0.060 | rand 35 % |
| PS7 | crush 50 %, crush_prob 30 % | two-proportion z, plus each side vs nominal p | \|Δp\| ≤ 4.89·√(2·0.21/4000) = 0.050 | prob 15 % → Δp 0.15, z ≈ 16 |

- Draws exist only behind their gates. Every PS row must also show `fxActive` bits set on ≥ 99 % of grains. Otherwise the draw is dead and the row is vacuous.
- Fields come from the extended `grains.tsv` (parent `rec.fcHz / ringHz / crushed`) and our `GrainLogEntry`.

### 5.3 Single-side census rows (parent V1a–d) — keep
- `FV1a/b` at **44.1 k**: rand 100 % spans ≥ 3.6 oct; rand 0 is bitwise the base; draws at or above 0.45·fs record exactly `float (0.45·fs)`. This is where the clamp is gated (Summary 5).
- `FV1c`: crush prob 0 → none, 100 % → all.
- `FV1d`: `fxActive` gate table.
- Optional, recommended: parent **V2** (BP centroid ρ ≥ 0.9, shared-latch control |ρ| ≤ 0.5) and **V3** (ring carrier ≥ 95 % within 2 Hz). These catch "latch right, renderer reads something else", which the census cannot see. They need the `fxSharedLatch` hook (parent P28 shape: re-plan with rand 0 / prob 1 and assign only the FX latch).

## 6. CPU (D19)

### 6.1 Projection — **INFERRED** from **VERIFIED** inputs

| term | value | source |
|---|---|---|
| C22 FX-off | 14.45 % @ 243 live | VERIFICATION-2.2 |
| kernel restructure | +0.3–0.6 % | parent C2 8.92 → 9.11–9.24 % |
| FX all-on per grain | 0.064 %/grain → +15.6 % | parent (17.33 − 9.11)/128, LP res 100 + drive/crush/ring 100 |
| **worst (2.3)** | **≈ 30–31 %** (band 29–32) | vs **36 %** |

- BP vs LP costs the same (one extra multiply). Crush 100 % on noise is data-independent (`nearbyint`).
- `std::tanh` cost depends on the argument. With BP output at 0.1-peak noise × d 16 the arguments are mid-range, so it is representative and not a fast path. Keep the D19 material at `loadNoise4 (…, 0.1f)` as in C22.
- At 96 k the cost is about ×2 (INFO only).

### 6.2 Method additions
- **C23w:** the C22 session with `all4` FX settings. **C22 is re-run in the same process** as the FX-off reference: print Δ = C23w − C22 and the per-grain FX cost.
- **Machine-speed witness (ARCHITECTURE CPU risk; parent 2.6 finding):**
  - Time a fixed-work calibration loop (for example 2e8 scalar FMAs) before and after each session.
  - If the after/before ratio is > 1.10, or "before" is > 1.15× the value recorded in the plan, the verdict is `[NOVERDICT — slowed core]`.
  - Per D19, NOVERDICT counts as FAIL for gating, and execute re-runs alone.
- **Mode:** the `--cpu23` flag, run alone, never inside the main harness exit code (2.2 precedent).

## 7. Typical proxy (Open Q6, D20) — **[refinement]**
- **CONTEXT's shape at default size/density is vacuous.** N = density × size = 10 × 0.1 = 1 per pair → about 12 live grains. Projected ≈ 0.06 %/grain × 12 + overhead ≈ 1 %, so ≤ 12 % can't fail.
- **Proposed frozen proxy C23t:**
  - 6-note chord (48, 52, 55, 59, 62, 64) × s1 + s2;
  - **density 40 g/s, size 150 ms** → N ≈ 6 per pair → **≈ 72 live (assert 60–90)**;
  - spray / jitter / spread at defaults; cap 256;
  - s1 LP 1.2 kHz res 40 % + drive 30 %; s2 FX off;
  - 48 k / 128, the C22 method + witness.
- Projected: 72 × 0.059 % + 36 × 0.020 % (SVF + tanh share) ≈ **5–6 %** against 12 %.
- It stays as a regression row; Stage 4 re-checks real presets.

## 8. D21 levers (Open Q5)
- **Not expected to fire** (§6.1).
- **Lever 1 (Hermite, rate ≤ 1, level 0) timbre tolerance:**
  - 4-point Hermite at frac ½ has the weights (−1, 9, 9, −1)/16, so `|H| = 2·(9/16·cos(ω/2) − 1/16·cos(3ω/2))`. That is −0.53 dB at 10 kHz, −1.07 dB at 12 kHz, −2.5 dB at 15 kHz and −8.4 dB at 20 kHz (48 k; computed this session).
  - **Gate:** the long-term spectral difference vs the band-limited read on white noise (frac swept) ≤ **0.6 dB to 10 kHz and ≤ 3 dB to 15 kHz**, with 15–20 kHz as INFO. The +24 / +48 alias rows must stay green (they are rate > 1, so the lever is not taken). Control: Hermite forced at rate > 1 must fail the alias rows.
  - Every 2.2 render golden whose grains are rate ≤ 1 at level 0 **changes**. That is a re-baseline, which CONTEXT does not authorise. So lever 1 **also stops and asks** before the hashes are re-pinned. **[flag]**
- **Lever 2 (SIMD over same-(k,s) grains):** must keep the PF/PN nulls, and is byte-identical only if lane order preserves the per-bus summation order. If it doesn't, state the null tolerance per row.
- **Lever 3 (FX skip):** already effective, because the `fxActive` bits are hoisted per segment. Verify with C23 at FX default vs C22 (Δ ≤ 0.5 %).

## 9. Stress / QUAL-01 / alloc
- **FZ1:**
  - res 100 % × LP/BP/HP, drive 100, crush 100/prob 100, ring 100 % at 5 kHz with rand 100;
  - 16 × 4 at cap 256; pitch ±48;
  - full-scale noise and a sine at fc, at 44.1 / 48 / 96 k.
  - Pass: every sample finite, 0 subnormal in a pool scan, `nanKills == 0`.
  - Peak is INFO: no wet safety in the gate, and LP/HP keep about +10 dB after D10.
- **FZ2:** inject NaN/±Inf into each SVF field, plus a fade slot. Exactly one kill, the slot retires, and the bus ≡ the `forceRetire` reference.
- **FA3 (A2 extension):** 0 allocations with all FX on, type switches mid-render, spawns, steals, a cap change and a NaN kill; liveness control as in A2.
- **FK2 (optional, parent K2 recipe):** a cutoff step click gate ≤ 2.0× static, with an `fxLiveCoeffs` control > 2.0.

## Pitfalls checklist (PLAN must-haves)

| # | pitfall | must-have |
|---|---|---|
| P1 | `svfMake` not latched → LP/HP null still passes | assign `make`; gain-band row + `legacySvfMake` control |
| P2 | −40 dBFS material → crush vacuous, BP dead | `wetSafetyOff` + per-row matPeak + parent peak ≤ 0.45 assert |
| P3 | crush null breaks on 1-ulp read drift | PF0 ≤ −130 dB precondition; stop-and-ask if not |
| P4 | clamp rows not constructible at 48 k | clamp gated in FU1a/FU4a + FV1a/b at 44.1 k |
| P5 | scatter statement split → FX-off hashes move | keep `L·GK·gainL` single statement; renders-2.2 bitwise; P18 fallback |
| P6 | row-ID collision (V3, H3, A2) | prefix FU/FV/FB/FZ/FA/PF |
| P7 | N2s counts 148/113 hard-coded | new readSet23 row; FX IDs leave the unread set |
| P8 | `ControlGrid.h` edit not re-hashed | FORK-MANIFEST col 5/7 + drift 0 |
| P9 | duty witness blind to a slowed core | calibration-loop witness → NOVERDICT = FAIL |
| P10 | typical proxy vacuous at defaults | density 40 / size 150 ms, live 60–90 asserted |
| P11 | stats rows with gate closed (filter Off / mix 0) draw nothing | assert `fxActive` bits ≥ 99 % per PS row |
| P12 | NaN reaches the bus | kill before scatter; FZ2 vs forceRetire |
| P13 | Hermite lever moves goldens | lever 1 also stops and asks |
| P14 | Standalone stale | build `OuariconGranulatorInstrument_Standalone` explicitly |

## Open items for the planner
- Freeze the C23t proxy (§7) and the calibration-loop reference value in the plan.
- Decide whether V2/V3 audio rows (§5.3) are in. Recommended: in, because they are the only detector for "renderer ignores the latch".
- Expected new harness rows: about FU 18 + FV 6 + FB 4 + FZ 2 + FA 1 + block 2 + N2-23 1 ≈ 34 (117 → ~151); parity ≈ 52 PF + 6 controls + 3 PS rows (41 → ~102).
- R19 / R22 (parent): resonant LP/HP headroom vs wet safety, and the crush silence threshold. The D10 make-down halves the resonance problem in dB. Both are a HANDS-ON-2.3 listening note, not a 2.3 gate.
