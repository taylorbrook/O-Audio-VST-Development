---
phase: 24-propagate
plan: 06
type: execute
wave: 6
depends_on: [24-05]
files_modified:
  - plugins/O-Bowed/CMakeLists.txt
  - plugins/O-Bowed/Source/PluginProcessor.h
  - plugins/O-Bowed/Source/PluginProcessor.cpp
  - plugins/O-Bowed/Source/BowedStringVoice.h
  - plugins/O-Bowed/Source/BowedStringVoice.cpp
  - plugins/O-Bowed/CHANGELOG.md
  - plugins/O-Bowed/.planning/STATUS.md
  - modules/registry.yaml
autonomous: false
requirements: [PROP-06, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05]

must_haves:
  truths:
    - "O-Bowed responds to Dorico VST3 NE — pitch lands +50¢ above C4 for quarter-sharp C4 in MPE mode."
    - "MPE composition: NE applied INSIDE `getBaseFrequencyFromTuning` helper at lines 291-296 — covers BOTH `noteStarted()` line 32 AND `notePitchbendChanged()` line 71 → `waveguideString.trigger(currentFrequency)` at lines 39, 76."
    - "Waveguide string period sized to tuned frequency on sample 0 (no attack zipper)."
    - "Chord polyphony correlated by `noteId`."
    - "Tri-format build clean. TRACK-01..05: /improve ran; PLUGIN_VERSION line ADDED at 1.3.0; CHANGELOG TRACK-03; STATUS updated; freshly installed; registry."
  artifacts:
    - path: "plugins/O-Bowed/CMakeLists.txt"
      provides: "Module + NEW PLUGIN_VERSION line + minor bump"
      contains: "ouaricon_add_module(O-Bowed note-expression)"
    - path: "plugins/O-Bowed/CMakeLists.txt"
      provides: "PLUGIN_VERSION line"
      contains: 'PLUGIN_VERSION "1.3.0"'
    - path: "plugins/O-Bowed/Source/PluginProcessor.h"
      provides: "VST3Extensions member"
      contains: "Ouaricon::NoteExpression::VST3Extensions vst3Extensions"
    - path: "plugins/O-Bowed/Source/BowedStringVoice.cpp"
      provides: "NE inside getBaseFrequencyFromTuning helper"
      contains: "Ouaricon::NoteExpression::applyPendingTuning("
    - path: "plugins/O-Bowed/CHANGELOG.md"
      provides: "TRACK-03 entry"
      contains: "adds VST3 Note Expression microtonal support for Dorico"
    - path: "modules/registry.yaml"
      provides: "Consumer registration"
      contains: "plugin: O-Bowed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Bowed.vst3"
      provides: "Freshly installed VST3"
    - path: "~/Library/Audio/Plug-Ins/Components/O-Bowed.component"
      provides: "Freshly installed AU"
    - path: ".planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md"
      provides: "Plan close-out"
  key_links:
    - from: "BowedStringVoice.cpp::getBaseFrequencyFromTuning helper (lines 291-296)"
      to: "applyPendingTuning"
      via: "INSIDE the helper — covers noteStarted() line 32 + notePitchbendChanged() line 71 + waveguideString.trigger at lines 39, 76"
      pattern: "applyPendingTuning\\(.*pendingTuningSource.*midiNote"
---

<objective>
Propagate `note-expression` into **O-Bowed** (Plan 24-06 — sixth wave). SECOND MPE plugin; same shape as O-Reed (helper-based composition). `BowedStringVoice : public juce::MPESynthesiserVoice` via `BowedMPESynthesiser`. Helper at lines 291-296 covers `noteStarted()` (line 25) AND `notePitchbendChanged()` (line 65) — both call `getBaseFrequencyFromTuning` then `waveguideString.trigger(currentFrequency)` (lines 39, 76).

CMake delta: PLUGIN_VERSION arg MISSING from `juce_add_plugin(O-Bowed ...)` — must be added.

Purpose: PROP-06 + TRACK-01..05 via `/improve O-Bowed`. Version 1.2.1 → 1.3.0 (minor; baseline from CHANGELOG top entry `[1.2.1]`).

Output: Source edits + new PLUGIN_VERSION line + CHANGELOG + STATUS + registry + bundles + commit + SUMMARY.
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
@.planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@CLAUDE.md
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp
@plugins/O-Bowed/CMakeLists.txt
@plugins/O-Bowed/Source/PluginProcessor.h
@plugins/O-Bowed/Source/PluginProcessor.cpp
@plugins/O-Bowed/Source/BowedStringVoice.h
@plugins/O-Bowed/Source/BowedStringVoice.cpp
@plugins/O-Bowed/Source/BoreWaveguide.h
@plugins/O-Bowed/CHANGELOG.md
@plugins/O-Bowed/.planning/STATUS.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/plugin-improve/SKILL.md
@scripts/verify-au-link.sh

<integrations>
[O-Bowed row in 24-INTEGRATION-MATRIX.md](./24-INTEGRATION-MATRIX.md#o-bowed)

**Match quality:** STRUCTURAL (MPE) — same as O-Reed. Helper-based consolidation already validated by plan 24-05; same pattern repeats here.

**Synthesiser variant:** `BowedMPESynthesiser` (subclass of `juce::MPESynthesiser`). addVoice + processBlock unchanged from canary. The voice receives `setVoiceIndex(i)` and `setHumanizeEngine(&humanizeEngine)` — preserve those calls; insert NE wiring alongside.
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight — confirm O-Bowed preconditions</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md ([O-Bowed row](./24-INTEGRATION-MATRIX.md#o-bowed))
    - .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md
    - plugins/O-Bowed/.planning/STATUS.md
    - plugins/O-Bowed/CMakeLists.txt (line 3 OuariconModules.cmake; line 6 juce_add_plugin; verify NO PLUGIN_VERSION inside)
    - plugins/O-Bowed/CHANGELOG.md (verify top `## [1.2.1]`)
    - plugins/O-Bowed/Source/BowedStringVoice.cpp lines 25-77 (`noteStarted` + `notePitchbendChanged`) AND lines 291-296 (`getBaseFrequencyFromTuning` helper)
  </read_first>
  <action>
    1. STATUS not 🚧.
    2. Confirm `juce_add_plugin(O-Bowed ...)` lacks PLUGIN_VERSION:
       ```
       awk '/^juce_add_plugin\(O-Bowed/,/^\)/' plugins/O-Bowed/CMakeLists.txt | grep -c PLUGIN_VERSION
       ```
       MUST be 0.
    3. CHANGELOG top is `## [1.2.1]` (1.3.0 minor target).
    4. Confirm `getBaseFrequencyFromTuning(int midiNote) const` exists at BowedStringVoice.cpp:291-296.
    5. JUCE patch markers; tree clean.
  </action>
  <verify>
    <automated>test "$(awk '/^juce_add_plugin\(O-Bowed/,/^\)/' plugins/O-Bowed/CMakeLists.txt | grep -c PLUGIN_VERSION)" = "0" && grep -E '^## \[1\.2\.1\]' plugins/O-Bowed/CHANGELOG.md && grep -nE 'getBaseFrequencyFromTuning' plugins/O-Bowed/Source/BowedStringVoice.cpp | head -3 && grep -E 'include\(\$\{CMAKE_SOURCE_DIR\}/modules/cmake/OuariconModules\.cmake\)' plugins/O-Bowed/CMakeLists.txt && grep -l 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h && git diff --quiet -- plugins/O-Bowed modules/registry.yaml</automated>
  </verify>
  <acceptance_criteria>
    - PLUGIN_VERSION absent; CHANGELOG top `[1.2.1]`; helper exists; tree clean.
  </acceptance_criteria>
  <done>Preconditions PASS.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Run /improve O-Bowed with locked specification</name>
  <read_first>
    - .planning/phases/24-propagate/24-PATTERNS.md §"6. O-Bowed (MPESynthesiserVoice + physical-model waveguide string)"
    - plugins/O-Bowed/CMakeLists.txt lines 6-50 (juce_add_plugin block)
    - plugins/O-Bowed/Source/BowedStringVoice.cpp lines 25-80 (noteStarted + notePitchbendChanged) AND lines 291-296 (helper)
    - plugins/O-Bowed/Source/BowedStringVoice.h (member declarations near `tuningEngine`)
    - plugins/O-Bowed/Source/PluginProcessor.cpp lines 246-250 (addVoice loop), line 288 (processBlock top)
    - plugins/O-Bowed/Source/PluginProcessor.h line 77 (`BowedMPESynthesiser synthesiser;`)
  </read_first>
  <action>
    Invoke `/improve O-Bowed` with locked specification.

    ### Edit 1 — `plugins/O-Bowed/CMakeLists.txt`

    Add PLUGIN_VERSION line inside `juce_add_plugin(O-Bowed ...)` block, BETWEEN `PRODUCT_NAME` and `IS_SYNTH`:
    ```cmake
        PLUGIN_VERSION "1.3.0"
    ```

    Add module call after line 49 (end of `target_sources`):
    ```cmake
    # Phase 24: VST3 Note Expression microtonal support (Dorico)
    ouaricon_add_module(O-Bowed note-expression)
    ```

    ### Edit 2 — `plugins/O-Bowed/Source/PluginProcessor.h`

    Add `#include "NoteExpression.h"`. Public override `getVST3ClientExtensions()`. Private member after `BowedMPESynthesiser synthesiser;` (line 77):
    ```cpp
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    ```

    ### Edit 3 — `plugins/O-Bowed/Source/PluginProcessor.cpp`

    addVoice loop (lines 246-250):
    ```cpp
    auto* voice = new BowedStringVoice (&parameters);
    voice->setVoiceIndex (i);
    voice->setTuningEngine (&tuningEngine);
    voice->setHumanizeEngine (&humanizeEngine);
    voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
    synthesiser.addVoice (voice);
    ```

    Top of `processBlock` at line 288 after `buffer.clear()`:
    ```cpp
    vst3Extensions.drainAndUpdate();
    ```

    ### Edit 4 — `plugins/O-Bowed/Source/BowedStringVoice.h`

    Add `#include "NoteExpression.h"`. Public setter `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable*)`. Private member alongside `tuningEngine`:
    ```cpp
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    ```

    ### Edit 5 — `plugins/O-Bowed/Source/BowedStringVoice.cpp` — modify helper

    **Replace existing helper body (lines 291-296):**
    ```cpp
    float BowedStringVoice::getBaseFrequencyFromTuning (int midiNote) const
    {
        return (tuningEngine != nullptr)
            ? static_cast<float> (tuningEngine->getFrequency (midiNote))
            : static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNote));
    }
    ```

    **With:**
    ```cpp
    float BowedStringVoice::getBaseFrequencyFromTuning (int midiNote) const
    {
        double freq = (tuningEngine != nullptr)
            ? tuningEngine->getFrequency (midiNote)
            : juce::MidiMessage::getMidiNoteInHertz (midiNote);

        // VST3 Note Expression tuning delta (Dorico microtonal).
        // Single source of truth for noteStarted() (line 32) and notePitchbendChanged()
        // (line 71). exchange(0.0) consume — first call tunes; held-note pitch-bend
        // updates return base unchanged (correct: NE applies once per noteStarted).
        if (pendingTuningSource != nullptr)
            freq = Ouaricon::NoteExpression::applyPendingTuning (*pendingTuningSource, midiNote, freq);

        return static_cast<float> (freq);
    }
    ```

    ### Edit 6 — `plugins/O-Bowed/CHANGELOG.md`

    Insert at top (above `## [1.2.1]`). Bracketed style:
    ```markdown
    ## [1.3.0] - 2026-04-25

    ### Added
    - **Adds VST3 Note Expression microtonal support for Dorico.** O-Bowed responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events). Composes with TuningEngine via the `getBaseFrequencyFromTuning` helper before `WaveguideString::trigger(freq)` — string period sized to the tuned frequency on the first sample. MPE pitch-bend stacks multiplicatively.
    - **Shared `note-expression` module adoption** (`modules/tuning/note-expression` v1.0.0).

    ### Technical Notes
    - Files modified: `Source/PluginProcessor.{h,cpp}`, `Source/BowedStringVoice.{h,cpp}`, `CMakeLists.txt` (added PLUGIN_VERSION line + ouaricon_add_module).
    - Helper consolidation: NE applied INSIDE `getBaseFrequencyFromTuning(midiNote)` at lines 291-296 — covers both `noteStarted()` (line 32) and `notePitchbendChanged()` (line 71). exchange(0.0) consume semantics: held-note pitch-bend updates return base unchanged, which is correct (NE applies once per noteStarted).
    - Version bump rationale: MINOR (1.2.1 → 1.3.0).
    ```

    Phrase `adds VST3 Note Expression microtonal support for Dorico` MUST appear (TRACK-03).

    ### Edits 7, 8

    /improve handles STATUS.md + registry.yaml.

    ### /improve invocation

    ```
    /improve O-Bowed
    ```
    Description: "Adds VST3 Note Expression microtonal support for Dorico via shared note-expression module — second MPE plugin (Phase 24, plan 24-06)."
  </action>
  <verify>
    <automated>
      grep -E 'ouaricon_add_module\(O-Bowed note-expression\)' plugins/O-Bowed/CMakeLists.txt && \
      awk '/^juce_add_plugin\(O-Bowed/,/^\)/' plugins/O-Bowed/CMakeLists.txt | grep -E 'PLUGIN_VERSION "1\.3\.0"' && \
      grep -E 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-Bowed/Source/PluginProcessor.h && \
      grep -E 'vst3Extensions\.drainAndUpdate\(' plugins/O-Bowed/Source/PluginProcessor.cpp && \
      grep -E 'setPendingTuningSource\(&vst3Extensions' plugins/O-Bowed/Source/PluginProcessor.cpp && \
      grep -E 'Ouaricon::NoteExpression::applyPendingTuning\(.*midiNote' plugins/O-Bowed/Source/BowedStringVoice.cpp && \
      grep -F 'adds VST3 Note Expression microtonal support for Dorico' plugins/O-Bowed/CHANGELOG.md && \
      grep -E 'plugin: O-Bowed' modules/registry.yaml
    </automated>
  </verify>
  <acceptance_criteria>
    - CMakeLists has new `PLUGIN_VERSION "1.3.0"` AND `ouaricon_add_module(O-Bowed note-expression)`.
    - PluginProcessor swap landed.
    - BowedStringVoice.h has setter + member.
    - BowedStringVoice.cpp `getBaseFrequencyFromTuning` body now contains the `applyPendingTuning` call.
    - CHANGELOG top is `## [1.3.0] - 2026-04-25` with TRACK-03 phrase.
    - registry has `plugin: O-Bowed`, `version: 1.3.0`.
    - Tri-format build clean; freshly installed; auval validates.
  </acceptance_criteria>
  <done>/improve cycle complete.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Build-side gate (D-08)</name>
  <read_first>
    - scripts/verify-au-link.sh
    - plugins/O-Bowed/CMakeLists.txt
  </read_first>
  <action>
    Same shape as plan 24-01 Task 3, substituting `O-Bowed`.

    1. `ninja -C build O-Bowed_VST3 O-Bowed_AU O-Bowed_Standalone 2>&1 | tee /tmp/o-bowed-build.log` — exit 0; no Steinberg undefined symbols.
    2. mtime check on installed bundles.
    3. `bash scripts/verify-au-link.sh O-Bowed` exits 0.
    4. `auval -a | grep -i 'O.Bowed'` returns ≥1 line.
  </action>
  <verify>
    <automated>! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-bowed-build.log 2>/dev/null && test -d ~/Library/Audio/Plug-Ins/VST3/O-Bowed.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/O-Bowed.component && bash scripts/verify-au-link.sh O-Bowed && auval -a 2>/dev/null | grep -i 'O.Bowed'</automated>
  </verify>
  <acceptance_criteria>
    - Build clean; bundles installed; auval validates.
  </acceptance_criteria>
  <done>Build-side gate PASS.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico 3-point smoke gate (D-07) — second MPE plugin</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    O-Bowed v1.3.0 with VST3 Note Expression — second MPE-based propagation. Helper-based composition (same shape as O-Reed). Tri-format built; freshly installed; AU validated.
  </what-built>
  <how-to-verify>
    Same procedure as plan 24-05 (Dorico MPE + Microtonality = "VST3 Note Expression" on expression map).

    **Gate point 1:** quarter-sharp C4 → +50¢ above C4. PASS / FAIL with observed Hz.
    **Gate point 2:** no attack zipper — waveguide string period sized to tuned frequency. PASS / FAIL.
    **Gate point 3:** chord (quarter-sharp C4 + natural E4) — only C4 voice detuned. PASS / FAIL.

    Record in `24-06-O-Bowed-SUMMARY.md`. Type `approved` or describe failure.
  </how-to-verify>
  <resume-signal>Type `approved` if 3-point gate PASS; else describe failure with observed Hz.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Write 24-06-O-Bowed-SUMMARY.md and close plan</name>
  <read_first>
    - .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md (format reference; same MPE pattern)
    - $HOME/.claude/get-shit-done/templates/summary.md
  </read_first>
  <action>
    Mirror prior SUMMARY structure. Document PROP-06 + TRACK-01..05; flag MPE structural variation (now validated on second instance, signaling pattern stability); record 3-point gate; "feeds 24-08-final-sweep-SUMMARY.md row 6 of 8".
  </action>
  <verify>
    <automated>test -f .planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md && grep -E 'PROP-06' .planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md && grep -iE 'PASS|FAIL' .planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md</automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY references PROP-06 + 5 TRACK reqs; 3-point gate; MPE notes; PLUGIN_VERSION line addition.
  </acceptance_criteria>
  <done>Plan 24-06 closed; ready for 24-07.</done>
</task>

</tasks>

<verification>
Per 24-INTEGRATION-MATRIX.md template applied to O-Bowed v1.3.0.
</verification>

<success_criteria>
PROP-06 + TRACK-01..05 satisfied; atomic commit landed; SUMMARY complete.
</success_criteria>

<output>
- `.planning/phases/24-propagate/24-06-O-Bowed-SUMMARY.md`
- Updated O-Bowed sources + CHANGELOG + STATUS + registry
- Freshly installed bundles
- One atomic commit
</output>
