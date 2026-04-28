---
title: "O-Bassoon Stage 1 (Foundation) — Execution Plan"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Single-pass execution plan for Stage 1 (Foundation). Delegates to foundation-shell-agent with CONTEXT.md + RESEARCH.md attached. Ships CMakeLists.txt + 5 source files: silent BassoonVoice stub, BassoonSound, PluginProcessor (APVTS + Synthesiser + headless TuningEngine + NE VST3Extensions + drainAndUpdate), GenericAudioProcessorEditor placeholder. Absorbs three discrepancies surfaced in research (D1: ouaricon_add_module-only; D2: TuningEngine global namespace; D3: NEEDS_WEBVIEW2 TRUE flag). Reserves PLUGIN_CODE OBsn."
domain: workflow
type: guide
keywords:
  - stage-1
  - foundation
  - bassoon
  - apvts
  - cmake
  - note-expression
  - scala-tuning-engine
  - juce8
stages: [1]
agents: [foundation-shell, build]
---

# O-Bassoon — Stage 1 Execution Plan (Foundation)

## Goal

Stand up the O-Bassoon plugin shell as a buildable, host-loadable, silent VST3/AU/Standalone synth. Lock in the 10-parameter APVTS, wire both shared modules (`note-expression` v1.1.0, `scala-tuning-engine` v2.0.0) headless from day one, and commit Windows WebView CMake flags up-front so Stage 3 only changes editor C++. Stage 1 produces no audio — first audio is Phase 2.1.

**Stage 1 is one wave, one agent (`foundation-shell-agent`). No DSP. No UI logic. No parameter→DSP wiring beyond the APVTS→host surface.**

---

## Inputs (Required Reading for Executor)

| File | Purpose |
|---|---|
| `plugins/O-Bassoon/.planning/stages/1-foundation/CONTEXT.md` | Discuss-phase decisions (parameter-spec lock, NE drain at Stage 1, BassoonVoice silent-stub spec, voice pre-allocation, sample-rate / latency stance) |
| `plugins/O-Bassoon/.planning/stages/1-foundation/RESEARCH.md` | Confirmed APIs (NoteExpression, TuningEngine), canonical CMake recipe, copy-paste snippets, three discrepancies (D1/D2/D3) absorbed below |
| `plugins/O-Bassoon/.planning/parameter-spec-draft.md` | Frozen 10-parameter APVTS spec (IDs, ranges, defaults) |
| `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` | Stage 0 architecture spec (read for context only — supersede with RESEARCH.md where they conflict) |
| `plugins/O-Bassoon/.planning/ROADMAP.md` | Stage 1 component list + acceptance criteria |
| `plugins/O-Wind/CMakeLists.txt` | CMake template — primary reference for `juce_add_plugin` flags, scala-tuning-engine direct sources, WebView block |
| `plugins/O-Lyrica/CMakeLists.txt` + `Source/PluginProcessor.{h,cpp}` + `Source/HarpSynthVoice.{h,cpp}` + `Source/HarpSynthSound.h` | NE wiring template + voice/sound shape |
| `modules/tuning/note-expression/cpp/NoteExpression.h` | NE public API (member type, drain call, `getPendingTable()`) |
| `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h` | TuningEngine public API (global namespace) |

---

## Discrepancies Absorbed (from RESEARCH.md §10)

These are committed as plan-phase decisions — no further deliberation:

- **D1.** Note-expression module is wired via `ouaricon_add_module(O-Bassoon note-expression)` **only**. Do not emit any `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line — that target does not exist. (Overrides ARCHITECTURE.md §8.)
- **D2.** `TuningEngine` is in the **global namespace**. All header lines and member declarations are `class TuningEngine` / `TuningEngine tuningEngine;` / `TuningEngine* getTuningEngine()`. **No `Ouaricon::` prefix anywhere.** (Overrides ARCHITECTURE.md §9 and CONTEXT.md "Approach Decisions" row 5.)
- **D3.** `juce_add_plugin(...)` includes **`NEEDS_WEBVIEW2 TRUE`** in addition to `NEEDS_WEB_BROWSER TRUE`. The compile-definition `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` remains in the `target_compile_definitions` block. All three flags are required for Windows static-linked WebView2. (Adds to CONTEXT.md "Constraints / Approach Decisions".)

`PLUGIN_CODE OBsn` is reserved (research §5 — `OBas` is taken by O-Bass).

---

## Tasks

Execution order is sequential (one wave). Files compile cleanly only after task 5 completes; build verification is task 6.

### 1. [ ] Create `plugins/O-Bassoon/CMakeLists.txt`

**Files created:**
- `plugins/O-Bassoon/CMakeLists.txt`

**Depends on:** none

**Spec:**
- Mirror `plugins/O-Wind/CMakeLists.txt` structure (NOT O-Lyrica — O-Lyrica keeps a local copy of `TuningEngine.cpp` which is wrong for v1.0+ inheritance of upstream fixes).
- `juce_add_plugin(O-Bassoon ...)` with the canonical Stage 1 flag set (RESEARCH.md §3):
  - `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`
  - `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`
  - `PLUGIN_CODE OBsn`
  - `FORMATS VST3 AU Standalone` (macOS gets all three; Windows builds VST3 only via the parent `if(APPLE)` block in O-Wind precedent — copy verbatim)
  - `PRODUCT_NAME "O-Bassoon${OUARICON_DEV_SUFFIX}"`
  - `PLUGIN_VERSION "1.0.0"`
  - `IS_SYNTH TRUE`
  - `NEEDS_MIDI_INPUT TRUE`
  - `NEEDS_MIDI_OUTPUT FALSE`
  - `IS_MIDI_EFFECT FALSE`
  - `NEEDS_WEB_BROWSER TRUE`
  - **`NEEDS_WEBVIEW2 TRUE`** (D3)
  - `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`
- `target_sources(O-Bassoon PRIVATE ...)`:
  - `Source/PluginProcessor.cpp`
  - `Source/PluginEditor.cpp`
  - `Source/BassoonSound.h`
  - `Source/BassoonVoice.h`
  - `Source/BassoonVoice.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp`
- `target_include_directories(O-Bassoon PRIVATE Source ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp)`
- `ouaricon_add_module(O-Bassoon note-expression)` **(D1 — single line, no target_link_libraries equivalent)**
- `target_link_libraries(O-Bassoon PRIVATE ...)` — full O-Lyrica module list (12 JUCE modules + 3 PUBLIC config groups). At minimum: `juce::juce_audio_utils juce::juce_audio_processors juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats juce::juce_audio_plugin_client juce::juce_dsp juce::juce_core juce::juce_data_structures juce::juce_events juce::juce_graphics juce::juce_gui_basics juce::juce_gui_extra` + the three Ouaricon PUBLIC config groups. Copy verbatim from `plugins/O-Wind/CMakeLists.txt`.
- `juce_generate_juce_header(O-Bassoon)` — **MUST come after** `target_link_libraries(...)` (RESEARCH.md pitfall #2; `juce8-critical-patterns.md` #22).
- `target_compile_definitions(O-Bassoon PUBLIC ...)`:
  - `JUCE_VST3_CAN_REPLACE_VST2=0`
  - `JUCE_WEB_BROWSER=1`
  - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
  - `JUCE_USE_CURL=0`
  - (Located after `juce_generate_juce_header`, matching O-Wind:108-114.)
- Optional licensing block at bottom (gated on `OUARICON_LICENSING`) — copy verbatim from O-Wind. Dead code at Stage 1 unless `-DOUARICON_LICENSING=ON` is passed at configure.

**Forbidden:**
- No `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line. (D1)
- No wildcard `target_sources(... ${CMAKE_SOURCE_DIR}/modules/tuning/note-expression/cpp/*)` glob — the `ouaricon_add_module` call already routes those files (and routes `cpp/vst3/*` to the per-format target only). RESEARCH.md pitfall #7.
- No `juce_add_binary_data(...)` at Stage 1 (no WebView resources yet). RESEARCH.md pitfall #8.

---

### 2. [ ] Create `plugins/O-Bassoon/Source/BassoonSound.h`

**Files created:**
- `plugins/O-Bassoon/Source/BassoonSound.h`

**Depends on:** Task 1 (CMake includes Source/ in include dir)

**Spec:**
- Header-only, byte-aligned with `plugins/O-Lyrica/Source/HarpSynthSound.h` pattern. RESEARCH.md §8 has the exact body:

```cpp
#pragma once
#include <JuceHeader.h>

class BassoonSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote   (int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
```

---

### 3. [ ] Create `plugins/O-Bassoon/Source/BassoonVoice.{h,cpp}`

**Files created:**
- `plugins/O-Bassoon/Source/BassoonVoice.h`
- `plugins/O-Bassoon/Source/BassoonVoice.cpp`

**Depends on:** Task 1, Task 2

**Spec:**
- Header surface byte-aligned with RESEARCH.md §9 (full method signatures, no DSP):
  - Inherits `juce::SynthesiserVoice`
  - `bool canPlaySound(juce::SynthesiserSound* s) override` — `dynamic_cast<BassoonSound*>(s) != nullptr`
  - `void startNote(int, float, juce::SynthesiserSound*, int) override` — empty body (Stage 1)
  - `void stopNote(float, bool) override` — body calls `clearCurrentNote();` (immediate clear; ADSR release wired in Phase 2.1)
  - `void pitchWheelMoved(int) override` — empty
  - `void controllerMoved(int, int) override` — empty
  - `void renderNextBlock(juce::AudioBuffer<float>&, int, int) override` — empty (writes nothing — silent stub)
  - Three setters (called once per voice from PluginProcessor ctor):
    - `void setAPVTS(juce::AudioProcessorValueTreeState* p)`
    - `void setTuningEngine(TuningEngine* engine)` **(D2 — `TuningEngine`, no namespace prefix)**
    - `void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* src)`
  - Three private raw-pointer members initialized to `nullptr`:
    - `juce::AudioProcessorValueTreeState* parameters = nullptr;`
    - `TuningEngine* tuningEngine = nullptr;`
    - `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;`
- Includes:
  - `<JuceHeader.h>`
  - `"TuningEngine.h"` (resolved via `target_include_directories` from Task 1)
  - `"NoteExpression.h"` (resolved via `ouaricon_add_module` include path)
  - `"BassoonSound.h"` (for `dynamic_cast` in `canPlaySound`)
- `BassoonVoice.cpp` — single-line constructor body (or default), out-of-line `stopNote` if not in header. Methods can all be inline-in-header at this stage; `.cpp` exists primarily so CMake `target_sources` has a real translation unit. Either form is acceptable provided the build passes.

**Forbidden:**
- No DSP code (no biquads, no excitation, no ADSR — those are Phase 2.1).
- No dereferencing of `parameters`, `tuningEngine`, or `pendingTuningSource` — Stage 1 voices receive these pointers and ignore them.
- No `setLatencySamples(...)` calls. RESEARCH.md pitfall #3.

---

### 4. [ ] Create `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}`

**Files created:**
- `plugins/O-Bassoon/Source/PluginProcessor.h`
- `plugins/O-Bassoon/Source/PluginProcessor.cpp`

**Depends on:** Task 1, Task 2, Task 3

**Spec:**

**Class name:** `OBassoonAudioProcessor` (matches `OBassoonAudioProcessorEditor` in Task 5).

**Header members (RESEARCH.md §6 — exact layout):**
```cpp
juce::AudioProcessorValueTreeState parameters;
juce::Synthesiser                  synthesiser;
TuningEngine                       tuningEngine;             // D2: global namespace
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
```

**Public methods (override surface):**
- `OBassoonAudioProcessor()` — constructor body per RESEARCH.md §6:
  - `BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)` only (no input bus — RESEARCH.md pitfall #1; `juce8-critical-patterns.md` #4)
  - Initialize `parameters(*this, nullptr, "Parameters", createParameterLayout())`
  - Loop `for (int i = 0; i < 16; ++i)`:
    - `auto* voice = new BassoonVoice();`
    - `voice->setAPVTS(&parameters);`
    - `voice->setTuningEngine(&tuningEngine);`
    - `voice->setPendingTuningSource(&vst3Extensions.getPendingTable());`
    - `synthesiser.addVoice(voice);`
  - `synthesiser.addSound(new BassoonSound());`
- `~OBassoonAudioProcessor() override` — defaulted (synthesiser owns voices).
- `void prepareToPlay(double sampleRate, int samplesPerBlock) override`:
  - `synthesiser.setCurrentPlaybackSampleRate(sampleRate);`
  - **No** `setLatencySamples(...)` call. (Modal synthesis is feed-forward; latency = 0; getter is non-virtual.) RESEARCH.md pitfall #3.
- `void releaseResources() override` — empty (Stage 1 owns no externally-allocated resources).
- `bool isBusesLayoutSupported(const BusesLayout&) const override` — accept stereo output only, mono input refused (copy from O-Lyrica).
- `void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override`:
  ```cpp
  juce::ScopedNoDenormals noDenormals;
  buffer.clear();
  vst3Extensions.drainAndUpdate();   // BEFORE renderNextBlock — RESEARCH.md pitfall #4
  synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
  ```
  No APVTS reads at Stage 1. No parameter smoothing. No per-voice mutation.
- `juce::AudioProcessorEditor* createEditor() override` — `return new OBassoonAudioProcessorEditor(*this);` (Task 5).
- `bool hasEditor() const override` — `return true;`
- `juce::VST3ClientExtensions* getVST3ClientExtensions() override` — `return &vst3Extensions;` (RESEARCH.md §1).
- Standard `getName / acceptsMidi / producesMidi / isMidiEffect / getTailLengthSeconds / getNumPrograms / getCurrentProgram / setCurrentProgram / getProgramName / changeProgramName / getStateInformation / setStateInformation` overrides — copy from O-Lyrica/O-Wind. `getStateInformation` / `setStateInformation` use APVTS XML round-trip (standard pattern).
- `juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }` — public accessor for the editor.

**`createParameterLayout()` — frozen 10-parameter spec (RESEARCH.md §7):**

Implement as `static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();` (or method on the class — match O-Wind's approach). All ranges, defaults, and IDs are committed:

| ID (string) | C++ class | Range | Default | Step / Skew | Suffix |
|---|---|---|---|---|---|
| `vibrato_rate` | `juce::AudioParameterFloat` | `0.0f, 10.0f` | `5.0f` | step `0.01f`, skew `1.0f` | `" Hz"` |
| `vibrato_depth` | `juce::AudioParameterFloat` | `0.0f, 100.0f` | `15.0f` | step `0.1f` | `" cents"` |
| `vibrato_onset` | `juce::AudioParameterFloat` | `0.0f, 2000.0f` | `400.0f` | step `1.0f` | `" ms"` |
| `breath` | `juce::AudioParameterFloat` | `0.0f, 1.0f` | `0.7f` | step `0.001f` | (none) |
| `tone` | `juce::AudioParameterFloat` | `0.0f, 1.0f` | `0.5f` | step `0.001f` | (none) |
| `attack_character` | `juce::AudioParameterFloat` | `0.0f, 1.0f` | `0.0f` | step `0.001f` | (none) |
| `attack_time` | `juce::AudioParameterFloat` | `0.0f, 2000.0f` | `300.0f` | step `1.0f` | `" ms"` |
| `release_time` | `juce::AudioParameterFloat` | `0.0f, 3000.0f` | `800.0f` | step `1.0f` | `" ms"` |
| `voice_count` | `juce::AudioParameterInt` | `1, 16` | `8` | (int) | (none) |
| `output_gain` | `juce::AudioParameterFloat` | `-24.0f, 6.0f` | `0.0f` | step `0.1f` | `" dB"` |

Use `juce::ParameterID(id, 1)` for the version-hint argument (matches O-Wind/O-Lyrica). Display names are title-cased equivalents (e.g. `"Vibrato Rate"`).

**Forbidden:**
- No allocations after the constructor (RESEARCH.md pitfall, project memory PERF-01).
- No `target_link_libraries` references to a non-existent `Ouaricon::note_expression` target — this is a CMake concern but mention here as defense-in-depth.
- No O-Reed source includes anywhere in this file (DSP-07).

---

### 5. [ ] Create `plugins/O-Bassoon/Source/PluginEditor.{h,cpp}`

**Files created:**
- `plugins/O-Bassoon/Source/PluginEditor.h`
- `plugins/O-Bassoon/Source/PluginEditor.cpp`

**Depends on:** Task 4

**Spec:**

`OBassoonAudioProcessorEditor : public juce::GenericAudioProcessorEditor` — RESEARCH.md §10:

```cpp
// PluginEditor.h
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OBassoonAudioProcessorEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit OBassoonAudioProcessorEditor(OBassoonAudioProcessor& p)
        : juce::GenericAudioProcessorEditor(p)
    {
        setSize(500, 480);   // ~10 params * ~40px row, room for labels
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassoonAudioProcessorEditor)
};
```

`PluginEditor.cpp` may be empty aside from `#include "PluginEditor.h"` (or contain the constructor body — either form acceptable). The placeholder auto-renders all 10 APVTS parameters as JUCE generic sliders, sufficient for Stage 1 manual smoke-test (knob-twiddle in DAW, verify host automation surfaces them).

**Forbidden:**
- No `juce::WebBrowserComponent`, no `juce::WebSliderRelay`, no resource provider — those are Stage 3.
- No knob-graphic includes from `juce::ImageCache` etc. — bare generic editor only.

---

### 6. [ ] Build verification — macOS VST3 + AU + Standalone

**Files modified:** none (build only)

**Depends on:** Tasks 1-5

**Commands (run from project root):**
```bash
cmake --build build --target O-Bassoon_VST3
cmake --build build --target O-Bassoon_AU
cmake --build build --target O-Bassoon_Standalone
```

**Pass conditions:**
- All three targets build clean (no warnings escalated to errors that aren't pre-existing in the workspace).
- Build artefacts appear at:
  - `build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/VST3/O-Bassoon.vst3` (or `Debug/` per cmake config)
  - `build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/AU/O-Bassoon.component`
  - `build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/Standalone/O-Bassoon.app`
- JUCE-NE-PATCH marker check passes at configure time (RESEARCH.md pitfall #5 — failure mode is `[note-expression] JUCE patch marker 'JUCE-NE-PATCH' not found`; remediation `./scripts/apply-juce-patches.sh`).
- No O-Reed sources pulled in: `grep -rn "O-Reed\|OReed" plugins/O-Bassoon/` returns empty (DSP-07).

**If build fails:** the executor must diagnose and fix in-place rather than skipping. Common failure modes:
- Forgot `juce_generate_juce_header` ordering → reorder after `target_link_libraries`.
- Forgot `target_include_directories` for `scala-tuning-engine/cpp/` → unresolved `TuningEngine.h`.
- Used `Ouaricon::TuningEngine` somewhere → unresolved class (D2).
- JUCE-NE-PATCH absent → run `./scripts/apply-juce-patches.sh`.

---

### 7. [ ] Install to system folders + DAW smoke test

**Files modified:** none (install only)

**Depends on:** Task 6

**Commands (per project CLAUDE.md cache-clearing protocol):**
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bassoon.component
cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/VST3/O-Bassoon.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/AU/O-Bassoon.component ~/Library/Audio/Plug-Ins/Components/
auval -a | grep -i bassoon   # verify AU registration
```

**Pass conditions:**
- `auval -a` lists `O-Bassoon` (or `O-Bassoon-DEV`) in the instrument category.
- AU validation does not need to be 100% clean for Stage 1 (DEF-24-01-style benign findings allowed per project memory) — but no crashes during validation.

---

### 8. [ ] pluginval strictness 5 (Stage 1 acceptance level)

**Files modified:** none

**Depends on:** Task 7

**Command:**
```bash
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3
```

**Pass conditions:**
- Returns exit code 0.
- Output ends with `ALL TESTS PASSED`.

**Note:** Stage 4 raises this to strictness 10. Stage 1 only needs strictness 5 — silent stub voices may not exercise enough surface for higher-strictness tests anyway.

**If pluginval fails:** the executor must triage. Common Stage 1 failures:
- `getTailLengthSeconds()` returning a finite value when synth has no tail logic — return `0.0` at Stage 1 (Phase 2.1 will revisit if ADSR release > 0 needs it).
- `BusesProperties` rejecting layouts pluginval probes — copy O-Lyrica's `isBusesLayoutSupported` exactly.
- `getVST3ClientExtensions()` returning nullptr — verify `vst3Extensions` is a long-lived member, not a stack temp.

---

### 9. [ ] Update `plugins/O-Bassoon/.planning/STATUS.md`

**Files modified:**
- `plugins/O-Bassoon/.planning/STATUS.md`

**Depends on:** Tasks 6-8

**Spec:**
- `phase:` → `execute_complete` (or whatever the verify-phase entry token is — match the workflow's convention; the verify phase will move it to `verify_complete` after the next command).
- `next_action:` → `verify_phase`
- Append a new "Stage 1 execute (foundation-shell)" section under "Completed So Far" with:
  - Files created (paths to the five Source files + CMakeLists.txt)
  - Build pass (commit hash)
  - pluginval strictness 5 pass
  - DAW load smoke pass
- Refresh `last_updated:` to today's date.

**Forbidden:**
- Do not modify `contract_checksums:` — those are owned by the contract-validation skill, not the executor.

---

## Dependency Graph

```
Task 1 (CMakeLists.txt)
     │
     ├──> Task 2 (BassoonSound.h)
     │         │
     │         └──> Task 3 (BassoonVoice.{h,cpp})
     │                   │
     │                   └──> Task 4 (PluginProcessor.{h,cpp})
     │                             │
     │                             └──> Task 5 (PluginEditor.{h,cpp})
     │                                       │
     ├─────────────────────────────────────> Task 6 (build VST3 + AU + Standalone)
     │                                                  │
     │                                                  └──> Task 7 (install + DAW smoke)
     │                                                                 │
     │                                                                 └──> Task 8 (pluginval --strictness 5)
     │                                                                                │
     │                                                                                └──> Task 9 (STATUS.md)
```

Tasks 2-5 could in principle execute as a fan-out, but the natural author-order is sequential and produces cleaner diff history. The foundation-shell-agent should write them in order 1 → 2 → 3 → 4 → 5, then run 6-9 sequentially.

---

## Success Criteria

Lifted from CONTEXT.md "Stage 1 Acceptance Criteria" + ROADMAP §"Stage 1 Test Criteria" — restated here as the definitive verification list:

- [ ] `cmake --build build --target O-Bassoon_VST3` succeeds (macOS).
- [ ] `cmake --build build --target O-Bassoon_AU` succeeds (macOS).
- [ ] `cmake --build build --target O-Bassoon_Standalone` succeeds (macOS).
- [ ] `cmake --build build --config Release --target O-Bassoon_VST3` succeeds on Windows (verify static linking compiles even if not run-tested every cycle — Mac-only Stage 1 build is acceptable provided the recipe is correct; Windows verification is a Stage 4 concern at the latest).
- [ ] `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3` passes.
- [ ] Plugin loads in Ableton or Logic without crash; appears in instrument category.
- [ ] All 10 APVTS parameters appear in the host's parameter list with correct names, ranges, and defaults (use the Generic editor or the host's parameter inspector to confirm).
- [ ] Plays silence (no audio bug, no crash) when MIDI notes are sent — verify by playing C3 in the host and confirming the meter reads zero.
- [ ] `getVST3ClientExtensions()` returns non-null `Ouaricon::NoteExpression::VST3Extensions*` (verified by Vst3PluginTestHost or pluginval VST3-extensions probe; alternatively a one-line print in `getVST3ClientExtensions` during Stage 1 build, removed before commit).
- [ ] JUCE-NE-PATCH CMake-time marker check passes at configure time.
- [ ] `grep -rn "O-Reed\|OReed" plugins/O-Bassoon/` is empty (DSP-07 — verify after sources are written).
- [ ] No `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line anywhere in `plugins/O-Bassoon/CMakeLists.txt` (D1).
- [ ] No `Ouaricon::TuningEngine` token anywhere in `plugins/O-Bassoon/Source/` (D2).
- [ ] `NEEDS_WEBVIEW2 TRUE` present inside `juce_add_plugin(O-Bassoon ...)` block (D3).
- [ ] `PLUGIN_CODE OBsn` set in `juce_add_plugin(...)`.

**Verifies requirements:** COMPAT-01 (pluginval pass), DSP-07 (no O-Reed dependency).

---

## Out of Scope (Hand off to Stage 2 / Stage 3 / Stage 4)

Explicitly NOT in Stage 1 — do not implement:

- Mode bank biquads, partial table, exciter, ADSR — Phase 2.1 / 2.2.
- APVTS parameter reads, smoothing, voice mutation — Phase 2.3.
- Vibrato LFO, breath/CC2 routing, `output_gain` smoothing — Phase 2.3.
- `findFreeVoice` voice-cap subclass — Phase 2.4.
- Attack-character morph, sustain noise, MPE pitch-bend, full NE consumption — Phase 2.4.
- `juce::WebBrowserComponent` editor, binary data, resource provider, knob graphics — Stage 3.
- Factory presets, CHANGELOG, Dorico playback template — Stage 4.

If the executor finds itself wanting to add any of the above to make Stage 1 "feel finished," **stop**. Stage 1 finishes silent on purpose — that's the whole point of staged implementation.

---

## Notes for the Executor (foundation-shell-agent)

- Read CONTEXT.md and RESEARCH.md in full before touching any file. The dependency hierarchy in research §6 is not optional — member-construction order and `getPendingTable()` lifetime correctness depend on it.
- When in doubt, **mirror O-Wind**, not O-Lyrica. O-Lyrica has a known divergence (local copy of `TuningEngine.cpp` under `Source/DSP/`) that pre-dates the shared-module promotion. RESEARCH.md §4 documents this.
- If a copy-from-O-Lyrica step produces a token like `Ouaricon::TuningEngine` (because O-Lyrica uses it inside the local-copy `DSP/`-folder includes) — strip the namespace. D2 stands.
- Commit each task as its own git commit (atomic, per project workflow standard). Suggested commit messages:
  1. `feat(O-Bassoon): Stage 1 CMakeLists - juce_add_plugin + module wiring`
  2. `feat(O-Bassoon): BassoonSound + BassoonVoice (silent stub)`
  3. (combined into 2 if simpler)
  4. `feat(O-Bassoon): PluginProcessor with APVTS + headless TuningEngine + NE drain`
  5. `feat(O-Bassoon): GenericAudioProcessorEditor placeholder`
  6. `chore(O-Bassoon): Stage 1 build pass (VST3 + AU + Standalone)`
  9. `docs(O-Bassoon): STATUS update - Stage 1 execute complete`
- If pluginval fails on the first run, **diagnose, do not skip**. Stage 1 silent-stub semantics should pass strictness 5 cleanly; failure usually indicates a real wiring bug (`getTailLengthSeconds`, bus layout, NE extensions lifetime).

---

## References

- CONTEXT.md (this stage): `plugins/O-Bassoon/.planning/stages/1-foundation/CONTEXT.md`
- RESEARCH.md (this stage): `plugins/O-Bassoon/.planning/stages/1-foundation/RESEARCH.md`
- ARCHITECTURE.md (Stage 0): `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md`
- ROADMAP.md (Stage 0): `plugins/O-Bassoon/.planning/ROADMAP.md`
- Parameter spec: `plugins/O-Bassoon/.planning/parameter-spec-draft.md`
- Project conventions: `CLAUDE.md` (root), `troubleshooting/patterns/juce8-critical-patterns.md`, `spike-findings-VST-development` skill
- Reference plugins: `plugins/O-Wind/`, `plugins/O-Lyrica/`
- Shared modules: `modules/tuning/note-expression/`, `modules/tuning/scala-tuning-engine/`

---

## Next Phase

Ready for **execute** phase: `/plugin-execute O-Bassoon 1-foundation`

Executor: `foundation-shell-agent` with this PLAN.md + CONTEXT.md + RESEARCH.md attached.
