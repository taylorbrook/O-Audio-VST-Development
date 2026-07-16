# Stage 1 (Foundation + Shell) — Execution Plan

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 1 of 4 (Foundation + Shell)
**Phase:** plan
**Date:** 2026-06-26
**Inputs:** `stages/1-foundation/CONTEXT.md`, `stages/1-foundation/RESEARCH.md`, `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md`
**Template:** O-simpleFM (clone skeleton, apply the 4 divergences — RESEARCH §1)

---

## Goal

Stand up the silent synth shell: CMake target + full **17-param APVTS** (IDs/ranges/defaults
verbatim from the locked `parameter-spec.md`) + 16-voice `juce::Synthesiser` with a header-only
**silent** `PhysicalModelVoice` + plain-APVTS state round-trip + render-harness scaffold. Builds
VST3 + AU, appears as an instrument (IS_SYNTH), passes pluginval (strictness 5+), all 17 params
visible in the generic editor, state save/restore round-trips, no audio yet, no crashes on note
input. **Zero DSP, zero WebView** — those are Stages 2 and 3.

---

## Tasks

### 1. [ ] CMakeLists.txt — plugin target (foundation config)
- **Files:** `CMakeLists.txt` (create)
- **Depends on:** none
- **Do:** Adapt O-simpleFM's `CMakeLists.txt` per RESEARCH §2.
  - `juce_add_plugin` with `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`,
    `IS_MIDI_EFFECT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`,
    `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, `FORMATS VST3 AU Standalone`.
  - `PLUGIN_CODE OsPM` (collision-checked free — RESEARCH §8). `PRODUCT_NAME` with `${OUARICON_DEV_SUFFIX}`.
  - `COMPANY_NAME`/`PLUGIN_MANUFACTURER_CODE` from `OuariconModules.cmake` `OUARICON_*` vars.
  - `target_sources`: `PluginProcessor.{h,cpp}` + `PhysicalModelVoice.h` (**no** PluginEditor.cpp —
    see Task 5; **no** FactoryPresets, **no** `ui/`).
  - `target_link_libraries`: standard JUCE modules **+ `juce_dsp`** (linked now even though DSP is Stage 2).
  - `juce_generate_juce_header(...)` **after** `target_link_libraries` (JUCE-8 ordering req).
  - `target_compile_definitions PUBLIC`: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`,
    `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (COMPAT-02), `JUCE_USE_CURL=0`.
  - `option(OUARICON_BUILD_TESTS "Build render-test harness" OFF)` → `add_subdirectory(tests/render-harness)`.
  - **DIVERGENCES from O-simpleFM (do NOT copy):** no `juce_add_binary_data` (D1), no
    `ouaricon_add_module(preset-manager)` (D2), no `dsp::Oversampling` setup (D4).
- **Note:** Root `CMakeLists.txt` auto-discovers via `file(GLOB plugins/*)` (lines 47-58) —
  **no manual `add_subdirectory` registration needed** (corrects RESEARCH §10).

### 2. [ ] PluginProcessor.h — ParamIDs namespace + processor class declaration
- **Files:** `Source/PluginProcessor.h` (create)
- **Depends on:** Task 1
- **Do:** Port O-simpleFM `PluginProcessor.h`, minus preset-manager/oversampler members.
  - `namespace ParamIDs` with the 17 string-ID constants (verbatim from `parameter-spec.md` / RESEARCH §3).
  - Processor class: `juce::AudioProcessor`; members `juce::AudioProcessorValueTreeState parameters`,
    `juce::Synthesiser synth`, `static constexpr int kNumVoices = 16`.
  - Declare `static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()`.
  - `createEditor()` inlined here under `#if JUCE_WEB_BROWSER` seam (see Task 5).
  - **Naming guard:** class/voice names avoid `juce::` shadows (`PhysicalModelVoice`/`PhysicalModelSound`,
    no `SamplerVoice/Sound`); no bare `end`/`begin` param IDs (RESEARCH §9 — both pre-cleared).

### 3. [ ] PhysicalModelVoice.h — header-only silent voice + sound
- **Files:** `Source/PhysicalModelVoice.h` (create)
- **Depends on:** Task 1 (independent of Task 2; both consumed by Task 4)
- **Do:** Mirror `FMVoice.h`/`FMSound`, RESEARCH §4.
  - `PhysicalModelSound : juce::SynthesiserSound` → `appliesToNote`/`appliesToChannel` return `true`.
  - `PhysicalModelVoice : juce::SynthesiserVoice` → `canPlaySound` `dynamic_cast`s the sound;
    `startNote`/`stopNote`/`pitchWheelMoved`/`controllerMoved` minimal; `renderNextBlock` **no-op (silent)**.
  - `stopNote` calls `clearCurrentNote()` so voices free under pluginval note storms.
  - Non-virtual `prepareToPlay(double sampleRate, int blockSize)` (JUCE-8: `SynthesiserVoice` has no
    virtual prepare) — dispatched from the processor (Task 4). Header-only ⇒ no extra .cpp in harness.

### 4. [ ] PluginProcessor.cpp — layout, synth wiring, processBlock, state, editor, factory
- **Files:** `Source/PluginProcessor.cpp` (create)
- **Depends on:** Tasks 2, 3
- **Do:**
  - **`createParameterLayout()`** — all 17 params, RESEARCH §3 table. ⚠ **D3 hazard:** percent params
    use explicit `{0.0f, 100.0f, …}` ranges + 0–100 defaults — **NOT** O-simpleFM's `unitRange()` 0–1.
    `coarseTune` → `AudioParameterInt (−24…+24)`; `outputLevel` → float `.withLabel("dB")` (−60…0, def −6);
    choice params via `StringArray` default index 0; `juce::ParameterID{ id, 1 }` on every param.
    Mild perceptual skew OK on `ampAttack`/`ampRelease` (endpoints/defaults unchanged → contract intact);
    percent params linear.
  - **Constructor** — output-only stereo bus (`BusesProperties().withOutput("Output", stereo(), true)`);
    `parameters(*this, nullptr, "PARAMETERS", createParameterLayout())`; add 16 `PhysicalModelVoice` +
    one `PhysicalModelSound`; `synth.setNoteStealingEnabled(true)`.
  - **`prepareToPlay`** — `synth.setCurrentPlaybackSampleRate(sr)`; `dynamic_cast` over `synth.getVoice(i)`
    to dispatch the voice's non-virtual `prepareToPlay`; `setLatencySamples(0)` (JUCE-8 getter non-virtual).
  - **`processBlock`** — `juce::ScopedNoDenormals`; `buffer.clear()`; `synth.renderNextBlock(buffer, midi, 0, n)`
    (silent for now). `isBusesLayoutSupported` accepts mono/stereo out.
  - **`getStateInformation`/`setStateInformation`** — plain APVTS XML round-trip (RESEARCH §5; NO preset manager).
  - **`createPluginFilter`** — returns the processor.
- **Verify inline:** 17 params build into the layout; no unitRange on percent params.

### 5. [ ] createEditor — generic placeholder behind the WebView seam
- **Files:** `Source/PluginProcessor.h` (inline `createEditor`) — **decision: skip `PluginEditor.{h,cpp}` at Stage 1**
- **Depends on:** Task 2
- **Do:** Inline `createEditor()` in the processor returning `new juce::GenericAudioProcessorEditor(*this)`,
  wrapped in the `#if JUCE_WEB_BROWSER / #else / #endif` seam (both branches Generic at Stage 1 — RESEARCH §6).
  `hasEditor()` returns `true`. **No separate `PluginEditor.cpp`** → harness compiles only `PluginProcessor.cpp`
  with no editor/WebView symbols under `JUCE_WEB_BROWSER=0` (RESEARCH §7; cleaner than O-simpleFM's retrofit).
  Stage 3 introduces the real `PluginEditor.{h,cpp}` WebView editor at this seam.

### 6. [ ] Render-harness scaffold — CMake (JUCE_WEB_BROWSER=0, no editor)
- **Files:** `tests/render-harness/CMakeLists.txt` (create)
- **Depends on:** Task 4
- **Do:** Adapt O-simpleFM `tests/render-harness/CMakeLists.txt` but **strip the editor/UI retrofit**
  (RESEARCH §7 — O-simpleFM's harness is the cautionary example, not the template):
  - `juce_add_console_app`; sources = `main.cpp` + `../../Source/PluginProcessor.cpp` **only** (voice is header-only).
  - `add_dependencies(... O-simplePhysicalModelSynth)`; borrow generated JuceHeader include dir via
    `$<TARGET_PROPERTY:O-simplePhysicalModelSynth,INCLUDE_DIRECTORIES>`.
  - `target_compile_definitions`: mirror O-simpleFM's `JucePlugin_*` block but **`JUCE_WEB_BROWSER=0`**
    and our own `JucePlugin_PluginCode`/name macros; link `juce_dsp` + JUCE modules; **no `*_UIResources`** link.

### 7. [ ] Render-harness stub — builds & links against the shell
- **Files:** `tests/render-harness/main.cpp` (create)
- **Depends on:** Task 6
- **Do:** Stage-1 **stub** (RESEARCH §7): instantiate the processor, `prepareToPlay(44100, 512)`, push a
  note-on via a MIDI buffer, `processBlock` a few blocks, assert output is finite + no crash, `return 0`.
  The **autocorrelation pitch probe** is Stage 2.1 — not here. This task only proves the harness
  *builds and links* against the silent shell, locking the `JUCE_WEB_BROWSER=0` / no-editor seam early.

### 8. [ ] Build, install, smoke-check
- **Files:** none (build artifacts)
- **Depends on:** Tasks 1–7
- **Do:**
  - Configure + `ninja O-simplePhysicalModelSynth_VST3 O-simplePhysicalModelSynth_AU`.
  - Build harness once with `-DOUARICON_BUILD_TESTS=ON` to prove the scaffold links (Tasks 6–7).
  - AU cache clear + dual-variant sweep + install via `./scripts/build-and-install.sh O-simplePhysicalModelSynth`
    (Phase-4 sweep handles `-dev` ↔ unsuffixed shadowing).
  - Smoke: `auval -a | grep -i physicalmodel` resolves; quick pluginval pass. **Formal** pluginval
    strictness-5 + 17-param + state round-trip + DAW instrument-list checks are the **verify phase**.

---

## Files (manifest)

| File | Task | Action |
|------|------|--------|
| `CMakeLists.txt` | 1 | create |
| `Source/PluginProcessor.h` | 2, 5 | create |
| `Source/PhysicalModelVoice.h` | 3 | create |
| `Source/PluginProcessor.cpp` | 4 | create |
| `tests/render-harness/CMakeLists.txt` | 6 | create |
| `tests/render-harness/main.cpp` | 7 | create |

**Deliberately NOT created at Stage 1:** `Source/PluginEditor.{h,cpp}` (inlined generic editor; real
editor is Stage 3), `juce_add_binary_data` target / `Source/ui/` (Stage 3), `FactoryPresets.*` /
preset-manager module (Stage 4). Root `CMakeLists.txt` needs **no** edit (GLOB auto-discovery).

---

## Dependency graph

```
Task 1 (CMake) ─┬─> Task 2 (Processor.h) ─┬─> Task 4 (Processor.cpp) ─> Task 6 (harness CMake) ─> Task 7 (harness main) ─┐
                │                          │                                                                              ├─> Task 8 (build/install/smoke)
                └─> Task 3 (Voice.h) ──────┘                                                                              │
                              Task 5 (createEditor seam) depends on Task 2 ──────────────────────────────────────────────┘
```
Tasks 2 and 3 are parallelizable after Task 1. Task 5 is a small addition to the Task-2 header.

---

## Success Criteria (Stage-1 exit gate — from ROADMAP / CONTEXT)

- [ ] VST3 + AU build cleanly; plugin appears in the DAW **instrument** list (IS_SYNTH)
- [ ] pluginval passes at **strictness 5+** (COMPAT-01)
- [ ] All **17 params** visible in the generic editor; IDs/ranges/defaults match `parameter-spec.md` (zero drift)
- [ ] State **save/restore round-trips** (plain APVTS)
- [ ] **Silent** — note input produces no audio, and **no crashes** under note storms
- [ ] WebView2 flags present: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (COMPAT-02)
- [ ] Render-harness **builds & links** under `-DOUARICON_BUILD_TESTS=ON` (`JUCE_WEB_BROWSER=0`, no editor)
- [ ] Percent params stored **0–100** (NOT 0–1) — the D3 copy hazard did not slip through

## Out of scope (later stages)
- Any DSP / audio generation, oversampling, autocorrelation pitch probe → **Stage 2**
- WebView UI, binary-data target, real `PluginEditor` → **Stage 3**
- Presets / preset-manager module, optimization, edge-case polish → **Stage 4**

---
*Plan phase complete. Next: execute phase → `/plugin-execute O-simplePhysicalModelSynth 1-foundation`.*
