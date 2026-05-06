# Phase 24: Propagate — Pattern Map

**Mapped:** 2026-04-25
**Files analyzed:** 7 plugins × 4 files each (CMakeLists + PluginProcessor.{h,cpp} + Voice.{h,cpp}) = 28 source files; plus 7 CHANGELOG.md + 7 STATUS.md
**Analogs found:** 7 / 7 (every plugin has a clean O-Lyrica analog; structural variations classified)

> **Reference shape (Phase 23 textbook):** `plugins/O-Lyrica/CMakeLists.txt:80`, `plugins/O-Lyrica/Source/PluginProcessor.h:22 + 119 + 200`, `plugins/O-Lyrica/Source/PluginProcessor.cpp:506 + 708`, `plugins/O-Lyrica/Source/HarpSynthVoice.h:21 + 88 + 129`, `plugins/O-Lyrica/Source/HarpSynthVoice.cpp:138-147`. Every per-plugin section below cross-references back to these line numbers.

---

## File Classification

| # | Plugin | Voice File | Voice Class | Synthesiser Variant | Voice Trigger | Base-Frequency Source | Composition Class | Match Quality |
|---|--------|------------|-------------|---------------------|---------------|------------------------|--------------------|--------------|
| 1 | **O-Bells** | `Source/BellVoice.{h,cpp}` | `BellVoice` | `juce::Synthesiser` | `startNote()` | `tuningEngine->getFrequency(midi)` (D-06 confirmed) | TuningEngine-composing | **EXACT** (= O-Lyrica shape) |
| 2 | **O-IntonationPad** | `Source/DSP/WavetableVoice.{h,cpp}` | `WavetableVoice` | `juce::Synthesiser` | `startNote()` (multi-sub-voice) | `resolveFrequency(midi, cents)` → `tuningEnginePtr->getFrequency(midi)` (D-06 confirmed) | TuningEngine-composing, **multi-sub-voice** | **STRUCTURAL VARIATION** (12 sub-voices per voice — see notes) |
| 3 | **O-Prism** | `Source/PrismVoice.{h,cpp}` | `PrismVoice` | `juce::Synthesiser` | `startNote()` | `tuningEngine->getFrequency(midi)` | TuningEngine-composing | **EXACT** |
| 4 | **O-Wind** | `Source/FluteSynthVoice.{h,cpp}` | `FluteSynthVoice` | `juce::Synthesiser` | `startNote()` | `tuningEngine->getFrequency(midi)` | TuningEngine-composing **+ physical-model period** (BoreWaveguide) | **EXACT** |
| 5 | **O-Reed** | `Source/ReedWindVoice.{h,cpp}` | `ReedWindVoice` | `juce::MPESynthesiser` | `noteStarted()` ⚠️ | `getBaseFrequencyFromTuning(note.initialNote)` | TuningEngine-composing **+ physical-model period** (BoreWaveguide) | **MPE** (different base class — see notes) |
| 6 | **O-Bowed** | `Source/BowedStringVoice.{h,cpp}` | `BowedStringVoice` | MPE via `BowedMPESynthesiser` | `noteStarted()` ⚠️ | `getBaseFrequencyFromTuning(midiNote)` | TuningEngine-composing **+ physical-model period** (WaveguideString) | **MPE** |
| 7 | **O-Formant** | `Source/FormantVoice.{h,cpp}` | `FormantVoice` | `juce::MPESynthesiser` | `noteStarted()` ⚠️ | `tuningEnginePtr->getFrequency(midi)` (cached as `tunedF0`) | TuningEngine-composing — fundamental drives `LFGlottalSource` | **MPE** |

**Variation summary:**
- **Standard `Synthesiser` + `startNote(int midi, float vel, ...)`:** O-Bells, O-IntonationPad, O-Prism, O-Wind. Map to O-Lyrica's `HarpSynthVoice::startNote` shape exactly.
- **`MPESynthesiserVoice` + `noteStarted()` (no params; reads `getCurrentlyPlayingNote().initialNote`):** O-Reed, O-Bowed, O-Formant. **MPE variation:** voice fetches its `midiNote` from `getCurrentlyPlayingNote().initialNote` instead of receiving it as a parameter. The `applyPendingTuning` call shape is identical; the MIDI-note source is the only difference. **No issue with NE correlation:** the module's `updatePendingFromEvents` correlates by `noteId` against the NoteOn that JUCE forwards regardless of MPE vs. classic mode (Pattern 1 in spike findings).
- **Multi-sub-voice (O-IntonationPad):** `WavetableVoice::startNote` calls `resolveFrequency(midi, centOffset)` per sub-voice (12 sub-voices). The `applyPendingTuning` call must wrap `resolveFrequency` so the NE delta applies to the **note-on MIDI pitch** only — sub-voice octave-shifts inherit the tuned root, but only the root-pitch slot in the `PendingTuningTable` is consumed. See O-IntonationPad notes for the recommended call placement.
- **Physical-model period derivation (O-Wind, O-Reed, O-Bowed):** `applyPendingTuning` modifies `currentFrequency` BEFORE the period assignment (`boreWaveguide.setBoreDelay(sr/freq)`, `waveguideString.trigger(freq)`, etc.). The composition order rule (D-10) holds — apply tuning first, then derive period. Pattern 2 (no attack zipper) is satisfied by `exchange(0.0)` consume in `applyPendingTuning`.

---

## Reference Shape (O-Lyrica) — copy these line shapes verbatim

### `plugins/O-Lyrica/CMakeLists.txt`

**Top of file (already present):**
```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
```

**Module consumption (line 80):**
```cmake
# Phase 23: VST3 Note Expression microtonal support (Dorico)
# Header-only module; its module.cmake auto-verifies the JUCE-NE-PATCH marker.
ouaricon_add_module(OLyrica note-expression)
```

### `plugins/O-Lyrica/Source/PluginProcessor.h` (lines 22, 119, 200)

```cpp
// Line 22 — include
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

// Line 119 — accessor (override)
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

// Line 200 — member (private section)
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
```

### `plugins/O-Lyrica/Source/PluginProcessor.cpp` (lines 506, 708)

```cpp
// Line 506 — voice wiring inside the addVoice loop in the constructor
voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // D-09: module-owned table

// Line 708 — top of processBlock, before renderNextBlock
buffer.clear();
// VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
// correlate tuning deltas to their NoteOn's MIDI pitch.
vst3Extensions.drainAndUpdate();
```

### `plugins/O-Lyrica/Source/HarpSynthVoice.h` (lines 21, 88, 129)

```cpp
// Line 21 — include
#include "NoteExpression.h"  // modules/tuning/note-expression (PendingTuningTable + helpers)

// Line 88 — public setter
void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source);

// Line 129 — private member
Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
```

### `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` (lines 84-87, 138-147)

```cpp
// Lines 84-87 — setter body
void HarpSynthVoice::setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
{
    pendingTuningSource = source;
}

// Lines 138-147 — composition site inside startNote()
// VST3 Note Expression tuning delta (Dorico microtonal).
// Composes multiplicatively with the frequency already set by
// TuningEngine / humanize above (D-10): base tuning * NE semitone offset.
// Helper uses exchange(0.0) internally so retriggered notes at the same
// pitch in a later block don't inherit a stale offset.
if (pendingTuningSource != nullptr)
{
    currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                           *pendingTuningSource, midiNoteNumber, currentFrequency);
}
```

---

## Per-Plugin Pattern Assignments

### 1. O-Bells (TuningEngine-composing — closest to O-Lyrica)

**Closest analog in O-Lyrica:** `HarpSynthVoice::startNote` lines 105-147 (TuningEngine query + composition + DSP trigger).

**Match quality:** EXACT.

**Files:**
- `plugins/O-Bells/CMakeLists.txt` (current `PLUGIN_VERSION "4.0.0"`, line 11; `OuariconModules.cmake` already included via line 94)
- `plugins/O-Bells/Source/PluginProcessor.h` (24 includes, no NoteExpression yet; `tuningEngine` member at line 101)
- `plugins/O-Bells/Source/PluginProcessor.cpp` (`addVoice` loop at lines 545-550; `processBlock` at line 739)
- `plugins/O-Bells/Source/BellVoice.{h,cpp}` (voice class declared in `BellVoice.h:18`; `startNote` body at `BellVoice.cpp:100-354`)
- `plugins/O-Bells/CHANGELOG.md`, `plugins/O-Bells/.planning/STATUS.md`

#### CMakeLists.txt patches

**Existing site to copy from (line 94):**
```cmake
# Module system integration
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
```

**Insert (after line 94 or after the existing `if(OUARICON_LICENSING)` block at line 78-82, conventionally near the bottom alongside other module calls):**
```cmake
# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Bells note-expression)
```

**Version bump (line 11):** `PLUGIN_VERSION "4.0.0"` → `PLUGIN_VERSION "4.1.0"` (minor — new user-visible feature).

> **Caveat:** `juce_add_plugin(...)` at line 5 must execute BEFORE `ouaricon_add_module(...)` so per-format subtargets (`O-Bells_VST3`, `O-Bells_AU`, `O-Bells_Standalone`) exist. They do today (line 23 closes `juce_add_plugin`). Adding the module call near line 94 (after the existing `OuariconModules.cmake` include but the `juce_add_plugin` already finished at line 23) is correct. Note that `OuariconModules.cmake` is currently included at line 94 — AFTER `juce_add_plugin` — which is the inverse of O-Lyrica's pattern (line 3, before plugin) but functionally equivalent for the per-format routing. Recommend keeping current order for minimal diff; insert the new line right after line 94's include.

#### PluginProcessor.h patches

**Add after line 23 (last `#include` before `OuariconLicense.h` guard):**
```cpp
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)
```

**Add to public section (insert near existing `getTuningEngine()` accessor at line 69):**
```cpp
// VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
```

**Add to private section (immediately after `juce::Synthesiser synthesiser;` at line 92):**
```cpp
// VST3 Note Expression support (module-owned table + raw-event scratch)
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
```

#### PluginProcessor.cpp patches

**Voice wiring (inside the `for` loop at lines 545-550):**
```cpp
for (int i = 0; i < 16; ++i)
{
    auto* voice = new BellVoice();
    voice->setTuningEngine(&tuningEngine);
    voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
    synthesiser.addVoice(voice);
}
```

**Drain call (top of `processBlock` at line 739, after `buffer.clear()` and BEFORE `renderNextBlock`):**
```cpp
vst3Extensions.drainAndUpdate();
```

#### BellVoice.h patches

**Add after line 14 (`#include "BellSound.h"`):**
```cpp
#include "NoteExpression.h"  // modules/tuning/note-expression
```

**Add public setter (near `setTuningEngine` at line 31):**
```cpp
void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
{
    pendingTuningSource = source;
}
```

**Add private member (near `tuningEngine` at line 210):**
```cpp
Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
```

#### BellVoice.cpp composition site

**Existing base-frequency assignment (lines 161-163):**
```cpp
float fundamental = tuningEngine
    ? static_cast<float>(tuningEngine->getFrequency(midiNoteNumber))
    : static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
```

**Insert immediately after (BEFORE `calculateMultiStageCoefficients(fundamental)` at line 166):**
```cpp
// VST3 Note Expression tuning delta (Dorico microtonal).
// Compose multiplicatively after TuningEngine, before any DSP setup.
if (pendingTuningSource != nullptr)
{
    fundamental = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
        *pendingTuningSource, midiNoteNumber, static_cast<double>(fundamental)));
}
```

> **Note:** O-Bells uses `float fundamental` whereas O-Lyrica uses `double currentFrequency`. The helper signature is `double(PendingTuningTable&, int, double)`, so cast through `double` at the call site. All downstream calculations use `fundamental` directly so this single assignment captures the NE delta.

#### CHANGELOG.md (insert at top, above current `## [4.0.0]` entry)

```markdown
## [4.1.0] - 2026-04-25

### Added

- **VST3 Note Expression microtonal support for Dorico.** O-Bells now responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling correct microtonal playback of quarter-tones, third-tones, and arbitrary tuning deltas authored in Dorico's tonality system. End users must set Microtonality to "VST3 Note Expression" on the assigned expression map (see O-Lyrica 2.3.0 for the procedure).
- **Shared `note-expression` module adoption.** O-Bells consumes the Ouaricon module at `modules/tuning/note-expression` (v1.0.0), same shape as O-Lyrica v2.3.0.

### Technical notes

- **Composition with TuningEngine.** `BellVoice::startNote` computes the fundamental via `TuningEngine::getFrequency(midi)` first, then applies the NE semitone delta via `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)` before `calculateMultiStageCoefficients()`.
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/BellVoice.{h,cpp}`, `CMakeLists.txt`.
- **Version bump rationale:** MINOR (4.0.0 → 4.1.0) — new user-visible feature, backward compatible, no preset impact.
```

#### STATUS.md update site

`plugins/O-Bells/.planning/STATUS.md` is YAML-fronted state file (verified via Read). The `/improve` workflow updates the front-matter fields (`stage`, `last_updated`, `next_action`) — Phase 24 plan inherits this; do not rewrite the file shape.

---

### 2. O-IntonationPad (TuningEngine-composing — multi-sub-voice variation)

**Closest analog in O-Lyrica:** `HarpSynthVoice::startNote` (D-10 composition order). **Variation:** the voice spawns 12 sub-voices each with its own MIDI pitch (chord generator output), so the NE delta must apply to the **note-on MIDI pitch** consistently.

**Match quality:** STRUCTURAL VARIATION — multi-sub-voice resolveFrequency.

**Files:**
- `plugins/O-IntonationPad/CMakeLists.txt` (current `PLUGIN_VERSION "2.7.2"`, line 9; `OuariconModules.cmake` included at line 2)
- `plugins/O-IntonationPad/Source/PluginProcessor.h` (32 lines of includes; ctor at `PluginProcessor.cpp:356`)
- `plugins/O-IntonationPad/Source/PluginProcessor.cpp` (`addVoice` at line 367; `processBlock` at line 511; `setChordGenerationParams` call site at lines 624-628)
- `plugins/O-IntonationPad/Source/DSP/WavetableVoice.{h,cpp}` (voice class declared at `WavetableVoice.h:44`; `resolveFrequency` at `WavetableVoice.cpp:43-53`; `startNote` at `WavetableVoice.cpp:100-220`)
- `plugins/O-IntonationPad/CHANGELOG.md`, `plugins/O-IntonationPad/.planning/STATUS.md`

#### CMakeLists.txt patches

**Insert after line 33 (end of `target_sources`) or anywhere after `juce_add_plugin` at line 5:**
```cmake
# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-IntonationPad note-expression)
```

**Version bump (line 9):** `PLUGIN_VERSION "2.7.2"` → `PLUGIN_VERSION "2.8.0"`.

#### PluginProcessor.h patches

**Add after line 19 (after the last `DSP/...h` include):**
```cpp
#include "NoteExpression.h"  // modules/tuning/note-expression
```

**Add to public section:**
```cpp
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
```

**Add to private section (immediately after `juce::Synthesiser synthesiser;` at line 104):**
```cpp
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
```

#### PluginProcessor.cpp patches

**addVoice loop (lines 365-368):** the constructor doesn't pass `tuningEngine` directly to voices — `tuningEnginePtr` is assigned per-block via `setChordGenerationParams` at line 628. **Recommended approach:** wire `setPendingTuningSource` once at construction (matches O-Lyrica's static wiring; voice can hold the pointer for its lifetime since `vst3Extensions` outlives all voices).

```cpp
// Replace lines 365-368:
for (int i = 0; i < 8; ++i)
{
    auto* voice = new WavetableVoice();
    voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
    synthesiser.addVoice(voice);
}
```

**Drain call (top of `processBlock` at line 511, after `buffer.clear()` at line 516):**
```cpp
vst3Extensions.drainAndUpdate();
```

#### WavetableVoice.h patches

**Add after line 34 (after `ChordGenerator.h` include):**
```cpp
#include "NoteExpression.h"
```

**Add public setter:**
```cpp
void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
{
    pendingTuningSource = source;
}
```

**Add private member (alongside `tuningEnginePtr` at line 144):**
```cpp
Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
```

#### WavetableVoice.cpp composition site

**Composition challenge:** the voice spawns 12 sub-voices via `chordGeneratorPtr->generateChord(midiNoteNumber, ...)`. Each sub-voice gets its own `baseMidiNote` resolved through `resolveFrequency(baseMidiNote, centOffset)` (lines 134, 142, 150). NE deltas are stored per-MIDI-pitch in the 128-slot table. Dorico sends NE for the **noteOn MIDI pitch** (the `midiNoteNumber` arg to `startNote`), not for sub-voice MIDI pitches.

**Recommended placement:** apply the NE delta as a multiplicative ratio derived from `applyPendingTuning(table, midiNoteNumber, 1.0)` and propagate to each sub-voice's frequency. Insert at the top of `startNote` body (after line 102, before the `chordGeneratorPtr` block at line 109):

```cpp
void WavetableVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentVelocity = velocity;
    currentSampleRate = getSampleRate();

    // Calculate gain smoothing coefficient...
    gainSmoothCoeff = 1.0f - std::exp(-1.0f / (0.25f * static_cast<float>(currentSampleRate)));

    // VST3 Note Expression: derive a multiplicative delta from the noteOn MIDI pitch
    // and propagate to all sub-voice frequencies via resolveFrequency. exchange(0.0)
    // inside the helper consumes the slot, so retriggered notes don't inherit stale offsets.
    double neRatio = 1.0;
    if (pendingTuningSource != nullptr)
        neRatio = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, 1.0);

    // ... existing chordGeneratorPtr block ...
}
```

**Then propagate `neRatio` into each `resolveFrequency` call (lines 134, 142, 150):**

```cpp
float baseFreq = static_cast<float>(resolveFrequency(baseMidiNote, centOffset) * neRatio);
// ... and similarly for spacingFreq and inversionFreq.
```

**Alternative (simpler — more conservative):** modify `resolveFrequency` to accept the NE ratio as a third parameter, defaulting to 1.0. The planner picks the cleaner option.

#### CHANGELOG.md entry

```markdown
## [2.8.0] - 2026-04-25

### Added

- **VST3 Note Expression microtonal support for Dorico** (per O-Lyrica 2.3.0 reference shape).
- **Shared `note-expression` module adoption** (`modules/tuning/note-expression` v1.0.0).

### Technical notes

- **Composition with TuningEngine + chord generator.** `WavetableVoice::startNote` computes a multiplicative NE delta from the noteOn MIDI pitch via `applyPendingTuning(table, midi, 1.0)` and applies it to every sub-voice's `resolveFrequency` result. NE deltas correlate to the original noteOn pitch (Pattern 1: noteId, not pitch); sub-voice octave shifts inherit the tuned root.
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/DSP/WavetableVoice.{h,cpp}`, `CMakeLists.txt`.
- **Version bump rationale:** MINOR (2.7.2 → 2.8.0) — new user-visible feature.
```

---

### 3. O-Prism (TuningEngine-composing — closest match)

**Closest analog in O-Lyrica:** `HarpSynthVoice::startNote` lines 113-147 — virtually identical pattern (TuningEngine getFrequency + voice-side multi-oscillator setFrequency).

**Match quality:** EXACT.

**Files:**
- `plugins/O-Prism/CMakeLists.txt` (current `VERSION 1.16.1`, line 12; already calls `ouaricon_add_module(O-Prism webview-relay-manager)` at line 81 — proves the macro works in this plugin)
- `plugins/O-Prism/Source/PluginProcessor.h` (clean class structure; ctor at `PluginProcessor.cpp:472-478`)
- `plugins/O-Prism/Source/PluginProcessor.cpp` (`addVoice` loop at lines 472-478; `processBlock` at line 559)
- `plugins/O-Prism/Source/PrismVoice.{h,cpp}` (`startNote` at `PrismVoice.cpp:157-238`; base-frequency assignment at lines 181-185)
- `plugins/O-Prism/CHANGELOG.md`, `plugins/O-Prism/.planning/STATUS.md`

#### CMakeLists.txt patches

**Insert near existing `ouaricon_add_module` at line 81:**
```cmake
# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Prism note-expression)
```

**Version bump (line 12):** `VERSION 1.16.1` → `VERSION 1.17.0`.

#### PluginProcessor.h patches

**Add after line 30 (after `OuariconPresetManager.h`):**
```cpp
#include "NoteExpression.h"
```

**Add to public section (near `getAPVTS()` at line 60):**
```cpp
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
```

**Add to private section (after `juce::Synthesiser synthesiser;` at line 153):**
```cpp
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
```

#### PluginProcessor.cpp patches

**addVoice loop (lines 472-478):**
```cpp
auto* voice = new PrismVoice();
voice->setTuningEngine(&tuningEngine);
voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24
synthesiser.addVoice(voice);
```

**Drain call (top of `processBlock` at line 559, after buffer.clear()):**
```cpp
vst3Extensions.drainAndUpdate();
```

#### PrismVoice.h patches

Add `#include "NoteExpression.h"`, public setter `setPendingTuningSource`, private member `pendingTuningSource` (mirror O-Lyrica HarpSynthVoice.h:21, 88, 129).

#### PrismVoice.cpp composition site

**Existing base-frequency assignment (lines 181-185):**
```cpp
// Get base frequency from TuningEngine
if (tuningEngine != nullptr)
    currentFrequency = tuningEngine->getFrequency (midiNoteNumber);
else
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
```

**Insert immediately after line 185, BEFORE `glide.setTarget(currentFrequency)` at line 196 and BEFORE the per-oscillator `setFrequency` calls at lines 202, 219, 238:**
```cpp
// VST3 Note Expression tuning delta (Dorico).
if (pendingTuningSource != nullptr)
{
    currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                           *pendingTuningSource, midiNoteNumber, currentFrequency);
}
```

> **Composition correctness:** `currentFrequency` is the multiplicative root for `freqA = currentFrequency * pow(2, ...)`, `freqB = currentFrequency * pow(2, ...)`, and `subOsc.setFrequency(currentFrequency)`. Applying NE before the multiplications is mathematically correct for any base tuning (D-10).

#### CHANGELOG.md entry

```markdown
## v1.17.0 (2026-04-25)

### Added
- **VST3 Note Expression microtonal support for Dorico** (per O-Lyrica 2.3.0 reference shape).
- **Shared `note-expression` module adoption.**

### Technical Notes
- Composition with TuningEngine: `PrismVoice::startNote` queries `TuningEngine::getFrequency(midi)`, then composes Dorico's NE delta via `applyPendingTuning(table, midi, freq)` before any oscillator `setFrequency` call.
- Files modified: `Source/PluginProcessor.{h,cpp}`, `Source/PrismVoice.{h,cpp}`, `CMakeLists.txt`.
- Version bump rationale: MINOR (1.16.1 → 1.17.0).
```

---

### 4. O-Wind (TuningEngine-composing + physical-model period — flute waveguide)

**Closest analog in O-Lyrica:** `HarpSynthVoice::startNote` lines 113-147 with a **physical-model post-tuning step** (analogous to `stringModel.trigger(currentFrequency, ...)` at line 259 of HarpSynthVoice.cpp). Here, `boreWaveguide.setBoreDelay(totalLoopDelay / (1.0f + initJetRatio))` at FluteSynthVoice.cpp:100 derives the period.

**Match quality:** EXACT (same shape; physical-model period derivation is post-frequency, identical to O-Lyrica's `stringModel.trigger`).

**Files:**
- `plugins/O-Wind/CMakeLists.txt` (line 6 `juce_add_plugin(O-Wind ...)` — **PLUGIN_VERSION not present in juce_add_plugin args**; relies on JUCE default. Planner adds `PLUGIN_VERSION "1.16.0"` explicitly. `OuariconModules.cmake` already included at line 3.)
- `plugins/O-Wind/Source/PluginProcessor.h` (clean shape; `tuningEngine` at line 70)
- `plugins/O-Wind/Source/PluginProcessor.cpp` (`addVoice` loop at lines 487-489; `processBlock` at line 532)
- `plugins/O-Wind/Source/FluteSynthVoice.{h,cpp}` (constructor at FluteSynthVoice.cpp:57-61 takes `tuningEngine` directly; `startNote` at lines 68-159; base-frequency assignment at lines 78-81; pitch-bend wrap at line 84; `boreWaveguide.setBoreDelay` at line 100)
- `plugins/O-Wind/CHANGELOG.md`, `plugins/O-Wind/.planning/STATUS.md`

#### CMakeLists.txt patches

**Insert after line 47 (end of `target_sources`):**
```cmake
# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Wind note-expression)
```

**Version bump:** O-Wind's CMakeLists has no `PLUGIN_VERSION` line in `juce_add_plugin`. Most recent CHANGELOG entry is `[1.15.1]`. Planner adds:
```cmake
PLUGIN_VERSION "1.16.0"
```
inside the `juce_add_plugin(O-Wind ...)` block (between line 11 `PRODUCT_NAME` and line 12 `IS_SYNTH`).

#### PluginProcessor.h patches

Standard shape (mirror O-Lyrica): include `NoteExpression.h`, add `getVST3ClientExtensions()` override, add `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;` private member after `juce::Synthesiser synthesiser;` at line 67.

#### PluginProcessor.cpp patches

**addVoice loop (lines 487-489):**
```cpp
auto* voice = new FluteSynthVoice (&parameters, &tuningEngine);
voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24
synthesiser.addVoice (voice);
```

**Drain call (top of `processBlock` at line 532):**
```cpp
vst3Extensions.drainAndUpdate();
```

#### FluteSynthVoice.h patches

Standard shape (`#include "NoteExpression.h"`, `setPendingTuningSource` setter, `pendingTuningSource` member).

#### FluteSynthVoice.cpp composition site

**Existing base-frequency assignment (lines 78-81):**
```cpp
if (tuningEngine != nullptr)
    currentFrequency = static_cast<float> (tuningEngine->getFrequency (midiNoteNumber));
else
    currentFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
```

**Insert immediately after line 81, BEFORE the pitch-bend application at line 84:**
```cpp
// VST3 Note Expression tuning delta (Dorico).
if (pendingTuningSource != nullptr)
{
    currentFrequency = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
        *pendingTuningSource, midiNoteNumber, static_cast<double>(currentFrequency)));
}
```

> **Why before pitch-bend:** Pitch-bend (line 84) is computed from `pitchWheelValue` (legacy MIDI pitch wheel) and applies as another multiplicative ratio. NE composes with both — apply NE first so the bore-delay derivation at line 100 reflects all tuning sources.

> **Float casting:** O-Wind uses `float currentFrequency` (not `double` like O-Lyrica). Cast through `double` at the helper boundary, same pattern as O-Bells.

#### CHANGELOG.md entry

```markdown
## [1.16.0] - 2026-04-25

### Added — VST3 Note Expression Microtonal Support for Dorico

O-Wind responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events) for microtonal playback. Composition order: TuningEngine → NE delta → pitch-bend → bore-delay derivation. The bore waveguide period sizes to the tuned frequency on the first sample (no attack zipper).

**Files Modified:** `Source/PluginProcessor.{h,cpp}`, `Source/FluteSynthVoice.{h,cpp}`, `CMakeLists.txt`. Version: 1.15.1 → 1.16.0.
```

---

### 5. O-Reed (MPESynthesiserVoice + physical-model bore waveguide)

**Closest analog in O-Lyrica:** `HarpSynthVoice::startNote` (composition order). **Variation:** `MPESynthesiserVoice::noteStarted()` reads MIDI pitch from `getCurrentlyPlayingNote().initialNote` instead of receiving it as a parameter.

**Match quality:** STRUCTURAL (MPE — different base class, same composition order).

**Files:**
- `plugins/O-Reed/CMakeLists.txt` (no `PLUGIN_VERSION` in `juce_add_plugin` — most recent CHANGELOG `v1.0.11`; `OuariconModules.cmake` included at line 3)
- `plugins/O-Reed/Source/PluginProcessor.h` (line 63 `juce::MPESynthesiser synthesiser;` — MPE variant)
- `plugins/O-Reed/Source/PluginProcessor.cpp` (`addVoice` loop at lines 341-344; `processBlock` at line 378)
- `plugins/O-Reed/Source/ReedWindVoice.{h,cpp}` (line 24 `class ReedWindVoice : public juce::MPESynthesiserVoice`; `noteStarted()` at lines 133-300; helper `getBaseFrequencyFromTuning` at lines 121-126; **two composition sites** — legato at line 141, normal at line 202)
- `plugins/O-Reed/Source/DSP/BoreWaveguide.h` (period derivation via `setFrequency()`)
- `plugins/O-Reed/CHANGELOG.md`, `plugins/O-Reed/.planning/STATUS.md`

#### CMakeLists.txt patches

**Insert after line 29 (end of `target_sources`):**
```cmake
ouaricon_add_module(O-Reed note-expression)
```

**Version bump:** add `PLUGIN_VERSION "1.1.0"` in `juce_add_plugin` (between line 11 `PRODUCT_NAME` and `IS_SYNTH`).

#### PluginProcessor.h patches

Standard shape; private member after line 63 `juce::MPESynthesiser synthesiser;`.

#### PluginProcessor.cpp patches

**addVoice loop (lines 341-344):**
```cpp
auto* voice = new ReedWindVoice(i);
voice->setAPVTS(&parameters);
voice->setTuningEngine(&tuningEngine);
voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24
synthesiser.addVoice(voice);
```

**Drain call (top of `processBlock` at line 378):**
```cpp
vst3Extensions.drainAndUpdate();
```

#### ReedWindVoice.h patches

Standard shape (`#include "NoteExpression.h"`, public setter, private `pendingTuningSource` member alongside `tuningEngine` at line 60).

#### ReedWindVoice.cpp composition sites

**TWO composition sites** (legato at line 141, normal at line 202). Each calls `getBaseFrequencyFromTuning(note.initialNote)` then applies pitch-bend.

**Recommended:** add the NE composition INSIDE the `getBaseFrequencyFromTuning` helper (lines 121-126), so both call sites inherit it. The MIDI note is already a parameter:

```cpp
// Modified getBaseFrequencyFromTuning (lines 121-126):
float ReedWindVoice::getBaseFrequencyFromTuning(int midiNote) const
{
    double freq = (tuningEngine != nullptr)
        ? tuningEngine->getFrequency(midiNote)
        : juce::MidiMessage::getMidiNoteInHertz(midiNote);

    // VST3 Note Expression tuning delta (Dorico).
    // Helper consumes slot via exchange(0.0); safe to call once per noteStarted().
    if (pendingTuningSource != nullptr)
        freq = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNote, freq);

    return static_cast<float>(freq);
}
```

> **Caveat:** the helper is currently `const` and called THREE times in `noteStarted()` (lines 141, 202) and once in `notePitchbendChanged()`-equivalent at line 374. `applyPendingTuning` uses `exchange(0.0)` which **consumes** the slot — calling it from inside the helper means the FIRST call consumes the offset; subsequent calls in the same block see 0.0 and return the base freq unchanged. **This is correct behavior:** Dorico delivers one NE per note-on, so consuming once per noteStarted is what we want. But if the helper is called from `notePitchbendChanged` (line 374) DURING a held note, that call will return base freq unchanged (slot already empty) — also correct, because pitch-bend updates shouldn't see stale NE state.
>
> **Alternative:** apply NE at each call site explicitly (after `getBaseFrequencyFromTuning` returns) to keep the helper pure and the NE consumption visible. The planner picks the cleaner shape; the in-helper approach is more DRY.

#### CHANGELOG.md entry

```markdown
## v1.1.0 (2026-04-25)

### Added
- **VST3 Note Expression microtonal support for Dorico.** Composes with TuningEngine via `getBaseFrequencyFromTuning` helper before bore waveguide period derivation. MPE-aware: NE delta applies to `note.initialNote`, MPE pitch-bend stacks multiplicatively on top.
- **Shared `note-expression` module adoption.**

### Technical Notes
- Files modified: `Source/PluginProcessor.{h,cpp}`, `Source/ReedWindVoice.{h,cpp}`, `CMakeLists.txt`.
- Composition order: tuning engine → NE delta → MPE pitch-bend → `bore.setFrequency(freq)` (bore waveguide period derived from final tuned frequency).
- Version: 1.0.11 → 1.1.0.
```

---

### 6. O-Bowed (MPESynthesiserVoice + physical-model waveguide string)

**Closest analog in O-Lyrica:** `HarpSynthVoice::startNote` (composition order). **Variation:** MPE — same as O-Reed.

**Match quality:** STRUCTURAL (MPE).

**Files:**
- `plugins/O-Bowed/CMakeLists.txt` (no `PLUGIN_VERSION` in `juce_add_plugin`; `OuariconModules.cmake` included at line 3)
- `plugins/O-Bowed/Source/PluginProcessor.h` (line 77 `BowedMPESynthesiser synthesiser;`)
- `plugins/O-Bowed/Source/PluginProcessor.cpp` (`addVoice` loop at lines 246-250; `processBlock` at line 288)
- `plugins/O-Bowed/Source/BowedStringVoice.{h,cpp}` (line 32 `class BowedStringVoice : public juce::MPESynthesiserVoice`; `noteStarted()` at lines 25-47; `getBaseFrequencyFromTuning` helper at lines 291-296; `notePitchbendChanged()` at lines 65-77 — **second composition site**; `waveguideString.trigger(currentFrequency)` at lines 39, 76)
- `plugins/O-Bowed/CHANGELOG.md`, `plugins/O-Bowed/.planning/STATUS.md`

#### CMakeLists.txt patches

```cmake
ouaricon_add_module(O-Bowed note-expression)
```
(after line 49, end of `target_sources`).

**Version bump:** add `PLUGIN_VERSION "1.X.0"` in `juce_add_plugin` (planner reads current bump tip from CHANGELOG.md). Use minor bump from current top entry.

#### PluginProcessor.h / .cpp patches

Standard shape. addVoice loop at lines 246-250:
```cpp
auto* voice = new BowedStringVoice (&parameters);
voice->setVoiceIndex (i);
voice->setTuningEngine (&tuningEngine);
voice->setHumanizeEngine (&humanizeEngine);
voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24
synthesiser.addVoice (voice);
```

#### BowedStringVoice.h / .cpp composition site

**Same pattern as O-Reed** — apply NE inside the `getBaseFrequencyFromTuning` helper at lines 291-296:

```cpp
float BowedStringVoice::getBaseFrequencyFromTuning (int midiNote) const
{
    double freq = (tuningEngine != nullptr)
        ? tuningEngine->getFrequency (midiNote)
        : juce::MidiMessage::getMidiNoteInHertz (midiNote);

    if (pendingTuningSource != nullptr)
        freq = Ouaricon::NoteExpression::applyPendingTuning (*pendingTuningSource, midiNote, freq);

    return static_cast<float> (freq);
}
```

This automatically covers BOTH composition sites: `noteStarted()` line 32 and `notePitchbendChanged()` line 71. Same `exchange(0.0)` semantics as O-Reed — first call consumes, second call returns base freq unchanged (correct: pitch-bend updates during a held note don't re-trigger NE).

#### CHANGELOG.md entry

Mirror O-Reed pattern; phrase "adds VST3 Note Expression microtonal support for Dorico"; cite composition with `WaveguideString::trigger(freq)` after NE application.

---

### 7. O-Formant (MPESynthesiserVoice — pitched fundamental drives glottal source)

**Closest analog in O-Lyrica:** `HarpSynthVoice::startNote` (composition order). **Variation:** MPE — same shape as O-Reed/O-Bowed. **Pitched confirmation:** `FormantVoice` IS pitched — `tunedF0` cached at line 188 is the glottal-source fundamental (`glottalSource.setFrequency(finalF0)` at line 650). `ConsonantEngine.h` is the consonant articulator (separate concern from pitched fundamental).

**Match quality:** STRUCTURAL (MPE — same as O-Reed and O-Bowed).

**Files:**
- `plugins/O-Formant/CMakeLists.txt` (current `VERSION 1.24.2`, line 10; **`OuariconModules.cmake` NOT INCLUDED** — needs to be added before `ouaricon_add_module` works)
- `plugins/O-Formant/Source/PluginProcessor.h` (line 81 `juce::MPESynthesiser synthesiser;`; line 58 `TuningEngine tuningEngine;` PUBLIC member — slightly unusual, but consumable as-is)
- `plugins/O-Formant/Source/PluginProcessor.cpp` (`addVoice` loop at lines 694-699; `processBlock` at line 742)
- `plugins/O-Formant/Source/FormantVoice.{h,cpp}` (line 29 `class FormantVoice : public juce::MPESynthesiserVoice`; line 103 `tuningEnginePtr`; line 104 `tunedF0` cache; `noteStarted()` at lines 132-...; base-frequency assignment at lines 188-191; `glottalSource.setFrequency(finalF0)` at line 650)
- `plugins/O-Formant/CHANGELOG.md`, `plugins/O-Formant/.planning/STATUS.md`

#### CMakeLists.txt patches

**CRITICAL:** O-Formant currently does NOT include `OuariconModules.cmake`. **Add at the top (immediately after `cmake_minimum_required` at line 1 and before line 4 `juce_add_plugin`):**
```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
```

**Then add module call (after line 34 `target_sources` block):**
```cmake
# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Formant note-expression)
```

**Version bump (line 10):** `VERSION 1.24.2` → `VERSION 1.25.0`.

#### PluginProcessor.h patches

**Add after line 23 (last `EmbeddedTunings.h` include):**
```cpp
#include "NoteExpression.h"
```

**Add to public section (near `getLyricsEngine()` at line 63):**
```cpp
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
```

**Add to private section (after `juce::MPESynthesiser synthesiser;` at line 81):**
```cpp
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
```

#### PluginProcessor.cpp patches

**addVoice loop (lines 694-699):**
```cpp
auto* voice = new FormantVoice (i);
voice->setAPVTS (&parameters);
voice->setWavetable (&glottalWavetable);
voice->setTuningEngine (&tuningEngine);
voice->setLyricsEngine (&lyricsEngine);
voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24
synthesiser.addVoice (voice);
```

**Drain call (top of `processBlock` at line 742):**
```cpp
vst3Extensions.drainAndUpdate();
```

#### FormantVoice.h / .cpp composition site

Standard shape — `#include "NoteExpression.h"`, public setter, private member alongside `tuningEnginePtr` at line 103.

**Existing base-frequency assignment (`FormantVoice.cpp` lines 187-191):**
```cpp
int midiNote = currentlyPlayingNote.initialNote;
tunedF0 = tuningEnginePtr != nullptr
    ? static_cast<float> (tuningEnginePtr->getFrequency (midiNote))
    : static_cast<float> (getCurrentlyPlayingNote().getFrequencyInHertz());
float f0 = tunedF0;
```

**Insert immediately after line 191, BEFORE `pitchGlide.setTarget(f0)` at line 196 / `pitchGlide.snapTo(f0)` at line 198:**
```cpp
// VST3 Note Expression tuning delta (Dorico).
if (pendingTuningSource != nullptr)
{
    tunedF0 = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
        *pendingTuningSource, midiNote, static_cast<double>(tunedF0)));
}
float f0 = tunedF0;  // re-read after NE composition
```

> **Composition correctness:** `tunedF0` is referenced in `renderNextBlock` at lines 488, 616 for spectral tilt and source-filter coupling. Applying NE before `pitchGlide.snapTo` ensures the glottal source samples the correct fundamental from sample 0 (Pattern 2: no attack zipper).

#### CHANGELOG.md entry

```markdown
## [1.25.0] - 2026-04-25

### Added — VST3 Note Expression Microtonal Support for Dorico

O-Formant responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events). The voice's cached `tunedF0` composes Dorico's NE delta multiplicatively after `TuningEngine::getFrequency` and before `PitchGlide` / glottal source frequency assignment. MPE pitch-bend stacks on top via `getCurrentlyPlayingNote().getFrequencyInHertz()` (per-sample lookup unaffected by NE).

**Files Modified:** `CMakeLists.txt` (added `include(OuariconModules.cmake)` + `ouaricon_add_module`), `Source/PluginProcessor.{h,cpp}`, `Source/FormantVoice.{h,cpp}`. Version: 1.24.2 → 1.25.0.
```

---

## Shared Patterns (cross-cutting — apply to every Phase 24 plan)

### Pattern A: Module include + macro call (CMakeLists.txt)

**Source:** `plugins/O-Lyrica/CMakeLists.txt:3 + 80`
**Apply to:** All 7 Phase 24 plugins.

**Pre-conditions:**
1. `juce_add_plugin(<Plugin> ...)` must execute before `ouaricon_add_module(<Plugin> note-expression)` so per-format subtargets (`<Plugin>_VST3`, `<Plugin>_AU`, `<Plugin>_Standalone`) exist for `OuariconModules.cmake`'s D-27 per-format loop.
2. `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` must run before `ouaricon_add_module`. **O-Formant lacks this — add it.**

**Universal one-liner:**
```cmake
ouaricon_add_module(<Plugin> note-expression)
```
No plugin-side per-format awareness needed (D-26/D-27/D-29). The macro auto-routes `cpp/vst3/NoteExpression_VST3.cpp` into `<Plugin>_VST3` only and `cpp/NoteExpression.cpp` into SharedCode.

### Pattern B: PluginProcessor extensions member + accessor (PluginProcessor.h/.cpp)

**Source:** `plugins/O-Lyrica/Source/PluginProcessor.h:22, 119, 200` + `plugins/O-Lyrica/Source/PluginProcessor.cpp:506, 708`
**Apply to:** All 7 plugins.

**Three-line shape:**
```cpp
// Header: #include + override + private member
#include "NoteExpression.h"
juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

// Cpp: voice wiring (addVoice loop) + drain call (top of processBlock)
voice->setPendingTuningSource(&vst3Extensions.getPendingTable());
vst3Extensions.drainAndUpdate();
```

### Pattern C: Voice composition (Voice.h/.cpp)

**Source:** `plugins/O-Lyrica/Source/HarpSynthVoice.h:21, 88, 129` + `plugins/O-Lyrica/Source/HarpSynthVoice.cpp:138-147`
**Apply to:** All 7 voice classes (with MPE adjustment for O-Reed/O-Bowed/O-Formant).

**Shape:**
```cpp
// Voice.h:
#include "NoteExpression.h"
void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source);
private:
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

// Voice.cpp inside startNote / noteStarted, AFTER base-frequency lookup, BEFORE DSP trigger:
if (pendingTuningSource != nullptr)
    currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                           *pendingTuningSource, midiNoteNumber, currentFrequency);
```

**Composition order (D-10) is load-bearing:**
1. Base-frequency lookup (`tuningEngine->getFrequency(midi)` or `getMidiNoteInHertz(midi)`).
2. **`applyPendingTuning(table, midi, freq)`** — NE delta applied here.
3. Pitch-bend / glide / humanization (each composes multiplicatively on top — order between 3-step components is plugin-specific).
4. DSP trigger (`stringModel.trigger`, `boreWaveguide.setBoreDelay`, `glottalSource.setFrequency`, `oscX.setFrequency`).

**For MPE plugins (O-Reed, O-Bowed, O-Formant):**
- `noteStarted()` reads `int midiNote = getCurrentlyPlayingNote().initialNote;`
- Helper recommended (see O-Reed): apply NE inside `getBaseFrequencyFromTuning(midiNote)` so multiple call sites (noteStarted, notePitchbendChanged) inherit it. The `exchange(0.0)` consume semantics are correct — second call in same block returns base freq unchanged.

### Pattern D: CHANGELOG.md entry style (per TRACK-03)

**Source:** `plugins/O-Lyrica/CHANGELOG.md:5-27` (the `## [2.3.0] - 2026-04-24` entry).
**Apply to:** All 7 plugins.

**Required first-line phrasing in entry:**
> "adds VST3 Note Expression microtonal support for Dorico"

**Recommended structure (mirror O-Lyrica 2.3.0):**
- `## [<new-version>] - <date>` heading.
- `### Added` — feature description with end-user-facing language (Dorico setup hint).
- `### Technical notes` — files modified, composition order, version bump rationale.

### Pattern E: STATUS.md update style

**Source:** `plugins/O-Bells/.planning/STATUS.md` (YAML front-matter shape).
**Apply to:** All 7 plugins.

**Format:** YAML front-matter file with fields `plugin`, `stage`, `phase`, `status`, `last_updated`, `next_action`. The `/improve` workflow updates these fields automatically; **Phase 24 PLAN.md does NOT manually rewrite the file shape** — `/improve`'s built-in STATUS-update logic handles it.

### Pattern F: Module registry update

**Source:** `modules/registry.yaml` `note-expression.used_by:` list (currently contains `OLyrica`).
**Apply to:** All 7 plugins via `/module-add note-expression` invoked inside each `/improve` cycle.

By plan 24-07 close, the list should contain all 8: `OLyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant`.

---

## No Analog Found

None — every plugin has a clean analog. The MPE plugins (O-Reed, O-Bowed, O-Formant) are structural variations, not "no analog" cases; the multi-sub-voice plugin (O-IntonationPad) is also a structural variation with a clear extension path (Pattern C with neRatio propagation).

---

## Recommended Easy-First Ordering (D-11)

Based on integration distance from O-Lyrica, planner can use this ordering for Plans 24-01 through 24-07:

| Order | Plugin | Reason |
|-------|--------|--------|
| 1 | **O-Bells** | EXACT match. `juce::Synthesiser` + `startNote(int, float, ...)` + TuningEngine. Cleanest canary. |
| 2 | **O-Prism** | EXACT match. Already calls `ouaricon_add_module` (webview-relay-manager) — proves the macro works end-to-end in this build. |
| 3 | **O-Wind** | EXACT match (TuningEngine + classic Synthesiser). Adds the physical-model period validation (BoreWaveguide). |
| 4 | **O-IntonationPad** | Multi-sub-voice variation — proves the neRatio propagation pattern before MPE plugins. |
| 5 | **O-Reed** | First MPE plugin. Validates the `getBaseFrequencyFromTuning` helper-based composition. |
| 6 | **O-Bowed** | Second MPE plugin (same shape as O-Reed). |
| 7 | **O-Formant** | Last because **also requires adding `include(OuariconModules.cmake)`** to CMakeLists — extra structural delta. |

---

## Metadata

**Analog search scope:** `plugins/O-Lyrica` (reference) + 7 target plugins' `Source/` and `CMakeLists.txt`.
**Files scanned:** 28 source files (7 plugins × 4 files each) + 7 CHANGELOGs + Phase 23 reference shape.
**Pattern extraction date:** 2026-04-25
**Consumed by:** `gsd-planner` to author 7 per-plugin PLAN.md files (24-01 through 24-07) plus 24-INTEGRATION-MATRIX.md cross-reference table.
