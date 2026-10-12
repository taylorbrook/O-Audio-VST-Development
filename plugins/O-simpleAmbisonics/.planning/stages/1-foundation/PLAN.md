# Stage 1 (Foundation) — PLAN

**Plugin:** O-simpleAmbisonics · **Stage:** 1 Foundation · **Date:** 2026-10-12
**Inputs:** `stages/1-foundation/CONTEXT.md`, `stages/1-foundation/RESEARCH.md` (§1–8), `research/ARCHITECTURE.md` §Parameter Mapping, `ROADMAP.md` §Stage 1
**Executor:** foundation-shell-agent

## Goal

A buildable, pluginval-clean **stereo effect shell** with the complete 12-parameter APVTS, the mono/stereo-in → stereo-out bus layout, cross-platform CMake (VST3 + AU + Standalone, WebView2 flags set now), XML state persistence, and the **final latency contract already in place**: `setLatencySamples(64)` with a pass-through chain *input conditioner → 64-sample delay → seeded output gain*. Stage 2 swaps the delay for `SpatialEngine` without changing host-visible timing. Plus the `Source/dsp/` seam and a `tests/render-harness/` scaffold whose Stage-1 probes carry forward into Phase 2.1.

Covers **COMPAT-01** (pluginval s10 VST3 + AU) and **FUNC-07** (input conditioner + bus layout).

## Decisions taken from RESEARCH.md (planner picks)

| Choice | Decision | Source |
|---|---|---|
| Editor shape | **O-Emulator shape**: no editor TU; `createEditor()` returns `new juce::GenericAudioProcessorEditor(*this)` inline in `PluginProcessor.cpp`; harness defines `JUCE_WEB_BROWSER=0` | RESEARCH §5 |
| 64-sample delay | **`Source/dsp/FixedDelay.h`** (`template <int N> struct FixedDelay`, `std::array`, zero alloc); `using DecoderDelay = FixedDelay<kPartition>` | RESEARCH §3b |
| Latency constant | `inline constexpr int kPartition = 64;` in `Source/dsp/SpatialEngine.h`, namespace `OSimpleAmbisonics::dsp` | RESEARCH §3a |
| Output gain | `juce::LinearSmoothedValue<float>`, `reset(fs, 0.02)` + `setCurrentAndTargetValue(decibelsToGain(dB, -60))` in `prepareToPlay`; `getNextValue()` per sample. **Not** `dsp::Gain` | RESEARCH §3d |
| Version stamp | `state.setProperty("pluginVersion", JucePlugin_VersionString, nullptr)` in `getStateInformation` | RESEARCH §4 |
| PLUGIN_CODE | `OSAm` (hex `0x4f53416d`) | CONTEXT decisions |

---

## Tasks

### T1 — `CMakeLists.txt`
**Files:** `plugins/O-simpleAmbisonics/CMakeLists.txt` (create)
**Depends on:** none
**Template:** `plugins/O-simpleSubtractive/CMakeLists.txt` with the §1 deltas from RESEARCH.md.

- `cmake_minimum_required(VERSION 3.15)`; `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`.
- `juce_add_plugin(O-simpleAmbisonics COMPANY_NAME "${OUARICON_COMPANY_NAME}" PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE} PLUGIN_CODE OSAm FORMATS VST3 AU Standalone PRODUCT_NAME "O-simpleAmbisonics${OUARICON_DEV_SUFFIX}" VERSION "1.0.0" IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE NEEDS_WEB_BROWSER TRUE NEEDS_WEBVIEW2 TRUE EDITOR_WANTS_KEYBOARD_FOCUS FALSE)`.
- `target_sources`: `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/dsp/SpatialEngine.h`, `Source/dsp/FixedDelay.h`. **No `PluginEditor.*`** (T4 decision). **No `juce_add_binary_data`**, no `_UIResources` link.
- `target_include_directories(... PRIVATE Source)`.
- Link the standard module list (`juce_audio_basics … juce_gui_extra`, incl. `juce_audio_plugin_client`, `juce_dsp`) PRIVATE; `juce_recommended_config_flags`, `juce_recommended_lto_flags`, `juce_recommended_warning_flags` PUBLIC.
- `juce_generate_juce_header(O-simpleAmbisonics)` **after** `target_link_libraries`.
- Compile defs PUBLIC: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
- `option(OUARICON_BUILD_TESTS "Build O-simpleAmbisonics render-test harness" OFF)` + `if(...) add_subdirectory(tests/render-harness) endif()`.
- Header comment: Stage-1 shell, effect, latency 64 fixed, HRTF data will be a generated header (never a second BinaryData target).
- Root CMake globs `plugins/*` — **no root edit**.

### T2 — DSP seam: `Source/dsp/SpatialEngine.h` + `Source/dsp/FixedDelay.h`
**Files:** `Source/dsp/SpatialEngine.h`, `Source/dsp/FixedDelay.h` (create)
**Depends on:** none (header-only, no JUCE processor dependency; `<array>`, `<cmath>` only)

`FixedDelay.h`:
```cpp
namespace OSimpleAmbisonics::dsp {
template <int N> struct FixedDelay {
    static_assert (N > 0);
    std::array<float, N> buf {}; int idx = 0;
    void reset() noexcept { buf.fill (0.0f); idx = 0; }
    float process (float x) noexcept { const float y = buf[(size_t) idx]; buf[(size_t) idx] = x; if (++idx == N) idx = 0; return y; }
};
}
```
`SpatialEngine.h`:
- `inline constexpr int kPartition = 64;` (the one latency constant — processor, harness, and Stage 2 all read it).
- `using DecoderDelay = FixedDelay<kPartition>;`
- Interface stub for Stage 2, **declared and no-op**, so the seam compiles in the harness today: `struct Params { float azimuthDeg, elevationDeg, distanceM; int order; float yawDeg, pitchDeg, rollDeg; int decoder; bool orbit; float orbitRateHz; };` and `class SpatialEngine { public: void prepare (double fs, int maxBlock); void reset(); void setParams (const Params&); void process (const float* monoIn, float* outL, float* outR, int n); };` with inline empty bodies (`process` writes zeros). Stage 1 **does not call it** from `processBlock`; the pass-through is wired directly (T3). Comment the Stage-2 handoff.

### T3 — `Source/PluginProcessor.h` / `.cpp`
**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` (create)
**Depends on:** T2 (includes `dsp/SpatialEngine.h`, `dsp/FixedDelay.h`)
**Templates:** `git show 62d4c0e8:plugins/O-simpleSubtractive/Source/PluginProcessor.{h,cpp}` (APVTS skeleton); `git show 8ec32da9:plugins/O-Emulator/Source/PluginProcessor.{h,cpp}` (effect shell); `plugins/O-Orbit/Source/PluginProcessor.cpp:323` (bus check).

**Class:** `OSimpleAmbisonicsAudioProcessor : public juce::AudioProcessor`.

**Buses:** `BusesProperties().withInput("Input", stereo(), true).withOutput("Output", stereo(), true)`.
`isBusesLayoutSupported`: reject disabled in/out; input ∈ {mono(), stereo()}; output == stereo(). Nothing else.

**Identity:** `acceptsMidi/producesMidi/isMidiEffect = false`; `getTailLengthSeconds() = 0.0`; `getNumPrograms() = 1`, `getCurrentProgram() = 0`, no-op `setCurrentProgram`, `getProgramName` = `{}`, no-op `changeProgramName`; `hasEditor() = true`; `createEditor()` → `new juce::GenericAudioProcessorEditor (*this)`.

**ParamIDs:** `namespace OSimpleAmbisonics::ParamIDs { inline constexpr auto azimuth = "azimuth"; … }` for all 12. `createParameterLayout()` builds exactly this table (all `juce::ParameterID { id, 1 }`):

| ID | Construction | Default | Label |
|---|---|---|---|
| `azimuth` | `AudioParameterFloat`, `NormalisableRange<float>{-180.f, 180.f, 0.01f}` | 0 | `"°"` |
| `elevation` | `NormalisableRange<float>{-90.f, 90.f, 0.01f}` | 0 | `"°"` |
| `distance` | `NormalisableRange<float> r{0.2f, 20.f, 0.001f}; r.setSkewForCentre(2.0f);` | 1 | `"m"` |
| `order` | `AudioParameterChoice`, `StringArray{"1","2","3"}` | index 0 | |
| `yaw` | `NormalisableRange<float>{-180.f, 180.f, 0.01f}` | 0 | `"°"` |
| `pitch` | `NormalisableRange<float>{-180.f, 180.f, 0.01f}` | 0 | `"°"` |
| `roll` | `NormalisableRange<float>{-180.f, 180.f, 0.01f}` | 0 | `"°"` |
| `decoder` | `AudioParameterChoice`, `StringArray{"Binaural","Stereo"}` | index 0 | |
| `orbit` | `AudioParameterBool` | false | |
| `orbitRate` | `NormalisableRange<float> r{0.02f, 2.f, 0.001f}; r.setSkewForCentre(0.2f);` | 0.1 | `"Hz"` |
| `inputMode` | `AudioParameterChoice`, `StringArray{"Mono sum","Left only"}` | index 0 | |
| `outputLevel` | `NormalisableRange<float>{-60.f, 0.f, 0.1f}` | 0 | `"dB"` |

Skews via `setSkewForCentre` only — **never** a lambda `ValueRemapFunction` (invisible to the Stage-3 WebView slider). Do not hard-code the skew numbers.

**Members:** `juce::AudioProcessorValueTreeState apvts;` `getAPVTS()` accessor; cached `std::atomic<float>*` for `inputMode` and `outputLevel` (`getRawParameterValue`); `dsp::DecoderDelay delay;` `juce::LinearSmoothedValue<float> outputGain;` `int numInputLegs = 2;` `int maxBlock = 0;`.

**`prepareToPlay(fs, samplesPerBlock)`:**
1. `setLatencySamples (dsp::kPartition);` (the only call site; constant forever).
2. `maxBlock = juce::jmax (samplesPerBlock, 1)`.
3. `numInputLegs = getMainInputChannelSet().size()` (1 or 2) — **not** the buffer width.
4. `delay.reset()`.
5. `outputGain.reset (fs, 0.02); outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));` — seeded, no fade-in.

**`releaseResources()`:** no-op.

**`processBlock(buffer, midi)`:**
```
ScopedNoDenormals; n = buffer.getNumSamples(); numOut = jmin(buffer.getNumChannels(), 2);
mode = (int) inputMode->load();  // 0 Mono sum, 1 Left only
outputGain.setTargetValue (Decibels::decibelsToGain (outputLevel->load(), -60.0f));
in0 = buffer.getReadPointer(0); in1 = numInputLegs == 2 ? buffer.getReadPointer(1) : nullptr;
out0 = buffer.getWritePointer(0); out1 = numOut > 1 ? buffer.getWritePointer(1) : nullptr;
for (start = 0; start < n; start += maxBlock) {            // slice loop (Stage 2 needs it)
    len = jmin (maxBlock, n - start);
    for i in [start, start+len):
        s = in1 ? (mode == 0 ? 0.5f*(in0[i]+in1[i]) : in0[i]) : in0[i];   // read L,R BEFORE writing
        y = delay.process (s) * outputGain.getNextValue();
        out0[i] = y; if (out1) out1[i] = y;
}
finite scrub: for each written channel, if any sample !isfinite → clear that channel's block (or per-sample guard).
```
No allocation, no locks, no `updateHostDisplay`. Buffer width is `max(in,out)` = 2 in every accepted layout; writes bounded by `buffer.getNumChannels()`.

**State:** `getStateInformation`: `auto state = apvts.copyState(); state.setProperty ("pluginVersion", JucePlugin_VersionString, nullptr); copyXmlToBinary (*state.createXml(), destData);`. `setStateInformation`: `getXmlFromBinary` → `hasTagName (apvts.state.getType())` → `apvts.replaceState (ValueTree::fromXml (*xml))`.

`createPluginFilter()` returns the processor.

### T4 — Editor (no new files)
**Depends on:** T3
Decision: **no `PluginEditor.{h,cpp}` at Stage 1.** `createEditor()` returns the Generic editor inline (T3). Stage 3 adds `PluginEditor.cpp` and wraps its include + the `createEditor` body in `#if JUCE_WEB_BROWSER … #else Generic #endif` so the harness (`JUCE_WEB_BROWSER=0`) keeps building. Leave a comment at `createEditor()` saying exactly that.

### T5 — Harness scaffold: `tests/render-harness/CMakeLists.txt` + `main.cpp`
**Files:** `tests/render-harness/CMakeLists.txt`, `tests/render-harness/main.cpp` (create)
**Depends on:** T1, T3
**Template:** `git show 8ec32da9:plugins/O-Emulator/tests/render-harness/{CMakeLists.txt,main.cpp}`.

CMake substitutions: console target `O-simpleAmbisonics-render-test`; plugin target **`O-simpleAmbisonics`** (== folder — drop the O-Emulator "target ≠ folder" comment); `add_dependencies` + `$<TARGET_PROPERTY:O-simpleAmbisonics,INCLUDE_DIRECTORIES>`; **keep the derived-version block** (`get_target_property(... JUCE_VERSION)` → hex code, `FATAL_ERROR` if unreadable); defs `JUCE_STANDALONE_APPLICATION=1`, `JUCE_USE_CURL=0`, `JUCE_WEB_BROWSER=0`, `JucePlugin_Build_Standalone=1`, `OUARICON_RENDER_HARNESS=1`, `JucePlugin_Name/Desc="O-simpleAmbisonics"`, `JucePlugin_Manufacturer="Ouaricon Audio Development"`, `JucePlugin_ManufacturerCode=0x4f754476`, **`JucePlugin_PluginCode=0x4f53416d`**, `JucePlugin_IsSynth=0`, `JucePlugin_WantsMidiInput=0`, `JucePlugin_ProducesMidiOutput=0`, `JucePlugin_IsMidiEffect=0`, `JucePlugin_EditorRequiresKeyboardFocus=0`. Sources: `main.cpp` + `../../Source/PluginProcessor.cpp` only; include `../../Source`. Same module link list as O-Emulator's harness (incl. `juce_dsp`).

`main.cpp` probes — each prints `PASS`/`FAIL` and the binary exits nonzero on any FAIL. Position-deterministic noise (`noiseAt(channel, index)` hash, O-Emulator style) so ragged and flat renders are comparable sample-for-sample. `fs = 48000`, default max block 512, `setPlayConfigDetails(2, 2, fs, max)` unless stated.

| Probe | Gate |
|---|---|
| **P0 contract** | 12 params exist by ID with the T3 types/defaults; `order` 3 choices, `decoder` 2, `inputMode` 2; `distance`/`orbitRate` ranges have non-unity skew; after `prepareToPlay`: `getLatencySamples() == dsp::kPartition == 64` |
| **P1 pass-through** | Mono sum, 0 dB, 4096 samples of noise in: `out[c][n] == 0.5f*(L[n-64]+R[n-64])` **bit-equal** for both channels, `out[0..63] == 0`, **`out[64]` exact** (no fade-in) |
| **P1b Left only + mono-in** | `inputMode = 1`, stereo in → `out == L[n-64]`; then `setPlayConfigDetails(1, 2, …)` + re-prepare → `out == in0[n-64]` on both channels |
| **P2 block sizes** | ragged sequence `{1, 7, 64, 333, 4096, …}` over 8192 samples ≡ flat 512-block render, bit-equal |
| **P3 oversized block** | `prepareToPlay(fs, 64)` then one 4096-sample block ≡ `prepareToPlay(fs, 4096)` render, bit-equal (slice-loop gate) |
| **P4 latency** | unit impulse at index 100 → first nonzero output index == `100 + getLatencySamples()` |
| **P5 silence floor** | `outputLevel = -60` set **before** `prepareToPlay` → every output sample exactly 0 (seed is 0, target is 0) |
| **digest** | FNV-1a 64 of the P1 flat render printed as the Stage-1 baseline (recorded in SUMMARY.md) |

No allocator hook yet (PERF-01 lands in Phase 2.1). Set parameters through `apvts.getParameter(id)->setValueNotifyingHost(normalised)` or `getParameterAsValue`, and run a flat-block pre-roll where a smoother target change must settle before a bit-equal compare (P1b).

### T6 — Build, install, validate
**Depends on:** T1–T5
1. `cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON` (reconfigure — new plugin folder must be globbed; from repo root).
2. `ninja -C build O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test` — clean under the recommended warning flags. Long builds in the background (600 s executor watchdog).
3. Run the harness binary (path under `build/plugins/O-simpleAmbisonics/tests/render-harness/`): all probes PASS; note the digest.
4. `./scripts/build-and-install.sh O-simpleAmbisonics` (cache clear + dual-variant sweep) — installs `O-simpleAmbisonics-dev.{vst3,component}`.
5. `auval -v aufx OSAm OuDv` — **not** `auval -a`.
6. pluginval strictness 10 on the installed VST3 and the `.component` (`--timeout-ms 600000`), run twice each (fuzz reseeds).

### T7 — Registry, status, SUMMARY
**Files:** `PLUGINS.md` (row already 🚧 Stage 1 — refresh date/notes only), `.planning/STATUS.md`, `stages/1-foundation/SUMMARY.md` (create)
**Depends on:** T6
- SUMMARY.md: files created, 12-param confirmation, bus layouts accepted, latency 64 measured (P4), harness probe table with PASS, digest, pluginval/auval results, decisions (Generic editor inline, FixedDelay, version stamp), Stage-2 handoff notes (where `SpatialEngine` plugs in, slice loop already present).
- STATUS.md: phase table `execute ✓`, `current_phase: execute`, `next_phase: verify`, `status: stage_1_execute_complete`.
- Commit path-scoped, temp-index (shared checkout): `git commit -- plugins/O-simpleAmbisonics PLUGINS.md`.

## Dependency graph

```
T1 (CMake) ──┐
T2 (dsp seam) ─┴─→ T3 (processor) → T4 (editor decision, inline) ─→ T5 (harness) → T6 (build/validate) → T7 (status)
```
T1 and T2 are parallel-authorable; T3 needs T2's headers; T5 needs T1's option block and T3's processor TU.

## Files

| Action | Path |
|---|---|
| create | `plugins/O-simpleAmbisonics/CMakeLists.txt` |
| create | `plugins/O-simpleAmbisonics/Source/PluginProcessor.h` |
| create | `plugins/O-simpleAmbisonics/Source/PluginProcessor.cpp` |
| create | `plugins/O-simpleAmbisonics/Source/dsp/SpatialEngine.h` |
| create | `plugins/O-simpleAmbisonics/Source/dsp/FixedDelay.h` |
| create | `plugins/O-simpleAmbisonics/Source/ui/.gitkeep` (Stage-3 placeholder) |
| create | `plugins/O-simpleAmbisonics/tests/render-harness/CMakeLists.txt` |
| create | `plugins/O-simpleAmbisonics/tests/render-harness/main.cpp` |
| create | `plugins/O-simpleAmbisonics/.planning/stages/1-foundation/SUMMARY.md` |
| modify | `plugins/O-simpleAmbisonics/.planning/STATUS.md` |
| modify | `PLUGINS.md` (date / note on the existing 🚧 Stage 1 row) |

Not created this stage: `PluginEditor.*`, any `juce_add_binary_data`, `BinauralFilterData.h`, `scripts/generate_binaural_filters.py`, `parameter-spec.md` (mockup finalisation).

## Gotchas checklist (from RESEARCH §8 — executor must tick each)

- [ ] `juce_generate_juce_header()` after `target_link_libraries()`; WebView2 dual flag set now.
- [ ] No `juce_add_binary_data` at foundation.
- [ ] `setLatencySamples(kPartition)` only in `prepareToPlay`.
- [ ] Output smoother **seeded** in `prepareToPlay`; P1 gates `out[64]` explicitly.
- [ ] No allocation on the audio thread — `FixedDelay` is a `std::array`; no `setSize`, no `DelayLine::prepare`.
- [ ] Input leg count from `getMainInputChannelSet().size()` in `prepareToPlay`, not from `buffer.getNumChannels()`; output writes bounded by `buffer.getNumChannels()`.
- [ ] Read inputs for sample *i* before writing outputs for sample *i* (in-place, single loop).
- [ ] `getNumPrograms() == 1`, no-op `setCurrentProgram`.
- [ ] Skews via `setSkewForCentre`; every Choice ≥ 2 entries; no `juce::` name collisions (verified).
- [ ] Harness: `JUCE_WEB_BROWSER=0`, derived version block, no editor TU.
- [ ] Trunk-based, path-scoped, temp-index commit; re-check `git branch --show-current` + `git status --short` immediately before committing.

## Success criteria (verify phase checks these)

- [ ] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU` builds clean.
- [ ] pluginval strictness 10 passes for VST3 **and** AU (COMPAT-01); `auval -v aufx OSAm OuDv` passes.
- [ ] Loads on a stereo and a mono track; stereo-in and mono-in accepted, every other layout rejected.
- [ ] Audio passes through (both outputs carry the conditioned mono source) with exactly 64 samples of latency **reported** (`getLatencySamples`) and **measured** (P4).
- [ ] All 12 parameters visible in the generic editor with the correct ranges, defaults, skews and choice strings.
- [ ] State round-trips (save / reload restores every value; `pluginVersion` property present).
- [ ] No fade-in at the first block after `prepareToPlay` (P1 `out[64]` exact); P3 oversized block bit-equal; no allocation in `processBlock` by inspection.
- [ ] `Source/dsp/SpatialEngine.h` seam + `FixedDelay.h` present; `tests/render-harness/` configures with `-DOUARICON_BUILD_TESTS=ON` and all probes P0–P5 PASS.
