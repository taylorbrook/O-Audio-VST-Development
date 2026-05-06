---
phase: 24-propagate
plan: 05
type: execute
wave: 5
depends_on: [24-04]
files_modified:
  - plugins/O-Reed/CMakeLists.txt
  - plugins/O-Reed/Source/PluginProcessor.h
  - plugins/O-Reed/Source/PluginProcessor.cpp
  - plugins/O-Reed/Source/ReedWindVoice.h
  - plugins/O-Reed/Source/ReedWindVoice.cpp
  - plugins/O-Reed/CHANGELOG.md
  - plugins/O-Reed/.planning/STATUS.md
  - modules/registry.yaml
autonomous: false
requirements: [PROP-05, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05]

must_haves:
  truths:
    - "O-Reed responds to Dorico VST3 NE — pitch lands +50¢ above C4 for quarter-sharp C4 played from Dorico in MPE mode."
    - "MPE composition: NE delta applied INSIDE the `getBaseFrequencyFromTuning` helper, single source of truth for both `noteStarted()` (legato site at line 141 + normal site at line 202) AND `notePitchbendChanged` (line 374). exchange(0.0) consume semantics correct: first call consumes, subsequent calls in same block return base unchanged."
    - "Bore waveguide period sized to tuned frequency on sample 0 (no attack zipper)."
    - "MPE pitch-bend stacks multiplicatively on top of NE delta — NE applies once per noteStarted, MPE pitch-bend updates per-block on top."
    - "Chord polyphony correlated by `noteId` (Dorico delivers MPE NoteOn + per-note NE; module's updatePendingFromEvents correlates by noteId regardless of MPE channel)."
    - "Tri-format build clean. TRACK-01..05: /improve ran; PLUGIN_VERSION line ADDED (was missing) at 1.1.0; CHANGELOG TRACK-03; STATUS updated; freshly installed; registry."
  artifacts:
    - path: "plugins/O-Reed/CMakeLists.txt"
      provides: "Module consumption + NEW PLUGIN_VERSION line + minor bump"
      contains: "ouaricon_add_module(O-Reed note-expression)"
    - path: "plugins/O-Reed/CMakeLists.txt"
      provides: "PLUGIN_VERSION line added"
      contains: 'PLUGIN_VERSION "1.1.0"'
    - path: "plugins/O-Reed/Source/PluginProcessor.h"
      provides: "VST3Extensions member"
      contains: "Ouaricon::NoteExpression::VST3Extensions vst3Extensions"
    - path: "plugins/O-Reed/Source/ReedWindVoice.cpp"
      provides: "NE inside getBaseFrequencyFromTuning helper (covers all 3 call sites)"
      contains: "Ouaricon::NoteExpression::applyPendingTuning("
    - path: "plugins/O-Reed/CHANGELOG.md"
      provides: "TRACK-03 entry"
      contains: "adds VST3 Note Expression microtonal support for Dorico"
    - path: "modules/registry.yaml"
      provides: "Consumer registration"
      contains: "plugin: O-Reed"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Reed.vst3"
      provides: "Freshly installed VST3"
    - path: "~/Library/Audio/Plug-Ins/Components/O-Reed.component"
      provides: "Freshly installed AU"
    - path: ".planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md"
      provides: "Plan close-out"
  key_links:
    - from: "ReedWindVoice.cpp::getBaseFrequencyFromTuning helper (lines 121-126)"
      to: "applyPendingTuning"
      via: "INSIDE the helper — covers noteStarted() legato (line 141) + normal (line 202) + notePitchbendChanged (line 374) call sites with one source of truth"
      pattern: "applyPendingTuning\\(.*pendingTuningSource.*midiNote"
---

<objective>
Propagate `note-expression` into **O-Reed** (Plan 24-05 — fifth wave). FIRST MPE plugin: `juce::MPESynthesiserVoice` base class; `noteStarted()` reads MIDI pitch from `getCurrentlyPlayingNote().initialNote` (no parameter). Helper `getBaseFrequencyFromTuning(int midiNote)` (lines 121-126) is called from THREE sites: `noteStarted()` legato (line 141), `noteStarted()` normal (line 202), and `notePitchbendChanged` (line 374).

**Composition strategy (D-04 / D-06 from CONTEXT.md, locked at planning time):** Apply NE INSIDE the `getBaseFrequencyFromTuning` helper — single source of truth covers all three call sites. The `exchange(0.0)` consume in `applyPendingTuning` semantically still correct:
- First call (legato OR normal in noteStarted): consumes the slot, returns tuned freq.
- Subsequent calls in same block (e.g., notePitchbendChanged during a held note): return base freq unchanged because slot is empty. **This is the right semantics** — pitch-bend updates during a held note shouldn't re-trigger NE; NE applies once per note-on.

**Two CMake structural deltas:** (a) PLUGIN_VERSION arg MISSING from `juce_add_plugin(O-Reed ...)` — must be added; (b) MPE base class swap from `juce::Synthesiser` (canary shape) to `juce::MPESynthesiser` — addVoice loop and processBlock unchanged (MPESynthesiser also has `addVoice(MPESynthesiserVoice*)` and `renderNextBlock`).

Purpose: PROP-05 + TRACK-01..05 via `/improve O-Reed`. Version 1.0.11 → 1.1.0 (minor; baseline from CHANGELOG since CMake has no version line).

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
@.planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@CLAUDE.md
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp
@plugins/O-Reed/CMakeLists.txt
@plugins/O-Reed/Source/PluginProcessor.h
@plugins/O-Reed/Source/PluginProcessor.cpp
@plugins/O-Reed/Source/ReedWindVoice.h
@plugins/O-Reed/Source/ReedWindVoice.cpp
@plugins/O-Reed/Source/DSP/BoreWaveguide.h
@plugins/O-Reed/CHANGELOG.md
@plugins/O-Reed/.planning/STATUS.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/plugin-improve/SKILL.md
@scripts/verify-au-link.sh

<integrations>
[O-Reed row in 24-INTEGRATION-MATRIX.md](./24-INTEGRATION-MATRIX.md#o-reed)

**Match quality:** STRUCTURAL (MPE) — different base class (`juce::MPESynthesiserVoice`), same composition order. Helper-based consolidation avoids per-site edits.

**MPE NE correlation note (from PATTERNS.md):** Dorico delivers MPE NoteOn (master channel + per-note channel) + per-note NE. The module's `updatePendingFromEvents` correlates by `noteId` (Pattern 1) regardless of MPE channel — same semantics as the canary. No MPE-specific NE work needed.

**MPE pitch-bend interplay:** MPE pitch-bend is per-note, applied as a multiplicative ratio in the voice. NE delta is also multiplicative. They compose by multiplication — order: tuning_engine → NE → MPE pitch-bend → bore.setFrequency. NE applies once per noteStarted; MPE pitch-bend re-applied each render block. Both are stable simultaneously.
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight — confirm O-Reed preconditions</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md ([O-Reed row](./24-INTEGRATION-MATRIX.md#o-reed))
    - .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md
    - plugins/O-Reed/.planning/STATUS.md
    - plugins/O-Reed/CMakeLists.txt (line 3 OuariconModules.cmake; line 5 juce_add_plugin; verify NO PLUGIN_VERSION inside the block)
    - plugins/O-Reed/CHANGELOG.md (verify top entry `## v1.0.11`)
    - plugins/O-Reed/Source/ReedWindVoice.cpp lines 120-130 (`getBaseFrequencyFromTuning` helper) AND lines 138-145 (legato call site at 141) AND lines 200-205 (normal call site at 202) AND lines 370-380 (notePitchbendChanged call site at 374)
  </read_first>
  <action>
    1. STATUS not 🚧.
    2. Confirm `juce_add_plugin(O-Reed ...)` has NO `PLUGIN_VERSION` arg:
       ```
       awk '/^juce_add_plugin\(O-Reed/,/^\)/' plugins/O-Reed/CMakeLists.txt | grep -c PLUGIN_VERSION
       ```
       MUST be 0.
    3. CHANGELOG top entry confirmed as `## v1.0.11` (style is `## vX.Y.Z (date)`).
    4. Confirm `getBaseFrequencyFromTuning(int midiNote)` exists at ReedWindVoice.cpp:121-126.
    5. JUCE patch markers present.
    6. Working tree clean.
  </action>
  <verify>
    <automated>test "$(awk '/^juce_add_plugin\(O-Reed/,/^\)/' plugins/O-Reed/CMakeLists.txt | grep -c PLUGIN_VERSION)" = "0" && grep -E '^## v1\.0\.11' plugins/O-Reed/CHANGELOG.md && grep -nE 'getBaseFrequencyFromTuning' plugins/O-Reed/Source/ReedWindVoice.cpp | head -3 && grep -E 'include\(\$\{CMAKE_SOURCE_DIR\}/modules/cmake/OuariconModules\.cmake\)' plugins/O-Reed/CMakeLists.txt && grep -l 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h && git diff --quiet -- plugins/O-Reed modules/registry.yaml</automated>
  </verify>
  <acceptance_criteria>
    - PLUGIN_VERSION absent (must be added during /improve).
    - CHANGELOG top is `## v1.0.11` (1.1.0 minor target).
    - getBaseFrequencyFromTuning helper exists.
    - OuariconModules.cmake included; tree clean.
  </acceptance_criteria>
  <done>Preconditions PASS.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Run /improve O-Reed with locked specification (helper-based MPE composition)</name>
  <read_first>
    - .planning/phases/24-propagate/24-PATTERNS.md §"5. O-Reed (MPESynthesiserVoice + physical-model bore waveguide)"
    - plugins/O-Reed/CMakeLists.txt lines 5-30 (juce_add_plugin block)
    - plugins/O-Reed/Source/ReedWindVoice.h around line 60 (`tuningEngine` member — alongside which we add `pendingTuningSource`)
    - plugins/O-Reed/Source/ReedWindVoice.cpp lines 121-126 (`getBaseFrequencyFromTuning` body — helper site)
    - plugins/O-Reed/Source/PluginProcessor.cpp lines 340-345 (addVoice loop), line 378 (processBlock top)
    - plugins/O-Reed/Source/PluginProcessor.h around line 63 (`juce::MPESynthesiser synthesiser;`)
  </read_first>
  <action>
    Invoke `/improve O-Reed` with locked specification.

    ### Edit 1 — `plugins/O-Reed/CMakeLists.txt`

    **Add PLUGIN_VERSION line** inside `juce_add_plugin(O-Reed ...)` block, BETWEEN `PRODUCT_NAME` (line 11) and `IS_SYNTH`:
    ```cmake
        PLUGIN_VERSION "1.1.0"
    ```

    **Add module call** after line 29 (end of `target_sources`):
    ```cmake
    # Phase 24: VST3 Note Expression microtonal support (Dorico)
    ouaricon_add_module(O-Reed note-expression)
    ```

    ### Edit 2 — `plugins/O-Reed/Source/PluginProcessor.h`

    Add `#include "NoteExpression.h"`. Add public override `getVST3ClientExtensions()` returning `&vst3Extensions`. Add private member after `juce::MPESynthesiser synthesiser;` (line 63):
    ```cpp
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    ```

    ### Edit 3 — `plugins/O-Reed/Source/PluginProcessor.cpp`

    addVoice loop (lines 341-344):
    ```cpp
    auto* voice = new ReedWindVoice(i);
    voice->setAPVTS(&parameters);
    voice->setTuningEngine(&tuningEngine);
    voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
    synthesiser.addVoice(voice);
    ```

    Top of `processBlock` at line 378 after `buffer.clear()`:
    ```cpp
    vst3Extensions.drainAndUpdate();
    ```

    ### Edit 4 — `plugins/O-Reed/Source/ReedWindVoice.h`

    Add `#include "NoteExpression.h"`. Add public setter `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable*)`. Add private member alongside `tuningEngine` at line 60:
    ```cpp
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    ```

    ### Edit 5 — `plugins/O-Reed/Source/ReedWindVoice.cpp` — modify getBaseFrequencyFromTuning helper

    **Replace existing helper body (lines 121-126):**
    ```cpp
    float ReedWindVoice::getBaseFrequencyFromTuning(int midiNote) const
    {
        return (tuningEngine != nullptr)
            ? static_cast<float>(tuningEngine->getFrequency(midiNote))
            : static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote));
    }
    ```

    **With:**
    ```cpp
    float ReedWindVoice::getBaseFrequencyFromTuning(int midiNote) const
    {
        double freq = (tuningEngine != nullptr)
            ? tuningEngine->getFrequency(midiNote)
            : juce::MidiMessage::getMidiNoteInHertz(midiNote);

        // VST3 Note Expression tuning delta (Dorico microtonal).
        // Helper consumes slot via exchange(0.0) — first call (in noteStarted) tunes;
        // subsequent calls (notePitchbendChanged during a held note) return base
        // unchanged, which is correct: NE applies once per noteStarted.
        if (pendingTuningSource != nullptr)
            freq = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNote, freq);

        return static_cast<float>(freq);
    }
    ```

    > **Why in the helper:** single source of truth covers BOTH `noteStarted()` legato call (line 141) + normal call (line 202) + `notePitchbendChanged` call (line 374). DRY.
    > **`const` preserved:** the helper is `const`; modifying the atomic slot is OK because `applyPendingTuning` operates on `pendingTuningSource` which is a pointer to a member elsewhere — the const-ness of `*this` is unaffected (`exchange` on `std::atomic<double>` doesn't violate const).
    > **Cast through `double`:** helper returns `float`; helper signature is `double(...)`. Cast at boundaries (matches O-Bells/O-Wind pattern).

    ### Edit 6 — `plugins/O-Reed/CHANGELOG.md`

    Insert at top (above `## v1.0.11`). O-Reed uses `## vX.Y.Z (date)` style:
    ```markdown
    ## v1.1.0 (2026-04-25)

    ### Added
    - **Adds VST3 Note Expression microtonal support for Dorico.** Composes with TuningEngine via `getBaseFrequencyFromTuning` helper before bore waveguide period derivation. MPE-aware: NE delta applies to `note.initialNote`, MPE pitch-bend stacks multiplicatively on top.
    - **Shared `note-expression` module adoption.**

    ### Technical Notes
    - Files modified: `Source/PluginProcessor.{h,cpp}`, `Source/ReedWindVoice.{h,cpp}`, `CMakeLists.txt` (added PLUGIN_VERSION line + ouaricon_add_module).
    - Composition order: tuning engine → NE delta → MPE pitch-bend → `bore.setFrequency(freq)` (bore waveguide period derived from final tuned frequency).
    - Helper consolidation: NE applied INSIDE `getBaseFrequencyFromTuning` so all three call sites (noteStarted legato, noteStarted normal, notePitchbendChanged) inherit it. exchange(0.0) consume semantics correct for one-NE-per-noteOn delivery.
    - Version: 1.0.11 → 1.1.0.
    ```

    Phrase `adds VST3 Note Expression microtonal support for Dorico` MUST appear (TRACK-03).

    ### Edits 7, 8

    /improve handles STATUS.md + registry.yaml.

    ### /improve invocation

    ```
    /improve O-Reed
    ```
    Description: "Adds VST3 Note Expression microtonal support for Dorico via shared note-expression module — first MPE plugin, helper-based composition (Phase 24, plan 24-05)."
  </action>
  <verify>
    <automated>
      grep -E 'ouaricon_add_module\(O-Reed note-expression\)' plugins/O-Reed/CMakeLists.txt && \
      awk '/^juce_add_plugin\(O-Reed/,/^\)/' plugins/O-Reed/CMakeLists.txt | grep -E 'PLUGIN_VERSION "1\.1\.0"' && \
      grep -E 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-Reed/Source/PluginProcessor.h && \
      grep -E 'vst3Extensions\.drainAndUpdate\(' plugins/O-Reed/Source/PluginProcessor.cpp && \
      grep -E 'setPendingTuningSource\(&vst3Extensions' plugins/O-Reed/Source/PluginProcessor.cpp && \
      grep -E 'Ouaricon::NoteExpression::applyPendingTuning\(.*midiNote' plugins/O-Reed/Source/ReedWindVoice.cpp && \
      grep -E 'getBaseFrequencyFromTuning' plugins/O-Reed/Source/ReedWindVoice.cpp | head -1 && \
      grep -F 'adds VST3 Note Expression microtonal support for Dorico' plugins/O-Reed/CHANGELOG.md && \
      grep -E 'plugin: O-Reed' modules/registry.yaml
    </automated>
  </verify>
  <acceptance_criteria>
    - CMakeLists has new `PLUGIN_VERSION "1.1.0"` AND `ouaricon_add_module(O-Reed note-expression)`.
    - PluginProcessor swap landed (header + cpp).
    - ReedWindVoice.h has setter + member.
    - ReedWindVoice.cpp `getBaseFrequencyFromTuning` body now contains the `applyPendingTuning` call between the tuningEngine query and the return.
    - CHANGELOG top is `## v1.1.0 (2026-04-25)` with TRACK-03 phrase.
    - registry has `plugin: O-Reed`, `version: 1.1.0`.
    - Tri-format build clean; freshly installed; auval validates; one atomic commit.
  </acceptance_criteria>
  <done>/improve cycle complete.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Build-side gate (D-08)</name>
  <read_first>
    - scripts/verify-au-link.sh
    - plugins/O-Reed/CMakeLists.txt (PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / IS_SYNTH)
  </read_first>
  <action>
    Same shape as plan 24-01 Task 3, substituting `O-Reed`.

    1. `ninja -C build O-Reed_VST3 O-Reed_AU O-Reed_Standalone 2>&1 | tee /tmp/o-reed-build.log` — exit 0; no Steinberg undefined symbols.
    2. mtime check on installed bundles.
    3. `bash scripts/verify-au-link.sh O-Reed` exits 0.
    4. `auval -a | grep -i 'O.Reed'` returns ≥1 line.
  </action>
  <verify>
    <automated>! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-reed-build.log 2>/dev/null && test -d ~/Library/Audio/Plug-Ins/VST3/O-Reed.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/O-Reed.component && bash scripts/verify-au-link.sh O-Reed && auval -a 2>/dev/null | grep -i 'O.Reed'</automated>
  </verify>
  <acceptance_criteria>
    - Build clean; bundles installed; auval validates.
  </acceptance_criteria>
  <done>Build-side gate PASS.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico 3-point smoke gate (D-07) — first MPE plugin</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    O-Reed v1.1.0 with VST3 Note Expression — first MPE-based propagation. Helper-based composition covers noteStarted (legato + normal) AND notePitchbendChanged. Tri-format built; freshly installed; AU validated.
  </what-built>
  <how-to-verify>
    Same procedure as plan 24-01 with MPE-specific notes.

    **Setup variant:** Dorico's MPE handling is built-in for compatible instruments; just ensure the assigned expression map has Microtonality = "VST3 Note Expression". Dorico delivers MPE NoteOn + per-note NE on the per-note MPE channel; the module's correlation by `noteId` works regardless of MPE channel.

    **Gate point 1:** quarter-sharp C4 → +50¢ above C4. PASS / FAIL with observed Hz.
    **Gate point 2:** no attack zipper — bore waveguide period sized to tuned frequency. PASS / FAIL.
    **Gate point 3:** chord (quarter-sharp C4 + natural E4) — only C4 voice detuned. PASS / FAIL.

    **Bonus check (MPE-specific, not gating):** play a held quarter-sharp C4 with Dorico's slur/legato (which can trigger notePitchbendChanged updates). Verify pitch stays at +50¢ throughout — confirms `notePitchbendChanged` returning the base unchanged (slot empty after first call) is the right semantics. If pitch resets to natural C4 mid-note, NE was unintentionally re-consumed. Note this in the SUMMARY but don't gate on it (D-07 is the binding gate).

    Record in `24-05-O-Reed-SUMMARY.md`. Type `approved` or describe failure.
  </how-to-verify>
  <resume-signal>Type `approved` if 3-point gate PASS; else describe failure with observed Hz.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Write 24-05-O-Reed-SUMMARY.md and close plan</name>
  <read_first>
    - .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md
    - $HOME/.claude/get-shit-done/templates/summary.md
  </read_first>
  <action>
    Mirror prior SUMMARY structure. Document PROP-05 + TRACK-01..05; flag MPE structural variation + helper-based consolidation + PLUGIN_VERSION line addition; record 3-point gate results + MPE-bonus observation; "feeds 24-08-final-sweep-SUMMARY.md row 5 of 8".
  </action>
  <verify>
    <automated>test -f .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md && grep -E 'PROP-05' .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md && grep -iE 'PASS|FAIL' .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md && grep -iE 'MPE' .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md</automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY references PROP-05 + 5 TRACK reqs; 3-point gate result; MPE notes; PLUGIN_VERSION line addition.
  </acceptance_criteria>
  <done>Plan 24-05 closed; ready for 24-06.</done>
</task>

</tasks>

<verification>
Per 24-INTEGRATION-MATRIX.md template applied to O-Reed v1.1.0. Additional structural checks: PLUGIN_VERSION line present; applyPendingTuning call inside `getBaseFrequencyFromTuning` body.
</verification>

<success_criteria>
PROP-05 + TRACK-01..05 satisfied; atomic commit landed; SUMMARY complete; MPE composition pattern validated.
</success_criteria>

<output>
- `.planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md`
- Updated O-Reed sources (with new PLUGIN_VERSION + helper-based NE) + CHANGELOG + STATUS + registry
- Freshly installed bundles
- One atomic commit
</output>
