# Stage 1: Foundation - Verification

## Verification Date

2026-10-12

## Method

Goal-backward against CONTEXT.md's success criteria. Every automated claim in SUMMARY.md was **re-run independently** in this verify session; nothing was taken on SUMMARY's word.

- `ninja O-simpleAmbisonics_{VST3,AU,Standalone} O-simpleAmbisonics-render-test`: no work to do, so the build is current with HEAD `013ef3db` (the Stage-1 execute commit) and the plugin tree is clean.
- The installed bundles `~/Library/Audio/Plug-Ins/{VST3,Components}/O-simpleAmbisonics-dev.*` are **byte-identical** (`cmp`) to the build artefacts.
- `PluginProcessor.cpp` was touched and rebuilt under `juce_recommended_warning_flags`: **0 warnings**.
- Host checks ran on the **installed** binaries: targeted `auval -v aufx OSAm OuDv`, pluginval VST3 + AU at strictness 10, and the installed VST3 loaded in pedalboard (`uv run --python 3.12 --with pedalboard`) — a measurer independent of the project's own harness.
- The project harness (`O-simpleAmbisonics-render-test`, probes P0–P5) was re-run: ALL PASS, digest `6a1df555bc125ae3` reproduced.

## Goal-Backward Analysis

### Original Goals (CONTEXT.md)

1. A buildable, pluginval-clean **stereo effect shell**; cross-platform CMake with the WebView2 flags already set, `juce_dsp` linked, no BinaryData target.
2. The complete **12-parameter APVTS** exactly per ARCHITECTURE.md §Parameter Mapping (IDs, ranges, skews, defaults, choice strings).
3. **Mono/stereo-in → stereo-out** bus layout and nothing else (FUNC-07), with the input conditioner (`Mono sum` = 0.5·(L+R) / `Left only`).
4. The **latency contract in place from day one**: `setLatencySamples(64)`, pass-through = conditioner → 64-sample delay → seeded output gain; no fade-in; oversized host blocks handled; RT-safe.
5. **State persistence** (APVTS XML round-trip) and the Stage-2 seam (`Source/dsp/SpatialEngine.h`, `tests/render-harness/`).

### Deliverables (SUMMARY.md + code inspection)

1. `CMakeLists.txt`: effect (`IS_SYNTH FALSE`, no MIDI), `PLUGIN_CODE OSAm`, `FORMATS VST3 AU Standalone`, `NEEDS_WEB_BROWSER` + `NEEDS_WEBVIEW2`, defs `JUCE_WEB_BROWSER=1` / `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` / `JUCE_USE_CURL=0` / `JUCE_VST3_CAN_REPLACE_VST2=0`, `juce_dsp` linked, `juce_generate_juce_header()` after `target_link_libraries()`, `OUARICON_BUILD_TESTS` harness option. No `juce_add_binary_data`, no editor TU.
2. `createParameterLayout()`: 12 parameters, all `ParameterID { id, 1 }`; `distance` / `orbitRate` log-shaped with `setSkewForCentre` (no lambda ranges); every Choice ≥ 2 entries; degree params continuous (interval 0 — the documented plan deviation).
3. `BusesProperties` stereo/stereo; `isBusesLayoutSupported` rejects disabled buses, accepts input ∈ {mono, stereo}, requires stereo output. Input leg count from `getChannelLayoutOfBus(true, 0)`, output writes bounded by `buffer.getNumChannels()`.
4. `prepareToPlay`: `setLatencySamples(kPartition)` (the one call site), `FixedDelay<64>` reset, `LinearSmoothedValue` seeded via `setCurrentAndTargetValue`. `processBlock`: `ScopedNoDenormals`, params read once, slice loop ≤ `maxBlock`, read-before-write in place, finite scrub, no allocation / locks.
5. `getStateInformation` / `setStateInformation` with a `pluginVersion` stamp and root-type gate; `SpatialEngine.h` (no-op, `kPartition = 64`, `Params`), `FixedDelay.h`, harness scaffold with P0–P5 + FNV digest.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Buildable, pluginval-clean shell; Stage-3-ready CMake | ✅ Achieved | Build current, 0 warnings on a forced TU rebuild; pluginval s10 SUCCESS on VST3 and AU; flags confirmed in `CMakeLists.txt`. |
| 2. 12-param contract | ✅ Achieved | Harness P0 (45 checks) PASS. Independently, pedalboard lists exactly 12 plugin params (+ the JUCE host `Bypass`) with ranges −180…180 / −90…90 / 0.2…20 / 0.02…2 / −60…0, defaults 0 / 0 / 1 / 0.1 / 0, choices `1 2 3`, `Binaural Stereo`, `Mono sum Left only`, `orbit` boolean Off. auval: 12 `-parameter PASS`. |
| 3. Mono/stereo-in → stereo-out, conditioner | ✅ Achieved | auval **Reported Channel Capabilities `[1, 2] [2, 2]`** — exactly the two accepted layouts. Harness P1 (Mono sum bit-equal to 0.5·(L+R) on both channels), P1b (Left only; mono input bus → `in0`). pedalboard: stereo noise in → both outputs == 0.5·(L+R), 8192/8192 samples exact, channels identical. |
| 4. Latency contract, no fade-in, oversized blocks, RT-safe | ✅ Achieved | P0 `getLatencySamples() == 64`; P4 impulse 100 → 164, amplitude exactly 1.0; P1 `out[64]` exact (no fade-in); P3 prepared-64 / fed-4096 bit-equal to prepared-4096; P2 ragged blocks bit-equal. pedalboard (which compensates the *reported* latency) returns the impulse at index **100 with residual lag 0**, so reported == actual in a real host wrapper. `processBlock` inspected: no allocation, no locks; auval Latency PASS. |
| 5. State round-trip + Stage-2 seam | ✅ Achieved | pedalboard: all 12 params set non-default → `raw_state` (927 bytes) → fresh instance restores every decoded value (45°, −30°, 5.003 m, order 3, 90°, −45°, 10.08°, Stereo, orbit On, 0.5 Hz, Left only, −12 dB). pluginval s10 state tests pass on both formats. `SpatialEngine.h` / `FixedDelay.h` present; harness target configured with `-DOUARICON_BUILD_TESTS=ON` and runs. |

## Requirements Verification

**Stage:** stage-1
**Requirements for this stage:** 2 total (2 must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: Passes pluginval validation (VST3 and AU) | must | ✅ Complete | pluginval strictness 10 on the installed bundles: VST3 exit 0 SUCCESS, AU exit 0 SUCCESS, 25 test groups each, 0 FAILED. `auval -v aufx OSAm OuDv` AU VALIDATION SUCCEEDED. |
| FUNC-07: Stereo input summed to the single mono source (Mono sum / Left only); mono input accepted | must | ✅ Complete | Mono sum = 0.5·(L+R) (−6 dB law, documented in code and SUMMARY) — P1 bit-equal and pedalboard exact; Left only — P1b; mono-in/stereo-out accepted — auval caps `[1, 2]`, P1b mono bus. Logic hands-on is a COMPAT-02 (stage-4) item. |

**Requirements Summary:**
- ✅ Complete: 2
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 25 (stage-2: 17, stage-3: 8, stage-4: 4 — FUNC-08, FUNC-09, COMPAT-02, QUAL-02)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build VST3 / AU / Standalone / render-test | ✅ Pass | `ninja`: no work to do at HEAD `013ef3db`; forced rebuild of `PluginProcessor.cpp`: 0 warnings |
| Installed == build | ✅ Pass | `cmp` on both Mach-O binaries: identical |
| Harness P0–P5 | ✅ Pass | 0 failures; digest `6a1df555bc125ae3` reproduced |
| `auval -v aufx OSAm OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED; caps `[1, 2] [2, 2]`; Latency / Tail Time PASS; 12 parameter PASS; 0 WARN |
| pluginval VST3 strictness 10 (installed) | ✅ Pass | exit 0, SUCCESS, 0 FAILED |
| pluginval AU strictness 10 (installed) | ✅ Pass | exit 0, SUCCESS, 0 FAILED; one `!!! WARNING: Current program is -1` (JUCE AU wrapper with `getNumPrograms() == 1`, standard across the suite) |
| pedalboard: params / pass-through / latency / state | ✅ Pass | V1 36 checks, V2 4 checks, V3 13 checks — ALL PASS; V4 mono-in is N/A in pedalboard (it only drives in == out channel counts) and is covered by auval + P1b |

## Human Verification (non-blocking for Stage 2)

The installed-binary checks above cover the substance of these, so they gate nothing in Stage 2. COMPAT-02 (stage-4) repeats them formally in Logic and Live.

- [ ] Logic: AU appears as an **audio effect** (Ouaricon → O-simpleAmbisonics-dev) on a stereo track **and** on a mono track with no I/O configuration; audio passes, latency-compensated.
- [ ] Generic editor shows 12 rows with the correct text (`°`, `m`, `Hz`, `dB` labels; `1/2/3`, `Binaural/Stereo`, `Mono sum/Left only`).
- [ ] Project save → reopen restores changed values.

## Issues Found

- **None blocking.**
- Plan deviation (degree params interval 0, not 0.01) confirmed correct: defaults are exactly 0 through the host path (P0, pedalboard raw 0.5 → 0.0). Recorded as a suite-wide latent pattern (`pattern_stepped_range_snap_makes_zero_default_inexact`).
- pluginval's pre-prepare "Reported latency: 0" is the query-before-`prepareToPlay` artefact documented in SUMMARY; hosts receive 64 (pedalboard's compensation lands the impulse back at index 100).
- Test-harness detail, not a defect: on a VST3 instance, `raw_value` read back after a host set is the wrapper's cached **un-snapped** host value (JUCE `Param::setNormalized` caches what the host sent), while the restored instance holds the snapped APVTS value — `order` 0.75 vs 1.0, both decoding to `3`. State round-trip gates must compare **decoded** values, not normalised floats. pedalboard also appends unit labels to parameter keys (`distance_m`, `orbit_rate_hz`, `output_level_db`) and parses numeric choice strings as floats.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes. Next is Stage 2 (DSP), Phase 2.1 — source → encoder → stereo decode. The Stage-2 handoff notes are in SUMMARY.md (§Stage-2 handoff): engine plugs into the existing slice loop, `setLatencySamples` never changes, P0 / P2 / P3 / P4 / P5 carry forward, P1's expectation becomes the engine render, digest `6a1df555bc125ae3` is the pass-through baseline and is expected to change.

**Blockers:** None
