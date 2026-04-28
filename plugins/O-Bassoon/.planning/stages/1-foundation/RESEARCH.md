---
title: "O-Bassoon Stage 1 (Foundation) — Research Phase"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Light Stage 1 research — confirms exact APIs of the two shared modules (Ouaricon::NoteExpression v1.1.0 and global TuningEngine v2.0.0), nails down the canonical CMake recipe (NEEDS_WEBVIEW2 must be added to CONTEXT.md's flag list), reserves PLUGIN_CODE OBsn (OBas is taken by O-Bass), and supplies copy-paste-ready include/wiring snippets for foundation-shell-agent. No novel APIs; no architectural changes. Three discrepancies vs. CONTEXT.md / ARCHITECTURE.md flagged for plan-phase reconciliation."
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
agents: [research, build, foundation-shell]
---

# O-Bassoon — Stage 1 Research (Foundation)

## Scope

Stage 1 is mechanical wiring against well-precedented patterns (O-Lyrica, O-Wind). This research phase deliberately stays light — its single job is to verify the three Stage-0 follow-ups that CONTEXT.md flagged, lock in exact API names, and surface the small set of discrepancies between CONTEXT.md / ARCHITECTURE.md and what the code in `modules/` actually exports. The plan phase consumes this directly.

**Inputs reviewed:**
- `plugins/O-Bassoon/.planning/stages/1-foundation/CONTEXT.md` (discuss findings)
- `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` (Stage 0 spec)
- `plugins/O-Bassoon/.planning/parameter-spec-draft.md` (locked param list)
- `plugins/O-Bassoon/.planning/ROADMAP.md` (Stage 1 component list)
- `modules/tuning/note-expression/cpp/NoteExpression.h` (public API)
- `modules/tuning/note-expression/module.cmake` (JUCE-NE-PATCH check + .doricolib install)
- `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h` (public API)
- `modules/cmake/OuariconModules.cmake` (`ouaricon_add_module` impl)
- `plugins/O-Lyrica/{CMakeLists.txt, Source/PluginProcessor.{h,cpp}, Source/HarpSynthVoice.{h,cpp}}` (reference: NE wiring + voice tuning hookup)
- `plugins/O-Wind/{CMakeLists.txt, Source/PluginProcessor.{h,cpp}}` (reference: scala-tuning-engine direct-source wiring + WebView CMake recipe)

---

## Investigating

- **Confirm `Ouaricon::NoteExpression::VST3Extensions` constructor and call sites** (Stage 0 follow-up #4)
- **Confirm `TuningEngine` namespace and constructor** (Stage 0 follow-up #3)
- **Confirm canonical `juce_add_plugin` recipe** (CONTEXT.md says `NEEDS_WEB_BROWSER` only — verify against O-Wind / O-Lyrica)
- **Confirm canonical CMake `ouaricon_add_module` invocation** for note-expression
- **Reserve a unique 4-char `PLUGIN_CODE`** (O-Bass already owns `OBas`)
- **Identify any hidden gotchas** that `foundation-shell-agent` would otherwise hit

---

## Research Findings

### 1. Note-Expression Module — Public API (CONFIRMED + CORRECTION)

**Source:** `modules/tuning/note-expression/cpp/NoteExpression.h`

- **Namespace:** `Ouaricon::NoteExpression` ✅ (matches ARCHITECTURE.md)
- **Type to expose as a long-lived `PluginProcessor` member:** `Ouaricon::NoteExpression::VST3Extensions`
- **Constructor:** default — `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;` (no args, internally reserves `blockEvents` to 64 slots)
- **`getVST3ClientExtensions()` override:** returns `&vst3Extensions` (raw pointer to the long-lived member). Inheriting from `juce::AudioProcessor` already exposes the virtual; just override.
- **Per-block drain call:** `vst3Extensions.drainAndUpdate();` — single call, drains the queue *and* dispatches `updatePendingFromEvents` via the registered slot. **MUST run at the top of `processBlock` BEFORE `synthesiser.renderNextBlock(...)`** (so per-voice `startNote` sees pending NE deltas).
- **Voice wiring:** `voice->setPendingTuningSource(&vst3Extensions.getPendingTable());` once per voice, in `prepareToPlay` (or constructor). `getPendingTable()` returns `PendingTuningTable&` (an `std::array<std::atomic<double>, 128>&`).
- **Voice-side consumption (Phase 2.4 — NOT Stage 1):** voice calls `Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, currentFrequency)` inside `startNote()` *after* `tuningEngine->getFrequency(...)`. Helper does `exchange(0.0)` so retriggers don't inherit stale offsets. **Stage 1 voices ignore the source pointer entirely** — they just receive it for forward compatibility.

**CORRECTION vs. CONTEXT.md / ARCHITECTURE.md:** ARCHITECTURE.md §8 mentions `target_link_libraries(O-Bassoon PRIVATE Ouaricon::note_expression)` as an alternative wiring. That target does **not** exist — there is no exported `Ouaricon::note_expression` CMake target. The canonical (and only) wiring is `ouaricon_add_module(O-Bassoon note-expression)`, which:
1. Globs `modules/tuning/note-expression/cpp/*.{cpp,h}` into `O-Bassoon`'s SharedCode TU
2. Globs `modules/tuning/note-expression/cpp/vst3/*` into the `O-Bassoon_VST3` per-format target
3. Adds `modules/tuning/note-expression/cpp` as a private include directory (so `#include "NoteExpression.h"` resolves directly — no path prefix)
4. Includes `modules/tuning/note-expression/module.cmake`, which performs the JUCE-NE-PATCH marker check at *configure* time and installs the Microtonal-Suite `.doricolib` to `~/Library/Application Support/Ouaricon/Microtonal Suite/` per consumer

CONTEXT.md is correct — `ouaricon_add_module` is canonical. ARCHITECTURE.md's mention of a `target_link_libraries(... PRIVATE Ouaricon::note_expression)` line should be ignored at Stage 1.

---

### 2. TuningEngine — Public API (CONFIRMED + IMPORTANT CORRECTION)

**Source:** `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h`

- **Namespace:** **GLOBAL** — `class TuningEngine` (NOT `Ouaricon::TuningEngine` as ARCHITECTURE.md / CONTEXT.md say). This is the most important correction in this research pass.
- **Constructor:** default — `TuningEngine tuningEngine;` (no args). Default state: `Mode::TwelveTET`, A4 = 440.0, 12-TET frequency table built immediately. **Functionally identical to `juce::MidiMessage::getMidiNoteInHertz()` at v1.0** (matches D6 in CONTEXT.md).
- **Non-copyable:** `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningEngine)` is set. Pass it to voices as a raw pointer.
- **Voice-side use (Phase 2.1+, NOT Stage 1):** `currentFrequency = tuningEngine->getFrequency(midiNoteNumber);` (signature: `double getFrequency(int midiNote, int midiChannel = 0)` — channel reserved for future use, pass nothing).
- **Voice wiring API:** Pattern verified in O-Lyrica `HarpSynthVoice` (`HarpSynthVoice.h:66 — void setTuningEngine(TuningEngine* engine)`). For O-Bassoon Stage 1, the voice gets a `setTuningEngine(TuningEngine*)` setter and stores a raw pointer; the Stage 1 stub never dereferences it (Phase 2.1 is first use).
- **Thread safety:** `getFrequency` reads from `std::array<std::atomic<double>, 128> frequencyTable;` — lock-free on the audio thread. Mutators (`setMode`, `setMasterTune`, etc.) are message-thread; APVTS-driven mutator calls live in `processBlock` per O-Lyrica precedent.

**CORRECTION vs. ARCHITECTURE.md:** ARCHITECTURE.md repeatedly says `Ouaricon::TuningEngine`. The actual class is in the **global namespace**. All header-include and member-declaration lines must be:
```cpp
#include "TuningEngine.h"
// ...
TuningEngine tuningEngine;            // PluginProcessor member
TuningEngine* getTuningEngine() { return &tuningEngine; }
```

Verified: O-Lyrica `PluginProcessor.h:206` declares `TuningEngine tuningEngine;`. O-Wind `PluginProcessor.h:77` likewise. No `Ouaricon::` prefix anywhere in either consumer.

---

### 3. CMake Recipe — `juce_add_plugin` Flags (CONFIRMED + ONE GAP)

**Reference:** `plugins/O-Wind/CMakeLists.txt` and `plugins/O-Lyrica/CMakeLists.txt` and `plugins/O-AnalogEQ/CMakeLists.txt` — all three carry the same WebView-Windows pattern.

**Canonical Stage 1 `juce_add_plugin` flag set for a synth with WebView:**
```cmake
juce_add_plugin(O-Bassoon
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OBsn                         # see Finding 5
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Bassoon${OUARICON_DEV_SUFFIX}"
    PLUGIN_VERSION "1.0.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE                       # ← MISSING from CONTEXT.md flag list
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

**GAP vs. CONTEXT.md:** Stage 1 CONTEXT.md "Windows WebView CMake flags" section lists only `NEEDS_WEB_BROWSER TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. The third flag — `NEEDS_WEBVIEW2 TRUE` inside `juce_add_plugin(...)` — is **also required** and is what links `WebView2LoaderStatic.lib` into the plugin binary. Project-memory entry `WebView2 on Windows: Static vs Dynamic Linking` says exactly this. All three reference plugins (O-Wind:18, O-Lyrica:16, O-AnalogEQ:14) carry `NEEDS_WEBVIEW2 TRUE`. Plan phase should add this to the flag list explicitly.

**Compile-definition block (separate from `juce_add_plugin`):**
```cmake
target_compile_definitions(O-Bassoon
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```
This block lives at the bottom of `CMakeLists.txt`, after `juce_generate_juce_header` and after binary-data linking (matches O-Wind:108-114 and O-Lyrica:111-117).

**Required-target-link-libraries** (12 JUCE modules + 3 PUBLIC config groups). Identical list to O-Lyrica. Notable inclusions: `juce::juce_dsp` (we use `juce::dsp::IIR::Filter` for the noise lowpass at Phase 2.3), `juce::juce_gui_extra` (WebView), `juce::juce_audio_utils` (for `GenericAudioProcessorEditor`).

**`juce_generate_juce_header(O-Bassoon)`** — *MUST* be called after `target_link_libraries(...)`. JUCE 8 requirement, captured in `juce8-critical-patterns.md` #22 and reinforced in CONTEXT.md.

---

### 4. CMake Wiring — `scala-tuning-engine` Direct File References (CONFIRMED)

`scala-tuning-engine` is **not** registered as an `ouaricon_add_module`-style module that auto-globs sources. Despite having a `module.yaml` and `module.cmake` in `modules/tuning/scala-tuning-engine/`, those are for the install pipeline (`install-microtonal-suite.cmake.in`) — they don't drive consumer wiring. Consumers add the four cpp files by direct reference (matches O-Wind:45-48):

```cmake
target_sources(O-Bassoon
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/BassoonSound.h
        Source/BassoonVoice.h
        Source/BassoonVoice.cpp
        # Tuning module files (referenced from shared module — verbatim from O-Wind:45)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

target_include_directories(O-Bassoon
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)
```

**Why all four files at Stage 1:** `TuningEngine.cpp` uses functions from `ScaleGenerator.cpp` (e.g. for built-in temperaments) and `EmbeddedTunings.cpp` (factory tuning library lookup). Even with O-Bassoon never touching anything beyond default 12-TET at v1.0, the unresolved-symbol link errors will fire if any are omitted. `TuningExporter.cpp` is a public-API method on the engine and must compile in (matches O-Wind, which keeps all four).

**Note on dual-include surface:** `O-Lyrica` keeps a *local* copy of `TuningEngine.cpp` (and the other three) under `Source/DSP/` rather than the shared module. This is a known divergence in O-Lyrica that pre-dates the shared-module promotion. **Use O-Wind's pattern**, not O-Lyrica's, for O-Bassoon — keep all four files referenced from `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/` so v1.1 microtonal additions inherit upstream fixes automatically.

---

### 5. PLUGIN_CODE — `OBsn` (RESERVED HERE)

**Conflict check:** `grep -n "PLUGIN_CODE" plugins/*/CMakeLists.txt` shows `OBas` is already taken by **O-Bass**. `OBsn` is unique across the existing plugin suite (also unique against `OBls` / `OBwd` / `OCbs` / `OuFm` / `OuIP` and the rest). It also reads cleanly: **OB**asso**n**. Plan phase should commit `PLUGIN_CODE OBsn` in the CMakeLists.txt without further deliberation.

---

### 6. PluginProcessor Member Layout (CONFIRMED PATTERN)

The pattern below is a literal merge of O-Wind (TuningEngine member) and O-Lyrica (NE extensions member). Stage 1 implements this exactly:

```cpp
// PluginProcessor.h — member section
juce::AudioProcessorValueTreeState parameters;     // APVTS — see createParameterLayout()
juce::Synthesiser                  synthesiser;    // 16 voices pre-allocated in prepareToPlay
TuningEngine                       tuningEngine;   // global namespace, default 12-TET A4=440
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;  // long-lived, default-constructed

juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
```

**Member-construction order in `PluginProcessor::PluginProcessor()`** (mirrors O-Wind/O-Lyrica precedent):
```cpp
OBassoonAudioProcessor::OBassoonAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new BassoonVoice();
        voice->setAPVTS(&parameters);
        voice->setTuningEngine(&tuningEngine);
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable());
        synthesiser.addVoice(voice);
    }
    synthesiser.addSound(new BassoonSound());
}
```

**Per-block drain** at the top of `processBlock`:
```cpp
juce::ScopedNoDenormals noDenormals;
buffer.clear();

vst3Extensions.drainAndUpdate();   // single line — module handles correlation + table writes

// (Stage 2.3+ will read APVTS smoothed values here)

synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
```

---

### 7. APVTS Layout — 10 Parameters (FROZEN IN CONTEXT.md)

Implementation pattern matches O-Wind `createParameterLayout()` style: each parameter as `std::make_unique<juce::AudioParameterFloat>(...)` (or `AudioParameterInt` for `voice_count`), all added to `juce::AudioProcessorValueTreeState::ParameterLayout` and returned. IDs/ranges/defaults are frozen exactly as in `parameter-spec-draft.md`:

| ID | Type | Range | Default | Notes |
|---|---|---|---|---|
| `vibrato_rate` | `AudioParameterFloat` | 0.0–10.0 Hz | 5.0 | NormalisableRange step 0.01, skew 1.0 |
| `vibrato_depth` | `AudioParameterFloat` | 0–100 cents | 15 | step 0.1 |
| `vibrato_onset` | `AudioParameterFloat` | 0–2000 ms | 400 | step 1.0 |
| `breath` | `AudioParameterFloat` | 0.0–1.0 | 0.7 | step 0.001 |
| `tone` | `AudioParameterFloat` | 0.0–1.0 | 0.5 | step 0.001 |
| `attack_character` | `AudioParameterFloat` | 0.0–1.0 | 0.0 | step 0.001 |
| `attack_time` | `AudioParameterFloat` | 0–2000 ms | 300 | step 1.0 |
| `release_time` | `AudioParameterFloat` | 0–3000 ms | 800 | step 1.0 |
| `voice_count` | `AudioParameterInt` | 1–16 | 8 | int param |
| `output_gain` | `AudioParameterFloat` | -24.0 to +6.0 dB | 0.0 | step 0.1, suffix " dB" via attribute |

All parameters appear in the host's flat parameter list at Stage 1. The `GenericAudioProcessorEditor` placeholder will auto-render them.

**Stage 1 reads:** none. The placeholder editor displays parameters; voices ignore them. Stage 2.1 wires the first APVTS reads (hardcoded ADSR placeholder in 2.1, real connection in 2.3 per ROADMAP.md).

---

### 8. BassoonSound — Trivial (CONFIRMED PATTERN)

Single shared instance, header-only:
```cpp
// BassoonSound.h
#pragma once
#include <JuceHeader.h>

class BassoonSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote   (int)  override { return true; }
    bool appliesToChannel(int)  override { return true; }
};
```
Matches O-Lyrica `HarpSynthSound.h` and O-Wind `FluteSynthSound.h` byte-for-byte. Synth owns it via `synthesiser.addSound(new BassoonSound())`.

---

### 9. BassoonVoice — Silent Stub Surface (CONFIRMED)

Stage 1 builds the class with full method signatures but a no-op `renderNextBlock`. Voices receive setters in the processor constructor (see Finding 6) and store raw pointers; nothing is dereferenced until Phase 2.1.

```cpp
// BassoonVoice.h (Stage 1 surface — no DSP yet)
#pragma once
#include <JuceHeader.h>
#include "TuningEngine.h"
#include "NoteExpression.h"   // resolved via ouaricon_add_module include path

class BassoonVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<BassoonSound*>(s) != nullptr;
    }
    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override {}
    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        clearCurrentNote();   // immediate clear at Stage 1; Stage 2.1 wires ADSR release
    }
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int /*startSample*/, int /*numSamples*/) override {}

    // Wiring setters (called once per voice from PluginProcessor ctor)
    void setAPVTS              (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine       (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }

private:
    juce::AudioProcessorValueTreeState* parameters = nullptr;
    TuningEngine*                       tuningEngine = nullptr;
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
};
```

This surface is byte-aligned with what Phase 2.1 / 2.4 will need — no method-signature churn between Stage 1 and Phase 2.4 NE integration.

---

### 10. PluginEditor — `GenericAudioProcessorEditor` Placeholder (CONFIRMED)

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
        setSize(500, 480);   // ~10 params * ~40px row
    }
};
```
Replaced wholesale at Stage 3 (WebView). For Stage 1, `GenericAudioProcessorEditor` auto-renders all 10 APVTS parameters — sufficient for manual smoke-test (knob-twiddle in DAW, verify automation).

---

## Discrepancies Surfaced (For Plan Phase to Reconcile)

| # | Source claim | Actual | Reconciliation |
|---|---|---|---|
| **D1** | ARCHITECTURE.md §8: `target_link_libraries(... PRIVATE Ouaricon::note_expression)` | No such CMake target exists. Module is wired via `ouaricon_add_module(O-Bassoon note-expression)` (which globs `cpp/*.cpp`, adds include dirs, runs JUCE-NE-PATCH check, installs `.doricolib`). | **Plan phase: use `ouaricon_add_module(...)` only.** Drop `target_link_libraries` mention from any plan-phase quote of ARCHITECTURE.md §8. |
| **D2** | ARCHITECTURE.md §9 + CONTEXT.md "Approach Decisions" row 5: `Ouaricon::TuningEngine` | Class is in **global namespace**: `class TuningEngine`. | **Plan phase: declare `TuningEngine tuningEngine;` and pass `TuningEngine*` to voices.** No namespace prefix anywhere. |
| **D3** | CONTEXT.md "Constraints / Approach Decisions" lists only `NEEDS_WEB_BROWSER TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. | All three reference plugins (O-Wind, O-Lyrica, O-AnalogEQ) also carry `NEEDS_WEBVIEW2 TRUE` inside `juce_add_plugin(...)`. Project memory (`WebView2 on Windows: Static vs Dynamic Linking`) says this is required. | **Plan phase: add `NEEDS_WEBVIEW2 TRUE` to the `juce_add_plugin` flag list** alongside `NEEDS_WEB_BROWSER TRUE`. |

None of these change the plan's substance — they're all surface-level corrections that the plan phase will absorb without further research.

---

## Module Reuse Confirmation

| Module | Version | Wiring | Purpose at Stage 1 |
|---|---|---|---|
| `note-expression` | v1.1.0 (header says v1.0.0 but module.yaml is v1.1.0 per CONTEXT.md — header version string is stale; CMake-time JUCE-NE-PATCH check is what matters) | `ouaricon_add_module(O-Bassoon note-expression)` | Type member `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;`, `getVST3ClientExtensions()` override, `vst3Extensions.drainAndUpdate()` at top of `processBlock`. Voice receives a `PendingTuningTable*` but never reads it. |
| `scala-tuning-engine` | v2.0.0 (per `TuningEngine.h` doc-block; `module.yaml` says v2.1.0 — likely additive bug-fix bump, no API change) | Direct `cpp/{TuningEngine,ScaleGenerator,EmbeddedTunings,TuningExporter}.cpp` references + `target_include_directories` adds `cpp/` | Type member `TuningEngine tuningEngine;` (global ns), default-constructed. Voice receives a `TuningEngine*` but never reads it. |

No new modules required for Stage 1.

---

## Pitfalls / Landmines

(Drawn from project memory + `juce8-critical-patterns.md` + `spike-findings-VST-development`.)

1. **Output-only `BusesProperties`** for synths. `BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)` only — *never* `.withInput(...)` on a synth. Source: `juce8-critical-patterns.md` #4.
2. **`juce_generate_juce_header(O-Bassoon)` after `target_link_libraries`**, not before. Source: `juce8-critical-patterns.md` #22.
3. **`getLatencySamples()` is non-virtual in JUCE 8** — do NOT override. Modal synthesis is feed-forward; latency=0 is the default already. Source: project memory.
4. **Drain BEFORE `renderNextBlock`** — `vst3Extensions.drainAndUpdate()` must populate the `PendingTuningTable` before any voice's `startNote` runs, otherwise NE deltas arrive a block late. Source: `spike-findings-VST-development` Pattern 4.
5. **JUCE-NE-PATCH absence at configure time** — module's `module.cmake` does a `file(READ)` + `string(FIND)` for the `JUCE-NE-PATCH` marker in two specific JUCE source files. If the local JUCE fork hasn't been re-patched after a JUCE upgrade, configure fails with `[note-expression] JUCE patch marker 'JUCE-NE-PATCH' not found`. Fix: `./scripts/apply-juce-patches.sh`. Source: `modules/tuning/note-expression/module.cmake:24-37`.
6. **`PLUGIN_CODE` collision with O-Bass.** `OBas` is taken; using `OBsn`. Failing to use a unique 4-char code makes both plugins overwrite each other in the AU registry on macOS. Source: `juce8-critical-patterns.md` (general AU registration rule) + grep evidence above.
7. **Don't sweep `cpp/vst3/` into SharedCode.** `ouaricon_add_module` already routes per-format files into `O-Bassoon_VST3` only — the foundation-shell-agent must NOT add wildcard `target_sources` lines that would re-include them in SharedCode (would break AU/Standalone link lines). Source: `OuariconModules.cmake:52-56` comment.
8. **Resource provider not used at Stage 1** — Stage 1 ships no WebView UI. The CMake flags (`NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) are committed for Stage 3, but no `juce_add_binary_data(...)` or resource-provider C++ at Stage 1. Source: CONTEXT.md "Approach Decisions" row 6.
9. **Voice-count cap is *not* enforced at Stage 1.** ARCHITECTURE.md §7 + ROADMAP Phase 2.4 cover the `findFreeVoice` override. Stage 1 just pre-allocates 16 voices and lets default `juce::Synthesiser` behavior steal the oldest. Source: CONTEXT.md "BassoonVoice ships as a silent stub".
10. **`NEEDS_WEBVIEW2 TRUE` triggers a Windows-only static lib link**, but the *actual* `WebView2LoaderStatic.lib` is shipped with JUCE 8 — no extra SDK install required on the user's Windows VM. Source: project memory.

---

## Ouaricon Family Conventions (Verified)

These are not Stage 1-specific but the foundation-shell-agent should follow them by reflex:

- **`COMPANY_NAME "${OUARICON_COMPANY_NAME}"`** — variable defined at the root `CMakeLists.txt`. Same for `OUARICON_MANUFACTURER_CODE` and `OUARICON_DEV_SUFFIX`.
- **`PRODUCT_NAME "O-Bassoon${OUARICON_DEV_SUFFIX}"`** — suffix is `-DEV` for non-release builds (separates dev plugin from installed version in the DAW scanner).
- **`PLUGIN_VERSION "1.0.0"`** at Stage 1 — bump only on user-visible release.
- **`FORMATS VST3 AU Standalone`** — no AAX, no LV2 in the Ouaricon family. macOS gets all three; Windows builds VST3 only (Standalone usually excluded; matches O-Wind line 6).
- **Licensing block** at the bottom of CMakeLists.txt (compile-flag-gated `OUARICON_LICENSING`) — exact copy from O-Wind/O-Lyrica:
  ```cmake
  if(OUARICON_LICENSING)
      ouaricon_add_module(O-Bassoon licensing)
      target_compile_definitions(O-Bassoon PRIVATE OUARICON_LICENSING_ENABLED=1)
      target_link_libraries(O-Bassoon PRIVATE juce::juce_cryptography)
  endif()
  ```
  (For Stage 1, this is dead code unless `OUARICON_LICENSING` is set at configure time. Plan phase decides whether to ship it now or in Stage 4.)

---

## Files Stage 1 Will Create (Unchanged from CONTEXT.md)

- `plugins/O-Bassoon/CMakeLists.txt`
- `plugins/O-Bassoon/Source/PluginProcessor.h` + `.cpp`
- `plugins/O-Bassoon/Source/PluginEditor.h` + `.cpp`  *(Generic placeholder — replaced wholesale at Stage 3)*
- `plugins/O-Bassoon/Source/BassoonVoice.h` + `.cpp`  *(silent stub)*
- `plugins/O-Bassoon/Source/BassoonSound.h`

## Files Stage 1 Will Reference (Read-Only)

- `modules/tuning/note-expression/cpp/NoteExpression.h` (via `ouaricon_add_module` include path)
- `modules/tuning/scala-tuning-engine/cpp/{TuningEngine,ScaleGenerator,EmbeddedTunings,TuningExporter}.cpp` and `.h` (direct file refs in `target_sources` + include dir)
- Templates: `plugins/O-Wind/CMakeLists.txt` (CMake recipe), `plugins/O-Lyrica/Source/PluginProcessor.{h,cpp}` (NE wiring + voice tuning hookup)

---

## Open Questions

**None blocking the plan phase.** All Stage-0 follow-ups that CONTEXT.md flagged are resolved here:

- ✅ TuningEngine API path: `modules/tuning/scala-tuning-engine/cpp/TuningEngine.{h,cpp}` (header includes via `target_include_directories(... PRIVATE ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp)`).
- ✅ TuningEngine namespace: **GLOBAL** — corrects ARCHITECTURE.md / CONTEXT.md.
- ✅ Module CMake function: `ouaricon_add_module(O-Bassoon note-expression)` for NE; direct `target_sources` for scala-tuning-engine.
- ✅ NoteExpression API: `Ouaricon::NoteExpression::VST3Extensions` member, `drainAndUpdate()` per block, `getPendingTable()` returns `PendingTuningTable&`.
- ✅ JUCE 8 plugin recipe: `juce_add_plugin(... IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_WEB_BROWSER TRUE NEEDS_WEBVIEW2 TRUE ...)` — adds `NEEDS_WEBVIEW2 TRUE` vs. CONTEXT.md.
- ✅ PLUGIN_CODE: `OBsn` (verified unique against existing 35-plugin suite).

**Deferred** (still tracked, still not blocking):
- Reference bassoon C3 recording → Phase 2.2 kickoff.
- UI mockup → Stage 3 prerequisite.

---

## Next Phase

Ready for **plan** phase: `/plugin-plan O-Bassoon 1-foundation`

Plan-phase scope: produce a single-pass execution plan (one wave) that spawns `foundation-shell-agent` with this RESEARCH.md + CONTEXT.md attached. The plan should explicitly absorb the three discrepancies above (D1/D2/D3) and reserve `PLUGIN_CODE OBsn`. No additional research needed.
