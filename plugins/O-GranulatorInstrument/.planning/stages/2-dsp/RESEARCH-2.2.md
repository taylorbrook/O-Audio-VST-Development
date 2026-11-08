# O-GranulatorInstrument — Stage 2 (DSP) — Phase 2.2 — RESEARCH

**Researched:** 2026-11-08
**Scope:** Phase 2.2 only: 4 streams × N voices, 80 schedulers + merge, spawner widening (spray, reverse, spread, size/gain/pitch rand, window + skew), ScaleQuantiser + scale state, water-fill over 80 demands, coherence law, mixer ramps, DSP-01 parity against O-Granulator at `82406fce`.
**Confidence:** HIGH on code facts, MEDIUM on the coherence and thinning recommendations (§11, §10.2; derived, not run). Every claim is tagged **VERIFIED** (read this session; `path:line`, or `82406fce:<parent path>:line` from `git show` blobs, never the parent working tree) or **INFERRED** (reasoning, not run).
**Inputs read:**
- `CONTEXT-2.2.md` (D14–D17, Defaults, criteria, 7 open questions)
- `RESEARCH-2.1.md`, `SUMMARY-2.1.md`, `VERIFICATION-2.1.md`, `PLAN-2.1.md` (P10/P11), `ROADMAP.md` § 2.2
- `research/ARCHITECTURE.md` §7–§16, Processing Chain, Algorithm §1/§5/§6, A4, Gate Catalogue; `parameter-spec.md` stream table + state table
- Plugin: `Source/dsp/{Engine.h,Engine.cpp,GiSpawner.h,GiRenderer.h,VoiceQuotas.h,GiConstants.h,VoiceGrain.h,ScanPlayhead.h,AmpEnv.h,MipTape.h}`, `forked/{WindowMorphBank,OutputLaw,ScaleQuantiser,EngineConstants}.h`, `FORK-MANIFEST.md`, `Source/PluginProcessor.cpp` (state codec), `CMakeLists.txt`, `tests/render-harness/engine_gates_21.h`
- Parent blobs at `82406fce` (`plugins/O-Granulator/`): `Source/dsp/*` (all 21), `Source/{PluginProcessor.cpp,.h,GranulatorParamIds.h,ScaleStateJson.h}`, `CMakeLists.txt`, `tests/render-harness/{main.cpp,engine_gates.h}`, `tests/n1/{run.sh,n1_render.cpp}`
- `modules/synthesis/grain-engine/cpp/{BandlimitedReader.h,GrainScheduler.h}`, `scripts/param-dump/ParamDump.cmake`
- Memory: getlargeintvalue, preset reset-to-defaults, stale `<CustomState>`, vacuous stimulus, valuetree type loss, tukey/hann LUT, forward-grain coherence, freeze scan-0 cluster, seeded out-of-tree pair, gate-design + render-harness indexes

<user_constraints>
## User Constraints (CONTEXT-2.2, binding; not reopened)
- **D14:** HANDS-ON-2.1 gates 2.2 **execute** only. Execute precondition reads VERIFICATION-2.1 Human Verification; any ❌ is fixed in 2.1 first; a 2.1 fix landing after the 2.2 plan makes the planner re-check 2.2 tasks touching that file.
- **D15:** DSP-01 = (a) derandomised null ≤ −90 dB after gain alignment vs the parent engine at `82406fce` on the same material as a frozen buffer (1 voice, root, kt 100 %, scan 0, fb 0, FX off, one stream; spray/jitter/size_rand/gain_rand/spread/pitch_rand 0; reverse 0 % **and** 100 %; sweep size, density, window, skew incl. Tukey/trapezoid/percussive and ±100 %, pitch, position; one-sample-shift or window+1 control must fail) + (b) statistical arm on **latched per-grain fields from a test hook** (spray, jitter, size rand, gain rand, spread, pitch rand, reverse 0<p<1; moments + range, onset-interval histogram for jitter; CI-derived tolerance; wrong-distribution control per row). Draw keys stay `(serial << 24) | onsetIndex`.
- **D16:** `kVoiceGainDb` from a measured target: same stationary-noise file, frozen, default stream params, same enabled streams; ours = 1 held voice at root, vel 127, `vel_to_amp` default (or neutral); long-term RMS within ±0.5 dB. HANDS-ON-2.2 single note + 6-note chord confirm/trim; a trim is recorded in `GiConstants.h` with both numbers.
- **D17:** Port the parent scale state verbatim: `customScale.name/.text/.period/.version` (one shared, `text` re-parsed on load), `degreeMasks[4][32]` uint64 as 16-hex strings (never `getLargeIntValue`; absent = all on); lock-free publish, no audio-thread parse, 0 allocs incl. a custom swap while sounding; keys beside `seed` / `stateVersion` / `modDestTableVersion` with parent names; restore treats absent as default; replace, never append; harness-only verification; no dev-editor UI.
- **Defaults (binding):** stream N reads slot N; s1 On, s2–s4 Off; defaults jitter 20 / spray 10 / spread 30 are honoured → one re-baseline task with old→new hashes, deterministic 2.1 rows pin random controls to 0; schedulers only for (sounding voice × enabled stream), first onset at the note-on sample, draw key `Draw (seed, kSchedStream + s, (serial << 24) | onsetIndex)`, telescoping jitter, Free only; merge (offset, serial, stream); spawner per §10 (Slot enum on `kGrainStream + s`, draw before obtain; `Q(pitchRand·(2u−1))` from unison, key term never quantised, ±48 after; size `·(1 + sizeRand·(2u−1)·0.5)` [5, 2000] ms; reverse `u < p`); forked WindowMorphBank verbatim, window 0 bit-identical to 2.1 Hann; water-fill ≤ 80 equal-weight demands, steal `4·stolen + 2·overQuota(k,s) + (k,s == spawning) + age·invLen`, 2.1 Q-rows unchanged; disabled = no spawn + leaves the fill, muted = keeps spawning + quota; law `1/sqrt(max(1, N_eff·μ₂/μ₂_Hann))` + parent coherence blend, per-(k,s) gains ramped over one cell, 2.1 law ≡ new law at window 0; mixer per (k,s), enable/mute/solo 10 ms ramps, additive solo, buses `float[20][4][2][32]`; feedback tap still zero; CPU INFO only (flag > 28 %); N2 unread-param row with the reduced set; thread rules unchanged; fork discipline (drift check 1 = 0).
</user_constraints>

## Project Constraints
- Trunk on `main`. Path-scoped commits only: `git commit -- plugins/O-GranulatorInstrument`. Re-check `git branch --show-current && git status --short` right before every commit. Foreign dirty paths right now (**VERIFIED** `git status`): `.claude/agent-memory/dsp-agent.md`, `.claude/agent-memory/gui-agent.md`, `modules/registry.yaml`, untracked `.claude/compaction-snapshot.md`, untracked `plugins/O-GrainScatter/.planning/improvements/adaptive-grain-length/nielsen-dafx26.txt`. Never sweep them in. Never tag.
- Build with `./scripts/build-and-install.sh O-GranulatorInstrument`; long commands in the background (600 s watchdog). Never build the parity reference from the parent working tree (uncommitted preset work); use `git archive 82406fce` / `git show "${SHA}:path"` with braces (zsh `:P` trap).

---

## Summary (what the plan must absorb)

1. **Q1 — use the suite's own out-of-tree null machinery, not an in-harness link.** The parent already renders other commits headlessly: `git archive <sha> plugins/O-Granulator modules | tar -x` into `$TMPDIR`, a 15-line CMake project that `add_subdirectory`s JUCE + the archived plugin, and `ouaricon_add_processor_console(OuariconGranulator <archived Source> <driver.cpp> nN)` with `OGRAN_TEST_HOOKS=1` (`82406fce:…/tests/n1/run.sh:41-68`). **VERIFIED.** A separate executable means zero ODR/namespace questions, nothing parent-side in our CMake graph (console exclusion is structural), and the parent processor is built exactly as its own harness built it. Add `tests/parity/{run.sh,parent_render.cpp}`; our harness gets a `--parity <dir>` mode that reads the parent `.f32` renders + grain logs.
2. **Q2 — the frozen parent is NOT derandomised by default.** Frozen spawns draw `PosSpray` even at spray 0 (min spray ±½ grain, `82406fce:…/dsp/Spawner.h:299-306`). The parent's own test hook `freezeNoMinSpray` (`Engine.h:246`, used at `Engine.cpp:1148`) restores the live formula + plain clamp (`Spawner.h:290-298`). **VERIFIED.** The null must set it. Transform: parent read time = `W0 − D + step·(age + frac)`; ours = `p0 + step·(age + frac)`; equality needs `Δ = (W0 − D) − p0` integer **and `Δ ≡ 0 (mod 2^ℓ)`** (mip decimation phase), with the parent input = material delayed by Δ (§2).
3. **Q3 — n = 4000 latched values per side per control** gives a two-sample KS critical D = 0.060 at α = 1e-6 vs D ≈ 0.25 for a ×0.5 error, a variance-ratio band [0.89, 1.13] at 6σ vs 0.25, and a 2 % range miss probability 6e-18. Jitter is tested on the **i.i.d. displacement** `t_k − t_0 − k·I`, plus a χ² (19 df, crit 63.7) on even-k intervals. ≈ 5 s of audio per side at 800 g/s; < 10 s harness time (§3).
4. **Q4 — the parent publish is: masks = 128 relaxed `std::atomic<uint64_t>`; custom table = SpinLock-guarded copy + `atomic<uint32_t>` version, copied on the audio thread by a NON-BLOCKING try-lock at the grid only when the version moved** (`Engine.cpp:269-298`, `PluginProcessor.cpp:150-165`). **VERIFIED.** It ports without touching any leaf: `ScaleSource`/`ActiveSet` live in the forked `ScaleQuantiser.h`, which names `GranulatorParamIds` **only in comments** (`forked/ScaleQuantiser.h:51,62`) → reclassify it VERBATIM. The parent's codec uses **three sibling children** `<Engine>`, `<CustomScale>`, `<DegreeMasks>` — not keys on `<Engine>` — and our Stage 1 codec already strips all three (`Source/PluginProcessor.cpp:222-234`). **VERIFIED.**
5. **Q5 — parent default patch: s1 only; mix 50 % (equal-power, wet ×sin(π/4) = −3.01 dB); knee identity ≤ −6 dBFS; reverb 0; output 0 dB; centre pan exact unity.** No other trim. **VERIFIED.** Velocity 127 at `vel_to_amp` 70 % is exactly 1.0f (`AmpEnv.h:63-68`, Sterbenz) — no neutralising needed. Measure against the parent at **mix 100 %**. Expected target ≈ **0 dB** (both chains are unity at N = 1, c = 0), i.e. **+6 dB louder than the provisional −6 dB** (`GiConstants.h:54`) — **[user flag]** (§5).
6. **Q6 — parent muted streams keep demand and keep spawning**: demand is `if (st.enable)` only (`Engine.cpp:362-363`); the scheduler skips only disabled streams, "muted / frozen streams fire too" (`:1041-1043`). **VERIFIED.** Matches the CONTEXT default; no decision needed.
7. **Q7 — the bitwise row holds, but only through the parent's P14 fast path**: step 0 && tiltT == 0.5 renders `hannWindow (phase)` closed form (`Engine.cpp:1477-1487`), not the bank table. The bank's step-0 table (2048-pt Tukey α=1, lerp) is not bitwise (LUT lerp error ≈ −112 dB measured on a similar table, memory `pattern_tukey_taper_is_a_hann_half`), so a "≤ −120 dB" fallback would likely **fail**. Keep P14 and the 2.1 product order `w·killGain·fade`. **VERIFIED/INFERRED** (§7).
8. **Integer thinning breaks in two new corners (§10.2).** (a) cap 32 with > 32 active pairs: `q < 1` → `m = 0` is forced to 1 (`VoiceQuotas.h:148`) → `Σm > Q`. (b) jitter: onsets displaced ±J·T/2 put up to `m + 1` grains in `L_max`. Fix (b) with `T = (L_max + 1)/(m − J)`; (a) is unavoidable → R3 kills are expected there. **INFERRED** — **[flag]**.
9. **The parent coherence model assumes a moving write head.** With a positional tape at scan 0, overlapping forward grains read content lagged by the onset spacing (memory `pattern_forward_grain_read_is_coherent`: coherence needs read velocity == content velocity). Verbatim inputs give c = 1 at spray 0 → up to **−24 dB** too quiet on noise at N = 400. Defaults (spray 10 %, region ≥ 50 ms) give c = 0, so default-patch gates are unaffected. **INFERRED** — **[decision]** (§11.1).
10. **There are no render goldens in-tree** (`tests/golden/` holds only spec TSVs; **VERIFIED** `ls`). "Re-baseline" needs a Wave-0 task that hashes a fixed render set on the **2.1 engine before any 2.2 edit**. The pinned-to-0 proof also needs a `coherenceOff` hook: spray 0 makes c = 1, which changes the law for N > 1.5 (§12).
11. **Every carried 2.1 row now sees spray 10 / jitter 20 / spread 30**, because `base()` does not set them (`engine_gates_21.h:222-223`; **VERIFIED**). The planner must classify each row (§12.2).
12. **Spawner draws must match the parent pick, not a snapped continuous draw.** Scale on = `ActiveSet::pick` = uniform `nextInt` over active degrees in `window (sN_pitch, span)` (`forked/ScaleQuantiser.h:148-161`, parent `Spawner.h:198-202`). CONTEXT's `Q(pitchRand·(2u−1))` is shorthand for this. **VERIFIED.**

---

## 1. Parent-at-pin reference build (Open Q1)

### 1.1 Minimal file set — **VERIFIED**
- Parent `Engine.cpp` is the only DSP TU. Its include closure is **all 20 headers of `Source/dsp/`** (Analysis, CaptureWriter, ControlGrid, EngineConstants, FeedbackChain, Grain, GrainFx, ModMatrix, ModSources, OutputLaw, Quotas, ScaleQuantiser, Spawner, StreamScheduler, TempoTracker, UiTelemetry, WindowMorphBank, Engine.h) + modules grain-engine, pitch-collections, reverb-unit (`ReverbUnit.h`, global-namespace class) + `juce_core/juce_audio_basics/juce_dsp` (grep of `#include` at `82406fce`). No `GranulatorParamIds.h` is needed by the DSP (comments only).
- `ParamPointers` is raw `std::atomic<float>*` with null-tolerant `latch` (`ControlGrid.h:169-252, 438-486`; null pointer = keep default, `:455-462`), so a bare `ogran::Engine` could run without a processor. **But** reproducing the parent harness's parameter path (APVTS raw values via its `setP` exact-raw search, `engine_gates.h:110-131`) is safer with the whole processor. The `n1` precedent does that.
- Module drift: `git diff 82406fce HEAD -- modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump` is **empty**, and the working tree touches only `modules/registry.yaml`. **VERIFIED.** The script should still refuse on drift (`n1/run.sh:41-44` pattern).

### 1.2 Recommended shape (precedent `82406fce:…/tests/n1/run.sh`)
```
tests/parity/run.sh [--strict] [WORK]        # WORK default ${TMPDIR:-/tmp}/ogi-parity (never inside the repo)
  1. refuse if grain-engine / pitch-collections / reverb-unit / scripts/param-dump differ from 82406fce
  2. rm -rf $WORK/pin; git archive 82406fce plugins/O-Granulator modules | tar -x -C $WORK/pin
  3. $WORK/proj/CMakeLists.txt: add_subdirectory(~/JUCE), add_subdirectory($WORK/pin/plugins/O-Granulator),
     ouaricon_add_processor_console(OuariconGranulator $WORK/pin/…/Source $REPO/…/tests/parity/parent_render.cpp parity)
     + cxx_std_20 + OGRAN_TEST_HOOKS=1; modules symlink → $WORK/pin/modules; configure once, ninja incrementally
  4. parent_render <rowfile> <outdir>  → <row>.f32 (L/R interleaved) + <row>.grains.tsv (SpawnRecord fields) + activesets.tsv
  5. our harness: O-GranulatorInstrument-render-harness --parity <outdir>  → renders our side, compares, exits rc
```
- Row specs live in `tests/parity/ParityRows.h`: plain structs + param-ID/value lists, no `ogi`/`ogran` types, included by both drivers. One source of truth stops the two sides drifting apart (memory `pattern_test_fixture_mirrors_drift_silently`).
- The driver drains `diag.log` (ring of `kSpawnLogSize = 4096`, `Engine.h:366`) after **every** block, through `getEngineForTest()` (`PluginProcessor.h:236-238`, under `OGRAN_TEST_HOOKS`). **VERIFIED.**
- **Console-exclusion coverage:** (a) `grep -c "ogi-parity\|82406fce\|O-Granulator/Source"` over our build's `compile_commands.json` → 0; (b) `nm -C` on the installed VST3/AU binaries and the harness: 0 `ogran::` symbols. Both rows are cheap and fire if someone later links parent code in. **INFERRED.**
- The first build compiles the archived plugin target too (ParamDump's `add_dependencies` on the plugin, `ParamDump.cmake:192`). Expect several minutes; later runs are incremental. Run it in the background. **VERIFIED** (dependency) / **INFERRED** (time).
- **Rejected:** linking `ogran::Engine` into our harness. It works in principle: distinct namespaces, identical module headers, quoted includes resolve siblings first. But it needs per-source include dirs, `OGRAN_TEST_HOOKS` per source, and the full JUCE-flag parity. It also puts parent code in our build graph. No benefit over a second process.

## 2. Position / frozen mapping (Open Q2) — derived from code

### 2.1 Parent side — **VERIFIED**
- Ring write: `ring.push (x·g + fb)` with g = input-gain ramp (0 dB → `dbToGain` snaps to exactly 1.0f) and fb = 0 → ring time k holds input sample k exactly (`CaptureWriter.h:144-156`). Frozen: no push (`:95`).
- Freeze rising edge at tick T_f: `W0 = writeCount + freezeDeferCells·32`, `freezeDeferCells = rampCellsFor (fs, 5 ms) + 1` (9 @ 48 k); `heldCaptured = W0 − margin(2) − 1 = W0 − 223` (`Engine.cpp:79, 834-835`). Spawns inside the deferral use `headLead` (`:1138-1150`), so **enable the stream only at a tick ≥ freezeStopTick**. The enable edge restarts the scheduler: first onset on the tick sample, u₀ = ½ (`StreamScheduler.h:106-110, 198-202, 227-232`).
- Frozen + `freezeNoMinSpray`: `D = margin(ℓ) + reflect (pos·S + spray·S·(u−½), [0,S])`, `S = min (buffer_length·fs, heldCaptured)`; forward floor `D ≥ margin(ℓ) + r·len + 1`; reverse `D ≥ margin(ℓ) + 1` and `D ≤ max (margin+1, heldCaptured − r·len)` (`Spawner.h:290-311`). `margin(ℓ) = 63(2^ℓ−1) + 8·2^ℓ + 1` = 9 / 80 / 222 (`BandlimitedReader.h:273`).
- Anchor: `wOn = W0` (frozen, `Engine.cpp:1208`); `readAbs = (wOn − D) + step·frac` (`Spawner.h:585`); logged as `rec.anchor` (`Engine.cpp:1347`).
- Read: `delay = (W − ⌊readAbs⌋) − round(frac·2²⁸)/2²⁸` → `locateStart` reads absolute time `W − delay = ⌊readAbs⌋ + fq` via `levelPos` (`Engine.h:561-617`, `BandlimitedReader.h:285-290, 429-439`).

### 2.2 Our side — **VERIFIED**
- Tape level-0 index n = material frame n (equal rates: bit-exact copy, K3), zeros before 0 and in the 1074-frame flush; levels in MipRing storage convention (T1/T2 bitwise, SUMMARY-2.1).
- Scan 0 → σ = 0 → `c = lo + wrap (x·W, W)` (`ScanPlayhead.h` `scanCentre`); forward `p0 = reflectInto (c (+ spray term), lo, hi − r·len)`; `readAbs = p0 + r·frac` (`Engine.cpp:841-886`); the read is `levelPos (ℓ, ⌊pos⌋, pos − ⌊pos⌋)` (`MipTape.h:181-202`). This is the same function as the ring, unquantised.

### 2.3 Transform
- Feed the parent `x_par[k] = M[k − Δ]` for k ∈ [Δ, Δ+F), else 0. Then parent ring time τ ≡ our tape frame τ − Δ, **for the level arrays too, iff Δ ≡ 0 (mod 2^ℓ)**: level-ℓ index j is the half-band output at parent index 2j+1 (`push` cascade), so a shift that is not a multiple of 2^ℓ changes the decimation phase. **INFERRED** from the storage convention (RESEARCH-2.1 §3.2).
- **Null condition per row:** `Δ = A − p0` with `A = W0 − D` (closed form above), `Δ ≥ 0`, `Δ mod 2^ℓ = 0`, `Δ + F ≤ W0 − 223`, and both read spans inside the material. The harness computes A in closed form before rendering the parent, and asserts `A == rec.anchor` and `rec.writeHead == W0` from the parent log after (a self-check of the derivation, not a tuning step).
- **Exactness:** every term is an integer if (i) T_f is a multiple of 32 (so W0 ≡ 0 mod 32), (ii) the parent span binds: `buffer_length·fs < heldCaptured` with a dyadic `S` (e.g. 2.0 s → 96 000 @ 48 k), (iii) `pos_par` is dyadic and `pos·S ≥ r·len + 1` (floor not binding; needed for pitched rows where r·len is irrational), (iv) our `x`, `loop_start/end` are dyadic and F is chosen so `x·F` is an integer. Then `A ≡ −margin(ℓ) − pos·S (mod 4)`: margin(2) = 222 ≡ 2, so pitched ℓ = 2 rows need `p0 ≡ 2 − pos·S (mod 4)`. Pick F per row to satisfy it (the harness solves; the row table states F). **INFERRED** (arithmetic).
- Fraction: at rate 1, integer start and integer interval (`fs/density`, e.g. 48000/10), every frac is 0 → reads are bitwise. Pitched rows differ only by the parent's 2⁻²⁸ frac quantisation, ≤ 1.9e-9 samples → < −150 dB. **INFERRED.**
- latchLevel: `rate < 2 → 0; else min(maxLevel, ilogb rate)` (`BandlimitedReader.h:253-257`) → parent (max 2) and ours (max 4) agree for |pitch| ≤ 24 st (rate 4 → ℓ 2 on both). **VERIFIED.** Null rows stay within ±24 st.
- **Controls (non-vacuous):** D15's one-sample onset shift (ours `noteOn` at T_on + 1); window step + 1; and a **Δ + 1 at ℓ ≥ 1** row that must fail (it proves the mip-phase term is real).
- **Output alignment:** `out_par[T_e + n] ↔ out_ours[T_on + n]`. Compare n ≥ attack + 2 cells: our amp env scales the voice bus per sample, so after the attack every sample is env 1.0 regardless of when its grains spawned. Parent at mix 100 % and output 0 dB: output IS the wet (`n1_render.cpp` header, `Engine.cpp:1684-1698`).
- **Our wet safety is not an identity:** `1.9953·tanh(x/1.9953)` deviates by x²/12 relative (−62 dB at |x| = 0.1). Use material at ≤ −40 dBFS peak (deviation ≤ 8.3e-6 → ≈ −105 dB peak, below −110 dB RMS) plus a liveness floor (reference RMS ≥ −70 dBFS). The parent knee is identity below 0.501 (`OutputLaw.h:182-188`). **VERIFIED/INFERRED.** Gain alignment = least-squares scalar; residual = `20·log10(‖p − g·o‖/‖p‖)`.
- **Parent `grain_limit` maps to 32/64/96/128** (`ControlGrid.h:357`), ours to 32/64/128/256. Null rows keep N far below 32. **VERIFIED.**

## 3. Statistical arm (Open Q3)

### 3.1 Latched fields and their exact distributions — **VERIFIED** (formulas, parent `Spawner.h:169-268`, `StreamScheduler.h:46-49`)
| Control | Statistic (both sides) | Law under the null |
|---|---|---|
| spray s | `(p0 − c)/W` ours; `(D − margin(ℓ) − pos·S)/S` parent | U(−s/2, s/2) |
| size_rand z | `len/(size·fs·1e-3) − 1` | U(−z/2, z/2) (keep size mid-range: no clamp) |
| gain_rand γ | `20·log10 g` from `gainL`/`gainR`·panGain⁻¹ (spread 0 in this row) | U(−24γ, 0) dB |
| spread σ | `px = 4·atan2(gR, gL)/π − 1` | U(−σ, σ) |
| pitch_rand ρ (scale Off) | `pitchCents/100` | U(−ρ, ρ) |
| pitch_rand (scale on) | member index within `[lo, hi)` | uniform categorical |
| reverse p | `dir == −1` | Bernoulli(p) |
| jitter J | `e_k = (t_k − t_0) − k·I`, t = onsetSample − frac, k ≥ 1 | i.i.d. U(−JI/2, JI/2) |
| jitter J (histogram) | even-k intervals `Δ_{2j}` (independent: no shared u) | I·(1 + J·(u − u′)), triangular |

Adjacent intervals share a draw (ρ = −½). The KS/χ² null distributions assume independence, so use the displacements or the even-k subsample. **INFERRED.**

### 3.2 Bounds (n = 4000 per side, α = 1e-6 per test) — **INFERRED** (math; numbers computed this session)
- **KS two-sample:** `D_crit = √(−ln(α/2)/2)·√(2/n) = 0.060`. A ×0.5 scale error gives D ≈ 0.25 (simulated 0.26). Also KS vs the analytic CDF: one-sample `D_crit = 0.043`.
- **Moments:** mean `|m_o − m_p| ≤ 6·σ·√(2/n)` (= 0.134·σ). Variance: uniform `Var(s²)/σ⁴ = 0.8/n`, so the log-ratio SE is √(1.6/n) = 0.020 → ratio ∈ [0.887, 1.127] at 6σ. ×0.5 → 0.25 (≈ 70σ).
- **Range:** observed min/max inside the analytic support + 1e-6, and range ≥ 0.98 × width. Miss probability ≈ 2·0.99⁴⁰⁰⁰ ≈ 6e-18; ×0.5 → 0.5 × width.
- **Bernoulli (p = 0.3):** `|p̂_o − p̂_p| ≤ 6·√(2p(1−p)/n)` = 0.061; ×0.5 → 0.15 off.
- **Jitter χ²:** 20 equiprobable bins of the triangular law on the even-k intervals, df 19, crit 63.7 (α 1e-6). At n = 2000, ×0.5 gives χ² ≈ 1067 and the true J gives ≈ 20 (simulated). Scale-on categorical: χ² with df = members − 1 (df 9 → 44.8).
- **Determinism:** seeds are fixed, so a row cannot flake run to run. α only matters when a code change re-keys draws; 6σ keeps that safe. **INFERRED.**
- **Controls:** apply the ×0.5 to our latched samples in the harness (proves the test has power), plus one engine-side hook (jitter ×0.5 in the scheduler callback), so at least one control goes through the audio path. **[planner choice]**

### 3.3 Cost
- One combined render per side covers all slot-independent controls (CounterRng is keyed by slot, so the controls do not interact). Settings: spray 30, size_rand 50, spread 70, pitch_rand 7 Off, reverse 30, position 50 %, region ≥ 2 s; gain_rand gets its own spread-0 row. A separate jitter row has quotas uncapped and no pitch. Use 4 streams × 200 g/s × size 20 ms (N = 4, no cap) → 800 g/s → n = 4000 in **5 s** of audio.
- Per-side render ≈ 0.1–0.5 s (2.1's 33 rows took 6.7 s in total). Stat arm < 10 s. Parity build: minutes once, then incremental. **INFERRED.**

## 4. Scale state (Open Q4)

### 4.1 Parent mechanism — **VERIFIED**
- Processor (`PluginProcessor.h:130-134, 244-247, 261-277`): `std::array<std::atomic<uint64_t>, 4·32> degreeMasks` (init all-on), `juce::SpinLock customLock`, `PitchCollections::Table customTable` (trivially copyable ≈ 264 B), `customText/customName`, `std::atomic<uint32_t> customVersion`. `maskIndex(stream 1-based, coll) = (stream−1)·32 + coll`; `canonicalMask` **sets every bit ≥ degreeCount** (`:131-134`), so any partial mask on a < 64-degree collection has **bit 63 set**.
- Binding: `ScaleSource {masks, lock, customTable, customVersion}` + static_asserts that the processor's and the engine's index/choice maps agree (`PluginProcessor.cpp:150-165`).
- Writers (message thread): `setCustomScale` parses with `PitchCollections::tableFromText` **outside** the lock, swaps under `ScopedLockType`, resets every stream's Custom mask to all-on, then `customVersion.fetch_add (acq_rel)` (`:373-392`). `setDegreeMask` stores `canonicalMask` relaxed (`:422-429`).
- Audio thread at each grid tick: `copyCustomTable (false)`. If the version moved, it takes `ScopedTryLockType`; a miss retries next tick, and it never blocks (`Engine.cpp:269-298`). `prepare` copies blocking (`:81`). `updateScaleState` loads `masks[scaleMaskIndex (s, choice)]` relaxed and rebuilds `ActiveSet` on key `(choice, mask, span, customVersion)`. `PitchSel = {set, window (pitch, span)}` (`:303-326`).
- **Port under `ogi`:** `ScaleSource`, `ActiveSet`, `scaleMaskIndex`, `scaleChoiceToId` are already in the forked leaf and compile as-is (only comments name `GranulatorParamIds`, `forked/ScaleQuantiser.h:51,62`). Move `ScaleQuantiser.h` from `ADAPTED (pending 2.2)` to **VERBATIM** in the manifest (its local sha256 is already the renamed pin blob). `copyCustomTable`, `updateScaleState` minus the `absSets`/follow block (2.5b, removed in the instrument) and `tableForChoice` are Engine code, rewritten in `Engine.cpp`. `PitchSel` lives in the parent `Spawner.h`, which is not a leaf, so re-declare it in `GiSpawner.h`. `PitchCollections::builtIns()` is already warmed in our constructor (`Source/PluginProcessor.cpp:58-59`).
- **Thread-rule note:** the try-lock is a lock acquisition attempt on the audio thread. It is wait-free on that side, and it is the parent mechanism D17 adopts verbatim. **[flag — confirm this reads as compliant with "no locks"]**

### 4.2 Codec — what exists, what to add
- **Now (VERIFIED `Source/PluginProcessor.cpp:205-300`):** `getStateInformation` writes `<Engine seed(int64 string) stateVersion modDestTableVersion/>` + an empty `<Slots/>` onto the `copyState()` copy, after stripping Engine/Slots/CustomScale/DegreeMasks. `setStateInformation` reads `<Engine>`, strips before `replaceState`, restores `uiLanguage` and the seed only. It has no reset-to-defaults (none needed until now).
- **Add (parent `:448-612` verbatim modulo names):**
  - `<CustomScale name text period version(int, &0x7fffffff)/>`
  - `<DegreeMasks><Mask stream collection bits="%016x"/></DegreeMasks>`, written **only for non-all-on masks**: `juce::String::toHexString ((int64) mask).paddedLeft ('0', 16)`
  - restore via `applyNonParameterState (custom, masks, seedVar)`, which calls `resetNonParameterState()` **first** (12-EDO, all-on, version bump). Absent seed keeps the live seed (there is no default seed). Text non-empty → `setCustomScale` (parse failure keeps the default). Each mask needs `bits.length() == 16 && containsOnly(hex)` → `getHexValue64()` (never `getLargeIntValue`).
  - Every property comes back as a String var: use `isVoid()` guards only (memory `critical_valuetree_xml_roundtrip_loses_type`).
- **D17 wording:** "keys live on the `<Engine>` child" vs the parent's sibling children. "Parent key names unchanged" plus the existing strip list both point to **siblings**. **[flag, low risk — recommend siblings]**
- **Rows:**
  - round-trip of all 4 × 32 masks (random 64-bit patterns, canonicalised) + custom scale
  - a 64-degree custom scale with only bit 63 set (not canonicalised away)
  - an absent-`<CustomScale>` blob after a non-default one → 12-EDO and all-on
  - a two-reopen sequence (save → load → change → save → load) → the second state wins, and `parameters.state` has 0 Engine/CustomScale/DegreeMasks children; control `testNoStateStrip` (parent ST2n idiom, `PluginProcessor.h:239`) shadows
  - malloc-logger A1-style: `setCustomScale` + `setDegreeMask` between blocks while 6 notes sound → 0 audio-thread allocs, with `customCopies` > 0 (liveness)

## 5. D16 target (Open Q5) — **VERIFIED** unless noted
- Parent defaults (`GranulatorParamIds.h:112-175`): only s1 enabled (`:129`, special-cased at `PluginProcessor.cpp:228-229`). Mix 50 % → `mixGains` equal-power, wet = sin(π/4) = 0.7071 (`ControlGrid.h:147-153`). Output 0 dB → `dbToGain` snapped 1.0f. reverb_mix 0 → the unit never runs. The D9 knee is identity ≤ 0.501 (`OutputLaw.h:182-188`). Level 0 dB, and pan centre → `panGains` returns exactly (1, 1) (`ControlGrid.h:157-164`). input_gain 0 dB → ring = input. No make-up gain anywhere.
- Ours: `kVoiceGainDb = −6.0` (`GiConstants.h:54`) → `voiceGainLin` (`Engine.cpp:45`), applied in `mixPiece` (`:941`). Velocity: `data2/127.0f` = 1.0f (`Engine.cpp:554`). `velocityGain = (1−d) + d·v²` (`AmpEnv.h:63-68`) at v = 1, d = 0.7f: 1 − d is exact (Sterbenz), so the sum is exactly 1.0f. **INFERRED** (float arithmetic). Amp sustain 100 % → flat env.
- **Measurement recipe:**
  - stationary noise (hash noise, 8 s, −30 dBFS RMS, peak ≤ 0.1, so the knee and tanh stay ≤ 0.01 dB)
  - parent frozen (≥ 8 s captured so S = span), **mix 100 %**, default stream params, s1 only, 20 s
  - ours: 1 voice at root (48), vel 127, defaults, s1 only, kVoiceGainDb **0** for the measurement
  - 4-render average with 4 seeds, skipping the first 0.5 s on both sides; target = `20·log10(RMS_par/RMS_ours)`; old-vs-old seed-spread row ≤ 0.2 dB
- At defaults c = 0 on both sides (spray 0.1 × 8 s ≫ 5 ms → `cSpray` = 0, `OutputLaw.h:241`). N = 1 → g_out = 1. Hann μ₂ is shared, and spread is power-neutral in expectation (`E[2cos²θ] = 1 − E[sin(π·px/2)] = 1`). **Expected target ≈ 0.0 dB**, so `kVoiceGainDb` → ≈ 0 dB. **INFERRED.**
- **[user flag]** That makes a single note 6 dB louder than 2.1. A 6-note chord then sits about +7.8 dB (incoherent sum) above one voice and engages the §18 knee sooner. If the user means "parent as it sounds by default" (mix 50 %), the target is −3.01 dB instead. Recommend mix 100 %: an instrument has no dry path, and D16 says "default **stream** parameters".

## 6. Muted streams in the water-fill (Open Q6) — **VERIFIED**
Parent demand = enabled only, mute ignored (`Engine.cpp:358-366`); scheduler skips disabled only, muted fire (`:1040-1043`); audibility `!mute && (!anySolo || solo)` with `anySolo` over **enabled** streams (`:364, :396`). A disabled stream's ringing grains keep the uncapped law `(n, q = grain_limit)` (`:378-385`). → "muted keeps demand and spawns" is the parent behaviour and satisfies "unmuting is instant". Port as is. Note: the parent has **no enable ramp on the output** (disabled grains ring out; only the feedback tap ramps `fbEnRamp`); CONTEXT's "enable ramps 10 ms" is binding and our 2.1 `audibleRamp` already folds enable in (`Engine.cpp:277`). Parity rows never toggle enable mid-render, so DSP-01 is unaffected. **INFERRED.**

## 7. Window 0 bitwise (Open Q7)
- Build paths differ. **VERIFIED.**
  - Bank step 0: `float (clamp (tukey (1.0, φ)))` with `φ = i/2047` and `u = min(φ, 1−φ)`, so `0.5(1 − cos(π·u/0.5))`. That is 2048 points in double, stored as float and read as `clamp (warp (float φ))·2047` + float lerp (`forked/WindowMorphBank.h:80-88, 146-152, 161-177`).
  - 2.1 renderer: `hannWindow (phase) = 0.5f − 0.5f·cos (2π_f·(float) phase)`, closed form in float (`forked/OutputLaw.h:82-86`, `GiRenderer.h:156`).
  - Not equal: table lerp error vs closed form ≈ 1e-6 relative (−112 dB on a comparable 2048-pt LUT, memory note).
- The parent keeps the closed form for `windowStep == 0 && tiltT == 0.5f` (P14, `Engine.cpp:1477-1487`) and reads the bank otherwise (`:1504-1512`). **VERIFIED.** So "window 0 ≡ 2.1" holds **bitwise** if 2.2 ports P14 and keeps the float grouping:
  - parent `W = hann·killGain·fade`, then `bl += W·L·gainL`
  - 2.1 `gk = w·killGain·fade`, then `bl += l·gk·gainL`
  - Commutative products and the same statement shape (FP contraction is per statement) give identical results. **INFERRED.**
- Rows: (a) window 0 / skew 0 render == stored 2.1 hash (§12); (b) control: a hook forcing the bank path at step 0 must differ (proves the fast path is under test); (c) law: `streamPower (n, n, hannSel, c = 0, spread 0)` == the 2.1 `voiceLawGain` bitwise (`OutputLaw.h:137-168` reduces exactly at c = 0, `:97-99`).

## 8. Scheduler port + merge
- **Per (k,s) state:** `GrainScheduler sched[20][4]`, `ScanAnchor scan[20][4]` (Free-run: `freeRun[4]`), `uint64 epoch[20][4]`, `periodT[20][4]`, `quotaM[20][4]`, `CellLerp lawGain[20][4]`. Run only for counted voices × enabled streams with a tape loaded. On a note-on → `reset()` at the event sample (2.1 path). On a stream enable edge while a voice sounds → `epoch += sched.getOnsetCount(); reset()` at the tick (parent R4, `StreamScheduler.h:227-232`). **INFERRED** (port).
- **Draw keys:** `idx = epoch_kS + k`; `u_k = Draw (seed, kSchedStream + s, (serial << 24) | (idx & 0xFFFFFF)).nextFloat (Slot::Jitter)` with `u_0 := ½` (`StreamScheduler.h:198-202`). The callback is `T_kS·(1 + J·(u(k+1) − u(k)))` and returns `T_kS` with no draw when J = 0 (`:205-210`). Grain draws use the same counter on `kGrainStream + s`. **VERIFIED** (parent) / **INFERRED** (key). The 24-bit index is about 23 h at 200 g/s.
- **rescheduleNext on a quota decrease** (2.1 `Engine.cpp:522-523`) must pass the jittered gap of the pending onset, `T_new·(1 + J(u(n) − u(n−1)))` with `n = getOnsetCount()`, so the decrease is jitter-consistent and block-invariant (it is a pure function of index and grid snapshot). **INFERRED.**
- **Merge, no allocation:** `PendingOnset merged[20·4·kOnsetCap]` (2640 entries; one onset per sample per pair, `GrainScheduler::kMinInterval = 1`) as an Engine member. Fill it by iterating pairs in **(serial ascending, s ascending)** order (sort the ≤ 20 sounding voices by serial once per segment), then do a **stable** insertion sort on offset alone. Ties end up in (offset, serial, stream) order and the voice slot never enters the key (V3). Typical count ≤ 80 per cell. **INFERRED.** The voice-end list keeps its (offset, serial) order and is applied before spawns at a shared offset (2.1 `Engine.cpp:783-793`).
- **Thinned period with jitter:** see §10.2.

## 9. Spawner widening — parent → ours (**VERIFIED** parent lines; target = `Engine::spawn` `Engine.cpp:812-921` + `GiSpawner.h`)
| Field | Parent (`Spawner.h`) | Ours in 2.2 |
|---|---|---|
| Slots | `Jitter=0, PosSpray, Size, Pitch, Reverse, Pan, Gain, FxCutoff, FxReso, FxDrive, FxCrush, FxCrushProb, FxRing, Window, Skew` (`forked/EngineConstants.h:246-266`) | unchanged; `Draw (seed, kGrainStream + s, counter)` before acquire |
| Size | `size·(1 + z·(2u−1)·0.5)`, clamp, ·fs (`:169-173`) | already wired (2.1); per stream |
| Pitch | Off: `st = pitch + ρ(2u−1)`; `pitchCents` from the same slot; On: `pick (draw, lo, hi)/100` if ρ > 0 and hi > lo, else no draw; clamp; `rate = exp2(st/12)` (`:182-204`) | `st = κ·key + pitch` then the same rand term, `window (sN_pitch, span)` (parent ±24 window, `forked/ScaleQuantiser.h:148-155`), clamp ±48, `·tapeRate/fs`. At root `κ·0 + pitch` is bit-equal to the parent `st`. Latch `g.pitchCents` |
| Reverse | `rp ≥ 1 → −1; rp > 0 → u < rp` (`:207-211`) | same; `g.dir`, `g.step = dir·rate`, `readAbs = p0 + step·frac`; region fit `[lo + reach, hi)`; renderer reverse wrap `readAbs < wrapLo → += wrapW` (2.1 handles forward only, `GiRenderer.h:188`) |
| Spray | `x = pos·S + s·S·(u−½)` → reflect + floor (`:221-238`) | `c + s·W·(u−½)` → `reflectInto (…, lo, hi − reach)` |
| Spread | `px = σ(2u−1)` → `panGains (px)` → `gainL/R` (`:247-253`) | verbatim |
| Gain | `g = 10^(−γ·u·24/20)` float, × both (`:257-263`) | verbatim |
| Window | `stepFor (window)`, `makeTilt (0.5 + 0.45·skew)` (`:266-268`) | verbatim → `g.windowStep/windowSkew/tiltT/A/B`; renderer P14 branch (§7) |
| Fill | `g.stream`, … (`:572-594`) | + `vg.slot = s` (stream N reads slot N), tape of slot s, region of stream s |
| Log | `SpawnRecord` (`Engine.h:323-364`) | extend `GrainLogEntry` with stream, dir, gainL/R, windowStep, tiltT, pitchCents, centre c, W (the stat arm reads these) |

## 10. Quotas, thinning, stealing over 80 pairs

### 10.1 Mechanics
- Demand index `k·4 + s`, `N = streamDemand (density_s, size_s, fs)` for counted voice × enabled stream × tape loaded, else 0 (a stolen voice has demand 0). Call `waterFillN<80>` unchanged (`VoiceQuotas.h:71-139`). Zeros sort first and subtract 0.0 exactly, so at a single stream the share arithmetic on the nonzero entries is identical to `waterFillN<20>`, and **the Q0–Q3 rows pass bitwise unchanged**. **INFERRED** (algorithm, `:112-129`). The insertion sort is ≤ 3160 compares and runs only on change.
- Steal key: `live[20][4]` in one scan (by `vg.voice`, `vg.g.stream`). `pk[k][s] = 4·stolen(k) + 2·(live_kS > m_kS) + ((k,s) == spawning)` replaces the voice-indexed arrays at `GiSpawner.h:98, 120-121`. The lexicographic argmax, the killing bonus of 8, R3 and D25 are unchanged. **VERIFIED** (current code) / **INFERRED** (shape).

### 10.2 Two new corners for the integer bound — **INFERRED** **[flag]**
1. **More active pairs than the cap.** At cap 32 with 33–64 pairs, `q < 1` → `⌊q⌋ = 0` → forced to 1 (`VoiceQuotas.h:148`) → `Σm` up to 64 > 32. Live ≤ cap still holds through R3, but steady-state R3 kills are expected. Cap 64 with ≤ 64 counted pairs (16 × 4) gives q ≥ 1. Gate "0 steady-state R3 kills" only where `pairs ≤ cap`; report the kill rate at 64 pairs / cap 32 as INFO, and keep "live ≤ cap at every tick" as the hard row.
2. **Jitter.** Onset k sits at `kT + J·T·(u_k − ½)`. Any m + 1 consecutive onsets span ≥ `T·(m − J)`. Thin with **`T = max (fs/density, (L_max + 1)/(m − J))`**, valid for m ≥ 2. For m = 1 and J → 1 no finite T bounds it. Accept R3 kills there, or clamp J for capped m = 1 pairs (changes the jitter law while capped; parity is only defined uncapped). Recommend the first: no extra policy. `N_eff = len̄/T` still holds, since telescoping keeps the mean rate exactly 1/T.

### 10.3 Non-vacuous stimuli (memory `pattern_gate_stimulus_below_threshold_is_vacuous`)
| Gate | Stimulus | Pass | Control |
|---|---|---|---|
| live ≤ cap | 16 notes × 4 streams (4 slots), d 200, size 2000 (N = 400/pair), caps 32/64/128/256, jitter 0 and 20 % | live ≤ cap every tick; 0 R3 kills steady state where pairs ≤ cap and m ≥ 2 | `thinningOff` → kills > 0 |
| stream-axis starvation | 1 voice: s1 N = 400 fills cap 64 alone; s2 enabled (d 10, size 100 → N = 1) | `q = (63, 1)`; s2 live ≥ 1 within L_max + 1 cell; s1 → ≤ 63 | 2.1 Q2 regime: `periodThinOff` + `overQuotaOff` → s2 starves |
| voice-axis starvation | 2.1 Q2 with 4 streams | as Q2 | as Q2 |
| cap lowered live | 8 notes × 4 streams, 256 → 32 | ≤ 32 within L_max + 5 ms + 1 cell | `thinningOff + d25Off` stays high |
| water-fill asymmetry | demands (400, 1, 1, 1) cap 64 | q = (61, 1, 1, 1) exactly | `noSort` |

## 11. Output law, coherence, mixer

### 11.1 Coherence — **[decision]**
- Parent: `c = (1 − rev)·(1 − clamp(spray·span/(5 ms·fs)))²·c_pitch`, with `c_pitch = (1 − |pitch|/0.5)²·(1 − ρ/0.5)²` when the scale is off (`OutputLaw.h:231-248`, call at `Engine.cpp:372-376`), and `P = max (c·S2, (c·S1)²) + (1−c)·S2`. **VERIFIED.**
- Premise: in a live ring, forward grains at rate 1 all read `x(t − D)`, so they are coherent. Ours at scan 0 reads `p0 + r(t − s_k)`; the k-term `−r·s_k` decorrelates noise. It is coherent only when the read velocity r equals the scan velocity `v = scan·tapeRate/fs`. **INFERRED** (memory `pattern_forward_grain_read_is_coherent`).
- Options:
  - (a) Verbatim with `pitchSt = sN_pitch`, `span = W·fs/tapeRate`. Parity-faithful, but at spray 0, scan 0, N = 400 on noise it is 10·log10(N/(4·0.375)) ≈ 24 dB too quiet.
  - (b) Verbatim function, but feed the "pitch" argument with the relative velocity `12·|log2(r_kS / v_s)|` (∞ → c_pitch = 0 when v ≤ 0 or reverse). This gives c = 0 at scan 0 and the parent's value when the scan follows the grain (scan 100 % at root).
- Recommend **(b)** with rows: noise, spray 0, scan 0, N = 1 vs 400 → level within ±1.5 dB; and scan 100 %, root, N = 400, sine → law coherent branch within ±1.5 dB. Both options give c = 0 at defaults, so D16 and the default level-flat gate do not depend on the choice. Per-(k,s) c under (b), per-stream under (a).

### 11.2 Law + ramps
- Per (k,s) at the grid when inputs change:
  - `win = {stepFor (window_s), makeTilt (0.5 + 0.45·skew_s)}`
  - `n = streamDemand`; `n_eff = len̄/T_kS`
  - `g = outputGainFromPower (streamPower (n_eff, n_eff, win, c, spread_s, bank))`
  - pushed into `CellLerp lawGain[k][s]` every tick
- Always use the full-window branch (`n ≤ q`, RESEARCH-2.1 §1.3). A disabled (k,s) holds the uncapped form (§6). **INFERRED.**

### 11.3 Mixer + buses
- `VoiceBus { float s[20][4][2][32]; }` = 5120 floats = **20 KB** (2.1: 5 KB). **VERIFIED** arithmetic.
- Per sample, per stream: `acc_s = Σ_k (env_k·lawGain_kS)·bus[k][s]`, then `y = Σ_s acc_s·level_s·aud_s·voiceGain·pan_s`.
  - `aud_s = !mute && (!anySolo || solo)`, with `anySolo` over enabled streams and enable folded in per CONTEXT
  - level, pan and aud are 10 ms `ExactRamp`s (`kMixerRampMs`, `forked/EngineConstants.h:95`)
- Skip pairs with a free voice or with no live grains.
- **Bitwise trap:** start `y` from the first active stream's term, not `0.0f +`. `−0 + 0 = +0`, and the 2.1 hashes compare with `memcmp`. **INFERRED.**
- Click rows (QUAL-01) for mute, solo and enable toggles: d2 peak ratio ≤ 2.0 vs a same-motion control (the same 10 ms trajectory via `sN_level` 0 → −60 dB, which snaps to exactly 0, `ControlGrid.h:141`). Control: a 0 ms mixer-ramp hook must exceed (memory `pattern_step_click_gate_needs_same_motion_control`).

## 12. Goldens re-baseline proof

### 12.1 Procedure — **INFERRED**
1. **Wave 0, before any 2.2 Source edit:** add a harness mode `--dump-render-hashes`. It renders a fixed set and writes `tests/golden/renders-2.1.sha256` (memory: goldens as checksums only):
   - N1 job
   - V3 per-serial logs
   - Q4 voice renders
   - E1 envelopes
   - H1 swap
   - P2 long grain
   - a default-patch chord
2. **After 2.2,** with `spray 0, jitter 0, spread 0, gain_rand 0, pitch_rand 0, reverse 0, window 0, skew 0`, s2–s4 Off, **and the hook `coherenceOff` (c := 0)**: every hash must match.
3. Then render at real defaults and record old → new per job in SUMMARY.
- Without `coherenceOff`, spray 0 makes c = 1 (option a), and any job with N > 1.5 (N1: 100 × 0.3 = 30) changes law. That is a law change, not a defaults change, so it must be isolated.

### 12.2 Carried 2.1 rows
`base()` (`engine_gates_21.h:222-223`) sets no spray/jitter/spread, so in 2.2 every row inherits 10/20/30.
- Bitwise / determinism rows (N1, V3, P1, P2, T*, H*): pin all randomisation to 0 + `coherenceOff`.
- Law-vs-noise rows (Q4, level-flat): keep spray ≥ 10 % with region ≥ 50 ms (c = 0 by `cSpray`), or use `coherenceOff`.
- N1's control row also needs the 2.2 draws on (§13).
- N2: re-derive the unread set (2.2 reads every `sN_` row except FX, sync aliases, and s2–s4 FX); its control stays `s1_density`.

## 13. Block-size null at 80 schedulers — risks and closures
| Risk | Closure |
|---|---|
| merge tie order | key (offset, serial, s) per §8; never the slot or the iteration order |
| re-enable replays draws | per-(k,s) epoch (§8) |
| quota/rescheduleNext timing | grid ticks only; the jittered gap is a pure function of the index (§8) |
| mask / custom table / ActiveSet | read only at grid ticks. The harness sets them at **shared block boundaries** (event-split blocks, parent `engine_gates.h:220` idiom), else the first tick after the write moves with block size |
| try-lock miss | timing-dependent in a host; deterministic in the single-threaded harness. Not a null risk |
| mixer / law ramps | ExactRamp/CellLerp at absolute cell positions (2.1) |
| oversized blocks | all scratch is per 32-cell; `merged` and buses are fixed members |
- Null config: 4 streams × 4 slots, a 6-note chord with steals, every randomisation on (spray 30, jitter 50, size_rand 50, spread 70, gain_rand 50, pitch_rand 7 + a scale on s3, reverse 40, window 0.6, skew −40), cap 64, an s2 enable toggle and an s3 mute toggle at shared boundaries, blocks 32/441/512/4096/8192/var. Control: seed + 1 differs.

## 14. CPU INFO recipe (parent R86 method, `82406fce:…/RESEARCH-2.6.md:45`, `PLAN-2.6.md:117`)
- Config: 48 k / 128; 16 notes held (distinct pitches ±12 st); 4 streams on 4 loaded slots; d 200, size 1000 ms, defaults otherwise; grain_limit 256. Assert saturation (mean live ≥ 240) and no FX.
- Timing: `CLOCK_THREAD_CPUTIME_ID` around `processBlock` only, input precomputed, 5 s warm-up, then 7 × 10 s. Median %, min/max.
- Duty witness: thread/wall ≥ 0.9 per run, else "contended — no verdict". Bypass ratio: the same session with all streams disabled.
- Print `INFO C22`. If the median is > 28 %, flag it in SUMMARY-2.2 for the 2.3 levers.
- Run it alone: no concurrent build or parity run (the shared checkout shows up in the witness).

---

## Pitfalls checklist (PLAN must-haves)
| # | Pitfall | Guard |
|---|---|---|
| P1 | frozen parent randomises position at spray 0 | `freezeNoMinSpray` hook in every null row |
| P2 | Δ not ≡ 0 mod 2^ℓ → mip phase mismatch | closed-form Δ check + Δ+1 control |
| P3 | our tanh wet safety is nonlinear | material ≤ −40 dBFS peak + liveness floor |
| P4 | parent grain_limit 32/64/96/128 ≠ ours | null N ≪ 32 |
| P5 | bank read at step 0 is not bitwise | port P14 closed-form branch; bank-path control |
| P6 | `−0 + 0` breaks memcmp hashes | first-term init in the stream sum |
| P7 | coherence c = 1 at spray 0 (scan-0 tape) | §11.1 decision; `coherenceOff` hook for 2.1 equivalence |
| P8 | carried rows inherit 10/20/30 defaults | classify rows (§12.2) |
| P9 | no render goldens exist | Wave-0 hash capture on the 2.1 engine |
| P10 | `q < 1` forced to `m = 1` | R3 kills expected when pairs > cap; INFO row |
| P11 | jitter breaks the integer bound | `T = (L_max + 1)/(m − J)` |
| P12 | adjacent jitter intervals are dependent | displacement test + even-k histogram |
| P13 | scale pick ≠ snapped continuous draw | `ActiveSet::pick` (nextInt) |
| P14 | re-enable replays draws | per-(k,s) epoch |
| P15 | parent log ring is 4096 | drain each block |
| P16 | D17 sibling vs `<Engine>` keys | siblings (parent + existing strip list) |
| P17 | `getLargeIntValue` on masks | `getHexValue64` + 16-hex check |
| P18 | absent state keeps stale values | `resetNonParameterState()` first |
| P19 | stale child on reopen | strip before `replaceState`; two-reopen row + `testNoStateStrip` control |
| P20 | reverse grains use the forward-only wrap | add reverse wrap + region fit `[lo + reach, hi)` |
| P21 | parent code leaking into our build | out-of-tree parity; `nm`/compile-commands rows |
| P22 | fork drift / manifest | ScaleQuantiser → VERBATIM; ControlGrid edits (4 streams + 2.2 fields) recorded in col 7 + new sha col 5; drift check 0 |
| P23 | thread rules | no alloc in the merge/sort (fixed members); try-lock only in `copyCustomTable`; no listener writes; `ScopedNoDenormals` |
| P24 | shared checkout | `git commit -- plugins/O-GranulatorInstrument` only; re-check branch + status; never touch the 5 foreign paths |
| P25 | D14 | the execute precondition checks VERIFICATION-2.1 Human Verification before any 2.2 task runs |

## Open items for the planner
- **[user decision]** D16 target reference: parent **mix 100 %** (recommended; expected `kVoiceGainDb` ≈ 0 dB, +6 dB vs 2.1) or default mix 50 % (−3.01 dB).
- **[user decision]** §11.1 coherence input on a positional tape: (b) relative-velocity mapping (recommended) or (a) parent verbatim (`sN_pitch`).
- **[flag]** §10.2: R3 kills in steady state are expected when pairs > cap (cap 32 × > 32 pairs) and for jittered m = 1 pairs. "0 steady-state kills" applies only where pairs ≤ cap and m ≥ 2.
- **[flag]** §4.1: the parent's audio-side SpinLock **try-lock** (non-blocking) is the D17 "verbatim" mechanism. Confirm it reads as compliant with "no locks".
- **[flag, low]** §4.2: siblings `<CustomScale>`/`<DegreeMasks>`, not keys on `<Engine>`.
- **[flag, low]** §6: the parent lets disabled-stream grains ring out; CONTEXT's 10 ms enable ramp is kept (binding).
- Planner choice: the stat-arm controls are harness-side ×0.5 on the latched samples, plus one engine-side jitter hook.
