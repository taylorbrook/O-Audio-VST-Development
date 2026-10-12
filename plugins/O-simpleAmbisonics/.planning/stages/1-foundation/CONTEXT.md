# Stage 1 (Foundation) — CONTEXT

**Plugin:** O-simpleAmbisonics
**Date:** 2026-10-11
**Phase:** discuss
**Source:** Compiled from locked Stage-0 contracts (BRIEF.md, REQUIREMENTS.md, research/ARCHITECTURE.md, ROADMAP.md, parameter-spec-draft.md, stages/0-ideation/CONTEXT.md). No open DSP/UI decisions at this gate — the foundation scope is fully determined by contracts. Same precedent as O-simpleSubtractive and O-simpleFM Stage 1.

---

## Stage Goal

A **buildable, pluginval-clean stereo effect shell** with the complete 12-parameter APVTS, the correct mono/stereo-in → stereo-out bus layout, cross-platform CMake (VST3 + AU on macOS, VST3 on Windows, WebView2 flags set now), state persistence, and the **latency contract already in place** (`setLatencySamples(64)`). `processBlock` is a pass-through: input conditioner → 64-sample delay → seeded output gain → stereo out. No encoder, rotation, or decoder yet — Stage 2 replaces the pass-through with `SpatialEngine` and the host-visible timing never changes.

## Scope (in)

1. **CMakeLists.txt** — mirror O-simpleSubtractive (primary template), adjusted to an effect:
   - `juce_add_plugin` with `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `FORMATS VST3 AU Standalone`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`.
   - Ouaricon branding vars: `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`, `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`, `PRODUCT_NAME "O-simpleAmbisonics${OUARICON_DEV_SUFFIX}"`, `VERSION "1.0.0"`.
   - **`PLUGIN_CODE OSAm`** — the Stage 0 suggestion `OSiA` is already taken by O-simpleAdditive (verified by grep across all 44 `plugins/*/CMakeLists.txt`); `OSAm` is unused.
   - Compile defs: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
   - Link `juce_dsp` (plus the standard module list); `juce_generate_juce_header()` **after** `target_link_libraries()`.
   - **No `juce_add_binary_data` at Foundation** (sibling precedent — the UI target is added in Stage 3). When it is added it will be the only binary-data target; the HRTF filters are a generated constexpr header (`Source/dsp/BinauralFilterData.h`, Stage 2.3), never a second BinaryData target.
   - `option(OUARICON_BUILD_TESTS ...)` + `add_subdirectory(tests/render-harness)` scaffold.
2. **PluginProcessor (.h/.cpp)**:
   - `BusesProperties().withInput("Input", stereo).withOutput("Output", stereo)`; `isBusesLayoutSupported`: input ∈ {mono, stereo}, output == stereo, nothing else.
   - APVTS with all **12 parameters** exactly per ARCHITECTURE.md §Parameter Mapping (table below).
   - `prepareToPlay`: `setLatencySamples(64)`; size the 64-sample stereo delay line; seed the output-gain smoother to the current `outputLevel` (no fade-in from silence — project memory); size an internal mono work buffer for `max(samplesPerBlock, 64)`.
   - `processBlock`: `ScopedNoDenormals`; read params once per block; input conditioner (`inputMode`: Mono sum = `0.5·(L+R)`, Left only = `L`; a mono input bus uses `in[0]` regardless) → 64-sample delay → output gain (dB → linear, 20 ms ramp, −60 dB = silence) → write the same mono signal to both outputs; block-level `isfinite` scrub. The conditioner + delay + gain stage are the parts of the final chain that survive into Stage 2 unchanged.
   - Host blocks larger than `samplesPerBlock`: the pass-through must not overrun (project memory: oversized host blocks) — either a slice loop or a delay line sized to the max of the two.
   - State: `apvts.state` → `getStateInformation` / `setStateInformation` XML round-trip. No custom state in v1.0.
3. **PluginEditor (.h/.cpp)** — minimal placeholder editor sufficient to load (generic/empty `AudioProcessorEditor`, sibling Stage 1 pattern). Full WebView editor is Stage 3.
4. **Project structure** — `Source/`, `Source/dsp/` with an empty `SpatialEngine.h` seam (headless-testable interface stub: `prepare(double fs, int maxBlock)` / `reset()` / `process(...)` / `setParams(const Params&)`), `Source/ui/` placeholder, `tests/render-harness/` CMake scaffold that links `Source/dsp/*.h` directly (sibling pattern).
5. **Registry / status** — PLUGINS.md row → 🚧 Stage 1; STATUS.md phase table.

## Scope (out — deferred to later stages)

- All DSP: encoder, distance model, wrap smoother, orbit LFO, rotator, MagLS pipeline, UPOLS convolver, stereo decoder, decoder mixer, meter tap → **Stage 2** (phases 2.1 / 2.2 / 2.3).
- WebView UI, relays/attachments, visualizations, tooltips, i18n → **Stage 3**.
- Factory preset tour, Logic/Ableton hands-on, listening check, CHANGELOG → **Stage 4**.
- `scripts/generate_binaural_filters.py` and `BinauralFilterData.h` → **Stage 2.3** (not needed to build the shell).

## The 12 parameters (authoritative — ARCHITECTURE.md §Parameter Mapping)

| ID | Type | Range / Choices | Default | Notes |
|---|---|---|---|---|
| `azimuth` | Float | −180 … +180 ° | 0 | 0 front, +90 **left** (AmbiX), ±180 behind; wraps |
| `elevation` | Float | −90 … +90 ° | 0 | +90 overhead |
| `distance` | Float | 0.2 … 20 m, log skew (centre ≈ 2 m) | 1 | |
| `order` | Choice | `1` / `2` / `3` | `1` | 4 / 9 / 16 channels |
| `yaw` | Float | −180 … +180 ° | 0 | wraps |
| `pitch` | Float | −180 … +180 ° | 0 | wraps |
| `roll` | Float | −180 … +180 ° | 0 | wraps |
| `decoder` | Choice | `Binaural` / `Stereo` | `Binaural` | |
| `orbit` | Bool | Off / On | Off | |
| `orbitRate` | Float | 0.02 … 2 Hz, log skew | 0.1 | |
| `inputMode` | Choice | `Mono sum` / `Left only` | `Mono sum` | |
| `outputLevel` | Float | −60 … 0 dB (−60 = −inf) | 0 | never set by presets |

Choice parameters all have ≥ 2 choices (project memory: a Choice needs ≥ 2). Parameter IDs must not shadow `juce::` free functions or types (project memory) — none of the twelve do. Skews: `distance` and `orbitRate` log-shaped via `NormalisableRange` with `setSkewForCentre` (2 m / ~0.2 Hz) — a plain skew, **not** a lambda range (project memory: lambda ranges are invisible to the WebView slider frontend in Stage 3).

## Requirements covered this stage

- **COMPAT-01** — pluginval strictness 10 passes for VST3 and AU.
- **FUNC-07** — stereo input summed to the single mono source (`Mono sum` = `0.5·(L+R)`, documented −6 dB law) or `Left only`; mono-in / stereo-out layout accepted.
- Foundation contracts for Stage 2/3: 12-param APVTS present and persisted; `setLatencySamples(64)` reported from day one; `Source/dsp/` seam + harness scaffold.

## Constraints honored

- JUCE 8.0.15 (pin `.github/juce-version.txt`); CMake + Ninja; local JUCE at `/Users/taylorbrook/JUCE`.
- macOS: build/install VST3 + AU via `./scripts/build-and-install.sh O-simpleAmbisonics` (cache-clear + dual-variant sweep). Windows: VST3 only.
- RT-safe `processBlock` (no alloc/lock/file I/O) even as a pass-through; `ScopedNoDenormals`.
- `getLatencySamples()` is non-virtual → `setLatencySamples(64)` in `prepareToPlay` (project memory).
- Output gain ramp seeded in `prepareToPlay` (project memory: unseeded `dsp::Gain` fades in from silence).
- Logic caches AU I/O config per version — bus layout must be right in 1.0.0 (project memory).
- Trunk-based, path-scoped commits: `git commit -- plugins/O-simpleAmbisonics PLUGINS.md`.

## Decisions made at this gate

| Decision | Choice | Rationale |
|---|---|---|
| `parameter-spec.md` missing (draft only) | Proceed; ARCHITECTURE.md §Parameter Mapping is the authoritative 12-param contract | O-simpleSubtractive and O-simpleFM both ran Stage 1 with only the draft; the mockup (Stage 3 input) must not add parameters, so the set is already locked |
| PLUGIN_CODE | `OSAm` | `OSiA` taken by O-simpleAdditive; `OSAm` verified unused |
| Foundation pass-through | conditioner → 64-sample delay → seeded gain | Host-visible latency and bus behaviour are final from day one; Stage 2 swaps the delay for the engine without changing timing |
| Editor at Foundation | placeholder, no WebView | Sibling precedent; avoids a BinaryData target with nothing to embed |

## Open decisions

**None at the contract level.** One process choice for the orchestrator: workflow pace through Stage 1 (manual phase-by-phase vs express). Surfaced at the discuss handoff.

## Sibling references (read at research / execute)

- O-simpleSubtractive — CMake, processor/editor skeleton, APVTS layout, `tests/render-harness` CMake pattern (adapt from synth to effect).
- O-simpleWavetable — latest WebView + gate conventions (Stage 3 input, note only).
- O-GrainScatter — `Source/dsp/SpatialEncoder.h`, `TripleBuffer.h` (Stage 2 input, note only).
- An existing stereo **effect** in the suite (e.g. O-Freeze or O-Gain) for `BusesProperties` / `isBusesLayoutSupported` effect boilerplate.

## Success criteria (verify phase will check)

- [ ] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU` builds clean (warnings-as-recommended-flags).
- [ ] pluginval strictness 10 passes for VST3 and AU (COMPAT-01); `auval -v` passes.
- [ ] Loads on a **stereo** and a **mono** track; stereo-in and mono-in layouts accepted, nothing else.
- [ ] Audio passes through (both outputs carry the conditioned mono source) with **exactly 64 samples** of latency reported and measured.
- [ ] All 12 parameters appear in the host's generic parameter list with the correct ranges, defaults, and choice strings.
- [ ] Parameter state round-trips (save / reload restores every value).
- [ ] No fade-in from silence at first `processBlock` after `prepareToPlay`; no allocation in `processBlock`; host block larger than `samplesPerBlock` handled.
- [ ] `Source/dsp/SpatialEngine.h` seam and `tests/render-harness/` scaffold present and the harness target configures with `-DOUARICON_BUILD_TESTS=ON`.
