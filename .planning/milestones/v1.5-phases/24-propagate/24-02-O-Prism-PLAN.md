---
phase: 24-propagate
plan: 02
type: execute
wave: 2
depends_on: [24-01]
files_modified:
  - plugins/O-Prism/CMakeLists.txt
  - plugins/O-Prism/Source/PluginProcessor.h
  - plugins/O-Prism/Source/PluginProcessor.cpp
  - plugins/O-Prism/Source/PrismVoice.h
  - plugins/O-Prism/Source/PrismVoice.cpp
  - plugins/O-Prism/CHANGELOG.md
  - plugins/O-Prism/.planning/STATUS.md
  - modules/registry.yaml
autonomous: false
requirements: [PROP-03, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05]

must_haves:
  truths:
    - "O-Prism responds to Dorico VST3 NE `kTuningTypeID` events — pitch lands +50¢ above C4 for quarter-sharp C4."
    - "First sample at tuned pitch — composition order applies NE BEFORE `glide.setTarget(currentFrequency)` (line 196) and BEFORE per-oscillator `setFrequency` (lines 202, 219, 238)."
    - "Polyphonic chord with mixed accidentals detunes only the C4 voice — `noteId` correlation."
    - "Tri-format build clean — no Steinberg-symbol regressions."
    - "TRACK-01..05: /improve ran end-to-end; version 1.16.1 → 1.17.0; CHANGELOG TRACK-03 phrase; STATUS updated; freshly installed; registry `used_by` includes `O-Prism`."
  artifacts:
    - path: "plugins/O-Prism/CMakeLists.txt"
      provides: "Module consumption + minor bump"
      contains: "ouaricon_add_module(O-Prism note-expression)"
    - path: "plugins/O-Prism/Source/PluginProcessor.h"
      provides: "VST3Extensions member + override"
      contains: "Ouaricon::NoteExpression::VST3Extensions vst3Extensions"
    - path: "plugins/O-Prism/Source/PluginProcessor.cpp"
      provides: "Voice wiring + drain"
      contains: "vst3Extensions.drainAndUpdate("
    - path: "plugins/O-Prism/Source/PrismVoice.cpp"
      provides: "Composition site"
      contains: "Ouaricon::NoteExpression::applyPendingTuning("
    - path: "plugins/O-Prism/CHANGELOG.md"
      provides: "TRACK-03 entry"
      contains: "adds VST3 Note Expression microtonal support for Dorico"
    - path: "modules/registry.yaml"
      provides: "Consumer registration"
      contains: "plugin: O-Prism"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Prism.vst3"
      provides: "Freshly installed VST3"
    - path: "~/Library/Audio/Plug-Ins/Components/O-Prism.component"
      provides: "Freshly installed AU"
    - path: ".planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md"
      provides: "Plan close-out + Dorico 3-point gate result"
  key_links:
    - from: "PluginProcessor.cpp processBlock"
      to: "vst3Extensions.drainAndUpdate()"
      via: "top of processBlock at line 559, after buffer.clear()"
      pattern: "vst3Extensions\\.drainAndUpdate\\("
    - from: "PrismVoice.cpp::startNote"
      to: "applyPendingTuning"
      via: "after lines 181-185 base-frequency assignment, BEFORE line 196 glide.setTarget"
      pattern: "applyPendingTuning\\(.*pendingTuningSource"
---

<objective>
Propagate `note-expression` module into **O-Prism** (Plan 24-02 — second wave per D-11). EXACT match to O-Lyrica: TuningEngine + classic `juce::Synthesiser` + multi-oscillator voice. O-Prism already calls `ouaricon_add_module(O-Prism webview-relay-manager)` at CMakeLists line 81 — proves the macro works end-to-end in this build.

Purpose: PROP-03 + TRACK-01..05 via `/improve O-Prism` (D-03, TRACK-01). Version bump 1.16.1 → 1.17.0 (minor).

Output: Source edits + CHANGELOG + STATUS + registry + freshly installed bundles + one atomic commit + SUMMARY.md.
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
@.planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@CLAUDE.md
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp
@plugins/O-Lyrica/Source/PluginProcessor.cpp
@plugins/O-Prism/CMakeLists.txt
@plugins/O-Prism/Source/PluginProcessor.h
@plugins/O-Prism/Source/PluginProcessor.cpp
@plugins/O-Prism/Source/PrismVoice.h
@plugins/O-Prism/Source/PrismVoice.cpp
@plugins/O-Prism/CHANGELOG.md
@plugins/O-Prism/.planning/STATUS.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/plugin-improve/SKILL.md
@scripts/verify-au-link.sh

<integrations>
[O-Prism row in 24-INTEGRATION-MATRIX.md](./24-INTEGRATION-MATRIX.md#o-prism)

**Match quality:** EXACT. No casts needed (PrismVoice uses `double` or compatible `currentFrequency` per PATTERNS.md analysis — same shape as O-Lyrica's `HarpSynthVoice`).

**CHANGELOG style note:** O-Prism uses `## v<ver> (date)` (parens, no brackets), unlike most other plugins which use `## [<ver>] - <date>`. Match the existing in-file convention.

Key landmines (each defended by 3-point smoke gate point N):
- Pattern 1 (correlate by noteId, not pitch) — gate point 3.
- Pattern 2 (apply BEFORE glide.setTarget at line 196 and oscillator setFrequency calls at 202, 219, 238) — gate point 2.
- Pattern 3 (240-semitone full-scale) — gate point 1.
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight — confirm /improve preconditions for O-Prism</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md ([O-Prism row](./24-INTEGRATION-MATRIX.md#o-prism))
    - .planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md (carry-forward signals from canary)
    - plugins/O-Prism/.planning/STATUS.md (verify ✅ Working / 📦 Installed)
    - plugins/O-Prism/CMakeLists.txt (current `VERSION 1.16.1` at line 12; `OuariconModules.cmake` at line 3; existing `ouaricon_add_module(O-Prism webview-relay-manager)` at line 81)
    - .claude/skills/plugin-improve/SKILL.md
  </read_first>
  <action>
    1. STATUS.md must NOT be 🚧 In Development.
    2. Confirm `plugins/O-Prism/CMakeLists.txt` line 12 reads `VERSION 1.16.1` (PATTERNS.md baseline; bump target 1.17.0).
    3. Confirm `OuariconModules.cmake` is included (line 3) AND `ouaricon_add_module(O-Prism webview-relay-manager)` exists at line 81 (proves macro works in this plugin's build).
    4. JUCE patch markers present (same check as plan 24-01).
    5. Git working tree clean for `plugins/O-Prism/**` and `modules/registry.yaml`.
    6. Carry-forward from 24-01-O-Bells-SUMMARY.md: confirm no unresolved structural issues escalated. If 24-08-fix-PLAN.md was created during plan 24-01 (D-12 escalation), wait for its closure before proceeding.
  </action>
  <verify>
    <automated>grep -E 'VERSION 1\.16\.1' plugins/O-Prism/CMakeLists.txt && grep -E 'include\(\$\{CMAKE_SOURCE_DIR\}/modules/cmake/OuariconModules\.cmake\)' plugins/O-Prism/CMakeLists.txt && grep -E 'ouaricon_add_module\(O-Prism webview-relay-manager\)' plugins/O-Prism/CMakeLists.txt && grep -l 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h && git diff --quiet -- plugins/O-Prism modules/registry.yaml</automated>
  </verify>
  <acceptance_criteria>
    - CMake baseline confirmed (`VERSION 1.16.1`, OuariconModules included, webview-relay-manager already added).
    - JUCE patch markers present.
    - Working tree clean.
    - 24-01 SUMMARY shows no unresolved structural issues.
  </acceptance_criteria>
  <done>Preconditions PASS — invoke /improve.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Run /improve O-Prism with locked specification</name>
  <read_first>
    - .planning/phases/24-propagate/24-PATTERNS.md §"3. O-Prism (TuningEngine-composing — closest match)"
    - plugins/O-Prism/Source/PrismVoice.cpp lines 157-250 (current startNote body; base-freq at 181-185; glide.setTarget at 196; oscillator setFrequency at 202, 219, 238)
    - plugins/O-Prism/Source/PluginProcessor.cpp lines 470-485 (addVoice loop) and 555-565 (processBlock top)
    - plugins/O-Prism/Source/PluginProcessor.h around line 60 (existing accessors) and line 153 (`juce::Synthesiser synthesiser;`)
    - plugins/O-Lyrica/Source/HarpSynthVoice.cpp:138-147 (reference composition site)
  </read_first>
  <action>
    Invoke `/improve O-Prism` with this LOCKED specification.

    ### Edit 1 — `plugins/O-Prism/CMakeLists.txt`

    Bump version (line 12): `VERSION 1.16.1` → `VERSION 1.17.0`.

    Add module call. Insert IMMEDIATELY AFTER the existing `ouaricon_add_module(O-Prism webview-relay-manager)` at line 81:
    ```cmake
    # Phase 24: VST3 Note Expression microtonal support (Dorico)
    ouaricon_add_module(O-Prism note-expression)
    ```

    ### Edit 2 — `plugins/O-Prism/Source/PluginProcessor.h`

    Add include after line 30 (`OuariconPresetManager.h`):
    ```cpp
    #include "NoteExpression.h"  // modules/tuning/note-expression
    ```

    Add public accessor near `getAPVTS()` (around line 60):
    ```cpp
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
    ```

    Add private member after `juce::Synthesiser synthesiser;` (line 153):
    ```cpp
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    ```

    ### Edit 3 — `plugins/O-Prism/Source/PluginProcessor.cpp`

    addVoice loop (lines 472-478) — insert wiring line:
    ```cpp
    auto* voice = new PrismVoice();
    voice->setTuningEngine(&tuningEngine);
    voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
    synthesiser.addVoice(voice);
    ```

    Top of `processBlock` (line 559), after `buffer.clear()`:
    ```cpp
    vst3Extensions.drainAndUpdate();
    ```

    ### Edit 4 — `plugins/O-Prism/Source/PrismVoice.h`

    Add `#include "NoteExpression.h"`, public setter `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable*)`, private `pendingTuningSource = nullptr`. Mirror O-Lyrica HarpSynthVoice.h shape exactly.

    ### Edit 5 — `plugins/O-Prism/Source/PrismVoice.cpp` — composition site

    Existing base-frequency assignment (lines 181-185) is UNCHANGED:
    ```cpp
    // Get base frequency from TuningEngine
    if (tuningEngine != nullptr)
        currentFrequency = tuningEngine->getFrequency (midiNoteNumber);
    else
        currentFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    ```

    Insert IMMEDIATELY AFTER line 185, BEFORE `glide.setTarget(currentFrequency)` at line 196 AND BEFORE oscillator `setFrequency` calls at lines 202, 219, 238 (Pattern 2 — apply NE BEFORE every downstream consumer of `currentFrequency`):
    ```cpp
    // VST3 Note Expression tuning delta (Dorico microtonal).
    // Compose multiplicatively after TuningEngine, before glide/oscillator setup.
    if (pendingTuningSource != nullptr)
    {
        currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                               *pendingTuningSource, midiNoteNumber, currentFrequency);
    }
    ```

    > **Composition correctness:** `currentFrequency` is the multiplicative root for `freqA = currentFrequency * pow(2, ...)`, `freqB = currentFrequency * pow(2, ...)`, and `subOsc.setFrequency(currentFrequency)`. Applying NE before these multiplications is mathematically correct for any base tuning (D-10).

    ### Edit 6 — `plugins/O-Prism/CHANGELOG.md`

    Insert at top (above current `## v1.16.0` entry — note O-Prism uses `## v<ver> (date)` style, NOT brackets):
    ```markdown
    ## v1.17.0 (2026-04-25)

    ### Added
    - **Adds VST3 Note Expression microtonal support for Dorico** (per O-Lyrica 2.3.0 reference shape). End users must set Microtonality to "VST3 Note Expression" on the assigned Dorico expression map.
    - **Shared `note-expression` module adoption** (`modules/tuning/note-expression` v1.0.0).

    ### Technical Notes
    - **Composition with TuningEngine:** `PrismVoice::startNote` queries `TuningEngine::getFrequency(midi)`, then composes Dorico's NE delta via `applyPendingTuning(table, midi, freq)` before `glide.setTarget()` and per-oscillator `setFrequency()` calls.
    - **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/PrismVoice.{h,cpp}`, `CMakeLists.txt`.
    - **Version bump rationale:** MINOR (1.16.1 → 1.17.0).
    ```

    The exact phrase `adds VST3 Note Expression microtonal support for Dorico` MUST appear (TRACK-03).

    ### Edit 7 — `plugins/O-Prism/.planning/STATUS.md`

    /improve handles automatically. Confirm post-/improve: `last_updated` = today, `version` = 1.17.0.

    ### Edit 8 — `modules/registry.yaml`

    /improve invokes `/module-add note-expression` for O-Prism. Resulting `used_by:` list MUST include `- plugin: O-Prism, version: 1.17.0`.

    ### /improve invocation

    Run:
    ```
    /improve O-Prism
    ```
    Description: "Adds VST3 Note Expression microtonal support for Dorico via shared note-expression module adoption (Phase 24, plan 24-02)."

    /improve Phase 5 invokes `build-automation` → `ninja O-Prism_VST3 O-Prism_AU O-Prism_Standalone` + AU cache clear + fresh install per CLAUDE.md.

    Failure handling per D-12: plan-local triage; structural failure → escalate to fix-plan.
  </action>
  <verify>
    <automated>
      grep -E 'ouaricon_add_module\(O-Prism note-expression\)' plugins/O-Prism/CMakeLists.txt && \
      grep -E 'VERSION 1\.17\.0' plugins/O-Prism/CMakeLists.txt && \
      grep -E 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-Prism/Source/PluginProcessor.h && \
      grep -E 'getVST3ClientExtensions.*&vst3Extensions' plugins/O-Prism/Source/PluginProcessor.h && \
      grep -E 'vst3Extensions\.drainAndUpdate\(' plugins/O-Prism/Source/PluginProcessor.cpp && \
      grep -E 'setPendingTuningSource\(&vst3Extensions' plugins/O-Prism/Source/PluginProcessor.cpp && \
      grep -E 'Ouaricon::NoteExpression::applyPendingTuning\(' plugins/O-Prism/Source/PrismVoice.cpp && \
      grep -E 'pendingTuningSource' plugins/O-Prism/Source/PrismVoice.h && \
      grep -F 'adds VST3 Note Expression microtonal support for Dorico' plugins/O-Prism/CHANGELOG.md && \
      grep -E 'plugin: O-Prism' modules/registry.yaml
    </automated>
  </verify>
  <acceptance_criteria>
    - CMakeLists has `VERSION 1.17.0` AND `ouaricon_add_module(O-Prism note-expression)`.
    - PluginProcessor.h has the include + override + private member.
    - PluginProcessor.cpp has voice wiring AND drainAndUpdate.
    - PrismVoice.h has setter + member; PrismVoice.cpp has applyPendingTuning call after lines 181-185 base-freq assignment AND before line 196 glide.setTarget.
    - CHANGELOG top entry uses `## v1.17.0 (2026-04-25)` style AND contains TRACK-03 phrase.
    - STATUS.md updated (last_updated = today; version = 1.17.0).
    - modules/registry.yaml `used_by:` contains `- plugin: O-Prism, version: 1.17.0`.
    - Tri-format build clean; freshly installed; auval validates; one atomic commit.
  </acceptance_criteria>
  <done>/improve cycle complete; atomic commit landed.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Build-side gate (D-08) — tri-format link + AU verify</name>
  <read_first>
    - scripts/verify-au-link.sh
    - plugins/O-Prism/CMakeLists.txt (PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / IS_SYNTH for AU type)
    - CLAUDE.md (cache clear protocol)
  </read_first>
  <action>
    1. Tri-format link: `ninja -C build O-Prism_VST3 O-Prism_AU O-Prism_Standalone 2>&1 | tee /tmp/o-prism-build.log` — exit 0; no `Undefined symbols ... Steinberg::*`.
    2. Fresh install mtimes: both `~/Library/Audio/Plug-Ins/VST3/O-Prism.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Prism.component` within last 5 min.
    3. AU runtime: `bash scripts/verify-au-link.sh O-Prism` exits 0.
    4. AU registration: `auval -a | grep -i 'O.Prism'` returns ≥1 line.

    Failure → triage per D-12.
  </action>
  <verify>
    <automated>! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-prism-build.log 2>/dev/null && test -d ~/Library/Audio/Plug-Ins/VST3/O-Prism.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/O-Prism.component && bash scripts/verify-au-link.sh O-Prism && auval -a 2>/dev/null | grep -i 'O.Prism'</automated>
  </verify>
  <acceptance_criteria>
    - ninja exit 0; no Steinberg undefined-symbol errors.
    - VST3 + AU bundles installed with recent mtimes.
    - verify-au-link.sh exits 0.
    - auval registers the AU.
  </acceptance_criteria>
  <done>Build-side gate PASS.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico 3-point smoke gate (D-07) — human-verified</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    O-Prism v1.17.0 with VST3 Note Expression integration. Tri-format built and freshly installed; AU validated. Ready for Dorico smoke.
  </what-built>
  <how-to-verify>
    Identical procedure to plan 24-01 Task 4, substituting `O-Prism` for `O-Bells`. Set Microtonality to "VST3 Note Expression" on the Dorico expression map.

    **Gate point 1:** quarter-sharp C4 → ~269.29 Hz (+50¢ above 261.63 Hz). PASS / FAIL with observed Hz.
    **Gate point 2:** no audible attack zipper on the same note. PASS / FAIL with observation.
    **Gate point 3:** chord (quarter-sharp C4 + natural E4) — only C4 detuned; E4 at natural 329.63 Hz. PASS / FAIL.

    Record results in `24-02-O-Prism-SUMMARY.md`.

    Type `approved` if all 3 PASS. If any FAIL, describe failure mode and observed pitch.
  </how-to-verify>
  <resume-signal>Type `approved` if 3-point gate PASS; else describe failure.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Write 24-02-O-Prism-SUMMARY.md and close plan</name>
  <read_first>
    - .planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md (format reference)
    - $HOME/.claude/get-shit-done/templates/summary.md
  </read_first>
  <action>
    Create `.planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md` mirroring the 24-01 structure: requirements (PROP-03 + TRACK-01..05), edits, build gate result, Dorico 3-point gate result with observed pitches, anomalies, "feeds 24-08-final-sweep-SUMMARY.md row 2 of 8".
  </action>
  <verify>
    <automated>test -f .planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md && grep -E 'PROP-03' .planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md && grep -iE 'PASS|FAIL' .planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md</automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY.md exists; references PROP-03 + all 5 TRACK reqs; records 3-point gate result; cites atomic commit SHA.
  </acceptance_criteria>
  <done>Plan 24-02 closed; ready for 24-03.</done>
</task>

</tasks>

<verification>
Per-plugin acceptance criteria applied to O-Prism v1.17.0 — see 24-INTEGRATION-MATRIX.md "Per-Plugin Acceptance Criteria Template", substituting `<Plugin>=O-Prism` and `<NewVersion>=1.17.0`. CHANGELOG style note: O-Prism uses `## v<ver> (date)` (parens, no brackets).
</verification>

<success_criteria>
PROP-03 satisfied; TRACK-01..05 satisfied; atomic commit landed; SUMMARY complete.
</success_criteria>

<output>
- `.planning/phases/24-propagate/24-02-O-Prism-SUMMARY.md`
- Updated O-Prism sources, CHANGELOG, STATUS, registry
- Freshly installed bundles
- One atomic commit
</output>
