# Phase 24: Integration Matrix

**Created:** 2026-04-25
**Source:** `24-PATTERNS.md` (per-plugin file:line analogs); `24-CONTEXT.md` D-05.
**Purpose:** Single structural source of truth across all 7 per-plugin plans (24-01..07). Each plan references its row by anchor link. Plans MUST NOT redo discovery — copy values from this table.

**Reference shape (Phase 23 textbook):**
- `plugins/O-Lyrica/CMakeLists.txt:80` — `ouaricon_add_module(OLyrica note-expression)`
- `plugins/O-Lyrica/Source/PluginProcessor.h:22, 119, 200` — include + `getVST3ClientExtensions()` override + `vst3Extensions` member
- `plugins/O-Lyrica/Source/PluginProcessor.cpp:506, 708` — voice wiring + `drainAndUpdate()`
- `plugins/O-Lyrica/Source/HarpSynthVoice.h:21, 88, 129` — include + setter + member
- `plugins/O-Lyrica/Source/HarpSynthVoice.cpp:84-87, 138-147` — setter body + `applyPendingTuning` call

---

## Cross-Reference Table

| Plugin | Voice file | Base-freq source class | Composition note | Voice-trigger entry point | PluginProcessor swap site | CHANGELOG file | STATUS file | Notes |
|--------|-----------|------------------------|------------------|---------------------------|---------------------------|----------------|-------------|-------|
| <a id="o-bells"></a>**O-Bells** | `plugins/O-Bells/Source/BellVoice.{h,cpp}` | `TuningEngine` (`Source/TuningEngine.h`) — `tuningEngine->getFrequency(midi)` | TuningEngine compose (matches O-Lyrica exactly) | `BellVoice::startNote(int, float, ...)` at `BellVoice.cpp:100` — base-freq line 161-163; insert NE at line 166 BEFORE `calculateMultiStageCoefficients()` | `Source/PluginProcessor.h` line 92 (after `juce::Synthesiser synthesiser;`); `Source/PluginProcessor.cpp` addVoice loop lines 545-550, drainAndUpdate at line 739 (top of processBlock after `buffer.clear()`) | `plugins/O-Bells/CHANGELOG.md` (top entry currently `[4.0.0]`) | `plugins/O-Bells/.planning/STATUS.md` (YAML front-matter) | **Float→double cast at helper boundary** — `BellVoice` uses `float fundamental`. CMake already includes `OuariconModules.cmake` at line 94. Version bump: 4.0.0 → 4.1.0. |
| <a id="o-prism"></a>**O-Prism** | `plugins/O-Prism/Source/PrismVoice.{h,cpp}` | `TuningEngine` — `tuningEngine->getFrequency(midi)` | TuningEngine compose (matches O-Lyrica exactly) | `PrismVoice::startNote()` at `PrismVoice.cpp:157` — base-freq line 181-185; insert NE BEFORE `glide.setTarget(currentFrequency)` at line 196 and BEFORE per-oscillator `setFrequency` calls (lines 202, 219, 238) | `Source/PluginProcessor.h` line 153; `Source/PluginProcessor.cpp` addVoice loop lines 472-478, drainAndUpdate at line 559 | `plugins/O-Prism/CHANGELOG.md` (top entry `## v1.16.0`; CMake line 12 says `VERSION 1.16.1`) | `plugins/O-Prism/.planning/STATUS.md` | Already calls `ouaricon_add_module(O-Prism webview-relay-manager)` at line 81 — proves macro works in this build. Version bump: 1.16.1 → 1.17.0. CHANGELOG style uses `## v<ver> (date)` (no brackets). |
| <a id="o-wind"></a>**O-Wind** | `plugins/O-Wind/Source/FluteSynthVoice.{h,cpp}` | `TuningEngine` — `tuningEngine->getFrequency(midi)` | TuningEngine compose **+ physical-model period** (BoreWaveguide) | `FluteSynthVoice::startNote()` at `FluteSynthVoice.cpp:68` — base-freq line 78-81; insert NE immediately after line 81, BEFORE pitch-bend application at line 84 (also BEFORE `boreWaveguide.setBoreDelay(...)` at line 100) | `Source/PluginProcessor.h` line 67 (after `juce::Synthesiser synthesiser;`); `Source/PluginProcessor.cpp` addVoice loop lines 487-489, drainAndUpdate at line 532 | `plugins/O-Wind/CHANGELOG.md` (top entry `[1.15.1]`) | `plugins/O-Wind/.planning/STATUS.md` | **Missing PLUGIN_VERSION line — add explicitly during version bump**: `juce_add_plugin(O-Wind ...)` at line 6 has no `PLUGIN_VERSION` arg. Add `PLUGIN_VERSION "1.16.0"` between `PRODUCT_NAME` (line 11) and `IS_SYNTH` (line 12). **Float→double cast at helper boundary**: voice uses `float currentFrequency`. CMake already includes `OuariconModules.cmake` at line 3. |
| <a id="o-intonationpad"></a>**O-IntonationPad** | `plugins/O-IntonationPad/Source/DSP/WavetableVoice.{h,cpp}` | `TuningEngine` — via `resolveFrequency(midi, cents)` → `tuningEnginePtr->getFrequency(midi)` | **Multi-sub-voice — derive neRatio**: voice spawns 12 sub-voices via `chordGeneratorPtr->generateChord`. Apply NE delta as multiplicative ratio derived from `applyPendingTuning(table, midiNoteNumber, 1.0)` at top of `startNote` (line 102, BEFORE `chordGeneratorPtr` block at line 109). Propagate `neRatio` into each `resolveFrequency` call at lines 134, 142, 150 (multiply result). | `WavetableVoice::startNote(int, float, ...)` at `WavetableVoice.cpp:100` | `Source/PluginProcessor.h` line 104 (after `juce::Synthesiser synthesiser;`); `Source/PluginProcessor.cpp` addVoice loop lines 365-368 (wire `setPendingTuningSource` at construction — voice holds pointer for lifetime since `vst3Extensions` outlives all voices), drainAndUpdate at line 511 (after `buffer.clear()` at line 516) | `plugins/O-IntonationPad/CHANGELOG.md` (top entry `[2.7.2]`) | `plugins/O-IntonationPad/.planning/STATUS.md` | `setPendingTuningSource` wires ONCE at construction (NOT per-block) — match O-Lyrica static wiring; constructor doesn't pass `tuningEngine` to voices (assigned per-block via `setChordGenerationParams`), but `vst3Extensions` reference is stable for life of processor. CMake already includes `OuariconModules.cmake` at line 2. Version bump: 2.7.2 → 2.8.0. |
| <a id="o-reed"></a>**O-Reed** | `plugins/O-Reed/Source/ReedWindVoice.{h,cpp}` | `TuningEngine` via helper `getBaseFrequencyFromTuning(int midiNote)` (lines 121-126) | **MPE — apply NE inside `getBaseFrequencyFromTuning` helper** (single source of truth for `noteStarted()` legato site at line 141 + normal site at line 202 + `notePitchbendChanged` at line 374). `exchange(0.0)` consume semantics still correct: first call consumes, subsequent calls return base unchanged. | `ReedWindVoice::noteStarted()` at `ReedWindVoice.cpp:133` (NO MIDI arg — reads from `getCurrentlyPlayingNote().initialNote`) | `Source/PluginProcessor.h` after line 63 `juce::MPESynthesiser synthesiser;`; `Source/PluginProcessor.cpp` addVoice loop lines 341-344, drainAndUpdate at line 378 | `plugins/O-Reed/CHANGELOG.md` (top entry `## v1.0.11`; uses `## vX.Y.Z (date)` style) | `plugins/O-Reed/.planning/STATUS.md` | **Missing PLUGIN_VERSION line — add explicitly during version bump**: add `PLUGIN_VERSION "1.1.0"` between `PRODUCT_NAME` (line 11) and `IS_SYNTH`. MPE base class: `juce::MPESynthesiserVoice`. CMake already includes `OuariconModules.cmake` at line 3. Version bump: 1.0.11 → 1.1.0. |
| <a id="o-bowed"></a>**O-Bowed** | `plugins/O-Bowed/Source/BowedStringVoice.{h,cpp}` | `TuningEngine` via helper `getBaseFrequencyFromTuning(int midiNote)` (lines 291-296) | **MPE — apply NE inside `getBaseFrequencyFromTuning` helper** (covers BOTH `noteStarted()` line 32 AND `notePitchbendChanged()` line 71 → `waveguideString.trigger(currentFrequency)` at lines 39, 76). | `BowedStringVoice::noteStarted()` at `BowedStringVoice.cpp:25` (MPE — reads `getCurrentlyPlayingNote().initialNote`) | `Source/PluginProcessor.h` line 77 (`BowedMPESynthesiser synthesiser;`); `Source/PluginProcessor.cpp` addVoice loop lines 246-250, drainAndUpdate at line 288 | `plugins/O-Bowed/CHANGELOG.md` (top entry `[1.2.1]`) | `plugins/O-Bowed/.planning/STATUS.md` | **Missing PLUGIN_VERSION line — add explicitly during version bump**: add `PLUGIN_VERSION "1.3.0"` between `PRODUCT_NAME` and `IS_SYNTH`. MPE: `juce::MPESynthesiserVoice` via `BowedMPESynthesiser`. CMake already includes `OuariconModules.cmake` at line 3. Version bump: 1.2.1 → 1.3.0. |
| <a id="o-formant"></a>**O-Formant** | `plugins/O-Formant/Source/FormantVoice.{h,cpp}` | `TuningEngine` — `tuningEnginePtr->getFrequency(midi)` cached as `tunedF0` | **MPE — pitched fundamental drives `LFGlottalSource`**. Insert NE immediately after the `tunedF0` assignment at lines 187-191, BEFORE `pitchGlide.setTarget(f0)` (line 196) / `pitchGlide.snapTo(f0)` (line 198). Re-read `f0 = tunedF0` after NE composition. | `FormantVoice::noteStarted()` at `FormantVoice.cpp:132` (MPE — reads `currentlyPlayingNote.initialNote`) | `Source/PluginProcessor.h` line 81 (`juce::MPESynthesiser synthesiser;`); `Source/PluginProcessor.cpp` addVoice loop lines 694-699, drainAndUpdate at line 742 | `plugins/O-Formant/CHANGELOG.md` (top entry `[1.24.2]`) | `plugins/O-Formant/.planning/STATUS.md` | **CMake delta: missing `OuariconModules.cmake` include — pre-step required**. Add `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` immediately after `cmake_minimum_required` (line 1) and BEFORE `juce_add_plugin` (line 4). Then add `ouaricon_add_module(O-Formant note-expression)` after `target_sources` block (line 34). **Float→double cast at helper boundary**: voice uses `float tunedF0`. Version bump: 1.24.2 → 1.25.0 (line 10 `VERSION 1.24.2`). |

---

## Sequencing (D-11 — Easy-First, locked by CONTEXT.md)

The execution order is **strictly serial** (D-13). Each plan is one wave.

| Wave | Plan | Plugin | Match quality | Why this slot |
|------|------|--------|---------------|---------------|
| 1 | 24-01 | O-Bells | EXACT (TuningEngine + classic Synthesiser) | Cleanest canary — minimum delta from O-Lyrica reference shape. |
| 2 | 24-02 | O-Prism | EXACT | Already calls `ouaricon_add_module` — proves the macro works end-to-end. |
| 3 | 24-03 | O-Wind | EXACT (+ physical-model period, missing PLUGIN_VERSION line) | Adds physical-model validation (BoreWaveguide period derivation). |
| 4 | 24-04 | O-IntonationPad | STRUCTURAL (multi-sub-voice) | Proves `neRatio` propagation pattern before MPE plugins. |
| 5 | 24-05 | O-Reed | STRUCTURAL (MPE + missing PLUGIN_VERSION line) | First MPE plugin — validates `getBaseFrequencyFromTuning` helper composition. |
| 6 | 24-06 | O-Bowed | STRUCTURAL (MPE + missing PLUGIN_VERSION line) | Second MPE plugin (same shape as O-Reed). |
| 7 | 24-07 | O-Formant | STRUCTURAL (MPE + missing `OuariconModules.cmake` include) | Last because it requires adding the module-include line — extra structural delta. |
| 8 | 24-08 | (final sweep) | n/a | All 8 plugins (7 above + O-Lyrica) freshly rebuilt + reinstalled; registry audit; aggregate Dorico smoke results. |

---

## Cross-Cutting Patterns (apply to every per-plugin plan)

### Pattern A — CMakeLists.txt
- Pre-condition: `juce_add_plugin(<Plugin> ...)` runs BEFORE `ouaricon_add_module(<Plugin> note-expression)` (so per-format subtargets `<Plugin>_VST3`, `<Plugin>_AU`, `<Plugin>_Standalone` exist for the per-format routing loop).
- Pre-condition: `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` is present. **O-Formant lacks this — Plan 24-07 adds it as a pre-step.**
- One-liner: `ouaricon_add_module(<Plugin> note-expression)`.

### Pattern B — PluginProcessor swap (3 lines header + 2 lines cpp)
```cpp
// PluginProcessor.h:
#include "NoteExpression.h"
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
// private:
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

// PluginProcessor.cpp inside addVoice loop:
voice->setPendingTuningSource(&vst3Extensions.getPendingTable());
// PluginProcessor.cpp top of processBlock (after buffer.clear(), BEFORE renderNextBlock):
vst3Extensions.drainAndUpdate();
```

### Pattern C — Voice composition (3 lines header + 1 call site cpp)
```cpp
// Voice.h:
#include "NoteExpression.h"
void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source);
// private:
Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

// Voice.cpp inside startNote / noteStarted, AFTER base-frequency lookup, BEFORE DSP trigger:
if (pendingTuningSource != nullptr)
    currentFrequency = Ouaricon::NoteExpression::applyPendingTuning(
                           *pendingTuningSource, midiNoteNumber, currentFrequency);
```

For `float currentFrequency` plugins (O-Bells, O-Wind, O-Formant): cast through `double` at helper boundary:
```cpp
fundamental = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
    *pendingTuningSource, midiNoteNumber, static_cast<double>(fundamental)));
```

For MPE plugins (O-Reed, O-Bowed, O-Formant): apply NE INSIDE `getBaseFrequencyFromTuning(midiNote)` helper — single source of truth covers `noteStarted()` and `notePitchbendChanged()` call sites; `exchange(0.0)` semantics correct (first call consumes, second call returns base unchanged).

For O-IntonationPad: derive `double neRatio = applyPendingTuning(*table, midi, 1.0)` once at top of `startNote`, multiply into each sub-voice's `resolveFrequency` result.

### Pattern D — CHANGELOG entry (TRACK-03 exact phrasing)
First-line phrasing in entry MUST contain:
> **adds VST3 Note Expression microtonal support for Dorico**

Match each plugin's existing CHANGELOG style (bracketed `## [X.Y.Z] - YYYY-MM-DD` for O-Bells/O-Wind/O-Bowed/O-Formant/O-IntonationPad; `## vX.Y.Z (YYYY-MM-DD)` for O-Prism/O-Reed).

### Pattern E — STATUS.md update
Plugin-local STATUS.md is YAML front-matter — `/improve` workflow updates fields automatically. Plan does NOT manually rewrite the file shape.

### Pattern F — Module registry update
Each `/improve` cycle invokes `/module-add note-expression` for its plugin → appends to `modules/registry.yaml` `note-expression.used_by:` list as part of the same atomic plugin commit. By plan 24-07 close, the list contains all 8 (`OLyrica` + 7 propagation targets).

---

## Per-Plugin Acceptance Criteria Template (referenced by every per-plugin plan)

For each plugin `<Plugin>` with bumped version `<NewVersion>`:

- `plugins/<Plugin>/CMakeLists.txt` contains `ouaricon_add_module(<Plugin> note-expression)`
- `plugins/<Plugin>/CMakeLists.txt` `PLUGIN_VERSION` (or `VERSION` for O-Prism) is bumped to `<NewVersion>`
- `plugins/<Plugin>/Source/PluginProcessor.h` contains `Ouaricon::NoteExpression::VST3Extensions vst3Extensions`
- `plugins/<Plugin>/Source/PluginProcessor.cpp` contains `getVST3ClientExtensions` returning `&vst3Extensions` AND a `vst3Extensions.drainAndUpdate(` call from `processBlock`
- `plugins/<Plugin>/Source/<VoiceFile>.cpp` contains `Ouaricon::NoteExpression::applyPendingTuning(`
- `plugins/<Plugin>/CHANGELOG.md` top entry contains `adds VST3 Note Expression microtonal support for Dorico` (TRACK-03 exact phrase)
- `plugins/<Plugin>/.planning/STATUS.md` updated entry referencing the version bump (timestamped today)
- `modules/registry.yaml` `note-expression.used_by:` list contains `<Plugin>`
- `ninja <Plugin>_VST3 <Plugin>_AU <Plugin>_Standalone` exits 0 with no `Undefined symbols for architecture arm64` errors mentioning `Steinberg::*`
- `~/Library/Audio/Plug-Ins/VST3/<Plugin>.vst3` exists with mtime within last 5 minutes (post-CLAUDE.md fresh install)
- `~/Library/Audio/Plug-Ins/Components/<Plugin>.component` exists with mtime within last 5 minutes
- `scripts/verify-au-link.sh <Plugin>` exits 0 (auval validates AU loads)
- `auval -a | grep -i <pluginname>` returns at least one line
- Dorico quarter-sharp smoke (3-point gate per D-07): pitch lands +50¢ above C4, no attack zipper, NE correlated by `noteId` for chord polyphony — recorded PASS in plan SUMMARY.md

---

*Created during plan-phase. Single source of structural truth across plans 24-01 through 24-08.*
