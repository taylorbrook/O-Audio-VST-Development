# Stage 2 — DSP · Phase 2.3 (Per-grain FX + CPU budget) — Plan

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.3 of 6
**GSD phase:** plan
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `e19bd02f`
**Inputs:**
- `CONTEXT-2.3.md` (D6–D8 locked, carry-ins, execution defaults; no listening checkpoint)
- `RESEARCH-2.3.md` (R13–R22, §1–§8 gate recipes). **Where it differs from ARCHITECTURE.md or CONTEXT-2.3, RESEARCH-2.3 wins.**
- `SUMMARY-2.2.md`, `VERIFICATION-2.2.md` (errata 6–8 accepted; W2o margin 0.017 dB flagged fragile)
- `research/ARCHITECTURE.md` §6, §7, §9, Algorithm §9, CPU risk, Gate Catalogue; `ROADMAP.md` Phase 2.3; `parameter-spec.md` (LOCKED); `REQUIREMENTS.md`
- The 2.2 engine in `Source/dsp/`, `PluginProcessor.*`, `tests/render-harness/`, `tests/r12/` (out-of-tree null pattern)

---

## Entry Check — contract checksums

Re-hashed this session (`shasum -a 256`). All six match STATUS.md frontmatter.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `6530b786…` | ✅ (2.2-verify revision) |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `79d69576…` | ✅ |

No drift. Also checked: since `c10d0f2c`, `git diff --stat c10d0f2c HEAD -- plugins/O-Granulator` touches only `.planning/` (STATUS, CONTEXT-2.3, RESEARCH-2.3), so `Source/` at HEAD is the verified 2.2 tree byte for byte. `git diff --quiet c10d0f2c -- modules scripts/param-dump` is clean.

---

## Goal

Add the per-grain FX chain, latched at spawn, in the ARCHITECTURE §9 order read → SVF → drive → crush → ring → window·gain·pan:
- a Cytomic TPT SVF: LP / BP (k·v1, D8) / HP, cutoff randomised ±2 oct, clamped to 0.45·fs;
- the D6 drive law, unity at a −12 dBFS reference;
- the D7 mid-tread crush with a per-grain Bernoulli;
- a quadrature ring modulator with a randomised carrier and per-sample renormalisation (R14);
- a latched `active` bit per stage and a NaN kill on SVF state.

Then take the **first CPU-budget measurement** (PERF-02).

With every FX at neutral, the engine stays **bit-identical to the verified 2.2 tree** (N0 vs `c10d0f2c`), so all 135 carried rows still pass, and the R12 2.1-equivalence null still holds. At the end of 2.3:
- DSP-05 is gated objectively: a census plus audio rows, each with a shared-latch negative control. There is no listening checkpoint (CONTEXT D1–D3).
- No bleed between slot occupants.
- QUAL-02: every sample stays finite under max-everything stress.
- The block-size null holds with every FX randomised.
- 0 allocations.
- C3 is within the 30 % worst / 10 % typical budget.
- The plugin installs and passes auval and pluginval 10 on VST3 and AU.

---

## Plan-phase decisions

- **P17 — The neutral-FX null lands first, on the unmodified tree** (CONTEXT gate 1, RESEARCH §7). T1 is a separate commit, made before any FX code.
  - `tests/n0/` is a sibling of `tests/r12/` (same machinery: `git archive`, one CMake wrapper per side, one driver compiled twice), with `OLD_SHA = c10d0f2c`.
  - Because `Source/` at HEAD *is* `c10d0f2c`, N0 passes by construction at T1, the way S0 did on the 2.1 tree. It is the net from T2 on, and every later task must keep it green.
  - There are four per-stage negative controls (N0n-F/D/C/R). Each prints IDENTICAL until its stage lands, must DIFFER from then on, and becomes strict (`--strict`) from T8.
  - R12 (vs `6508c635`) keeps running unchanged. Its cases are FX-neutral, so it must stay 0.
- **P18 — One kernel: gather → FX → scatter, for every grain** (R13; RESEARCH §1 and §8 supersede CONTEXT's "specialised loop per combination, plan may pick").
  - The gather is the 2.2 per-sample loop body. Both window branches (Hann P14 and bank) are kept. It writes `L/R/W[kCell]` locals and stops at the retire condition.
  - FX: one tight loop per active stage, with the flag test hoisted per segment.
  - Scatter: `bl[from + j] += W[j] * L[j] * g.gainL`.
  - **Statement-form rule:** the `W` expression and the scatter expression stay single statements with the exact 2.2 operand order. Apple clang contracts FMAs within a statement, so splitting one changes bits.
  - Every FX operation is per sample or latched. Only the finiteness check runs per segment.
  - **Lever, decided by measurement in T8:** if C2 (neutral FX) regresses by more than 10 % against 8.92 % on two runs, grains with `fxActive == 0` take the verbatim 2.2 loops. That path is also bitwise, and N0 proves it either way.
- **P19 — Tracer first: the SVF lands end to end before the other three stages** (T3–T4). The path runs latch → spawner draw → kernel → audio, and is proven by N0, V2 (per-grain centroid) and an SVF-form N6 block-size null. The kernel and R13 are the architectural risk, so they are proven on one stage, after one commit, before drive, crush and ring are built on them.
- **P20 — FX math lives in a new header-only `GrainFx.h`.** It holds free functions: the spawn-time latch math, the per-array stages and `svfStateFinite`. `Spawner::plan` calls the **same** latch functions the U rows test. The law gates (D6, D7, D8, R14) are unit rows on those functions, with no engine render. No CMakeLists change is needed.
- **P21 — Ring renormalisation is per sample** (R14). This supersedes CONTEXT's "once per 32-sample pass" and ARCHITECTURE §9 / Algorithm §9's "per pass", which would also break PERF-03 under R13.
  - One Newton step after every rotation: `m = 1.5f − 0.5f·(re² + im²)`.
  - The gate is ≤ −135 dB over the max grain length (2 s), at 5 Hz … 0.45·fs × 44.1 / 48 / 96 / 192 k. The worst corner is a **low** carrier frequency, not 0.45·fs as CONTEXT assumed.
- **P22 — Draw slots: the reserved `FxCutoff`, `FxCrushProb`, `FxRing`** (R20). This supersedes CONTEXT's "append FilterRand / CrushProb / RingRand"; nothing is appended.
  - `FxReso`, `FxDrive`, `FxCrush`, `Window` and `Skew` stay reserved and undrawn.
  - Draw gates: `FxCutoff` when `filterType ≠ Off && filterRand > 0`; `FxCrushProb` when `crush > 0 && 0 < crushProb < 1`; `FxRing` when `ringMix > 0 && ringRand > 0`. All are drawn in `plan()`, before `acquire()`.
  - CounterRng is keyed by slot, so **no 2.2 draw moves**. Every 2.2 row runs at neutral FX, so every deterministic 2.2 row must print the **same measured value** as the T1 baseline, including W2o at +0.483 dB.
  - If W2o moves at all, the neutral path has regressed: bisect it. Widening the seed set is allowed only for a deliberate draw-order change, and none is planned. The threshold is never loosened (VERIFICATION-2.2).
- **P23 — D6 implementation and gate** (R15, R16).
  - `d = 16·drive²`; drive is active iff `d ≥ 1e-4`, so drive 0 is the bitwise bypass.
  - `driveNorm = r / std::tanh(d·r)` with r = 0.25, computed at spawn in float through the **same** shaper function the stage uses. Per sample: `y = driveNorm · std::tanh(d·x)`.
  - The law gate is the steady-state **sample peak**. RMS (+2.24 dB at full drive) and the fundamental are INFO.
  - `std::tanh` is used. The Padé shaper is not implemented: it approximates D6's tanh, so it would need Taylor's sign-off (Risks).
- **P24 — D7 implementation.**
  - `q(x) = std::nearbyint(x·scale) / scale`, with `scale = 2^(15 − 14·crush) = 2^(b−1)`. This is RESEARCH §5's ties-to-even form; D7 writes "round". Both are mid-tread, and silence → 0 either way.
  - Hold: `inc = (1000/fs)^crush`. One phase counter per grain is shared by L/R. `Grain::crushHoldPhase` now defaults to **1.0f**, so value-initialisation gives the CONTEXT "fresh hold" (the first sample is captured). The 2.2 default of 0 would hold 0 until the first wrap.
  - There is no clip: |x| > 1 passes quantised.
- **P25 — NaN kill = skip this segment's scatter and retire.**
  - After the SVF stage, when `! svfStateFinite(g)` (`isfinite(ic1[0] + ic2[0] + ic1[1] + ic2[1])`), the grain contributes nothing for the segment and returns false. A test build also increments `diag.nanKills`.
  - This supersedes CONTEXT's "kill the grain and write 0 for the rest of the pass". It is audibly identical and keeps no state.
  - It covers fade-slot grains, which go through the same lambda.
- **P26 — CPU: C3 is INFO in the harness exit code, and the budget is a phase gate.**
  - Timing never enters the harness exit code (memory: absolute slack is runner-flaky). That follows RESEARCH's "non-blocking".
  - ROADMAP and CONTEXT make ≤ 30 % worst and ≤ 10 % typical a test criterion, so T13 and verify **fail the phase** when C3w > 30 % or C3t > 10 %. The verdict uses the median of 3 timed passes, on the absolute % of one core (conservative, because it includes harness overhead).
  - The delta over the bypass baseline and the ratio C3w / C2′ are printed.
  - If over budget, apply ARCHITECTURE's lever order before moving on (Risks).
- **P27 — K2's negative control moves from the sweep to a step** (planner refinement of RESEARCH §7).
  - A per-segment live-coefficient hook on a 200 Hz → 8 kHz sweep over 4 s moves fc by about 0.06 % per 32-sample segment. That is below κ resolution, so the control would be vacuous (memory: stimulus below threshold).
  - The subject rows keep RESEARCH's sweeps (K2a/K2b), plus a latched step row K2c. The control K2n is the step with the `fxLiveCoeffs` hook.
  - RESEARCH's sweep control is printed as INFO (K2i).
- **P28 — Test hooks for the per-grain, bleed and stress rows.** All are compiled out of the plugin (`OGRAN_TEST_HOOKS`).
  - `fxSharedLatch`: `Engine::spawn` calls `plan()` a second time on a stream copy with `filterRand = ringRand = 0` and `crushProb = 1`. That is side-effect free, because CounterRng is stateless. It writes the copy's FX latch into the grain through the same `assignFxLatch()` helper `fill()` uses, while the spawn log keeps the real draws. This is the "latch right, renderer ignores it" failure.
  - `fxNoReset`: a per-slot shadow of the FX state, recorded at retire in test builds and copied into the next occupant at fill only when the hook is set.
  - `snapshotSteals`, `injectNan` / `forceRetire`, `fxLiveCoeffs` (T8).
  - **The crush audio row is a per-grain spectral split** (out-of-band energy vs latched `crushed`), not RESEARCH §4's "≤ 2^b distinct values on an FX tap". This needs no render-path tap, and the exactness lives in U3. Its stimulus sits above the R22 floor (sine 0.5 at crush 80 %).
- **P29 — Row files.** The new rows go in `tests/render-harness/engine_gates_23.h` (U units, V per-grain, H bleed) and `engine_gates_23b.h` (N6, Z, K2, M3, C3). This mirrors P16. The 2.1 and 2.2 rows are not edited. The expected harness total is **175** (135 carried + 40 new).
- **P30 — Not built in 2.3.** The output law ignores FX (D8): no g_out change. QUAL-02 gates on **finite**, never on peak ≤ 1 (R19), and D8's "headroom checked by the QUAL-02 stress row" means an INFO peak only. **R19** (resonant LP/HP headroom vs wet safety) and **R22** (crush silence threshold) are carried into the 2.4 discuss, not implemented.

---

## Source layout (deltas against 2.2)

| File | Change |
|---|---|
| `Source/dsp/EngineConstants.h` | `kFxNyquist = 0.45`; `kFxRandOct = 4.0` (`2^(rand·4·(u − 0.5))` = ±2 oct); `kSvfKMax = 2.0`, `kSvfKSpan = 1.9`; `kDriveRef = 0.25f` (D6 r, −12 dBFS); `kDriveGain = 16.0f`; `kDriveGuard = 1e-4f` (R16); `kCrushHoldHz = 1000.0`; `kCrushScaleTop = 15.0`, `kCrushScaleSpan = 14.0`; FX bits `kFxSvf 1`, `kFxDrive 2`, `kFxCrush 4`, `kFxRing 8`. **The `Slot` enum is unchanged** (R20) |
| `Source/dsp/Grain.h` | `float driveNorm = 1.0f` (D6 latched normaliser); `crushHoldPhase` default `1.0f` (P24). Still trivially copyable (static_assert). A layout addition, not a renumbering |
| `Source/dsp/ControlGrid.h` | `ParamPointers::Stream` gains 10 FX pointers. `GridParams::Stream` gains `filterType` (int 0–3, `lround` clamp like `scale`), `filterCutoffHz` (`read` 20…20000, **raw Hz**), `filterRand`, `filterRes`, `drive`, `crush`, `crushProb`, `ringFreqHz` (`read` 20…5000, raw Hz), `ringRand`, `ringMix` (`readPct`). Defaults = spec defaults (Off, 20000, 0, 0.2, 0, 0, **1.0**, 200, 0, 0). The skew is never inverted by hand (CONTEXT carry-in) |
| `Source/dsp/GrainFx.h` **(new, header-only)** | Latch: `randOct(base, rand, u)`, `svfLatch(mode, fcHz, res, fs)`, `driveLatch(drive)`, `crushLatch(crush, fs)`, `ringLatch(fHz, mix, fs)`. Stages over `float* l, float* r, int n`: `svf`, `drive`, `crush`, `ring`. `svfStateFinite(const Grain&)`. `noexcept`, no allocation, no `juce::dsp`; `tan` / `pow` / `exp2` / `cos` / `sin` only in the latch functions |
| `Source/dsp/Spawner.h` | `GrainPlan` gains FX latch fields (`fxActive`, `svfMode`, `svfK`, `svfA1..3`, `driveD`, `driveNorm`, `crushHoldInc`, `crushScale`, `ringRotRe/Im`, `ringMix`) and telemetry (`fcHz`, `ringHz`, `crushed`). `plan()` makes the gated FX draws and latches through GrainFx. `assignFxLatch(Grain&, const GrainPlan&)` is called by `fill()` and writes **latched fields only, never state** (no-bleed comes from `activate()`'s value-init) |
| `Source/dsp/Engine.h/.cpp` | `renderSegment` becomes gather → FX → scatter (P18) with the NaN kill (P25). `SpawnRecord` gains `fxActive`, `svfMode`, `fcHz`, `ringHz`, `crushed`, `driveD`. `Diag` gains `nanKills`, `fxStateNonDefaultAtSpawn`, `stealStateChecked`, `stealStateMismatches`. `TestHooks` (P28) |
| `Source/PluginProcessor.cpp` | Cache 4 × 10 FX pointers. No new params, no state-codec change |
| `tests/n0/run.sh`, `tests/n0/n0_render.cpp` **(new)** | The N0 out-of-tree neutral-FX null vs `c10d0f2c` (T1) |
| `tests/render-harness/engine_gates_23.h` **(new)** | U1–U4, V1–V4, H1–H3 |
| `tests/render-harness/engine_gates_23b.h` **(new)** | N6, Z1–Z2, K2, M3, C3 |
| `tests/render-harness/main.cpp` | Two `#include`s and two section calls (added in T3 and T4 only) |

---

## Tasks

Build commands (from `build/`, Release, `OUARICON_BUILD_TESTS=ON`): `ninja O-Granulator-render-harness`, then `ninja OuariconGranulator_VST3 OuariconGranulator_AU`. The harness binary is `build/plugins/O-Granulator/O-Granulator-render-harness_artefacts/Release/O-Granulator-render-harness`, and exit 0 means ALL PASS.

The harness already takes 60–87 s and grows in 2.3. **Run it, `tests/n0/run.sh` and `tests/r12/run.sh` in the background** (memory: 600 s watchdog).

**The carried-row diff** is used after every task from T2 on:
- Take the sorted `[PASS]` / `[FAIL]` lines of the current run.
- Drop the new row prefixes (`U1`–`U4`, `V1`–`V4`, `H1`–`H3`, `Z1`, `Z2`, `K2`, `N6`, `M3`).
- Compare with the T1 baseline. The diff must be empty. Timing lives on `[INFO]` lines and is excluded automatically.

### Wave 0 — the precondition, on the 2.2 tree (P17)

**T1. N0 neutral-FX out-of-tree null vs `c10d0f2c`**
- Files: `tests/n0/run.sh` (new), `tests/n0/n0_render.cpp` (new). **No `Source/` change.**
- `run.sh` is a copy of the `tests/r12/run.sh` machinery with these changes:
  - `OLD_SHA=c10d0f2c`; WORK defaults to `${TMPDIR:-/tmp}/ogran-n0`; projects are `proj-{old,new}`, target `proj-*-n0`.
  - The `git diff --quiet c10d0f2c -- modules scripts/param-dump` refusal is kept, and the old tree is re-extracted on every run.
  - Revpaths use brace form, `"${OLD_SHA}"` (memory: zsh `:` modifier).
  - It takes `--strict`.
  - Exit 0 = both cases PASS and N0c differs. With `--strict`, every N0n must also DIFFER.
- `n0_render.cpp` is the R12 driver extended. The setup is 48 k, 512-sample blocks, 6 s of hash-noise input, the R12 seed, and params set by ID through the exact `setP` search.
  - **Every stream (s1–s4 enabled), FX dials non-default but neutral:** `filter_type` 0 (Off) with `filter_cutoff` 1000, `filter_res` 90, `filter_rand` 100; `drive` 0; `crush` 0 with `crush_prob` 37; `ring_mix` 0 with `ring_freq` 3000, `ring_rand` 100.
  - **2.2 features on, so both the Hann and bank paths are covered:**
    - s1: spray 10 %, jitter 20 %, window 0, spread 0.
    - s2: window 0.6, skew −40, spread 70.
    - s3: scale 1 (Chromatic), pitch_rand 3.
    - s4: reverse_prob 50, gain_rand 50.
  - **Case `free`:** each stream density 25 × size 1000 ms (ΣN 100), `grain_limit` 3 (128). Both sides must show 0 steals and 0 kills.
  - **Case `capped`:** each stream density 100 × 1000 ms (ΣN 400), `grain_limit` 0 (32). Both sides must show kills > 0.
  - **Controls (new side only):**
    - `N0c`: seed + 1 must differ.
    - `N0n-F`: s1 LP with cutoff 20000 (clamps to 0.45·fs), res 0, rand 0.
    - `N0n-D`: s1 drive 1 %.
    - `N0n-C`: s1 crush 1 %.
    - `N0n-R`: s1 ring_mix 1 %.
    - Each N0n is compared with the old side's same case and prints DIFFERS / IDENTICAL.
  - The diag line adds per-stream `planCalls[s]`, which must be > 0 on both sides (non-vacuity), and requires every param exact on both sides.
- **Baseline capture:** build and run the 2.2 harness on this tree, and save stdout to `${TMPDIR:-/tmp}/ogran-23/harness-2.2-baseline.txt`. This is not committed; it is the reference for every carried-row diff.
- Acceptance:
  - `tests/n0/run.sh` exits 0: N0 free and capped max|Δ| == 0, with the non-vacuity conditions met, and N0c differs.
  - The four N0n print **IDENTICAL**. That is expected and recorded, because no FX code exists yet.
  - Harness 135/135. `tests/r12/run.sh` PASS.
- Commit **alone, before Wave 1**: re-check `git branch --show-current` and `git status --short`, then `git commit -- plugins/O-Granulator/tests/n0`.
- Depends on: none.

### Wave 1 — plumbing (bitwise no-op)

**T2. Constants, Grain, ControlGrid, processor pointers, plan / record / diag / hook declarations**
- Files: `EngineConstants.h`, `Grain.h`, `ControlGrid.h`, `Spawner.h` (the `GrainPlan` fields only), `Engine.h` (the `SpawnRecord`, `Diag` and `TestHooks` declarations; `Diag::reset()` extended), `PluginProcessor.cpp`.
- The changes are listed in the layout table. The `TestHooks` fields are declared but stay unused until T4 and T8: `fxSharedLatch`, `fxNoReset`, `fxLiveCoeffs`, `snapshotSteals`, `injectNan {slot, atSample, field, value}`, `forceRetire {slot, atSample}`.
- Acceptance:
  - Plugin and harness build with 0 new warnings.
  - Harness 135/135, and the carried-row diff vs the T1 baseline is empty.
  - N0 PASS.
  - The `is_trivially_copyable_v<Grain>` static_assert holds.
- Depends on: T1.

### Wave 2 — tracer: the SVF end to end (P19)

**T3. `GrainFx.h`: SVF latch + stage + finite check; U1 rows**
- Files: `Source/dsp/GrainFx.h` (new), `tests/render-harness/engine_gates_23.h` (new), `main.cpp` (include + section call).
- `randOct(base, rand, u) = base · 2^(rand · 4 · (u − 0.5))` in double.
- `svfLatch`:
  - `fc = clamp(fc, 20, 0.45·fs)`, `g = tan(π·fc/fs)` in double.
  - `k = 2 − 1.9·res`, `a1 = 1/(1 + g(g + k))`, `a2 = g·a1`, `a3 = g·a2`, each cast to float.
- `svf(Grain&, l, r, n)`:
  - Per sample, per channel (RESEARCH §6): `v3 = x − ic2; v1 = a1·ic1 + a2·v3; v2 = ic2 + a2·ic1 + a3·v3; ic1 = 2v1 − ic1; ic2 = 2v2 − ic2`.
  - Output: LP `v2`, **BP `k·v1`** (D8), HP `x − k·v1 − v2`.
  - One loop per mode, with the mode switch outside the loop.
- `svfStateFinite`: `std::isfinite(ic1[0] + ic2[0] + ic1[1] + ic2[1])`. The build has no fast-math, so this is not optimised away.
- Write the U1 rows first, against a stub, and see them fail. Then implement.
  - **U1a — BP 0 dB (D8, R18):** steady-state magnitude at fc, from a least-squares sine fit over the last 0.5 s of 2 s. Expected 0.00000 dB.
    - Range: res {0, 0.1 … 1.0} × fc {20, 200, 2 k, 10 k, 0.45·fs} × fs {44.1, 48, 96, 192} k.
    - Pass: |dB| ≤ 0.1. A sample peak on noise is **not** used, because ‖h‖₁ > 1 (R18).
  - **U1b — analytic response:** LP, BP and HP at {fc/4, fc, 4·fc below 0.5·fs} × res {0, 0.5, 0.8, 0.9, 1.0}, against the prewarped bilinear oracle.
    - Oracle: with `Ω = tan(πf/fs)/tan(πfc/fs)` and `D = 1 − Ω² + jkΩ`, LP = 1/D, BP = jkΩ/D, HP = −Ω²/D.
    - Pass: ±0.05 dB wherever |H| > −40 dB.
  - **U1i (INFO) — R18 peak table:** LP/HP peaks at res 50 / 80 / 90 / 100 % (expected +0.98 / +6.63 / +10.84 / +20.01 dB).
  - **U1c — stability:** k = 0.1 at 0.45·fs, all three modes, 10 s of full-scale white noise at 48 k and 192 k. Pass: every output sample finite, |y| ≤ 100, end state finite.
  - **U1d — detector covers the class:** `svfStateFinite` is false for NaN, +Inf and −Inf in each of the four integrators (12 cases), and true for finite states with |ic| ≤ 1e6.
- Acceptance: U1 PASS; the 135 carried rows unchanged (diff empty); N0 PASS. The engine is not yet touched.
- Depends on: T2.

**T4. Tracer wiring: spawner latch, kernel, NaN kill, spawn record, `fxSharedLatch`; V2 and N6 (SVF form)**
- Files: `Spawner.h`, `Engine.h`, `Engine.cpp`, `tests/render-harness/engine_gates_23.h` (V2), `tests/render-harness/engine_gates_23b.h` (new: N6), `main.cpp` (include + section call).
- **`plan()`:**
  - If `filterType ≠ Off`: `u = uni(Slot::FxCutoff)` only when `filterRand > 0` (P22); fc via `randOct` then `svfLatch`.
  - Set `fxActive |= kFxSvf` and `GrainPlan.fcHz` (post-clamp).
  - Drawn before `acquire()`, like every other draw.
- **`assignFxLatch()`**, called from `fill()`: writes `fxActive`, `svfMode`, `svfK`, `svfA1..3`. T5–T7 extend it with their stage fields.
- **`renderSegment`**, as in P18:
  - Gather into `float L[kCell], R[kCell], W[kCell]`, keeping each window branch's 2.2 statements verbatim, giving `n′` samples and a retire flag.
  - `if (g.fxActive & kFxSvf)`: run `svf(g, L, R, n′)`, then the P25 kill check.
  - Scatter.
- **`Engine::spawn` (test builds):**
  - Record the FX fields in the `SpawnRecord`.
  - The `fxSharedLatch` hook (P28).
  - The H3 counter: after `fill`, any FX state field ≠ `Grain {}` default → `++diag.fxStateNonDefaultAtSpawn`.
- **V2 — BP centroid (R17):**
  - Settings: s1 only, white noise, BP res 80 %, cutoff 2 kHz, filter_rand 100 %. Isolated grains: size 100 ms, density 5, jitter 0, spray 10 %, spread 0. At least 40 grains (≥ 9 s).
  - Measure each grain's log₂ spectral centroid (zero-padded FFT, power-weighted over 50 Hz … 0.45·fs, segmented by the spawn log's onset + len).
  - Pass: Spearman ρ(log₂ fcHz, centroid) ≥ 0.9 (RESEARCH: 0.998).
  - **V2n:** the same render with `fxSharedLatch` must give |ρ| ≤ 0.5 (RESEARCH: +0.17).
- **N6 (SVF form):** the N5 recipe with SVF on every stream.
  - Settings: LP / BP / HP / LP, cutoff 1.5 kHz, res 60 %, filter_rand 50 %. One type change at a shared boundary (s1 LP → HP).
  - Pass: blocks 441 / 512 / 4096 / variable vs 32, max|Δ| == 0. **N6c**: a different seed differs.
- **Tracer acceptance (end to end):**
  - N0 PASS on both cases, and **N0n-F DIFFERS**.
  - V2 and V2n PASS; N6 and N6c PASS; U1 PASS.
  - The carried-row diff is empty (S0, W2o and R12 included).
  - 0 new warnings.
- Commit the tracer (path-scoped `-- plugins/O-Granulator`, after the branch / status re-check).
- Depends on: T3.

### Wave 3 — the remaining stages (sequential: they share `GrainFx.h`, `Spawner.h`, `Engine.cpp`)

**T5. Drive (D6, R15, R16) + U2**
- Files: `GrainFx.h`, `Spawner.h`, `Engine.cpp`, `engine_gates_23.h`.
- Latch per P23: `d = 16·drive²`, active iff `d ≥ kDriveGuard`, `driveNorm` from the shared `driveShape` function. `assignFxLatch` gains `driveD` and `driveNorm`. The record gains `driveD`.
- Stage: `y = driveNorm · std::tanh(d·x)`, per sample, per channel.
- Kernel: `if (fx & kFxDrive) drive(g, L, R, n′)`, after the SVF.
- **U2a:** `y(±r) == ±r` within 1 ulp, at drive 1 % … 100 % in 1 % steps.
- **U2b — peak law:**
  - Stimulus: a −12 dBFS sine at **f = fs/100** (480 Hz at 48 k), so a sample lands on every crest. 1 s, steady state.
  - Pass: output sample peak within ±0.1 dB of 0.25 at drive {0, 0.25 %, 0.5 %, 1 % … 100 %}. Expected +0.0000 dB.
- **U2c — guard continuity:**
  - At `d = 1e-4` (the first active value), max |y − x|/|x| ≤ 2e-7 over |x| ∈ [1e-6, 4] (R16: 1.59e-7).
  - Just below the guard, the stage is inactive (bit clear) and the output is bitwise x.
- **U2i (INFO):** RMS and fundamental Δ at drive {0.1, 0.25, 0.5, 0.75, 1}. Expected RMS +0.001 / +0.044 / +0.573 / +1.578 / +2.237 dB.
- Acceptance: U2 PASS; N0 PASS; **N0n-D DIFFERS**; N6 (SVF form) PASS; carried-row diff empty.
- Depends on: T4.

**T6. Crush (D7, P24) + U3**
- Files: `GrainFx.h`, `Spawner.h`, `Engine.cpp`, `engine_gates_23.h`.
- Latch, only when `crush > 0`:
  - `crushed = crushProb ≥ 1 ? true : crushProb ≤ 0 ? false : uni(Slot::FxCrushProb) < crushProb`.
  - If crushed: `inc = (float) pow(1000/fs, crush)` (double pow), `scale = (float) exp2(15 − 14·crush)`, `fxActive |= kFxCrush`.
  - Record `crushed`. `assignFxLatch` gains `crushHoldInc` and `crushScale`.
- Stage, per sample: `if (phase ≥ 1) { phase −= 1; held[ch] = nearbyint(x[ch]·scale)/scale for both ch; } out[ch] = held[ch]; phase += inc;`. One phase per grain.
- Kernel: after drive.
- **U3a — silence (D7):** x ∈ {+0, −0, ±1e-30, ±0.49/scale}, under `ScopedNoDenormals`, at crush 1 % … 100 % in 1 % steps (b 16 → 2). Pass: output == 0.
- **U3b — mid-tread lattice:** on a fine grid of x over [−2, 2], `q(x)·scale` is an integer, |q(x) − x| ≤ 0.5/scale (+1 ulp), and `q(q(x)) == q(x)`.
- **U3n — negative control:** the ARCHITECTURE mid-rise reference, kept harness-local, gives |q(0)| = 0.5/scale ≠ 0 at every depth. This proves U3a discriminates.
- **U3c — hold law:** on a ramp input, every run of equal output has length floor or ceil(1/inc). The first output == q(x[0]) (fresh hold). At crush 100 % and 48 k, the runs are 48 samples (1 kHz).
- **U3i (INFO) — R22 table:** the grain is silent when its peak is below −12.0 / −18 / −20.5 / −33 dBFS at crush 100 / 93 / 90 / 75 %.
- Acceptance: U3 PASS; N0 PASS; **N0n-C DIFFERS**; N6 PASS; carried-row diff empty.
- Depends on: T5.

**T7. Ring (R14, P21) + U4**
- Files: `GrainFx.h`, `Spawner.h`, `Engine.cpp`, `engine_gates_23.h`.
- Latch, only when `ringMix > 0`:
  - `f = randOct(ringFreqHz, ringRand, uni(Slot::FxRing))`, drawn only when `ringRand > 0`; `f ≤ 0.45·fs`.
  - `ω = 2πf/fs`; `rot = ((float) cos ω, (float) sin ω)`, computed in double.
  - Latch `ringMix`, set `fxActive |= kFxRing`, record `ringHz`.
  - The carrier starts at the `Grain` default `(1, 0)`, with no draw (CONTEXT).
- Stage, per sample:
  - `c = re`; `y[ch] = x[ch]·((1 − mix) + mix·c)` for L and R, which share the carrier.
  - Then rotate `(re, im) ← (re·rr − im·ri, re·ri + im·rr)` and apply the Newton step `m = 1.5f − 0.5f·(re² + im²); re *= m; im *= m`.
  - **Never per segment.**
- Kernel: last stage, before the scatter.
- **U4a — drift:** max ||c| − 1| ≤ **−135 dB** over 2 s (the max grain length), at f {5, 20, 200, 5 k, 0.45·fs} × fs {44.1, 48, 96, 192} k. RESEARCH measured ≤ −135.5 dB, at 48 k only.
- **U4b — frequency:** the realised carrier frequency (unwrapped phase slope over 1 s) is within 0.01 Hz of f at every (f, fs). A frequency criterion is used because a fixed phase bound would scale with fs.
- **U4c — mix law:** the first output == x·((1 − mix) + mix) exactly; L and R see the same carrier; mix 100 % gives x·Re c.
- **U4n — negative control:** the same rotator, renormalised once per 32 samples (harness-local variant), reads **> −135 dB** at 5 Hz / 48 k (RESEARCH: −114.9 dB).
- Acceptance: U4 PASS; N0 PASS; **N0n-R DIFFERS**; N6 PASS; carried-row diff empty.
- Depends on: T6.

### Wave 4 — render-side hooks + integration

**T8. Remaining test hooks; integration build; regression; kernel lever**
- Files: `Engine.h`, `Engine.cpp` (test-hook code only, all under `OGRAN_TEST_HOOKS`).
- Hooks:
  - **`fxNoReset`:** the retire shadow is always recorded in test builds. It stores `svfIc1/2`, `crushHeld`, `crushHoldPhase` and `ringRe/Im` per slot. With the hook set, `spawn` copies it into the new occupant after `fill`.
  - **`snapshotSteals`:**
    - Before `acquire`, copy the 128 live grains.
    - When `acq.stolen`, find the fade slot that holds the victim (`stream`, `age`, `readAbs` equal bitwise to the snapshot of `acq.slot`). Compare its FX latch and state fields bitwise.
    - Count `diag.stealStateChecked` and `diag.stealStateMismatches`.
  - **`injectNan {slot, atSample, field, value}`:** in the first segment whose absolute start is ≥ `atSample`, write `value` into the named SVF integrator of that slot, provided it is active and SVF-active. This happens after `applyPendingKill`, before the gather.
  - **`forceRetire {slot, atSample}`:** the same trigger. It forces the P25 finiteness verdict to false for that grain and segment, so the code path is identical to a real NaN kill.
  - **`fxLiveCoeffs`:** at each segment, the SVF coefficients of every SVF-active grain are re-derived from `gp.s[g.stream]`'s live cutoff and res through `svfLatch`. This test-only `tan` runs per segment and steps the coefficients mid-grain.
- Build the plugin and harness with 0 new warnings.
- Regression:
  - Carried-row diff empty (incl. S0, W2o +0.483 dB, W1, N5, M2); `tests/r12/run.sh` PASS.
  - **`tests/n0/run.sh --strict` PASS**, with all four N0n DIFFERING. From here on, N0 always runs strict.
- **Kernel lever (P18):** run C2 twice. If both runs are > 9.8 % (1.10 × 8.92 %), route grains with `fxActive == 0` through the verbatim 2.2 loops, then re-run N0 `--strict` and the carried-row diff. Record C2′ and the decision either way.
- Depends on: T7.

### Wave 5 — gates

All rows print PASS / FAIL and count toward the exit code, except the INFO rows. They are seeded, with position-deterministic stimuli. T9 → T10 share `engine_gates_23.h`, and T11 → T12 share `engine_gates_23b.h`. Each pair runs in sequence. The two pairs are file-disjoint, and neither touches `Source/` or `main.cpp`.

**T9. DSP-05 per-grain rows: V1, V3, V4** (`engine_gates_23.h`)
- **V1a — cutoff census:**
  - Settings: s1 LP, cutoff 1 kHz, filter_rand 100 %, density 100 × size 50 ms, at least 200 grains.
  - Pass: every `log₂(fcHz/1000)` ∈ [−2, 2], with max − min ≥ 3.6.
  - At rand 0, every `fcHz == 1000.0f` bitwise.
  - Clamp sub-row: cutoff 20 kHz, rand 100 % at 44.1 k. Every draw at or above 0.45·fs records exactly `(float) (0.45·44100)`, at least one does, and none exceeds it (CONTEXT: no rescale).
- **V1b — ring census:** the same structure. Base 200 Hz, ring mix 100 %, ring_rand 100 %: span ≥ 3.6 within [−2, 2], and rand 0 is bitwise 200. Clamp: base 5 kHz at 44.1 k, with draws sitting exactly at 19 845 Hz.
- **V1c — crush Bernoulli:** at crush 50 %:
  - prob 0 → 0 crushed, and the bit is never set;
  - prob 100 % → all crushed;
  - prob 50 % over ≥ 2000 spawns (4 streams × 100 g/s × 5 s) → |z| ≤ 3.29 (binomial 99.9 %).
- **V1d — `active` gates:** the record's `fxActive` bits equal the expected bits on every one of these configs:
  - filter Off with rand 100 % → no SVF bit; LP at the 20 kHz default → SVF bit (still active at the clamp, CONTEXT);
  - drive 0 / 0.2 % (d = 6.4e-5) → no drive bit; drive 0.3 % (d = 1.44e-4) → drive bit;
  - crush 0 with prob 37 % → no crush bit; crush 1 % with prob 0 → no crush bit;
  - ring mix 0 with rand 100 % → no ring bit; ring mix 1 % → ring bit.
  - Wherever a bit is clear, that stage's latched grain fields equal the `Grain {}` defaults.
- **V3 — ring carrier (R17):**
  - Settings: sine 1 kHz at 0.5, ring mix 100 %, ring 200 Hz, ring_rand 100 %, with the V2 isolation settings; at least 40 grains.
  - Measure f̂ = (upper-sideband FFT peak, zero-padded, parabolic) − 1000.
  - Pass: ≥ 95 % of grains within 2 Hz of the record's `ringHz` (RESEARCH: 40 / 40).
  - **V3n:** with `fxSharedLatch`, fewer than 20 % within 2 Hz.
- **V4 — crush split (P28):**
  - Settings: sine 440 Hz at **0.5**, crush **80 %** (½ LSB = −29 dBFS; at crush 100 % a −12 dBFS grain would be silenced, R22), prob 50 %, V2 isolation, at least 40 grains.
  - Per grain: the fraction of energy outside 440 ± 50 Hz, classified at −30 dB.
  - Pass: ≥ 99 % agreement with the record's `crushed`, and at least 10 grains in each class (non-vacuity).
  - **V4n:** with `fxSharedLatch` (every grain crushed in the render), agreement ≤ 60 %.
- Depends on: T8.

**T10. DSP-05 no bleed: H1, H2, H3** (`engine_gates_23.h`)
- **H1 — slot reuse after a resonant grain:**
  - Settings: s1 only, with all four stages active: BP res 100 % at 1 kHz (rand 0), drive 30 %, crush 30 % (prob 100 %), ring mix 50 % at 200 Hz. Density 2, size 200 ms, jitter 0, spray 0, position 0, mix 100 %.
  - Input: a 1 kHz sine at 0.5 for [0, 1.8) s, then exact 0.
  - Grain A = the last onset whose read lies fully in the sine. Grain B = the first onset at or after 1.8 s + margin.
  - Pass:
    - `rec(B).slot == rec(A).slot`;
    - A's retire-shadow |svfIc| > 1e-3 (non-vacuity: A really rang);
    - **every output sample in B's lifetime == 0.0f**.
  - **H1n:** with `fxNoReset`, at least one sample in B has |y| > 1e-6.
- **H2 — steal keeps FX state:**
  - Settings: s1, density 200 × size 1000 ms (N 200) at `grain_limit` 128, so native steals occur. All four FX on. `snapshotSteals` set.
  - Pass: `stealStateChecked ≥ 50` and `stealStateMismatches == 0`.
  - Non-vacuity: on the next segment, the fade-slot copy's `svfIc` moves on, so the stolen grain keeps running through its FX.
- **H3 — fresh state at every spawn:** two renders with all FX on: 4 streams at cap 32 (kills), and N > 128 at cap 128 (steals).
  - Pass: `fxStateNonDefaultAtSpawn == 0` on both, with at least 1000 spawns, kills > 0 and steals > 0 respectively.
  - This covers ring state, which is invisible on silent input in H1.
- Depends on: T9.

**T11. QUAL-02: Z1, Z2** (`engine_gates_23b.h`)
- **Z1a — max everything:**
  - Settings: 4 streams, density 200 × size 2000 ms, cap 128 (kills and steals); res 100 %, cutoff 2 kHz with filter_rand 100 %; drive 100 %; crush 100 % (prob 100 %); ring mix 100 % at 5 kHz with ring_rand 100 % (draws pile at the 0.45·fs clamp); pitch_rand 12 st.
  - Sections: 4 s of full-scale noise, 4 s of a 0 dBFS sine at fc, then 2 s of silence (decay through FTZ).
  - Runs: one per mode (all streams LP, BP, HP) at 48 k, plus LP at 44.1 k and 192 k.
  - Pass:
    - every output sample `isfinite`;
    - at the end of each section, no float field of any of the 144 slots is `FP_SUBNORMAL` (pool scan);
    - `nanKills == 0`, because finite input never trips the kill.
- **Z1b — the hottest numeric path:** as Z1a with drive 0 and crush 0. Settings: LP res 100 %, ring 100 %, sine at fc, spray 0, every stream at level +6 dB (the R19 coherent worst case).
  - Pass: the same conditions as Z1a.
  - The wet peak is printed as INFO (R19 expects up to about +38 dBFS). There is **no ≤ 1 assertion** (P30).
- **Z2 — NaN kill (P25):** five runs:
  - NaN into `ic1[0]`, `ic1[1]`, `ic2[0]` and `ic2[1]` of a live SVF grain;
  - +Inf into `ic1[0]` of a fade-slot grain right after a steal.
  - Each run is paired with a `forceRetire` reference on the same slot and segment.
  - Pass:
    - every output sample finite;
    - `nanKills == 1`;
    - the slot is inactive after that segment;
    - **max|Δ| == 0 between the injected run and its reference over the whole render** (no side effect beyond retiring that grain).
- Depends on: T8.

**T12. QUAL-01 / PERF rows: K2, N6 (all FX), M3, C3** (`engine_gates_23b.h`)
- **K2a — cutoff sweep:**
  - Settings: the K1 source and cloud (sine 440, spray 0, linear wet sum), s1 only, density 20 × size 200 ms, LP res 50 %.
  - Sweep: cutoff exponential 200 Hz → 8 kHz over [3, 7) s. The start value is set before `prepare`, so the render head holds no jump (memory: sweep start hides a step).
  - κ = maxCurv / RMS over [3, 7) s. κ_ref = the max over static renders at {200, 632, 2000, 8000} Hz.
  - Pass: κ / κ_ref ≤ 2.0.
- **K2b — ring sweep:** ring mix 100 %, ring 50 → 2000 Hz over [3, 7) s, with static references at {50, 316, 2000} Hz. Pass ≤ 2.0.
- **K2c — cutoff step:** 200 Hz → 8 kHz at one grid point at 5.0 s, latched. κ over [4.5, 5.5) s vs static {200, 8000}. Pass ≤ 2.0.
- **K2n — negative control (P27):** K2c with `fxLiveCoeffs` must read **> 2.0**. Non-vacuity: at least one SVF-active grain is alive at the step sample.
- **K2i (INFO):** K2a with `fxLiveCoeffs` (RESEARCH's sweep control); the ratio is printed.
- **N6 widened to all FX:**
  - Settings: the N5 everything-on recipe (spray, size_rand, jitter, scale + mask + Custom, reverse 50 %, window 0.6, skew −0.4, spread 70 %, gain_rand 50 %, cap 32, mute / solo / enable toggles, custom swap). Plus, on every stream:
    - SVF: LP / BP / HP / LP, cutoff 1.5 kHz, res 60 %, filter_rand 50 %;
    - drive 50 %;
    - crush 40 % with prob 50 %;
    - ring 300 Hz, ring_rand 50 %, mix 50 %.
  - At shared boundaries: s1 LP → HP, s4 Off → BP, a ring_freq change.
  - Pass: 441 / 512 / 4096 / variable vs 32, max|Δ| == 0. N6c: a different seed differs.
  - Non-vacuity: every `fxActive` bit appears in the spawn log; crushed and uncrushed grains are both present; kills > 0.
  - This replaces the T4 SVF form.
- **M3 — allocations:** the M2 coverage plus:
  - filter type sweeps Off ↔ LP ↔ BP ↔ HP mid-render;
  - cutoff / res / rand sweeps;
  - drive across the guard;
  - crush / prob sweeps;
  - ring freq / rand / mix sweeps;
  - spawn and steal bursts, fade slots with FX, and one `injectNan` kill.
  - Pass: 0 allocations, with liveness 1 as a separate row.
- **C3 (INFO in the exit code; the verdict is at T13, P26):** the same harness run, Release build. Each config gets a 4.5 s warm-up, then 3 timed 10 s passes, and the median is reported.
  - **C3b:** bypass, all streams off.
  - **C2′:** the C2 config re-run.
  - **C3w (worst):** C2 config + LP res 100 % with filter_rand 100 %, drive 100 %, crush 100 % (prob 100 %), ring mix 100 % with ring_rand 100 %.
  - **C3t (typical):** 4 streams × density 8 × size 2000 ms (N 16 each), LP res 20 % + drive 30 %, window 0, scale Off.
  - Print: % of one core, Δ over C3b, and C3w / C2′. RESEARCH projects C3w at about 13–15 %.
- Depends on: T11.

### Wave 6 — validate + docs

**T13. Build, install, validate, budget verdict**
- Harness ALL PASS (expected **175**: 135 carried + 40 new; record the actual count) with 0 new warnings. Carried-row diff empty.
- `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep and AU cache clear). Installs as `O-Granulator-dev.{vst3,component}`.
- `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10 on VST3 and on AU (in-process). pluginval's randomised parameters exercise the FX at extreme settings (memory: pluginval-10 catches latent NaN).
- `tests/n0/run.sh --strict` PASS. `tests/r12/run.sh` PASS.
- Static checks. Grep only non-comment lines (`grep -v '^\s*//'`), and confirm the baseline at HEAD first:
  - `StateVariableTPTFilter` or `IIR::` in `Source/` → 0 (baseline 0);
  - `std::vector`, `new `, `malloc` or `make_unique` in `GrainFx.h` → 0;
  - `std::(tan|pow|exp2|cos|sin)\b` in `GrainFx.h` only inside the latch functions;
  - `git diff --stat c10d0f2c -- modules/` empty (grain-engine 1.1.0 and pitch-collections 1.0.0 untouched, D1);
  - the Grain static_assert holds.
- **Budget verdict (P26):** C3w ≤ 30 % and C3t ≤ 10 %. If either is over, stop and apply the levers in order (Risks) before T14.
- Depends on: T10, T12.

**T14. Docs + state**
- Write SUMMARY-2.3.md:
  - the gate table, with every row and its measured value;
  - N0 / R12 results and the final harness count;
  - C2′, C3b / C3w / C3t, and the P18 lever decision;
  - **the errata list below**;
  - the 2.4 carry list;
  - any execution deviation, with its reason.
- STATUS.md → `stage_2_phase_2_3_execute_complete` (next: verify). REQUIREMENTS stays untouched until verify.
- Commit path-scoped: `git commit -- plugins/O-Granulator`, after re-checking `git branch --show-current` and `git status --short`. Other sessions' staged files must not join this commit (shared checkout).
- Depends on: T13.

---

## Dependency graph

```
T1 ─ T2 ─ T3 ─ T4 ─ T5 ─ T6 ─ T7 ─ T8 ─┬─ T9 ── T10 ─┬─ T13 ─ T14
 (N0 first)   └tracer┘  └─ stages ─┘   └─ T11 ── T12 ─┘
```

Each stage task (T4–T7) flips its own N0n control to DIFFERS. T8 makes N0 strict. N0, the carried-row diff and N6 stay green from T2 to T13.

---

## Gate criteria (new rows)

There are 49 gate rows: 40 in the harness exit code, 7 in the N0 script and 2 at the phase gate. There are also 7 INFO rows: U1i, U2i, U3i, Z1b peak, K2i, C3b and C2′.

| Row | Measures | Pass | Negative control | Req | Task |
|---|---|---|---|---|---|
| N0 free / capped | out-of-tree null vs `c10d0f2c`, 4 streams, neutral non-default FX dials, 2.2 features on | max\|Δ\| == 0; free 0 steals/kills, capped kills > 0; params exact; planCalls > 0 per stream | N0c | DSP-05, COMPAT-01 | T1 |
| N0c | new side, seed + 1 | differs | — | — | T1 |
| N0n-F / D / C / R | one stage minimally on (LP 20 kHz / drive 1 % / crush 1 % / ring 1 %) | each differs (strict from T8) | — | DSP-05 | T1, T4–T8 |
| U1a | BP k·v1 at fc, res × fc × fs | \|dB\| ≤ 0.1 | — | DSP-05 (D8) | T3 |
| U1b | LP/BP/HP vs bilinear oracle | ±0.05 dB where \|H\| > −40 dB | — | DSP-05 | T3 |
| U1c | k 0.1 at 0.45·fs, noise, 48 / 192 k | finite, \|y\| ≤ 100 | — | QUAL-02 | T3 |
| U1d | `svfStateFinite` over 4 fields × NaN/±Inf | 12 false, finite true | — | QUAL-02 | T3 |
| U2a | y(±r) | == ±r ≤ 1 ulp | — | DSP-05 (D6) | T5 |
| U2b | −12 dBFS sine sample peak, drive 0→100 % | ±0.1 dB | — | DSP-05 (D6) | T5 |
| U2c | guard d = 1e-4; below the guard | ≤ 2e-7 rel; bitwise x | — | DSP-05 (D6) | T5 |
| U3a | silence at every depth | == 0 | U3n | DSP-05 (D7) | T6 |
| U3b | mid-tread lattice, error, idempotence | exact | — | DSP-05 (D7) | T6 |
| U3c | hold run lengths, fresh hold | floor/ceil(1/inc); first == q(x₀) | — | DSP-05 | T6 |
| U3n | ARCH mid-rise reference | \|q(0)\| = ½ LSB ≠ 0 | (is the control) | — | T6 |
| U4a | rotator drift, 2 s, 5 f × 4 fs | ≤ −135 dB | U4n | DSP-05 (R14) | T7 |
| U4b | realised carrier frequency | ≤ 0.01 Hz | — | DSP-05 | T7 |
| U4c | mix law, first sample, shared L/R carrier | exact | — | DSP-05 | T7 |
| U4n | per-32 renorm variant at 5 Hz | > −135 dB | (is the control) | — | T7 |
| V1a / V1b | cutoff / ring census | span ≥ 3.6 oct within ±2; rand 0 bitwise; clamp exact | — | DSP-05 | T9 |
| V1c | crush Bernoulli | 0 / all exact; 50 % \|z\| ≤ 3.29 | — | DSP-05 | T9 |
| V1d | `fxActive` gate table | bits == expected; clear → default fields | — | DSP-05 | T9 |
| V2 | BP per-grain centroid vs latched fc | ρ ≥ 0.9 | V2n \|ρ\| ≤ 0.5 | DSP-05 | T4 |
| V3 | per-grain ring carrier | ≥ 95 % within 2 Hz | V3n < 20 % | DSP-05 | T9 |
| V4 | per-grain crush split | ≥ 99 % agreement, ≥ 10 per class | V4n ≤ 60 % | DSP-05 | T9 |
| H1 | slot reuse after resonant grain | B exactly 0; same slot; A rang | H1n non-zero | DSP-05 | T10 |
| H2 | stolen grain's fade copy | ≥ 50 checked, 0 mismatches | — | DSP-05 | T10 |
| H3 | FX state at every spawn | 0 non-default (kills and steals renders) | — | DSP-05 | T10 |
| Z1a / Z1b | max stress / hottest path | all finite; 0 subnormal; nanKills 0 | — | QUAL-02 | T11 |
| Z2 | NaN/Inf injection × 5 | finite; 1 kill; slot retired; ≡ forceRetire ref | forceRetire ref | QUAL-02 | T11 |
| K2a / K2b / K2c | cutoff sweep / ring sweep / cutoff step | κ ratio ≤ 2.0 vs static | K2n > 2.0 | QUAL-01 | T12 |
| N6 / N6c | block-size null, all FX randomised | 0 diffs; seed control differs | N6c | PERF-03 | T4, T12 |
| M3 | allocations with FX | 0; liveness 1 | liveness | PERF-01 | T12 |
| C3w / C3t | CPU worst / typical, 48 k / 128 | ≤ 30 % / ≤ 10 % (phase gate) | C3b baseline | PERF-02 | T12, T13 |

---

## Success criteria (gate for verify)

| # | Criterion | Req | Task |
|---|---|---|---|
| 1 | N0 neutral-FX null vs `c10d0f2c` (free + capped) max\|Δ\| 0; seed control differs; all four per-stage controls differ (strict) | DSP-05, COMPAT-01 | T1, T8 |
| 2 | All 135 carried rows PASS, with every deterministic value identical to the T1 baseline (S0, W2o +0.483 dB); R12 vs `6508c635` PASS | COMPAT-01, PERF-02 | T8, T13 |
| 3 | SVF: BP k·v1 0 dB ±0.1 across res × fc × fs; LP/BP/HP match the bilinear oracle; stable at k 0.1 at 0.45·fs | DSP-05 (D8) | T3 |
| 4 | Drive: sample peak unity ±0.1 dB, 0→100 %; y(r) == r; continuous at the guard; bitwise bypass at 0 | DSP-05 (D6) | T5 |
| 5 | Crush: silence exactly 0 at every depth (mid-rise control fails); lattice; hold law | DSP-05 (D7) | T6 |
| 6 | Ring: drift ≤ −135 dB at every f × fs (per-pass control fails); carrier ≤ 0.01 Hz | DSP-05 (R14) | T7 |
| 7 | Per-grain: census spans, clamp, Bernoulli and gates; BP centroid ρ ≥ 0.9, ring carrier ≥ 95 %, crush split ≥ 99 %, each with its shared-latch control failing | DSP-05 | T4, T9 |
| 8 | No bleed: slot reuse exactly 0 (no-reset control non-zero); steal copy bitwise; fresh state at every spawn | DSP-05 | T10 |
| 9 | Max-everything stress finite, no subnormal, no false kills; NaN kill exact on every SVF field and in a fade slot | QUAL-02 | T11 |
| 10 | Cutoff / ring sweeps and a cutoff step ≤ 2.0× static; the live-coefficient control > 2.0 | QUAL-01 (and QUAL-03 cutoff) | T12 |
| 11 | Block-size null with every FX on and randomised, type changes at shared boundaries; seed control differs | PERF-03 | T4, T12 |
| 12 | 0 allocations with FX sweeps, type switches, spawns / steals and a NaN kill; liveness 1 | PERF-01 | T12 |
| 13 | CPU (first measurement): C3w ≤ 30 %, C3t ≤ 10 % of one core at 48 k/128, median of 3; Δ vs bypass and ratio vs C2′ recorded | PERF-02 | T12, T13 |
| 14 | 0 new warnings; install; auval; pluginval 10 VST3 + AU; static checks clean | COMPAT-01 | T13 |

Requirement status expected at verify:
- DSP-05 complete.
- QUAL-02 partial: max resonance and max drive are covered; max feedback is 2.4 / 2.6.
- QUAL-01 partial: FX sweeps are covered; freeze is 2.4.
- PERF-01 partial: freeze 2.4, matrix 2.5.
- PERF-02 partial: the full worst case with feedback and reverb is re-measured in 2.6 (CONTEXT).
- PERF-03 partial: the feedback and modulation re-gates are still to come.

---

## Errata for SUMMARY-2.3 (record at T14)

1. **D6 drive law** vs ARCHITECTURE §9 Drive `tanh(dx)/d`, the Parameter Mapping `d = 1 + 15·drive²`, and ROADMAP 2.3 Components "drive tanh(dx)/d".
   - Now `y = r·tanh(d·x)/tanh(d·r)`, with r = 0.25, d = 16·drive², inactive below d = 1e-4 (R16).
   - It is peak-unity, not RMS-unity: RMS rises +2.24 dB at full drive (R15).
   - ROADMAP is not edited (checksum).
2. **D7 crush quantiser** vs ARCHITECTURE Algorithm §9 mid-rise and ROADMAP "mid-rise bits". Now mid-tread `nearbyint(x·2^(b−1))/2^(b−1)`, ties-to-even (P24); silence stays 0.
3. **D8 SVF BP output** vs ARCHITECTURE Algorithm §9 "BP = v1". Now BP = k·v1 with an exact 0 dB peak (R18). LP/HP keep their raw peak (+20 dB at res 100 %). The output law ignores FX.
4. **R14 ring renormalisation is per sample** (Newton). This supersedes ARCHITECTURE §9 / Algorithm §9 "per pass" and CONTEXT-2.3's "once per 32-sample pass". The worst corner is a low carrier frequency, not 0.45·fs.
5. **R20 draw slots:** the reserved `FxCutoff` / `FxCrushProb` / `FxRing` are used, against CONTEXT-2.3's "append FilterRand / CrushProb / RingRand". Nothing is appended.
6. **R15 D6 gate metric:** CONTEXT's unspecified "within ±0.1 dB" is the steady-state sample peak.
7. **P25 NaN-kill semantics:** the segment's scatter is skipped and the grain retires, against CONTEXT's "kill and write 0 for the rest of the pass". Audibly identical.
8. **P18 kernel:** per-stage ifs hoisted per segment, against CONTEXT's "specialised loop per combination, plan may pick" (RESEARCH §8). Record C2′ and whether the lever was applied.
9. **P26 CPU gate mechanics:** C3 is INFO in the harness exit code and the budget is enforced at the phase gate. This reconciles RESEARCH's "non-blocking" with ROADMAP / CONTEXT's ≤ 30 % / ≤ 10 % criterion.
10. **P27 K2 negative control on a step,** against RESEARCH §7's sweep control (vacuous: ~0.06 % fc per segment). K2i records the sweep control's ratio.
11. **P28 crush per-grain audio row = spectral split,** against RESEARCH §4's "≤ 2^b distinct values on an FX tap". The stimulus is at crush 80 % with a sine at 0.5, above the R22 floor.
12. **D8 / R19 QUAL-02 scope:** CONTEXT D8 says "headroom is checked by the QUAL-02 stress row". QUAL-02 gates on finite only, and the peak is INFO.
13. **`Grain::crushHoldPhase` default 0 → 1** (P24): a layout default change, not a renumbering.
14. **U4b** uses a frequency criterion (≤ 0.01 Hz) instead of RESEARCH's "phase error ≤ 7e-4 rad after 1 s", which was measured at 48 k only and would scale with fs.
15. Any execution deviation, with the measured reason, and the final harness row count.

## Carried to the 2.4 discuss (not implemented in 2.3)

- **R19 — resonant LP/HP headroom vs wet safety.**
  - At res 100 % on a tone at fc, the stream wet reaches about +20 dB. With 4 coherent streams at +6 dB it reaches about +38 dBFS before any safety. Z1b's INFO peak gives the measured value.
  - Option (a): accept ARCHITECTURE §14's `1.9953·tanh(x/1.9953)` as the safety it is.
  - Option (b): a per-stream make-down `1/peak(k)` latched at spawn, `peak = 1/(k·√(1 − k²/4))` for k < √2. This would amend D8.
- **R22 — crush silence threshold.** Mid-tread silences any grain whose peak is below ½ LSB: −12 dBFS at crush 100 %, −20.5 at 90 %, −33 at 75 %. A typical −18 dBFS grain vanishes above about 93 %. Hear it at the 2.4 listening checkpoint. No change to D7.
- Also carried: the W2o rule (widen seeds, never the threshold), the full PERF-02 worst case with feedback and reverb (2.6), QUAL-02 max feedback (2.4 / 2.6), `getTailLengthSeconds` (2.6).

---

## Risks / watch items

- **FP contraction breaks N0.** Splitting the `W` or scatter statements, or folding the window into a different expression, changes FMA fusion and so changes bits. If N0 fails:
  - bisect to the statement;
  - restore the 2.2 statement text;
  - if needed, take the P18 fallback (verbatim 2.2 loops for `fxActive == 0`).
  - Never loosen N0.
- **U4a at 44.1 / 96 / 192 k is unmeasured** (RESEARCH covered 48 k only). If a corner reads between −135 and −120 dB (CONTEXT's floor), the row FAILs: stop, record the corner, and decide. Do not move the threshold silently.
- **K2n for the ring stage.** A live ring-frequency step is phase-continuous and may not click, which is why the control is on the SVF step. The ring rows are subjects only.
- **Per-grain measurement margins:** V2 ρ 0.998 against a 0.9 gate, V3 40 / 40 against 95 %. V4 is a planner recipe: if its two classes don't separate at −30 dB, re-derive the threshold from the measured distribution (record it) before touching the gate logic. The R22 floor is the known trap.
- **R19 in dev builds:** until 2.4's wet safety exists, extreme res / level settings in a DAW can output far above 0 dBFS. Keep the monitors down for any ear-check of resonant presets. This is not a 2.3 defect (P30).
- **CPU over budget (not expected; projected 13–15 %).** The ARCHITECTURE lever order is:
  1. Hermite reads for rate ≤ 1 at level 0, with a timbre gate;
  2. SIMD lanes of 4;
  3. pool 96.
  - The Padé drive shaper (RESEARCH §2.3) is cheaper but approximates D6's tanh, so it needs Taylor's sign-off first.
  - Re-run N0, the carried rows and U2 after any lever.
- **Timing noise in C3.** It uses the median of 3 passes. If C3w lands within 2 points of 30 %, re-run it on an idle machine before deciding.
- **Harness wall time:** the new render rows (V2–V4, H, Z, K2, N6) add about 30–60 s. Use the 2.2 streaming render pool (one processor per job, built on the main thread) and run in the background. Do not thin the grids.
- **`fxSharedLatch` must only rewrite FX latch fields.** If it touched rate, gains or the window, V2n / V3n would fail for the wrong reason. It shares `assignFxLatch()` with `fill()`.
- **Do not touch grain-engine 1.1.0 or pitch-collections 1.0.0** (D1). `GrainPool` value-init is the no-bleed guarantee; it is used, not modified.
- **Shared checkout:** path-scoped commits only, with the branch and staging re-checked immediately before each commit. Other sessions' changes under `.claude/` and `plugins/O-GrainScatter/` are sitting in the tree right now.
