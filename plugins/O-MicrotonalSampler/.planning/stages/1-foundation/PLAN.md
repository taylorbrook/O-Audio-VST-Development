---
title: "O-MicrotonalSampler Stage 1 (Foundation) — Execution Plan"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Single-pass execution plan for Stage 1 (Foundation). Delegates to foundation-shell-agent with RESEARCH.md attached. Ships CMakeLists.txt + 7 source files: silent MicrotonalSamplerVoice stub, MicrotonalSamplerSound, PluginProcessor (APVTS + Synthesiser + headless TuningEngine + NE VST3Extensions + drainAndUpdate + sample-map shared_ptr surface), SampleMap struct, SampleLoader juce::Thread skeleton, GenericAudioProcessorEditor placeholder. Reserves PLUGIN_CODE OMtS. Freezes the 7-parameter APVTS layout (ADSR + Polyphony + Velocity Crossfade + Output Gain). Three deeper DSP questions (voice stealing, loop auto-detect, +50c AA margin) are explicitly deferred to Stage 2 — Stage 1 wiring is invariant to their answers."
domain: workflow
type: guide
keywords:
  - stage-1
  - foundation
  - sampler
  - microtonal
  - apvts
  - cmake
  - note-expression
  - scala-tuning-engine
  - varispeed
  - audio-format-manager
  - juce8
stages: [1]
agents: [foundation-shell, build]
---

# O-MicrotonalSampler — Stage 1 Execution Plan (Foundation)

## Goal

Stand up the O-MicrotonalSampler plugin shell as a buildable, host-loadable, silent VST3/AU/Standalone synth. Lock in the 7-parameter APVTS, wire both shared modules (`note-expression` v1.1.0, `scala-tuning-engine` v2.0.0) headless from day one, freeze the sample-loading message-thread surface (`SampleMap` POD + `SampleLoader` `juce::Thread` skeleton) so Stage 2.2 inherits a working API rather than redesigning, and commit Windows WebView CMake flags up-front so Stage 3 only changes editor C++. Stage 1 produces no audio — first audio is Phase 2.1.

**Stage 1 is one wave, one agent (`foundation-shell-agent`). No DSP. No UI logic. No sample loading. No parameter→DSP wiring beyond the APVTS→host surface.**

---

## Inputs (Required Reading for Executor)

| File | Purpose |
|---|---|
| `plugins/O-MicrotonalSampler/.planning/stages/1-foundation/RESEARCH.md` | Confirmed APIs (NoteExpression v1.1.0, TuningEngine global ns), canonical CMake recipe, copy-paste snippets, SampleMap/SampleLoader design, three Stage-2-deferred questions |
| `plugins/O-MicrotonalSampler/.planning/BRIEF.md` | Locked plugin concept (varispeed retune, ≤±50c, 4 vel layers, 16 voices, RAM-only, no onboard FX) — frozen 7-parameter list |
| `plugins/O-MicrotonalSampler/.planning/REQUIREMENTS.md` | 22 reqs across FUNC/DSP/UI/PERF/COMPAT/QUAL — Stage 1 verifies COMPAT-01 only |
| `plugins/O-MicrotonalSampler/.planning/STATUS.md` | Current state marker (research_complete) — task 9 advances it |
| `plugins/O-Wind/CMakeLists.txt` | CMake template — primary reference for `juce_add_plugin` flags, scala-tuning-engine direct sources, WebView trio |
| `plugins/O-Bassoon/CMakeLists.txt` + `Source/PluginProcessor.{h,cpp}` + `Source/BassoonVoice.{h,cpp}` + `Source/BassoonSound.h` | Most recent NE+TuningEngine wiring template — byte-aligned with this plan |
| `plugins/O-TextureForge/Source/dsp/CorpusLoader.{h,cpp}` | Background `juce::Thread` + `AudioFormatManager` precedent for `SampleLoader` skeleton |
| `modules/tuning/note-expression/cpp/NoteExpression.h` | NE public API (`VST3Extensions`, `drainAndUpdate`, `getPendingTable`) |
| `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h` | TuningEngine public API (global namespace, `getFrequency(int, int)`) |

---

## Decisions Locked (from RESEARCH.md §11 D-1/D-2 + Findings 4, 5)

These are committed as plan-phase decisions — no further deliberation:

- **D-1 (class name).** Processor class is `OMicrotonalSamplerAudioProcessor`. Editor class is `OMicrotonalSamplerAudioProcessorEditor`. Matches the Ouaricon family convention (O-Wind → `OWindAudioProcessor`, O-Bassoon → `OBassoonAudioProcessor`).
- **D-2 (sample loader at Stage 1).** Ship the `SampleLoader` class skeleton + `SampleMap` POD struct now. ~120 LOC total, freezes the message-thread-safe API surface so Stage 2.2 doesn't introduce a wave-2 plumbing change.
- **D-3 (PLUGIN_CODE).** `OMtS` reserved (RESEARCH.md §4 — confirmed unique against the 30 existing plugin codes).
- **D-4 (TuningEngine namespace).** Global namespace, **no `Ouaricon::` prefix** anywhere. `class TuningEngine` / `TuningEngine tuningEngine;` / `setTuningEngine(TuningEngine*)`. (RESEARCH.md §2 / O-Bassoon Stage 1 finding D2.)
- **D-5 (note-expression wiring).** Single `ouaricon_add_module(O-MicrotonalSampler note-expression)` line. **Do not emit** `target_link_libraries(... PRIVATE Ouaricon::note_expression)` — that target does not exist.
- **D-6 (WebView trio).** All three flags — `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` — committed at Stage 1, even though no WebView UI exists yet. Avoids Stage 3 CMake churn and matches project memory `WebView2 on Windows: Static vs Dynamic Linking`.

---

## Tasks

Execution order is sequential (one wave). Files compile cleanly only after task 7 completes; build verification is task 8.

### 1. [ ] Create `plugins/O-MicrotonalSampler/CMakeLists.txt`

**Files created:**
- `plugins/O-MicrotonalSampler/CMakeLists.txt`

**Depends on:** none

**Spec:**
- Mirror `plugins/O-Wind/CMakeLists.txt` structure (NOT O-Lyrica — O-Lyrica keeps a local copy of `TuningEngine.cpp` which is wrong for v1.0+ inheritance of upstream fixes).
- `juce_add_plugin(O-MicrotonalSampler ...)` with the canonical Stage 1 flag set (RESEARCH.md §3):
  - `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`
  - `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`
  - `PLUGIN_CODE OMtS` (D-3)
  - `FORMATS VST3 AU Standalone` (macOS gets all three; Windows builds VST3 only via the parent `if(APPLE)` block in O-Wind precedent — copy verbatim)
  - `PRODUCT_NAME "O-MicrotonalSampler${OUARICON_DEV_SUFFIX}"`
  - `PLUGIN_VERSION "1.0.0"`
  - `IS_SYNTH TRUE`
  - `NEEDS_MIDI_INPUT TRUE`
  - `NEEDS_MIDI_OUTPUT FALSE`
  - `IS_MIDI_EFFECT FALSE`
  - `NEEDS_WEB_BROWSER TRUE`
  - `NEEDS_WEBVIEW2 TRUE` (D-6)
  - `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`
- `target_sources(O-MicrotonalSampler PRIVATE ...)`:
  - `Source/PluginProcessor.cpp`
  - `Source/PluginEditor.cpp`
  - `Source/MicrotonalSamplerSound.h`
  - `Source/MicrotonalSamplerVoice.h`
  - `Source/MicrotonalSamplerVoice.cpp`
  - `Source/SampleMap.h`
  - `Source/SampleLoader.h`
  - `Source/SampleLoader.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp`
  - `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp`
- `target_include_directories(O-MicrotonalSampler PRIVATE Source ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp)`
- `ouaricon_add_module(O-MicrotonalSampler note-expression)` (D-5 — single line, no `target_link_libraries` equivalent)
- `target_link_libraries(O-MicrotonalSampler PRIVATE ...)` — full O-Wind module list. **Critical addition vs. O-Bassoon: `juce::juce_audio_formats` is required** (provides `AudioFormatManager`, `WavAudioFormat`, `AiffAudioFormat`, `LagrangeInterpolator` — even though Stage 1 doesn't yet use them, linking now means Stage 2.2 has zero CMake churn). Full list:
  - `juce::juce_audio_basics`
  - `juce::juce_audio_devices`
  - `juce::juce_audio_formats` (← new for this plugin family)
  - `juce::juce_audio_plugin_client`
  - `juce::juce_audio_processors`
  - `juce::juce_audio_utils`
  - `juce::juce_core`
  - `juce::juce_data_structures`
  - `juce::juce_dsp`
  - `juce::juce_events`
  - `juce::juce_graphics`
  - `juce::juce_gui_basics`
  - `juce::juce_gui_extra`
  - + the three Ouaricon `PUBLIC` config groups (`juce::juce_recommended_config_flags`, `juce::juce_recommended_lto_flags`, `juce::juce_recommended_warning_flags`).
- `juce_generate_juce_header(O-MicrotonalSampler)` — **MUST come after** `target_link_libraries(...)` (RESEARCH.md pitfall #2; `juce8-critical-patterns.md` #22).
- `target_compile_definitions(O-MicrotonalSampler PUBLIC ...)`:
  - `JUCE_VST3_CAN_REPLACE_VST2=0`
  - `JUCE_WEB_BROWSER=1`
  - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (D-6)
  - `JUCE_USE_CURL=0`
  - (Located after `juce_generate_juce_header`, matching O-Wind:108-114.)
- Optional licensing block at bottom (gated on `OUARICON_LICENSING`) — copy verbatim from O-Wind. Dead code at Stage 1 unless `-DOUARICON_LICENSING=ON` is passed at configure time.

**Forbidden:**
- No `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line. (D-5)
- No wildcard `target_sources(... ${CMAKE_SOURCE_DIR}/modules/tuning/note-expression/cpp/*)` glob — `ouaricon_add_module` already routes those files (and routes `cpp/vst3/*` to the per-format target only). RESEARCH.md pitfall #7.
- No `juce_add_binary_data(...)` at Stage 1 (no WebView resources yet). RESEARCH.md pitfall #8.

---

### 2. [ ] Create `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerSound.h`

**Files created:**
- `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerSound.h`

**Depends on:** Task 1 (CMake includes Source/ in include dir)

**Spec:**
- Header-only, byte-aligned with `plugins/O-Bassoon/Source/BassoonSound.h`. RESEARCH.md §9 has the exact body:

```cpp
#pragma once
#include <JuceHeader.h>

class MicrotonalSamplerSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote   (int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
```

**Forbidden:**
- No use of `juce::SamplerSound`. We deliberately ship a custom voice-pair (RESEARCH.md §9 — built-in `juce::SamplerSound` lacks velocity-layer crossfade hooks, NE/TuningEngine integration, and uses linear interp only).

---

### 3. [ ] Create `plugins/O-MicrotonalSampler/Source/SampleMap.h`

**Files created:**
- `plugins/O-MicrotonalSampler/Source/SampleMap.h`

**Depends on:** Task 1

**Spec:**

POD shape from RESEARCH.md §7 (committed verbatim):

```cpp
#pragma once
#include <JuceHeader.h>
#include <vector>

struct SampleSlot
{
    juce::AudioBuffer<float> audio;
    double                   sourceSampleRate = 0.0;
    int                      midiNote = -1;
    int                      velocityLayer = 0;     // 0..3
    int                      loopStart = 0;
    int                      loopEnd   = 0;         // 0 = no loop (one-shot)
};

struct SampleMap
{
    std::vector<SampleSlot> slots;
    int                     lowestNote        = 127;
    int                     highestNote       = 0;
    int                     numVelocityLayers = 1;  // 1..4

    // Stage 1 stub: returns nullptr unconditionally. Stage 2.2 implements real lookup.
    const SampleSlot* findSlot(int /*midiNote*/, int /*velocity*/) const noexcept
    {
        return nullptr;
    }
};
```

**Forbidden:**
- No `findSlot` body that touches `slots` — Stage 1 ships a no-op. The voice's silent-stub `renderNextBlock` never calls it anyway, but a no-op return guards against accidental dereferences during pluginval probes.
- No inheritance from `juce::ReferenceCountedObject` — RESEARCH.md §7 explicitly chose `std::shared_ptr` over `juce::ReferenceCountedObjectPtr` for lock-free atomic-swap semantics.

---

### 4. [ ] Create `plugins/O-MicrotonalSampler/Source/SampleLoader.{h,cpp}`

**Files created:**
- `plugins/O-MicrotonalSampler/Source/SampleLoader.h`
- `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp`

**Depends on:** Task 1, Task 3

**Spec:**

Header surface from RESEARCH.md §8 — frozen at Stage 1 so Stage 2.2 only fills in `run()`:

```cpp
// SampleLoader.h
#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include "SampleMap.h"

class SampleLoader : public juce::Thread
{
public:
    using CompletionCallback = std::function<void(std::shared_ptr<SampleMap>)>;
    using FailureCallback    = std::function<void(const juce::String&)>;

    SampleLoader();
    ~SampleLoader() override;

    void loadFolder(const juce::File& folder,
                    double targetSampleRate,
                    CompletionCallback onComplete,
                    FailureCallback    onFailure = nullptr);

    void cancelLoad();

private:
    void run() override;

    juce::File         pendingFolder;
    double             targetSampleRate    = 48000.0;
    CompletionCallback completionCallback;
    FailureCallback    failureCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleLoader)
};
```

**`SampleLoader.cpp` — Stage 1 stub bodies:**

- Constructor: `juce::Thread("SampleLoader")` base init; nothing else.
- Destructor: `stopThread(2000)` to ensure clean shutdown if `run()` is ever in flight.
- `loadFolder(folder, sr, onComplete, onFailure)`:
  - Stop any previous run with `stopThread(500)`.
  - Store args in members.
  - Stage 1 explicit stub behavior: dispatch failure callback via `juce::MessageManager::callAsync` with message `"SampleLoader stub — Stage 1; sample loading lands in Stage 2.2"`. **Do not** call `startThread()`.
  - Rationale: ensures the message-thread caller observes a deterministic "no map produced" outcome rather than a silent no-op.
- `cancelLoad()`: `stopThread(500);` (signal `run()` to bail; `run()` is empty at Stage 1 so this is defensive).
- `run()`: empty body. Stage 2.2 fills in: construct `juce::AudioFormatManager`, register basic formats, scan folder, parse filenames, load buffers, build `SampleMap`, dispatch completion via `MessageManager::callAsync`.

**Forbidden:**
- No `juce::AudioFormatManager` member — RESEARCH.md pitfall #9 / §8: must be constructed inside `run()` only.
- No actual file I/O at Stage 1 — pure surface scaffolding.
- No call to `startThread()` from `loadFolder` at Stage 1 — keeps the failure-callback path deterministic and avoids a brief background thread that does nothing.

---

### 5. [ ] Create `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.{h,cpp}`

**Files created:**
- `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.h`
- `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp`

**Depends on:** Task 1, Task 2, Task 3

**Spec:**
- Header surface byte-aligned with RESEARCH.md §10 (full method/setter signatures, no DSP):
  - Inherits `juce::SynthesiserVoice`.
  - `bool canPlaySound(juce::SynthesiserSound* s) override` — `dynamic_cast<MicrotonalSamplerSound*>(s) != nullptr`.
  - `void startNote(int, float, juce::SynthesiserSound*, int) override` — empty body (Stage 1).
  - `void stopNote(float, bool) override` — body calls `clearCurrentNote();` (immediate clear; ADSR release wired in Phase 2.1).
  - `void pitchWheelMoved(int) override` — empty.
  - `void controllerMoved(int, int) override` — empty.
  - `void renderNextBlock(juce::AudioBuffer<float>&, int, int) override` — empty (writes nothing — silent stub).
  - Four setters (called once per voice from PluginProcessor ctor):
    - `void setAPVTS(juce::AudioProcessorValueTreeState* p)`
    - `void setTuningEngine(TuningEngine* engine)` (D-4 — no namespace prefix)
    - `void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* src)`
    - `void setSampleMapSource(std::shared_ptr<SampleMap>* src)`
  - Four private members initialized to `nullptr`:
    - `juce::AudioProcessorValueTreeState* parameters = nullptr;`
    - `TuningEngine* tuningEngine = nullptr;`
    - `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;`
    - `std::shared_ptr<SampleMap>* sampleMapSource = nullptr;`
- Includes:
  - `<JuceHeader.h>`
  - `<memory>`
  - `"TuningEngine.h"` (resolved via `target_include_directories` from Task 1)
  - `"NoteExpression.h"` (resolved via `ouaricon_add_module` include path)
  - `"SampleMap.h"`
  - `"MicrotonalSamplerSound.h"` (for `dynamic_cast` in `canPlaySound`)
- `MicrotonalSamplerVoice.cpp` — minimal translation unit (defaulted ctor / out-of-line `stopNote` if not in header). Methods may be inline-in-header at this stage; the `.cpp` exists primarily so CMake `target_sources` has a real translation unit. Either form acceptable provided the build passes.

**Forbidden:**
- No DSP code (no varispeed read, no interpolation, no ADSR, no crossfade — those are Phase 2.1 / 2.3 / 2.4).
- No dereferencing of `parameters`, `tuningEngine`, `pendingTuningSource`, or `sampleMapSource` — Stage 1 voices receive these pointers and ignore them.
- No `setLatencySamples(...)` calls. RESEARCH.md pitfall #3 — `getLatencySamples()` is non-virtual in JUCE 8.
- No `juce::SamplerVoice` inheritance (RESEARCH.md §9 — explicitly built from scratch).

---

### 6. [ ] Create `plugins/O-MicrotonalSampler/Source/PluginProcessor.{h,cpp}`

**Files created:**
- `plugins/O-MicrotonalSampler/Source/PluginProcessor.h`
- `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp`

**Depends on:** Tasks 1-5

**Spec:**

**Class name:** `OMicrotonalSamplerAudioProcessor` (D-1).

**Header members (RESEARCH.md §6 — exact layout):**
```cpp
juce::AudioProcessorValueTreeState           parameters;
juce::Synthesiser                            synthesiser;
TuningEngine                                 tuningEngine;          // D-4: global namespace
Ouaricon::NoteExpression::VST3Extensions     vst3Extensions;
std::shared_ptr<SampleMap>                   currentSampleMap;      // atomic-swap target
std::unique_ptr<SampleLoader>                sampleLoader;          // owns juce::Thread
```

**Public methods (override surface):**
- `OMicrotonalSamplerAudioProcessor()` — constructor body per RESEARCH.md §6:
  - `BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)` only (no input bus — RESEARCH.md pitfall #1; `juce8-critical-patterns.md` #4).
  - Initialize `parameters(*this, nullptr, "Parameters", createParameterLayout())`.
  - Initialize `currentSampleMap = std::make_shared<SampleMap>();` (empty SampleMap with `findSlot` returning nullptr — keeps audio thread reads valid even before any sample folder is loaded).
  - Loop `for (int i = 0; i < 16; ++i)`:
    - `auto* voice = new MicrotonalSamplerVoice();`
    - `voice->setAPVTS(&parameters);`
    - `voice->setTuningEngine(&tuningEngine);`
    - `voice->setPendingTuningSource(&vst3Extensions.getPendingTable());`
    - `voice->setSampleMapSource(&currentSampleMap);`
    - `synthesiser.addVoice(voice);`
  - `synthesiser.addSound(new MicrotonalSamplerSound());`
  - `sampleLoader = std::make_unique<SampleLoader>();`
- `~OMicrotonalSamplerAudioProcessor() override` — defaulted (synthesiser owns voices; `sampleLoader` cleans up via its destructor's `stopThread`).
- `void prepareToPlay(double sampleRate, int samplesPerBlock) override`:
  - `synthesiser.setCurrentPlaybackSampleRate(sampleRate);`
  - **No** `setLatencySamples(...)` call. (Sampler is feed-forward; latency = 0; getter is non-virtual.) RESEARCH.md pitfall #3.
- `void releaseResources() override` — empty (Stage 1 owns no externally-allocated resources).
- `bool isBusesLayoutSupported(const BusesLayout&) const override` — accept stereo output only, refuse input. Copy from O-Wind/O-Lyrica.
- `void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override`:
  ```cpp
  juce::ScopedNoDenormals noDenormals;
  buffer.clear();
  vst3Extensions.drainAndUpdate();   // BEFORE renderNextBlock — RESEARCH.md pitfall #4
  synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
  ```
  No APVTS reads at Stage 1. No parameter smoothing. No per-voice mutation. No `output_gain` post-render multiply (Stage 2.4 wires that with `juce::SmoothedValue`).
- `juce::AudioProcessorEditor* createEditor() override` — `return new OMicrotonalSamplerAudioProcessorEditor(*this);` (Task 7).
- `bool hasEditor() const override` — `return true;`.
- `juce::VST3ClientExtensions* getVST3ClientExtensions() override` — `return &vst3Extensions;` (RESEARCH.md §1).
- Standard `getName / acceptsMidi / producesMidi / isMidiEffect / getTailLengthSeconds / getNumPrograms / getCurrentProgram / setCurrentProgram / getProgramName / changeProgramName / getStateInformation / setStateInformation` overrides — copy from O-Wind/O-Bassoon. `getStateInformation` / `setStateInformation` use APVTS XML round-trip (standard pattern).
  - `getTailLengthSeconds()` returns `0.0` at Stage 1 (silent stub has no tail; revisit at Phase 2.1 if ADSR release > 0 needs it surfaced).
- `juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }` — public accessor for the editor.

**`createParameterLayout()` — frozen 7-parameter spec (RESEARCH.md §5):**

Implement as `static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();`. All ranges, defaults, and IDs are committed:

| ID (string) | C++ class | Range | Default | Step / Skew | Suffix |
|---|---|---|---|---|---|
| `attack` | `juce::AudioParameterFloat` | `0.0f, 10.0f` | `0.005f` | step `0.001f`, skew `0.5f` | `" s"` |
| `decay` | `juce::AudioParameterFloat` | `0.0f, 10.0f` | `0.1f` | step `0.001f`, skew `0.5f` | `" s"` |
| `sustain` | `juce::AudioParameterFloat` | `0.0f, 1.0f` | `1.0f` | step `0.001f` | (none) |
| `release` | `juce::AudioParameterFloat` | `0.0f, 10.0f` | `0.3f` | step `0.001f`, skew `0.5f` | `" s"` |
| `polyphony` | `juce::AudioParameterInt` | `1, 16` | `16` | (int) | (none) |
| `velocity_crossfade` | `juce::AudioParameterFloat` | `0.0f, 1.0f` | `1.0f` | step `0.001f` | (none) |
| `output_gain` | `juce::AudioParameterFloat` | `-24.0f, 12.0f` | `0.0f` | step `0.1f` | `" dB"` |

Use `juce::ParameterID(id, 1)` for the version-hint argument (matches O-Wind/O-Bassoon). Display names are title-cased (`"Attack"`, `"Decay"`, `"Sustain"`, `"Release"`, `"Polyphony"`, `"Velocity Crossfade"`, `"Output Gain"`).

For `attack`/`decay`/`release`, use `juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.5f)` to apply the perceptual-log skew. For `sustain` and `velocity_crossfade`, plain `(0.0f, 1.0f, 0.001f)`. For `output_gain`, plain `(-24.0f, 12.0f, 0.1f)`.

**Forbidden:**
- No allocations in `processBlock` (RESEARCH.md pitfall, project memory PERF-01).
- No reads of `parameters` from the audio thread at Stage 1 — those land in Phase 2.1+.
- No call to `currentSampleMap->findSlot(...)` from `processBlock` — voice ownership of the lookup is enforced by giving voices `setSampleMapSource(&currentSampleMap)` instead.
- No tuning-related parameters in `createParameterLayout()` — tuning is consumed via NE+TuningEngine, never as APVTS (BRIEF.md "Tuning behavior is driven by VST3 note expression and the suite's internal tuning module — not exposed as plugin parameters.").

---

### 7. [ ] Create `plugins/O-MicrotonalSampler/Source/PluginEditor.{h,cpp}`

**Files created:**
- `plugins/O-MicrotonalSampler/Source/PluginEditor.h`
- `plugins/O-MicrotonalSampler/Source/PluginEditor.cpp`

**Depends on:** Task 6

**Spec:**

`OMicrotonalSamplerAudioProcessorEditor : public juce::GenericAudioProcessorEditor` — RESEARCH.md §11:

```cpp
// PluginEditor.h
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OMicrotonalSamplerAudioProcessorEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit OMicrotonalSamplerAudioProcessorEditor(OMicrotonalSamplerAudioProcessor& p)
        : juce::GenericAudioProcessorEditor(p)
    {
        setSize(500, 360);   // 7 params * ~40px row + breathing room
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OMicrotonalSamplerAudioProcessorEditor)
};
```

`PluginEditor.cpp` may be empty aside from `#include "PluginEditor.h"` (or contain the constructor body — either form acceptable). The placeholder auto-renders all 7 APVTS parameters as JUCE generic sliders, sufficient for Stage 1 manual smoke-test (knob-twiddle in DAW; verify host automation surfaces them).

**Forbidden:**
- No `juce::WebBrowserComponent`, no `juce::WebSliderRelay`, no resource provider — those are Stage 3.
- No sample-mapping grid, no waveform display, no drag-drop target — Stage 3 UI mockup work.
- No knob-graphic includes from `juce::ImageCache` etc. — bare generic editor only.

---

### 8. [ ] Build verification — macOS VST3 + AU + Standalone

**Files modified:** none (build only)

**Depends on:** Tasks 1-7

**Commands (run from project root):**
```bash
cmake --build build --target O-MicrotonalSampler_VST3
cmake --build build --target O-MicrotonalSampler_AU
cmake --build build --target O-MicrotonalSampler_Standalone
```

**Pass conditions:**
- All three targets build clean (no warnings escalated to errors that aren't pre-existing in the workspace).
- Build artefacts appear at:
  - `build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/VST3/O-MicrotonalSampler.vst3` (or `Debug/` per cmake config)
  - `build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/AU/O-MicrotonalSampler.component`
  - `build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/Standalone/O-MicrotonalSampler.app`
- JUCE-NE-PATCH marker check passes at configure time (RESEARCH.md pitfall #5 — failure mode is `[note-expression] JUCE patch marker 'JUCE-NE-PATCH' not found`; remediation `./scripts/apply-juce-patches.sh`).
- `juce::juce_audio_formats` linked in (smoke-test: a no-op `juce::AudioFormatManager mgr; mgr.registerBasicFormats();` line could be added to `prepareToPlay` and immediately removed; or simply observe that the link succeeds with the line in `target_link_libraries`).

**If build fails:** the executor must diagnose and fix in-place rather than skipping. Common failure modes:
- Forgot `juce_generate_juce_header` ordering → reorder after `target_link_libraries`.
- Forgot `target_include_directories` for `scala-tuning-engine/cpp/` → unresolved `TuningEngine.h`.
- Used `Ouaricon::TuningEngine` somewhere → unresolved class (D-4).
- Forgot `juce::juce_audio_formats` link → unresolved when `<juce_audio_formats/juce_audio_formats.h>` is pulled in transitively (Stage 1 may not surface this; Stage 2.2 will).
- JUCE-NE-PATCH absent → run `./scripts/apply-juce-patches.sh`.
- `<memory>` not included where `std::shared_ptr<SampleMap>` is used → add to PluginProcessor.h and MicrotonalSamplerVoice.h.

---

### 9. [ ] Install to system folders + DAW smoke test

**Files modified:** none (install only)

**Depends on:** Task 8

**Commands (per project CLAUDE.md cache-clearing protocol):**
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-MicrotonalSampler.component
cp -R build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/VST3/O-MicrotonalSampler.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/AU/O-MicrotonalSampler.component ~/Library/Audio/Plug-Ins/Components/
auval -a | grep -i microtonal   # verify AU registration
```

**Pass conditions:**
- `auval -a` lists `O-MicrotonalSampler` (or `O-MicrotonalSampler-DEV`) in the instrument category.
- AU validation does not need to be 100% clean for Stage 1 (DEF-24-01-style benign findings allowed per project memory `O-Lyrica is the validated spike/reference for note-expression`) — but no crashes during validation.

---

### 10. [ ] pluginval strictness 5 (Stage 1 acceptance level)

**Files modified:** none

**Depends on:** Task 9

**Command:**
```bash
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3
```

**Pass conditions:**
- Returns exit code 0.
- Output ends with `ALL TESTS PASSED`.

**Note:** Stage 4 raises this to strictness 10. Stage 1 only needs strictness 5 — silent-stub voices may not exercise enough surface for higher-strictness tests anyway.

**If pluginval fails:** the executor must triage. Common Stage 1 failures:
- `getTailLengthSeconds()` returning a finite value when synth has no tail logic — return `0.0` at Stage 1 (Phase 2.1 will revisit if ADSR release > 0 needs it).
- `BusesProperties` rejecting layouts pluginval probes — copy O-Wind's `isBusesLayoutSupported` exactly.
- `getVST3ClientExtensions()` returning nullptr — verify `vst3Extensions` is a long-lived member, not a stack temp.
- `SampleLoader::loadFolder` invoked under pluginval's automation probes — Stage 1 stub dispatches a failure callback to message thread; if pluginval's harness doesn't pump the message thread, the callback is benign (won't crash; just unobserved).

---

### 11. [ ] Update `plugins/O-MicrotonalSampler/.planning/STATUS.md`

**Files modified:**
- `plugins/O-MicrotonalSampler/.planning/STATUS.md`

**Depends on:** Tasks 8-10

**Spec:**
- `status:` → `execute_complete` (or whatever the verify-phase entry token is — match the workflow's convention; the verify phase will move it to `verify_complete` after the next command).
- Append a new "Stage 1 execute (foundation-shell)" section under "Completed So Far" with:
  - Files created (paths to the seven Source files + CMakeLists.txt)
  - Build pass (commit hashes)
  - pluginval strictness 5 pass
  - DAW load smoke pass (auval listing or DAW screenshot)
- Refresh `last_updated:` to today's date.
- Update `Next Steps:` to point at `/plugin-verify O-MicrotonalSampler 1-foundation`.

**Forbidden:**
- Do not modify `contract_checksums:` — those are owned by the contract-validation skill, not the executor.
- Do not advance to Stage 2 markers — verify phase owns that transition.

---

## Dependency Graph

```
Task 1 (CMakeLists.txt)
     │
     ├──> Task 2 (MicrotonalSamplerSound.h)
     │
     ├──> Task 3 (SampleMap.h)
     │         │
     │         └──> Task 4 (SampleLoader.{h,cpp})
     │         │
     │         └──> Task 5 (MicrotonalSamplerVoice.{h,cpp})  ◄── also needs Task 2
     │                            │
     │                            └──> Task 6 (PluginProcessor.{h,cpp})  ◄── also needs Task 4
     │                                              │
     │                                              └──> Task 7 (PluginEditor.{h,cpp})
     │                                                            │
     ├───────────────────────────────────────────────────────────> Task 8 (build VST3 + AU + Standalone)
     │                                                                          │
     │                                                                          └──> Task 9 (install + DAW smoke)
     │                                                                                        │
     │                                                                                        └──> Task 10 (pluginval --strictness 5)
     │                                                                                                       │
     │                                                                                                       └──> Task 11 (STATUS.md)
```

Tasks 2-7 could in principle execute as a fan-out — Tasks 2, 3 are siblings, then Tasks 4, 5 depend on 3 (and 5 also on 2), then 6 depends on 4+5, then 7 on 6. The natural author-order is sequential (1 → 2 → 3 → 4 → 5 → 6 → 7) and produces cleaner diff history. The foundation-shell-agent should write them in order, then run 8-11 sequentially.

---

## Success Criteria

The definitive verification list for Stage 1. Verifies COMPAT-01 (pluginval) plus structural invariants required by Stage 2:

- [ ] `cmake --build build --target O-MicrotonalSampler_VST3` succeeds (macOS).
- [ ] `cmake --build build --target O-MicrotonalSampler_AU` succeeds (macOS).
- [ ] `cmake --build build --target O-MicrotonalSampler_Standalone` succeeds (macOS).
- [ ] `cmake --build build --config Release --target O-MicrotonalSampler_VST3` succeeds on Windows (verify static linking compiles even if not run-tested every cycle — Mac-only Stage 1 build is acceptable provided the recipe is correct; Windows verification is a Stage 4 concern at the latest).
- [ ] `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3` passes.
- [ ] Plugin loads in Ableton or Logic without crash; appears in instrument category.
- [ ] All 7 APVTS parameters appear in the host's parameter list with correct names, ranges, defaults, and units (`s`, `dB` where applicable; `polyphony` shows as int 1-16).
- [ ] Plays silence (no audio bug, no crash) when MIDI notes are sent — verify by playing C3 in the host and confirming the meter reads zero.
- [ ] `getVST3ClientExtensions()` returns non-null `Ouaricon::NoteExpression::VST3Extensions*` (verified by Vst3PluginTestHost or pluginval VST3-extensions probe; alternatively a one-line print in `getVST3ClientExtensions` during Stage 1 build, removed before commit).
- [ ] JUCE-NE-PATCH CMake-time marker check passes at configure time.
- [ ] No `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line anywhere in `plugins/O-MicrotonalSampler/CMakeLists.txt` (D-5).
- [ ] No `Ouaricon::TuningEngine` token anywhere in `plugins/O-MicrotonalSampler/Source/` (D-4) — verify with `grep -rn "Ouaricon::TuningEngine" plugins/O-MicrotonalSampler/`.
- [ ] `NEEDS_WEBVIEW2 TRUE` present inside `juce_add_plugin(O-MicrotonalSampler ...)` block (D-6).
- [ ] `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` present in `target_compile_definitions` block (D-6).
- [ ] `juce::juce_audio_formats` listed in `target_link_libraries` (Stage 2 dependency surfaced now).
- [ ] `PLUGIN_CODE OMtS` set in `juce_add_plugin(...)` (D-3).
- [ ] `SampleMap` and `SampleLoader` classes compile and link; voice setter `setSampleMapSource(&currentSampleMap)` is reached during processor construction.
- [ ] `currentSampleMap` is a non-null `std::shared_ptr<SampleMap>` after construction (initialized in ctor with `std::make_shared<SampleMap>()`).

**Verifies requirements:** COMPAT-01 (pluginval pass).

---

## Out of Scope (Hand off to Stage 2 / Stage 3 / Stage 4)

Explicitly NOT in Stage 1 — do not implement:

- Sample loading logic (`SampleLoader::run` body, filename-convention parser, `AudioFormatManager` use, `LagrangeInterpolator` SR conversion) — Phase 2.2.
- `SampleMap::findSlot` real implementation (pitch × velocity-layer lookup, velocity-layer crossfade weight calc) — Phase 2.2 / 2.3.
- Varispeed read with cubic-Hermite interpolation in `MicrotonalSamplerVoice::renderNextBlock` — Phase 2.1.
- ADSR envelope generator (4 params → per-voice envelope) — Phase 2.1.
- NE consumption in `startNote` (`Ouaricon::NoteExpression::applyPendingTuning(...)` after `tuningEngine->getFrequency(...)`) — Phase 2.1.
- Equal-power velocity-layer crossfade (`velocity_crossfade` param) — Phase 2.3.
- `findFreeVoice` / `findVoiceToSteal` override (`polyphony` param + oldest-released steal order) — Phase 2.4.
- `output_gain` post-render multiply with `juce::SmoothedValue` — Phase 2.3 / 2.4.
- Sustain-loop auto-detection (DSP-05) — Phase 2.5.
- Manual loop-point override per sample (DSP-06) — Stage 3 UI.
- Sample-mapping grid UI, waveform display, drag-drop target — Stage 3.
- WebView resource bundle, parameter relays, attachments — Stage 3.
- Factory presets, CHANGELOG, Dorico playback template — Stage 4.

If the executor finds itself wanting to add any of the above to make Stage 1 "feel finished," **stop**. Stage 1 finishes silent on purpose — that's the whole point of staged implementation.

---

## Notes for the Executor (foundation-shell-agent)

- Read RESEARCH.md in full before touching any file. The dependency hierarchy in §6 is not optional — member-construction order and `getPendingTable()` lifetime correctness depend on it.
- When in doubt, **mirror O-Wind for CMake** and **O-Bassoon for Source/ shape** — those are the closest precedents. O-TextureForge's `CorpusLoader.{h,cpp}` is the SampleLoader template (lines 1-185).
- O-Lyrica has a known divergence (local copy of `TuningEngine.cpp` under `Source/DSP/`) that pre-dates the shared-module promotion. Do not copy from O-Lyrica's CMake — RESEARCH.md §2 documents this.
- If a copy-from-O-Lyrica step produces a token like `Ouaricon::TuningEngine` (because O-Lyrica uses it inside the local-copy `DSP/`-folder includes) — strip the namespace. D-4 stands.
- `std::atomic_load`/`std::atomic_store` on `std::shared_ptr` are deprecated in C++20 but still work on Apple Clang 16 / MSVC 19 (RESEARCH.md pitfall #10). Stage 1 doesn't use these calls — voice stub never touches `*sampleMapSource`. Stage 2.2 will introduce the actual atomic-load read path.
- Commit each task as its own git commit (atomic, per project workflow standard). Suggested commit messages:
  1. `feat(O-MicrotonalSampler): Stage 1 CMakeLists - juce_add_plugin + module wiring + audio_formats link`
  2. `feat(O-MicrotonalSampler): MicrotonalSamplerSound (trivial)`
  3. `feat(O-MicrotonalSampler): SampleMap POD struct`
  4. `feat(O-MicrotonalSampler): SampleLoader skeleton (juce::Thread)`
  5. `feat(O-MicrotonalSampler): MicrotonalSamplerVoice (silent stub with NE+TuningEngine+SampleMap setters)`
  6. `feat(O-MicrotonalSampler): PluginProcessor with APVTS + headless TuningEngine + NE drain + sample-map shared_ptr`
  7. `feat(O-MicrotonalSampler): GenericAudioProcessorEditor placeholder`
  8. `chore(O-MicrotonalSampler): Stage 1 build pass (VST3 + AU + Standalone)`
  11. `docs(O-MicrotonalSampler): STATUS update - Stage 1 execute complete`
- Tasks 2 + 3 are siblings (no inter-dependency) and could be combined into one commit if desired.
- If pluginval fails on the first run, **diagnose, do not skip**. Stage 1 silent-stub semantics should pass strictness 5 cleanly; failure usually indicates a real wiring bug (`getTailLengthSeconds`, bus layout, NE extensions lifetime, missing `audio_formats` link).

---

## References

- RESEARCH.md (this stage): `plugins/O-MicrotonalSampler/.planning/stages/1-foundation/RESEARCH.md`
- BRIEF.md: `plugins/O-MicrotonalSampler/.planning/BRIEF.md`
- REQUIREMENTS.md: `plugins/O-MicrotonalSampler/.planning/REQUIREMENTS.md`
- STATUS.md: `plugins/O-MicrotonalSampler/.planning/STATUS.md`
- Project conventions: `CLAUDE.md` (root), `troubleshooting/patterns/juce8-critical-patterns.md`, `spike-findings-VST-development` skill
- Reference plugins:
  - CMake template: `plugins/O-Wind/CMakeLists.txt`
  - Most recent NE+TuningEngine wiring: `plugins/O-Bassoon/Source/{PluginProcessor,BassoonVoice,BassoonSound}.{h,cpp}` (once Stage 1 lands; currently in-progress per `STATUS.md`) — fall back to `plugins/O-Lyrica/Source/{PluginProcessor,HarpSynthVoice,HarpSynthSound}.{h,cpp}` if O-Bassoon hasn't landed
  - Background loader: `plugins/O-TextureForge/Source/dsp/CorpusLoader.{h,cpp}`
- Shared modules: `modules/tuning/note-expression/`, `modules/tuning/scala-tuning-engine/`

---

## Next Phase

Ready for **execute** phase: `/plugin-execute O-MicrotonalSampler 1-foundation`

Executor: `foundation-shell-agent` with this PLAN.md + RESEARCH.md attached.
