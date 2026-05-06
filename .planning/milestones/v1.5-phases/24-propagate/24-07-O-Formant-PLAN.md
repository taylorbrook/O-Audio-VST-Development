---
phase: 24-propagate
plan: 07
type: execute
wave: 7
depends_on: [24-06]
files_modified:
  - plugins/O-Formant/CMakeLists.txt
  - plugins/O-Formant/Source/PluginProcessor.h
  - plugins/O-Formant/Source/PluginProcessor.cpp
  - plugins/O-Formant/Source/FormantVoice.h
  - plugins/O-Formant/Source/FormantVoice.cpp
  - plugins/O-Formant/CHANGELOG.md
  - plugins/O-Formant/.planning/STATUS.md
  - modules/registry.yaml
autonomous: false
requirements: [PROP-07, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05]

must_haves:
  truths:
    - "O-Formant responds to Dorico VST3 NE — pitch lands +50¢ above C4 for quarter-sharp C4 in MPE mode."
    - "Glottal source fundamental (`tunedF0`) sees the tuned frequency on sample 0 — composition order applies NE BEFORE `pitchGlide.setTarget(f0)` (line 196) / `pitchGlide.snapTo(f0)` (line 198) / `glottalSource.setFrequency(finalF0)` (line 650)."
    - "ConsonantEngine articulation independent of pitch — formants intelligible at +50¢ shift."
    - "Chord polyphony correlated by `noteId` (MPE)."
    - "Tri-format build clean. Pre-step CMake delta: `OuariconModules.cmake` include ADDED. TRACK-01..05: /improve ran; version 1.24.2 → 1.25.0; CHANGELOG TRACK-03; STATUS updated; freshly installed; registry."
  artifacts:
    - path: "plugins/O-Formant/CMakeLists.txt"
      provides: "OuariconModules.cmake include ADDED + module call + minor bump"
      contains: "ouaricon_add_module(O-Formant note-expression)"
    - path: "plugins/O-Formant/CMakeLists.txt"
      provides: "OuariconModules.cmake include line"
      contains: "include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)"
    - path: "plugins/O-Formant/Source/PluginProcessor.h"
      provides: "VST3Extensions member"
      contains: "Ouaricon::NoteExpression::VST3Extensions vst3Extensions"
    - path: "plugins/O-Formant/Source/FormantVoice.cpp"
      provides: "Composition site — tunedF0 NE delta before pitchGlide"
      contains: "Ouaricon::NoteExpression::applyPendingTuning("
    - path: "plugins/O-Formant/CHANGELOG.md"
      provides: "TRACK-03 entry"
      contains: "adds VST3 Note Expression microtonal support for Dorico"
    - path: "modules/registry.yaml"
      provides: "Consumer registration"
      contains: "plugin: O-Formant"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3"
      provides: "Freshly installed VST3"
    - path: "~/Library/Audio/Plug-Ins/Components/O-Formant.component"
      provides: "Freshly installed AU"
    - path: ".planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md"
      provides: "Plan close-out"
  key_links:
    - from: "FormantVoice.cpp::noteStarted"
      to: "applyPendingTuning"
      via: "after tunedF0 assignment at lines 187-191, BEFORE pitchGlide.setTarget(f0)/snapTo(f0)"
      pattern: "applyPendingTuning\\(.*pendingTuningSource.*midiNote"
    - from: "PluginProcessor.cpp processBlock"
      to: "vst3Extensions.drainAndUpdate()"
      via: "top of processBlock at line 742"
      pattern: "vst3Extensions\\.drainAndUpdate\\("
---

<objective>
Propagate `note-expression` into **O-Formant** (Plan 24-07 — seventh and final per-plugin wave). MPE plugin (`juce::MPESynthesiser`); pitched fundamental drives `LFGlottalSource`. Per D-11 placed last because of the largest CMake delta:

**Pre-step CMake delta:** O-Formant currently does NOT include `OuariconModules.cmake`. The include must be added at the top of CMakeLists.txt BEFORE `juce_add_plugin` so the macro `ouaricon_add_module` is in scope.

**Voice-side composition:** at `FormantVoice.cpp:187-191`, `tunedF0` is assigned from `tuningEnginePtr->getFrequency(midi)` cast to `float`. Insert NE delta IMMEDIATELY AFTER, BEFORE `pitchGlide.setTarget(f0)` (line 196) / `pitchGlide.snapTo(f0)` (line 198), so the glottal source samples the correct fundamental from sample 0. Re-read `f0 = tunedF0` after NE composition (since `f0` was a local copy before the NE step). `tunedF0` is referenced downstream at lines 488, 616 for spectral tilt and source-filter coupling — applying NE pre-glide ensures all downstream consumers see the tuned value.

Float→double cast at helper boundary (`tunedF0` is `float`). MPE composition uses Pattern 3-style `noteStarted()` reading `currentlyPlayingNote.initialNote`.

Purpose: PROP-07 + TRACK-01..05 via `/improve O-Formant`. Version 1.24.2 → 1.25.0 (minor; baseline from CMake `VERSION 1.24.2` at line 10).

Output: Source edits + new `OuariconModules.cmake` include + module call + minor version bump + CHANGELOG + STATUS + registry + bundles + commit + SUMMARY.
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
@.planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@CLAUDE.md
@plugins/O-Lyrica/CMakeLists.txt
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp
@plugins/O-Formant/CMakeLists.txt
@plugins/O-Formant/Source/PluginProcessor.h
@plugins/O-Formant/Source/PluginProcessor.cpp
@plugins/O-Formant/Source/FormantVoice.h
@plugins/O-Formant/Source/FormantVoice.cpp
@plugins/O-Formant/Source/dsp/ConsonantEngine.h
@plugins/O-Formant/CHANGELOG.md
@plugins/O-Formant/.planning/STATUS.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/plugin-improve/SKILL.md
@scripts/verify-au-link.sh

<integrations>
[O-Formant row in 24-INTEGRATION-MATRIX.md](./24-INTEGRATION-MATRIX.md#o-formant)

**Match quality:** STRUCTURAL (MPE) — same MPE pattern as O-Reed/O-Bowed but voice does NOT have a `getBaseFrequencyFromTuning` helper. Composition applied directly at the `tunedF0` assignment site.

**Critical CMake delta:** O-Formant lacks `include(...OuariconModules.cmake)`. Without this, `ouaricon_add_module` is undefined and CMake configure fails. This is a one-line addition; pre-step the only structural delta vs prior plans.

**Why this slot:** placed at end of D-11 ordering specifically because the CMake structural delta is unique to O-Formant. By plan 24-07, the macro pattern is fully proven (`ouaricon_add_module` worked end-to-end across 6 prior plugins) so the include addition is safe.
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight — confirm O-Formant preconditions and CMake delta</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md ([O-Formant row](./24-INTEGRATION-MATRIX.md#o-formant))
    - .planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md
    - plugins/O-Formant/.planning/STATUS.md
    - plugins/O-Formant/CMakeLists.txt (CRITICAL: verify `OuariconModules.cmake` is NOT included; line 10 `VERSION 1.24.2`; line 4 juce_add_plugin)
    - plugins/O-Formant/CHANGELOG.md (verify top `## [1.24.2]`)
    - plugins/O-Formant/Source/FormantVoice.cpp lines 187-200 (tunedF0 assignment + pitchGlide calls)
  </read_first>
  <action>
    1. STATUS not 🚧.
    2. **CRITICAL:** Confirm `OuariconModules.cmake` is NOT currently included in `plugins/O-Formant/CMakeLists.txt`:
       ```
       grep -c 'OuariconModules.cmake' plugins/O-Formant/CMakeLists.txt
       ```
       MUST return 0. (If non-zero, the assumption from PATTERNS.md is wrong and the plan needs re-scoping.)
    3. Confirm `VERSION 1.24.2` at line 10.
    4. Confirm CHANGELOG top is `## [1.24.2]`.
    5. Confirm `tunedF0` assignment at FormantVoice.cpp lines 187-191 and `pitchGlide.setTarget(f0)`/`snapTo(f0)` at lines 196-198.
    6. JUCE patch markers; tree clean.
  </action>
  <verify>
    <automated>test "$(grep -c 'OuariconModules.cmake' plugins/O-Formant/CMakeLists.txt)" = "0" && grep -E 'VERSION 1\.24\.2' plugins/O-Formant/CMakeLists.txt && grep -E '^## \[1\.24\.2\]' plugins/O-Formant/CHANGELOG.md && grep -nE 'tunedF0' plugins/O-Formant/Source/FormantVoice.cpp | head -3 && grep -l 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h && git diff --quiet -- plugins/O-Formant modules/registry.yaml</automated>
  </verify>
  <acceptance_criteria>
    - `OuariconModules.cmake` NOT currently included in O-Formant's CMakeLists (CMake delta confirmed).
    - `VERSION 1.24.2` baseline; CHANGELOG top `[1.24.2]`.
    - `tunedF0` assignment site confirmed.
    - JUCE patch markers; tree clean.
  </acceptance_criteria>
  <done>Preconditions PASS; CMake delta confirmed.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Run /improve O-Formant with locked specification (includes OuariconModules.cmake pre-step)</name>
  <read_first>
    - .planning/phases/24-propagate/24-PATTERNS.md §"7. O-Formant (MPESynthesiserVoice — pitched fundamental drives glottal source)"
    - plugins/O-Lyrica/CMakeLists.txt:1-10 (reference shape for the OuariconModules.cmake include)
    - plugins/O-Formant/CMakeLists.txt lines 1-15 (cmake_minimum_required + juce_add_plugin head; identify exact insert location)
    - plugins/O-Formant/Source/FormantVoice.h around line 103 (`tuningEnginePtr` member; line 104 `tunedF0` cache)
    - plugins/O-Formant/Source/FormantVoice.cpp lines 132-200 (noteStarted body; tunedF0 at 187-191; pitchGlide at 196-198)
    - plugins/O-Formant/Source/PluginProcessor.cpp lines 690-705 (addVoice loop), line 742 (processBlock top)
    - plugins/O-Formant/Source/PluginProcessor.h around line 81 (`juce::MPESynthesiser synthesiser;`); line 58 (`TuningEngine tuningEngine;` PUBLIC member); line 63 (`getLyricsEngine()` accessor location)
  </read_first>
  <action>
    Invoke `/improve O-Formant` with locked specification.

    ### Edit 1 — `plugins/O-Formant/CMakeLists.txt`

    **PRE-STEP (CMake delta unique to O-Formant):** Insert the OuariconModules.cmake include IMMEDIATELY AFTER `cmake_minimum_required(VERSION 3.15)` at line 1, and BEFORE `juce_add_plugin` at line 4:
    ```cmake
    include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
    ```

    Bump version (line 10): `VERSION 1.24.2` → `VERSION 1.25.0`.

    Add module call AFTER line 34 (end of `target_sources` block) — `juce_add_plugin` already finished by then so per-format subtargets exist:
    ```cmake
    # Phase 24: VST3 Note Expression microtonal support (Dorico)
    ouaricon_add_module(O-Formant note-expression)
    ```

    ### Edit 2 — `plugins/O-Formant/Source/PluginProcessor.h`

    Add `#include "NoteExpression.h"` after line 23 (last include before guards):
    ```cpp
    #include "NoteExpression.h"  // modules/tuning/note-expression
    ```

    Add public override near `getLyricsEngine()` accessor (around line 63):
    ```cpp
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
    ```

    Add private member after `juce::MPESynthesiser synthesiser;` (line 81):
    ```cpp
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    ```

    ### Edit 3 — `plugins/O-Formant/Source/PluginProcessor.cpp`

    addVoice loop (lines 694-699):
    ```cpp
    auto* voice = new FormantVoice (i);
    voice->setAPVTS (&parameters);
    voice->setWavetable (&glottalWavetable);
    voice->setTuningEngine (&tuningEngine);
    voice->setLyricsEngine (&lyricsEngine);
    voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
    synthesiser.addVoice (voice);
    ```

    Top of `processBlock` at line 742 after `buffer.clear()`:
    ```cpp
    vst3Extensions.drainAndUpdate();
    ```

    ### Edit 4 — `plugins/O-Formant/Source/FormantVoice.h`

    Add `#include "NoteExpression.h"`. Public setter `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable*)`. Private member alongside `tuningEnginePtr` (around line 103):
    ```cpp
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    ```

    ### Edit 5 — `plugins/O-Formant/Source/FormantVoice.cpp` — composition site

    Existing assignment (lines 187-191) is UNCHANGED:
    ```cpp
    int midiNote = currentlyPlayingNote.initialNote;
    tunedF0 = tuningEnginePtr != nullptr
        ? static_cast<float> (tuningEnginePtr->getFrequency (midiNote))
        : static_cast<float> (getCurrentlyPlayingNote().getFrequencyInHertz());
    float f0 = tunedF0;
    ```

    Insert IMMEDIATELY AFTER line 191 (`float f0 = tunedF0;`), BEFORE `pitchGlide.setTarget(f0)` at line 196 / `pitchGlide.snapTo(f0)` at line 198:
    ```cpp
    // VST3 Note Expression tuning delta (Dorico microtonal).
    // Apply BEFORE pitchGlide so the glottal source samples the correct fundamental
    // from sample 0 (Pattern 2 — no attack zipper). tunedF0 is referenced downstream
    // at lines 488, 616 for spectral tilt and source-filter coupling — all consumers
    // see the tuned value.
    if (pendingTuningSource != nullptr)
    {
        tunedF0 = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
            *pendingTuningSource, midiNote, static_cast<double>(tunedF0)));
        f0 = tunedF0;  // re-read after NE composition
    }
    ```

    > **Cast through `double`**: `tunedF0` is `float`; helper signature is `double(...)` (matches O-Bells/O-Wind pattern).
    > **`f0` re-read** required because `f0` was set from `tunedF0` BEFORE the NE step. Without re-read, `pitchGlide` would target the untuned value.

    ### Edit 6 — `plugins/O-Formant/CHANGELOG.md`

    Insert at top (above `## [1.24.2]`). Bracketed style:
    ```markdown
    ## [1.25.0] - 2026-04-25

    ### Added — VST3 Note Expression Microtonal Support for Dorico

    Adds VST3 Note Expression microtonal support for Dorico. O-Formant responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events). The voice's cached `tunedF0` composes Dorico's NE delta multiplicatively after `TuningEngine::getFrequency` and before `PitchGlide` / glottal source frequency assignment. MPE pitch-bend stacks on top via `getCurrentlyPlayingNote().getFrequencyInHertz()` (per-sample lookup unaffected by NE). End users must set Microtonality to "VST3 Note Expression" on the Dorico expression map.

    **Files Modified:** `CMakeLists.txt` (added `include(OuariconModules.cmake)` + `ouaricon_add_module`), `Source/PluginProcessor.{h,cpp}`, `Source/FormantVoice.{h,cpp}`. Version: 1.24.2 → 1.25.0.
    ```

    Phrase `adds VST3 Note Expression microtonal support for Dorico` MUST appear (TRACK-03).

    ### Edits 7, 8

    /improve handles STATUS.md + registry.yaml.

    ### /improve invocation

    ```
    /improve O-Formant
    ```
    Description: "Adds VST3 Note Expression microtonal support for Dorico via shared note-expression module — third MPE plugin, includes adding OuariconModules.cmake include (Phase 24, plan 24-07)."
  </action>
  <verify>
    <automated>
      grep -E 'include\(\$\{CMAKE_SOURCE_DIR\}/modules/cmake/OuariconModules\.cmake\)' plugins/O-Formant/CMakeLists.txt && \
      grep -E 'ouaricon_add_module\(O-Formant note-expression\)' plugins/O-Formant/CMakeLists.txt && \
      grep -E 'VERSION 1\.25\.0' plugins/O-Formant/CMakeLists.txt && \
      grep -E 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-Formant/Source/PluginProcessor.h && \
      grep -E 'vst3Extensions\.drainAndUpdate\(' plugins/O-Formant/Source/PluginProcessor.cpp && \
      grep -E 'setPendingTuningSource\(&vst3Extensions' plugins/O-Formant/Source/PluginProcessor.cpp && \
      grep -E 'Ouaricon::NoteExpression::applyPendingTuning\(.*midiNote' plugins/O-Formant/Source/FormantVoice.cpp && \
      grep -F 'adds VST3 Note Expression microtonal support for Dorico' plugins/O-Formant/CHANGELOG.md && \
      grep -E 'plugin: O-Formant' modules/registry.yaml
    </automated>
  </verify>
  <acceptance_criteria>
    - `plugins/O-Formant/CMakeLists.txt` NOW INCLUDES `OuariconModules.cmake` (was missing before this plan).
    - CMakeLists has `VERSION 1.25.0` AND `ouaricon_add_module(O-Formant note-expression)`.
    - PluginProcessor swap landed.
    - FormantVoice.h has setter + member.
    - FormantVoice.cpp `tunedF0` is updated via `applyPendingTuning` AFTER lines 187-191 assignment AND BEFORE line 196 `pitchGlide.setTarget`; `f0 = tunedF0` re-read present.
    - CHANGELOG top is `## [1.25.0] - 2026-04-25` with TRACK-03 phrase.
    - registry has `plugin: O-Formant`, `version: 1.25.0`.
    - Tri-format build clean; freshly installed; auval validates; one atomic commit.
  </acceptance_criteria>
  <done>/improve cycle complete; final per-plugin propagation done.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Build-side gate (D-08)</name>
  <read_first>
    - scripts/verify-au-link.sh
    - plugins/O-Formant/CMakeLists.txt
  </read_first>
  <action>
    Same shape as plan 24-01 Task 3, substituting `O-Formant`.

    1. `ninja -C build O-Formant_VST3 O-Formant_AU O-Formant_Standalone 2>&1 | tee /tmp/o-formant-build.log` — exit 0; no Steinberg undefined symbols.
    2. mtime check on installed bundles.
    3. `bash scripts/verify-au-link.sh O-Formant` exits 0.
    4. `auval -a | grep -i 'O.Formant'` returns ≥1 line.
  </action>
  <verify>
    <automated>! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-formant-build.log 2>/dev/null && test -d ~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/O-Formant.component && bash scripts/verify-au-link.sh O-Formant && auval -a 2>/dev/null | grep -i 'O.Formant'</automated>
  </verify>
  <acceptance_criteria>
    - Build clean; bundles installed; auval validates.
  </acceptance_criteria>
  <done>Build-side gate PASS.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico 3-point smoke gate (D-07) — third MPE plugin, formant focus</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    O-Formant v1.25.0 with VST3 Note Expression — third MPE-based propagation. Glottal-source fundamental `tunedF0` now sees the NE delta on sample 0; ConsonantEngine articulation should remain intelligible at +50¢ shift. Tri-format built (with new OuariconModules.cmake include); freshly installed; AU validated.
  </what-built>
  <how-to-verify>
    Same procedure as plan 24-05/24-06 with formant-specific notes.

    **Setup variant:** O-Formant is a vocal/formant synth — choose a vowel preset for clearer pitch perception (the formant articulation can mask fundamental if the spectral envelope is dominant).

    **Gate point 1:** quarter-sharp C4 → +50¢ above C4 fundamental (~269.29 Hz). PASS / FAIL with observed Hz.
    **Gate point 2:** no attack zipper — glottal source `tunedF0` correct from sample 0. Important here because pitchGlide can mask zipper if NE is applied AFTER snapTo. PASS / FAIL.
    **Gate point 3:** chord (quarter-sharp C4 + natural E4) — only C4 voice detuned. With formant articulation, you may notice the "vowel center" of each voice is correct — the formant frequencies (defined relative to the fundamental in some modes) shouldn't shift unexpectedly. PASS / FAIL.

    **Bonus check (formant-specific, not gating):** confirm consonant articulation intelligibility at +50¢ — vowel transitions should feel natural, not "off" timbrally. ConsonantEngine is independent of fundamental pitch (separate from `tunedF0`); record observation in SUMMARY.

    Record in `24-07-O-Formant-SUMMARY.md`. Type `approved` or describe failure.
  </how-to-verify>
  <resume-signal>Type `approved` if 3-point gate PASS; else describe failure with observed Hz.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Write 24-07-O-Formant-SUMMARY.md and close plan</name>
  <read_first>
    - .planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md
    - $HOME/.claude/get-shit-done/templates/summary.md
  </read_first>
  <action>
    Mirror prior SUMMARY structure. Document PROP-07 + TRACK-01..05; flag the OuariconModules.cmake include addition (the unique CMake delta among Phase 24 plugins); record 3-point gate + formant-specific observations; "feeds 24-08-final-sweep-SUMMARY.md row 7 of 8". This SUMMARY closes the per-plugin propagation phase — note the readiness for plan 24-08 final sweep.
  </action>
  <verify>
    <automated>test -f .planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md && grep -E 'PROP-07' .planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md && grep -iE 'PASS|FAIL' .planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md && grep -iE 'OuariconModules\.cmake' .planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md</automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY references PROP-07 + 5 TRACK reqs; 3-point gate result; OuariconModules.cmake include addition documented; formant-specific notes.
  </acceptance_criteria>
  <done>Plan 24-07 closed; all 7 per-plugin propagations done; ready for 24-08 final sweep.</done>
</task>

</tasks>

<verification>
Per 24-INTEGRATION-MATRIX.md template applied to O-Formant v1.25.0. Additional structural check: `OuariconModules.cmake` include added to CMakeLists (was missing prior to this plan).
</verification>

<success_criteria>
PROP-07 + TRACK-01..05 satisfied; atomic commit landed; SUMMARY complete; all 7 per-plugin propagations done.
</success_criteria>

<output>
- `.planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md`
- Updated O-Formant CMakeLists (with OuariconModules.cmake include + ouaricon_add_module + version bump), sources, CHANGELOG, STATUS, registry
- Freshly installed bundles
- One atomic commit
</output>
