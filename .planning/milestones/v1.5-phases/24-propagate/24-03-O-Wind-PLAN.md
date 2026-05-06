---
phase: 24-propagate
plan: 03
type: execute
wave: 3
depends_on: [24-02]
files_modified:
  - plugins/O-Wind/CMakeLists.txt
  - plugins/O-Wind/Source/PluginProcessor.h
  - plugins/O-Wind/Source/PluginProcessor.cpp
  - plugins/O-Wind/Source/FluteSynthVoice.h
  - plugins/O-Wind/Source/FluteSynthVoice.cpp
  - plugins/O-Wind/CHANGELOG.md
  - plugins/O-Wind/.planning/STATUS.md
  - modules/registry.yaml
autonomous: false
requirements: [PROP-04, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05]

must_haves:
  truths:
    - "O-Wind responds to Dorico VST3 NE — pitch lands +50¢ above C4."
    - "First sample at tuned pitch — composition order applies NE BEFORE pitch-bend at line 84 AND BEFORE `boreWaveguide.setBoreDelay(...)` at line 100 (physical-model period derivation sees the tuned frequency)."
    - "Polyphonic chord NE correlated by `noteId`."
    - "Tri-format build clean — physical-model BoreWaveguide validation passes."
    - "TRACK-01..05: /improve ran; PLUGIN_VERSION line ADDED (was missing) at 1.16.0; CHANGELOG TRACK-03; STATUS updated; freshly installed; registry."
  artifacts:
    - path: "plugins/O-Wind/CMakeLists.txt"
      provides: "Module consumption + NEW PLUGIN_VERSION line + minor bump"
      contains: "ouaricon_add_module(O-Wind note-expression)"
    - path: "plugins/O-Wind/CMakeLists.txt"
      provides: "PLUGIN_VERSION line added (was missing)"
      contains: 'PLUGIN_VERSION "1.16.0"'
    - path: "plugins/O-Wind/Source/PluginProcessor.h"
      provides: "VST3Extensions member"
      contains: "Ouaricon::NoteExpression::VST3Extensions vst3Extensions"
    - path: "plugins/O-Wind/Source/FluteSynthVoice.cpp"
      provides: "Composition site"
      contains: "Ouaricon::NoteExpression::applyPendingTuning("
    - path: "plugins/O-Wind/CHANGELOG.md"
      provides: "TRACK-03 entry"
      contains: "adds VST3 Note Expression microtonal support for Dorico"
    - path: "modules/registry.yaml"
      provides: "Consumer registration"
      contains: "plugin: O-Wind"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Wind.vst3"
      provides: "Freshly installed VST3"
    - path: "~/Library/Audio/Plug-Ins/Components/O-Wind.component"
      provides: "Freshly installed AU"
    - path: ".planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md"
      provides: "Plan close-out"
  key_links:
    - from: "FluteSynthVoice.cpp::startNote"
      to: "applyPendingTuning"
      via: "after lines 78-81 base-frequency assignment, BEFORE pitch-bend at line 84 and BoreWaveguide setBoreDelay at line 100"
      pattern: "applyPendingTuning\\(.*pendingTuningSource"
---

<objective>
Propagate `note-expression` into **O-Wind** (Plan 24-03 — third wave per D-11). EXACT match to O-Lyrica + physical-model period validation (BoreWaveguide). Two CMake structural deltas vs. canary: (a) `PLUGIN_VERSION` argument is MISSING from `juce_add_plugin(O-Wind ...)` — must be added explicitly; (b) `float currentFrequency` requires double-cast at the helper boundary.

Purpose: PROP-04 + TRACK-01..05 via `/improve O-Wind`. Version 1.15.1 → 1.16.0 (minor; baseline from CHANGELOG top entry since CMake has no version line today).

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
@.planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@CLAUDE.md
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp
@plugins/O-Wind/CMakeLists.txt
@plugins/O-Wind/Source/PluginProcessor.h
@plugins/O-Wind/Source/PluginProcessor.cpp
@plugins/O-Wind/Source/FluteSynthVoice.h
@plugins/O-Wind/Source/FluteSynthVoice.cpp
@plugins/O-Wind/CHANGELOG.md
@plugins/O-Wind/.planning/STATUS.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/plugin-improve/SKILL.md
@scripts/verify-au-link.sh

<integrations>
[O-Wind row in 24-INTEGRATION-MATRIX.md](./24-INTEGRATION-MATRIX.md#o-wind)

**Match quality:** EXACT structurally — but two deltas:
1. **Missing `PLUGIN_VERSION`** in `juce_add_plugin(O-Wind ...)` block (line 6 onward). Must be added explicitly during version bump.
2. **`float currentFrequency`** — cast through `double` at helper boundary (same pattern as O-Bells).

Composition order is critical (Pattern 2):
- Base-freq from TuningEngine at lines 78-81.
- **Apply NE here.**
- Pitch-bend at line 84 (composes multiplicatively on top).
- BoreWaveguide period derivation at line 100 (`boreWaveguide.setBoreDelay(totalLoopDelay / (1.0f + initJetRatio))` — physical-model period sized to FINAL tuned frequency).

If NE is applied AFTER line 100, the bore-delay is sized for untuned frequency — pitch arrives but model timbre/period mismatched on first sample (Pattern 2 violation; smoke gate point 2 would catch).
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight — confirm O-Wind preconditions + PLUGIN_VERSION addition target</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md ([O-Wind row](./24-INTEGRATION-MATRIX.md#o-wind))
    - .planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md (carry-forward)
    - plugins/O-Wind/.planning/STATUS.md
    - plugins/O-Wind/CMakeLists.txt (verify NO `PLUGIN_VERSION` arg in `juce_add_plugin(O-Wind ...)` block lines 6-46; OuariconModules.cmake at line 3; current CHANGELOG top entry `[1.15.1]`)
    - plugins/O-Wind/CHANGELOG.md (verify `[1.15.1]` is current top — establishes 1.16.0 minor bump target)
  </read_first>
  <action>
    1. STATUS not 🚧.
    2. Confirm `juce_add_plugin(O-Wind ...)` has NO `PLUGIN_VERSION` line — verify via:
       ```
       awk '/^juce_add_plugin\(O-Wind/,/^\)/' plugins/O-Wind/CMakeLists.txt | grep -c PLUGIN_VERSION
       ```
       MUST return 0. (PATTERNS.md confirmed; planner needs to defend against any drift.)
    3. CHANGELOG.md top entry confirmed as `## [1.15.1]` — bump target is 1.16.0.
    4. JUCE patch markers present.
    5. Working tree clean.
  </action>
  <verify>
    <automated>test "$(awk '/^juce_add_plugin\(O-Wind/,/^\)/' plugins/O-Wind/CMakeLists.txt | grep -c PLUGIN_VERSION)" = "0" && grep -E '^## \[1\.15\.1\]' plugins/O-Wind/CHANGELOG.md && grep -E 'include\(\$\{CMAKE_SOURCE_DIR\}/modules/cmake/OuariconModules\.cmake\)' plugins/O-Wind/CMakeLists.txt && grep -l 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h && git diff --quiet -- plugins/O-Wind modules/registry.yaml</automated>
  </verify>
  <acceptance_criteria>
    - PLUGIN_VERSION absent from `juce_add_plugin(O-Wind ...)` block (must be added during /improve).
    - CHANGELOG top entry is `## [1.15.1]` (1.16.0 minor target derived).
    - OuariconModules.cmake included.
    - JUCE patch markers present; tree clean.
  </acceptance_criteria>
  <done>Preconditions PASS.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Run /improve O-Wind with locked specification (includes PLUGIN_VERSION line addition)</name>
  <read_first>
    - .planning/phases/24-propagate/24-PATTERNS.md §"4. O-Wind (TuningEngine-composing + physical-model period — flute waveguide)"
    - plugins/O-Wind/CMakeLists.txt lines 6-46 (`juce_add_plugin(O-Wind ...)` block — PRODUCT_NAME at line 11, IS_SYNTH at line 12; PLUGIN_VERSION goes between)
    - plugins/O-Wind/Source/FluteSynthVoice.cpp lines 60-110 (startNote body; base-freq 78-81; pitch-bend 84; BoreWaveguide 100)
    - plugins/O-Wind/Source/PluginProcessor.cpp lines 485-495 (addVoice loop) and lines 528-540 (processBlock top)
    - plugins/O-Wind/Source/PluginProcessor.h around line 67 (`juce::Synthesiser synthesiser;`)
  </read_first>
  <action>
    Invoke `/improve O-Wind` with locked specification.

    ### Edit 1 — `plugins/O-Wind/CMakeLists.txt`

    **Add PLUGIN_VERSION line** inside the `juce_add_plugin(O-Wind ...)` block, BETWEEN `PRODUCT_NAME` (line 11) and `IS_SYNTH` (line 12):
    ```cmake
        PLUGIN_VERSION "1.16.0"
    ```

    **Add module call** after line 47 (end of `target_sources` block):
    ```cmake
    # Phase 24: VST3 Note Expression microtonal support (Dorico)
    ouaricon_add_module(O-Wind note-expression)
    ```

    ### Edit 2 — `plugins/O-Wind/Source/PluginProcessor.h`

    Add `#include "NoteExpression.h"`. Add public override `getVST3ClientExtensions()` returning `&vst3Extensions`. Add private member `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;` IMMEDIATELY AFTER `juce::Synthesiser synthesiser;` (line 67).

    ### Edit 3 — `plugins/O-Wind/Source/PluginProcessor.cpp`

    addVoice loop (lines 487-489):
    ```cpp
    auto* voice = new FluteSynthVoice (&parameters, &tuningEngine);
    voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
    synthesiser.addVoice (voice);
    ```

    Top of `processBlock` (line 532) after `buffer.clear()`:
    ```cpp
    vst3Extensions.drainAndUpdate();
    ```

    ### Edit 4 — `plugins/O-Wind/Source/FluteSynthVoice.h`

    Standard shape: include, public setter, private member.

    ### Edit 5 — `plugins/O-Wind/Source/FluteSynthVoice.cpp` — composition site

    Existing assignment (lines 78-81):
    ```cpp
    if (tuningEngine != nullptr)
        currentFrequency = static_cast<float> (tuningEngine->getFrequency (midiNoteNumber));
    else
        currentFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
    ```

    Insert IMMEDIATELY AFTER line 81, BEFORE the pitch-bend application at line 84:
    ```cpp
    // VST3 Note Expression tuning delta (Dorico).
    // Apply BEFORE pitch-bend (line 84) and BEFORE BoreWaveguide period derivation
    // (boreWaveguide.setBoreDelay at line 100) so the physical-model period sees the
    // tuned frequency on sample 0 (Pattern 2 — no attack zipper).
    if (pendingTuningSource != nullptr)
    {
        currentFrequency = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
            *pendingTuningSource, midiNoteNumber, static_cast<double>(currentFrequency)));
    }
    ```

    > **Float→double cast** required: `FluteSynthVoice` uses `float currentFrequency` (same pattern as O-Bells).

    ### Edit 6 — `plugins/O-Wind/CHANGELOG.md`

    Insert at top (above `## [1.15.1]`). O-Wind uses bracketed style:
    ```markdown
    ## [1.16.0] - 2026-04-25

    ### Added — VST3 Note Expression Microtonal Support for Dorico

    Adds VST3 Note Expression microtonal support for Dorico. O-Wind responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events). Composition order: TuningEngine → NE delta → pitch-bend → bore-delay derivation. The bore waveguide period sizes to the tuned frequency on the first sample (no attack zipper). End users must set Microtonality to "VST3 Note Expression" on the Dorico expression map.

    **Files Modified:** `Source/PluginProcessor.{h,cpp}`, `Source/FluteSynthVoice.{h,cpp}`, `CMakeLists.txt` (added PLUGIN_VERSION line + ouaricon_add_module). Version: 1.15.1 → 1.16.0.
    ```

    Phrase `adds VST3 Note Expression microtonal support for Dorico` MUST appear (TRACK-03).

    ### Edits 7, 8

    /improve handles STATUS.md + registry.yaml.

    ### /improve invocation

    ```
    /improve O-Wind
    ```
    Description: "Adds VST3 Note Expression microtonal support for Dorico via shared note-expression module adoption (Phase 24, plan 24-03)."
  </action>
  <verify>
    <automated>
      grep -E 'ouaricon_add_module\(O-Wind note-expression\)' plugins/O-Wind/CMakeLists.txt && \
      awk '/^juce_add_plugin\(O-Wind/,/^\)/' plugins/O-Wind/CMakeLists.txt | grep -E 'PLUGIN_VERSION "1\.16\.0"' && \
      grep -E 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-Wind/Source/PluginProcessor.h && \
      grep -E 'vst3Extensions\.drainAndUpdate\(' plugins/O-Wind/Source/PluginProcessor.cpp && \
      grep -E 'Ouaricon::NoteExpression::applyPendingTuning\(' plugins/O-Wind/Source/FluteSynthVoice.cpp && \
      grep -F 'adds VST3 Note Expression microtonal support for Dorico' plugins/O-Wind/CHANGELOG.md && \
      grep -E 'plugin: O-Wind' modules/registry.yaml
    </automated>
  </verify>
  <acceptance_criteria>
    - CMakeLists has `PLUGIN_VERSION "1.16.0"` INSIDE `juce_add_plugin(O-Wind ...)` block AND `ouaricon_add_module(O-Wind note-expression)`.
    - PluginProcessor swap landed (header + cpp).
    - FluteSynthVoice has setter + member; .cpp has applyPendingTuning call after lines 78-81 and BEFORE line 84 pitch-bend / line 100 BoreWaveguide setBoreDelay.
    - CHANGELOG top is `[1.16.0]` with TRACK-03 phrase.
    - registry has `plugin: O-Wind`, `version: 1.16.0`.
    - Tri-format build + install + AU verify all clean; one atomic commit.
  </acceptance_criteria>
  <done>/improve cycle complete.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Build-side gate (D-08)</name>
  <read_first>
    - scripts/verify-au-link.sh
    - plugins/O-Wind/CMakeLists.txt (PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / IS_SYNTH)
  </read_first>
  <action>
    Same shape as plan 24-01 Task 3, substituting `O-Wind`.

    1. `ninja -C build O-Wind_VST3 O-Wind_AU O-Wind_Standalone 2>&1 | tee /tmp/o-wind-build.log` — exit 0; no Steinberg undefined symbols.
    2. mtime check on installed `.vst3` and `.component`.
    3. `bash scripts/verify-au-link.sh O-Wind` exits 0.
    4. `auval -a | grep -i 'O.Wind'` returns ≥1 line.
  </action>
  <verify>
    <automated>! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-wind-build.log 2>/dev/null && test -d ~/Library/Audio/Plug-Ins/VST3/O-Wind.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/O-Wind.component && bash scripts/verify-au-link.sh O-Wind && auval -a 2>/dev/null | grep -i 'O.Wind'</automated>
  </verify>
  <acceptance_criteria>
    - Build clean; bundles installed; auval validates; AU registered.
  </acceptance_criteria>
  <done>Build-side gate PASS.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico 3-point smoke gate (D-07)</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    O-Wind v1.16.0 with VST3 Note Expression. Tri-format built; freshly installed; AU validated. Physical-model BoreWaveguide period derivation now sees tuned frequency.
  </what-built>
  <how-to-verify>
    Identical procedure to plan 24-01 Task 4, substituting O-Wind.

    **Gate point 1:** quarter-sharp C4 → +50¢ above C4. PASS / FAIL with observed Hz.
    **Gate point 2:** no attack zipper (especially important here — physical-model bore delay is sensitive to mid-note frequency change). PASS / FAIL.
    **Gate point 3:** chord (quarter-sharp C4 + natural E4) — only C4 detuned. PASS / FAIL.

    Record in `24-03-O-Wind-SUMMARY.md`. Type `approved` or describe failure.
  </how-to-verify>
  <resume-signal>Type `approved` if 3-point gate PASS; else describe failure.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Write 24-03-O-Wind-SUMMARY.md and close plan</name>
  <read_first>
    - .planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md (format reference)
    - $HOME/.claude/get-shit-done/templates/summary.md
  </read_first>
  <action>
    Mirror 24-02 SUMMARY structure. Document PROP-04 + TRACK-01..05; flag the PLUGIN_VERSION line addition (structural delta worth recording for retrospective); record 3-point gate observations; "feeds 24-08-final-sweep-SUMMARY.md row 3 of 8".
  </action>
  <verify>
    <automated>test -f .planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md && grep -E 'PROP-04' .planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md && grep -iE 'PASS|FAIL' .planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md</automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY references PROP-04 + 5 TRACK reqs; 3-point gate result recorded; PLUGIN_VERSION line addition noted.
  </acceptance_criteria>
  <done>Plan 24-03 closed; ready for 24-04.</done>
</task>

</tasks>

<verification>
Per 24-INTEGRATION-MATRIX.md template applied to O-Wind v1.16.0. Additional structural check: PLUGIN_VERSION line present inside `juce_add_plugin(O-Wind ...)` block.
</verification>

<success_criteria>
PROP-04 + TRACK-01..05 satisfied; atomic commit landed; SUMMARY complete; physical-model integration validated.
</success_criteria>

<output>
- `.planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md`
- Updated O-Wind sources (with new PLUGIN_VERSION line) + CHANGELOG + STATUS + registry
- Freshly installed bundles
- One atomic commit
</output>
