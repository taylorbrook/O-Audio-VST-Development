---
title: "O-MicrotonalSampler Stage 1 (Foundation) — Research Phase"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Stage 1 research for O-MicrotonalSampler — a 16-voice sample-engine with VST3 Note-Expression-driven microtonal varispeed retune. No prior Stage 0 architecture artifact exists, so this RESEARCH.md is the first authoritative spec: it locks the CMake recipe (mirrors O-Wind/O-Lyrica WebView pattern), reserves PLUGIN_CODE OMtS, freezes the 7-parameter APVTS layout from BRIEF.md, and confirms module reuse (Ouaricon::NoteExpression v1.1.0 + global TuningEngine v2.0.0). Sample-loading architecture (background thread, AudioFormatManager, AudioBuffer ownership) is sketched at Stage 1 surface depth — voices ship as silent stubs with the wiring setters they will need at Stage 2 (sample buffer pointer, ADSR placeholder). Three deeper architectural questions — voice-stealing strategy, sustain-loop auto-detection algorithm, varispeed anti-aliasing margin at +50c — are explicitly deferred to Stage 2 research with documented evidence that nothing about Stage 1 wiring depends on those answers."
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
agents: [research, foundation-shell]
---

# O-MicrotonalSampler — Stage 1 Research (Foundation)

## Scope

Stage 1 is **mechanical wiring** for a 16-voice sample-engine plugin with VST3 Note Expression microtonal support. The work is well-precedented (O-Bassoon, O-Lyrica, O-Wind all share the same CMake + APVTS + NE + TuningEngine spine) — the only novelty at Stage 1 is the sample-loading **storage surface** (a member that will hold per-(pitch × velocity-layer) `juce::AudioBuffer<float>` references in RAM) and the **background loader** stub.

This research deliberately stays surface-level on Stage 1 concerns:
1. Confirm exact APIs of the two shared modules to wire (NoteExpression v1.1.0 + TuningEngine v2.0.0).
2. Lock the canonical CMake recipe — synth + WebView + tuning, copy-paste from O-Wind.
3. Freeze the 7-parameter APVTS from BRIEF.md exactly.
4. Reserve a unique 4-char `PLUGIN_CODE`.
5. Identify the sample-storage data structure and the background-loader thread shape (so Stage 2 has a place to drop the load logic without surface churn).
6. Identify deeper DSP questions that **defer to Stage 2 research** — Stage 1 wiring is invariant to their answers.

**Inputs reviewed:**
- `plugins/O-MicrotonalSampler/.planning/BRIEF.md` (7 params, varispeed retune, 4 vel layers, 16 voices, sample loading rules)
- `plugins/O-MicrotonalSampler/.planning/REQUIREMENTS.md` (22 reqs across FUNC/DSP/UI/PERF/COMPAT/QUAL)
- `plugins/O-MicrotonalSampler/.planning/STATUS.md` (ideation complete, no prior stages)
- `modules/tuning/note-expression/cpp/NoteExpression.h` (public API)
- `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h` (public API)
- `modules/cmake/OuariconModules.cmake` (module integration helper)
- `plugins/O-Wind/CMakeLists.txt` (closest CMake template — synth + NE + Scala + WebView)
- `plugins/O-Lyrica/Source/{PluginProcessor.{h,cpp}, HarpSynthVoice.{h,cpp}}` (NE + tuning voice wiring reference)
- `plugins/O-Bassoon/.planning/stages/1-foundation/RESEARCH.md` (most recent Stage 1 precedent — same module pair)
- `plugins/O-TextureForge/Source/dsp/CorpusLoader.{h,cpp}` (closest precedent for **background sample loading via `juce::Thread` + `juce::AudioFormatManager`**)
- `.claude/skills/spike-findings-VST-development/SKILL.md` (NE patch + Dorico expression-map gotchas)

---

## Investigating

- **Confirm `Ouaricon::NoteExpression::VST3Extensions` constructor and call sites** (consumed verbatim from O-Bassoon/O-Wind/O-Lyrica precedent).
- **Confirm `TuningEngine` class location and namespace** (global namespace per O-Bassoon Stage 1 finding D2).
- **Confirm canonical `juce_add_plugin` recipe for synth + WebView** including the three Windows WebView flags.
- **Reserve a unique 4-char `PLUGIN_CODE`** that does not collide with the existing 31-plugin suite.
- **Identify the sample-storage data structure** — what type is the sampler's "sample map" member, what owns the audio buffers, what is the read-side guarantee on the audio thread?
- **Identify the background-loader thread shape** so Stage 2 inherits a wiring surface, not a redesign.
- **Identify DSP questions that defer to Stage 2** so Stage 1 doesn't bottleneck on them.

---

## Research Findings

### 1. Note-Expression Module — Public API (CONFIRMED — REUSE)

**Source:** `modules/tuning/note-expression/cpp/NoteExpression.h`

- **Namespace:** `Ouaricon::NoteExpression`.
- **Type to expose as a long-lived `PluginProcessor` member:** `Ouaricon::NoteExpression::VST3Extensions`.
- **Constructor:** default — `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;` (no args; reserves 64 event slots internally).
- **`getVST3ClientExtensions()` override:** returns `&vst3Extensions` (raw pointer to long-lived member).
- **Per-block drain call:** `vst3Extensions.drainAndUpdate();` — single call at the **top of `processBlock`, BEFORE `synthesiser.renderNextBlock(...)`**. Critical: `drainAndUpdate` populates `PendingTuningTable` with semitone deltas; voices read it in `startNote` via `Ouaricon::NoteExpression::applyPendingTuning(...)`.
- **Voice wiring:** `voice->setPendingTuningSource(&vst3Extensions.getPendingTable());` once per voice in `PluginProcessor` ctor. `getPendingTable()` returns `PendingTuningTable&` (an `std::array<std::atomic<double>, 128>&`).
- **Voice-side consumption (Stage 2 — NOT Stage 1):** voice calls
  ```cpp
  currentFrequency = Ouaricon::NoteExpression::applyPendingTuning(
                         *pendingTuningSource, midiNoteNumber, currentFrequency);
  ```
  inside `startNote()` *after* the base frequency has been computed via `tuningEngine->getFrequency(...)`. Helper internally `exchange(0.0)` so retriggers don't inherit stale offsets. **At Stage 1, voices simply receive the source pointer for forward compatibility; the silent stub never reads it.**

**CMake wiring (canonical):** `ouaricon_add_module(O-MicrotonalSampler note-expression)` — globs `cpp/*.{cpp,h}` into SharedCode, globs `cpp/vst3/*` into the per-format `O-MicrotonalSampler_VST3` target only, adds `cpp/` as a private include directory (so `#include "NoteExpression.h"` resolves directly), and runs the `JUCE-NE-PATCH` marker check at *configure* time.

**Patch dependency:** Verify `JUCE-NE-PATCH` markers are present in the local JUCE fork before the first build. Quick check: `grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/ | wc -l` should return **4**. If not, run `./scripts/apply-juce-patches.sh`.

---

### 2. TuningEngine — Public API (CONFIRMED — REUSE)

**Source:** `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h`

- **Namespace:** **GLOBAL** — `class TuningEngine` (no `Ouaricon::` prefix). Confirmed by O-Bassoon Stage 1 finding D2 and direct grep of O-Wind/O-Lyrica consumer code.
- **Constructor:** default — `TuningEngine tuningEngine;` (no args). Default state: `Mode::TwelveTET`, A4 = 440.0, 12-TET frequency table built immediately.
- **Functionally identical to `juce::MidiMessage::getMidiNoteInHertz()` at v1.0** when in default 12-TET mode — meaningful only once a Scala scale is loaded (v1.1+ feature, out of Stage 1 scope).
- **Non-copyable:** `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningEngine)`. Pass to voices as raw pointer.
- **Voice-side use (Stage 2):** `currentFrequency = tuningEngine->getFrequency(midiNoteNumber);` (signature: `double getFrequency(int midiNote, int midiChannel = 0)` — channel reserved, pass nothing).
- **Voice wiring:** `setTuningEngine(TuningEngine* engine)` setter; voice stores raw pointer; Stage 1 stub never dereferences.
- **Thread safety:** `getFrequency` reads from `std::array<std::atomic<double>, 128> frequencyTable;` — lock-free on audio thread. Mutators are message-thread.

**CMake wiring (NOT a registered module):** Despite living under `modules/tuning/scala-tuning-engine/`, this module is wired by **direct file references** from the consumer (matches O-Wind:45-48, O-Bassoon Stage 1 RESEARCH §4). Reason: the four `.cpp` files have inter-dependencies (`TuningEngine.cpp` references `ScaleGenerator`, `EmbeddedTunings`, `TuningExporter`) and consumers want to link all four for forward compatibility, but `ouaricon_add_module(...)` would also pull in `module.cmake`'s install pipeline which isn't relevant to the engine itself.

```cmake
target_sources(O-MicrotonalSampler
    PRIVATE
        # ... plugin-local sources ...
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

target_include_directories(O-MicrotonalSampler
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)
```

---

### 3. CMake Recipe — `juce_add_plugin` Flags (CONFIRMED)

**Reference templates:** `plugins/O-Wind/CMakeLists.txt`, `plugins/O-Lyrica/CMakeLists.txt`, `plugins/O-Bassoon/.planning/stages/1-foundation/RESEARCH.md` Finding 3.

**Canonical Stage 1 `juce_add_plugin` flag set (synth with WebView):**

```cmake
juce_add_plugin(O-MicrotonalSampler
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OMtS                          # see Finding 5
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-MicrotonalSampler${OUARICON_DEV_SUFFIX}"
    PLUGIN_VERSION "1.0.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE                        # static-link WebView2LoaderStatic.lib on Windows
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

**Compile definitions block (separate, at the bottom of CMakeLists.txt):**
```cmake
target_compile_definitions(O-MicrotonalSampler
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```

**WebView trio of flags is mandatory** (per project memory `WebView2 on Windows: Static vs Dynamic Linking`):
1. `NEEDS_WEB_BROWSER TRUE` — enables WebBrowserComponent.
2. `NEEDS_WEBVIEW2 TRUE` — links `WebView2LoaderStatic.lib` on Windows.
3. `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` — selects static linking path inside JUCE (auto-defines `JUCE_USE_WIN_WEBVIEW2=1`).

**Required JUCE module link list** (12 modules, identical to O-Wind):
```cmake
target_link_libraries(O-MicrotonalSampler
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats        # ← critical: AudioFormatManager + WavAudioFormat + AiffAudioFormat
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils          # ← GenericAudioProcessorEditor
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp                  # ← optional at Stage 1; needed at Stage 2 for LagrangeInterpolator (sample-rate conversion on load)
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra            # ← WebView
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

**`juce_generate_juce_header(O-MicrotonalSampler)` must come AFTER `target_link_libraries(...)`** (JUCE 8 rule, captured in `juce8-critical-patterns.md` #22).

**`juce::juce_audio_formats` is the new dependency** for this plugin vs. the modal/physical-modeling synths in the suite. It provides `juce::AudioFormatManager`, `juce::AudioFormatReader`, `juce::WavAudioFormat`, `juce::AiffAudioFormat`, and `juce::LagrangeInterpolator` for the sample loader. Stage 1 doesn't yet *use* the loader, but linking the module now means Stage 2 implementation has zero CMake churn.

---

### 4. PLUGIN_CODE — `OMtS` (RESERVED HERE)

**Conflict check:** `grep -hn "PLUGIN_CODE" plugins/*/CMakeLists.txt | sort -u` returns 30 distinct codes across the existing suite. None match `OMtS`.

**Mnemonic:** **O**-**M**icro**t**onal**S**ampler.

**Used codes verified safe-against:** OuFm, OuTr, OBsn, OFPu, OuSr, OuDt, OuOr, OuTF, OuTx, OBas, OMbc, OuMa, OBls, OCbs, OGan, ORed, OSpS, OuCp, OuGS, OuIP, OuPs, OaSa, OBwd, OFCR, OLyr, OuAE, OuCh, OuDD, OuPr, OWnd.

`OMtS` is unique. Plan phase commits `PLUGIN_CODE OMtS` in CMakeLists.txt.

---

### 5. APVTS Layout — 7 Parameters (FROZEN FROM BRIEF.md)

The 7 parameters from BRIEF.md §Parameters lock exactly. No additions, no renames. Tuning behavior is **not** parameterized — it's driven by the suite's NE+TuningEngine pair, per BRIEF.md "Tuning behavior is driven by VST3 note expression and the suite's internal tuning module — not exposed as plugin parameters."

| ID | Type | Range | Default | Notes |
|---|---|---|---|---|
| `attack` | `AudioParameterFloat` | 0.0–10.0 s | 0.005 | step 0.001, skew 0.5 (perceptual log) |
| `decay` | `AudioParameterFloat` | 0.0–10.0 s | 0.1 | step 0.001, skew 0.5 |
| `sustain` | `AudioParameterFloat` | 0.0–1.0 | 1.0 | step 0.001, linear |
| `release` | `AudioParameterFloat` | 0.0–10.0 s | 0.3 | step 0.001, skew 0.5 |
| `polyphony` | `AudioParameterInt` | 1–16 | 16 | int — voice cap (user-trim down for CPU) |
| `velocity_crossfade` | `AudioParameterFloat` | 0.0–1.0 | 1.0 | step 0.001, linear (UI displays as 0–100%) |
| `output_gain` | `AudioParameterFloat` | -24.0 to +12.0 dB | 0.0 | step 0.1, suffix " dB" via attribute |

**Param ID convention:** snake_case, matches O-Wind / O-Lyrica conventions. (O-Bassoon used the same.)

**Stage 1 reads:** none. The placeholder `GenericAudioProcessorEditor` will auto-render all 7 — sufficient for manual smoke-test (knob-twiddle in a DAW; verify automation reaches the parameter; verify save/restore round-trips through `getStateInformation` / `setStateInformation`).

**Stage 2.x will wire:** ADSR (4 params) → per-voice ADSR generator. `polyphony` → `findFreeVoice` override that respects the dynamic cap. `velocity_crossfade` → equal-power crossfade weight at velocity-layer boundaries. `output_gain` → master gain at the bottom of `processBlock` (smoothed via `juce::SmoothedValue` to avoid zipper noise).

---

### 6. PluginProcessor Member Layout (CONFIRMED PATTERN)

The Stage 1 surface is a **literal merge of O-Bassoon (NE+TuningEngine pair) and O-TextureForge (background sample loader)**. No novelty:

```cpp
// PluginProcessor.h — member section (Stage 1 surface)

juce::AudioProcessorValueTreeState           parameters;        // APVTS — 7 params per Finding 5
juce::Synthesiser                            synthesiser;       // 16 voices pre-allocated in ctor
TuningEngine                                 tuningEngine;      // global namespace, default 12-TET A4=440
Ouaricon::NoteExpression::VST3Extensions     vst3Extensions;    // long-lived, default-constructed

// Sample-map storage (see Finding 7)
std::shared_ptr<SampleMap>                   currentSampleMap;  // atomic-swap by background loader

// Background sample loader (see Finding 8)
std::unique_ptr<SampleLoader>                sampleLoader;      // owns juce::Thread; ctor in prepareToPlay

juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
```

**Member-construction order in `PluginProcessor::PluginProcessor()`:**
```cpp
OMicrotonalSamplerAudioProcessor::OMicrotonalSamplerAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))   // synth — output only
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new MicrotonalSamplerVoice();
        voice->setAPVTS(&parameters);
        voice->setTuningEngine(&tuningEngine);
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable());
        // Sample-map pointer is set later — voice reads via shared_ptr load() each startNote
        voice->setSampleMapSource(&currentSampleMap);
        synthesiser.addVoice(voice);
    }
    synthesiser.addSound(new MicrotonalSamplerSound());
}
```

**Per-block drain at the top of `processBlock`** (mirrors O-Bassoon Stage 1 RESEARCH §6):
```cpp
juce::ScopedNoDenormals noDenormals;
buffer.clear();

vst3Extensions.drainAndUpdate();                 // top of block, before render
synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
// (Stage 2 will apply output_gain post-render with juce::SmoothedValue)
```

---

### 7. Sample-Map Storage — `std::shared_ptr<SampleMap>` (DESIGN, MIRRORS O-TextureForge)

**Problem:** The audio thread (`renderNextBlock` → voice `startNote`) needs lock-free access to a sample map (pitch × velocity-layer → `juce::AudioBuffer<float>`). The message thread (background loader completion callback) needs to swap a freshly-loaded map atomically without a lock.

**Solution (precedent: O-TextureForge `SharedCorpus`):** Wrap the entire sample map in `std::shared_ptr<SampleMap>`. The processor holds an `std::atomic<std::shared_ptr<SampleMap>>` (via `std::atomic_load` / `std::atomic_store` on `shared_ptr` — note: deprecated in C++20, use `std::atomic<std::shared_ptr<T>>` directly when available, or fall back to message-thread swap with audio-thread `weak_ptr` lock).

**Stage 1 simplification:** Use plain `std::shared_ptr<SampleMap>` member with `std::atomic_load`/`std::atomic_store` (works on JUCE-supported toolchains). Voices grab `shared_ptr` once per `startNote` (rare event, lock-free read of the atomic), hold it for the duration of the note, and release on `clearCurrentNote`. **Audio thread never allocates** — `shared_ptr` copy-construct is a refcount increment (atomic, lock-free, real-time-safe).

**`SampleMap` shape (Stage 1 stub — Stage 2 fills in):**
```cpp
// Source/SampleMap.h
struct SampleSlot
{
    juce::AudioBuffer<float> audio;     // owns the loaded PCM data; mono or stereo
    double                   sourceSampleRate = 0.0;   // pre-resample SR; voice computes ratio relative to current host SR
    int                      midiNote = -1;
    int                      velocityLayer = 0;        // 0..3 (up to 4 layers per pitch)
    int                      loopStart = 0;            // sample index — Stage 2 may auto-detect or default to 0
    int                      loopEnd   = 0;            // 0 = no loop (one-shot)
};

struct SampleMap
{
    std::vector<SampleSlot>  slots;                    // flat — voice does lookup by (note, layer)
    int                      lowestNote  = 127;
    int                      highestNote = 0;
    int                      numVelocityLayers = 1;    // 1..4

    const SampleSlot* findSlot(int midiNote, int velocity) const noexcept;   // Stage 2 implements
};
```

**At Stage 1, `SampleMap` ships with the struct definition + an inline `findSlot` returning `nullptr`** — this is enough for the silent-stub voice to compile and run pluginval without crashing.

**Why `shared_ptr` and not `juce::ReferenceCountedObjectPtr`:** ReferenceCountedObjectPtr requires the inner type to inherit `juce::ReferenceCountedObject` and uses message-thread-only release. `shared_ptr` works with the plain POD struct above and gives true lock-free atomic-swap semantics on the audio thread.

---

### 8. Background Sample Loader — `juce::Thread` Subclass (DESIGN, MIRRORS O-TextureForge::CorpusLoader)

**Problem:** `juce::AudioFormatManager` + `AudioFormatReader::read(...)` are blocking I/O. Must not run on the audio thread (PERF-01: real-time safe `processBlock`).

**Solution:** Subclass `juce::Thread`, mirror `O-TextureForge::CorpusLoader` (`plugins/O-TextureForge/Source/dsp/CorpusLoader.{h,cpp}`):

```cpp
// Source/SampleLoader.h (Stage 1 surface — Stage 2 fills in run())
class SampleLoader : public juce::Thread
{
public:
    using CompletionCallback = std::function<void(std::shared_ptr<SampleMap>)>;
    using FailureCallback    = std::function<void(const juce::String&)>;

    SampleLoader();
    ~SampleLoader() override;

    // Called from message thread (UI drag-drop, file chooser).
    // Stage 1: stub — logs the request and immediately calls onFailure with "Stage 1 stub".
    // Stage 2: parses filenames, loads with AudioFormatManager, builds SampleMap, callback.
    void loadFolder(const juce::File& folder,
                    double targetSampleRate,
                    CompletionCallback onComplete,
                    FailureCallback    onFailure = nullptr);

    void cancelLoad();

private:
    void run() override;   // Stage 2 fills in

    juce::File pendingFolder;
    double     targetSampleRate = 48000.0;
    CompletionCallback completionCallback;
    FailureCallback    failureCallback;
};
```

**Key Stage 1 invariants:**
- `juce::AudioFormatManager` is **constructed inside `run()`**, not as a member — keeps the message-thread / audio-thread surface clean. (Matches CorpusLoader.cpp:170.)
- Completion callback is dispatched via `juce::MessageManager::callAsync` so the audio thread never sees a direct callback from the loader thread.
- Atomic-swap of `currentSampleMap` happens on the **message thread** via `std::atomic_store(&processor.currentSampleMap, newMap)`. Audio thread reads via `std::atomic_load`.

**Stage 1 ships with:**
- `SampleLoader.h` + `SampleLoader.cpp` skeleton (ctor / dtor / `loadFolder` stub / empty `run()`).
- Wiring in `PluginProcessor::prepareToPlay` to construct the loader.
- No actual sample loading at Stage 1 — the engine plays silence until Stage 2.2 fills in `run()`.

**Why now:** Defining the `SampleLoader` class surface at Stage 1 means Stage 2.2 has a place to drop the actual loading code without touching `PluginProcessor` headers or CMake. This avoids a Stage 1→Stage 2 wiring churn cycle.

**Reference reading order:** `plugins/O-TextureForge/Source/dsp/CorpusLoader.{h,cpp}` lines 1-185 — the load/cancel/run/loadAudioFile methods are the closest 1:1 precedent.

---

### 9. MicrotonalSamplerSound — Trivial (CONFIRMED PATTERN)

```cpp
// Source/MicrotonalSamplerSound.h
#pragma once
#include <JuceHeader.h>

class MicrotonalSamplerSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote   (int)  override { return true; }
    bool appliesToChannel(int)  override { return true; }
};
```

Single shared instance, header-only. Owned by `synthesiser.addSound(new MicrotonalSamplerSound())`. Matches O-Bassoon `BassoonSound.h`, O-Lyrica `HarpSynthSound.h`, O-Wind `FluteSynthSound.h` byte-for-byte.

**Note vs `juce::SamplerSound`:** JUCE ships a built-in `juce::SamplerSound` + `juce::SamplerVoice` pair (in `juce_audio_basics`). It's deliberately **not** used here because (a) it locks one sound to one source `AudioBuffer` (no built-in velocity-layer crossfade), (b) it doesn't expose hooks for VST3 NE + TuningEngine integration, (c) its varispeed renderer uses linear interpolation only (DSP-02 requires ≥3rd-order). The custom voice in Finding 10 is closer in spirit to JUCE's `SamplerVoice` but built from scratch for control over interpolation, layer crossfade, and tuning sources.

---

### 10. MicrotonalSamplerVoice — Silent Stub Surface (CONFIRMED)

Stage 1 builds the class with the full method/setter surface but a no-op `renderNextBlock`. Voices receive setters in the processor constructor and store raw pointers / shared_ptr handles; nothing is dereferenced in DSP at Stage 1.

```cpp
// Source/MicrotonalSamplerVoice.h (Stage 1 surface — no DSP yet)
#pragma once
#include <JuceHeader.h>
#include "TuningEngine.h"
#include "NoteExpression.h"      // resolved via ouaricon_add_module include path
#include "SampleMap.h"
#include "MicrotonalSamplerSound.h"

class MicrotonalSamplerVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<MicrotonalSamplerSound*>(s) != nullptr;
    }

    void startNote(int /*midiNote*/, float /*velocity*/, juce::SynthesiserSound*, int) override {}
    void stopNote (float, bool /*allowTailOff*/) override
    {
        clearCurrentNote();   // immediate clear at Stage 1; Stage 2.1 wires ADSR release
    }
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>&, int /*startSample*/, int /*numSamples*/) override {}

    // Wiring setters (called once per voice from PluginProcessor ctor)
    void setAPVTS              (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine       (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }
    void setSampleMapSource    (std::shared_ptr<SampleMap>* src)       { sampleMapSource = src; }

private:
    juce::AudioProcessorValueTreeState*           parameters          = nullptr;
    TuningEngine*                                 tuningEngine        = nullptr;
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    std::shared_ptr<SampleMap>*                   sampleMapSource     = nullptr;   // Stage 2: load() in startNote, hold for note duration
};
```

This surface is byte-aligned with what Stage 2.1–2.4 will need — no method-signature churn between Stage 1 and Stage 2 sample-playback wiring. Specifically:
- `setAPVTS` → ADSR + crossfade params (Stage 2.1, 2.3, 2.4).
- `setTuningEngine` → Stage 2.1 base-frequency calc.
- `setPendingTuningSource` → Stage 2.1 NE delta apply (mirrors O-Lyrica `HarpSynthVoice.cpp:143-147`).
- `setSampleMapSource` → Stage 2.2 sample lookup + per-voice retain via `std::atomic_load` then local `shared_ptr` copy.

---

### 11. PluginEditor — `GenericAudioProcessorEditor` Placeholder (CONFIRMED)

```cpp
// Source/PluginEditor.h (Stage 1 placeholder — replaced wholesale at Stage 3)
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
};
```

Replaced wholesale at Stage 3 (WebView). For Stage 1, `GenericAudioProcessorEditor` auto-renders all 7 APVTS parameters — sufficient for manual smoke-test in a DAW.

---

### 12. Sample-Loading Format Coverage (CONFIRMED)

`juce::AudioFormatManager::registerBasicFormats()` registers `WavAudioFormat` + `AiffAudioFormat`, which together handle **every format COMPAT-02 calls out**:
- `.wav` — `WavAudioFormat` reads 16/24/32-bit PCM, 32-bit float, mono/stereo, any sample rate from 8 kHz to 192 kHz+.
- `.aif` / `.aiff` — `AiffAudioFormat` same coverage.

Sample-rate conversion on load (when source SR ≠ host SR) uses `juce::LagrangeInterpolator` (3rd-order) — matches O-TextureForge `CorpusLoader.cpp:206-225`. This handles host-SR mismatch in the load path; the per-note ±50c retune (Stage 2.1) is a separate, finer-grained varispeed step that operates on the already-resampled buffer.

**Stage 1 ships:** the `juce::juce_audio_formats` link dependency. No load code yet.

---

## Module Reuse Confirmation

| Module | Version | Wiring | Purpose at Stage 1 |
|---|---|---|---|
| `note-expression` | v1.1.0 (per `module.yaml`; header doc-string says v1.0.0 — known stale, ignore) | `ouaricon_add_module(O-MicrotonalSampler note-expression)` — globs `cpp/*.{cpp,h}` to SharedCode, `cpp/vst3/*` to VST3 target, adds `cpp/` include dir, runs JUCE-NE-PATCH check | Type member `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;`, `getVST3ClientExtensions()` override, `vst3Extensions.drainAndUpdate()` at top of `processBlock`. Voice receives a `PendingTuningTable*` but never reads it. |
| `scala-tuning-engine` | v2.0.0 (per `TuningEngine.h` doc-block — `module.yaml` says v2.1.0; treat as additive bug-fix bump, no API change vs O-Bassoon Stage 1 finding) | Direct `cpp/{TuningEngine,ScaleGenerator,EmbeddedTunings,TuningExporter}.cpp` references via `target_sources` + `target_include_directories(... PRIVATE ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp)` | Type member `TuningEngine tuningEngine;` (global ns), default-constructed (12-TET, A4=440). Voice receives `TuningEngine*` but never reads it. |

**No new shared modules required for Stage 1.** No Ouaricon module exists for sample loading — `juce::AudioFormatManager` provides everything needed, and the loader is a small enough piece (~100 LOC at Stage 2) that promoting it to a shared module is a v1.1+ consideration once a second sample-engine plugin lands.

---

## Files Stage 1 Will Create

| Path | Purpose |
|---|---|
| `plugins/O-MicrotonalSampler/CMakeLists.txt` | juce_add_plugin recipe + module wiring + target_link_libraries + WebView resource block + compile defs |
| `plugins/O-MicrotonalSampler/Source/PluginProcessor.h` + `.cpp` | APVTS, Synthesiser, NE, TuningEngine, SampleMap, SampleLoader; `processBlock` drain → render |
| `plugins/O-MicrotonalSampler/Source/PluginEditor.h` (header-only) | `GenericAudioProcessorEditor` placeholder — replaced wholesale at Stage 3 |
| `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerSound.h` | trivial `appliesToNote/Channel` returning true |
| `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.h` + `.cpp` | silent-stub voice with full setter surface (APVTS, TuningEngine, NE pending source, sample-map source) |
| `plugins/O-MicrotonalSampler/Source/SampleMap.h` | POD `SampleSlot` + `SampleMap` struct with stub `findSlot` returning nullptr |
| `plugins/O-MicrotonalSampler/Source/SampleLoader.h` + `.cpp` | `juce::Thread` subclass with stub `run()`, `loadFolder` API surface frozen |

## Files Stage 1 Will Reference (Read-Only)

- `modules/tuning/note-expression/cpp/NoteExpression.h` (via `ouaricon_add_module` include path)
- `modules/tuning/scala-tuning-engine/cpp/{TuningEngine,ScaleGenerator,EmbeddedTunings,TuningExporter}.{h,cpp}` (direct file refs + include dir)
- Templates: `plugins/O-Wind/CMakeLists.txt` (CMake recipe), `plugins/O-Lyrica/Source/PluginProcessor.{h,cpp}` (NE + tuning wiring), `plugins/O-TextureForge/Source/dsp/CorpusLoader.{h,cpp}` (background loader pattern)

---

## Pitfalls / Landmines

(Drawn from project memory + `juce8-critical-patterns.md` + `spike-findings-VST-development` + O-Bassoon Stage 1 RESEARCH lessons.)

1. **Output-only `BusesProperties`** for synths. `BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)` only — *never* `.withInput(...)` on a synth. (`juce8-critical-patterns.md` #4.)
2. **`juce_generate_juce_header(O-MicrotonalSampler)` after `target_link_libraries`** — JUCE 8 requirement (`juce8-critical-patterns.md` #22).
3. **`getLatencySamples()` is non-virtual in JUCE 8** — do NOT override. Sampler is feed-forward; latency=0 by default. (Project memory.)
4. **Drain BEFORE `renderNextBlock`** — `vst3Extensions.drainAndUpdate()` must populate `PendingTuningTable` before any voice's `startNote` runs. (`spike-findings-VST-development` Pattern 4.)
5. **JUCE-NE-PATCH absence at configure time** — `module.cmake` does a `file(READ)` + `string(FIND)` for the marker in two JUCE source files. If the local JUCE fork hasn't been re-patched after a JUCE upgrade, configure fails. Fix: `./scripts/apply-juce-patches.sh`.
6. **`PLUGIN_CODE` collision** — `OMtS` is reserved in Finding 4. Failing to use a unique 4-char code makes plugins overwrite each other in the AU registry on macOS.
7. **Don't sweep `cpp/vst3/` into SharedCode** — `ouaricon_add_module` already routes per-format files into the VST3 target only. `foundation-shell-agent` must NOT add wildcard `target_sources` lines that re-include them.
8. **WebView trio of flags is mandatory** — `NEEDS_WEB_BROWSER TRUE` + `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. Missing any → blank WebView on Windows. (Project memory.)
9. **`juce::AudioFormatManager` constructed inside `run()` only** — never as a `PluginProcessor` member. The manager itself is fine real-time-wise but constructing it allocates and registers formats; doing it lazily inside the loader thread keeps the audio-thread surface clean. (Matches CorpusLoader.cpp pattern.)
10. **`std::atomic_load` / `std::atomic_store` on `std::shared_ptr` are deprecated in C++20** — Stage 1 still uses them per the JUCE-supported toolchain (Apple Clang 16, MSVC 19). If a future C++23 cutover lands, migrate to `std::atomic<std::shared_ptr<T>>`. Stage 1 surface is unchanged either way; this is a one-line implementation swap in `processBlock` / startNote.
11. **No streaming from disk** — RAM-only per BRIEF.md. The loader fully decodes each `.wav`/`.aif` into a `juce::AudioBuffer<float>` before publishing the new `SampleMap`. Out-of-memory on huge libraries is an explicit user concern; v1.0 doesn't guard against it.
12. **One-shot `juce::SamplerVoice` is not used** — see Finding 9. Custom voice gives control over interpolation order, velocity-layer crossfade, and NE/TuningEngine hookup.
13. **AU registration on macOS** — first launch of a fresh build hits `auval` cache; the project memory `Plugin Cache Clearing` note in `CLAUDE.md` applies (kill `AudioComponentRegistrar`, clear caches, install fresh). Stage 4 polish phase formalizes; Stage 1 just needs a single successful build to land in `~/Library/Audio/Plug-Ins/Components/`.

---

## Ouaricon Family Conventions (Verified)

These are not Stage 1-specific but `foundation-shell-agent` should follow them by reflex:

- **`COMPANY_NAME "${OUARICON_COMPANY_NAME}"`** — variable defined at the root `CMakeLists.txt`. Same for `OUARICON_MANUFACTURER_CODE` and `OUARICON_DEV_SUFFIX`.
- **`PRODUCT_NAME "O-MicrotonalSampler${OUARICON_DEV_SUFFIX}"`** — suffix is `-DEV` for non-release builds (separates dev plugin from installed version in the DAW scanner).
- **`PLUGIN_VERSION "1.0.0"`** at Stage 1 — bump only on user-visible release.
- **`FORMATS VST3 AU Standalone`** — no AAX, no LV2 in the Ouaricon family. macOS gets all three; Windows builds VST3 only (Standalone usually excluded; matches O-Wind line 6).
- **Licensing block** at the bottom of CMakeLists.txt (compile-flag-gated `OUARICON_LICENSING`), exact copy from O-Wind/O-Lyrica:
  ```cmake
  if(OUARICON_LICENSING)
      ouaricon_add_module(O-MicrotonalSampler licensing)
      target_compile_definitions(O-MicrotonalSampler PRIVATE OUARICON_LICENSING_ENABLED=1)
      target_link_libraries(O-MicrotonalSampler PRIVATE juce::juce_cryptography)
  endif()
  ```
  Stage 1 ships this block; it's dead code unless `-DOUARICON_LICENSING=ON` is set at configure time.

---

## Architectural Questions Deferred to Stage 2 Research

The BRIEF.md "Next Steps" lists three Stage-0-flavor research questions that are **not blocking Stage 1**. Stage 1 wiring is invariant to their answers — the surface above (silent-stub voice, sample-map shape, NE+TuningEngine hookup) doesn't depend on the outcomes. Capturing them here so Stage 2 research has a working agenda:

| # | Question | Why Stage 2 (not Stage 1) | Affected requirement |
|---|---|---|---|
| **Q1** | **Voice-stealing strategy** — when 17th note arrives with all 16 voices active, what's the steal order? BRIEF.md says "oldest-released first; oldest-held if all are held". Need to override `juce::Synthesiser::findVoiceToSteal` (default impl is oldest-active, ignores release state). | Stage 1 pre-allocates 16 voices and lets default JUCE behavior steal; that's enough for pluginval and a manual 16-note smoke-test. The custom override lands in Stage 2.4 alongside the ADSR release stage (the two are intertwined — "released" is meaningless until ADSR exists). | FUNC-07 (Voice stealing — verified at Stage 2). Stage 1 acceptance: 16 voices play simultaneously and the 17th doesn't crash. |
| **Q2** | **Sustain-loop auto-detection algorithm** — BRIEF.md says "scan each sample for a low-energy zero-crossing region in the latter portion of the file and loop there during sustain". Specific algorithm (RMS window size, search range, crossfade width at loop boundary, fallback when no usable region) is not yet specified. | Stage 1 ships `SampleSlot::loopStart = 0` / `loopEnd = 0` (= no loop, one-shot). Stage 2.5 implements the auto-detect pass that runs once at load time. The voice's `renderNextBlock` doesn't need to know whether a loop exists at Stage 1 — it's not rendering audio. | DSP-05 (auto-detect — verified Stage 2). DSP-06 (manual override — verified Stage 3 UI). |
| **Q3** | **Varispeed anti-aliasing margin at +50c** — BRIEF.md claims "anti-aliasing for upward varispeed (≤+50c → ~3% speedup) is handled by the interpolator's natural rolloff; further filtering is unnecessary". This needs measurement: for a sine sweep up to Nyquist (24 kHz at 48 kHz host), does cubic-Hermite at +3% rate produce audible foldback? **Likely yes** for content above ~8 kHz — the interpolator's natural rolloff is mild. May need a 1st-order LPF tilted at Nyquist before the interp, or a higher-order interpolator (Lagrange-5 / sinc-windowed). | Stage 1 voice doesn't read samples — no audio is produced. Stage 2.1 introduces the read path; Stage 2.1 acceptance test (sine-sweep null test) is the natural place to measure aliasing. If +50c needs a pre-filter, it's a one-line `juce::dsp::IIR::Filter` insertion ahead of the interpolator. | DSP-01, DSP-02, QUAL-01 (no aliasing — verified Stage 2). |

**Why this matters for Stage 1 success:** none of the three questions require an answer to land a working Stage 1 build. The plugin will load in a DAW, expose 7 parameters, accept MIDI without crashing, and pass pluginval `--strictness 10` — the full Stage 1 acceptance bar (per REQUIREMENTS.md COMPAT-01).

---

## Open Questions (Stage 1 Plan-Phase Only)

**None blocking.** The plan phase can proceed directly. Two minor decisions are flagged for the plan author to resolve:

- **D-1: Class-name prefix in code** — `OMicrotonalSamplerAudioProcessor` is verbose. Reference plugins use the unprefixed plugin name (e.g. O-Wind → `OWindAudioProcessor`, O-Bassoon → `OBassoonAudioProcessor`). Recommend `OMicrotonalSamplerAudioProcessor` to match (keeps the `O` prefix for namespace clarity in the AU registry). Plan phase commits.
- **D-2: Should Stage 1 ship the `SampleLoader` class skeleton or defer it to Stage 2.2?** Recommend **ship at Stage 1**. The class is ~50 LOC of header + stub `.cpp` and freezing the message-thread-safe API surface now means Stage 2.2 doesn't introduce a wave-2 plumbing change. Plan phase commits.

---

## Next Phase

Ready for **plan** phase: `/plugin-plan O-MicrotonalSampler 1-foundation`

Plan-phase scope: produce a single-pass execution plan (one wave) that spawns `foundation-shell-agent` with this RESEARCH.md attached. The agent creates the 7 Stage 1 files (CMakeLists.txt + 6 source files), reserves `PLUGIN_CODE OMtS`, freezes the 7-parameter APVTS, and lands a clean pluginval-passing build. No CONTEXT.md is needed since no discuss phase ran for Stage 1 — this RESEARCH.md is self-contained.
