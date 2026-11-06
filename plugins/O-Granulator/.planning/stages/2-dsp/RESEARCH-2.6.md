# Stage 2 — DSP · Phase 2.6 (Hardening) — Research

**Plugin:** O-Granulator
**Stage / phase:** 2-dsp / 2.6 of 7 (last Stage 2 phase)
**GSD phase:** research
**Date:** 2026-11-06
**Tree:** `main` @ `32158704` (`Source/`, `tests/`, `modules/` unchanged since the 2.5b-verified `454c8a31`)
**Inputs:**
- `CONTEXT-2.6.md` (D21–D25 locked; seven open questions)
- `VERIFICATION-2.5b.md`, `SUMMARY-2.5b.md`, `PLAN-2.5b.md`; `ROADMAP.md` 2.6 test list; `REQUIREMENTS.md` QUAL-01/02, PERF-01/02/03; `research/ARCHITECTURE.md` (CPU risk, memory at 192 k, Constraints)
- Source: `Engine.cpp/.h`, `CaptureWriter.h`, `GrainFx.h`, `FeedbackChain.h`, `Analysis.h`, `ModMatrix.h`, `ModSources.h`, `TempoTracker.h`, `Spawner.h`, `Quotas.h`, `OutputLaw.h`, `PluginProcessor.cpp/.h`
- Modules: grain-engine 1.1.0 (`BandlimitedReader.h`, `GrainPool.h`), reverb-unit 1.2.0 (`ReverbUnit.h`)
- Harness: `engine_gates.h` (render, O1, M1), `engine_gates_22b.h` (K1, X1, M2), `engine_gates_23b.h` (C3, M3), `engine_gates_24b.h` (FZ2, Z3/Z4), `engine_gates_25ab.h` (MF7, M5), `engine_gates_25bb.h` (QB1, N9, M6, Z6, C6)
- JUCE 8.0.15 (`/Users/taylorbrook/JUCE`): `juce_audio_plugin_client_VST3.cpp`, `juce_audio_plugin_client_AU_1.mm`, `AU/AudioUnitSDK/AUBase.cpp`, `AUEffectBase.h`, `juce_VST3PluginFormatImpl.h`, `BREAKING_CHANGES.md`; VST3 SDK `ivstaudioprocessor.h`
- pluginval source (`Tracktion/pluginval` develop, `source/tests/BasicTests.cpp`, fetched with `gh api`); pedalboard source (`spotify/pedalboard` `ExternalPlugin.h`, `juce_PatchedVST3PluginFormat.cpp`)
- Memory: step-click same-motion control, grain-steal fade, curvature probe vs noise, distinct addresses ≠ allocation bound, stale host instance, oversampler oversized block, SC Nyquist clamp, gate inherits the finding's scope, stimulus below threshold, 600 s watchdog, `SmoothedValue::reset` NaN, pluginval-10 latent NaN, malloc_logger traps, absolute-slack / load-flaky timing, click confounds in a reverb

**Scratch experiments:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/19d8d1ef-4962-445a-8bf2-749962913243/scratchpad/r26/`
No file in the plugin tree or `modules/` was edited. Everything was built out of tree from the read-only sources (the n3 `make_proj` recipe). Nothing was installed.

| Item | What it does |
|---|---|
| `cpu/cpu.cpp` → `proj-cpuh` (`OGRAN_TEST_HOOKS=1`) and `proj-cpu0` (hooks compiled out, the plugin's code) | CPU sweep at 48 k / 128 over 21 configs. It times **only `processBlock`** (Σ `steady_clock` per block), with the input precomputed. It also records thread CPU time (`CLOCK_THREAD_CPUTIME_ID`), per-block p50 / p99 / max, liveness counters (spawns, steals, events, MPM confidence, live grains), and the 44.1 / 48 / 96 / 192 k cost, plus a 32–1024 block sweep |
| `cpu/probe.cpp` → `proj-probe` (hooks on) | Host NaN / +Inf input. Decay to exact zero with FTZ on (`processBlock`) vs off (`Engine::process` direct), with a per-state subnormal census: grains, fb chain, diffuser lines, ring levels 0–2, **reverb tank (via explicit-instantiation member access, no module change)**, analysis, `fbDelay`. Also an allocation probe with the **first block armed**, and ring bytes / RSS / prepare time per rate |
| `tail/` | `git archive HEAD` copy with `getTailLengthSeconds()` → `+∞`. Built as a VST3 only (product `O-Granulator-tailprobe`, never installed). Validated by path with pluginval strictness 10 |
| Logs | `quick.log`, `sweep1.log` (21-config sweep, hooks on and off), `spread.log` (N = 9 × 10 s, rates), `blocks.log`, `probe.log`, `decay.log`, `tail/pluginval.log` |

**Machine:** Apple M4 Max, macOS 26 (Darwin 25.6), Release, single render thread at default QoS. Load average was 3–7 during the runs. Duty cycle (user / real under `/usr/bin/time -p`) was 0.98–0.99 for every timing run.

All seven open questions are answered (§10). There are **twenty-five findings** (R79–R103). Five of them would make a 2.6 gate fail, or pass vacuously, if missed: **R83, R84, R93, R96, R97**. **R96 is a latent defect:** a single non-finite host sample poisons the output permanently.

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| R79 | **D21, VST3.** `getTailSamples()` maps `isinf → Vst::kInfiniteTail` (= `kMaxInt32u`). JUCE's own VST3 host maps it back to `+∞`. **Measured:** pluginval strictness 10 on a scratch ∞-tail VST3 prints `Reported taillength: inf` and ends **SUCCESS**. pluginval only logs the tail | D21 confirmed (VST3) | `return std::numeric_limits<double>::infinity();` (`#include <limits>`). No test treats ∞ as a failure |
| R80 | **D21, AU.** `GetTailTime()` returns `getTailLengthSeconds()` verbatim (`SupportsTail` true), so `kAudioUnitProperty_TailTime` = ∞. JUCE's AU derives from `MusicDeviceBase`, so `AUEffectBase::IsInputSilent`, which casts `sr·(latency + tail)` to `UInt32` (UB at ∞), is never reached. auval runs a "Tail Time" check. **Not verified with ∞**: that needs a registered (installed) AU | Risk LOW, unverified | **T1 checkpoint:** D21 → `build-and-install.sh` → `auval -v aufx OuGn OuDv` + pluginval 10 AU first, before any other task. The fallback, if auval rejects ∞, needs sign-off (§11) |
| R81 | **Measured tail** (D21 justification). D22-style config (fb 100 %, span 30 s, spray 100 %, reverb size 100): after the input stops, the output stays near full scale for about 30 s (the ring content is re-read), then falls about **2.6 dB/s**. It reaches −60 dBFS at about 60 s and **exact 0 at 295.6 s** (FTZ). No finite tail is honest | Confirms D21 | Cite in the D21 commit. Stage 3 DAW item: bounce / "include tail" behaviour with ∞ in Logic, Live and Reaper (R80 host behaviour is unverified) |
| R82 | **Cost structure (measured).** Cost ∝ **live grains** × per-grain-sample path. Once the pool saturates (128 live + 3–5 fading spares), density, pitch, reverse, window, scale, Tempo, feedback + reverb and the 16 routes each move the figure < 0.5 pp. Size 5 ms at 200 g/s × 4 (53 live) costs **6.5 %**. Bypass costs 0.33 %. FX add **+5.8 pp** on top of a 10.0 % bare read: drive +2.6, ring +2.0, SVF +1.6, crush@1 % +0.8, crush@100 % +0.4 | D22 argmax | The worst case is a saturated pool with all FX. Density is irrelevant once size × density ≥ 32 per stream |
| **R83** | **D22 "everything maxed" is not the argmax.** `crush` at 100 % holds at 1 kHz, so it quantises rarely. `crush` at 1 % quantises every sample and costs **+0.4–0.5 pp** more. Argmax measured: all max, **crush 1 %**, honest stimulus (R84) = **16.68 % (hooks off) / 16.85 % (hooks on)** of one core, N = 9 × 10 s, range 16.48–16.92. **Margin 13 pp** under the 30 % budget | **Gate under-measures if missed** (sign-off) | C7 config = D22 with crush 1 % (§3). Report the literal all-max config beside it as INFO |
| **R84** | **D22 stimulus.** A sustained *harmonic* tone keeps the R60 detector **disarmed**: fast/slow stays above +3 dB, so it never re-arms. Result: **0 events/s** with bursts on top (measured). An `input_gain` route makes events fire at the LFO rate instead and cuts MPM confidence to 73–83 %. **Honest stimulus:** pure 220 Hz sine at 0.2 + 3 ms Hann 10 kHz bursts at 0.8 every 52 ms (above the 4.8 kHz decimator band at 48 k). Result: **19–20 events/s** (refractory limit 20/s), **MPM 8437/8437 confident**, **832 steals/s** | **Vacuous worst case if missed** | C7 uses this stimulus and **no route to input_gain** (route 13 retargeted, sign-off). Liveness asserts events ≥ 15/s, confident ≥ 95 % of hops, steals ≥ 500/s, live ≥ 128 |
| R85 | The R60 edge trigger re-arms only when fast < 1.41·slow. A sustained, crest-rich tone (harmonics, or a saw: fast ≈ peak, slow ≈ mean |x|) holds fast/slow ≥ 1.41. **Onsets on top of a sustained pad are never detected.** A pure sine re-arms only marginally (fast min ≈ 0.85 A vs 1.41·0.64 A = 0.90 A) | INFO / DAW watch | No 2.6 change (it would be a behaviour change to signed-off R60). Add to the Stage 3 DAW pass: "drums over a sustained pad". Taylor decides whether to revisit (§11) |
| R86 | **CPU method (measured).** Wall vs thread CPU agree within 0.03 pp. Run-to-run spread is ±1 % relative (N = 9 × 10 s). Hooks on vs off differ ≤ 0.5 pp in either direction. Block size 32 → 1024 moves the figure ≤ 0.2 pp (16.34 / 16.36 / 16.38 / 16.53 / 16.53 %). **C3w / C3t are the *worst* / *typical* configs** (`engine_gates_23b.h:33-34`), not wall / thread | Method | N = 7 × 10 s after 5 s warm-up. Median verdict. Report min / max. Duty witness: thread / wall ≥ 0.9 per run, or "contended — re-run" (never FAIL). Time `processBlock` only, input precomputed, `prepareToPlay (48000, 128)`, a playing MockPlayHead attached |
| R87 | **High-rate cost (INFO, measured, argmax config).** 44.1 k 15.3 %, 48 k 16.6 %, **96 k 33.2 %**, **192 k 67.3 %**. At 192 k / 128 the per-block p99 is **518 µs of a 667 µs budget**, max 2261 µs (one OS spike) | INFO (D22) | Print it. Stage 3/4 copy: "at 192 kHz the worst case uses about ⅔ of a core" |
| R88 | **"Reverb + shimmer + mod" is unreachable.** The engine fixes `revMod = 0.2`, `revShimmer = 0.0` (`EngineConstants.h:146-147`, `Engine.h:654`). No parameter reaches either. The D22 / QUAL-02 "max reverb" is `reverb_mix / size / damp` | Erratum | Plans read "reverb_mix 100, size 100, damp {0, 100}". No shimmer row |
| R89 | **QUAL-01 coverage today.** K1a/b steal + kill (one config). X1 mute / solo / level / pan. FZ2a/b freeze (1 stream). MF7 / MX2 route **removal** on **mix only** (LFO / Env / Macro / Pitch / Onset). QB1 bursts. K2 FX steps. ZQ2 zipper. **Missing:** stream **enable** edges; route **add** and **depth step**; every destination other than mix; **burst steal in a capped pool** (QB1 runs `grain_limit` 128 with about 1.2 grains live: it never steals); every edge under the full feature set | Scope | §5 sweep |
| R90 | **QUAL-01 sweep fits the wall budget as a full cross**, in two halves. (a) **Trajectory half.** Every source × all 112 destinations × 6 edit kinds, read from the per-cell `destTelemetry` (normalised v01, written every tick for every dest) against the per-cell source telemetry. 16 slots carry 16 dests per render, so about **460 renders × 1 s**. (b) **Audio half.** κ on the **23 ramped + tick destination implementations** × {add, remove, depth step} under LFO sine and Onset, against the held-route reference | Design | Trajectory bound 1.05× (MF7 / MX2 precedent); κ ≤ 2.0× held-route (QB1 / MF7 precedent). Negative controls: `noResidual` (exists) and a new `noDepthRamp` hook |
| R91 | **Burst steal needs a capped, saturated pool and a steal-free twin.** Cap 32 vs cap 128 on the *same* spawn plan (demand between 32 and 128): only the cap differs, which is the K1b form. Liveness: spawn records with `burst && stolen` ≥ 50. Negative control: a test hook passing `fadeMs ≈ 0` to `pool.prepare` makes `fadeStep = 1`, a hard cut (`GrainPool.h:105-108`) | **QB1 is vacuous for steal** | New QB3 / QB3n rows |
| R92 | **Click verdicts need a linear wet sum.** Mix 100, wet below the D9 knee T (−6 dBFS), **reverb off** (the tank's LFO-modulated level and ringing confound κ), **crush off** (steps by design), tones on every channel | Gate design | Every new κ row. Reverb-on variants are INFO |
| **R93** | **The QUAL-02 subnormal census is structural under FTZ** (`ScopedNoDenormals` at `PluginProcessor.cpp:253`; arithmetic flushes). **Measured over 400 s:** FTZ on gave 0 subnormals in every state. FTZ off: every state goes subnormal at **246–271 s** (ring 168 000, reverb 49 192, diffuser 4 666, grains 136) and the output **limit-cycles at 2.4e-42 for ever**. FTZ is load-bearing. A census with no FTZ-off arm cannot show it can see anything | **Vacuous if missed** | Z7d decay row (48 k, fb 100 %, ≥ 330 s, background): FTZ on → exact 0 reached, 0 subnormals; **FTZ-off arm (`Engine::process` direct) must show subnormals** (the non-vacuity control) |
| R94 | **Census reach.** Already scanned: grains, chain HP/LP/env, analysis states. **Never scanned:** diffuser lines (public, `FeedbackChain.h:241`), the ring (`getRingForTest()` → `levelSample`), `fbDelay`, LFO / env / matrix residual / ramps, and the **reverb tank** (private in a module). The explicit-instantiation member-access idiom reads the tank **with no module change** (compiled and run in `probe.cpp`) | Gate design | Add `getFbDelayForTest()` / `getReverbForTest()` (hooks-only, plugin side) + the idiom for the module internals |
| R95 | **Extreme list** (max + min-side hazards + fuzz) | Gate design | §6.3. Includes a seeded full-parameter fuzz at 4 rates (pluginval-10 found latent NaN in siblings that hand-picked extremes missed) |
| **R96** | **LATENT DEFECT: one non-finite host sample poisons the plugin permanently.** Measured: one NaN on L at 1.000 s (mix 50, fb 50, reverb 30). Non-finite output from 1.226 s **to the end of the 12 s render** (1.03 M samples). Ring 160 non-finite. **Reverb tank 32 656 non-finite.** fb chain scrubbed 192 times and still re-fed. +Inf behaves the same. The ring holds it for 41 s; the tank never recovers (`clearTank` only on an enable edge or prepare) | **Defect (class)** | Fix at the entry: `CaptureWriter` writes 0 when `!(|in·g| <= kAnalysisTapMax)`. That is the analysis-tap predicate (`Analysis.h:800`), bitwise neutral for any input ≤ +80 dBFS. Optional wet-bus guard before `reverb.process`. New Z7h row. **Sign-off** (scope) |
| **R97** | **PERF-01: the first `processBlock` is never armed** in M1–M6 (each sets `armDuringProcess` in `after`). **Measured:** with block 0 armed, the full config (4 streams, cap 32, every FX, follow, triggers, 16 routes, frozen at load, mid-render unfreeze + Tempo switch + route removal) gives **0 allocations** at 512 and at 8192-first-after-prepare-512 (liveness 1). The static audit is clean | **Coverage gap** (no defect) | M7 arms from block 0, with one whole-feature scope (§7) |
| R98 | **`grain_limit` determinism** reuses R3 (`maxAlive ≤ cap`, scan) + D5 + S0, applied at the 4 caps under the C7 config | Gate design | G-rows: same seed → spawn logs + audio bitwise; non-killing live ≤ cap at every spawn; CPU per cap INFO |
| R99 | **PERF-03: nothing sizes from `maxBlockSize`** (Engine, reverb-unit, analysis, matrix, tracker, grain-engine, `processBlock`). `render()` already takes `prepBlock` + `block`. The var schedule already plays 8192 blocks after prepare 512 (`engine_gates.h:217`), **but never as a fresh instance's first block** (var starts with 1). O1 (`engine_gates.h:804-845`) is oversize with one stream and no null | Design | N10: the C7 feature set + the N9 events + route edits, block 32 / 441 / 512 / 4096 / var / **8192 fresh (prepBlock 512)** / 8192 fresh (prepBlock 1), bitwise vs 32, + canaries. MockPlayHead v2 needs no change |
| R100 | reverb-unit evaluates its idle transition per **call** (`ReverbUnit.h:186-195`), and calls start mid-cell under non-32 blocks. Analysis: the skipped samples differ only by `dry·1 + out·0` vs pass-through, which is bitwise equal because the wet bus is never −0 (it is filled +0, and `+=` cannot produce −0). Safe | Verified safe | N10 includes a `reverb_mix → 0` edge so the class stays covered |
| R101 | **192 k (Q7, measured).** `kRingSamples` 7 872 000. Ring **110 209 920 B (110.2 MB)**, matching the `EngineConstants.h:68` comment and ARCH. RSS +111.6 MB. Prepare **7.9 ms** fresh, 1.2 ms repeat. The analysis tap is the conditioned input only (`Analysis.h:792`, R74), so **the MP1h watch is load-independent by construction** | Q7 closed | Gate: hop log at 192 k under the full C7 config bitwise == the same input with streams disabled. Don't use the 10 kHz burst stimulus at 192 k (the 64-tap decimator's transition band is wide at D = 16) |
| R102 | **Nyquist class sweep: no defect.** Every frequency-derived coefficient clamps relative to fs: SVF `[20, 0.45 fs]` (`GrainFx.h:65`), ring carrier `min (f, 0.45 fs)` (`GrainFx.h:152`), fb TPT `[20, 0.45 fs]` (`FeedbackChain.h:59`), decimator `0.4·fs/D`. Onset HP is a fixed 150 Hz (FR0 covers 8–768 k). The reverb has no frequency coefficient | Verified | Smoke row: spawn-record `fcHz`, `ringHz` ≤ 0.45 fs at 4 rates (44.1 k binds: 20 k → 19.845 k) |
| R103 | **Wall budget.** The harness takes 267 s today. 2.6 adds about 3–4 min (C7 13 s + INFO rates 15 s, Z7 4 rates about 60 s, Z7d about 60 s, QUAL-01 about 20 s, N10 + M7 about 10 s, fuzz about 30 s). Total about 7–8 min: **beyond the 5-min foreground limit** | Process | `--only26` + `--long` (Z7d, C7 rates). Every full harness run goes in the background (memory: 600 s watchdog) |

---

## 1. D21: infinite tail (R79–R81)

### 1.1 VST3 (R79)

`juce_audio_plugin_client_VST3.cpp:3478-3489`:

```cpp
Steinberg::uint32 PLUGIN_API getTailSamples() override
{
    auto tailLengthSeconds = getPluginInstance().getTailLengthSeconds();
    if (tailLengthSeconds <= 0.0 || processSetup.sampleRate <= 0.0)
        return Vst::kNoTail;
    if (std::isinf (tailLengthSeconds))
        return Vst::kInfiniteTail;
    return (Steinberg::uint32) roundToIntAccurate (tailLengthSeconds * processSetup.sampleRate);
}
```

- `kInfiniteTail = kMaxInt32u` (`VST3_SDK/pluginterfaces/vst/ivstaudioprocessor.h:163-168`).
- JUCE's host side maps it back: `if (tailSamples == Vst::kInfiniteTail) return std::numeric_limits<double>::infinity();` (`juce_VST3PluginFormatImpl.h:2738-2741`).
- `BREAKING_CHANGES.md:4089-4106`: "getTailLengthSeconds can now return infinity for VST/VST3/AU/AUv3". The warning there is that hosts which allocate from the value may crash.
- **pluginval:** `BasicTests.cpp:42` only logs `"Reported taillength: " + String (getTailLengthSeconds())`. Nothing in `source/tests/*.cpp` asserts on the tail (grep of all six files).
- **Measured:** `tail/pluginval.log` — pluginval 1.0.4, `--strictness-level 10 --validate-in-process`, seed 0xca6cde, on the scratch ∞ build:
  - `Reported taillength: inf`
  - every section ran (audio processing, state, automation, fuzz, buses)
  - **SUCCESS**, rc 0
- **pedalboard (the HA gate):** `ExternalPlugin.h` contains no reference to the tail. The patched VST3 host returns ∞ (`juce_PatchedVST3PluginFormat.cpp:2800-2815`), and nothing reads it. **HA is unaffected.**

### 1.2 AU (R80)

- `juce_audio_plugin_client_AU_1.mm:1201-1202`: `SupportsTail() { return true; }` and `GetTailTime() { return juceFilter->getTailLengthSeconds(); }`.
- `AUBase.cpp:546-548` writes `GetTailTime()` into the property as-is. Hosts therefore read `kAudioUnitProperty_TailTime = +∞`.
- `AUEffectBase::IsInputSilent` (`AUEffectBase.h:122-126`) computes `static_cast<UInt32> (sr·(latency + tail))`, which is UB at ∞. It is **unreachable**: `JuceAU` derives from `ausdk::MusicDeviceBase` (`AU_1.mm:131-132`), and the wrapper clears the silence flag itself (`AU_1.mm:1485`).
- auval prints `VERIFYING PROPERTY: Tail Time` (`auval_current.txt`, current install, tail 0: PASS). What it checks is not documented (strings only).
- **∞ was not run through auval.** auval sees only registered components, and the rules forbid installing a scratch AU.
- `[ASSUMED]` Logic / GarageBand treat ∞ as "never idle the plugin"; "include tail" in a bounce honours the user's length. **Stage 3 DAW item.**

### 1.3 Measured tail (R81)

`decay.log`. Config: 4 streams, 200 g/s × 2 s, spray 100 %, span 30 s, pitch ±12, LP 2 kHz res 100, fb 100 % + diffusion 100 + sat 100, reverb 100 / size 100, mix 100. Input: 10 s of 220 + 3311 Hz, then silence:

| t (s) | 20 | 40 | 60 | 80 | 120 | 200 | 300 | 320 |
|---|---|---|---|---|---|---|---|---|
| peak \|out\| over the previous 20 s (dBFS) | +2.1 | −2.3 | −60.5 | −121 | −227 | −435 | −691 | **0 (exact)** |

The last non-zero sample is at **295.63 s**. The slope after the ring content ages out is about 2.6 dB/s. Any finite tail either truncates a 30 s plateau, or pads every bounce by minutes. D21 stands.

---

## 2. CPU cost structure (R82, R86)

All figures are % of one core at 48 k / 128 and are **measured**. `sweep1.log`: N = 5 × 5 s per config, 4.5 s warm-up, hooks-on column, harmonic tone + 10 kHz bursts, routes include `input_gain`.

| Config (D22 literal = 4 streams, density 200, size 2000, every Float at max, crush 100, LP res 100, 16 routes ±100, follow + triggers ×4, fb 100 + diff + sat, reverb 100) | % | live | steals/s |
|---|---|---|---|
| bypass (streams off) | 0.32 | 0 | 0 |
| C6-like (2.5b C6 recipe, density 16) | 15.15 | 126 | 26 |
| **D22 literal** | 15.57 | 131 | 833 |
| D22, crush 1 % | **16.09** | 131 | 833 |
| D22, window 0 (closed-form Hann, `cos` per sample) | 15.73 | 131 | 833 |
| D22, density 16 | 15.70 | 129 | 268 |
| D22, **size 5 ms** (spawn-bound, 839 spawns/s) | **6.45** | 53 | 0 |
| D22, pitch 0 rand 0 / reverse 0 / scale Off | 15.73 / 15.99 / 15.93 | 131 | 833 |
| D22, fb 0 + reverb 0 | 15.78 | 131 | 833 |
| D22, no routes / triggers + follow off / Tempo 1/64T | 16.11 / 16.07 / 15.87 | 130–132 | 344–800 |
| D22, **no FX** | **10.03** | 131 | 833 |
| no FX + SVF / + drive / + crush 100 / + crush 1 / + ring | 11.54 / **12.19** / 10.41 / 10.81 / 11.98 | 131 | 833 |

Hooks off (`proj-cpu0`, same sweep) shifts every row by −0.2 … +0.6 pp. For example, D22 literal is 16.06 and no FX is 10.24. **The hooks build is representative within 0.5 pp.**

**Reading:**
- Per live grain the cost is about 0.116 % at both 53 and 131 live grains. **Spawn cost is negligible**: 839 spawns/s at 6.45 %.
- The kernel cost is per grain-sample and constant:
  - 16-tap kernel lerp + 2× `dot16` + `wrap` (an int64 `%` per read, `BandlimitedReader.h:395-399`)
  - window
  - FX
- Pitch, level and reverse don't change it.
- The FX ranking follows the per-sample transcendental and branch count:
  - drive: `tanh` × 2 per sample (`GrainFx.h:94, 229-236`)
  - ring: rotation + Newton step
  - SVF: TPT, 2 channels
  - crush: `nearbyint` + divide, only on a hold boundary. `inc = (1000/fs)^crush` (`GrainFx.h:132`), so crush 100 % quantises at 1 kHz and crush ≈ 0 every sample.

**Method data (R86):**
- Wall Σ `processBlock` vs thread CPU: ≤ 0.03 pp apart on every row.
- Spread: `spread.log`, N = 9 × 10 s, D22 literal 15.58–16.12 (median 15.91).
- Block size (`blocks.log`, argmax config):

| Block | % | p99 µs (budget) |
|---|---|---|
| 32 | 16.34 | 126 (667) |
| 64 | 16.36 | 245 (1333) |
| 128 | 16.38 | 469 (2667) |
| 512 | 16.53 | 1820 (10 667) |
| 1024 | 16.53 | 3603 (21 333) |

The engine always runs 32-sample cells, so "honest 128" needs only exact 128-sample blocks: the figure is flat in block size.

**C6 harness method notes:** `timeConfigStim` (`engine_gates_25bb.h:1164-1204`) times the per-sample `std::function` stimulus fill inside the measured loop and attaches no playhead. Both effects are < 0.05 pp. C7 should still time `processBlock` only and attach a playing MockPlayHead.

---

## 3. D22: the argmax and its stimulus (R83, R84, R85, R87)

**Argmax (measured).** `spread.log`, N = 9 × 10 s:

| Config | hooks off | hooks on | Liveness (hooks on) |
|---|---|---|---|
| D22 literal (harmonic tone, input_gain route) | 15.91 (15.58–16.12) | 16.30 | events 20/s (from the input_gain LFO), conf 6975/8437 |
| crush 1 % (same) | 16.42 (16.19–16.56) | 16.80 | same |
| **crush 1 %, route 13 → s1_level, pure-sine stimulus** | **16.68 (16.48–16.80)** | **16.85 (16.63–16.92)** | **events 19/s, conf 8437/8437, steals 834/s, live 132** |

**C7 recommended config** (the D22 intent at the measured argmax):
- 4 streams on.
- Every stream Float at its max **except crush 1 %**: density 200, size 2000, spray / position / size_rand / jitter 100, pitch +24, pitch_rand 24, reverse_prob 100, window 1, skew +100, spread / gain_rand 100, LP cutoff 20 k, rand 100, res 100, drive 100, crush_prob 100, ring 5 k, rand 100, mix 100. Level +6.
- Scale Major, follow + onset trigger on ×4.
- `grain_limit` 128.
- fb 100 + low cut 2 k + diffusion 100 + saturation 100.
- reverb mix / size 100, damp 100.
- LFOs at 20 Hz across shapes, macros at 100.
- **16 routes ±100 % from all 11 sources, none on `input_gain`.**

Reverse 0 % measured +0.14–0.42 pp (within noise; optional).

**Stimulus (R84).** 220 Hz sine at 0.2, plus 3 ms Hann-gated 10 kHz bursts at 0.8 every 52 ms.
- The bursts sit above the decimator passband (0.4·fs/D = 4.8 kHz at 48 k), so the MPM sees the sine only (100 % confident).
- The detector sees the bursts at 19–20/s, against a refractory ceiling of 20/s.
- The pool steals on every grid spawn (832/s).

**Why not a richer tone (R85):** with the harmonic tone (1, ½, ¼ partials at 0.15) and the same bursts, "D22 no routes" logged **0 events/s over 25 s** (`sweep1.log`). The fast envelope rides the waveform peaks (0.5 ms attack, 10 ms release); the slow one sits at the mean |x|. Their ratio stays above `kOnsetRearm` 1.4125 (`EngineConstants.h:196`), so the edge-triggered detector (R60) never re-arms.

**High rates (R87, INFO, `spread.log` rates block):**

| fs | % | Block p99 µs (budget) |
|---|---|---|
| 44.1 k | 15.28 | 503 (2902) |
| 48 k | 16.57 | 510 (2667) |
| 96 k | **33.19** | 478 (1333) |
| 192 k | **67.30** | 518 (**667**); max 2261 |

---

## 4. CPU gate method (Q3 → R86)

```
C7 (PERF-02 gate, 48 k / 128):
  p = make(); setAll (C7 config); setSeed (fixed); attach MockPlayHead (playing, 120 bpm)
  prepareToPlay (48000, 128)
  stim = precomputed buffer (R84), looped
  warm-up 5 s (pool saturated after ~2 s; fb / reverb filled)
  7 runs × 10 s: t_run = Σ steady_clock around processBlock only;  c_run = thread CPU over the run
  verdict: median (t_run / 10 s) <= 30 %          (phase fail above)
  flag   : any single run > 30 % with median <= 30 %  (CONTEXT: flagged, not a fail)
  witness: c_run / t_run >= 0.9 for every run, else "contended — no verdict, re-run" (never FAIL)
  liveness: events >= 15/s, confident >= 95 % of hops, steals >= 500/s, live >= 128 (hooks counters)
  INFO   : min / max / spread; per-block p50 / p99 / max; 96 k and 192 k medians (N = 3); the literal D22 config; % per grain_limit
```

- Thread QoS: set `pthread_set_qos_class_self_np (QOS_CLASS_USER_INTERACTIVE, 0)` for the timing thread. Real audio threads run at real-time priority on the performance cores. `[ASSUMED]` the default-QoS runs here were on P-cores: the spread was ±1 % and the absolute figure matched ARCH's 10–15 % scalar estimate plus FX.
- **N rationale:** with ±1 % run-to-run spread, the median of 7 is stable to about 0.1 pp. The 13 pp margin makes N a robustness choice, not a power one.

---

## 5. QUAL-01 sweep (Q4 → R89–R92)

### 5.1 Sources × destinations × edits

**Sources (11):**
- LFO 1–4: sine / triangle / smooth random / **S&H**. Stepped shapes are "no worse than the source's own step" by design (R51).
- Env (on a tonal input).
- Macro 1–4 (driven by host ramps).
- Pitch.
- **Onset.** Its attack is an instant step by design (MX2 excludes the attack cell).

**Destination implementations** (each a separate code path, `Engine.cpp` traces at the given lines):

| Class | Destinations (count) | Path |
|---|---|---|
| Ramped (R47 substitutes) | InputGain, OutputGain, Mix, Feedback, FbDiff, FbSat, Level ×4, Pan ×4 (14) | `ExactRamp` / `CellLerp` per sample (`:581-656`) |
| Tick | FbLowCut, FbHighCut (TPT coefficients per cell, `:211-212`), RevMix / RevSize / RevDamp (atomics → reverb smoothing, `:199-201`), LfoRate ×4 (`:836`) (9) | per cell |
| Latched | 22 stream fields × 4, BufferLength (89) | spawn (`:1049-1052`); no within-grain step by construction (ZQ1) |

**Edit kinds (6):**
1. add (None → src)
2. source → None
3. dest → None
4. depth → 0
5. depth step +60 → −60
6. dest swap / source swap

Mechanism (`ModMatrix.h:187-243`): a route change pushes `lastContrib` into the 20 ms per-dest residual and restarts the slot depth from 0 with a 20 ms ramp. A depth edit ramps for 20 ms. So every edit is value-continuous in the **normalised** domain, independent of source and dest. That is why a full cross is cheap to run and the only thing per-dest is the application path.

### 5.2 Trajectory half (full cross, about 460 renders)

- Render 1 s at 48 k, block 32. Up to 16 slots per render carry one source to 16 distinct dests, and every slot is edited at the same tick. That gives 112 dests / 16 = 7 renders per (source, edit kind), so 11 × 7 × 6 = 462.
- After every block (one cell), read `getDestTelemetry (d)` (v01 at the knob, written every tick for every dest, `ModMatrix.h:281-297`) and `getSrcTelemetry (src)`.
- **Bound (normalised):** `|Δv01|_cell ≤ 1.05·(|depth|·max|Δsrc|_cell(±1 cell) + |c_edit|/rampCells)`. Clamping to [0, 1] is 1-Lipschitz, so the bound survives it.
- The source term is the same-motion reference in trajectory form. Onset attack cells are excluded (the MX2 rule).
- Negative controls:
  - `noResidual` (exists) on source → None must exceed 2× the bound.
  - **New hook `noDepthRamp`** (slot depth snaps): add / depth step must exceed 2× the bound.

### 5.3 Audio half (the application paths, about 140 renders)

- The 23 ramped + tick dests × {add, source → None, depth step} under **LFO sine 2 Hz** and **Onset**.
- 440 Hz sine on both channels, mix 100, wet below T (R92).
- κ in [edit − 30 ms, edit + 50 ms] ≤ **2.0×** the held-route reference (route present the whole render; MF7 / MX2a form).
- FbLowCut / FbHighCut / Feedback need fb > 0 and a fed loop (fb 50 %).
- Reverb dests: reverb on, so the κ verdict is **INFO** (R92 confound). Their trajectory half is the gate.

### 5.4 Edge rows (new)

| Row | Subject | Reference(s) | Liveness / control |
|---|---|---|---|
| EN1 enable on / off (s2) | 2 streams, s2 toggled at t1 (on) and t2 (off), tonal | s2 always on; s2 always off; verdict κ ≤ 2.0× max of the two | ≥ 1 spawn on s2 within 20 ms of on; control: a hard-switch model (X1 "HARD") > 2.0× |
| FZ8 freeze toggle, full feature | C7 feature set minus crush and reverb, 440 Hz at 0.001 | the unfrozen / still-frozen control (FZ2a/b form) | `freezeEngages` ≥ 1 per edge; FZ2an / bn controls reused |
| K3 steal + kill, full feature | 4 streams + FX (no crush) + follow + triggers, caps 32 / 128 | steal-free twin (demand < 128 at cap 128) | steals / kills in window ≥ 50 |
| **QB3 burst steal** (R91) | triggers on, cap 32, demand in (32, 128), swell carrier | the same at cap 128 (steal-free, same spawns) | `burst && stolen` records ≥ 50; QB3n hook `stealFadeMs 0` > 2.0× |
| MX3 / MF8 | §5.2 / §5.3 | | |

Thresholds stay at the precedent values: **κ ≤ 2.0× a same-motion reference**, trajectory **≤ 1.05× the analytic bound**. No row compares against a static render when the subject moves (memory: step-click same-motion control).

---

## 6. QUAL-02 (Q5 → R93–R96)

### 6.1 What FTZ means for the census (R93)

`processBlock` holds `juce::ScopedNoDenormals` (`PluginProcessor.cpp:253`). Under AArch64 FPCR.FZ (and MXCSR FTZ/DAZ on x86) no arithmetic on the audio thread can store a subnormal. The only exceptions are a bitwise copy of a subnormal **host input** (the mix-0 fast path, `Engine.cpp` `mixOut`), which is the host's own value, and values computed in `prepare()` on the message thread.

**Measured (`decay.log`, 400 s, R81 config):**
- **FTZ on:** 0 subnormals in grains, chain, diffuser, ring, reverb, analysis and `fbDelay` at every 1 s scan. The output reaches exact 0 at 295.6 s.
- **FTZ off** (`Engine::process` called directly):

| State | First subnormal (s) | Peak count |
|---|---|---|
| analysis (detector envelopes) | 11 | 3 |
| ring | 246 | 168 000 |
| diffuser | 259 | 4 666 |
| reverb | 262 | 49 192 |
| chain | 263 | 9 |
| grains | 270 | 136 |
| `fbDelay` | 271 | 64 |

  The output reaches 3e-40 at 340 s and **sticks at 2.4e-42** from 360 s on (a subnormal limit cycle).
- **Census self-test:** `fpclassify (1e-40f)` → SUBNORMAL. The census runs outside the `ScopedNoDenormals` scope (after `processBlock` returns), so classification is not flushed.

**Decay tail length (Q5):** about 300 s from input stop to the last subnormal transit at fb 100 % / 48 k (2.6 dB/s across ≈ 760 dB). A shorter tail cannot see late subnormals in the FTZ-off arm, and cannot show "exact zero" in the FTZ-on arm.

**Rows:**
- **Z7a** extreme stress, 4 rates × (60 s stimulus + 30 s silence). Gate: finite, wet ≤ A, out ≤ the Z6 bound, 0 scrubs / kills, census = 0 (FTZ on), scans ≥ 3.
- **Z7d** decay at 48 k, background (`--long`):
  - FTZ on: exact-zero output for ≥ 1 s before 330 s, census 0 at every scan.
  - **FTZ-off arm: census > 0 by 300 s** (the non-vacuity control, required).
- Static row: `ScopedNoDenormals` is the first statement of `processBlock`.

### 6.2 State reach (R94)

| State | Reach today | 2.6 |
|---|---|---|
| Grains (all FX state, read position) | `getPool().getGrains()` + `eg23b::anySubnormal` | reuse |
| fb chain HP/LP/env | `getChainForTest()` (Z3) | reuse |
| **Diffuser lines** `line[2][4][4167]` | public in `FeedbackChain` (`:241`), never scanned | add |
| **Ring levels 0–2** | `getRingForTest().levelSample / getLevelCount` (public, module), never scanned | scan the last 1 s per level per scan |
| Half-band history | private in `MipRing` (module) | not needed: copies of the level data, covered by the level scan |
| **`fbDelay[2][32]`** | private | `getFbDelayForTest()` (hooks-only) |
| **Reverb tank** (tank lines, filters `z`, diffusion lines / state, pre-delay) | private in reverb-unit **and** `Engine::reverb` private | `getReverbForTest()` (plugin, hooks-only) + explicit-instantiation member access for the module privates. **Compiled and run in `probe.cpp`; no module change** |
| Analysis (detector, decimator, window, NSDF, guard, tracker) | `getAnalysis()` (Z6a) | reuse |
| Env / LFO / sources | `getEnv().state()`, `getLfo (n).value()`, `getSources()` | add env + LFO |
| Matrix residuals | `getMatrix().residualValue (d)` | add |
| Ramps / CellLerps | private | optional (accessor). FTZ covers them, and their targets are clamped |

The idiom used in `probe.cpp` (standard: access checks are waived in explicit instantiations):

```cpp
template <typename Tag> struct TagBase { friend auto get (Tag); };
template <typename Tag, auto M> struct Robber { friend auto get (Tag) { return M; } };
struct TankTag : TagBase<TankTag> {};  template struct Robber<TankTag, &ReverbUnit::tankDelays>;
struct BufTag  : TagBase<BufTag>  {};  template struct Robber<BufTag,  &ReverbUnit::DelayLine::buffer>;
// auto& tanks = rev.*get (TankTag {});  for (auto& d : tanks) for (float x : d.*get (BufTag {})) ...
```

### 6.3 Extreme list (R95)

**Max side:** the C7 config. Also:
- each stream at **+24 and −24**, reverse 100 and 0
- fb 100 with low cut 20 **and** 2000, high cut 1000 **and** 20 000
- reverb size 100, damp 0 **and** 100
- 16 routes ±100 from every source, including `input_gain` / `output_gain` (both signs)
- triggers + follow ×4
- freeze toggled 6 times

**Min side (hazards):**
- `filter_cutoff` 20 Hz + res 100 (k = 0.1, g = 3.3e-4 at 192 k), with LP / BP / HP
- `filter_rand` 100 at cutoff 20 (randOct → 5 Hz → clamps to 20)
- `ring_freq` 20 + rand 100 (carrier 5 Hz)
- **`buffer_length` 1 s** with size 2000 / pitch +24 / reverse (reach far above span: `A = max (span − floor, 0.5·r·len)`, `Spawner.h` frozen path; `jassert (d <= maxD)` on reverse)
- `size` 5 ms with density 200 (spawn-bound)
- `input_gain` −24 dB on a −60 dBFS input (subnormal-adjacent levels)
- `s_level` −60 with `gain_rand` 100 (−84 dB grains)
- `env_attack` 1 ms / `env_release` 10 ms at 192 k
- `lfo_rate` 0.01 Hz
- `onset_threshold` −60 (fires on the noise floor)
- `drive` just above the 1e-4 guard (norm ≈ 1e4, `GrainFx.h:104-111`)
- `crush` 1 %
- `grain_limit` 32 under demand 400 (R3 kills + steals)
- Custom scale `edo 64 1` if the parser accepts a 1-cent period: AbsSet / ActiveSet outward-truncation path. Check `tableFromText`'s minimum period at plan

**Fuzz (Z7f):** 64 seeded random full-parameter states × 2 s, each with 8 random mid-render parameter jumps, at 44.1 / 48 / 96 / 192 k. Gate: finite, out ≤ bound, census 0, 0 reach violations.

### 6.4 R96: host non-finite input (latent defect, class)

**Measured** (`probe.log`; s1 50 g/s × 200 ms, spray 50, mix 50, fb 50, reverb 30; one NaN on L at 1.000 s):

| | NaN | +Inf |
|---|---|---|
| Non-finite output samples (to 12 s) | 1 030 727 | 1 024 875 |
| First after the input sample | 1.226 s | 1.323 s |
| Last | **12.000 s (end of render)** | **12.000 s** |
| Ring non-finite at 12 s | 160 | 160 |
| Reverb tank non-finite | **32 656** | **31 960** |
| fb scrubs | 192 | 144 |

**Mechanism:**
1. `CaptureWriter::write` pushes `inL·g + fb` unguarded (`CaptureWriter.h:116-117, 125`).
2. The NaN stays in the ring for 41 s, and every grain whose sinc taps cover it outputs NaN.
3. The wet bus feeds `reverb.process` (`Engine.cpp:1476`). The tank clamps with `jlimit (-2, 2, …)` (`ReverbUnit.h:283`), which passes NaN, and the tank re-circulates it **for ever**: `clearTank` runs only on an inactive → active edge (`ReverbUnit.h:186-190`) or in prepare.
4. The knee passes NaN by design (`OutputLaw.h:186`).
5. The fb chain scrub fires, but the ring re-feeds it.

**Class (D25), non-finite entry points:**

| Entry | Guard |
|---|---|
| Host → analysis tap | `Analysis.h:800` |
| Host → env | `ModSources.h:268` scrub |
| Grain SVF | NaN kill (`Engine.cpp` renderSegment) |
| fb return | `FeedbackChain.h:190-197` |
| Matrix offsets | `ModMatrix.h:236` |
| Parameter latch | `ControlGrid.h:82-483` `isfinite` |
| Tracker | scrub |
| **Host → ring** | **none** |
| **Wet → reverb tank** | **none** (module) |
| Dry → out | the host's own sample, out of scope |

**Fix (no module change):**
- In both `CaptureWriter` push statements, form `x = in·g`; `if (! (std::abs (x) <= kAnalysisTapMax)) x = 0;` then push `x + fb`. This is the analysis-tap predicate (P61 / 2.5b deviation 2). It is bitwise neutral for any |in·g| ≤ 1e4 (+80 dBFS), so the N0–N3 / R12 nulls and baseline A″ stay identical.
- Optional defence in depth: a per-piece `isfinite` check on `wet` before `reverb.process`, zeroing the sample and counting it.

**Gate Z7h:** NaN / +Inf / 1e30 at one host sample (L, then both channels) at 4 rates. Output is finite from `at + 1` on. The ring and tank census is finite at the end. The audio is bitwise == the render where that one sample is 0, for every sample after `at`. **Negative control:** the pre-fix tree fails it (measured above).

---

## 7. PERF-01: whole-feature allocation gate (R97)

**Gap:**
- Every M-row sets `armDuringProcess = true` inside `j.after`: `engine_gates.h:881-884`, `engine_gates_22b.h:922-925` ("first block unarmed (warm-up)"), `engine_gates_23b.h:450-453`, `engine_gates_24b.h:699-702`, `engine_gates_25ab.h:926-929`, `engine_gates_25bb.h:1008-1011`.
- So **block 0, the first tick after prepare, is never armed.** That block runs the first latch, ActiveSet / AbsSet builds, the reverb first-run `clearTank` and snap, and the tracker's first adopt.

**Measured** (`probe.cpp --alloc`, malloc_logger, render-thread scoped, liveness 1, **block 0 armed**):
- Config: 4 streams, cap 32, 200 g/s × 2 s, scale, SVF + drive + crush + ring, follow + triggers, 16 routes, fb 100, reverb 100, **frozen at load**; at block 100: unfreeze + `sync_mode` Tempo + route 3 removed.
- 512 blocks: **0 allocs** (block 0: 0).
- First block 8192 after prepare 512, then variable 1…2048: **0 allocs**.

**Static audit (read):**
- `std::vector` appears only in `WindowMorphBank::build` (ctor) and the module `prepare`s.
- No `juce::String` / `Array` / `std::function` in `Source/dsp`.
- `revView.setDataToReferTo` uses the 32-entry preallocated channel array (`juce_AudioSampleBuffer.h:1214-1216`).
- The only lock on the audio thread is a `SpinLock` **try**-lock (`Engine.cpp:249`). The message thread allocates `juce::String` while holding the same lock (`PluginProcessor.cpp:351-356`); the audio side never waits on it.

**M7 (one scope, CONTEXT):** two renders (Free, Tempo), armed **from block 0**, block schedule `var` incl. a fresh 8192 first block, C7 feature set at cap 32 and 128. Event script, every one on the message side between blocks:
- freeze ×4 (mid-cell)
- route add / remove ×4 kinds / depth step / dest swap / source swap, each from all 11 sources
- scale change, mask change, custom apply
- `sync_mode` switches; tempo step, loop wrap, locate, stop / start, time signature
- seed change
- reverb_mix 0 → 50 → 0 (clearTank edge)
- `onset_threshold` sweep
- `grain_limit` 128 → 32 (kills)
- **preset-like mass change:** all 220 parameters rewritten between two blocks
- hook injections: fb NaN, env NaN, analysis-tap NaN, SVF NaN kill, and (after R96) host NaN

Gate: 0 allocations, liveness 1, counters live (spawns, steals, kills, events, hops, hits, bursts, route changes, re-anchors, re-locks, scrubs ≥ 1 each).

---

## 8. PERF-02: `grain_limit` determinism (R98)

For each cap 32 / 64 / 96 / 128 under the C7 config (10 s):
- **(a)** `diag.maxAlive ≤ cap` and a per-block scan of non-killing live slots ≤ cap (the R3 row form, `engine_gates.h:914`).
- **(b)** two renders with the same seed: spawn logs (slot, stolen, killed, globalIndex) and audio bitwise; seed + 1 differs.
- **(c)** CPU per cap as INFO (expected roughly ∝ cap; measured only at 128 here).

Block-size invariance of the steal sequence is carried by N10.

---

## 9. PERF-03 (Q6 → R99, R100)

**Nothing sizes from the host block** (read):

| Component | Evidence |
|---|---|
| Engine | `Engine::prepare` ignores `maxBlockSize` (`Engine.cpp:32`); pieces ≤ kCell (`:408`); per-pass arrays `[kCell]` with canaries (`Engine.h` fixed per-pass storage) |
| reverb-unit | prepared at `kCell` (`Engine.cpp:119`); `prepare (double, int /*maxBlockSize*/)` (`ReverbUnit.h:125`); per-sample loop |
| Analysis | `processPiece (n ≤ kCell)` |
| ModMatrix, TempoTracker, GrainPool, MipRing | fixed arrays, per-sample / per-tick work |
| `processBlock` | no scratch buffers (`PluginProcessor.cpp:251-312`) |

**Harness support:**
- `render()` already separates `prepBlock` (default 512) from `block` (`engine_gates.h:165-169`).
- It creates a fresh processor per job.
- It **cuts host blocks at every event and transport-segment start** (`:219-220`, `withTransport` `engine_gates_25a.h:41-63`).
- The MockPlayHead reports the true position at each block start.

So the 8192 case keeps the transport and event schedule identical to the 512 reference. Requirement: no event before sample 8192, so the first block really is 8192 (N9's first event is at 48017).

**N10:** the C7 feature set (minus crush, plus feedback 80) + N9's input, events and transport (Tempo form) + route edits + a `reverb_mix → 0` edge (R100). Blocks:
- 32 (reference)
- 441, 512, 4096
- var
- **8192 with prepBlock 512 (fresh)**
- 8192 with prepBlock 1

Each must be bitwise == 32, with events / hops / f0 ring identical and `guardsIntact()`.
- Seed + 1 differs.
- **N10n:** `perBlockOrigin` (Tempo) must differ at 441.
- **O2:** O1's host-guard poison test (`engine_gates.h:804-845`) with the C7 feature set.

---

## 10. Open questions → answers

| CONTEXT question | Answer |
|---|---|
| 1. D21 mapping | R79: VST3 `isinf → kInfiniteTail` (`VST3.cpp:3486`); measured pluginval 10 SUCCESS with ∞. R80: AU ∞ verbatim (`AU_1.mm:1202`, `AUBase.cpp:548`), the UB path unreachable (`MusicDeviceBase`). auval with ∞ is unverified → T1 checkpoint |
| 2. D22 stimulus / argmax | R82–R84: cost ∝ saturated live grains × FX; argmax = all max with **crush 1 %**, 16.7–16.9 %; stimulus = pure sine + 10 kHz bursts, no input_gain route. Size dominates density; max size beats min size (6.5 % vs 16 %) |
| 3. CPU method | R86 / §4: N = 7 × 10 s, 5 s warm-up, median, duty witness, `processBlock`-only timing, exact 128 blocks (block size moves < 0.2 pp), MockPlayHead, P-core QoS. C3w / C3t are configs, not clocks |
| 4. QUAL-01 matrix | R89–R92 / §5: full cross in trajectory space (≈ 460 renders) + audio κ on the 23 application paths (≈ 140 renders) + EN1 / FZ8 / K3 / QB3. Thresholds 1.05× trajectory, 2.0× κ |
| 5. QUAL-02 census | R93–R95 / §6: census structural under FTZ; FTZ-off arm as the control; tail ≥ 300 s at fb 100 %; reach table (diffuser, ring, `fbDelay`, reverb tank are new); extreme + min + fuzz lists |
| 6. PERF-03 variable / oversized | R99: no change to MockPlayHead v2; `render()` keeps the schedule identical; first block 8192 on a fresh instance; nothing sizes from `maxBlockSize` |
| 7. 192 k | R101: 110.2 MB (exact), RSS +111.6 MB, prepare 7.9 ms; MP1h load-independent by construction → hop-log equality gate; CPU 67 % INFO |

---

## 11. Sign-offs needed at plan (Taylor)

1. **R96 (defect fix, scope):** sanitise the host input at the ring write (the `kAnalysisTapMax` predicate). Optionally also guard `wet` before the reverb. Add row Z7h.
   - Bitwise neutral for any input ≤ +80 dBFS. No module change.
   - Host input isn't in CONTEXT's QUAL-02 wording; this extends it.
2. **R83 (amends D22's wording):** the C7 config uses **crush 1 %**, because crush 100 % is cheaper. "Everything maxed" is kept as an INFO row.
3. **R84 (amends D22's "16 slots from every source"):** no route on `input_gain` in C7, because it drives the detector at the LFO rate and costs MPM confidence. All 11 sources still route; route 13 moves to `s1_level`.
4. **R80 fallback:** if auval or pluginval AU rejects ∞ at the T1 checkpoint, choose between:
   - **(a)** a large finite AU-only value. JUCE has no per-format hook, so it would have to be global, which breaks VST3's `kInfiniteTail`.
   - **(b)** a large finite value everywhere, e.g. 600 s (R81: exact zero at 296 s).
   
   Recommendation: (b), only if needed.
5. **R85:** the sustained-tone onset deafness stays a Stage 3 DAW watch item (recommended). Revisiting the R60 re-arm is a behaviour change outside 2.6.
6. **R93:** the QUAL-02 "no subnormal" criterion is read as "shipped path (FTZ) census 0 + an FTZ-off control that shows the census can see them". The 300 s decay row runs in `--long` / background.

## 12. Carry-ins for plan

- **D24 first:** baseline A″ = the 388-row harness on the 454c8a31 tree + N3 / N2 / N1 / N0 `--strict` + R12. Every task diffs empty against A″, except rows a signed-off fix moves, each attributed.
- **T1 = D21 + install + auval / pluginval 10 (VST3 + AU) + HA.** This is the R80 checkpoint, and it changes no audio.
- **Hooks to add** (hooks-only; defaults off, so the N-suites stay identical):
  - `noDepthRamp` (ModMatrix)
  - `stealFadeMs` (`pool.prepare (fs, x)`)
  - `getFbDelayForTest()`
  - `getReverbForTest()`
  - an FTZ-off render path in the harness (`Engine::process` direct)
- **Harness:** `--only26`, `--long`. Run every full harness, Z7d, C7 at rates and the N-suite in the background with logs (memory: 600 s watchdog).
- **Reference implementations:** `scratchpad/r26/cpu/cpu.cpp` (C7 timing, stimulus, liveness) and `cpu/probe.cpp` (host-NaN rows, FTZ-off census, member-access idiom, block-0-armed alloc probe) are the measured prototypes.
- **W2o (carried):** if it flips, widen the seeds. Never widen the threshold.
- **Stage 3 DAW list additions:**
  - ∞ tail: bounce and idle behaviour in Logic / Live / Reaper (R80)
  - drums over a sustained pad (R85)
  - 192 k CPU (R87)
- **Update** the `PluginProcessor.h:63-64` comment ("P7: stays 0 in 2.1 …") with D21.

## Assumptions log

| # | Claim | Where | Risk if wrong |
|---|---|---|---|
| A1 | auval accepts `TailTime = +∞` | R80 | T1 fails → sign-off 4 |
| A2 | Logic / Live / Reaper handle ∞ sanely (no bounce hang) | R80, R81 | DAW item; fallback (b) |
| A3 | The default-QoS timing thread ran on P-cores | §4 | E-core figures would be pessimistic (higher), not optimistic |
| A4 | A custom `edo 64 1` table parses | §6.3 | Drop that extreme if not |
