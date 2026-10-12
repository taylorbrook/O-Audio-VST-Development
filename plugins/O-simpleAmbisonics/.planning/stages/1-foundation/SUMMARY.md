# Stage 1 (Foundation) — SUMMARY

**Plugin:** O-simpleAmbisonics · **Stage:** 1 Foundation · **Phase:** execute · **Date:** 2026-10-12
**Inputs:** `stages/1-foundation/PLAN.md` (T1–T7), `RESEARCH.md`, `CONTEXT.md`, `research/ARCHITECTURE.md` §Parameter Mapping
**Executor:** plugin-workflow orchestrator executing PLAN.md inline (the plan was a complete spec; the foundation-shell-agent has no Bash for T6, so the orchestrator ran T1–T7 directly)
**Gate:** 0-ideation → 1-foundation run with `--force` (build check cannot pass before a source tree exists — standing pattern; justification logged to `.planning/gate-bypasses.log`)

## Result

A buildable, pluginval-clean **stereo effect shell** with the complete 12-parameter APVTS, mono/stereo-in → stereo-out buses, XML state with a `pluginVersion` stamp, and the **final 64-sample latency contract in place**: `setLatencySamples(kPartition)` with a pass-through chain *input conditioner → `FixedDelay<64>` → seeded output gain*. All seven Stage-1 harness probes pass; VST3 and AU pass pluginval strictness 10 twice each; `auval -v aufx OSAm OuDv` succeeds.

Covers **COMPAT-01** (pluginval s10 VST3 + AU) and **FUNC-07** (input conditioner + bus layout).

## Files created

| Path | Purpose |
|---|---|
| `CMakeLists.txt` | Effect target `O-simpleAmbisonics` (== folder), `PLUGIN_CODE OSAm`, VST3 + AU + Standalone, WebView2 flags set, `juce_dsp`, `OUARICON_BUILD_TESTS` harness option. No `juce_add_binary_data`, no editor TU. |
| `Source/PluginProcessor.h` | `OSimpleAmbisonicsAudioProcessor`; `OSimpleAmbisonics::ParamIDs` namespace (12 `inline constexpr` IDs); cached atomics for `inputMode` / `outputLevel`; `DecoderDelay delay`, `LinearSmoothedValue outputGain`, `numInputLegs`, `maxBlock`. |
| `Source/PluginProcessor.cpp` | 12-param layout, buses, pass-through `processBlock` with slice loop + finite scrub, Generic editor inline, state round-trip + `pluginVersion`. |
| `Source/dsp/SpatialEngine.h` | The Stage-2 seam: `inline constexpr int kPartition = 64`, `using DecoderDelay = FixedDelay<kPartition>`, `Params` struct, no-op `SpatialEngine` (`prepare / reset / setParams / process` writes zeros). **Not called** from `processBlock` at Stage 1. |
| `Source/dsp/FixedDelay.h` | `template <int N> struct FixedDelay` — `std::array` ring, zero alloc, exact integer delay. |
| `Source/ui/.gitkeep` | Stage-3 placeholder. |
| `tests/render-harness/CMakeLists.txt` | Console target `O-simpleAmbisonics-render-test`; derived-version block (`JUCE_VERSION` → hex, `FATAL_ERROR` if unreadable); `JUCE_WEB_BROWSER=0`, `OUARICON_RENDER_HARNESS=1`, `JucePlugin_PluginCode=0x4f53416d`, `JucePlugin_IsSynth=0`. Sources: `main.cpp` + `../../Source/PluginProcessor.cpp` only. |
| `tests/render-harness/main.cpp` | Probes P0–P5 + FNV-1a digest (below). |

Not created (per plan): `PluginEditor.*`, any `juce_add_binary_data`, `BinauralFilterData.h`, `scripts/generate_binaural_filters.py`, `parameter-spec.md`.

## The 12 parameters (as built — P0 verified)

| ID | Type | Range | Default | Skew | Label |
|---|---|---|---|---|---|
| `azimuth` | Float | −180 … 180 | 0 | 1 (linear) | ° |
| `elevation` | Float | −90 … 90 | 0 | 1 | ° |
| `distance` | Float | 0.2 … 20, step 0.001 | 1 | 0.289 (`setSkewForCentre(2 m)`) | m |
| `order` | Choice | `1` / `2` / `3` | `1` | | |
| `yaw` | Float | −180 … 180 | 0 | 1 | ° |
| `pitch` | Float | −180 … 180 | 0 | 1 | ° |
| `roll` | Float | −180 … 180 | 0 | 1 | ° |
| `decoder` | Choice | `Binaural` / `Stereo` | `Binaural` | | |
| `orbit` | Bool | Off / On | Off | | |
| `orbitRate` | Float | 0.02 … 2, step 0.001 | 0.1 | 0.289 (`setSkewForCentre(0.2 Hz)`) | Hz |
| `inputMode` | Choice | `Mono sum` / `Left only` | `Mono sum` | | |
| `outputLevel` | Float | −60 … 0, step 0.1 | 0 | 1 | dB |

All `ParameterID { id, 1 }`. Skews via `setSkewForCentre` only (no lambda ranges). Exactly 12 `RangedAudioParameter`s (P0 counts them).

### Deviation from PLAN T3: degree parameters use interval **0**, not 0.01

The first harness run failed P0 "default" for all five degree parameters: `get()` returned **−4.02e-06**, not 0. Cause (read in JUCE 8.0.15 `juce_RangedAudioParameter.cpp:57`): `convertFrom0to1` calls `range.snapToLegalValue`, which computes `start + interval·floor((v−start)/interval + 0.5)` = `−180 + 0.01f·18000`, and `0.01f·18000` is `179.99999598…` in float. Every value set through the host path (including APVTS's own default seeding) inherits the error, so 0° could never be exactly 0°. Fix: `NormalisableRange<float>(−half, half, 0.0f)` — continuous — so `0.5` normalised → exactly `0`. RESEARCH §4 already classed the interval as display granularity only; the Generic editor still formats to two decimals, the Stage-3 knob has its own step, and the Stage-2 wrap-aware angle smoother wants continuous values anyway. `distance` / `orbitRate` / `outputLevel` keep their plan intervals (their defaults pass `approximatelyEqual`; `outputLevel`'s 0 snaps exactly to `end`).

Note for the suite: every sibling with a negative-start stepped range (e.g. `−12 … 12, 0.1`) carries the same latent snap offset at its 0 default; nobody gates exact defaults, so it was never observed. Worth a memory.

## Bus layout (as built)

`BusesProperties().withInput("Input", stereo).withOutput("Output", stereo)`; `isBusesLayoutSupported` rejects disabled in/out, accepts input ∈ {mono, stereo}, requires output == stereo.
auval **Reported Channel Capabilities: `[1, 2] [2, 2]`** — exactly the two accepted layouts. Input leg count read from `getChannelLayoutOfBus(true, 0).size()` in `prepareToPlay`, never from the buffer width; output writes bounded by `buffer.getNumChannels()`.

## Pass-through chain (as built)

```
prepareToPlay:  setLatencySamples(kPartition)      // the ONE call site, 64 forever
                maxBlock = max(samplesPerBlock, 1); numInputLegs = negotiated input width (1|2)
                delay.reset(); outputGain.reset(fs, 0.02); setCurrentAndTargetValue(dB→gain, −60 = 0)   // seeded
processBlock:   ScopedNoDenormals; mode = inputMode; outputGain.setTargetValue(...)
                for slice in ≤ maxBlock pieces:
                    s = in1 ? (mode==0 ? 0.5(L+R) : L) : in0     // read i before write i, in place
                    y = delay.process(s) * outputGain.getNextValue(); out0[i] = out1[i] = y
                finite scrub per written channel; clear channels ≥ 2 (none in an accepted layout)
```
No allocation, no locks, no `updateHostDisplay` on the audio thread.

## Harness — all probes PASS

Binary: `build/plugins/O-simpleAmbisonics/tests/render-harness/O-simpleAmbisonics-render-test_artefacts/Release/O-simpleAmbisonics-render-test` (configure with `-DOUARICON_BUILD_TESTS=ON`).

| Probe | Gate | Result |
|---|---|---|
| P0 contract | 12 params by ID with types/ranges/defaults; `order` 3 / `decoder` 2 / `inputMode` 2 choices; `distance`+`orbitRate` skew ≠ 1, others = 1; exactly 12 ranged params; `getLatencySamples() == kPartition == 64` after prepare | **PASS** (45 checks) |
| P1 pass-through | Mono sum, 0 dB, 8192 noise samples: `out[c][n] == 0.5f·(L[n−64]+R[n−64])` bit-equal both channels; `out[0..63] == 0`; `out[64]` exact | **PASS** (`out[64]` = 0.328433 exact — no fade-in) |
| P1b Left only + mono-in | `inputMode = 1` → `out == L[n−64]`; mono input bus (`setPlayConfigDetails(1, 2, …)`) → `out == in0[n−64]` both channels | **PASS** |
| P2 block sizes | ragged `{1, 7, 64, 333, 4096, 13, 512, 2}` ≡ flat 512, bit-equal | **PASS** |
| P3 oversized block | `prepareToPlay(fs, 64)` fed 4096-sample blocks ≡ `prepareToPlay(fs, 4096)`, ≡ flat — bit-equal | **PASS** |
| P4 latency | impulse at 100 → first nonzero at **164** = 100 + reported 64; amplitude exactly 1.0 | **PASS** |
| P5 silence floor | `outputLevel = −60` before prepare → every output sample exactly 0 | **PASS** |
| digest | FNV-1a 64 of the P1 flat render | **`6a1df555bc125ae3`** (8192 × 2 ch, flat 512, mono sum, 0 dB) — Stage-1 baseline |

## Build / install / validation

| Step | Result |
|---|---|
| `cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON` | configured; new folder globbed; harness version `1.0.0 (0x10000)` derived from the plugin target |
| `ninja O-simpleAmbisonics_VST3 _AU _Standalone -render-test` | clean, **0 warnings** under `juce_recommended_warning_flags` |
| `./scripts/build-and-install.sh O-simpleAmbisonics` | installed `O-simpleAmbisonics-dev.{vst3,component}`; AU cache cleared; no alternate-variant orphan |
| `auval -v aufx OSAm OuDv` | **AU VALIDATION SUCCEEDED**; Latency / Tail Time / Bypass PASS; channel caps `[1,2] [2,2]` |
| pluginval strictness 10, VST3 (`--timeout-ms 600000`) ×2 | **SUCCESS ×2**, 0 failures |
| pluginval strictness 10, AU ×2 | **SUCCESS ×2**, 0 failures |

pluginval's "Plugin info" block prints `Reported latency: 0` — that query runs before `prepareToPlay`, and every sibling (O-Emulator, O-AnalogSaturation, O-Strata) sets latency in `prepareToPlay` too, so this is suite-consistent, not a defect. Hosts receive 64 at prepare (and `updateHostDisplay(latencyChanged)` fires once).

## Gotchas checklist (PLAN §Gotchas) — all ticked

- [x] `juce_generate_juce_header()` after `target_link_libraries()`; WebView2 dual flag set now.
- [x] No `juce_add_binary_data` at foundation.
- [x] `setLatencySamples(kPartition)` only in `prepareToPlay`.
- [x] Output smoother seeded in `prepareToPlay`; P1 gates `out[64]` explicitly.
- [x] No allocation on the audio thread — `FixedDelay` is a `std::array`; no `setSize`, no `DelayLine::prepare`.
- [x] Input leg count from the negotiated bus (`getChannelLayoutOfBus(true, 0).size()`), not the buffer width; output writes bounded by `buffer.getNumChannels()`.
- [x] Read inputs for sample *i* before writing outputs for sample *i*.
- [x] `getNumPrograms() == 1`, no-op `setCurrentProgram`.
- [x] Skews via `setSkewForCentre`; every Choice ≥ 2 entries; no `juce::` name collisions.
- [x] Harness: `JUCE_WEB_BROWSER=0`, derived version block, no editor TU.
- [x] Trunk-based, path-scoped, temp-index commit with compare-and-swap.

### Two build-time corrections (not deviations)
- `PluginProcessor.cpp` uses namespace aliases (`namespace osa_dsp = OSimpleAmbisonics::dsp;`) instead of `using namespace OSimpleAmbisonics;` — JuceHeader.h injects `using namespace juce;`, so a bare `dsp::` was ambiguous between `juce::dsp` and `OSimpleAmbisonics::dsp`. Stage 2 code in this TU must qualify `dsp` the same way.
- `getMainInputChannelSet()` is a `BusesLayout` method, not an `AudioProcessor` one; the processor reads `getChannelLayoutOfBus(true, 0)`.

## Stage-2 handoff

- **Where the engine plugs in:** `processBlock`'s slice loop already bounds work to `≤ maxBlock` per pass. Phase 2.1 replaces `delay.process(s)` with a per-slice `engine.process(monoSlice, outL, outR, len)` (conditioned mono into a prepared-size scratch, outputs written back under the same read-before-write discipline), keeps `outputGain.getNextValue()` per sample after it, and keeps the finite scrub.
- **Latency:** never touch `setLatencySamples`. `kPartition` in `SpatialEngine.h` is the UPOLS partition size and the stereo-path `DecoderDelay` length; P0/P4 enforce 64 and must stay green.
- **`FixedDelay`** survives unchanged into `DecoderMixer` for the stereo path.
- **Harness carry-forward:** P0 / P2 / P3 / P4 / P5 stay as written; P1's expectation becomes the engine render (or a null against a reference); add the allocation counter (PERF-01) and the DSP probes from ROADMAP §Phase 2.1. Digest `6a1df555bc125ae3` is the pass-through baseline — it is *expected* to change when the engine lands.
- **Parameters not yet read by DSP:** `azimuth elevation distance order yaw pitch roll decoder orbit orbitRate` — Stage 2 wires them through `Params` (values are continuous floats; angles are not stepped).
- **Editor:** Stage 3 adds `PluginEditor.{h,cpp}` and wraps both the include and `createEditor()` in `#if JUCE_WEB_BROWSER … #else GenericAudioProcessorEditor #endif`; the harness must never list the editor TU.

## Decisions taken (from PLAN planner picks, confirmed in execution)

| Choice | Decision |
|---|---|
| Editor shape | O-Emulator: no editor TU, Generic editor inline |
| 64-sample delay | `FixedDelay<kPartition>` in `Source/dsp/FixedDelay.h` |
| Latency constant | `OSimpleAmbisonics::dsp::kPartition = 64` in `SpatialEngine.h` |
| Output gain | `LinearSmoothedValue`, seeded, `getNextValue()` per sample |
| Version stamp | `pluginVersion` property in `getStateInformation` |
| PLUGIN_CODE | `OSAm` (0x4f53416d) |
| Degree intervals | **0 (continuous)** — deviation from PLAN's 0.01, see above |

## Success criteria (for the verify phase)

- [x] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU` builds clean (0 warnings).
- [x] pluginval strictness 10 passes for VST3 **and** AU (COMPAT-01) ×2; `auval -v aufx OSAm OuDv` passes.
- [x] Stereo-in and mono-in accepted, every other layout rejected (auval caps `[1,2] [2,2]`; `isBusesLayoutSupported` by inspection). *DAW hands-on (stereo + mono track) is a verify-phase human step.*
- [x] Audio passes through, both outputs carry the conditioned mono source, 64 samples reported and measured (P1, P4).
- [x] All 12 parameters with correct ranges, defaults, skews, choice strings (P0). *Generic-editor visual check is a verify-phase step.*
- [x] State round-trip code present with `pluginVersion` (pluginval s10 state tests pass; explicit value-by-value round-trip is a verify-phase probe candidate).
- [x] No fade-in at first block (P1 `out[64]`); P3 oversized block bit-equal; no allocation in `processBlock` by inspection.
- [x] `Source/dsp/SpatialEngine.h` + `FixedDelay.h` present; harness configures with `-DOUARICON_BUILD_TESTS=ON`; P0–P5 PASS.
