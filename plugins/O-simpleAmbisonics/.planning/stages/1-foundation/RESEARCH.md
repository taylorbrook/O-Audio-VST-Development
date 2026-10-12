# Stage 1 (Foundation) — RESEARCH

**Plugin:** O-simpleAmbisonics
**Date:** 2026-10-11
**Phase:** research
**Depth:** Light-plus — the shell is a sibling-port of SOLVED patterns (O-simpleSubtractive CMake/APVTS, O-Emulator effect shell + Stage-1 harness scaffold). Every JUCE call named below was read in the local 8.0.15 source (`/Users/taylorbrook/JUCE/modules`, pin `.github/juce-version.txt` = 8.0.15). The one piece of real design at this gate is the **pass-through with the final 64-sample latency** — that is where the research effort went.

---

## 1. Templates to copy (verbatim shapes)

| Concern | Template | Why this one |
|---|---|---|
| `CMakeLists.txt` | `plugins/O-simpleSubtractive/CMakeLists.txt` | Same `O-simple*` naming (target == folder), WebView2 flags at foundation, `OUARICON_BUILD_TESTS` block, `juce_generate_juce_header` after `target_link_libraries` |
| APVTS skeleton | `git show 62d4c0e8:plugins/O-simpleSubtractive/Source/PluginProcessor.{h,cpp}` (Stage 1 commit) | `ParamIDs` namespace of `inline constexpr auto`, `ParameterID{id, 1}`, `createParameterLayout()`, XML state round-trip, `getAPVTS()` accessor |
| Effect buses + pass-through + harness scaffold | `git show 8ec32da9:plugins/O-Emulator/...` (Stage 1 commit: processor, `Source/dsp/.gitkeep`, `tests/render-harness/{CMakeLists.txt,main.cpp}`) | The only sibling that shipped a **Stage-1 render-harness scaffold** with pass-through / block-size gates; derives the version code from the target (no fixture mirror) |
| Mono-or-stereo-in → stereo-out bus check | `plugins/O-Orbit/Source/PluginProcessor.cpp:323` | Rejects disabled buses, accepts mono/stereo input, constrains output |
| Seeded output gain | `plugins/O-simpleSubtractive/Source/PluginProcessor.cpp:231-233, 616-619` | `LinearSmoothedValue` reset(fs, 0.02) + `setCurrentAndTargetValue(decibelsToGain(dB, -60))` in prepare; `skip(n)` per block |

Root `CMakeLists.txt:48-56` globs `plugins/*` and `add_subdirectory`s anything with a `CMakeLists.txt` — **no root edit needed**.

### CMake deltas from O-simpleSubtractive
- `juce_add_plugin(O-simpleAmbisonics ...)`: `PLUGIN_CODE OSAm`, `VERSION "1.0.0"`, `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`; keep `FORMATS VST3 AU Standalone`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, the Ouaricon branding vars and the four compile defs.
- `target_sources`: `Source/PluginProcessor.cpp/.h` (+ `Source/dsp/SpatialEngine.h`, `Source/dsp/FixedDelay.h` — see §3). **No `juce_add_binary_data`** and no `_UIResources` link at foundation (sibling precedent; the Stage-3 UI target will be the single binary-data target).
- Same module list incl. `juce_dsp`; `option(OUARICON_BUILD_TESTS ...)` + `add_subdirectory(tests/render-harness)` — the option name is suite-wide and deliberately shared.
- Target name == folder name (`O-simpleAmbisonics`), so `./scripts/build-and-install.sh O-simpleAmbisonics` resolves without the `resolve_cmake_target()` fallback (memory: `build_script_target_name_vs_folder`).

## 2. Bus layout (effect, FUNC-07)

```cpp
: AudioProcessor (BusesProperties()
      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))

bool isBusesLayoutSupported (const BusesLayout& l) const override
{
    const auto in  = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();
    if (in.isDisabled() || out.isDisabled()) return false;             // O-Orbit shape
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    return out == juce::AudioChannelSet::stereo();
}
```

- An effect **must** declare an input bus or VST3 reports "instrument category, but no valid event input bus" (`troubleshooting/runtime-issues/vst3-bus-config-effect-missing-input-Shake-20251106.md`; `juce8-critical-patterns.md` §4).
- Logic caches AU I/O config **per version** (memory `critical_logic_caches_au_io_config_per_version`): the mono-in/stereo-in acceptance must be right in 1.0.0 — it is, by the check above.
- Buffer width in `processBlock` is `max(totalIn, totalOut)` = 2 in every accepted layout. Read the input leg count from `getMainInputChannelSet().size()` captured in `prepareToPlay` (or `getTotalNumInputChannels()`), **not** from `buffer.getNumChannels()` (memories `pattern_stereo_in_mono_out_buffer_width_lies`, `pattern_standalone_canonical_channelset_oob`). With a mono input bus the conditioner reads `in[0]` regardless of `inputMode`; both outputs are always written, so no "clear extra channels" loop is needed, but keep the write bounded by `buffer.getNumChannels()`.
- Standalone on a 2-out device negotiates `stereo()` → fine; Standalone with a mono input device negotiates `mono()` input → accepted.

## 3. The pass-through chain (conditioner → 64-sample delay → seeded gain)

### 3a. Latency contract — `setLatencySamples(64)`
`AudioProcessor::setLatencySamples` (`juce_audio_processors_headless/.../juce_AudioProcessor.cpp:415`) stores the value and calls `updateHostDisplay(...withLatencyChanged(true))` **only when it changes**; `getLatencySamples()` is a non-virtual inline getter (`.h:846`). Call it once in `prepareToPlay` with the constant `kPartition = 64` (project memory: JUCE 8 `getLatencySamples()` non-virtual). It never changes afterwards — no audio-thread calls (O-AnalogSaturation's `WR-01` comment: `updateHostDisplay` is not RT-safe).

Put the constant in the DSP seam so Stage 2 reuses it: `namespace OSimpleAmbisonics::dsp { inline constexpr int kPartition = 64; }` in `Source/dsp/SpatialEngine.h`.

### 3b. The delay — recommendation: hand-rolled `Source/dsp/FixedDelay.h`
The delayed signal is **mono** (the conditioned source); both outputs copy it. A fixed integer delay of exactly 64 samples needs no interpolation and no runtime length:

```cpp
// Source/dsp/FixedDelay.h — headless, no JUCE processor dependency
template <int N> struct FixedDelay {
    std::array<float, N> buf {}; int idx = 0;
    void reset() noexcept { buf.fill (0.0f); idx = 0; }
    float process (float x) noexcept { const float y = buf[(size_t) idx]; buf[(size_t) idx] = x; if (++idx == N) idx = 0; return y; }
};
using DecoderDelay = FixedDelay<kPartition>;
```
Why over `juce::dsp::DelayLine`: (1) zero allocation and no `prepare(ProcessSpec)`; (2) exact integer delay by construction — `DelayLine::setDelay` floors/limits against `totalSize - 2` and `setMaximumDelayInSamples` allocates `max(4, N+2)` (`juce_DelayLine.cpp:57-67, 93-99`), all unnecessary here; (3) no push/pop lockstep trap (memory `critical_delayline_push_without_pop_shifts_delay`); (4) it is the same object Stage 2's `DecoderMixer.h` needs for the stereo path ("64-sample stereo delay"), so it survives unchanged — write it in `Source/dsp/` now and the Phase 2.1 planner lifts it. **Alternative** if the planner prefers suite stock: `juce::dsp::DelayLine<float, DelayLineInterpolationTypes::None> d { 64 }`, `prepare({fs, maxBlock, 1})`, `setDelay(64.f)`, then per sample `y = d.popSample(0); d.pushSample(0, x);` (pop-then-push, one each per sample — O-AnalogSaturation `dryDelay` precedent, `PluginProcessor.h:266`).

### 3c. Input conditioner (ARCHITECTURE §Input Conditioner)
Per block: `inLegs = numInputChannels` (1 or 2). Per sample: `s = inLegs == 2 ? (inputMode == MonoSum ? 0.5f·(L+R) : L) : in0`. Read `inputMode` once per block via `getRawParameterValue(...)->load()`. Because the delay is mono and the outputs are written from the delayed mono, the conditioner can read `in[0]/in[1]` from the same buffer the outputs are written to **only if** the read of sample *i* happens before the write of sample *i* — true in a single per-sample loop (read L,R → delay → write L,R). No scratch buffer is needed for the pass-through; keep the `max(samplesPerBlock, 64)` mono work buffer out of Stage 1 (nothing uses it yet; Stage 2 sizes the SH bus itself).

### 3d. Output gain — seeded, −60 dB = silence
`juce::LinearSmoothedValue<float> outputGain;` in `prepareToPlay`: `outputGain.reset (sampleRate, 0.02); outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (dB, -60.0f));` (`decibelsToGain(x, minusInfinityDb)` returns exactly 0 for `x <= -60`). Per block: `setTargetValue(...)` then either `getNextValue()` per sample or the sibling's `g0 = getCurrentValue(); g1 = skip(n)` + per-sample lerp. **Seeding is mandatory** — an unseeded smoother with a nonzero ramp fades 0 → target over the first 20 ms at every `prepareToPlay` (memory `pattern_gain_ramp_without_seeding_fades_in_from_silence`, proven on O-AnalogEQ v1.5.1; `SmoothedValue::reset` copies *target* into *current*, and the default target is 0). Do **not** use `juce::dsp::Gain` (same object, same trap, plus a ProcessSpec it does not need).

### 3e. Oversized host blocks
With `FixedDelay` and a per-sample loop there is **no per-block scratch buffer**, so a host block larger than `samplesPerBlock` cannot overrun anything (memory `pattern_oversampler_overruns_on_oversized_host_block`: the overrun comes from prepared-size scratch buffers, not from the loop). Still write the slice loop now — `for (start = 0; start < n; start += maxBlock) { len = min(maxBlock, n - start); ... }` — because Stage 2's engine *will* have prepared-size buses and ARCHITECTURE §Processing Chain promises "the processor loops the whole chain over ≤ maxBlock slices". Having the loop in place at Stage 1 means the Stage-1 harness gate (prepare 64, feed 4096 ≡ prepare 4096) carries forward unchanged.

### 3f. Block-level finite scrub
`for each out channel: if (! std::isfinite(sum)) clear block` or per-sample `std::isfinite` guard — cheap and it is what COMPAT-01's strictness-10 fuzz looks for (memory `pattern_ci_pluginval10_catches_latent_nan`). The pass-through cannot produce a NaN unless the host feeds one, but the scrub is part of the final chain and should exist from day one.

### 3g. Per-block skeleton
```
ScopedNoDenormals; n = numSamples; inLegs; mode = inputMode (atomic); outputGain.setTargetValue(decibelsToGain(outputLevel, -60));
for slice in [0, n) step maxBlock:
    for i in slice: s = condition(in, i); y = delay.process(s); g = outputGain.getNextValue(); out0[i] = out1[i] = y·g;
finite scrub
```
(Nothing in this stage needs the slice loop to be visible to the gain smoother — `getNextValue()` is per sample either way.)

## 4. APVTS — the 12 parameters (ARCHITECTURE §Parameter Mapping)

| ID | Construction (JUCE 8.0.15, all `ParameterID{id, 1}`) |
|---|---|
| `azimuth`, `yaw`, `pitch`, `roll` | `AudioParameterFloat`, `NormalisableRange<float>{-180.f, 180.f, 0.01f}`, default 0, label `"°"` |
| `elevation` | `NormalisableRange<float>{-90.f, 90.f, 0.01f}`, default 0, `"°"` |
| `distance` | `NormalisableRange<float> r {0.2f, 20.f, 0.001f}; r.setSkewForCentre (2.0f);` default 1, label `"m"` |
| `order` | `AudioParameterChoice`, `StringArray{"1", "2", "3"}`, default index 0 |
| `decoder` | `AudioParameterChoice`, `StringArray{"Binaural", "Stereo"}`, default 0 |
| `orbit` | `AudioParameterBool`, default false |
| `orbitRate` | `NormalisableRange<float> r {0.02f, 2.f, 0.001f}; r.setSkewForCentre (0.2f);` default 0.1, label `"Hz"` |
| `inputMode` | `AudioParameterChoice`, `StringArray{"Mono sum", "Left only"}`, default 0 |
| `outputLevel` | `NormalisableRange<float>{-60.f, 0.f, 0.1f}`, default 0, label `"dB"` |

Verified facts:
- `NormalisableRange::setSkewForCentre` (`juce_NormalisableRange.h:210-218`) asserts `start < centre < end`, sets `symmetricSkew = false`, `skew = log(0.5)/log((c−start)/(end−start))`. Skews are **transmitted** to the Stage-3 WebView slider frontend; lambda `ValueRemapFunction` ranges are not (memory `critical_lambda_normalisable_range_invisible_to_webview_slider_frontend`) — so skew-based ranges, never lambda ranges. The resulting skews are `distance` = `log(0.5)/log(1.8/19.8) ≈ 0.289` and `orbitRate` = `log(0.5)/log(0.18/1.98) ≈ 0.289` (same centre fraction by coincidence); let `setSkewForCentre` compute them — do not hard-code.
- Every Choice has ≥ 2 entries (memory `critical_choice_param_needs_two_choices`).
- Scanned all JUCE 8.0.15 module headers for free functions or types named `azimuth/elevation/distance/order/yaw/pitch/roll/decoder/orbit/orbitRate/inputMode/outputLevel`: **no collisions** (only comment/word hits in zlib, FLAC, sheenbidi). `using namespace OSimpleAmbisonics::ParamIDs;` is safe (memory `critical_paramid_shadows_juce_free_function`).
- Interval choices (0.01° / 0.001 m / 0.001 Hz / 0.1 dB) are display granularity only; the DSP reads the raw float. A range change after 1.0.0 would need a preset-migration gate, so the ranges above are final.
- `getNumPrograms()` returns 1 with a no-op `setCurrentProgram` (every sibling: O-Gain, O-Strata, O-Emulator, O-simpleWavetable). The Ableton state doc (`troubleshooting/parameter-issues/ableton-preset-interference-...`) is satisfied by the no-op.
- State: `apvts.copyState().createXml()` → `copyXmlToBinary`; restore via `hasTagName(state.getType())` + `replaceState`. O-Emulator additionally stamps `state.setProperty("pluginVersion", JucePlugin_VersionString, nullptr)` at v1.0.0 — recommended (free now, every house migration gate keys off it).

## 5. Editor at foundation — recommendation: O-Emulator shape

Two sibling shapes exist:
- **O-simpleSubtractive:** `PluginEditor.{h,cpp}` hosting a `juce::GenericAudioProcessorEditor`; the harness compiles the editor TU with `JUCE_WEB_BROWSER=1` and links the UI BinaryData — which only works once the UI target exists.
- **O-Emulator (recommended):** no editor TU at Stage 1; `createEditor()` returns `new juce::GenericAudioProcessorEditor (*this)` inline in `PluginProcessor.cpp`; the harness defines `JUCE_WEB_BROWSER=0` and never lists an editor TU. Stage 3 then adds `PluginEditor.cpp` and wraps both the include and the `createEditor` body in `#if JUCE_WEB_BROWSER` with the Generic editor as the `#else` (memory `pattern_render_harness_breaks_on_webview_editor`), so the harness keeps building through Stage 3/4 without ever linking BinaryData.

CONTEXT.md item 3 ("placeholder editor sufficient to load") is met by either; the O-Emulator shape has one fewer file and one fewer trap. If the planner wants the explicit `PluginEditor.{h,cpp}` pair for sibling symmetry, keep the harness on the O-Emulator CMake pattern anyway and guard `createEditor` from day one.

## 6. Harness scaffold — `tests/render-harness/`

Copy O-Emulator's Stage-1 `CMakeLists.txt` + `main.cpp` (commit `8ec32da9`) with these substitutions:
- Target `O-simpleAmbisonics-render-test`, plugin target `O-simpleAmbisonics` (== folder, unlike `OEmulator`), `JucePlugin_PluginCode=0x4f53416d` (`'OSAm'`), `JucePlugin_IsSynth=0`, `JucePlugin_WantsMidiInput=0`, `JUCE_WEB_BROWSER=0`, `OUARICON_RENDER_HARNESS=1`.
- Keep the **derived version block** (`get_target_property(... JUCE_VERSION)` → hex code) — never mirror the version string (memory `pattern_test_fixture_mirrors_drift_silently`).
- Sources: `main.cpp` + `../../Source/PluginProcessor.cpp` only; `Source/dsp/*.h` are header-only and reach in via the include dir.

Stage-1 probes (the scaffold Phase 2.1 inherits):

| Probe | Gate |
|---|---|
| P0 contract | 12 params exist with the §4 types/defaults; `order` 3 choices, `decoder` 2, `inputMode` 2; `getLatencySamples() == 64` after `prepareToPlay` |
| P1 pass-through | position-deterministic noise in (O-Emulator `noiseAt`), `out[n] == 0.5·(L[n−64]+R[n−64])` bit-equal (Mono sum, 0 dB); first 64 output samples exactly 0; **`out[64]` exact — no fade-in** |
| P1b `Left only` + mono-in | `setPlayConfigDetails(1, 2, fs, max)` → `out == in0[n−64]` on both channels |
| P2 block sizes | ragged `{1, 7, 64, 333, 4096}` ≡ flat 512, bit-equal |
| P3 oversized block | `prepareToPlay(fs, 64)` then one 4096-sample block ≡ `prepareToPlay(fs, 4096)` render, bit-equal (slice-loop gate from memory `pattern_oversampler_overruns…`) |
| P4 latency | impulse → first nonzero index == `getLatencySamples()` == 64 (ARCHITECTURE §Latency gate) |
| P5 silence floor | `outputLevel = −60` → all-zero output after 64 + ramp samples |
| digest | FNV-1a 64 of the flat render printed as the Stage-1 baseline |

Allocation counting (PERF-01) lands in Phase 2.1 — the scaffold needs no allocator hook yet.

## 7. Verification tooling (for the verify phase)

- Build/install: `./scripts/build-and-install.sh O-simpleAmbisonics` (cache clear + dual-variant sweep). Dev branding installs `O-simpleAmbisonics-dev.{vst3,component}`.
- pluginval: `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3` (and the `.component`) — COMPAT-01 is strictness **10**, same as CI (`build-and-release.yml`). Run 2–3× (fuzz reseeds).
- AU: targeted `auval -v aufx OSAm OuDv` — **not** `auval -a` (memory: `auval -a` SIGABRTs here). Effect type is `aufx`.
- Harness: `cmake -B build -DOUARICON_BUILD_TESTS=ON && ninja O-simpleAmbisonics-render-test && ./build/plugins/O-simpleAmbisonics/tests/render-harness/O-simpleAmbisonics-render-test_artefacts/...` (path per `juce_add_console_app`; check `build/` after configure).

## 8. Gotchas honored (checklist for plan/execute)

- `juce_generate_juce_header()` after `target_link_libraries()`; WebView2 dual flag set now (sibling precedent).
- No `juce_add_binary_data` at foundation; the HRTF data is a generated header later — never a second BinaryData target (memory `critical_dual_binary_data_namespace_collision`).
- `setLatencySamples(64)` only in `prepareToPlay`; constant forever.
- Seed the output smoother in `prepareToPlay`; gate `out[64]` explicitly (a mid-render step never sees block 0).
- No allocation / no `DelayLine::prepare` / no `setSize` on the audio thread; `FixedDelay` is a `std::array`.
- Bound output writes by `buffer.getNumChannels()`; take input leg count from the negotiated input bus, not the buffer width.
- `getNumPrograms() == 1`, no-op `setCurrentProgram`.
- Trunk-based, path-scoped commits: `git commit -- plugins/O-simpleAmbisonics PLUGINS.md`; shared checkout → temp-index commit (memory `index_git_shared_checkout`).
- Version 1.0.0 bus layout is what Logic will cache — it is correct above.

## 9. Open questions

**None blocking.** One planner choice, recommendation given: editor shape (O-Emulator inline Generic editor, §5) and delay implementation (`FixedDelay<64>` in `Source/dsp/`, §3b). Both are reversible within the stage and neither touches a contract.
