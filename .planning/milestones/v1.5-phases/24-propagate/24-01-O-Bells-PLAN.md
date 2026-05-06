---
phase: 24-propagate
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - plugins/O-Bells/CMakeLists.txt
  - plugins/O-Bells/Source/PluginProcessor.h
  - plugins/O-Bells/Source/PluginProcessor.cpp
  - plugins/O-Bells/Source/BellVoice.h
  - plugins/O-Bells/Source/BellVoice.cpp
  - plugins/O-Bells/CHANGELOG.md
  - plugins/O-Bells/.planning/STATUS.md
  - modules/registry.yaml
autonomous: false
requirements: [PROP-01, TRACK-01, TRACK-02, TRACK-03, TRACK-04, TRACK-05]

must_haves:
  truths:
    - "O-Bells responds to Dorico VST3 Note Expression `kTuningTypeID` events on its quarter-sharp C4 input — pitch lands +50¢ above C4."
    - "First sample of the note is already at the tuned frequency (no attack zipper) — composition order applies NE BEFORE `calculateMultiStageCoefficients(fundamental)`."
    - "Polyphonic chord with mixed accidentals (quarter-sharp C4 + natural E4) detunes only the C4 voice — NE correlated by `noteId`, not by pitch."
    - "O-Bells builds clean across VST3 + AU + Standalone with zero `Undefined symbols ... Steinberg::*` regressions (per-format module-source convention from Phase 23 D-22..D-29 holds)."
    - "All 7 TRACK requirements satisfied: /improve workflow ran, version 4.0.0→4.1.0, CHANGELOG TRACK-03 phrase present, STATUS.md updated, freshly installed to ~/Library/Audio/Plug-Ins/, registry `used_by` includes `O-Bells`."
  artifacts:
    - path: "plugins/O-Bells/CMakeLists.txt"
      provides: "Module consumption + minor version bump"
      contains: "ouaricon_add_module(O-Bells note-expression)"
    - path: "plugins/O-Bells/Source/PluginProcessor.h"
      provides: "VST3Extensions member + override accessor"
      contains: "Ouaricon::NoteExpression::VST3Extensions vst3Extensions"
    - path: "plugins/O-Bells/Source/PluginProcessor.cpp"
      provides: "Voice wiring + drain call"
      contains: "vst3Extensions.drainAndUpdate("
    - path: "plugins/O-Bells/Source/BellVoice.cpp"
      provides: "Voice-side composition site"
      contains: "Ouaricon::NoteExpression::applyPendingTuning("
    - path: "plugins/O-Bells/CHANGELOG.md"
      provides: "TRACK-03 entry"
      contains: "adds VST3 Note Expression microtonal support for Dorico"
    - path: "modules/registry.yaml"
      provides: "Consumer registration"
      contains: "plugin: O-Bells"
    - path: "~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3"
      provides: "Freshly installed VST3 bundle"
    - path: "~/Library/Audio/Plug-Ins/Components/O-Bells.component"
      provides: "Freshly installed AU bundle"
    - path: ".planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md"
      provides: "Plan close-out + Dorico smoke 3-point gate result"
  key_links:
    - from: "plugins/O-Bells/Source/PluginProcessor.cpp processBlock"
      to: "vst3Extensions.drainAndUpdate()"
      via: "top of processBlock, after buffer.clear(), before synthesiser.renderNextBlock"
      pattern: "vst3Extensions\\.drainAndUpdate\\("
    - from: "plugins/O-Bells/Source/BellVoice.cpp::startNote"
      to: "Ouaricon::NoteExpression::applyPendingTuning"
      via: "after TuningEngine fundamental assignment (BellVoice.cpp:161-163), BEFORE calculateMultiStageCoefficients(fundamental)"
      pattern: "applyPendingTuning\\(.*pendingTuningSource"
    - from: "plugins/O-Bells/Source/PluginProcessor.cpp addVoice loop"
      to: "BellVoice::setPendingTuningSource(&vst3Extensions.getPendingTable())"
      via: "constructor, lines 545-550"
      pattern: "setPendingTuningSource\\(&vst3Extensions"
---

<objective>
Propagate the `note-expression` shared module into **O-Bells** as the Phase 24 propagation canary. O-Bells is the cleanest analog of the O-Lyrica reference shape (TuningEngine + classic `juce::Synthesiser` + parameterized `startNote`). Per D-11 it goes first to build momentum and validate the propagation playbook before the harder MPE plugins.

Purpose: Land PROP-01 + the full TRACK-01..05 discipline through the standard `/improve` workflow (D-03, TRACK-01) — version bump 4.0.0→4.1.0, CHANGELOG entry with the TRACK-03 phrase, STATUS.md update, clean tri-format build, fresh install per CLAUDE.md, AU verify gate, Dorico quarter-sharp smoke 3-point gate (D-07).

Output:
- Edited sources at `plugins/O-Bells/{CMakeLists.txt, Source/PluginProcessor.{h,cpp}, Source/BellVoice.{h,cpp}, CHANGELOG.md, .planning/STATUS.md}`.
- `modules/registry.yaml` `note-expression.used_by:` list gains `O-Bells`.
- Freshly installed `~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Bells.component`.
- One atomic commit (D-12) — `/improve` enforces this transactionally.
- `.planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md` recording build/AU/Dorico results.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/ROADMAP.md
@.planning/REQUIREMENTS.md
@.planning/STATE.md
@.planning/phases/24-propagate/24-CONTEXT.md
@.planning/phases/24-propagate/24-PATTERNS.md
@.planning/phases/24-propagate/24-INTEGRATION-MATRIX.md
@.planning/phases/23-extract/23-CONTEXT.md
@modules/tuning/note-expression/README.md
@modules/tuning/note-expression/cpp/NoteExpression.h
@CLAUDE.md
@plugins/O-Lyrica/CMakeLists.txt
@plugins/O-Lyrica/Source/PluginProcessor.h
@plugins/O-Lyrica/Source/PluginProcessor.cpp
@plugins/O-Lyrica/Source/HarpSynthVoice.h
@plugins/O-Lyrica/Source/HarpSynthVoice.cpp
@plugins/O-Lyrica/CHANGELOG.md
@plugins/O-Bells/CMakeLists.txt
@plugins/O-Bells/Source/PluginProcessor.h
@plugins/O-Bells/Source/PluginProcessor.cpp
@plugins/O-Bells/Source/BellVoice.h
@plugins/O-Bells/Source/BellVoice.cpp
@plugins/O-Bells/CHANGELOG.md
@plugins/O-Bells/.planning/STATUS.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
@.claude/skills/plugin-improve/SKILL.md
@scripts/verify-au-link.sh

<integrations>
[O-Bells row in 24-INTEGRATION-MATRIX.md](./24-INTEGRATION-MATRIX.md#o-bells)

**Match quality:** EXACT (TuningEngine + classic `juce::Synthesiser`). No structural variation.
**Float→double cast** required at helper boundary (`BellVoice` uses `float fundamental`).

Key contract surfaces from `modules/tuning/note-expression/cpp/NoteExpression.h`:
```cpp
namespace Ouaricon::NoteExpression {
    using PendingTuningTable = std::array<std::atomic<double>, 128>;
    inline double applyPendingTuning(PendingTuningTable& table, int midiNote, double freq);  // header-only, voice-side
    class VST3Extensions : public juce::VST3ClientExtensions {
        public:
            VST3Extensions();
            ~VST3Extensions();
            juce::VST3ClientExtensions::InterfaceResultWithDeleter queryIEditController(const juce::Steinberg::TUID iid) override;
            void onVst3RawEvent(const Vst3RawEvent& evt) override;
            void drainBlockEvents(std::vector<Vst3RawEvent>& outEvents);
            void drainAndUpdate();
            PendingTuningTable& getPendingTable();
    };
}
```

O-Lyrica reference shape (copy verbatim):
- `plugins/O-Lyrica/CMakeLists.txt:80` — `ouaricon_add_module(OLyrica note-expression)`
- `plugins/O-Lyrica/Source/PluginProcessor.h:22, 119, 200`
- `plugins/O-Lyrica/Source/PluginProcessor.cpp:506, 708`
- `plugins/O-Lyrica/Source/HarpSynthVoice.h:21, 88, 129`
- `plugins/O-Lyrica/Source/HarpSynthVoice.cpp:84-87, 138-147`

Spike landmines (must each be defended in this plan):
- **Pattern 1 — correlate by `noteId`, not pitch.** Module-internal in `updatePendingFromEvents`; voice-side composition is unaffected. Validate via 3-point smoke gate point #3.
- **Pattern 2 — apply tuning BEFORE DSP trigger** (`calculateMultiStageCoefficients(fundamental)` at `BellVoice.cpp:166`). Validate via 3-point smoke gate point #2 (no attack zipper).
- **Pattern 3 — 240 semitones full-scale conversion** lives inside the helper. Validate via 3-point smoke gate point #1 (+50¢ above C4 for quarter-sharp C4).
</integrations>
</context>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Pre-flight verification — confirm /improve preconditions</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md (the [O-Bells row](./24-INTEGRATION-MATRIX.md#o-bells))
    - plugins/O-Bells/.planning/STATUS.md (verify status is ✅ Working or 📦 Installed; /improve refuses 🚧 In Development)
    - plugins/O-Bells/CMakeLists.txt (current PLUGIN_VERSION; OuariconModules.cmake include presence)
    - .claude/skills/plugin-improve/SKILL.md (Phase 0.9 backup verification gate, Phase 4 CHANGELOG, Phase 5 build/install)
    - scripts/apply-juce-patches.sh (sanity check — patch already applied at /Users/taylorbrook/JUCE/, verified by JUCE-NE-PATCH marker)
  </read_first>
  <action>
    Confirm preconditions before invoking /improve:

    1. Verify `plugins/O-Bells/.planning/STATUS.md` shows status is ✅ Working or 📦 Installed (NOT 🚧 In Development). If 🚧 In Development, halt and report — /improve refuses.
    2. Verify `plugins/O-Bells/CMakeLists.txt` line 11 reads `PLUGIN_VERSION "4.0.0"` (matches PATTERNS.md baseline; this is the source of the 4.0.0 → 4.1.0 minor bump per D-19/TRACK-02).
    3. Verify `plugins/O-Bells/CMakeLists.txt` line 94 contains `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` (no pre-step needed for O-Bells, unlike O-Formant).
    4. Sanity-check the JUCE patch is applied:
       ```
       grep -l "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h \
                              /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
       ```
       Both files MUST list. If missing, run `bash scripts/apply-juce-patches.sh` before proceeding.
    5. Confirm git working tree is clean for `plugins/O-Bells/**` and `modules/registry.yaml` (atomic-commit invariant per D-12). If dirty, halt — surface the dirty files for resolution before /improve runs.
  </action>
  <verify>
    <automated>grep -l 'JUCE-NE-PATCH' /Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h /Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp && grep -E 'PLUGIN_VERSION "4\.0\.0"' plugins/O-Bells/CMakeLists.txt && grep -E 'include\(\$\{CMAKE_SOURCE_DIR\}/modules/cmake/OuariconModules\.cmake\)' plugins/O-Bells/CMakeLists.txt && git diff --quiet -- plugins/O-Bells modules/registry.yaml</automated>
  </verify>
  <acceptance_criteria>
    - JUCE patch markers present in both target JUCE files (grep returns both paths).
    - `plugins/O-Bells/CMakeLists.txt` baseline is `PLUGIN_VERSION "4.0.0"` and includes `OuariconModules.cmake`.
    - Git working tree clean for `plugins/O-Bells/**` and `modules/registry.yaml`.
    - Plugin status is ✅ Working or 📦 Installed (not 🚧 In Development).
  </acceptance_criteria>
  <done>All preconditions PASS — safe to invoke /improve.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Run /improve O-Bells — full plugin-improvement cycle (TRACK-01 execution)</name>
  <read_first>
    - .planning/phases/24-propagate/24-INTEGRATION-MATRIX.md ([O-Bells row](./24-INTEGRATION-MATRIX.md#o-bells))
    - .planning/phases/24-propagate/24-PATTERNS.md §"1. O-Bells (TuningEngine-composing — closest to O-Lyrica)" — exact file:line patches with code excerpts
    - plugins/O-Bells/Source/BellVoice.cpp lines 100-170 (current startNote body + base-frequency assignment at 161-163, calculateMultiStageCoefficients at 166)
    - plugins/O-Bells/Source/PluginProcessor.cpp lines 540-560 (addVoice loop) and lines 730-745 (processBlock top + buffer.clear)
    - plugins/O-Bells/Source/PluginProcessor.h around line 92 (`juce::Synthesiser synthesiser;` member) and line 69 (existing accessors)
    - plugins/O-Lyrica/Source/HarpSynthVoice.cpp:84-87, 138-147 (reference for setter body + composition site shape)
    - plugins/O-Lyrica/Source/PluginProcessor.cpp:506, 708 (reference for addVoice wiring + drainAndUpdate placement)
    - .claude/skills/plugin-improve/SKILL.md (workflow phases — Phase 0 specificity, Phase 0.9 backup, Phase 4 changelog, Phase 5 build, Phase 5.5 regression)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md (Patterns 1, 2, 3)
  </read_first>
  <action>
    Invoke `/improve O-Bells` with the LOCKED specification below. This is the canonical TRACK-01 execution path. The /improve skill handles backup, version bump, CHANGELOG automation, build, install, regression — but the spec it consumes (what to change, where, exact text) is defined here verbatim.

    **Specification handed to /improve (no discovery needed by /improve — all values pre-computed in PATTERNS.md):**

    ### Edit 1 — `plugins/O-Bells/CMakeLists.txt`

    Bump version (line 11):
    ```cmake
    PLUGIN_VERSION "4.0.0"  →  PLUGIN_VERSION "4.1.0"
    ```

    Add module call. Insert these two lines IMMEDIATELY AFTER line 94 (`include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`):
    ```cmake
    # Phase 24: VST3 Note Expression microtonal support (Dorico)
    ouaricon_add_module(O-Bells note-expression)
    ```

    ### Edit 2 — `plugins/O-Bells/Source/PluginProcessor.h`

    Add include after the existing last `#include` before any guard (after line 23):
    ```cpp
    #include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)
    ```

    Add public-section accessor near the existing `getTuningEngine()` accessor (around line 69):
    ```cpp
    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
    ```

    Add private-section member IMMEDIATELY AFTER `juce::Synthesiser synthesiser;` (line 92):
    ```cpp
    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    ```

    ### Edit 3 — `plugins/O-Bells/Source/PluginProcessor.cpp`

    Inside the `addVoice` loop (lines 545-550), insert the voice wiring line:
    ```cpp
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new BellVoice();
        voice->setTuningEngine(&tuningEngine);
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
        synthesiser.addVoice(voice);
    }
    ```

    At top of `processBlock` (line 739), AFTER `buffer.clear()` and BEFORE `synthesiser.renderNextBlock(...)`:
    ```cpp
    // VST3 Note Expression: drain raw event queue and correlate tuning deltas to NoteOn pitches.
    vst3Extensions.drainAndUpdate();
    ```

    ### Edit 4 — `plugins/O-Bells/Source/BellVoice.h`

    Add include after line 14 (`#include "BellSound.h"`):
    ```cpp
    #include "NoteExpression.h"  // modules/tuning/note-expression
    ```

    Add public setter (near `setTuningEngine` at line 31):
    ```cpp
    void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
    {
        pendingTuningSource = source;
    }
    ```

    Add private member (alongside `tuningEngine` member around line 210):
    ```cpp
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    ```

    ### Edit 5 — `plugins/O-Bells/Source/BellVoice.cpp` — composition site

    Existing base-frequency assignment (lines 161-163) is UNCHANGED:
    ```cpp
    float fundamental = tuningEngine
        ? static_cast<float>(tuningEngine->getFrequency(midiNoteNumber))
        : static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
    ```

    Insert IMMEDIATELY AFTER line 163, BEFORE `calculateMultiStageCoefficients(fundamental)` at line 166 (Pattern 2: apply tuning BEFORE DSP trigger — this prevents attack zipper):
    ```cpp
    // VST3 Note Expression tuning delta (Dorico microtonal).
    // Compose multiplicatively after TuningEngine, before any DSP coefficient setup.
    // Helper uses exchange(0.0) internally so retriggered notes don't inherit stale offsets.
    if (pendingTuningSource != nullptr)
    {
        fundamental = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
            *pendingTuningSource, midiNoteNumber, static_cast<double>(fundamental)));
    }
    ```

    > **Cast rationale:** `BellVoice` uses `float fundamental`; helper signature is `double(PendingTuningTable&, int, double)`. Cast through `double` at the call boundary.

    ### Edit 6 — `plugins/O-Bells/CHANGELOG.md`

    Insert at top (above current `## [4.0.0]` entry). TRACK-03 mandates the EXACT phrase "adds VST3 Note Expression microtonal support for Dorico":
    ```markdown
    ## [4.1.0] - 2026-04-25

    ### Added

    - **Adds VST3 Note Expression microtonal support for Dorico.** O-Bells now responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling correct microtonal playback of quarter-tones, third-tones, and arbitrary tuning deltas authored in Dorico's tonality system. End users must set Microtonality to "VST3 Note Expression" on the assigned expression map (see O-Lyrica 2.3.0 for the procedure).
    - **Shared `note-expression` module adoption.** O-Bells consumes the Ouaricon module at `modules/tuning/note-expression` (v1.0.0), same shape as O-Lyrica v2.3.0.

    ### Technical notes

    - **Composition with TuningEngine.** `BellVoice::startNote` computes the fundamental via `TuningEngine::getFrequency(midi)` first, then applies the NE semitone delta via `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)` before `calculateMultiStageCoefficients()`.
    - **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/BellVoice.{h,cpp}`, `CMakeLists.txt`.
    - **Version bump rationale:** MINOR (4.0.0 → 4.1.0) — new user-visible feature, backward compatible, no preset impact.
    ```

    ### Edit 7 — `plugins/O-Bells/.planning/STATUS.md`

    /improve's Phase 4 STATUS-update logic updates the YAML front-matter `last_updated`, `version`, and `next_action`. DO NOT manually rewrite the file shape. Confirm post-/improve that the timestamp reflects today and version is 4.1.0.

    ### Edit 8 — `modules/registry.yaml`

    /improve invokes `/module-add note-expression` for O-Bells, which appends to `note-expression.used_by:` (currently `[OLyrica@2.3.0]`). After /improve completes, the list MUST include:
    ```yaml
    - plugin: O-Bells
      version: 4.1.0
    ```

    ### /improve invocation

    Run with auto-confirmation flags (express path) since the spec is fully locked:
    ```
    /improve O-Bells
    ```
    When /improve prompts for the change description, paste:
    > "Adds VST3 Note Expression microtonal support for Dorico via shared note-expression module adoption (Phase 24, see .planning/phases/24-propagate/24-01-O-Bells-PLAN.md)."

    /improve's Phase 5 invokes `build-automation` which runs `ninja O-Bells_VST3 O-Bells_AU O-Bells_Standalone` (per CLAUDE.md `Build Targets`) and the AU cache clear + fresh install sequence. Phase 5.5 runs the plugin's standard regression baseline (no NE-specific tests required — when no NE events arrive, `applyPendingTuning` returns the original frequency unchanged via `exchange(0.0)` → `pow(2, 0)` = 1.0, so 12-TET behavior is preserved by construction per D-09).

    If /improve fails at any phase, halt the plan and triage in-place per D-12 (atomic-commit invariant — plan-local fix, no separate plan unless the failure is structural at module level).
  </action>
  <verify>
    <automated>
      grep -E 'ouaricon_add_module\(O-Bells note-expression\)' plugins/O-Bells/CMakeLists.txt && \
      grep -E 'PLUGIN_VERSION "4\.1\.0"' plugins/O-Bells/CMakeLists.txt && \
      grep -E 'Ouaricon::NoteExpression::VST3Extensions vst3Extensions' plugins/O-Bells/Source/PluginProcessor.h && \
      grep -E 'getVST3ClientExtensions.*&vst3Extensions' plugins/O-Bells/Source/PluginProcessor.h && \
      grep -E 'vst3Extensions\.drainAndUpdate\(' plugins/O-Bells/Source/PluginProcessor.cpp && \
      grep -E 'setPendingTuningSource\(&vst3Extensions' plugins/O-Bells/Source/PluginProcessor.cpp && \
      grep -E 'Ouaricon::NoteExpression::applyPendingTuning\(' plugins/O-Bells/Source/BellVoice.cpp && \
      grep -E 'pendingTuningSource' plugins/O-Bells/Source/BellVoice.h && \
      grep -F 'adds VST3 Note Expression microtonal support for Dorico' plugins/O-Bells/CHANGELOG.md && \
      grep -E 'plugin: O-Bells' modules/registry.yaml
    </automated>
  </verify>
  <acceptance_criteria>
    - `plugins/O-Bells/CMakeLists.txt` line 11 reads `PLUGIN_VERSION "4.1.0"`.
    - `plugins/O-Bells/CMakeLists.txt` contains `ouaricon_add_module(O-Bells note-expression)`.
    - `plugins/O-Bells/Source/PluginProcessor.h` contains `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` (private member) and `getVST3ClientExtensions() override` returning `&vst3Extensions`.
    - `plugins/O-Bells/Source/PluginProcessor.cpp` contains `voice->setPendingTuningSource(&vst3Extensions.getPendingTable())` in addVoice loop AND `vst3Extensions.drainAndUpdate()` in processBlock.
    - `plugins/O-Bells/Source/BellVoice.h` contains `setPendingTuningSource` setter and `pendingTuningSource` member.
    - `plugins/O-Bells/Source/BellVoice.cpp` contains `Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, static_cast<double>(fundamental))` AFTER the existing line 163 `fundamental` assignment AND BEFORE `calculateMultiStageCoefficients(fundamental)` at line 166.
    - `plugins/O-Bells/CHANGELOG.md` top entry contains the exact phrase `adds VST3 Note Expression microtonal support for Dorico` (TRACK-03 verbatim).
    - `plugins/O-Bells/.planning/STATUS.md` `last_updated` field timestamps today; `version` reflects 4.1.0 (TRACK-04).
    - `modules/registry.yaml` `note-expression` module's `used_by:` list contains an entry with `plugin: O-Bells` and `version: 4.1.0`.
    - /improve completed Phase 5 build successfully — `build/plugins/O-Bells/O-Bells_artefacts/Release/{VST3/O-Bells.vst3, AU/O-Bells.component, Standalone/O-Bells.app}` all exist; ninja exit 0; no `Undefined symbols ... Steinberg::*` in build log.
    - /improve completed Phase 5 install successfully — `~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Bells.component` mtime within last 5 minutes; AU cache cleared per CLAUDE.md.
    - One atomic git commit landed the entire diff (D-12).
  </acceptance_criteria>
  <done>/improve cycle complete; all source edits + version bump + CHANGELOG + STATUS + registry + build + install in one commit.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Build-side gate — tri-format link clean + AU verify (D-08)</name>
  <read_first>
    - scripts/verify-au-link.sh (auval gate; reads PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE from CMakeLists.txt)
    - plugins/O-Bells/CMakeLists.txt (PLUGIN_CODE, PLUGIN_MANUFACTURER_CODE, IS_SYNTH for AU type derivation)
    - CLAUDE.md (Plugin Cache Clearing protocol — must be exercised before any DAW open)
  </read_first>
  <action>
    Validate the build-side gate (D-08, inherited from D-30/D-31). /improve's Phase 5 should have already executed this; this task is the explicit verification checkpoint.

    1. Tri-format link check:
       ```bash
       ninja -C build O-Bells_VST3 O-Bells_AU O-Bells_Standalone 2>&1 | tee /tmp/o-bells-build.log
       ! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-bells-build.log
       ```
       MUST exit 0 AND produce no Steinberg-symbol errors. (Per-format module-source convention from Phase 23 D-22..D-29 should make this automatic, but this gate enforces it as a regression check — Plan 23-04's defect class.)

    2. Fresh install sanity (CLAUDE.md protocol — /improve should have run this, re-verify mtimes):
       ```bash
       stat -f "%m %N" ~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3 \
                       ~/Library/Audio/Plug-Ins/Components/O-Bells.component
       ```
       Both mtimes must be within the last 5 minutes.

    3. AU runtime check (auval load test):
       ```bash
       bash scripts/verify-au-link.sh O-Bells
       ```
       MUST exit 0. Codes are auto-parsed from `plugins/O-Bells/CMakeLists.txt` (PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / AU type from IS_SYNTH).

    4. AU registration sanity:
       ```bash
       auval -a | grep -i 'O.Bells'
       ```
       MUST return at least one line.

    If any gate fails, halt the plan and triage in-place (D-12). Module-level structural failure → escalate to a `24-NN-fix-PLAN.md` per Phase 23's playbook.
  </action>
  <verify>
    <automated>
      ! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-bells-build.log 2>/dev/null && \
      test -d ~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3 && \
      test -d ~/Library/Audio/Plug-Ins/Components/O-Bells.component && \
      bash scripts/verify-au-link.sh O-Bells && \
      auval -a 2>/dev/null | grep -i 'O.Bells'
    </automated>
  </verify>
  <acceptance_criteria>
    - Tri-format ninja build exits 0 with no `Undefined symbols for architecture arm64 ... Steinberg::*` errors in the log.
    - `~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3` exists; mtime within last 5 minutes.
    - `~/Library/Audio/Plug-Ins/Components/O-Bells.component` exists; mtime within last 5 minutes.
    - `bash scripts/verify-au-link.sh O-Bells` exits 0 (auval validates AU loads).
    - `auval -a | grep -i 'O.Bells'` returns at least one line (AU is registered with macOS).
  </acceptance_criteria>
  <done>Build-side gate PASS — safe to proceed to Dorico smoke test.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico quarter-sharp smoke 3-point gate (D-07) — human-verified</name>
  <read_first>
    - .planning/phases/24-propagate/24-CONTEXT.md §D-07 (3-point gate spec)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md (Patterns 1, 2, 3 — the three failure modes the gate validates)
    - plugins/O-Lyrica/CHANGELOG.md (LYR-03 5-test reference; we run a faster 3-point subset here per D-07)
  </read_first>
  <action>Execute the human-verified Dorico smoke procedure described in <how-to-verify>. The task pauses for the user to perform the steps and report PASS/FAIL via the resume-signal. No autonomous code action is performed by the executor for this task.</action>
  <what-built>
    O-Bells v4.1.0 with VST3 Note Expression integration via shared `note-expression` module. VST3 + AU + Standalone built and freshly installed; AU verified by auval. Ready for human-verified Dorico smoke test.
  </what-built>
  <how-to-verify>
    Execute the 3-point Dorico quarter-sharp smoke gate (D-07). Each point validates one of the three Phase 23 spike landmines.

    **Setup (one-time):**
    1. Open Dorico Pro 5+ (or current installed version).
    2. Create a new project, add an instrumental staff with O-Bells assigned as the playback instrument.
       - Endpoint Setup → Add VST → O-Bells → set Microtonality to "VST3 Note Expression" on the assigned expression map (CRITICAL — "Auto" picks pitch-bend for non-Steinberg VST3s, which would silently bypass NE).

    **Gate point 1 — Pitch lands at +50¢ above C4 (Pattern 3: 240-semitone full-scale conversion).**
    1. Notate a single quarter-sharp C4 (use the "+" accidental in Dorico's accidental palette).
    2. Play. Listen via a tuner plugin in your audio chain (or audible reference against a 277.18 Hz reference tone — that's +50¢ above C4=261.63 Hz).
    3. PASS criterion: pitch is **261.63 × 2^(0.5/12) ≈ 269.29 Hz** (+50¢ above C4). FAIL criterion: pitch reads C4 (261.63 Hz, NE skipped) or any other value.

    **Gate point 2 — No attack zipper (Pattern 2: apply tuning BEFORE DSP trigger).**
    1. Same project; play the same quarter-sharp C4 with a moderate-attack patch.
    2. PASS criterion: the very first sample of the note is at +50¢ — no audible glide/zipper from C4 up to the tuned pitch. FAIL criterion: audible pitch ramp at note onset (means NE was applied AFTER `calculateMultiStageCoefficients`, so coefficients were computed for untuned C4 and then drift toward the NE pitch).

    **Gate point 3 — Polyphonic chord NE correlated by `noteId`, not pitch (Pattern 1).**
    1. Notate a two-note chord: quarter-sharp C4 + natural E4 (no accidental on the E).
    2. Play simultaneously.
    3. PASS criterion: only the C4 voice is detuned to +50¢; the E4 plays at its natural 12-TET frequency (329.63 Hz). FAIL criterion: both voices detuned (means correlation by pitch instead of noteId), or only the wrong voice detuned, or neither detuned.

    Record the three gate results (PASS/FAIL with observed Hz values where applicable) in `24-01-O-Bells-SUMMARY.md`.

    Type `approved` if all 3 gate points PASS. If any point FAILS, describe the failure mode and the observed pitch — triage will determine whether the fix is plan-local (D-12) or structural (escalate to 24-NN-fix-PLAN.md).
  </how-to-verify>
  <resume-signal>Type `approved` if 3-point gate PASS, or describe the failure mode and observed pitch values.</resume-signal>
</task>

<task type="auto" tdd="false">
  <name>Task 5: Write 24-01-O-Bells-SUMMARY.md and close plan</name>
  <read_first>
    - .planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md (format reference for Dorico smoke results)
    - .planning/phases/24-propagate/24-CONTEXT.md §D-10 (PASS/FAIL line + observed values format)
    - $HOME/.claude/get-shit-done/templates/summary.md (GSD summary template)
  </read_first>
  <action>
    Create `.planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md` documenting:

    1. **Plan close-out header:** plan id, phase, completion date, atomic commit SHA.
    2. **Requirements claimed:** PROP-01 (with Dorico smoke result inline), TRACK-01..05 (with /improve cycle completion confirmation).
    3. **Edits landed:** itemized list (8 edits per Task 2 spec).
    4. **Build-side gate result (D-08):** ninja exit codes, AU verify-link output, install paths + mtimes.
    5. **Dorico smoke 3-point gate result (D-07):** for each gate point, PASS/FAIL with observed pitch values.
    6. **Anomalies/notes:** anything to remember when planning the next plugin (e.g., float→double cast confirmed working; addVoice loop existed at expected lines).
    7. **Aggregation hook:** "feeds 24-08-final-sweep-SUMMARY.md row 1 of 8".

    Format mirrors `23-04-version-readme-dorico-smoketest-SUMMARY.md`.
  </action>
  <verify>
    <automated>test -f .planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md && grep -E 'PROP-01' .planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md && grep -E 'TRACK-0[1-5]' .planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md && grep -iE 'PASS|FAIL' .planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md</automated>
  </verify>
  <acceptance_criteria>
    - SUMMARY.md exists at the canonical path.
    - SUMMARY.md references PROP-01 and all 5 TRACK requirements.
    - SUMMARY.md records 3-point Dorico gate result (PASS or FAIL with observed values).
    - SUMMARY.md cites the atomic commit SHA from /improve's transaction.
  </acceptance_criteria>
  <done>Plan 24-01 complete — O-Bells propagation closed, ready for plan 24-02.</done>
</task>

</tasks>

<verification>
**Per-plugin acceptance criteria (from 24-INTEGRATION-MATRIX.md template, applied to O-Bells v4.1.0):**

- `plugins/O-Bells/CMakeLists.txt` contains `ouaricon_add_module(O-Bells note-expression)`
- `plugins/O-Bells/CMakeLists.txt` `PLUGIN_VERSION "4.1.0"` (bumped from 4.0.0)
- `plugins/O-Bells/Source/PluginProcessor.h` contains `Ouaricon::NoteExpression::VST3Extensions vst3Extensions`
- `plugins/O-Bells/Source/PluginProcessor.cpp` contains `getVST3ClientExtensions` returning `&vst3Extensions` AND a `vst3Extensions.drainAndUpdate(` call from `processBlock`
- `plugins/O-Bells/Source/BellVoice.cpp` contains `Ouaricon::NoteExpression::applyPendingTuning(`
- `plugins/O-Bells/CHANGELOG.md` top entry contains `adds VST3 Note Expression microtonal support for Dorico`
- `plugins/O-Bells/.planning/STATUS.md` updated entry timestamped today, version 4.1.0
- `modules/registry.yaml` `note-expression.used_by:` list contains `O-Bells`
- `ninja O-Bells_VST3 O-Bells_AU O-Bells_Standalone` exits 0 with no `Undefined symbols ... Steinberg::*`
- `~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3` mtime within last 5 minutes
- `~/Library/Audio/Plug-Ins/Components/O-Bells.component` mtime within last 5 minutes
- `scripts/verify-au-link.sh O-Bells` exits 0
- `auval -a | grep -i 'O.Bells'` returns at least one line
- Dorico 3-point smoke gate PASS recorded in `24-01-O-Bells-SUMMARY.md`
</verification>

<success_criteria>
PROP-01 satisfied (O-Bells consumes shared module; passes Dorico quarter-sharp smoke). TRACK-01..05 satisfied (/improve cycle ran; minor version bump; CHANGELOG TRACK-03 phrase; STATUS.md updated; freshly installed). Atomic commit landed. SUMMARY.md complete. Phase 24 propagation pattern proven on the canary; safe to proceed to plan 24-02 (O-Prism).
</success_criteria>

<output>
After completion, the following files exist:
- `.planning/phases/24-propagate/24-01-O-Bells-SUMMARY.md`
- Updated `plugins/O-Bells/{CMakeLists.txt, Source/PluginProcessor.{h,cpp}, Source/BellVoice.{h,cpp}, CHANGELOG.md, .planning/STATUS.md}`
- Updated `modules/registry.yaml`
- Freshly installed `~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Bells.component`
- One atomic git commit landing all of the above (per D-12)
</output>
