# Stage 1 (Foundation) — PLAN

**Plugin:** O-simpleSubtractive
**Date:** 2026-06-25
**Phase:** plan
**Execute agent:** foundation-shell-agent (tools: Read/Write/Edit + context7 — **no Bash**; orchestrator runs the build in verify)

---

## Goal

Produce a **silent, loadable, pluginval-valid synth shell** for O-simpleSubtractive: CMake (VST3+AU+Standalone, synth, MIDI-in, WebView2 flags), a complete **20-parameter APVTS** matching ARCHITECTURE.md §Parameter Mapping exactly, XML state persistence, and a `GenericAudioProcessorEditor` so every parameter is visible/testable. No DSP — `processBlock` outputs silence. This is the contract surface Stage 2 (DSP) and Stage 3 (GUI) bind against.

## Tasks

### T1 — CMakeLists.txt
**File:** `plugins/O-simpleSubtractive/CMakeLists.txt` (create)
Mirror `plugins/O-simpleFM/CMakeLists.txt` foundation form:
- `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`.
- `juce_add_plugin(O-simpleSubtractive ...)`: `PLUGIN_CODE OSiS`, `PRODUCT_NAME "O-simpleSubtractive${OUARICON_DEV_SUFFIX}"`, `VERSION "1.0.0"`, `COMPANY_NAME`/`PLUGIN_MANUFACTURER_CODE` via Ouaricon vars, `FORMATS VST3 AU Standalone`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`.
- `target_sources`: `PluginProcessor.cpp/.h`, `PluginEditor.cpp/.h`.
- `target_include_directories(... PRIVATE Source)`.
- JUCE module links (incl. `juce_dsp`, `juce_gui_extra`, `juce_audio_utils`) + `juce::juce_recommended_*` PUBLIC.
- `juce_generate_juce_header(O-simpleSubtractive)` **after** link libraries.
- `target_compile_definitions(... PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_WEB_BROWSER=1 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 JUCE_USE_CURL=0)`.
- **Do NOT** add `juce_add_binary_data` (Stage 3), `ouaricon_add_module` (Stage 4), or the test harness `add_subdirectory` (Stage 2).
**Depends:** none.

### T2 — PluginProcessor.h
**File:** `plugins/O-simpleSubtractive/Source/PluginProcessor.h` (create)
- Parameter-ID block: `static constexpr const char*` (or `juce::ParameterID`) for all 20 IDs — single source of truth, exact IDs from §Parameters below.
- Class `OSimpleSubtractiveAudioProcessor : public juce::AudioProcessor`.
- Standard overrides; `getStateInformation`/`setStateInformation`; `acceptsMidi()=true`, `producesMidi()=false`, `isMidiEffect()=false`.
- `juce::AudioProcessorValueTreeState parameters;` + `getAPVTS()` accessor.
- `static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();`.
**Depends:** T1 (names).

### T3 — PluginProcessor.cpp
**File:** `plugins/O-simpleSubtractive/Source/PluginProcessor.cpp` (create)
- Ctor: `parameters(*this, nullptr, "PARAMETERS", createParameterLayout())`.
- `createParameterLayout()` — build all 20 params per §Parameters (exact ranges/defaults/skews). Use `ParameterID{id, 1}` 2-arg form (versionHint=1).
- `prepareToPlay`: `setLatencySamples(0)`; (reserve smoothing setup for Stage 2 — no-op now).
- `processBlock`: `juce::ScopedNoDenormals`; clear output channels (incl. any beyond input count); ignore MIDI (consume, no synthesis). **Silence.**
- `getStateInformation`: `copyXmlToBinary(*parameters.copyState().createXml(), destData)`.
- `setStateInformation`: restore via `getXmlFromBinary` → `parameters.replaceState(ValueTree::fromXml(...))` (guard tag name).
- `createEditor()` → `new juce::GenericAudioProcessorEditor(*this)`; `hasEditor()=true`.
- `createPluginFilter()` factory returns the processor.
**Depends:** T2.

### T4 — PluginEditor.h/.cpp
**Files:** `plugins/O-simpleSubtractive/Source/PluginEditor.{h,cpp}` (create)
- Foundation: thin wrapper is optional — simplest is to skip a custom editor class and return `GenericAudioProcessorEditor` directly from the processor (T3). **Decision:** return `GenericAudioProcessorEditor` from `createEditor()` and still create stub `PluginEditor.h/.cpp` files (referenced in CMake target_sources) OR drop them from CMake. **Chosen:** keep `target_sources` to processor only is cleaner — but to match sibling layout and ease Stage 3, create a minimal `OSimpleSubtractiveAudioProcessorEditor` that just hosts a `GenericAudioProcessorEditor` child sized to fill, so Stage 3 swaps its body for WebView. Either is acceptable; agent picks the lower-risk one and notes it in SUMMARY.md.
**Depends:** T3.

### T5 — SUMMARY.md
**File:** `plugins/O-simpleSubtractive/.planning/stages/1-foundation/SUMMARY.md` (create)
- What was built, files created, the 20 params as implemented, editor choice (T4), and any deviations. Template: summary-standard.
**Depends:** T1–T4.

## §Parameters (authoritative — ARCHITECTURE.md §Parameter Mapping)

| ID | JUCE type | Range / Choices | Default | Skew |
|----|-----------|-----------------|---------|------|
| `oscWave` | Choice | [Saw, Square, Triangle, Sine] | Saw (0) | — |
| `subLevel` | Float | 0..1 (disp 0–100%) | 0 | lin |
| `noiseLevel` | Float | 0..1 (disp 0–100%) | 0 | lin |
| `filterType` | Choice | [LP, HP, BP, Notch] | LP (0) | — |
| `filterSlope` | Choice | [6 dB/oct, 12 dB/oct, 24 dB/oct] | 24 (2) | — |
| `cutoff` | Float | 20..20000 Hz | 2000 | log (skew ≈0.25) |
| `resonance` | Float | 0..1 (disp 0–100%) | 0.10 | lin |
| `filterEnvAmount` | Float | −1..+1 (disp −100..+100%) | +0.50 | lin, bipolar |
| `keyTrack` | Float | 0..1 (disp 0–100%) | 0 | lin |
| `filterAttack` | Float | 0..5 s | 0.005 | skew 0.35 |
| `filterDecay` | Float | 0..5 s | 0.30 | skew 0.35 |
| `filterSustain` | Float | 0..1 (disp 0–100%) | 0.40 | lin |
| `filterRelease` | Float | 0..5 s | 0.20 | skew 0.35 |
| `ampAttack` | Float | 0..5 s | 0.005 | skew 0.35 |
| `ampDecay` | Float | 0..5 s | 0.30 | skew 0.35 |
| `ampSustain` | Float | 0..1 (disp 0–100%) | 0.80 | lin |
| `ampRelease` | Float | 0..5 s | 0.10 | skew 0.35 |
| `voiceMode` | Choice | [Poly, Mono, Legato] | Poly (0) | — |
| `glide` | Float | 0..1 s | 0 | skew 0.5 |
| `outputLevel` | Float | −60..0 dB (−inf≈−60 floor) | 0 | lin (dB) |

> Display labels/units (%, Hz, s, dB) via `AudioParameterFloat` lambdas where it aids the host generic view; not load-bearing at foundation.

## Files created
- `plugins/O-simpleSubtractive/CMakeLists.txt`
- `plugins/O-simpleSubtractive/Source/PluginProcessor.h`
- `plugins/O-simpleSubtractive/Source/PluginProcessor.cpp`
- `plugins/O-simpleSubtractive/Source/PluginEditor.h` (+ `.cpp`) — or processor-returned generic editor
- `plugins/O-simpleSubtractive/.planning/stages/1-foundation/SUMMARY.md`

## Success criteria (verify phase)
1. Configure + `ninja O-simpleSubtractive_VST3 O-simpleSubtractive_AU` build clean (no warnings-as-errors failures).
2. `auval -v aumu OSiS OuDv` (dev) / pluginval `--strictness-level 5` pass shell checks.
3. All **20** parameters present with correct ranges/defaults in the host generic editor.
4. Parameter state round-trips: set values → save state → reload → values restored.
5. `processBlock` outputs silence; no crash on MIDI input; no denormal stalls.
6. `PLUGIN_CODE OSiS` unique; loads as an **instrument** (aumu/synth category).

## Out of scope (later stages)
DSP (oscillators/filter/ADSR/voices) → Stage 2 · WebView UI + viz → Stage 3 · presets/optimization → Stage 4.
