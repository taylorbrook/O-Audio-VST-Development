# Stage 2 — DSP · Phase 2.4 (Freeze, feedback network, output reverb) — Plan

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.4 of 6
**GSD phase:** plan
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `bb036601`
**Inputs:**
- `CONTEXT-2.4.md` (D9–D12 locked, carry-ins, execution defaults, listening checkpoint)
- `RESEARCH-2.4.md` (R23–R38). **Where it differs from ARCHITECTURE.md, ROADMAP.md or CONTEXT-2.4, RESEARCH-2.4 wins.**
- `SUMMARY-2.3.md`, `VERIFICATION-2.3.md` (deviation 4: B2 unseeded; W2o +0.483 dB, margin 0.017 dB)
- `research/ARCHITECTURE.md` §2, §3, §10–§14, Algorithm §1, §3, §4; `ROADMAP.md` Phase 2.4; `parameter-spec.md` (LOCKED)
- The 2.3 engine in `Source/dsp/`, `PluginProcessor.*`, `tests/render-harness/`, `tests/n0/`, `tests/r12/`; `modules/effects/reverb-unit` 1.1.0

---

## Sign-offs (Taylor, 2026-11-05, at plan)

| Item | Decision |
|---|---|
| **R29** — feedback tap law (ARCH §10 erratum) | **Adopted: `g_fb_s = 1 / (S1_s + 1)`** (unimodal bound). Loop level −0.3 … −6 dB vs the ARCH law (−3.5 dB at default overlap). The per-step ripple table stays in reserve, used only if the listening checkpoint says tails are too short. |
| **R30** — decay-gate scope (ROADMAP Phase 2.4 erratum) | **Adopted: scope the linear-decay gate to FX small-signal gain ≤ 1.** Drive > 0, LP/HP res > 30.83 % and crush > 0 are gated on **boundedness**. No tap compensation. Listening item 7 added (drive + feedback sustain: feature or fault). |

---

## Entry Check — contract checksums

Re-hash all six at the start of execute (`shasum -a 256`) against the STATUS.md frontmatter. CONTEXT-2.4 hashed them at `6aed8017`, and `bb036601` touched only `.planning/` (RESEARCH-2.4, STATUS). The executor confirms, before T1:
- `git diff --stat 6aed8017 HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt` is empty, so `Source/` at HEAD is the verified 2.3 tree byte for byte.
- `git diff --quiet 6aed8017 -- modules scripts/param-dump` is clean.

---

## Goal

Build the last three signal-path blocks of the engine:
- **Freeze** — a head-stop Write Arbiter with the ring-domain grain anchor (R23), the frozen reach floor and position origin (R24/R27), forced release on engage with a reflect at the head (R25), and the mirror content crossfade on release (R26).
- **Feedback network** — the bound tap law (R29), HP → LP → diffuser mix → `tanh(d·x)/d` (D12) → AGC → NaN scrub → z⁻³² (canonical +0, R36).
- **Output** — ReverbUnit 1.2.0 through plugin-owned atomics (D11), the D9 knee wet safety, and the D10 LP/HP half make-down.

At neutral settings (feedback 0, freeze off, reverb_mix 0, wet below −6 dBFS, no LP/HP res > 30.83 %) the engine stays **bit-identical to the verified 2.3 tree** (N1 vs `6aed8017`). The carried rows change **only** where D9 or D10 apply, proven by a legacy-hook run that reproduces the 2.3 baseline exactly. N0 and R12 keep passing.

At the end of 2.4:
- FUNC-02, FUNC-05, DSP-06, DSP-07 are gated objectively.
- QUAL-01 (freeze edges), QUAL-02 (max feedback), PERF-01 and PERF-03 (feedback + freeze + reverb) advance.
- The listening checkpoint (items 1–7) is ready to run at verify.
- The plugin installs and passes auval and pluginval 10 on VST3 and AU.

---

## Plan-phase decisions

- **P31 — N1, the neutral null, lands first on the unmodified tree** (CONTEXT gate 1). T1 is a separate commit, made before any `Source/` change.
  - `tests/n1/` is a sibling of `tests/n0/` (same `git archive` machinery), with `OLD_SHA = 6aed8017`.
  - `Source/` at HEAD *is* `6aed8017`, so N1 passes by construction at T1, the way N0 did on the 2.2 tree. It is the safety net from T2 on.
  - There are five negative controls (N1n-FB / RV / FZ / KN / MK). Each prints IDENTICAL until its stage lands, must DIFFER from then on, and is strict (`--strict`) from T10.
  - N0 (vs `c10d0f2c`) and R12 (vs `6508c635`) keep running unchanged.
- **P32 — R23 is the first `Source/` change** (RESEARCH "land R23 first"). It is a pure refactor before freeze exists: `W_piece == absCount` holds until the first freeze, so N1, N0, R12 and the carried diff must all be bitwise. It is committed alone.
- **P33 — Attribution by legacy hooks, not by a wet probe.** Two test hooks, `legacyKnee` (bypass D9) and `legacySvfMake` (force `svfMake = 1`), are defaulted from a harness flag `--legacy-24`.
  - After T5, `harness --legacy-24` must reproduce **baseline A** (T1) with an empty carried diff. That proves D9 and D10 are the only sources of change.
  - The rows that differ in the normal run are the **D9/D10 re-baseline list**. Each is attributed to D9 or D10 by running with only one hook. The list is recorded in SUMMARY-2.4.
  - The normal post-T5 run becomes **baseline B**, the reference for every carried diff from T6 on.
  - This replaces RESEARCH §10's "max|wet| INFO pass per carried row". The harness's parallel render pool makes a per-row global probe unreliable; the hook run is exact.
  - Carried units that test the SVF stage directly (U1b oracle, U1i INFO table) are the expected D10 changes. U1b's oracle gains the `m(k)` factor for LP/HP in T4: an edit to a carried row, listed in the re-baseline record.
- **P34 — The tap law is signed off:** `g_fb_s = 1 / (S1_s + 1)`, with S1 being `streamPower`'s `s1` **before** `spreadAmplitudeFactor` (R29, R31).
  - `OutputLaw.h` gains `fbTapS1(...)`, the same branch structure as `streamPower` (Hann / bank, free / capped) returning the pre-spread s1. `streamPower` is not edited: it uses `fbTapS1` only if the refactor is bitwise (the carried diff proves it); otherwise the two are kept as separate functions.
  - `tap_s = g_fb_s · level_s · aud_s / max(1, Σ level·aud)`. There is no stream pan, and mute / solo / disabled streams contribute 0.
  - Ramps: g_fb 20 ms like g_out; level and aud are the 10 ms mixer ramps' current values; `g_knob = 0.95·feedback` is an ExactRamp, cell-lerped.
  - Laws are computed from latched GridParams at ticks, never from `pool.liveWeight()`.
- **P35 — The decay gate is scoped** (signed off, R30, R32).
  - **Linear-decay rows (F3):** corners with FX gain ≤ 1 only. The gate is dB/s ≤ −0.05 from a linear fit over 2 s at −52 dBFS after the input is removed. dB/gen is INFO, using the diffuser credit computed from the plugin's rounded-length constant (never re-typed).
  - **Boundedness rows (F4):** drive / LP-HP res > 30.83 % / crush corners. Wet-path peak ≤ float(A), every sample finite, no subnormal chain state, and the return envelope ≤ the AGC ceiling + the attack overshoot bound.
  - **Negative control F3n:** an `fbArchLaw` hook (`1/max(1, S1)`) at the Tukey 0.25 / jitter 0 / spray 0 / position 0 corner must read dB/s **> 0** (RESEARCH: up to +4.3 dB/gen).
- **P36 — One shared TPT step function.** `GrainFx.h` gains `inline void tptStep(float& ic1, float& ic2, float a1, float a2, float a3, float x, float& v1, float& v2) noexcept`, holding the 2.3 statements verbatim. GrainFx's three mode loops call it, and so do the fb HP/LP.
  - Inlining does not merge statements, so FMA contraction is unchanged. The carried diff and N1 prove it.
  - **Fallback:** if either moves a bit, restore GrainFx's loops verbatim, give the fb filters their own copy of the statements, and record the duplication.
- **P37 — Feedback chain lives in a new header-only `FeedbackChain.h`.** It holds a `FeedbackChain` struct (state + `prepare(fs)` sizing + `process(float* l, float* r, int n)` + `scrubReset()`) and free law functions: `fbSat(x, d)`, `agcCoeffs(fs)`, `diffuserLengths(fs)`.
  - Diffuser lines are sized in `prepare()` on the message thread, at 4167 samples × 4 sections × 2 channels (21.7 ms × 192 k).
  - Lengths are `lround(ms·fs/1000)`: 226 / 398 / 667 / 1042 at 48 k, and 207 / 366 / 613 / 957 at 44.1 k. `kDiffuserMs` is the single source the rows read.
  - Stage order is exactly ARCH §12 / RESEARCH §8. AGC is stereo-linked, per sample, with `a = exp(−1/(τ·fs))` at τ 1 ms / 400 ms and ceiling 0.7.
  - The scrub tests `isfinite(xL + xR)` per sample. On a fault it resets every state, including `std::fill` of the diffuser lines (writes, no allocation), and writes 0.
  - The store is `fbDelay[ch][c] = y + 0.0f` (R36).
  - `kFbK = 1.41421366f`, the next float above √2 (R31).
  - fb HP/LP coefficients use `tan()` at ticks only. The cutoff targets are ramped on the grid (exponential, cell-lerped like the other 20 ms ramps). Read denormalised Hz; LP is clamped ≤ 0.45·fs.
  - The chain runs every pass, **frozen included**. Its output is stored, and the arbiter discards it while frozen.
- **P38 — Freeze is split into two tasks:** the arbiter and edge handling (T8), then the spawner geometry (T9).
  - **Edges** are sampled from the latched `freeze` Bool at absolute 32-sample ticks. No listener writes.
  - **Rising edge:**
    - Latch `W0 = W_stop` and `heldCaptured = W_stop − margin(2) − 1` (clamped ≥ 0).
    - Run the R25 forced-release scan over live and fade slots: forward grains with `δ − r·m < margin(level)` get the 2.2 5 ms kill.
  - **While frozen:** no push. The chain still runs.
  - **Falling edge:** a 10 ms raised-cosine `w` indexed from the release tick. `push = w·(in·g + fb) + (1 − w)·ring.levelSample(0, ch, 2·W0 − 1 − W)` (mirror, R26), with the mirror index clamped to `[W0 − heldCaptured, W0 − 1]`.
  - **Re-engage during the 10 ms fade:** latch the new W0 at the current head. The fade is abandoned, with no blend, because the push stops. Record this as an execution detail and cover it in N7's toggle pattern.
  - **R25 reflect** in `readGrain`: `if (delay < margin) delay = 2·margin − delay`.
    - `reachViolations` counts pre-reflection crossings by **non-killing** grains only.
    - New INFO counters: `headReflections` (all) and `headReflectionsFrozenSpawn` (grains spawned while frozen, expected 0).
  - The ring is never cleared, and the input is never gain-ramped.
- **P39 — Frozen spawner path** (R24, R27) in `Spawner::plan`, selected by a `frozen` flag plus `span_frz = min(spanSamples, heldCaptured)` passed in from the engine at every grid point.
  - Forward: `floorOff = r·len + 1`. Reverse: `floorOff = 0`, with the cap `D + r·len ≤ heldCaptured`.
  - `A = max(span_frz − floorOff, r·len/2)`; `w = min(max(spray·span_frz, r·len), A)`.
  - `x = position·A + w·(u − ½)` with `u = uni(Slot::PosSpray)`, **always drawn while frozen**, even at user spray 0.
  - `D = margin + floorOff + reflectInto(x, A)`.
  - The live path is untouched. CounterRng is keyed by slot, so no other draw moves.
  - **Hook `freezeNoMinSpray`:** the frozen path uses the live formula with the plain ARCH clamp. It is the ρ_H negative control.
- **P40 — reverb-unit 1.2.0** (D11, R35).
  - Add `void bind(std::atomic<float>* enabled, size, damp, predelay, mix, mod, shimmer) noexcept`, which only assigns the seven pointers. No other line of the module changes.
  - Bookkeeping: `module.yaml` version 1.2.0 + changelog entry; `modules/registry.yaml` version 1.2.0; the `used_by` O-Granulator entry carries the module version consumed, following the registry's existing convention.
  - **O-Marimba (the other consumer) must still compile unchanged**: build its target once as a non-breaking check. It is not reinstalled.
  - Plugin side:
    - It owns `revEnabled {1}`, `revPredelay {20}`, `revMod {0.2f}`, `revShimmer {0}`, `revSize`, `revDamp`, `revMix`.
    - The last three are written at ticks from `reverb_size/damp/mix ÷ 100`.
    - The reverb runs once per piece on a `setDataToReferTo` wrap of `wet[2][kCell]`.
  - The reverb (and the knee) run **before** `mixOut`'s mix-0 fast path, so tank state does not depend on the outer mix. The output stays bitwise dry.
  - PERF-03 rows with reverb toggles compare values (max|Δ| == 0), never memcmp or hashes (R35 −0/+0 hand-off).
- **P41 — Output structure** (R34 structure rule).
  - `mixOut` forms the wet once into `float wet[2][kCell]` with the 2.3 `wl += bus·gain` statement verbatim.
  - Then reverb → knee → `(xl·dg + wl·wg)·og`, also verbatim.
  - The knee is the branch form with `std::tanh`, with `kKneeT = 0.501187234f` and `kKneeA = 1.99526231f`.
  - The feedback tap is formed in the same per-cell pass, pre-g_out, into `tap[2][kCell]`.
  - Both new arrays have canaries in test builds (the existing guard pattern).
- **P42 — D10** (R33). `svfLatch` computes `m = k < √2 ? 1/√(1/(k·√(1 − k²/4))) : 1.0` in double from the latched float k, and casts to float into a new `Grain::svfMake` (default 1.0f; `GrainPlan` gains the same field).
  - LP output becomes `m * v2`; HP becomes `m * (x − k*v1 − v2)`. The `v1`/`v2` statements are untouched, and BP is unchanged.
  - The H2 steal compare and the H3 state check include the new field.
- **P43 — B2 is seeded in T1, before baseline A is captured** (VERIFICATION-2.3 deviation 4). `q->setSeed(0x5EED'B2B2'0000'0001ull)` goes right after `make()` in the B2 defaults loop. The three B2 rows then rejoin the carried diff. Run the harness twice at T1 to confirm they are deterministic.
- **P44 — Row files.** New rows go in `tests/render-harness/engine_gates_24.h` (U5–U8 units, F feedback) and `engine_gates_24b.h` (FZ, RV, Z3/Z4, N7, M4, C4). Carried row files are not edited, except U1b's D10 oracle (P33). The expected harness total is **about 220** (175 carried + about 45 new); record the actual count.
- **P45 — Listening checkpoint at verify,** as in 2.1. T13 writes `LISTENING-2.4.md`: a settings sheet per item (generic editor, monitors down). Items 1–7, where item 7 is drive + feedback sustain. Fallbacks are pre-wired as one-line changes: D12 `1 + 3·sat`, the R29 ripple table, and the R22 note. Execute does not wait on the ear.

---

## Source layout (deltas against 2.3)

| File | Change |
|---|---|
| `Source/dsp/EngineConstants.h` | `kFbKnob = 0.95f`; `kFbK = 1.41421366f`; `kFbNyquist = 0.45`; `kDiffuserMs[4] = {4.7, 8.3, 13.9, 21.7}`; `kDiffuserG = 0.7f`; `kDiffuserMaxLen = 4167`; `kSatSpan = 7.0f` (D12; fallback 3); `kAgcAttackS = 0.001`, `kAgcReleaseS = 0.4`, `kAgcCeil = 0.7f`; `kKneeT = 0.501187234f`, `kKneeA = 1.99526231f`; `kFreezeXfadeS = 0.010`; reverb fixed internals (`kRevPredelayMs = 20`, `kRevMod = 0.2f`, `kRevShimmer = 0`) |
| `Source/dsp/Grain.h` | `float svfMake = 1.0f` (D10). Still trivially copyable (static_assert) |
| `Source/dsp/ControlGrid.h` | `ParamPointers` + `GridParams` gain `freeze` (bool), `feedback` (readPct), `fbLowCutHz` (raw Hz 20…2000), `fbHighCutHz` (raw Hz 1000…20000), `fbSat`, `fbDiff`, `reverbMix/Size/Damp` (readPct), unless already present. Defaults = spec defaults. Never derive a unity value from `get()` on the skewed fb cut ranges |
| `Source/dsp/GrainFx.h` | `tptStep` shared statements (P36); `svfLatch` adds `make` (P42); LP/HP output × m |
| `Source/dsp/FeedbackChain.h` **(new, header-only)** | P37 |
| `Source/dsp/OutputLaw.h` | `fbTapS1(...)`, `fbTapGain(s1) = 1/(s1 + 1)`; `fbTapGainArch` (test reference only, for F3n / U8n) |
| `Source/dsp/Spawner.h` | `plan(..., bool frozen, double spanFrz, double heldCaptured)` frozen path (P39); `fill(..., std::int64_t W_on)` (R23: the argument is ring-domain, semantics documented) |
| `Source/dsp/CaptureWriter.h` | Arbiter: `write(...)` gains the frozen / crossfade state (`frozen`, `W0`, `heldCaptured`, `xfadePos`, `xfadeLen`) and the mirror continuation. The live path's statement is unchanged |
| `Source/dsp/Engine.h/.cpp` | R23 anchor + assert rewrite; freeze edge handling at ticks; forced-release scan; `readGrain` reflect + diag split; tap formation; `FeedbackChain` per pass; `wet[2][kCell]`, `tap[2][kCell]`; ReverbUnit member + atomics; knee; banner steps 6–8 say 2.4. `Diag` gains `freezeEngages`, `freezeReleases`, `forcedReleases`, `headReflections`, `headReflectionsFrozenSpawn`, `fbScrubs`. `TestHooks` gain `legacyKnee`, `legacySvfMake`, `fbArchLaw`, `freezeNoMinSpray`, `releaseSpliceOld` (W − heldSpan continuation), `freezeNoReflect`, `injectFbNan {atSample}`, `snapshotChain` (copy of chain state + ring FNV at every tick into a fixed ring of 64 snapshots) |
| `Source/PluginProcessor.cpp` | Cache the 9 new pointers; prepare the chain + reverb (message thread); `getTailLengthSeconds` unchanged (2.6) |
| `modules/effects/reverb-unit/cpp/ReverbUnit.h`, `module.yaml`; `modules/registry.yaml` | P40 |
| `tests/n1/run.sh`, `tests/n1/n1_render.cpp` **(new)** | The N1 out-of-tree neutral null vs `6aed8017` (T1) |
| `tests/render-harness/engine_gates_24.h`, `engine_gates_24b.h` **(new)** | P44 |
| `tests/render-harness/main.cpp` | B2 seed (T1); `--legacy-24` flag (T4); two `#include`s and section calls |
| `tests/render-harness/engine_gates_23.h` | U1b oracle × m(k) for LP/HP (T4, P33) — the only carried-row edit |

---

## Tasks

Build commands (from `build/`, Release, `OUARICON_BUILD_TESTS=ON`): `ninja O-Granulator-render-harness`, then `ninja OuariconGranulator_VST3 OuariconGranulator_AU`. The harness binary is `build/plugins/O-Granulator/O-Granulator-render-harness_artefacts/Release/O-Granulator-render-harness`; exit 0 means ALL PASS.

The harness takes about 2:05 at 175 rows and grows to about 3–4 min. **Run it, `tests/n1/run.sh`, `tests/n0/run.sh` and `tests/r12/run.sh` in the background** (memory: 600 s watchdog).

**The carried-row diff:** sorted `[PASS]` / `[FAIL]` lines, dropping the new prefixes (`U5`–`U8`, `F`, `FZ`, `RV`, `Z3`, `Z4`, `N7`, `M4`).
- **T2–T4** compare against **baseline A** (T1).
- **T5** checks the legacy run against A (P33) and captures **baseline B**.
- **T6 onward** compare against **B**. The diff must be empty.

### Wave 0 — the precondition, on the 2.3 tree (P31, P43)

**T1. N1 neutral null vs `6aed8017`; B2 seed; baseline A**
- Files: `tests/n1/run.sh` (new), `tests/n1/n1_render.cpp` (new), `tests/render-harness/main.cpp` (B2 seed only). **No `Source/` change.**
- `run.sh` copies `tests/n0/run.sh`:
  - `OLD_SHA=6aed8017`; WORK `${TMPDIR:-/tmp}/ogran-n1`.
  - The `modules scripts/param-dump` refusal becomes `git diff --quiet 6aed8017 -- modules/core/grain-engine modules/tuning/pitch-collections scripts/param-dump`. reverb-unit changes at T10, so the old side must extract its own `modules/effects/reverb-unit` from `6aed8017` into the work tree. Check that the archive includes it, and that the old CMake wrapper points at the extracted copy.
  - Brace-form revpaths `"${OLD_SHA}"` (memory: zsh `:` modifier); `--strict`.
- `n1_render.cpp` extends the N0 driver (48 k, 512 blocks, 6 s hash noise, R12 seed, exact `setP`).
  - Four streams, with 2.3 FX **on but non-peaking**: s1 LP 1.5 kHz res 20 % rand 50 %; s2 BP res 80 %; s3 drive 30 %, crush 40 % prob 50 %; s4 ring 300 Hz mix 50 % rand 50 %. Plus the N0 2.2 feature spread (spray, jitter, window 0.6 / skew, scale, reverse, gain_rand).
  - **Neutral 2.4 dials, non-default:** feedback 0 with fb_low_cut 400, fb_high_cut 3000, fb_saturation 80, fb_diffusion 70; freeze off; reverb_mix 0 with size 90, damp 10.
  - Cases `free` and `capped` as N0.
  - **Below-knee non-vacuity:** the driver prints max|wet| per side, where wet = (out − dry·dg)/wg is measured at mix 100 %, so out = wet·og with og = 1. Both cases must read < `kKneeT` with ≥ 1 dB margin. If one doesn't, lower `input_gain` on **both** sides (a driver config, recorded).
  - Controls (new side only, each vs the old side's same case):
    - `N1c`: seed + 1, must differ.
    - `N1n-FB`: feedback 1 %.
    - `N1n-RV`: reverb_mix 1 % (> the module's 0.1 % threshold).
    - `N1n-FZ`: freeze on at 2.0 s, off at 3.0 s.
    - `N1n-KN`: s1 level +12 dB (wet > T).
    - `N1n-MK`: s1 LP res 100 %.
- **Baselines also checked:** N0 and R12 print max|wet| (driver-side print added the same way) to confirm their cases sit below the knee. If one doesn't, apply the same both-sides `input_gain` fix and record it. Neither script's verdict changes at T1.
- **B2 seed** (P43). Then capture **baseline A**: harness stdout → `${TMPDIR:-/tmp}/ogran-24/harness-A.txt`, run twice, and diff the two runs (empty, so B2 is now deterministic).
- Acceptance:
  - `tests/n1/run.sh` exit 0: both cases max|Δ| == 0, N1c differs, max|wet| < T.
  - All five N1n print **IDENTICAL** (expected: no 2.4 code exists yet).
  - Harness 175/175. N0 `--strict` PASS. R12 PASS.
- Commit **alone**: re-check `git branch --show-current` + `git status --short`, then `git commit -- plugins/O-Granulator/tests`.
- Depends on: none.

### Wave 1 — ring-domain anchor, then plumbing (bitwise no-ops)

**T2. R23 ring-domain grain anchor**
- Files: `Engine.cpp`, `Spawner.h` (doc comment + param name), `Engine.h` (`SpawnRecord::writeHead` comment).
- `W_piece = ring.getWriteCount()` read before step 3. A live spawn uses `W_on = W_piece + offset + 1`. The frozen branch (`W_on = W_frozen`) is added in T8; T2 leaves a `frozen == false` constant.
- The spawn log's `writeHead = W_on`.
- The assert becomes `jassert(ring.getWriteCount() == W_piece + (frozen ? 0 : len))`.
- Acceptance: N1 PASS (both cases); carried diff vs A empty; N0 `--strict` + R12 PASS; 0 new warnings.
- Commit alone (path-scoped).
- Depends on: T1.

**T3. Plumbing: constants, Grain, ControlGrid, processor pointers, Diag / TestHooks / record declarations, banner**
- Files: `EngineConstants.h`, `Grain.h` (`svfMake`, unused until T4), `ControlGrid.h`, `Engine.h` (Diag, TestHooks declared, `Diag::reset()` extended; banner steps 6–8 → 2.4), `PluginProcessor.cpp`.
- Acceptance: build with 0 new warnings; carried diff vs A empty; N1 PASS; the `Grain` static_assert holds.
- Depends on: T2.

### Wave 2 — output laws (D10, D9), the only planned carried-row changes

**T4. D10 make-down + U6; `--legacy-24`**
- Files: `GrainFx.h` (`svfLatch` m, LP/HP × m, `tptStep` extraction per P36), `Spawner.h` (`GrainPlan::svfMake`, `assignFxLatch`), `Engine.cpp` (`legacySvfMake` hook; H2/H3 field lists), `main.cpp` (`--legacy-24` sets `legacyKnee` + `legacySvfMake` defaults for every processor the harness makes), `engine_gates_23.h` (U1b oracle × m), `engine_gates_24.h` (new: U6).
- Write U6 against a stub first and watch it fail.
  - **U6a:** `svfMake == 1.0f` bitwise for res on a 1e-4 grid over [0, 0.3083], at 44.1 / 48 / 96 / 192 k. At res 0.3084 it is < 1.
  - **U6b:** LP and HP steady-sine peak (sine at the mapped peak frequency, LS fit over the last 0.5 s of 2 s) at res 100 % = **+10.0 dB ± 0.1**, at fc 1 kHz and 0.45·fs × 44.1 / 48 / 96 / 192 k.
  - **U6c:** P(k) analytic vs the measured LP peak at res 50 / 80 / 90 / 100 % within ±0.01 dB (+0.977 / +6.633 / +10.844 / +20.011 dB before make-down); m matches `1/√P` within 1 ulp.
  - U1a (BP 0 dB, D8) re-runs unchanged and must PASS.
- Acceptance:
  - U6 PASS; U1a / U1b (new oracle) PASS.
  - **N1n-MK DIFFERS**; N1 PASS (its LP is res 20 %).
  - `harness --legacy-24` carried diff vs A empty (the knee doesn't exist yet, so this isolates D10).
  - The normal-run diff vs A lists only D10-attributable rows. Record them.
- Depends on: T3.

**T5. D9 knee + U5; `wet[2][kCell]` restructure; baseline B**
- Files: `Engine.h/.cpp` (`mixOut`: the wet array + knee + `legacyKnee` hook; the `tap[2][kCell]` array is declared but stays unused until T7), `engine_gates_24.h` (U5).
- **U5a — identity:** exhaustive over every float with |x| ≤ T, both signs (2.11 × 10⁹): `knee(x)` bitwise == x. Run it on 4 threads; it takes about seconds (RESEARCH `knee.cpp`).
- **U5b — shape:** monotone over (T, 2²⁰]; y ≤ x above T; **max y == float(A)** (≤ A, R34); NaN → NaN; ±inf → ±A.
- **U5c — C¹ and slope:** the finite-difference slope with step ≥ 1e-3 is ≤ 1 over (0, 8], and the slope at T⁺ (step 1e-3) is ≥ 0.999.
- **Attribution (P33):**
  - `--legacy-24` diff vs A empty.
  - Run with only `legacyKnee` and with only `legacySvfMake` to split the re-baseline list into D9 and D10 rows.
  - **W2o must not be on the list.** If it is, stop: CONTEXT says it sits far below the knee, so bisect.
  - Save the normal run as **baseline B** (`harness-B.txt`).
- Acceptance: U5 PASS; **N1n-KN DIFFERS**; N1 PASS; N0 `--strict` + R12 PASS (their cases sit below the knee, per T1); re-baseline list recorded.
- Commit T3–T5 (path-scoped, after the branch / status re-check).
- Depends on: T4.

### Wave 3 — feedback network (tracer: the chain lands end to end before freeze)

**T6. `FeedbackChain.h` + U7, U8**
- Files: `GrainFx.h` (`tptStep` already shared), `FeedbackChain.h` (new), `OutputLaw.h` (`fbTapS1`, `fbTapGain`, `fbTapGainArch`), `engine_gates_24.h`.
- Write the units against stubs first.
  - **U7a — fb HP/LP:** at k = kFbK, the response matches the bilinear oracle ±0.05 dB where |H| > −40 dB, for fc {20, 80, 2 k} (HP) and {1 k, 12 k, 0.45·fs} (LP) × 44.1 / 48 / 192 k. Peak |H| ≤ 1 + 1e-6 (no resonance, R31). The LP clamp holds at 20 kHz / 44.1 k.
  - **U7b — diffuser:** lengths equal the exact tables at 44.1 and 48 k (and are computed from `kDiffuserMs`); each section |H| = 1 ± 1e-4 dB (impulse-response FFT); the mix law is `(1 − m)·x + m·AP4(x)`, and m = 0 is bitwise x.
  - **U7c — saturation (D12):** `fbSat(x, d)` slope at 0 is 1 (≤ 1e-6 relative at |x| = 1e-6); |y| ≤ 1/d; d = 1 at sat 0; monotone.
  - **U7d — AGC:** a constant 0 dBFS input settles to gain 0.7 ± 0.001; attack reaches 63 % within 1 ms ± 1 sample and release within 400 ms ± 1 %; env 0 → gain exactly 1 (finite).
  - **U7e — R36:** an all-±0 input yields a stored `fbDelay` of +0 (bit pattern 0) at every stage.
  - **U7f — scrub:** NaN / +Inf / −Inf injected at the chain input on L, R and both (9 cases) → output 0, every state zeroed (diffuser lines included), and `fbScrubs` +1. Finite inputs never trip it.
  - **U8 — tap law:** `peak_t Σ_k w(t − kH) · fbTapGain(S1)` ≤ −0.17 dB + 0.01 for every bank step × tilt {−1, 0, 1} × N ∈ [0.05, 40] (log grid, 200 points), Hann included (unimodal bound, R29). **U8n:** `fbTapGainArch` at Tukey 0.25 reads ≥ +4.0 dB (RESEARCH +4.78), which proves U8 discriminates.
- Acceptance: U5–U8 PASS; carried diff vs B empty (`tptStep` refactor bitwise, P36); N1 PASS.
- Depends on: T5.

**T7. Tap + chain wiring; F1, F2, F3, F4, F5; N7 (feedback form)**
- Files: `Engine.h/.cpp`, `engine_gates_24.h` (F), `engine_gates_24b.h` (new: N7), `main.cpp` (include + section calls).
- Per pass: step 6 forms `tap[ch][i] = Σ_s bus[s][ch][i]·fbTap_s.at(c)` in the mixer loop. Step 7 runs `chain.process(tapL, tapR, len)` and stores `fbDelay`. Step 3 of the **next** pass reads it (z⁻³²).
- Hooks wired: `fbArchLaw`, `injectFbNan`.
- **F1 — recirculates:** an impulse burst (10 ms noise at −20 dBFS, then silence), s1 only, feedback 50 %, diffusion 0. Energy in the 200 ms after the burst's grains end is ≥ 20 dB above the feedback-0 control.
- **F2 — each stage changes the loop:** HP (fb_low_cut 80 → 1000 Hz), LP (12 k → 2 k), saturation (0 → 100 % on a −6 dBFS loop), diffusion (0 → 100 %). Each pair differs from its same-settings control by ≥ 1 dB of band energy (HP: < 500 Hz; LP: > 4 kHz), or by ≥ 0.5 dB of crest (sat), or by echo-density autocorrelation (diffusion: the peak at the loop period falls by ≥ 50 %). Each control row also proves the measurement is non-vacuous: the stage at neutral differs from the other config.
- **F3a — linear decay, scoped (P35):**
  - Corners: windows Hann / Tukey 0.25 / Trapezoid / Expo × jitter 0 × spray 0 × position 0; feedback 100 %; FX Off / BP res 100 % / LP res 30 % / ring 100 %.
  - Input −52 dBFS noise for 3 s, then removed.
  - Pass: dB/s ≤ −0.05 at every corner. Print dB/gen as INFO.
- **F3b:** tone and diffusion extremes (fb_low_cut 2 k, fb_high_cut 1 k, diffusion 100 %) and +12 / +24 st pitch, under the same rule.
- **F3n — negative control:** `fbArchLaw` at Tukey 0.25 / jitter 0 / spray 0 / position 0 gives dB/s **> 0**.
- **F4 — FX-gain corners (boundedness):** drive 100 %, LP res 100 %, HP res 100 %, crush 100 %, each at feedback 100 % with a −52 dBFS then −6 dBFS input, 20 s, mix 100 %, output 0 dB.
  - Wet-path peak ≤ float(A); every sample finite; no `FP_SUBNORMAL` in the chain state at section ends; the return envelope over the last 5 s ≤ 0.7 + 1/d.
- **F5 — tap composition:** a hard-panned s1 (pan 100 %) gives tap L == tap R (no pan in the tap). A muted / un-soloed / disabled s2 at +6 dB changes the tap by exactly 0 (bitwise tap equal to the s1-only render).
- **N7 (feedback form):** the N6 all-FX recipe + feedback 80 %, diffusion 50 %, saturation 50 %, with a fb_high_cut change at a shared boundary. Blocks 441 / 512 / 4096 / variable vs 32: max|Δ| == 0. **N7c:** a different seed differs.
- Acceptance: F1–F5, N7 PASS; **N1n-FB DIFFERS**; N1 PASS (feedback 0: chain running, R36); carried diff vs B empty; 0 new warnings.
- Commit the tracer (path-scoped).
- Depends on: T6.

### Wave 4 — freeze

**T8. Write Arbiter, edges, forced release, head reflect; FZ1, FZ2, FZ6, FZ7**
- Files: `CaptureWriter.h`, `Engine.h/.cpp`, `Spawner.h` (frozen W_on only), `engine_gates_24b.h`.
- Implement P38: tick-sampled edges, rising-edge latch + R25 scan, no push while frozen, mirror crossfade, reflect + diag split.
- Hooks wired: `releaseSpliceOld`, `freezeNoReflect`, `snapshotChain`.
- **FZ1 — FUNC-02 hold:** freeze at 2 s, then 10 s of new noise input. The FNV-1a hash of level 0 (both channels, full capacity, harness-side) is identical at the freeze tick and 10 s later. **FZ1n:** the same with freeze off differs.
- **FZ2a — engage click** (QUAL-01; memory: click probes need a tonal source + a linear wet sum):
  - A sine 440 source at spray 0 / position 0 (100 % forced-release crossers). κ over ±0.5 s around the engage tick vs a same-motion control (freeze held from the start, identical automation).
  - Pass ≤ 2.0. **FZ2an:** `freezeNoReflect` > 2.0.
  - If FZ2an reads ≤ 2.0, the stimulus is below threshold. Re-derive it (e.g., +24 st, so crossers cross faster) **before** trusting FZ2a, and record it.
- **FZ2b — release click:** the same source, release at 5 s, vs the same-motion control. Pass ≤ 2.0. **FZ2bn:** `releaseSpliceOld` > 2.0 (R26).
- **FZ6 — reach diag:**
  - Engage at position 0 / spray 0 / +12 st: `reachViolations == 0` (non-killing), `headReflections > 0` (the engage crossers reflect), and `forcedReleases` == the count from the geometry recomputed on the hook's pre-engage snapshot.
  - A 10 s frozen run: `headReflectionsFrozenSpawn == 0` (from T9 on; T8 records it as INFO, because the frozen spawner path doesn't exist yet).
- **FZ7 — freeze + feedback:** feedback 80 %, freeze at 2 s for 5 s.
  - `snapshotChain`: HP/LP integrators and the AGC env are non-zero and differ between ≥ 90 % of consecutive tick pairs while frozen (the chain is warm).
  - The ring hash is unchanged across the hold (the return is discarded).
  - The output stays finite.
- Acceptance: FZ1, FZ2a/b (+ controls), FZ6, FZ7 PASS; **N1n-FZ DIFFERS**; N1 PASS; carried diff vs B empty.
- Depends on: T7.

**T9. Frozen spawner geometry; FZ3, FZ4, FZ5, FZ8; N7 (freeze form)**
- Files: `Spawner.h`, `Engine.cpp` (passes `frozen`, `span_frz`, `heldCaptured`), `engine_gates_24b.h`.
- Implement P39. Hook wired: `freezeNoMinSpray`.
- **FZ3 — anti-buzz (R28):** a noise source, freeze ≥ 10 s rendered, at spray 0 / position 0 / jitter 0, Hann, density 10, size 100 ms (where `A ≥ r·len`). ρ_H at H = fs/density, mean removed, is **≤ 0.2**. **FZ3n:** `freezeNoMinSpray` gives ρ_H **≥ 0.9**.
- **FZ3c — ROADMAP crest row:** frozen crest ≤ 1.5× live. PASS, but labelled "non-diagnostic on noise (R28)" in the row text.
- **FZ4 — freeze 0.5 s after load:** at default size and at 500 ms grains, the frozen output RMS over 2 s is > −60 dBFS (not silent) and ρ_H ≤ 0.5 (INFO at 500 ms, R27).
- **FZ5 — 60 s drift:** a frozen 60 s render, the RMS of the first 5 s vs the last 5 s < 0.5 dB.
- **FZ6** (from T8) is now strict: `headReflectionsFrozenSpawn == 0`. A reverse sub-row checks that no frozen reverse grain reads beyond `heldCaptured` (record-side check `D + r·len ≤ heldCaptured`).
- **FZ8 — R37 release decay:**
  1. Freeze 5 s at feedback 100 % (input −52 dBFS, a linear corner per P35), then release.
  2. At release + 10 ms, remove the input.
  3. Pass: dB/s within ±0.2 of the steady-loop row with the same settings, and < 0.
- **N7 widened:** + freeze toggles at three shared ticks (including one re-engage inside the 10 ms release fade), with a frozen position automation. max|Δ| == 0; N7c differs.
- **Live-path guard:** the N1 / N0 / R12 nulls must stay bitwise (the frozen path is unreachable live).
- Acceptance: FZ3–FZ8, N7 PASS; N1 PASS; carried diff vs B empty.
- Commit T8–T9 (path-scoped).
- Depends on: T8.

### Wave 5 — reverb

**T10. reverb-unit 1.2.0 + plugin integration; RV1–RV4**
- Files: `modules/effects/reverb-unit/cpp/ReverbUnit.h`, `module.yaml`, `modules/registry.yaml`, `Engine.h/.cpp` (or `PluginProcessor.*`, whichever owns `mixOut`'s call site), `engine_gates_24b.h`.
- Implement P40 + P41's order (reverb → knee → dry/wet; reverb and knee ahead of the mix-0 fast path).
- **Non-breaking check:** O-Marimba's VST3 target compiles with 0 new warnings against 1.2.0. Build only; no install.
- **RV1 — audible:** reverb_mix 50 %, size 0: wet energy minus the reverb_mix-0 control > −40 dBFS during input, and a tail RT60 > 0.2 s after the input stops.
- **RV2 — mix → 0:** reverb_mix 50 → 0 at a tick. κ step ratio vs a static control ≤ 2.0; the wet-minus-input energy < −90 dBFS within 60 ms.
- **RV3 — re-enable:** 0 → 50 % after 2 s off. κ ≤ 2.0 vs a static-on control (cold tank, 0 → 1 ramp). The `clearTank` spike is INFO.
- **RV4 — fast path:** outer mix 0 % with reverb_mix 30 %: the output is bitwise dry (memcmp vs input × og), and the tank is active (a hook reads `running == true`).
- Acceptance:
  - RV1–RV4 PASS; **N1n-RV DIFFERS**; **N1 `--strict` PASS** (all five N1n differ; strict from here on).
  - N0 `--strict` + R12 PASS. Carried diff vs B empty (the B2 mix-0 memcmp rows stay exact: RV4's property).
- Commit (path-scoped: `-- plugins/O-Granulator modules/effects/reverb-unit modules/registry.yaml`).
- Depends on: T9.

### Wave 6 — gates (`engine_gates_24b.h`; no `Source/` change)

**T11. DSP-06 / QUAL-02 stress: Z3; NaN scrub in the engine: Z4**
- **Z3 — the ROADMAP row:** 60 s, feedback 100 %, density 200, size 500 ms, pitch +12, 4 streams at +6 dB, LP res 100 %, mix 100 %, output 0 dB, saturation default, diffusion 50 %, reverb_mix 30 %. Noise then sine-at-fc sections. Runs at 48 k + 44.1 k + 192 k.
  - Wet-path peak ≤ float(A) (+6.0 dBFS); every sample finite; no subnormal in the pool / chain state at section ends; `fbScrubs == 0` and `nanKills == 0`.
- **Z3b:** the same with HP res 100 % + drive 100 % + crush 100 % + ring 100 % (all FX-gain corners at once). Same pass conditions.
- **Z4 — scrub in situ:** `injectFbNan` at 3.0 s at feedback 80 %. `fbScrubs == 1`; every output sample finite; the chain state is all zero at the next tick; the render after 3.0 s + 32 samples equals a reference render whose chain was reset (`std::fill`) by the hook at the same tick without a NaN (max|Δ| == 0).
- Depends on: T10.

**T12. PERF rows: N7 (final), M4, C4**
- **N7 final:** the T9 recipe + reverb_mix 0 → 30 % → 0 at shared boundaries, every randomisation on, the custom swap and the mute / solo toggles from N6. 32 / 441 / 512 / 4096 / variable; **value compare** (R35); N7c differs. Non-vacuity: forced releases > 0, head reflections > 0, fbScrubs == 0, reverb `running` toggled.
- **M4 — allocations:** M3 coverage + freeze toggles, feedback 0 ↔ 100 %, diffusion / tone / sat sweeps, an `injectFbNan` scrub, and reverb enable / disable. 0 allocations; liveness 1.
- **C4 (INFO):** C3w + feedback 100 % + diffusion 100 % + reverb_mix 50 %. Warm-up 4.5 s, median of 3 × 10 s. Print % of one core, Δ vs C3w (re-run), and the remaining 2.5/2.6 headroom against 30 %. RESEARCH projects about 18 %. If over 25 %, record it and flag it for 2.6; it does not fail the phase (CONTEXT: the budget gate is in 2.6).
- Depends on: T11.

### Wave 7 — validate + docs

**T13. Build, install, validate; listening sheet**
- Harness ALL PASS (record the count, about 220) with 0 new warnings; carried diff vs B empty.
- `tests/n1/run.sh --strict`, `tests/n0/run.sh --strict` and `tests/r12/run.sh` PASS.
- `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep, AU cache clear). `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10, VST3 + AU. Its randomised params now hit feedback 100 % and freeze toggles (memory: pluginval-10 catches latent NaN).
- Static checks: grep only non-comment lines, and confirm the baseline at HEAD first.
  - `StateVariableTPTFilter|IIR::` in `Source/` → 0.
  - `std::vector|new |malloc|make_unique` in `FeedbackChain.h` → 0.
  - `std::tan` in `FeedbackChain.h` only in the tick-coefficient function.
  - `git diff --stat 6aed8017 -- modules/core/grain-engine modules/tuning/pitch-collections` empty (D1).
  - `git diff 6aed8017 -- modules/effects/reverb-unit/cpp` adds the bind overload only.
- Write `stages/2-dsp/LISTENING-2.4.md` (P45). Items 1–7 each get a settings recipe for the generic editor, what to listen for, and a pre-wired fallback.
- Depends on: T12.

**T14. Docs + state**
- Write SUMMARY-2.4.md:
  - the gate table with measured values;
  - N1 / N0 / R12 results; the D9/D10 re-baseline list (P33) and the attribution runs; C4;
  - the errata list below, the 2.5 carry list, and the execution deviations.
- STATUS.md → `stage_2_phase_2_4_execute_complete` (next: verify, with the listening checkpoint). REQUIREMENTS stays untouched until verify.
- Commit path-scoped after the re-check: `git commit -- plugins/O-Granulator` (+ `modules/effects/reverb-unit modules/registry.yaml` if they were not already committed at T10).
- Depends on: T13.

---

## Dependency graph

```
T1 ─ T2 ─ T3 ─ T4 ─ T5 ─ T6 ─ T7 ─ T8 ─ T9 ─ T10 ─ T11 ─ T12 ─ T13 ─ T14
(N1) (R23)     └ D10 / D9 ┘ └ feedback ┘ └ freeze ┘ (reverb)  └─ gates ─┘
```

The chain is strictly sequential: every task touches `Engine.cpp` or the N1 control set, and each N1n control flips at its own task (MK T4, KN T5, FB T7, FZ T8, RV T10 → strict). Within T11–T12, the row authoring has no `Source/` change and could run in parallel with T13's install prep, but it is kept sequential for clean carried diffs.

---

## Gate criteria (new rows)

There are about 45 harness rows, 8 N1 script rows and the T13 validation. INFO rows: dB/gen (F3), FZ4 ρ at 500 ms, the RV3 spike, C4.

| Row | Measures | Pass | Negative control | Req | Task |
|---|---|---|---|---|---|
| N1 free / capped | out-of-tree null vs `6aed8017`, neutral non-default 2.4 dials, 2.3 FX non-peaking | max\|Δ\| == 0; max\|wet\| < T − 1 dB | N1c | COMPAT-01 | T1 |
| N1n-MK / KN / FB / FZ / RV | one 2.4 element minimally on | each differs (strict from T10) | — | — | T1, T4–T10 |
| Legacy attribution | `--legacy-24` vs baseline A | carried diff empty | — | COMPAT-01 | T4, T5 |
| U5a / b / c | knee identity, shape, slope | bitwise ≤ T; max == float(A); slope ≤ 1 | — | DSP-06 (D9) | T5 |
| U6a / b / c | D10 m, peak, P(k) | m == 1 for res ≤ 30.83 %; +10.0 ± 0.1 dB; ±0.01 dB | — | DSP-05 (D10) | T4 |
| U7a–f | fb filters, diffuser, sat, AGC, R36, scrub | see T6 | — | FUNC-05, QUAL-02 | T6 |
| U8 | bound tap law over bank × tilt × N | peak ≤ −0.16 dB | U8n ≥ +4.0 dB | FUNC-05 (R29) | T6 |
| F1 | recirculation | ≥ 20 dB over fb-0 | fb-0 control | FUNC-05 | T7 |
| F2 (×4) | HP / LP / sat / diffusion each change the loop | thresholds in T7 | same-settings controls | FUNC-05 | T7 |
| F3a / F3b | linear decay, scoped corners | dB/s ≤ −0.05 | F3n > 0 | FUNC-05, DSP-06 | T7 |
| F4 | FX-gain corners | peak ≤ A; finite; env ≤ 0.7 + 1/d | — | DSP-06 (R30) | T7 |
| F5 | tap: no pan, muted streams 0 | L == R; bitwise | — | FUNC-05 | T7 |
| FZ1 | frozen ring hash, 10 s | identical | FZ1n differs | FUNC-02 | T8 |
| FZ2a / FZ2b | engage / release click vs same motion | κ ≤ 2.0 | FZ2an / FZ2bn > 2.0 | QUAL-01, FUNC-02 | T8 |
| FZ3 / FZ3c | ρ_H anti-buzz; crest (non-diagnostic) | ≤ 0.2; ≤ 1.5× | FZ3n ≥ 0.9 | FUNC-02 | T9 |
| FZ4 | freeze 0.5 s after load | > −60 dBFS | — | FUNC-02 | T9 |
| FZ5 | 60 s frozen drift | < 0.5 dB | — | FUNC-02 | T9 |
| FZ6 | reach diag | 0 violations; engage reflections > 0; frozen-spawn reflections 0; reverse cap | — | FUNC-02 | T8, T9 |
| FZ7 | freeze + feedback: chain warm, ring untouched | ≥ 90 % ticks evolving; hash equal | — | FUNC-05 | T8 |
| FZ8 | release decay vs steady | ±0.2 dB/s, < 0 | — | FUNC-05 (R37) | T9 |
| RV1–RV4 | reverb audible / mix → 0 / re-enable / fast path | see T10 | static controls | DSP-07 | T10 |
| Z3 / Z3b | 60 s max feedback stress, 3 rates | peak ≤ float(A); finite; no subnormal; 0 scrubs | — | DSP-06, QUAL-02 | T11 |
| Z4 | in-situ scrub | 1 scrub; ≡ reset reference | reset reference | QUAL-02 | T11 |
| N7 / N7c | block-size null, feedback + freeze + reverb | max\|Δ\| == 0; seed differs | N7c | PERF-03 | T7, T9, T12 |
| M4 | allocations | 0; liveness 1 | liveness | PERF-01 | T12 |
| C4 | CPU, INFO | report vs 30 % | C3w re-run | PERF-02 | T12 |

---

## Success criteria (gate for verify)

| # | Criterion | Req | Task |
|---|---|---|---|
| 1 | N1 neutral null vs `6aed8017` bitwise (free + capped), below knee, seed control differs; all five N1n differ (strict); N0 `--strict` + R12 PASS | COMPAT-01 | T1, T10 |
| 2 | Carried rows: every change attributable to D9 / D10 (legacy run ≡ baseline A); from T6 on, the diff vs B is empty; W2o unchanged; B2 deterministic | COMPAT-01 | T4, T5, T13 |
| 3 | D9 knee: bitwise identity ≤ T, monotone, y ≤ x, max == float(A), slope ≤ 1 | DSP-06 | T5 |
| 4 | D10: m == 1 for res ≤ 30.83 %; LP/HP res 100 % = +10.0 ± 0.1 dB at every fs; BP D8 unchanged | DSP-05 | T4 |
| 5 | Feedback chain laws: Butterworth fb filters, diffuser lengths + allpass, D12, AGC, R36, scrub; bound tap law ≤ −0.16 dB with the ARCH-law control ≥ +4 dB | FUNC-05 | T6 |
| 6 | Feedback recirculates; HP / LP / sat / diffusion each change the loop; tap excludes pan and muted streams | FUNC-05 | T7 |
| 7 | Linear decay dB/s ≤ −0.05 at every FX-gain-≤-1 corner (ARCH-law control grows); FX-gain corners bounded | FUNC-05, DSP-06 | T7 |
| 8 | Freeze: ring hash held 10 s; engage + release κ ≤ 2.0 (controls > 2.0); ρ_H ≤ 0.2 (control ≥ 0.9); not silent 0.5 s after load; 60 s drift < 0.5 dB; reach diag clean | FUNC-02, QUAL-01 | T8, T9 |
| 9 | Freeze + feedback: chain warm, ring untouched; release decay matches steady ±0.2 dB/s | FUNC-05 | T8, T9 |
| 10 | Reverb audible with a tail; mix → 0 fades with no step; re-enable no click; mix-0 fast path bitwise dry with tank running | DSP-07 | T10 |
| 11 | 60 s max-feedback stress at 3 rates (and all FX-gain corners): peak ≤ +6 dBFS, finite, no subnormal; in-situ scrub exact | DSP-06, QUAL-02 | T11 |
| 12 | Block-size null with feedback + freeze toggles + reverb toggles, value compare; seed control differs | PERF-03 | T12 |
| 13 | 0 allocations across freeze / feedback / reverb / scrub; liveness 1 | PERF-01 | T12 |
| 14 | reverb-unit 1.2.0 non-breaking (O-Marimba compiles; bind overload only); 0 new warnings; install; auval; pluginval 10 VST3 + AU; static checks clean | COMPAT-01 | T10, T13 |
| 15 | C4 reported (INFO); LISTENING-2.4.md ready (items 1–7) | PERF-02 | T12, T13 |

Requirement status expected at verify (after listening):
- **Complete:** FUNC-02, FUNC-05, DSP-06, DSP-07.
- **QUAL-01** freeze covered; matrix sweeps are 2.5.
- **QUAL-02** max feedback covered; the full hardening re-run is 2.6.
- **PERF-01 / PERF-03** advanced (matrix 2.5).
- **PERF-02** C4 measured; the budget gate is 2.6.

---

## Errata for SUMMARY-2.4 (record at T14; ARCHITECTURE / ROADMAP not edited — checksums)

1. **D9 wet safety** vs ARCH §14 `1.9953·tanh(x/1.9953)`: a knee with identity ≤ −6 dBFS. The ceiling is **≤ A** (max y == float(A)), not "< A" (R34, CONTEXT correction).
2. **D10 amends D8:** LP/HP × 1/√P(k), with P depending on k only (R33). The LP/HP rows with res > 30.83 % re-baseline (list from P33).
3. **R29 tap law** (signed off): `1/(S1 + 1)` vs ARCH §10 `1/max(1, N_eff·μ₁)`. S1 is taken pre-spread; there is no pan in the tap (R31).
4. **R30 decay-gate scope** (signed off) vs ROADMAP "dB/generation < 0" at every corner: linear decay at FX gain ≤ 1, boundedness elsewhere. The gate metric is dB/s (sign-equivalent, R32); dB/gen is INFO.
5. **R23** ring-domain anchor: the `writeCount == absCount + len` assert is replaced.
6. **R24 / R27** frozen floor + floor origin vs the ARCH plain clamp. PosSpray is drawn at spray 0 while frozen.
7. **R25** reflect at the head vs ARCH's implicit "5 ms fade fits"; the `reachViolations` definition changes (non-killing only).
8. **R26** mirror continuation vs CONTEXT / ARCH `ring[W − heldSpan]`.
9. **R28** anti-buzz detector ρ_H vs CONTEXT / ROADMAP crest. The crest row is kept and labelled non-diagnostic.
10. **R35** reverb mix ≤ 0.1 % is silently off (module threshold); mix → 0 fades the tail in 50 ms (no let-ring mode). PERF-03 uses value compare.
11. **P33** attribution by legacy hooks instead of the RESEARCH §10 per-row wet probe. U1b's oracle is edited (× m).
12. **P43** B2 seeded; the three rows rejoin the carried diff.
13. **D11** reverb-unit 1.2.0 (non-breaking bind overload); registry updated.
14. The `Engine.h` banner says 2.4 for the reverb + wet safety (was 2.6).
15. Any execution deviation, with the measured reason, and the final harness count.

## Carried to 2.5 / 2.6 (not implemented in 2.4)

- The matrix writes the reverb atomics (2.5); 2.4 writes base values only.
- Full PERF-02 worst-case budget gate (2.6); `getTailLengthSeconds` with reverb + feedback (2.6).
- R29 ripple-table refinement — only if listening item 2 says tails are too short.
- D12 `1 + 3·sat` fallback and R22 — decided by listening items 2 and 5.
- W2o rule unchanged: widen seeds, never the threshold.

---

## Risks / watch items

- **FP contraction breaks N1** (`tptStep` extraction, the `wet[]` restructure, tap formation in the mixer loop). Bisect to the statement and restore its 2.3 text. Take the P36 fallback if needed. Never loosen N1.
- **The knee changes rows nobody expected.** P33's legacy run separates the knee from bugs. If any carried row changes with **both** hooks on, that is a regression, not a re-baseline: stop and bisect.
- **N0 / R12 cases above the knee.** T1 measures this. Fix it with a both-sides input-gain change, never by excluding the row.
- **FZ2an / F3n vacuity** (memory: stimulus below threshold). Each negative control must fail its gate **before** the subject row's PASS is trusted. If one doesn't, re-derive the stimulus and record it.
- **The decay fit on a self-similar frozen wash (FZ8).** If the slope noise exceeds ±0.2 dB/s, lengthen the fit window (4 s) before widening anything. Record it.
- **The reverb `clearTank` memset spike** (about 50–100 µs at enable). It is INFO; it is not an allocation. If pluginval flags a timing issue at 192 k, record it for 2.6.
- **O-Marimba build break from 1.2.0.** Not expected: the change is an overload only. If it happens, fix it in the module, never in O-Marimba.
- **Re-engage inside the release fade.** This is the riskiest new state transition. It is covered by N7 + M4. If FZ2 controls show a click at re-engage, add an FZ2c row and record it.
- **Harness wall time** grows to about 3–4 min (60 s Z3 × 3 rates, the FZ5 60 s render, the U5 exhaustive pass). Use the streaming render pool, run in the background, and do not thin the grids.
- **Do not touch grain-engine 1.1.0 or pitch-collections 1.0.0** (D1). reverb-unit changes only by the P40 overload.
- **Shared checkout:** path-scoped commits only. Re-check the branch and staging immediately before each commit. Other sessions' changes under `.claude/` and `plugins/O-GrainScatter/` are in the tree.
