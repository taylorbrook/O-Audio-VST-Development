# Stage 2 — DSP · Phase 2.6 (Hardening: QUAL-01 / QUAL-02 / PERF-01 / PERF-02 / PERF-03, infinite tail, 96 / 192 k smoke) — Plan

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.6 of 7 (last Stage 2 phase)
**GSD phase:** plan
**Date:** 2026-11-06
**Branch:** `main` (trunk-based) @ `62cda45b`
**Inputs:**
- `CONTEXT-2.6.md` (D21–D25 locked; constraints: no feature, no parameter change, no module change, latency 0)
- `RESEARCH-2.6.md` (R79–R103). **Where it differs from ARCHITECTURE.md, ROADMAP.md or CONTEXT-2.6, RESEARCH-2.6 wins**, as amended by the sign-offs and P79–P96 below.
- `SUMMARY-2.5b.md`, `VERIFICATION-2.5b.md` (harness 388/388 in 267 s; N3 / N2 / N1 / N0 `--strict` + R12; C6 15.77 % execute / 15.30 % verify; W2o +0.483 dB, margin 0.017 dB; the 2.6 carry list)
- `ROADMAP.md` Phase 2.6 test list; `REQUIREMENTS.md` QUAL-01, QUAL-02, PERF-01, PERF-02, PERF-03 (all `partial`); `research/ARCHITECTURE.md` (CPU budget 30 %, 110 MB ring at 192 k)
- Read at plan (tree == `454c8a31`): `PluginProcessor.h/.cpp`, `CaptureWriter.h`, `Engine.h/.cpp`, `ModMatrix.h`, `Spawner.h`, `FeedbackChain.h`, `Analysis.h`, `GrainFx.h`, `EngineConstants.h`; harness `engine_gates.h` (render, O1, M1, R3), `_22b.h` (K1, X1), `_24b.h` (FZ2, `kappa`), `_25a.h` (`withTransport`, `renderAll`), `_25bb.h` (QB1, N9, M6, Z6, C6), `main.cpp`; modules grain-engine 1.1.0 (`GrainPool.h`, `BandlimitedReader.h`), reverb-unit 1.2.0 (`ReverbUnit.h`), pitch-collections 1.0.0 (`tryEdo`)
- `ref-2.6/` — the research prototypes and logs, copied at plan from the ephemeral scratchpad (table below)

### Reference prototypes (`stages/2-dsp/ref-2.6/`)

Copied at plan from `/private/tmp/claude-501/…/19d8d1ef-…/scratchpad/r26/` (sources and logs only; no build dirs, no binaries). This plan and RESEARCH-2.6 are normative; the prototypes are the measured reference implementations. Absolute scratch paths inside them are historical.

| File | Use |
|---|---|
| `cpu.cpp` | C7 reference: `maxAll` config (`:151-175`), `makeStim (fs, 10, 0.052, 0)` = the R84 stimulus (the code is a pure 220 Hz sine + 10 kHz bursts; the `:46-54` header comment "harmonic tone" is stale), `timeConfig` (processBlock-only Σ `steady_clock`, `CLOCK_THREAD_CPUTIME_ID`, p50 / p99 / max, liveness deltas) |
| `probe.cpp` | Census (`:57-93`), member-access idiom (`:32-44`), host NaN / +Inf probe (`:171-208`), FTZ on / off decay (`:210-278`, `Engine::process` direct), block-0-armed alloc probe (`:101-141`), ring bytes / RSS / prepare (`:142-163`) |
| `proj-CMakeLists.txt.ref`, `tail-proj-CMakeLists.txt.ref` | Out-of-tree build recipe (`ouaricon_add_processor_console`, hooks on / off; the `-tailprobe` VST3) |
| `tail-PluginProcessor.h.diff` | The scratch D21 one-liner (compiled without `#include <limits>` only because `JuceHeader.h` pulls it in) |
| `sweep1.log`, `quick.log`, `spread.log`, `blocks.log` | R82–R87 CPU figures (21-config sweep; N = 9 × 10 s spread + rates; block-size sweep) |
| `probe.log`, `decay.log`, `probe-build.log` | R96 (NaN 1 030 727 / +Inf 1 024 875 non-finite samples, tank 32 656 / 31 960); R81 / R93 (FTZ on: exact 0 at 295.63 s; FTZ off: ring subnormal at 246 s, limit cycle 2.4e-42) |
| `tail-pluginval.log` | R79: pluginval 10, `Reported taillength: inf`, SUCCESS |

---

## Sign-offs (Taylor, 2026-11-06, at plan)

| # | Item | Decision |
|---|---|---|
| 1 | **R96** — fix scope | **Ring-write guard + wet guard.** `CaptureWriter` writes 0 when `! (|in·g| <= kAnalysisTapMax)` (the `Analysis.h:800` predicate), plus a finite guard on the wet bus before `reverb.process`. Class-scoped per D25. Bitwise neutral ≤ +80 dBFS; no module change. New Z7h rows (NaN and +Inf, L and both channels) with a guard-off negative control that reproduces the R96 poisoning. Extends QUAL-02 to host input. |
| 2 | **R83** — C7 config | **Accepted.** C7 = D22 with **crush 1 %**; the literal all-max config (crush 100 %) is an INFO row. |
| 3 | **R84** — C7 routing + stimulus | **Accepted.** No route on `input_gain` (route 13 → `s1_level`). Stimulus 220 Hz sine 0.2 + 3 ms Hann 10 kHz bursts 0.8 every 52 ms. Liveness: events ≥ 15/s, MPM confident ≥ 95 % of hops, steals ≥ 500/s, live ≥ 128. |
| 4 | **R80** — AU fallback | **(b) 600 s everywhere**, applied only if auval or pluginval 10 AU rejects ∞ at the T1 checkpoint. The executor applies it without stopping and records a deviation. |
| 5 | **R85** — sustained-tone onset deafness | **Accepted as a Stage 3 DAW watch item.** No R60 change in 2.6. |
| 6 | **R93** — QUAL-02 "no subnormal" | **Accepted:** shipped FTZ path census == 0 **plus** an FTZ-off control arm (`Engine::process` direct) that MUST show subnormals. The ≥ 330 s decay row (Z7d) runs under `--long` / in the background. |

---

## Entry Check — contract checksums

Re-hash all six at the start of execute (`shasum -a 256`) against the STATUS.md frontmatter: brief `f9c748a8…`, requirements `5f481caa…` (the 2.5b-verify revision), param-spec draft `d0b1457e…`, param-spec `f5ce3595…`, architecture `f0cf753b…`, roadmap `1060dae0…`. Before T0 the executor confirms:
- `git diff --stat 454c8a31 HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt` is empty (checked at plan @ `62cda45b`: empty). `Source/` at HEAD is the verified 2.5b tree, byte for byte.
- `git diff --quiet 454c8a31 HEAD -- modules scripts/param-dump` is clean (checked at plan). **2.6 changes no module.**
- Stale comments (verified at plan; T1 / T2 fix them): `PluginProcessor.h:63-64` ("P7: stays 0 in 2.1. Phase 2.6 sets the real tail …"), `PluginProcessor.h:35` ("Stage 2 (Phases 2.1–2.2)"), `Engine.h:46` and `EngineConstants.h:33` ("Phases 2.1–2.5b"), harness banner `main.cpp:787` ("2.1–2.5b").

---

## Goal

Close the five open requirements across the whole feature set built in 2.1–2.5b, and nothing else:
- **QUAL-01:** a click sweep over every source × destination × route edit (trajectory full cross + audio κ on every application path), stream enable / mute / solo, freeze, steal / kill and **burst steal / kill in a capped pool**, all under the full feature set, each against a same-motion reference with a failing negative control.
- **QUAL-02:** extreme max-side and min-side stress, a seeded full-parameter fuzz at 4 rates, a census of every persistent state (incl. ring, diffuser, `fbDelay`, reverb tank), the FTZ-off non-vacuity arm, and the **R96 host non-finite input fix**.
- **PERF-01:** one whole-feature allocation gate armed from block 0.
- **PERF-02:** C7 = the measured D22 argmax at 48 k / 128 ≤ 30 % of one core; `grain_limit` deterministic at 4 caps.
- **PERF-03:** a full-feature seeded null at 32 / 441 / 512 / 4096 / var plus the oversized host block (8192 on a fresh instance prepared at 512 and at 1).
- **D21:** `getTailLengthSeconds()` = +∞, validated through auval / pluginval 10 / HA on the installed binaries before any other Source task.
- 96 / 192 k smoke: hop log load-independent, every frequency coefficient ≤ 0.45 fs, ring bytes == the documented figure.

At neutral settings nothing moves: D21 is audio-neutral, the R96 guards are bitwise-neutral for |in·g| ≤ 1e4 (+80 dBFS), every new hook defaults off. **Every carried row diffs empty against A″ at every task; N3 / N2 / N1 / N0 `--strict` + R12 keep passing. No audio re-baseline is planned.**

At the end of 2.6: QUAL-01, QUAL-02, PERF-01, PERF-02, PERF-03 complete (stage-2); Stage 2 closes at verify; `DAW-2.6.md` joins DAW-2.5a / DAW-2.5b in the Stage 3 DAW pass; the plugin installs and passes auval, pluginval 10 VST3 + AU and HA.

---

## Plan-phase decisions

- **P79 — D24 baseline A″ first, on the unmodified tree (T0).**
  - HEAD == `454c8a31` for `Source/`, `tests/`, `CMakeLists.txt` (Entry Check). Fresh `ninja O-Granulator-render-harness`.
  - Harness 388/388, run **twice** in the background → `${TMPDIR:-/tmp}/ogran-26/harness-A2-1.txt`, `-2.txt`. The carried extract (sorted `[PASS]` / `[FAIL]` lines, INFO and timing rows dropped — C2–C6 detail values move run to run) is identical between the two → **A″**.
  - N3 / N2 / N1 / N0 `--strict` + R12 rc 0 (background, logs kept).
  - Static baselines for T11, counted on non-comment lines at HEAD: heap / thread tokens in `Source/dsp`; `ring.push` (2, `CaptureWriter.h:116, 125`); `reverb.process` (1, `Engine.cpp:1476`); `kAnalysisTapMax` uses (1 use `Analysis.h:800` + 1 definition `EngineConstants.h:200`); playhead call sites (1, `PluginProcessor.cpp:284`).
  - No commit (nothing changes).
- **P80 — D21 tail: the tracer and the R80 checkpoint (T1).**
  - `PluginProcessor.h`: `#include <limits>`; `getTailLengthSeconds()` returns `std::numeric_limits<double>::infinity()`. Replace the P7 comment (`:63-64`) with the D21 rationale: freeze holds content forever; fb 100 % reaches exact 0 only at 295.6 s under FTZ (R81); JUCE maps ∞ to VST3 `kInfiniteTail` (R79, `juce_audio_plugin_client_VST3.cpp:3486`) and AU `TailTime` ∞ (R80, `AU_1.mm:1202`). Refresh the `:35` banner (comment only).
  - Audio-neutral: carried diff empty, N-suites unchanged.
  - **The tracer:** one Source line taken end to end through the installed binaries — TA1 row → `build-and-install.sh` → auval → pluginval 10 VST3 + AU → HA — before any other Source task.
  - **R80 fallback (sign-off 4).** "Rejects" = auval ends in anything but `AU VALIDATION SUCCEEDED`, or prints an error / warning in its `Tail Time` section; or pluginval 10 AU is not `SUCCESS`. Then apply **(b)**: `return 600.0;` with a comment citing R81 (exact 0 at 295.6 s), **without stopping**; re-run TA1 (expects 600.0), auval, pluginval VST3 + AU and HA; record the deviation with the verbatim output. A warning that still ends in SUCCEEDED keeps ∞; record its text.
  - No test treats ∞ as a failure (R79: pluginval only logs it; pedalboard never reads it).
- **P81 — R96 fix, class-scoped (sign-off 1, D25) (T3).**
  - **Class** (non-finite entry points, RESEARCH §6.4): host → analysis tap (`Analysis.h:800`, guarded); host → env (`ModSources.h:268` scrub); grain SVF (P25 NaN kill); fb return (`FeedbackChain.h:190-197` scrub); matrix offsets (`ModMatrix.h:236`); parameter latch (`ControlGrid.h` `isfinite`); tracker (scrub); **host → ring (`CaptureWriter.h:116-117, 125`: none)**; **wet → reverb tank (`Engine.cpp:1476`: none, module)**; dry → out (the host's own sample, out of scope).
  - **Ring-write guard**, both push statements. Predicate = the tap's: `! (std::abs (in·g) <= kAnalysisTapMax)` (catches NaN, ±Inf and absurd finite in one branch; 2.5b deviation 2).
  - **Contraction-safe form (plan correction of RESEARCH §6.4).** Per channel, select the input operand in its own statement — `xl = bad ? 0.0f : inL[i]` (likewise `xr`) — and keep each push statement's arithmetic **verbatim**, with `xl` / `xr` in place of `inL[i]` / `inR[i]`. The predicate's product is a separate statement. Do **not** write RESEARCH's `x = in·g; push x + fb`: FP contraction is per statement (`Engine.cpp:1446-1447`, PLAN-2.4 P36), so splitting `in·g + fb` un-fuses it and moves every carried row. With `xl = 0`, `0·g + fb == fb` exactly.
  - `CaptureWriter::write` returns the number of guarded samples (`int`) and takes a trailing `bool sanitise = true`. Engine passes `! hooks.hostGuardOff` in test builds and nothing otherwise; `diag.hostScrubs += n` (test builds).
  - **Wet guard** (defence in depth): in `mixOut`, after the stream-mix loop and before `revView.setDataToReferTo` / `reverb.process` (`Engine.cpp:1464-1476`), per sample per channel `if (! std::isfinite (wet[ch][i])) { wet[ch][i] = 0.0f; ++n; }`, skipped when `hooks.wetGuardOff`; `diag.wetScrubs += n`. It runs whether or not the reverb is active (the knee passes NaN by design, `OutputLaw.h:186`). The tank itself is untouched (no module change). Cost ≈ 64 `isfinite` per cell.
  - Comments: `CaptureWriter.h` header formula gains the guard; `EngineConstants.h:200` `kAnalysisTapMax` → "host-input sanitiser: analysis tap (P61) and ring write (R96)".
  - **Outside the signed scope, INFO only:** the env follower takes a finite 1e30 as a level (`ModSources.h:262-263`; only non-finite state is scrubbed), so the Env source pins at 1 for its release (finite, self-recovering). Z7h3 prints it. Taylor decides at verify whether the env input gets the same predicate (bitwise neutral ≤ +80 dBFS).
- **P82 — Hooks and accessors (hooks-only; defaults off → A″ and the N-suites identical) (T2).**
  - `noDepthRamp` — a `ModMatrix` member like `noResidual` (`ModMatrix.h:301`), copied in `matrixTick` (`Engine.cpp:811`). On: a route change resets the slot depth to `want.depth`, and a depth edit snaps (`reset (want.depth)`). Off: the existing `s.depth.reset (0.0f)` / `setTarget` / `tick` statements verbatim (else-branch).
  - `stealFadeMs` (double, −1 = off): test builds call `pool.prepare (fs, hooks.stealFadeMs >= 0.0 ? hooks.stealFadeMs : kPoolFadeMs)` (`Engine.cpp:42`). 0 → `fadeStep = 1` (`GrainPool.h:105-108`): a hard cut on a native steal.
  - **`killFadeMs` (double, −1 = off) — new, not in RESEARCH (P89).** `Spawner::prepare (double sampleRate, double killMs = kKillFadeMs)` (`Spawner.h:140-145`); test builds pass `hooks.killFadeMs >= 0.0 ? … : kKillFadeMs`. 0 → `killStep = 1`: a hard cut on an R3 kill. `freezeDeferCells` keeps `kKillFadeMs`.
  - Both fade hooks are set in `Job::pre` (before `prepareToPlay`).
  - `getFbDelayForTest()` → `const float (&)[2][kFbDelay]`; `getReverbForTest()` → `const ReverbUnit&` (the `Engine.h` test section).
  - **Reverb privates via the explicit-instantiation member-access idiom** (`ref-2.6/probe.cpp:32-44`), harness-side only: `ReverbUnit::tankDelays`, `tankFilters`, `diffusionDelays`, `diffusionState`, `preDelayL`, `preDelayR`, `DelayLine::buffer`, `OnePole::z`. Engine privates through the two accessors, not the idiom. **No module change.**
  - **FTZ-off render path**, harness-side: a helper that calls `Engine::process (in, numIn, out, numOut, n, seed, ts)` directly, outside `processBlock`'s `ScopedNoDenormals` (R93). Before the run it asserts an FTZ probe on the calling thread (`volatile float a = 1e-30f, b = 1e-10f;` `a·b` classifies `FP_SUBNORMAL`).
  - T3 adds `hostGuardOff`, `wetGuardOff`, `Diag::hostScrubs`, `Diag::wetScrubs` (P81). Every new TestHooks / Diag field sits inside `#if OGRAN_TEST_HOOKS`; `Diag::reset()` covers the new counters.
- **P83 — Census (R93, R94) (T2 helper).**
  - Per state: subnormal count and non-finite count. Runs after `processBlock` returns (outside `ScopedNoDenormals`, so classification is not flushed).
  - **States:** grains (all `kPoolLive + kPoolFade` slots: SVF ic1 / ic2, `crushHeld`, `ringRe` / `ringIm`, `killGain`, `readAbs` (double); `eg23b::anySubnormal` plus the rest); fb chain HP / LP integrators + `env`; diffuser `line[2][4][kDiffuserMaxLen]` (`FeedbackChain.h:241`); ring levels 0–2 via `levelSample` (the last 1 s per level per scan, the full 41 s at render end); `fbDelay[2][32]`; reverb tank lines, tank filter `z`, diffusion lines + state, `preDelayL` **and `preDelayR`** (`probe.cpp` scanned L only); analysis (detector HP / fast / slow, decimator history, window, + the Z6a list: NSDF scratch, guard, slew); env state; LFO ×4 values; matrix residuals d = 1…112; sources v[1…11].
  - Scans every 1 s of render time (block-aligned, in `after`) and at render end.
- **P84 — C7, the PERF-02 gate (D22 as amended by sign-offs 2 and 3; R83, R84, R86, R87) (T4).**
  - **Config** = `ref-2.6/cpu.cpp` `maxAll` + `crush 1 %` ×4 + `mod13_dest = 5`:
    - 4 streams: density 200, size 2000, spray / position / size_rand / jitter 100, pitch +24, pitch_rand 24, reverse_prob 100, window 1, window_skew +100, stereo_spread / gain_rand 100, filter LP, cutoff 20 k, filter_rand 100, res 100, drive 100, **crush 1 %**, crush_prob 100, ring_freq 5 k, ring_rand 100, ring_mix 100, level +6, scale Major, onset_trigger On, pitch_follow On.
    - grain_limit 128; buffer_length 30; mix 100; input_gain 0; onset_threshold −45; Free; seed `0xC2C2`.
    - feedback 100, fb_low_cut 2 k, fb_high_cut 20 k, fb_diffusion 100, fb_saturation 100; reverb_mix / size / damp 100.
    - LFO 1–4 at 20 Hz, shapes 2–5; macros 1–4 at 100.
    - 16 routes ±100 % (alternating sign), sources 1–11 all present, dests per `cpu.cpp:170-172` with **route 13 → `s1_level` (dest 5)**. No route on `input_gain` (R84).
  - **Stimulus (R84):** 220 Hz sine 0.2 + 3 ms Hann-gated 10 kHz bursts at 0.8 every 52 ms, both channels, a precomputed 10 s buffer looped.
  - **Method (R86):** a dedicated `std::thread` that first sets `pthread_set_qos_class_self_np (QOS_CLASS_USER_INTERACTIVE, 0)`, then builds the processor and calls `prepareToPlay (48000, 128)`. A playing 120 bpm MockPlayHead is attached; `timeInSamples` moves between blocks (untimed). Exact 128-sample blocks. Warm-up 5 s. **N = 7 runs × 10 s.** t_run = Σ `steady_clock` around `processBlock` only; c_run = `CLOCK_THREAD_CPUTIME_ID` over the run.
  - **Verdict:** median (t_run / 10 s) ≤ 30 % → PASS. Median > 30 % → **phase fail** (stop, investigate, D25). A single run > 30 % with the median ≤ 30 % prints a FLAG, not a fail (CONTEXT).
  - **Duty witness:** a run counts only if c_run / t_run ≥ 0.9. A contended run is replaced (≤ 3 extra). Fewer than 7 valid runs → `[NOVERDICT] C7 contended — re-run`, no row, harness rc 2. **Never FAIL on contention**; acceptance needs a verdict.
  - **C7L liveness** (hooks counters over the 70 s): events ≥ 15/s, confident ≥ 95 % of hops, steals ≥ 500/s, `maxAlive == 128` and ≥ 128 active live slots at the end of warm-up. Measured (R84, hooks on): 19/s, 8437/8437, 834/s, 132.
  - **INFO:** min / max / spread; per-block p50 / p99 / max (last run). `--long`: the literal D22 config (crush 100 %, route 13 → input_gain) N = 3; 44.1 / 96 / 192 k, N = 3 × 10 s each (R87: 15.3 / 33.2 / 67.3 %; 192 k p99 518 µs of a 667 µs budget); % per `grain_limit` (N = 3).
  - Expected median 16.7–16.9 % (margin ≈ 13 pp). Hooks on vs off ≤ 0.5 pp (R82); block size 32–1024 ≤ 0.2 pp (R86).
  - C7 runs alone: harness sections are sequential, and no build or N-suite runs concurrently (shared checkout: another session's build shows up in the witness).
- **P85 — `grain_limit` determinism, GL (R98) (T4).**
  - The C7 config at caps 32 / 64 / 96 / 128 (choice 0–3), 10 s at 48 k, block 128, the C7 stimulus. Spawn logs through `eg22::collect` (the Diag ring holds 4096; 10 s ≈ 8400 spawns).
  - **GL1:** `diag.maxAlive ≤ cap` and a per-block scan of non-killing live slots ≤ cap (the R3 row form, `engine_gates.h` R3 loop); kills > 0 at caps < 128, steals > 0 at 128.
  - **GL2:** two renders with the same seed: spawn logs (`onsetSample`, `slot`, `stolen`, `killed`, `globalIndex`) and audio bitwise, at every cap. **GL2c:** seed + 1 differs (logs and audio).
  - Block-size invariance of the steal sequence is N10's.
- **P86 — QUAL-01 conventions (R89, R92; memory: same-motion reference, tonal source, curvature vs a noise channel) (T5–T7).**
  - Thresholds at precedent: **κ ≤ 2.0× a same-motion reference**; **trajectory ≤ 1.05× the analytic bound**. No row compares a moving subject against a static render.
  - κ = the existing step-click measure (`eg24b::kappa`, maxCurv / RMS over the window).
  - **Linear wet sum (R92):** mix 100; crush off (`crush_prob 0`); `reverb_mix 0` (reverb-on variants INFO); a tone on every channel; wet below the D9 knee T. Precondition on subject and reference: `diag.maxWetPeak < 0.5` (knee identity), else scale the input on both sides and record it — never the threshold. Default input: 440 Hz sine at 0.001 (the K1 convention: drive and saturation stay linear).
  - **Q set** = the full feature set for κ rows: the C7 config with `crush_prob 0`, `reverb_mix 0`, LFO rates 2 Hz, route depths ±30 % (20 Hz ±100 % routes would AM the carrier and blunt κ; each row's NC proves sensitivity).
  - Every NC must fail its gate **before** the subject's PASS is trusted (memory: stimulus below threshold).
- **P87 — MX3, trajectory half: the full cross (R90a) (T5).**
  - 11 sources × 112 destinations × **7 edit kinds**: (a) add None → src, (b) source → None, (c) dest → None, (d) depth → 0, (e) depth step +60 → −60, (f) dest swap, (g) source swap. RESEARCH folds (f) and (g) into one kind; they are separate code paths in `ModMatrix::tick` (`:198-209`), so both get every source. ≈ 7 × 11 × 7 = 539 renders × 1 s.
  - Each render: 48 k, block 32; 16 slots carry one source to 16 distinct dests (112 = 7 × 16); every slot is edited at the same tick (0.5 s, cell-aligned). Removal kinds route from t = 0; add starts empty.
  - After every block: `getDestTelemetry (d)` for all 112 dests and `getSrcTelemetry (src)`.
  - **Bound per cell (normalised):** `|Δv01| ≤ 1.05 × (|depth|·max|Δsrc| over the cell ± 1 + |Δdepth| + |Δresidual|)`, the ramp terms from `ModMatrix.h:187-243`: a depth ramp moves `|c|/rampCells` per cell, a residual decays `|lastContrib|/rampCells` per cell. The [0, 1] clamp is 1-Lipschitz. Onset attack cells excluded (MX2 rule). If a row fails, re-derive the bound from the mechanism first; the 1.05 factor is fixed.
  - Source drives: LFO 1–4 at 2 Hz, sine / triangle / smooth random / S&H (S&H steps are the source's own and sit inside the bound, R51); Env on a 220 Hz tone with 125 ms swells; macros 1–4 host-ramped 0 → 100 % over 1 s (per-block `setP`); Pitch on 233 → 311 Hz steps; Onset on 2 ms bursts every 180 ms.
  - Base raw values at 50 % normalised (±depth stays out of the clamp); light audio (1 stream, density 10): the matrix runs regardless of audio load.
  - Cross-talk check in the same pass: every dest outside a render's 16 holds its telemetry constant.
  - Rows MX3a–g, one per edit kind over 11 × 112. **MX3nR:** `noResidual` on (b) exceeds 2× the bound in ≥ 1 cell for every source. **MX3nD:** `noDepthRamp` on (a) and (e) exceeds 2× the bound for every source.
- **P88 — MF8, audio half (R90b) (T6).**
  - The 23 application paths: ramped 14 (InputGain, OutputGain, Mix, Feedback, FbDiff, FbSat, Level ×4, Pan ×4; `Engine.cpp:581-656`) + tick 9 (FbLowCut, FbHighCut `:211-212`; RevMix, RevSize, RevDamp `:199-201`; LfoRate ×4 `:836`).
  - × {add, source → None, depth step +60 → −60} × {LFO1 sine 2 Hz, Onset} = 138 subjects + 46 held-route references (route present the whole render: pre-edit depth for remove / step, post-edit for add; the MF7 / MX2a form). Depth 100 % for add / remove.
  - Config: 4 streams (density 20, size 200 ms, spray 0, jitter 20, Hann), fb 50 % (a fed loop for Feedback / FbDiff / FbSat / FbLowCut / FbHighCut); LfoRateN gets a carrier route LFO_N → s1_level at 30 % held all render.
  - Stimulus: LFO runs — 440 Hz sine 0.001 both channels. Onset runs — the QB1 swell carrier (440 Hz, +18 dB 1 ms-attack swells every 125 ms) at base 0.001, `onset_threshold` −50; edits placed ≥ 30 ms from any event.
  - κ window [edit − 30 ms, edit + 50 ms] ≤ 2.0× the reference's κ in the same window.
  - Reverb dests (3): reverb on, κ is **INFO** (MF8i; R92 tank confound). Their gate is MX3.
  - Rows MF8a–c (LFO: add / remove / step) and MF8d–f (Onset), each over the 20 gated dests. **MF8n:** `noDepthRamp` add on Mix and s1_level (LFO) > 2.0×.
- **P89 — Edge rows: EN1, FZ9, K3, QB3 (R89, R91) (T6–T7).**
  - **EN1** — stream enable / mute / solo under the Q set with 2 streams; s2 edge at t1 = 2 s and back at t2 = 4 s. References: the same render with the edge state held "on" all render and held "off" all render. Verdict κ in [t − 30 ms, t + 50 ms] ≤ 2.0× max (κ_on, κ_off). EN1a enable (liveness: ≥ 1 s2 spawn within 20 ms of on), EN1b mute, EN1c solo (s1 solo toggled; s2 audibly muted by it). **EN1n:** the X1 hard-switch model (`out_off + step·(out_on − out_off)`, `engine_gates_22b.h:767-781`) > 2.0×.
  - **FZ9** — freeze under the Q set. **Renamed from RESEARCH §5.4 "FZ8"**: the carried 2.4 row "FZ8 release decay" holds that token. The FZ2 method (`engine_gates_24b.h:190-244`): engage at 2 s vs the unfrozen control, release at 5 s vs the still-frozen control, FZ2's windows. FZ9a engage, FZ9b release. **FZ9an** `freezeNoDefer` > 2.0×; **FZ9bn** `releaseSpliceOld` > 2.0×. Liveness: `freezeEngages` ≥ 1, `freezeReleases` ≥ 1.
  - **K3** — steal / kill under the Q set (4 streams, every FX but crush, follow + triggers on), the K1 method (`engine_gates_22b.h:607-683`): window 3–7 s; reference = 4 × 20 g/s × 1 s at cap 128 (0 steals / kills, spawns > 400). K3a: the same at cap 32 (R3 kills ≥ 50 in window). K3b: 4 × 50 g/s × 1 s at cap 128 (native steals ≥ 50). **K3an:** K3a with `killFadeMs 0` > 2.0×. **K3bn:** K3b with `stealFadeMs 0` > 2.0×. Routes on demand-changing dests (Size, SizeRand, Density, BufferLength) are retargeted to non-demand dests so the cap is the only difference.
  - **QB3 — burst steal / kill in a capped pool (R91, corrected at plan).** At cap 32 with demand in (32, 128) the over-cap path is the **R3 kill** (`Spawner::acquire`, `Spawner.h:497-503`; `killStep` from `kKillFadeMs`, `Spawner.h:142`), not a pool steal. `rec.stolen` stays 0 there and `stealFadeMs` never acts, so RESEARCH's liveness (`burst && stolen` ≥ 50) and NC would both be vacuous. QB3 therefore takes the K1a / K1b arms:
    - **QB3a** (kill path): triggers On ×4, cap 32, 4 × 20 g/s × 1 s (demand 80), Hann, the QB1 swell carrier; reference = the same at cap 128 (steal-free, the same spawn plan). Liveness: records with `burst && killed` ≥ 50 in window; reference 0 steals / kills. **QB3an:** `killFadeMs 0` > 2.0×.
    - **QB3b** (steal path): triggers On ×4, cap 128, 4 × 50 g/s × 1 s (demand 200 → native steals); reference = QB3a's steal-free render (the K1b form). Liveness `burst && stolen` ≥ 50. **QB3bn:** `stealFadeMs 0` > 2.0×.
    - Swell carrier scaled so events ≥ 20 and `maxWetPeak` < 0.5 (P86).
- **P90 — QUAL-02 (R93, R95, sign-off 6) (T8).**
  - **Criterion (sign-off 6):** the shipped path (`processBlock`, FTZ) census == 0, plus an FTZ-off arm (`Engine::process` direct) that MUST show subnormals.
  - **Common gate** (Z7a / Z7m / Z7f): every output sample finite; the Z5 / Z6a split gate (wet ≤ float (A); out ≤ its gain bound); census 0 subnormal and 0 non-finite at every 1 s scan and at the end; `fbScrubs`, `envScrubs`, `analysisScrubs`, `hostScrubs`, `wetScrubs` and `nanKills` == 0 (a scrub on finite input means a NaN was made inside); `reachViolations` == 0; `guardsIntact()`.
  - **Z7a — max composite, 4 rates** (44.1 / 48 / 96 / 192 k), one render each: 60 s stimulus (the C7 stimulus + `hashNoise` 0.25) + 30 s silence. The C7 config with two of the 16 routes moved onto `input_gain` and `output_gain` (one +100 %, one −100 %; then swapped at 30 s). Every 10 s an event flips: pitch +24 ↔ −24, reverse_prob 100 ↔ 0, fb_low_cut 20 ↔ 2000, fb_high_cut 1000 ↔ 20 000, reverb_damp 0 ↔ 100. Freeze toggled 6 times.
  - **Z7m — min-side hazards**, each 10 s at 48 k and 192 k + 10 s silence:
    1. filter_cutoff 20 + res 100, LP / BP / HP; + filter_rand 100 at cutoff 20 (g = 3.3e-4 at 192 k)
    2. ring_freq 20 + ring_rand 100 (carrier 5 Hz → clamp 20)
    3. buffer_length 1 s with size 2000, pitch +24, reverse 100, live and frozen (`A = max (span − floor, 0.5·r·len)`, the `Spawner.h` frozen path). Liveness: the reach clamp is taken
    4. size 5 ms × density 200 ×4 (spawn-bound) + grain_limit 32 under demand 400. Liveness: kills > 0 and steals > 0
    5. input_gain −24 dB on a −60 dBFS input; s_level −60 with gain_rand 100 (−84 dB grains) — the subnormal-adjacent levels
    6. env_attack 1 ms / env_release 10 ms, lfo_rate 0.01 Hz, onset_threshold −60 (fires on the floor)
    7. drive at the guard: `d = 16·drive²` just above and just below `kDriveGuard` 1e-4 (drive ≈ 0.26 % / 0.24 %; `driveLatchD` active with norm ≈ 1e4 on one side, inactive on the other, `GrainFx.h:104-111`); crush 1 %
    8. Custom `edo 64 1.001` with scale + follow On ×4: ActiveSet / AbsSet outward truncation at `kMaxActive` 512 (`wasTruncated` true). **`edo 64 1` does not parse** — `tryEdo` rejects a period ≤ 1 ¢ (`! (period > 1.0f)` → `badPeriod`, `PitchCollections.h:277`); 1.001 ¢ is the smallest round value that does (A4 resolved).
  - **Z7f — seeded full-parameter fuzz**, 4 rates: 64 states × 2 s per rate. Each state: all 220 parameters at seeded uniform normalised values (seed k), plus 8 mid-render jumps that re-draw 32 random parameters. Stimulus: the C7 stimulus + `hashNoise` 0.25. One row per rate. At 192 k at most 8 jobs in parallel (110 MB ring each). Memory: pluginval 10 found latent NaN in siblings that hand-picked extremes missed.
  - **Z7d (`--long`, background):** 48 k, the R81 config (`ref-2.6/probe.cpp:211-216`), 10 s of 0.5·sin 220 + 0.1·sin 3311, then silence to 340 s. FTZ on (`processBlock`): exact-zero output for ≥ 1 s before 330 s (R81: last non-zero 295.63 s) and census 0 at every 1 s scan. **Z7dn:** FTZ off (`Engine::process` direct), same input: census > 0 by 300 s (R93: ring 246 s, reverb 262 s) — the non-vacuity control; the FTZ probe asserted on that arm's thread. The two arms run in parallel.
  - Static (T11): `juce::ScopedNoDenormals` is the first statement of `processBlock` (`PluginProcessor.cpp:253`).
- **P91 — PERF-01, M7 (R97) (T9).**
  - **Armed from block 0:** set `armDuringProcess = true` before `render` (no arming in `after`). The M1 liveness probe first (a deliberate malloc counts exactly 1).
  - The C7 feature set at cap 128, 12 s. Two transports (Free; Tempo on the v2 mock with a tempo step, loop wrap, locate, stop / start, time-signature change) × two schedules (var: 1, 8192, …; constant 8192 with prepBlock 512 on a fresh instance, so block 0 is 8192) = 4 sequential renders on the main thread.
  - **Event script** (message side, between blocks): freeze ×4 (mid-cell); route add / source → None / dest → None / depth → 0 / depth step / dest swap / source swap, from each of the 11 sources; scale change, mask change, custom apply; `sync_mode` switches; seed change; reverb_mix 0 → 50 → 0 (the `clearTank` edge); onset_threshold sweep; grain_limit 128 → 32 (kills); **preset-like mass change** (all 220 parameters rewritten between two blocks); injections: fb NaN, env NaN, analysis-tap NaN, SVF NaN kill, host NaN (a stimulus sample).
  - Rows: **M7L** liveness 1; **M7F** Free, both schedules, 0 allocations; **M7T** Tempo, 0 allocations; **M7c** counters live — spawns, steals, kills, onsetEvents, mpmHops, followHits, bursts, routeChanges, tempoReanchors / tempoRelocks (T), fb / env / analysis / host scrubs, nanKills, freezeEngages / freezeReleases, customCopies, each ≥ 1.
  - Counts allocations (`malloc_logger`, render thread), not buffer addresses (memory).
  - M5 / M6 stay as carried rows; M7 is the one-scope PERF-01 gate (memory: a gate inherits the review finding's scope).
- **P92 — PERF-03, N10 (R99, R100) (T9).**
  - Config: the C7 feature set with feedback 80, **crush kept on at 50 %** (plan amendment: RESEARCH §9 drops crush without a stated reason; a bitwise null has no tolerance for crush to amplify, so dropping it only removes coverage — R92's crush rule is for κ rows), + N9's input, events and transport, + route edits, + a `reverb_mix → 0` edge and re-enable (R100). N10F Free; N10T Tempo (v2 mock: tempo step, loop, locate). 12 s at 48 k.
  - Blocks: 32 (reference), 441, 512, 4096, var, **8192 on a fresh instance prepared at 512**, 8192 on a fresh instance prepared at 1. First event ≥ 8192 (N9's is at 48017), so block 0 really is 8192 (memory: stale host instance).
  - Each: value compare max|Δ| == 0 vs 32 (R35); the event list, hop schedule and f0 ring snapshot identical; `guardsIntact()`. Liveness: events, bursts, confident hops, follow hits, steals, re-anchors (T) and ≥ 1 reverb running → idle edge.
  - **N10c:** seed + 1 differs. **N10n:** `perBlockOrigin` (Tempo) differs at 441.
  - **O2:** the O1 host-guard poison test (`engine_gates.h:803-845`) with the C7 feature set: prepare 512, blocks 8192 / 4096 alternating; finite, host guard poison intact, canaries, reach 0, spawns > 500.
- **P93 — 96 / 192 k smoke (R101, R102) (T10).**
  - **HR1:** at 96 k and 192 k, the hop log under the full C7 config == the same input with every stream disabled (hop samples, notes, clarity, confident flags, bitwise). Input: the QB1 swell carrier on 220 Hz — **no 10 kHz bursts at 192 k** (R101: the 64-tap decimator's transition band is wide at D = 16). Liveness: confident ≥ 90 % of hops, events > 0. The MP1h watch (worst 0.76 ¢) is load-independent by construction (the tap is the conditioned input, `Analysis.h:792`).
  - **HR2:** at 44.1 / 48 / 96 / 192 k, every spawn record's `fcHz` and `ringHz` (post-clamp, `Spawner.h:94-95`) ≤ (float)(0.45·fs). At 44.1 k, max `fcHz` == 19 845 (the clamp binds: non-vacuity; if `ringHz` doesn't bind, print its max as INFO). fb_high_cut 20 k: the chain's LP coefficients == `tptCoeffs (min (20000, 0.45·fs), …)` recomputed (`FeedbackChain.h:57-59`). Config: filter_cutoff 20 k + rand 100, ring_freq 5 k + rand 100, fb_high_cut 20 k, feedback 50.
  - **HR3:** ring bytes = `8 · Σ_{l=0..2} (getCapacity (l) + kTaps)` == `8 · Σ ((ceil (41·fs) >> l) + 64 + 16)` = **25 315 320 / 27 553 920 / 55 105 920 / 110 209 920** at 44.1 / 48 / 96 / 192 k (`BandlimitedReader.h:298`; matches `EngineConstants.h:68` "27.6 MB @ 48 k, 110 MB @ 192 k"); `kRingSamples (fs) == ceil (41·fs)`. INFO: prepare ms fresh / repeat, RSS delta (R101: 7.9 / 1.2 ms, +111.6 MB at 192 k).
  - Finiteness and census at 96 / 192 k come from Z7a / Z7m / Z7f.
- **P94 — Harness files, flags, counts (T1–T10).**
  - New: `tests/render-harness/engine_gates_26.h` (TA1, Z7s / Z7h / Z7a / Z7m / Z7f / Z7d, HR; the census, the FTZ-off helper, the reverb idiom, the C7 config / stimulus builders, the Q set) and `engine_gates_26b.h` (C7, GL, MX3, MF8, EN1, FZ9, K3, QB3, M7, N10, O2). **Carried row files are not edited.**
  - `main.cpp`: `--only26` (2.6 sections only; the iteration aid) and `--long` (adds Z7d / Z7dn and the C7 INFO extras), parsed anywhere in argv and combinable (`--long` alone = full + long). Existing flags unchanged. Banner "2.1–2.6". rc 0 ALL PASS, 1 any FAIL, 2 C7 no verdict (no FAIL).
  - New tokens (whole-token match, dropped from the carried diff): `TA1 Z7s Z7h1–5 Z7hn Z7hr Z7a Z7m1–8 Z7f Z7d Z7dn C7 C7L GL1 GL2 GL2c MX3a–g MX3nR MX3nD MF8a–f MF8n EN1a–c EN1n FZ9a FZ9b FZ9an FZ9bn K3a K3b K3an K3bn QB3a QB3b QB3an QB3bn M7L M7F M7T M7c N10F N10T N10c N10n O2 HR1–3`. Checked at plan against the carried tokens (FZ1–FZ8, G1–G6, K1a/b/n, K2*, MX1/2, MF1–7, M1–6, N1–N9, O1, C6, Z1a–Z6b, QB1, …): no equal token.
  - **Estimate: 76 new rows (74 default + 2 `--long`) + ≈ 10 INFO lines → 462 default / 464 with `--long`.** Record the actual.
  - Wall: ≈ 9 min default (267 s + C7 75 s + Z7a ≈ 60 s + the rest ≈ 2 min), ≈ 15 min with `--long` (+ Z7d ≈ 60 s, C7 extras ≈ 4.5 min). **Every full run, every `--long` run, Z7d, the C7 extras and every N-suite run in the background with logs** (memory: 600 s watchdog). The full run executes alone (C7).
- **P95 — Carried rows, attribution, no-change rules.**
  - A″ is the reference at every task. **No re-baseline planned** (D21 audio-neutral; R96 bitwise ≤ +80 dBFS; hooks default off).
  - Any carried move → stop, bisect to the statement, attribute (D24). A D25 fix that moves neutral audio needs Taylor's sign-off at verify; record the moved rows and the new baseline A‴ with its cause.
  - W2o (+0.483 dB, margin 0.017 dB): if it flips, widen its seeds. **Never** the threshold.
  - No parameter change (220 LOCKED: `GranulatorParamIds.h` and `tests/golden` unchanged); latency 0; no module change (a module defect → stop and ask before any version bump).
  - HA unchanged (no new parameter): HA1 79/79, HA2 142 untouched, HA3 79/79.
- **P96 — Stage 3 DAW additions (D23: no LISTENING-2.6.md) (T12).**
  - `DAW-2.6.md` items: (1) ∞ tail in Logic / Live / Reaper (R80, R81); (2) drums over a sustained pad (R85, sign-off 5); (3) 192 k CPU (R87). Recipes in T12.
  - They join DAW-2.5a (items 1–8) and DAW-2.5b (items 1–5) in the Stage 3 pass. Execute doesn't wait.

---

## Source layout (deltas against 2.5b)

| File | Change |
|---|---|
| `Source/PluginProcessor.h` | D21: `#include <limits>`; `getTailLengthSeconds()` → `std::numeric_limits<double>::infinity()` (fallback (b): `600.0`); the `:63-64` comment; the `:35` banner (P80) |
| `Source/dsp/CaptureWriter.h` | R96 ring-write guard in both push statements, contraction-safe operand select; `write()` → `int` (guarded count), trailing `bool sanitise = true`; header comment (P81) |
| `Source/dsp/Engine.h` | TestHooks: `noDepthRamp`, `stealFadeMs`, `killFadeMs`, `hostGuardOff`, `wetGuardOff`. Diag: `hostScrubs`, `wetScrubs` (+ `reset()`). Accessors: `getFbDelayForTest()`, `getReverbForTest()`. Banner `:46` "2.1–2.6" |
| `Source/dsp/Engine.cpp` | Test-build `pool.prepare` / `spawner.prepare` fade arguments (P82); `matrix.noDepthRamp` copy in `matrixTick`; the `CaptureWriter::write` return + `sanitise`; the wet guard in `mixOut` before the reverb (P81) |
| `Source/dsp/ModMatrix.h` | `noDepthRamp` member + its branch in `tick` (P82) |
| `Source/dsp/Spawner.h` | `prepare (double sampleRate, double killMs = kKillFadeMs)` (P82) |
| `Source/dsp/EngineConstants.h` | `:200` comment (host-input sanitiser); banner `:33` "2.1–2.6". No new constant |
| `tests/render-harness/main.cpp` | `--only26`, `--long`, rc 2, two `#include`s + section calls, banner "2.1–2.6" (P94) |
| `tests/render-harness/engine_gates_26.h`, `engine_gates_26b.h` **(new)** | P94 |
| `stages/2-dsp/DAW-2.6.md` **(new)** | P96 |

No change: `modules/`, `scripts/param-dump`, `GranulatorParamIds.h`, `tests/golden`, `tests/hostcache/run.py`, `tests/n0–n3`, `tests/r12`, every carried `engine_gates*.h`.

---

## Tasks

Build commands (from `build/`, Release, `OUARICON_BUILD_TESTS=ON`): `ninja O-Granulator-render-harness`, then `ninja OuariconGranulator_VST3 OuariconGranulator_AU`. The harness binary is `build/plugins/O-Granulator/O-Granulator-render-harness_artefacts/Release/O-Granulator-render-harness`; rc 0 = ALL PASS, rc 2 = C7 no verdict (re-run).

**Run every full harness, every `--long` run and every out-of-tree script (`tests/n3`, `n2`, `n1`, `n0`, `r12`) in the background with a log** (memory: 600 s watchdog). `--only26` is the iteration aid; every acceptance uses the full run.

**The carried-row diff:** sorted `[PASS]` / `[FAIL]` lines, dropping the P94 tokens (whole token, not a leading letter — carried rows start with `C`, `F`, `G`, `K`, `M`, `N`, `O`, `Q`, `Z`) and the INFO / timing rows, compared against **A″** (T0). It must be empty at every task. A non-empty diff is a regression: stop and bisect.

**N-suite set** = `tests/n3/run.sh --strict`, `tests/n2/run.sh --strict`, `tests/n1/run.sh --strict`, `tests/n0/run.sh --strict`, `tests/r12/run.sh`; all rc 0.

### Wave 0 — the precondition (P79)

**T0. Baseline A″ on the 454c8a31 tree (D24)**
- Files: none. **No `Source/` or `tests/` change.**
- Entry Check; fresh harness build; harness twice (background) → A″; N-suite set (background); record the static baselines (P79).
- Acceptance: harness **388/388** both runs; the two carried extracts identical; N-suite set rc 0; W2o +0.483 dB.
- No commit.
- Depends on: none.

### Wave 1 — D21 through the installed binaries (tracer + the R80 checkpoint)

**T1. Infinite tail; TA1; harness scaffolding; install + auval + pluginval 10 VST3 / AU + HA**
- Files: `Source/PluginProcessor.h` (P80); `tests/render-harness/main.cpp` (`--only26`, `--long`, rc 2, banner, include + section call); `tests/render-harness/engine_gates_26.h` (new: section scaffold + TA1).
- **TA1:** a fresh processor reports `std::isinf (getTailLengthSeconds()) && getTailLengthSeconds() > 0` (fallback: `== 600.0`), before and after `prepareToPlay (48000, 512)`; `getLatencySamples() == 0`.
- Harness (background) → **389/389**; carried diff vs A″ empty; N-suite set rc 0.
- `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep, AU cache clear).
- **R80 checkpoint, before any other Source task:**
  - `auval -v aufx OuGn OuDv` → `AU VALIDATION SUCCEEDED`; read its `VERIFYING PROPERTY: Tail Time` section and record it.
  - `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate-in-process --validate ~/Library/Audio/Plug-Ins/VST3/O-Granulator-dev.vst3` → SUCCESS, log `Reported taillength: inf`.
  - The same for `~/Library/Audio/Plug-Ins/Components/O-Granulator-dev.component` → SUCCESS.
  - **HA:** `uv run --no-project --python 3.12 --with pedalboard --with numpy plugins/O-Granulator/tests/hostcache/run.py` (repo root, the installed VST3) → HA1 79/79, HA2 142 untouched, HA3 79/79.
  - Any AU rejection → fallback (b) per P80, applied without stopping; re-run TA1 / auval / pluginval ×2 / HA; deviation recorded.
- Acceptance: TA1 PASS; 389/389; carried diff empty; N-suite set rc 0; 0 new warnings; auval + pluginval 10 VST3 + AU + HA PASS (with ∞, or with 600 s under the recorded fallback).
- Commit **alone**: re-check `git branch --show-current` + `git status --short`, then `git commit -- plugins/O-Granulator`.
- Depends on: T0.

### Wave 2 — plumbing, then the R96 fix

**T2. Hooks, accessors, census, FTZ-off path, builders (bitwise no-ops)**
- Files: `Engine.h`, `Engine.cpp`, `ModMatrix.h`, `Spawner.h`, `EngineConstants.h` (banner only) (P82); `engine_gates_26.h` (census P83, the reverb idiom, the FTZ-off helper + FTZ probe, `c7Params()`, `c7Stim (fs)`, `qSet()`), `main.cpp` (banner if not done at T1).
- **Z7s — census self-test + reach:** (a) `fpclassify (1e-40f) == FP_SUBNORMAL` on the census thread; (b) the scan primitive over a local array with one planted 1e-40f and one NaN reports exactly 1 / 1; (c) on a prepared engine after 2 s of the C7 render, every census state reports its scanned element count == its declared size (diffuser 2·4·`kDiffuserMaxLen`, `fbDelay` 64, ring 3 levels × 1 s × 2 ch at the level rate, tank / diffusion / pre-delay buffer sizes > 0, residuals 112, sources 11) and 0 / 0.
- Hook neutrality is proven by A″, not by a row: every hook defaults off.
- Acceptance: Z7s PASS; harness **390/390**; carried diff vs A″ empty; N-suite set rc 0; 0 new warnings; canaries hold.
- Depends on: T1.

**T3. R96: ring-write guard + wet guard (class fix); Z7h**
- Files: `CaptureWriter.h`, `Engine.h`, `Engine.cpp`, `EngineConstants.h` (`:200` comment) (P81); `engine_gates_26.h`.
- Hooks wired: `hostGuardOff`, `wetGuardOff`; Diag `hostScrubs`, `wetScrubs`.
- Z7h base config (`ref-2.6/probe.cpp:176-177`): s1 50 g/s × 200 ms, spray 50, position 5, mix 50, feedback 50, reverb_mix 30; **no route, no trigger, no follow** (the analysis and env see a scrubbed sample differently from a zero sample by design; neither reaches audio without them). 220 Hz sine 0.2, 12 s, injection at `at = fs + 5`. Rates 44.1 / 48 / 96 / 192 k × channel modes {L only, L + R, mono bus (inCh 1, L = R pointer)}. Reference: the same render with the injected sample(s) set to 0.
  - **Z7h1 NaN / Z7h2 +Inf / Z7h3 1e30:** every output sample finite from `at + 1`; output bitwise == the reference for every sample > `at`; end-of-render census non-finite == 0 in ring, tank, fb chain, diffuser, `fbDelay`; `hostScrubs` == injected ring-write samples (L 1, L + R 2, mono 2); `wetScrubs`, `fbScrubs`, `nanKills` == 0. Z7h3 prints the INFO line on the Env source (P81: cells pinned at 1 after 1e30 vs the reference).
  - **Z7h4 release-crossfade path:** freeze on at 2 s, off at 4 s; NaN on L at the release tick + 100 samples (inside the 10 ms crossfade, `CaptureWriter.h:88-120`); `fz.xfading` true at the injection (read in `after`); the Z7h1 gates.
  - **Z7h5 full feature:** the C7 config at 4 rates; NaN on L at 1 s, +Inf on R at 3 s, 1e30 on both at 5 s; finite from each `at + 1`, end census non-finite 0, `hostScrubs` == 4 per rate. No bitwise claim (routes, triggers and follow legitimately see the scrub).
  - **Z7hn (NC, both guards off):** Z7h1 L NaN at 48 k reproduces R96 — non-finite output at the render end, tank non-finite > 0, ring non-finite > 0 (`ref-2.6/probe.log`: 1 030 727 samples, tank 32 656).
  - **Z7hr (ring guard off, wet guard on):** output finite from `at + 1`; ring non-finite > 0; `wetScrubs` > 0; tank non-finite == 0 (the wet guard's reach, recorded).
- Acceptance: Z7h1–5 and Z7hr PASS; Z7hn reproduces R96; harness **397/397**; **carried diff vs A″ empty** (the guard is bitwise for every carried stimulus: none feeds a non-finite or > 1e4 host sample, checked at plan); N-suite set rc 0; 0 new warnings. A carried move here means the push statement shape changed: restore the verbatim arithmetic (P81), never re-baseline.
- Commit T2–T3 (path-scoped, after the re-check).
- Depends on: T2.

### Wave 3 — PERF-02 (`engine_gates_26b.h`; no `Source/` change)

**T4. C7 CPU gate; `grain_limit` determinism**
- Files: `engine_gates_26b.h` (new), `main.cpp` (include + section call).
- **C7 / C7L** per P84; **GL1 / GL2 / GL2c** per P85. `--long` extras: C7 literal all-max, C7 at 44.1 / 96 / 192 k, % per cap (INFO).
- Run the full harness alone (no concurrent build / N-suite). `--long` once in the background for the INFO extras.
- Acceptance: C7 median ≤ 30 % with a verdict (expected 16.7–16.9 %); C7L liveness met; GL1 / GL2 PASS, GL2c differs; INFO printed (min / max / p99; `--long`: all-max, rates, per cap); harness **402/402**; carried diff empty. A median > 30 % is a phase fail: stop and investigate (D25).
- Depends on: T3.

### Wave 4 — QUAL-01 (`engine_gates_26b.h`; no `Source/` change)

**T5. MX3 trajectory full cross**
- MX3a–g, MX3nR, MX3nD per P87 (≈ 539 renders, `renderAll`).
- Acceptance: MX3a–g PASS across 11 × 112 per kind with the cross-talk check; MX3nR and MX3nD exceed 2× the bound; harness **411/411**; carried diff empty.
- Depends on: T4.

**T6. MF8 audio κ on every application path; EN1 enable / mute / solo**
- MF8a–f, MF8n, MF8i (INFO) per P88; EN1a–c, EN1n per P89.
- Acceptance: MF8a–f ≤ 2.0× on all 20 gated dests; MF8n > 2.0×; MF8i printed; EN1a–c ≤ 2.0×; EN1n > 2.0×; every `maxWetPeak` < 0.5; harness **422/422**; carried diff empty.
- Depends on: T5.

**T7. FZ9 freeze, K3 steal / kill, QB3 burst kill / steal (full feature)**
- FZ9a / FZ9b / FZ9an / FZ9bn, K3a / K3b / K3an / K3bn, QB3a / QB3b / QB3an / QB3bn per P89.
- Acceptance: subjects ≤ 2.0×; every NC > 2.0×; liveness met (FZ9 engages / releases ≥ 1; K3a kills ≥ 50, K3b steals ≥ 50; QB3a `burst && killed` ≥ 50, QB3b `burst && stolen` ≥ 50; references clean); harness **434/434**; carried diff empty.
- Commit T4–T7 (path-scoped).
- Depends on: T6.

### Wave 5 — QUAL-02, PERF-01, PERF-03, high-rate smoke (no `Source/` change unless D25)

**T8. Z7 extreme stress, min-side hazards, fuzz, FTZ decay**
- Files: `engine_gates_26.h`.
- Z7a ×4, Z7m1–8, Z7f ×4 (default); Z7d, Z7dn (`--long`, background) per P90.
- Acceptance: Z7a / Z7m / Z7f PASS on the P90 common gate with their liveness; Z7d exact zero before 330 s and census 0; **Z7dn census > 0 by 300 s**; harness **450/450** default, **452/452** `--long`; carried diff empty.
- Depends on: T7.

**T9. M7 whole-feature allocation gate; N10 block null + oversized host block; O2**
- Files: `engine_gates_26b.h`.
- M7L / M7F / M7T / M7c per P91; N10F / N10T / N10c / N10n / O2 per P92.
- Acceptance: M7L == 1, M7F / M7T 0 allocations, M7c all counters ≥ 1; N10F / N10T max|Δ| == 0 at all 6 variants with events / hops / ring identical and canaries; N10c and N10n differ; O2 PASS; harness **459/459**; carried diff empty.
- Depends on: T8.

**T10. 96 / 192 k smoke**
- Files: `engine_gates_26.h`.
- HR1 / HR2 / HR3 per P93 (+ INFO prepare / RSS).
- Acceptance: HR1 hop logs bitwise at 96 and 192 k with liveness; HR2 every `fcHz` / `ringHz` ≤ 0.45·fs and the 44.1 k bind; HR3 bytes == the four figures; harness **462/462** default, **464/464** `--long`; carried diff empty.
- Commit T8–T10 (path-scoped).
- Depends on: T9.

### Wave 6 — validate + docs

**T11. Static checks**
- Grep non-comment lines only, against the T0 baselines (`grep -v '^\s*//'` before counting):
  - `ring.push` in `Source/` → 2, both in `CaptureWriter.h`, both taking `xl` / `xr`; `reverb.process` → 1, preceded by the wet guard in `mixOut`.
  - `kAnalysisTapMax` → the T0 count + the ring-write uses (tap + ring + definition).
  - `getTailLengthSeconds` returns `std::numeric_limits<double>::infinity()` (or the recorded 600.0 fallback); `#include <limits>` present.
  - `juce::ScopedNoDenormals` is the first statement of `processBlock`.
  - `std::vector|new |malloc|make_unique|std::function|std::thread|std::async` in `Source/dsp` → no increase over T0.
  - `->setValue|setValueNotifyingHost|beginChangeGesture` in `Source/dsp` → 0.
  - Playhead call sites in `Source/` → still exactly one (`processBlock`); `setLatencySamples (0)` untouched.
  - Every new TestHooks / Diag field and both new accessors inside `#if OGRAN_TEST_HOOKS` (read the `git diff 454c8a31 -- plugins/O-Granulator/Source`).
  - `git diff --stat 454c8a31 -- modules scripts/param-dump plugins/O-Granulator/Source/GranulatorParamIds.h plugins/O-Granulator/tests/golden` empty.
- Depends on: T10.

**T12. Build, install, validate; HA; DAW sheet**
- Harness ALL PASS (record the count, ≈ 462) and `--long` (≈ 464), 0 new warnings; carried diff vs A″ empty; N-suite set rc 0.
- `./scripts/build-and-install.sh O-Granulator`. `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10 VST3 + AU, in-process, on the installed bundles (randomised parameters now meet the R96 guards and the ∞ tail; memory: pluginval-10 catches a latent NaN). HA as T1.
- Write `stages/2-dsp/DAW-2.6.md` — per item a session recipe (Logic / Live / Reaper; the generic editor, or the WebView UI once Stage 3 lands) and what to listen / look for:
  1. **∞ tail (R80, R81):** freeze on and fb 100 % in Logic, Live and Reaper. Bounce with "include tail" / tail options: the bounce ends at the user's range (no hang, no endless render). Stop the transport and let the region end: the tail isn't cut (the host never idles the plugin). Note the CPU while stopped. A hang or a cut reopens D21 (fallback (b) 600 s, sign-off 4).
  2. **Drums over a sustained pad (R85):** onset_trigger on a drum loop mixed with a held harmonic pad. Expected: hits under the pad are missed (the R60 re-arm never fires; documented). Taylor decides whether to revisit R60 (a behaviour change, out of 2.6).
  3. **192 k CPU (R87):** a 192 kHz session with a C7-like preset at 128 and 256 buffers: the meter near ⅔ of a core; listen for dropouts.
- These join DAW-2.5a (items 1–8, watch item 6) and DAW-2.5b (items 1–5) in the Stage 3 DAW pass. Execute does not wait on the DAW.
- Depends on: T11.

**T13. Docs + state**
- Write SUMMARY-2.6.md: the gate table with measured values (C7 median / min / max / p99, liveness; the INFO rates); N-suite set; HA; auval / pluginval incl. the Tail Time text; the A″ attribution (empty, or each move with its cause); the errata below; carries to Stage 3 / 3.x; execution deviations.
- STATUS.md → `stage_2_phase_2_6_execute_complete` (next: verify). REQUIREMENTS stays untouched until verify.
- Commit path-scoped after the re-check: `git commit -- plugins/O-Granulator`.
- Depends on: T12.

---

## Dependency graph

```
T0 ─ T1 ─ T2 ─ T3 ─ T4 ─ T5 ─ T6 ─ T7 ─ T8 ─ T9 ─ T10 ─ T11 ─ T12 ─ T13
(A″) (D21 tracer, (hooks)(R96) (C7/GL) └─ QUAL-01 ─┘ └ Q2 / P1 / P3 / HR ┘ └─ validate ─┘
      R80 ckpt)
```

7 waves (0–6), 14 tasks, strictly sequential. T1 must land and validate on the installed AU before any other Source task (R80). T2–T3 are the only other Source tasks. T4–T10 author rows only, but they share `engine_gates_26.h` / `engine_gates_26b.h` / `main.cpp`, and C7 must run on a quiet machine, so they stay sequential for clean diffs. A D25 fix inside T4–T10 re-runs the full harness and the N-suite set before the task closes.

---

## Gate criteria (new rows)

76 new harness rows (74 default + 2 `--long`) + ≈ 10 INFO lines, plus the T1 / T12 validation (auval, pluginval 10 VST3 + AU, HA) and the T11 static checks. INFO: C7 spread / p99, C7 all-max, C7 rates (3), C7 per cap (`--long`), MF8i, Z7h3 Env pin, HR3 prepare / RSS, Z7d timing.

| Row | Measures | Pass | Negative control | Req | Task |
|---|---|---|---|---|---|
| TA1 | tail, latency | +∞ (fallback 600.0); latency 0 | — | D21 | T1 |
| auval / pluginval 10 / HA | installed AU + VST3 with ∞ | SUCCEEDED / SUCCESS ×2 / HA1 79, HA2 142, HA3 79 | — | D21, COMPAT-01 | T1, T12 |
| Z7s | census self-test + reach | planted 1 / 1 seen; sizes == declared; 0 / 0 | — | QUAL-02 (R94) | T2 |
| Z7h1–3 | host NaN / +Inf / 1e30, L / LR / mono, 4 rates | finite from at+1; bitwise == zero-sample ref after at; census non-finite 0; hostScrubs exact | Z7hn | QUAL-02 (R96) | T3 |
| Z7h4 | injection inside the release crossfade | as Z7h1 | — | QUAL-02 (R96) | T3 |
| Z7h5 | full feature, 3 injections, 4 rates | finite from each at+1; census 0; hostScrubs 4 | — | QUAL-02 (R96) | T3 |
| Z7hn | both guards off | **reproduces R96** (non-finite to the end; tank > 0) | — | QUAL-02 | T3 |
| Z7hr | ring guard off, wet guard on | output finite; wetScrubs > 0; tank 0 | — | QUAL-02 | T3 |
| C7 / C7L | CPU, argmax config, 48 k / 128, N = 7 × 10 s | median ≤ 30 %; witness ≥ 0.9; events ≥ 15/s, conf ≥ 95 %, steals ≥ 500/s, live ≥ 128 | literal all-max (INFO) | PERF-02 (D22, R83, R84) | T4 |
| GL1 | live ≤ cap at caps 32 / 64 / 96 / 128 | maxAlive and scan ≤ cap; kills / steals > 0 | — | PERF-02 (R98) | T4 |
| GL2 / GL2c | same-seed determinism per cap | spawn logs + audio bitwise | seed + 1 differs | PERF-02 (R98) | T4 |
| MX3a–g | 11 sources × 112 dests × 7 edit kinds, trajectory | ≤ 1.05× the analytic bound; no cross-talk | MX3nR, MX3nD > 2× | QUAL-01 (R90a) | T5 |
| MF8a–f (+MF8i) | 20 gated paths × {add, remove, step} × {LFO, Onset} | κ ≤ 2.0× held-route ref | MF8n > 2.0× | QUAL-01 (R90b) | T6 |
| EN1a–c | enable / mute / solo, Q set | κ ≤ 2.0× max (on, off) | EN1n > 2.0× | QUAL-01 | T6 |
| FZ9a / FZ9b | freeze engage / release, Q set | κ ≤ 2.0× unfrozen / still-frozen | FZ9an, FZ9bn > 2.0× | QUAL-01 | T7 |
| K3a / K3b | R3 kill / native steal, Q set | κ ≤ 2.0× steal-free ref; ≥ 50 events | K3an (killFadeMs 0), K3bn (stealFadeMs 0) > 2.0× | QUAL-01 | T7 |
| QB3a / QB3b | burst kill (cap 32) / burst steal (cap 128) | κ ≤ 2.0×; `burst && killed` / `burst && stolen` ≥ 50 | QB3an, QB3bn > 2.0× | QUAL-01 (R91) | T7 |
| Z7a ×4 | max composite 90 s, 44.1 / 48 / 96 / 192 k | P90 common gate | — | QUAL-02 (R95) | T8 |
| Z7m1–8 | min-side hazards, 48 + 192 k | P90 common gate + item liveness | — | QUAL-02 (R95) | T8 |
| Z7f ×4 | 64 seeded full-param states × 8 jumps per rate | P90 common gate | — | QUAL-02 (R95) | T8 |
| Z7d / Z7dn | decay to 340 s, FTZ on / off (`--long`) | on: exact 0 before 330 s, census 0 | **Z7dn census > 0 by 300 s** | QUAL-02 (R93) | T8 |
| M7L / M7F / M7T / M7c | allocations armed from block 0, whole feature, var + fresh 8192 | 1; 0; 0; counters ≥ 1 | liveness | PERF-01 (R97) | T9 |
| N10F / N10T | block null 441 / 512 / 4096 / var / 8192 fresh (prep 512, prep 1) vs 32 | max\|Δ\| == 0; events / hops / ring equal | N10c seed; N10n perBlockOrigin | PERF-03 (R99, R100) | T9 |
| O2 | oversized host blocks, poison guards, full feature | finite, guards, canaries, reach 0 | — | PERF-03 | T9 |
| HR1 | hop log at 96 / 192 k, full load vs streams off | bitwise; confident ≥ 90 % | — | DSP-08 watch, smoke (R101) | T10 |
| HR2 | fcHz / ringHz / fb LP at 4 rates | ≤ 0.45·fs; 44.1 k binds | — | smoke (R102) | T10 |
| HR3 | ring bytes at 4 rates | == 25 315 320 / 27 553 920 / 55 105 920 / 110 209 920 | — | smoke (R101) | T10 |

---

## Errata for SUMMARY-2.6 (record at T13; ARCHITECTURE / ROADMAP / CONTEXT not edited — checksums)

1. **D21 / R79 / R80** ∞ tail (or the fallback (b) 600 s, if taken at T1, with the auval / pluginval text).
2. **R83** (signed off) C7 uses crush 1 %; crush 100 % is cheaper (it quantises at 1 kHz). D22's "everything maxed" survives as an INFO row.
3. **R84** (signed off) no `input_gain` route in C7 (route 13 → s1_level); the R84 stimulus.
4. **R86** C3w / C3t are the worst / typical configs, not wall / thread clocks (CONTEXT Q3 wording).
5. **R88** "reverb + shimmer + mod" is unreachable (fixed internals `EngineConstants.h:146-147`): max reverb = reverb_mix / size / damp.
6. **R93** (signed off) QUAL-02 "no subnormal" = FTZ-path census 0 + the FTZ-off control (Z7dn).
7. **R96** (signed off) latent defect fixed: ring-write + wet guards; QUAL-02 extended to host input. P81's contraction-safe form replaces RESEARCH §6.4's `x = in·g; push x + fb`.
8. **P87** MX3 splits "dest swap / source swap" into two kinds (7 kinds, ≈ 539 renders).
9. **P89** QB3 corrected to kill (cap 32) and steal (cap 128) arms with a new `killFadeMs` hook: R91's cap-32 subject never steals, so `burst && stolen` and `stealFadeMs` were vacuous there. FZ8 → FZ9 (token collision).
10. **P90** `edo 64 1` → `edo 64 1.001` (A4: `tryEdo` rejects a period ≤ 1 ¢).
11. **P92** N10 keeps crush (R99 dropped it without a reason).
12. **P83** the census adds `preDelayR` (the research probe scanned L only).
13. Any execution deviation with its measured reason, and the final harness counts (default / `--long`).

## Carried (not implemented in 2.6)

- **Stage 3 DAW pass:** DAW-2.5a.md items 1–8 (watch item 6) + DAW-2.5b.md items 1–5 + DAW-2.6.md items 1–3.
- **3.x UI copy:** "at 192 kHz the worst case uses about ⅔ of a core" (R87); the 2.5b items (Pitch source ≈ 75 ms lag / ≈ 150 ms to halfway; 40–1500 Hz; follow harmonises against the source note; R45 size-sync tooltip).
- **Decision at verify:** the Env 1e30 pin (P81 INFO) — same predicate at the env input or leave.
- **Decision after the Stage 3 DAW pass:** the R60 re-arm under sustained tones (R85).
- W2o rule unchanged: widen seeds, never the threshold.

---

## Risks / watch items

- **FP contraction breaks R96 neutrality.** The push statement must keep its arithmetic verbatim (P81). A carried move at T3 → restore and bisect; never re-baseline for this task.
- **C7 contention.** The shared checkout may have another session building. The witness discards contended runs; rc 2 = re-run on a quiet machine. Never accept a contended verdict, never FAIL on contention. If the median lands in 25–30 %, investigate before accepting (R83 expects 16.7–16.9 %).
- **auval with ∞ (A1).** Unverified until T1. Fallback (b) is pre-signed; record the auval text either way.
- **Vacuous negative controls.** MX3nR, MX3nD, MF8n, EN1n, FZ9an / bn, K3an / bn, QB3an / bn, Z7hn, Z7dn, N10c, N10n and GL2c must each fail their gate **before** the subject's PASS is trusted. If one doesn't, re-derive the stimulus and record it.
- **MX3 bound derivation.** A failure may be a missing term in the analytic bound (concurrent residual + depth ramp on a source swap). Re-derive from `ModMatrix.h:187-243` and document it; the 1.05 factor never changes.
- **192 k memory.** 110 MB per instance: cap 192 k jobs at 8 in parallel (Z7f, Z7m, Z7a, HR).
- **Module internals via the idiom.** The harness names reverb-unit 1.2.0 privates (`tankDelays`, `tankFilters`, `diffusionDelays`, `diffusionState`, `preDelayL/R`, `DelayLine::buffer`, `OnePole::z`). A module rename breaks the harness build, not the plugin. Accepted; no module change.
- **D25 defects.** Fix in-phase, class-scoped (sweep every site of the pattern, memory: a gate written to one instance prints green over its siblings), record a deviation; a neutral-audio change needs sign-off at verify. **A defect inside grain-engine or reverb-unit → stop and ask** (siblings consume them).
- **Harness wall** ≈ 9 / 15 min. Background + logs; don't thin the grids.
- **Carried C rows** (C2–C6) are timing lines, dropped from the carried diff; C6's ≤ 25 % ceiling keeps ample margin.
- **Env 1e30 pin** (P81): INFO only; not a QUAL-02 failure (finite, recovers).
- **Shared checkout:** path-scoped commits only; re-check branch and staging immediately before each commit. Other sessions' changes under `.claude/` and `plugins/O-GrainScatter/` are in the tree.
- **Ephemeral references:** the scratchpad may be gone; `ref-2.6/` holds the copies.

## Deviations protocol

- Record every deviation in SUMMARY-2.6 "Execution deviations" with the measured reason; Taylor reviews them at verify.
- Pre-authorised (record, don't stop): the R80 fallback (b); a stimulus re-derivation that makes a gate observable or a NC fail (scale both sides, never the threshold); a D25 class-scoped fix inside the plugin; MX3 bound re-derivations from the mechanism.
- **Stop and ask:** a module defect; any parameter change; a carried-row move without an attributable cause; C7 median > 30 %; a threshold change of any kind (κ 2.0×, trajectory 1.05×, C7 30 %, W2o).
- Neutral-audio changes (a carried row moves for a named reason) are signed off at verify, with A‴ recorded.

## Handoff

After this plan: `/clear`, then `/plugin-execute O-Granulator` (T0 first: baseline A″; T1 is the R80 checkpoint). Verify (`/plugin-verify O-Granulator`) closes Stage 2.

---

## Success criteria (gate for verify)

| # | Criterion | Rows / checks | Req | Task |
|---|---|---|---|---|
| 1 | Carried rows identical to A″ at every task; N3 / N2 / N1 / N0 `--strict` + R12 PASS; W2o unchanged | carried diff; N-suite set | COMPAT-01 (D24) | all |
| 2 | ∞ tail through the installed AU + VST3 (or the recorded 600 s fallback) | TA1; auval; pluginval 10 VST3 + AU; HA | D21 | T1, T12 |
| 3 | Route edits click-free under every source on every destination and application path | MX3a–g (+MX3nR, MX3nD); MF8a–f (+MF8n; MF8i INFO) | QUAL-01 | T5, T6 |
| 4 | Enable / mute / solo, freeze, steal / kill and burst steal / kill click-free under the full feature set | EN1a–c (+EN1n); FZ9a / b (+an / bn); K3a / b (+an / bn); QB3a / b (+an / bn) | QUAL-01 | T6, T7 |
| 5 | No NaN / Inf / subnormal under extreme, min-side and fuzzed settings at 4 rates; the census reaches every persistent state; FTZ is load-bearing and the census can see it | Z7s; Z7a ×4; Z7m1–8; Z7f ×4; Z7d + **Z7dn** | QUAL-02 | T2, T8 |
| 6 | A non-finite or absurd host sample can't poison the plugin | Z7h1–5; Z7hn reproduces R96; Z7hr | QUAL-02 (R96) | T3 |
| 7 | 0 allocations over the whole feature, armed from block 0, var + fresh 8192 | M7L, M7F, M7T, M7c; T11 heap grep | PERF-01 | T9, T11 |
| 8 | Worst case ≤ 30 % of one core at 48 k / 128 (median of 7, witness); `grain_limit` caps and is deterministic | C7, C7L (+INFO); GL1, GL2, GL2c | PERF-02 | T4 |
| 9 | Full-feature block null incl. the oversized host block on a fresh instance | N10F, N10T (+N10c, N10n); O2 | PERF-03 | T9 |
| 10 | 96 / 192 k: analysis load-independent, coefficients ≤ 0.45·fs, ring bytes as documented | HR1, HR2, HR3 (+ Z7a / Z7f at 96 / 192 k) | smoke (ROADMAP) | T10 |
| 11 | Static: guards at every ring push and before the reverb; no new heap / APVTS write; one playhead site; latency 0; modules / params / golden unchanged | T11 | PERF-01, COMPAT-01 | T11 |
| 12 | Install; auval; pluginval 10 VST3 + AU; HA; 0 new warnings; DAW-2.6.md ready | T12 | COMPAT-01, D23 | T12 |

**Requirement → gate rows:**

| Req | Closed by |
|---|---|
| **QUAL-01** No artifacts | MX3a–g, MX3nR, MX3nD · MF8a–f, MF8n (MF8i INFO) · EN1a–c, EN1n · FZ9a, FZ9b, FZ9an, FZ9bn · K3a, K3b, K3an, K3bn · QB3a, QB3b, QB3an, QB3bn — on top of the carried K1, X1, FZ2, QB1, MF7, MX2, ZQ2 |
| **QUAL-02** No NaN / denormal | Z7s · Z7h1–5, Z7hn, Z7hr · Z7a ×4 · Z7m1–8 · Z7f ×4 · Z7d, Z7dn (`--long`) · T11 `ScopedNoDenormals` first — on top of the carried Z1–Z6 |
| **PERF-01** RT safety | M7L, M7F, M7T, M7c · T11 heap / APVTS / playhead greps (M1–M6 carried) |
| **PERF-02** CPU + cap | C7, C7L (INFO: all-max, 44.1 / 96 / 192 k, per cap) · GL1, GL2, GL2c |
| **PERF-03** Block-size invariance | N10F, N10T, N10c, N10n · O2 (N8, N9, O1 carried) |
| **D21** Infinite tail | TA1 · auval + pluginval 10 VST3 + AU + HA at T1 and T12 · DAW-2.6 item 1 (Stage 3) |

Requirement status expected at verify:
- **Complete (stage-2):** QUAL-01, QUAL-02, PERF-01, PERF-02, PERF-03 — on harness evidence; the DAW items are Stage 3 confirmations.
- Stage 2 closes at 2.6 verify; next is Stage 3 (3.1).
