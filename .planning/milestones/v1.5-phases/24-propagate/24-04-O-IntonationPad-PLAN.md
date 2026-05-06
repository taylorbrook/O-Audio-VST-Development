---
phase: 24-propagate
plan: 04
type: execute
wave: 4
depends_on: [24-03]
files_modified:
  - plugins/O-IntonationPad/CMakeLists.txt
  - plugins/O-IntonationPad/Source/PluginProcessor.h
  - plugins/O-IntonationPad/Source/PluginProcessor.cpp
  - plugins/O-IntonationPad/Source/DSP/WavetableVoice.h
  - plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp
  - plugins/O-IntonationPad/CHANGELOG.md
  - plugins/O-IntonationPad/.planning/STATUS.md
  - modules/registry.yaml
autonomous: false
requirements: [PROP-02, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05]

must_haves:
  truths:
    - "O-IntonationPad responds to Dorico VST3 NE — pitch lands +50¢ above C4 for quarter-sharp C4 (root pitch detuned correctly)."
    - "Multi-sub-voice composition: NE delta on the root MIDI pitch propagates as a multiplicative `neRatio` to all 12 sub-voices' frequencies — sub-voice octave shifts inherit the tuned root."
    - "First sample at tuned pitch (no attack zipper) — `neRatio` derived once at top of `startNote` BEFORE chord generator runs."
    - "Chord polyphony correlates by `noteId`: only the C4-rooted voice cluster detuned; an E4-rooted voice cluster sounds at natural 12-TET."
    - "Tri-format build clean. TRACK-01..05: /improve ran; version 2.7.2 → 2.8.0; CHANGELOG TRACK-03; STATUS updated; freshly installed; registry."
  artifacts:
    - path: "plugins/O-IntonationPad/CMakeLists.txt"
      provides: "Module consumption + minor bump"
      contains: "ouaricon_add_module(O-IntonationPad note-expression)"
    - path: "plugins/O-IntonationPad/Source/PluginProcessor.h"
      provides: "VST3Extensions member"
      contains: "Ouaricon::NoteExpression::VST3Extensions vst3Extensions"
    - path: "plugins/O-IntonationPad/Source/PluginProcessor.cpp"
      provides: "Static voice wiring at construction (vst3Extensions outlives all voices) + drain"
      contains: "vst3Extensions.drainAndUpdate("
    - path: "plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp"
      provides: "neRatio derivation + sub-voice propagation"
      contains: "Ouaricon::NoteExpression::applyPendingTuning("
    - path: "plugins/O-IntonationPad/CHANGELOG.md"
      provides: "TRACK-03 entry"
      contains: "adds VST3 Note Expression microtonal support for Dorico"
    - path: "modules/registry.yaml"
      provides: "Consumer registration"
      contains: "plugin: O-IntonationPad"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-IntonationPad.vst3"
      provides: "Freshly installed VST3"
    - path: "~/Library/Audio/Plug-Ins/Components/O-IntonationPad.component"
      provides: "Freshly installed AU"
    - path: ".planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md"
      provides: "Plan close-out"
  key_links:
    - from: "WavetableVoice.cpp::startNote (top of body, line 102 area)"
      to: "neRatio = applyPendingTuning(*table, midiNoteNumber, 1.0)"
      via: "BEFORE chordGeneratorPtr block at line 109"
      pattern: "applyPendingTuning\\(.*pendingTuningSource.*1\\.0"
    - from: "WavetableVoice.cpp resolveFrequency call sites at lines 134, 142, 150"
      to: "neRatio multiplication"
      via: "multiply each result by neRatio so sub-voice octave shifts inherit tuned root"
      pattern: "resolveFrequency\\(.*\\)\\s*\\*\\s*neRatio|neRatio\\s*\\*\\s*resolveFrequency"
    - from: "PluginProcessor.cpp addVoice loop (lines 365-368)"
      to: "voice->setPendingTuningSource(&vst3Extensions.getPendingTable())"
      via: "static wiring at construction (NOT per-block) — vst3Extensions outlives all voices, matches O-Lyrica static wiring"
      pattern: "setPendingTuningSource\\(&vst3Extensions"
---

<objective>
Propagate `note-expression` into **O-IntonationPad** (Plan 24-04 — fourth wave). STRUCTURAL VARIATION: multi-sub-voice. The voice spawns 12 sub-voices via `chordGeneratorPtr->generateChord(midiNoteNumber, ...)`; each sub-voice has its own `baseMidiNote` resolved through `resolveFrequency(baseMidiNote, centOffset)` (lines 134, 142, 150). Dorico sends NE for the **root noteOn MIDI pitch** (the `midiNoteNumber` arg to `startNote`), NOT for sub-voice MIDI pitches.

**Approach (D-04 / D-06 from CONTEXT.md, locked at planning time):** derive a multiplicative ratio `neRatio = applyPendingTuning(*table, midiNoteNumber, 1.0)` at top of `startNote`, then multiply each sub-voice's `resolveFrequency` result by `neRatio`. Conservative variant — no signature change to `resolveFrequency`. Per PATTERNS.md the "alternative simpler" option (modify `resolveFrequency` to accept ratio param defaulted to 1.0) was offered but the planner picks the multiplicative-rescore approach to minimize call-site delta.

**Static wiring (D-06 from PATTERNS.md):** `setPendingTuningSource` wires ONCE at construction (NOT per-block). The constructor doesn't pass `tuningEngine` to voices (assigned per-block via `setChordGenerationParams`), but `vst3Extensions` outlives all voices — voice can hold the pointer for its lifetime. Match O-Lyrica's static wiring pattern.

Purpose: PROP-02 + TRACK-01..05 via `/improve O-IntonationPad`. Version 2.7.2 → 2.8.0 (minor).

Output: Source edits + CHANGELOG + STATUS + registry + bundles + commit + SUMMARY.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/ROADMAP.md
@.planning/REQUIREMENTS.md
@.planning/phases/24-propagate/24-CONTEXT.md
@.planning/phases/24-propagate/24-PATTERNS.md
@.planning/phases/24-propagate/24-INTEGRATION-MATRIX.md
@.planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@CLAUDE.md
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp
@plugins/O-IntonationPad/CMakeLists.txt
@plugins/O-IntonationPad/Source/PluginProcessor.h
@plugins/O-IntonationPad/Source/PluginProcessor.cpp
@plugins/O-IntonationPad/Source/DSP/WavetableVoice.h
@plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp
@plugins/O-IntonationPad/Source/DSP/ChordGenerator.h
@plugins/O-IntonationPad/CHANGELOG.md
@plugins/O-IntonationPad/.planning/STATUS.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/plugin-improve/SKILL.md
@scripts/verify-au-link.sh

<integrations>
[O-IntonationPad row in 24-INTEGRATION-MATRIX.md](./24-INTEGRATION-MATRIX.md#o-intonationpad)

**Match quality:** STRUCTURAL VARIATION — multi-sub-voice. Composition shape diverges from canary at the call-site count (3 sub-voice freq derivations) but pattern integrity preserved (NE applied BEFORE every downstream consumer of frequency = Pattern 2 satisfied).

**Why neRatio not freq directly:** the helper `applyPendingTuning(table, midi, freq)` returns `freq * pow(2, semis/12)`. With `freq=1.0`, the return is just the multiplicative ratio — clean to apply per sub-voice. The `exchange(0.0)` consume happens on the FIRST call (with `freq=1.0`); subsequent calls in the same block return `1.0` if invoked again on the same MIDI slot — but we only call ONCE per `startNote`, store in `neRatio`, and propagate. Correct semantics.
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight — confirm O-IntonationPad preconditions</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md ([O-IntonationPad row](./24-INTEGRATION-MATRIX.md#o-intonationpad))
    - .planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md
    - plugins/O-IntonationPad/.planning/STATUS.md
    - plugins/O-IntonationPad/CMakeLists.txt (line 9 `PLUGIN_VERSION "2.7.2"`; line 2 OuariconModules.cmake; line 5 juce_add_plugin)
    - plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp lines 100-160 (startNote body; resolveFrequency calls at lines 134, 142, 150)
    - plugins/O-IntonationPad/Source/DSP/ChordGenerator.h (sub-voice generation API)
  </read_first>
  <action>
    1. STATUS not 🚧.
    2. Confirm `PLUGIN_VERSION "2.7.2"` at line 9, `OuariconModules.cmake` included at line 2.
    3. Confirm WavetableVoice.cpp's startNote at line 100 with `resolveFrequency(baseMidiNote, centOffset)` calls at lines 134, 142, 150 (the three sub-voice frequency derivation sites).
    4. JUCE patch markers present.
    5. Working tree clean.
  </action>
  <verify>
    <automated>grep -E 'PLUGIN_VERSION "2\.7\.2"' plugins/O-IntonationPad/CMakeLists.txt && grep -E 'include\(\$\{CMAKE_SOURCE_DIR\}/modules/cmake/OuariconModules\.cmake\)' plugins/O-IntonationPad/CMakeLists.txt && grep -nE 'resolveFrequency\(' plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp | head -5 && grep -l 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h && git diff --quiet -- plugins/O-IntonationPad modules/registry.yaml</automated>
  </verify>
  <acceptance_criteria>
    - Baseline `PLUGIN_VERSION "2.7.2"`; OuariconModules.cmake at line 2.
    - resolveFrequency call sites confirmed in WavetableVoice.cpp.
    - JUCE patch markers present; tree clean.
  </acceptance_criteria>
  <done>Preconditions PASS.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Run /improve O-IntonationPad with locked specification (multi-sub-voice neRatio)</name>
  <read_first>
    - .planning/phases/24-propagate/24-PATTERNS.md §"2. O-IntonationPad (TuningEngine-composing — multi-sub-voice variation)"
    - plugins/O-IntonationPad/Source/DSP/WavetableVoice.h around line 44 (class declaration) and line 144 (`tuningEnginePtr` member — alongside which we add `pendingTuningSource`)
    - plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp lines 43-53 (`resolveFrequency` definition) and lines 100-220 (startNote body)
    - plugins/O-IntonationPad/Source/PluginProcessor.cpp lines 365-368 (addVoice loop), line 511 (processBlock top after buffer.clear at line 516), line 624-628 (setChordGenerationParams call site for reference)
  </read_first>
  <action>
    Invoke `/improve O-IntonationPad` with locked specification.

    ### Edit 1 — `plugins/O-IntonationPad/CMakeLists.txt`

    Bump version (line 9): `PLUGIN_VERSION "2.7.2"` → `PLUGIN_VERSION "2.8.0"`.

    Add module call after line 33 (end of `target_sources`) — anywhere AFTER `juce_add_plugin` at line 5:
    ```cmake
    # Phase 24: VST3 Note Expression microtonal support (Dorico)
    ouaricon_add_module(O-IntonationPad note-expression)
    ```

    ### Edit 2 — `plugins/O-IntonationPad/Source/PluginProcessor.h`

    Add `#include "NoteExpression.h"` after line 19 (last DSP/...h include). Add public override `getVST3ClientExtensions()` returning `&vst3Extensions`. Add private member after `juce::Synthesiser synthesiser;` (line 104):
    ```cpp
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    ```

    ### Edit 3 — `plugins/O-IntonationPad/Source/PluginProcessor.cpp`

    Replace the existing addVoice loop (lines 365-368) with **static wiring at construction** (D-06 — wire ONCE; voice holds pointer for life since `vst3Extensions` outlives all voices, matching O-Lyrica's pattern):
    ```cpp
    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new WavetableVoice();
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE (static wiring)
        synthesiser.addVoice(voice);
    }
    ```

    Top of `processBlock` at line 511, AFTER `buffer.clear()` at line 516:
    ```cpp
    vst3Extensions.drainAndUpdate();
    ```

    Note: leave `setChordGenerationParams(...)` per-block call at lines 624-628 unchanged — it sets `tuningEnginePtr` per-block, orthogonal to NE wiring.

    ### Edit 4 — `plugins/O-IntonationPad/Source/DSP/WavetableVoice.h`

    Add `#include "NoteExpression.h"` after line 34 (`ChordGenerator.h`). Add public setter:
    ```cpp
    void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
    {
        pendingTuningSource = source;
    }
    ```

    Add private member alongside `tuningEnginePtr` at line 144:
    ```cpp
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    ```

    ### Edit 5 — `plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp` — neRatio derivation + sub-voice propagation

    **5a. Insert neRatio derivation at top of `startNote` body** — after `gainSmoothCoeff` calculation (around line 102) and BEFORE the `chordGeneratorPtr->generateChord(...)` block at line 109:
    ```cpp
    // VST3 Note Expression: derive a multiplicative delta from the noteOn MIDI pitch
    // and propagate to every sub-voice frequency via resolveFrequency * neRatio.
    // exchange(0.0) inside the helper consumes the slot — retriggered notes don't
    // inherit stale offsets. Sub-voice octave shifts inherit the tuned root, so chord
    // intervals remain musically correct relative to Dorico's microtonal root pitch.
    double neRatio = 1.0;
    if (pendingTuningSource != nullptr)
        neRatio = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, 1.0);
    ```

    **5b. Propagate `neRatio` into each `resolveFrequency` call site.** The three call sites at lines 134, 142, 150 all have shape:
    ```cpp
    float baseFreq = static_cast<float>(resolveFrequency(baseMidiNote, centOffset));
    ```
    Change each to:
    ```cpp
    float baseFreq = static_cast<float>(resolveFrequency(baseMidiNote, centOffset) * neRatio);
    ```
    (Apply identically to `spacingFreq` at line 142 and `inversionFreq` at line 150 — naming may differ; confirm via Read pass before editing.)

    > **Pattern 2 satisfied:** `neRatio` derived BEFORE any sub-voice frequency assignment; first sample of every sub-voice is at the tuned ratio. No attack zipper.
    > **Pattern 1 satisfied:** NE correlated by `noteId` inside `updatePendingFromEvents`; only the slot for the noteOn MIDI pitch (`midiNoteNumber`) carries the offset. A different `midiNoteNumber` voice consumes a different (or empty) slot.

    ### Edit 6 — `plugins/O-IntonationPad/CHANGELOG.md`

    Insert at top (above current `## [2.7.2]`). Bracketed style:
    ```markdown
    ## [2.8.0] - 2026-04-25

    ### Added

    - **Adds VST3 Note Expression microtonal support for Dorico** (per O-Lyrica 2.3.0 reference shape). End users must set Microtonality to "VST3 Note Expression" on the assigned Dorico expression map.
    - **Shared `note-expression` module adoption** (`modules/tuning/note-expression` v1.0.0).

    ### Technical notes

    - **Composition with TuningEngine + chord generator.** `WavetableVoice::startNote` derives a multiplicative NE delta from the noteOn MIDI pitch via `applyPendingTuning(table, midi, 1.0)` and applies it to every sub-voice's `resolveFrequency` result. NE deltas correlate to the original noteOn pitch (Pattern 1: noteId, not pitch); sub-voice octave shifts inherit the tuned root.
    - **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/DSP/WavetableVoice.{h,cpp}`, `CMakeLists.txt`.
    - **Version bump rationale:** MINOR (2.7.2 → 2.8.0) — new user-visible feature.
    ```

    Phrase `adds VST3 Note Expression microtonal support for Dorico` MUST appear (TRACK-03).

    ### Edits 7, 8

    /improve handles STATUS.md + registry.yaml.

    ### /improve invocation

    ```
    /improve O-IntonationPad
    ```
    Description: "Adds VST3 Note Expression microtonal support for Dorico via shared note-expression module adoption with multi-sub-voice neRatio propagation (Phase 24, plan 24-04)."
  </action>
  <verify>
    <automated>
      grep -E 'ouaricon_add_module\(O-IntonationPad note-expression\)' plugins/O-IntonationPad/CMakeLists.txt && \
      grep -E 'PLUGIN_VERSION "2\.8\.0"' plugins/O-IntonationPad/CMakeLists.txt && \
      grep -E 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-IntonationPad/Source/PluginProcessor.h && \
      grep -E 'vst3Extensions\.drainAndUpdate\(' plugins/O-IntonationPad/Source/PluginProcessor.cpp && \
      grep -E 'setPendingTuningSource\(&vst3Extensions' plugins/O-IntonationPad/Source/PluginProcessor.cpp && \
      grep -E 'Ouaricon::NoteExpression::applyPendingTuning\(.*midiNoteNumber.*1\.0' plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp && \
      grep -E 'neRatio' plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp && \
      grep -F 'adds VST3 Note Expression microtonal support for Dorico' plugins/O-IntonationPad/CHANGELOG.md && \
      grep -E 'plugin: O-IntonationPad' modules/registry.yaml
    </automated>
  </verify>
  <acceptance_criteria>
    - CMakeLists has `PLUGIN_VERSION "2.8.0"` AND `ouaricon_add_module(O-IntonationPad note-expression)`.
    - PluginProcessor swap landed.
    - WavetableVoice.h has setter + member.
    - WavetableVoice.cpp has `neRatio = applyPendingTuning(*pendingTuningSource, midiNoteNumber, 1.0)` derivation BEFORE chordGeneratorPtr block; each `resolveFrequency` call result multiplied by `neRatio`.
    - CHANGELOG top is `[2.8.0]` with TRACK-03 phrase.
    - registry has `plugin: O-IntonationPad`, `version: 2.8.0`.
    - Tri-format build clean; freshly installed; auval validates; one atomic commit.
  </acceptance_criteria>
  <done>/improve cycle complete.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Build-side gate (D-08)</name>
  <read_first>
    - scripts/verify-au-link.sh
    - plugins/O-IntonationPad/CMakeLists.txt
  </read_first>
  <action>
    Same shape as plan 24-01 Task 3, substituting `O-IntonationPad`.

    1. `ninja -C build O-IntonationPad_VST3 O-IntonationPad_AU O-IntonationPad_Standalone 2>&1 | tee /tmp/o-intonationpad-build.log` — exit 0; no Steinberg undefined symbols.
    2. mtime check on `~/Library/Audio/Plug-Ins/VST3/O-IntonationPad.vst3` and `~/Library/Audio/Plug-Ins/Components/O-IntonationPad.component`.
    3. `bash scripts/verify-au-link.sh O-IntonationPad` exits 0.
    4. `auval -a | grep -i 'O.IntonationPad'` returns ≥1 line.
  </action>
  <verify>
    <automated>! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-intonationpad-build.log 2>/dev/null && test -d ~/Library/Audio/Plug-Ins/VST3/O-IntonationPad.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/O-IntonationPad.component && bash scripts/verify-au-link.sh O-IntonationPad && auval -a 2>/dev/null | grep -i 'O.IntonationPad'</automated>
  </verify>
  <acceptance_criteria>
    - Build clean; bundles installed; auval validates.
  </acceptance_criteria>
  <done>Build-side gate PASS.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico 3-point smoke gate (D-07) — multi-sub-voice variant</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    O-IntonationPad v2.8.0 with VST3 Note Expression — multi-sub-voice neRatio propagation. The 12 sub-voices spawned by ChordGenerator now inherit the tuned root. Tri-format built; freshly installed; AU validated.
  </what-built>
  <how-to-verify>
    Same procedure as plan 24-01, with one variation note for sub-voice behavior:

    **Gate point 1:** quarter-sharp C4 → +50¢ above C4 detected on the root pitch. With chord generator active (ChordGenerator default settings produce a layered chord), the entire 12-sub-voice cluster should be transposed +50¢ relative to the natural-C4 reference. Listen for the cluster's overall pitch center, or use a tuner on the root sub-voice. PASS / FAIL with observed root Hz.
    **Gate point 2:** no attack zipper — the sub-voice cluster's pitch is correct from sample 0 (neRatio derived BEFORE the chordGeneratorPtr block). PASS / FAIL.
    **Gate point 3:** chord polyphony — notate quarter-sharp C4 and natural E4 in Dorico (this triggers two simultaneous WavetableVoice instances, each with their own 12-sub-voice cluster). The C4 cluster center is +50¢; the E4 cluster center is at natural 12-TET (329.63 Hz root). PASS / FAIL.

    > **Sub-voice intervals preserved:** ChordGenerator's voicing intervals are relative ratios; the multiplicative `neRatio` rescales the entire cluster while preserving internal interval ratios. Verify by ear that chord quality is preserved (e.g., a major 7 chord still sounds major, just shifted ±50¢).

    Record in `24-04-O-IntonationPad-SUMMARY.md`. Type `approved` or describe failure.
  </how-to-verify>
  <resume-signal>Type `approved` if 3-point gate PASS; else describe failure with observed Hz on root sub-voice.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Write 24-04-O-IntonationPad-SUMMARY.md and close plan</name>
  <read_first>
    - .planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md (format reference)
    - $HOME/.claude/get-shit-done/templates/summary.md
  </read_first>
  <action>
    Mirror prior SUMMARY structure. Document PROP-02 + TRACK-01..05; flag the `neRatio` propagation pattern (structural variation worth recording for future multi-sub-voice consumers — could become a documented module-level pattern in Phase 25 DOCS-01 work); record 3-point gate observations including chord quality preservation; "feeds 24-08-final-sweep-SUMMARY.md row 4 of 8".
  </action>
  <verify>
    <automated>test -f .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md && grep -E 'PROP-02' .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md && grep -iE 'PASS|FAIL' .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md && grep -E 'neRatio' .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md</automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY references PROP-02 + 5 TRACK reqs; 3-point gate result recorded; `neRatio` pattern documented.
  </acceptance_criteria>
  <done>Plan 24-04 closed; ready for 24-05.</done>
</task>

</tasks>

<verification>
Per 24-INTEGRATION-MATRIX.md template applied to O-IntonationPad v2.8.0. Additional structural check: `neRatio` derivation present BEFORE chord-generator invocation; `resolveFrequency` results multiplied by `neRatio` at all three call sites.
</verification>

<success_criteria>
PROP-02 + TRACK-01..05 satisfied; atomic commit landed; SUMMARY complete; multi-sub-voice neRatio pattern validated.
</success_criteria>

<output>
- `.planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md`
- Updated O-IntonationPad sources (with neRatio propagation) + CHANGELOG + STATUS + registry
- Freshly installed bundles
- One atomic commit
</output>
