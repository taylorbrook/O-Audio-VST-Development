---
phase: 23-extract
plan: 04
type: execute
wave: 3
depends_on: ["23-03"]
files_modified:
  - plugins/O-Lyrica/CMakeLists.txt
  - plugins/O-Lyrica/CHANGELOG.md
  - modules/tuning/note-expression/README.md
autonomous: false
requirements:
  - MOD-05
  - LYR-03
  - LYR-04
tags: [version-bump, changelog, readme, dorico, smoke-test, human-verify]

must_haves:
  truths:
    - "plugins/O-Lyrica/CMakeLists.txt juce_add_plugin(OLyrica ...) block carries VERSION \"2.3.0\" (D-19)."
    - "plugins/O-Lyrica/CHANGELOG.md has a new [2.3.0] entry dated 2026-04-24 documenting shared-module adoption + microtonal NE support."
    - "modules/tuning/note-expression/README.md covers: Quick Start, Features, Installation (JUCE patch apply + CMake include + processor wiring), Usage in Audio Processing, Dorico End-User Setup, JUCE Patch Management, Integration Approach."
    - "O-Lyrica is rebuilt and installed to system plugin folders per CLAUDE.md Plugin Cache Clearing protocol (AU cache cleared, old bundles removed, fresh VST3 + AU copied)."
    - "auval -a | grep -i lyrica shows the AU component."
    - "Dorico quarter-sharp smoke test passes: quarter-sharp C4 above middle C plays at +50 cents (expected 269.29 Hz vs 261.63 Hz), no attack zipper, NE events correlated by noteId, existing 12-TET / Scala tunings still work alongside NE offsets."
  artifacts:
    - path: "plugins/O-Lyrica/CMakeLists.txt"
      provides: "juce_add_plugin() carries VERSION \"2.3.0\""
    - path: "plugins/O-Lyrica/CHANGELOG.md"
      provides: "[2.3.0] entry with Added / Changed / Removed / Technical notes sections"
    - path: "modules/tuning/note-expression/README.md"
      provides: "Consumer integration documentation per MOD-05"
  key_links:
    - from: "plugins/O-Lyrica/CHANGELOG.md"
      to: "plugins/O-Lyrica/CMakeLists.txt VERSION"
      via: "matching 2.3.0"
      pattern: "2\\.3\\.0"
    - from: "modules/tuning/note-expression/README.md"
      to: "scripts/apply-juce-patches.sh"
      via: "JUCE patch setup instructions"
      pattern: "apply-juce-patches\\.sh"
    - from: "Dorico expression-map setup instructions (README + CHANGELOG)"
      to: "VST3 Note Expression microtonality setting"
      via: "Dorico -> Library -> Expression Maps -> Microtonality -> VST3 Note Expression"
      pattern: "VST3 Note Expression"
---

<objective>
Wrap up Phase 23: bump O-Lyrica version to 2.3.0 (CMakeLists + CHANGELOG), write the comprehensive module README (MOD-05), and execute the Dorico quarter-sharp smoke test per CLAUDE.md Plugin Cache Clearing protocol (LYR-03). This is the final plan in the phase; on completion, all 12 phase requirements are satisfied.

Purpose: MOD-05 (module README documents consumer integration + JUCE patch + Dorico end-user setup), LYR-03 (Dorico quarter-sharp smoke test passes — pitch at +50¢, no attack zipper, NE correlated by noteId, TuningEngine composes cleanly), LYR-04 (O-Lyrica version bump + CHANGELOG entry).

Output:
- `plugins/O-Lyrica/CMakeLists.txt` with `VERSION "2.3.0"` in `juce_add_plugin(OLyrica ...)`
- `plugins/O-Lyrica/CHANGELOG.md` with [2.3.0] entry
- `modules/tuning/note-expression/README.md` expanded to ~150 lines covering all required sections
- Fresh install at `~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3` and `~/Library/Audio/Plug-Ins/Components/OLyrica.component`
- Human-verified Dorico smoke-test outcome
</objective>

<execution_context>
@.claude/get-shit-done/workflows/execute-plan.md
@.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/phases/23-extract/23-CONTEXT.md
@.planning/phases/23-extract/23-PATTERNS.md
@.planning/phases/23-extract/23-03-SUMMARY.md
@CLAUDE.md
@plugins/O-Lyrica/CMakeLists.txt
@plugins/O-Lyrica/CHANGELOG.md
@modules/tuning/scala-tuning-engine/README.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md
</context>

<threat_model>
No new trust boundaries. The Dorico smoke test is the human-verify gate — audit is outcome-based (pitch, no zipper, noteId correlation), not code-based. Threats:
- T-23-10 (Denial — user skips cache clear): mitigate via CLAUDE.md protocol codified in Task 3's action block; explicit `killall -9 AudioComponentRegistrar` + `rm -rf ~/Library/Caches/AudioUnitCache/` commands.
- T-23-11 (Tampering — stale bundle masks regression): mitigate via `rm -rf` of old bundle before `cp -R` of fresh bundle.
- T-23-12 (Repudiation — "works on my machine"): mitigate via human-verify checkpoint requiring the user to confirm actual Dorico output (pitch, attack transient, tuning composition).
</threat_model>

<tasks>

<task type="auto" tdd="false">
  <name>Task 1: Version bump (CMakeLists VERSION + CHANGELOG entry)</name>
  <files>
    plugins/O-Lyrica/CMakeLists.txt,
    plugins/O-Lyrica/CHANGELOG.md
  </files>
  <read_first>
    - plugins/O-Lyrica/CMakeLists.txt lines 6-16 (juce_add_plugin block; currently has no VERSION field per pattern mapper)
    - plugins/O-Lyrica/CHANGELOG.md lines 1-30 (style reference — [2.2.2] entry uses Added/Changed/Removed/Technical notes headings)
    - .planning/phases/23-extract/23-CONTEXT.md (D-19: bump 2.2.2 → 2.3.0 minor, new user-visible feature)
    - .planning/phases/23-extract/23-PATTERNS.md §plugins/O-Lyrica/CHANGELOG.md (MODIFY) and §plugins/O-Lyrica/CMakeLists.txt — version bump (D-19)
  </read_first>
  <action>
    **Edit 1 — `plugins/O-Lyrica/CMakeLists.txt`:** Inside `juce_add_plugin(OLyrica ...)` block, add a `VERSION "2.3.0"` line. The current block has 10 argument lines and no VERSION field. Insert AFTER `PRODUCT_NAME "O-Lyrica${OUARICON_DEV_SUFFIX}"` (line 11) and BEFORE `IS_SYNTH TRUE` (line 12):
    ```cmake
        VERSION "2.3.0"
    ```
    Resulting block (showing the 3-line context):
    ```cmake
        PRODUCT_NAME "O-Lyrica${OUARICON_DEV_SUFFIX}"
        VERSION "2.3.0"
        IS_SYNTH TRUE
    ```
    Do NOT change any other lines in the juce_add_plugin block.

    **Edit 2 — `plugins/O-Lyrica/CHANGELOG.md`:** Insert a new `## [2.3.0]` entry ABOVE the existing `## [2.2.2]` entry at line 5. Use this content (style matches existing entries):

    ```markdown
    ## [2.3.0] - 2026-04-24

    ### Added

    - **VST3 Note Expression microtonal support for Dorico.** O-Lyrica now responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling correct microtonal playback of quarter-tones, third-tones, and arbitrary tuning deltas authored in Dorico's tonality system. Required procedure for end users: `Dorico -> Library -> Expression Maps...` -> duplicate an existing map -> set **Microtonality** to **"VST3 Note Expression"** -> assign the new map to O-Lyrica's Endpoint Setup channel. Default expression maps route microtones to VST2 detune or pitch bend — neither reaches VST3 plugins, so without this setup microtonal playback falls back to 12-TET.
    - **Shared `note-expression` module adoption.** O-Lyrica is the reference consumer for the new Ouaricon module at `modules/tuning/note-expression` (v1.0.0). The 128-slot pending-tuning table, NEC advertisement, raw-event drain, and voice-side pitch-offset helper now live in the module rather than O-Lyrica's own sources. Phase 24 will propagate the same module to O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-IntonationPad, and O-Prism.

    ### Changed

    - **Composition with existing tuning.** Voice code now computes its base frequency via `TuningEngine::getFrequency(midi)` FIRST, then applies the NE semitone delta via `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)`. This is multiplicatively correct for any base tuning (12-TET, Scala, MTS-ESP future) and satisfies the "no raw `std::pow` in voice" constraint — the `pow(2, semis/12)` call lives inside the module helper.
    - **Spike diagnostic code stripped.** `OLyrica::detail::neTrace` and `detail::iidToHex` helpers plus all `neTrace(...)` call sites removed from `PluginProcessor.cpp` and `HarpSynthVoice.cpp`. `<fstream>` include removed. No more audio-thread file I/O (the spike wrote `/tmp/olyrica-ne-trace.log` synchronously).

    ### Removed

    - **`Source/VST3/NoteExpressionSupport.h`** — deleted entirely. Replaced by the shared module (no plugin-local shim kept; sets the clean reference shape for Phase 24 adopters).

    ### Technical notes

    - **JUCE patch required:** the NE path only works if `/Users/taylorbrook/JUCE` has the `JUCE-NE-PATCH` markers applied. After a JUCE upgrade, run `./scripts/apply-juce-patches.sh`. CMake verifies the markers at configure time and fails fast if missing.
    - **Composition order is load-bearing:** `applyPendingTuning` must run AFTER the TuningEngine lookup but BEFORE `stringModel.trigger()`, so the first waveguide sample sizes to the tuned frequency. Landmine 4 of `vst3-note-expression-dorico.md`.
    - **Version bump rationale:** MINOR (v2.2.2 -> v2.3.0) — new user-visible feature (Dorico microtonal playback), backward compatible, no preset impact, no parameter changes.
    - **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/HarpSynthVoice.{h,cpp}`, `CMakeLists.txt`.
    - **Files deleted:** `Source/VST3/NoteExpressionSupport.h`.
    ```

    Leave the existing [2.2.2] / [2.2.1] / [2.2.0] entries unchanged below the new entry.
  </action>
  <verify>
    <automated>grep -c 'VERSION "2.3.0"' plugins/O-Lyrica/CMakeLists.txt | grep -q '^1$' && grep -c "^## \[2.3.0\] - 2026-04-24$" plugins/O-Lyrica/CHANGELOG.md | grep -q '^1$'</automated>
  </verify>
  <acceptance_criteria>
    - `grep -c 'VERSION "2.3.0"' plugins/O-Lyrica/CMakeLists.txt` returns `1`
    - `awk '/juce_add_plugin\(OLyrica/,/^\)/' plugins/O-Lyrica/CMakeLists.txt | grep -c 'VERSION "2.3.0"'` returns `1` (VERSION is INSIDE the juce_add_plugin block, not floating)
    - `grep -c "^## \[2.3.0\] - 2026-04-24$" plugins/O-Lyrica/CHANGELOG.md` returns `1`
    - `grep -c "^## \[2.2.2\] - 2026-04-13$" plugins/O-Lyrica/CHANGELOG.md` returns `1` (old entry still present)
    - CHANGELOG [2.3.0] entry contains all four required sections: `awk '/^## \[2.3.0\]/,/^## \[2.2.2\]/' plugins/O-Lyrica/CHANGELOG.md | grep -cE '^### (Added|Changed|Removed|Technical notes)$'` returns `4`
    - CHANGELOG [2.3.0] entry mentions shared-module adoption AND Dorico NE support: `awk '/^## \[2.3.0\]/,/^## \[2.2.2\]/' plugins/O-Lyrica/CHANGELOG.md | grep -c "note-expression\|Note Expression"` returns at least `3`
    - CHANGELOG [2.3.0] entry is above [2.2.2]: line number of `## [2.3.0]` < line number of `## [2.2.2]`
  </acceptance_criteria>
  <done>VERSION field added; CHANGELOG 2.3.0 entry inserted with all required sections; old entries untouched.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Write comprehensive module README (MOD-05)</name>
  <files>modules/tuning/note-expression/README.md</files>
  <read_first>
    - modules/tuning/scala-tuning-engine/README.md (~231 lines — structure template per pattern mapper; aim for ~150 lines)
    - .claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md landmine 3 (4-step Dorico expression-map instructions)
    - .planning/phases/23-extract/23-CONTEXT.md (MOD-05 requirements; Claude's discretion covers README structure)
    - .planning/phases/23-extract/23-PATTERNS.md §modules/tuning/note-expression/README.md (CREATE) — template sections
    - scripts/apply-juce-patches.sh (reference the actual script path)
    - modules/tuning/note-expression/module.yaml (match the module metadata in documentation)
  </read_first>
  <action>
    Replace the Plan-01 stub `modules/tuning/note-expression/README.md` with a comprehensive README (~150 lines) covering all three MOD-05 requirements: (a) consumer integration, (b) required local JUCE patch, (c) end-user Dorico expression-map setup.

    Required section hierarchy (all H2 unless noted):

    **H1: `# note-expression v1.0.0`**

    **Intro paragraph (2-3 sentences):** Explain what the module does — VST3 Note Expression (kTuningTypeID) support for Dorico microtonal playback. State that it's header-only C++ under `Ouaricon::NoteExpression` namespace. Note that it owns the 128-slot pending tuning table, advertises the NEC, drains the patched JUCE wrapper's raw-event queue, and provides a one-line voice helper.

    **## Quick Start**

    Three numbered steps:
    1. Apply the JUCE patch once: `./scripts/apply-juce-patches.sh`
    2. Add the module to your plugin's CMakeLists.txt:
       ```cmake
       include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
       ouaricon_add_module(YourPlugin note-expression)
       ```
    3. See `Installation` and `Integration Approach` below for processor + voice wiring.

    **## Features**

    Bullet list (4 items):
    - `Ouaricon::NoteExpression::Controller` — advertises `kTuningTypeID` to hosts on `IEditController` query.
    - `Ouaricon::NoteExpression::VST3Extensions` — subclass of `juce::VST3ClientExtensions` that owns the 128-slot `PendingTuningTable`, drains raw VST3 events, and dispatches `queryIEditController` to the NEC.
    - `Ouaricon::NoteExpression::updatePendingFromEvents` — two-pass drain/correlate helper that maps NE events to MIDI pitches via noteId.
    - `Ouaricon::NoteExpression::applyPendingTuning` — one-line voice helper; composes multiplicatively with any base frequency (TuningEngine, MIDI-standard, MTS-ESP future).

    **## Installation**

    Three sub-sections (H3):

    ### 1. Apply the JUCE patch

    Brief explanation: upstream JUCE 8.0.4 drops `kNoteExpressionValueEvent` and noteId-tagged NoteOn/NoteOff events in `MidiEventList::toMidiBuffer`. The local patch at `scripts/juce-patches/note-expression-juce-8.0.4.patch` adds `VST3ClientExtensions::onVst3RawEvent` so the plugin sees these events. The idempotent applier:
    ```bash
    ./scripts/apply-juce-patches.sh
    ```
    Skips application if the `JUCE-NE-PATCH` marker is already present. After a JUCE upgrade, regenerate the patch file (procedure documented in the patch header) and re-run the script.

    ### 2. Register the module in your plugin's CMakeLists.txt

    ```cmake
    include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
    ouaricon_add_module(YourPlugin note-expression)
    ```
    This auto-includes the module's `module.cmake`, which fatal-errors at configure time if the JUCE-NE-PATCH marker is missing from `/Users/taylorbrook/JUCE` (the `_NE_JUCE_ROOT` default; overridable via `JUCE_DIR` env var).

    ### 3. Wire the processor

    Show a code block covering: include the header, declare the member, return it from `getVST3ClientExtensions()`, wire each voice:
    ```cpp
    #include "NoteExpression.h"

    class YourPluginProcessor : public juce::AudioProcessor {
    public:
        juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
            buffer.clear();
            vst3Extensions.drainAndUpdate();   // one call: drain + correlate
            synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
        }

        // In your voice-init loop:
        //   voice->setPendingTuningSource(&vst3Extensions.getPendingTable());

    private:
        Ouaricon::NoteExpression::VST3Extensions vst3Extensions;
    };
    ```

    ### 4. Wire the voice

    Code block (bare minimum):
    ```cpp
    class YourVoice : public juce::SynthesiserVoice {
    public:
        void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* t) { pendingTuningSource = t; }

        void startNote(int midi, float vel, juce::SynthesiserSound*, int) override {
            // 1) compute base frequency via your tuning pipeline (TuningEngine, MIDI, humanize, etc.)
            double currentFrequency = computeBaseFrequency(midi);

            // 2) apply the NE semitone delta — AFTER base frequency, BEFORE your DSP trigger
            if (pendingTuningSource != nullptr)
                currentFrequency = Ouaricon::NoteExpression::applyPendingTuning (
                                       *pendingTuningSource, midi, currentFrequency);

            // 3) trigger your DSP model with the final tuned frequency
            stringModel.trigger(currentFrequency, vel);
        }

    private:
        Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    };
    ```
    State that composition order matters: the helper must run AFTER base-frequency computation (so tunings stack multiplicatively) and BEFORE the DSP trigger (so the first output sample sizes to the tuned frequency — no zipper).

    **## Dorico End-User Setup**

    Explain the UX trap: Dorico's default/"Auto" microtonality uses pitch-bend or VST2 detune for non-Steinberg VST3 plugins; neither reaches JUCE-based VST3 plugins. The user MUST create a custom expression map. Four-step procedure:

    1. `Dorico -> Library -> Expression Maps...`
    2. Duplicate an existing compatible map (or create a new one).
    3. Set **Microtonality** to **"VST3 Note Expression"**.
    4. In `Play -> Endpoint Setup`, assign the new expression map to your plugin's channel.

    Verification: play a quarter-sharp note; the plugin should pitch it +50¢ above the neighbor semitone. If it plays 12-TET, the expression map is not assigned or Microtonality is still set to Auto.

    **## JUCE Patch Management**

    - Marker string (do not rename): `JUCE-NE-PATCH (Ouaricon local fork, 2026-04-22)`. Both `grep -rn "JUCE-NE-PATCH" /Users/taylorbrook/JUCE/modules/` and the CMake-time check rely on this verbatim string.
    - Re-apply after JUCE upgrade: re-generate the patch by diffing pristine-new vs forked-new (procedure documented in the patch file's header). Rename the file to `note-expression-juce-<NEW-VERSION>.patch`, update `module.yaml`'s `requirements.juce_patch.file` and `.juce_version`, commit, and re-run `scripts/apply-juce-patches.sh`.
    - The patch surfaces `kNoteExpressionValueEvent` + noteId-tagged NoteOn/NoteOff events to `VST3ClientExtensions::onVst3RawEvent` BEFORE `MidiEventList::toMidiBuffer` drops them. Approach 2 (side-channel queue) was chosen over Approach 1 (MidiBuffer mutation) to avoid round-tripping through JUCE's MIDI layer.

    **## Integration Approach**

    Contrast with `scala-tuning-engine`: this module is header-only C++ with no UI (no JS, no CSS, no native functions). Consumers need three lines: the `ouaricon_add_module` CMake call, a VST3Extensions member, and a voice-side `applyPendingTuning` call. Phase 24's 7 pitched plugins follow this same pattern.

    **## Dependencies**

    - JUCE modules: `juce_audio_processors`, `juce_core`.
    - C++20.
    - Local JUCE patch (see §JUCE Patch Management).
    - No dependency on `scala-tuning-engine` — the module composes multiplicatively with any base-frequency source.

    Aim for ~150 lines total. Keep code blocks tight. Do not repeat the full spike findings; reference `.claude/skills/spike-findings-VST-development/` for deep-dive.
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/README.md && [ "$(wc -l < modules/tuning/note-expression/README.md)" -ge "80" ] && grep -q "^## Quick Start" modules/tuning/note-expression/README.md && grep -q "^## Dorico End-User Setup" modules/tuning/note-expression/README.md && grep -q "^## JUCE Patch Management" modules/tuning/note-expression/README.md && grep -q "apply-juce-patches.sh" modules/tuning/note-expression/README.md && grep -q "VST3 Note Expression" modules/tuning/note-expression/README.md</automated>
  </verify>
  <acceptance_criteria>
    - `test -f modules/tuning/note-expression/README.md` exits 0
    - README line count is between 80 and 250 (`wc -l < modules/tuning/note-expression/README.md` within that range)
    - Required H2 sections present (all three grep counts return at least `1`):
      - `grep -c "^## Quick Start$" modules/tuning/note-expression/README.md`
      - `grep -c "^## Features$" modules/tuning/note-expression/README.md`
      - `grep -c "^## Installation$" modules/tuning/note-expression/README.md`
      - `grep -c "^## Dorico End-User Setup$" modules/tuning/note-expression/README.md`
      - `grep -c "^## JUCE Patch Management$" modules/tuning/note-expression/README.md`
      - `grep -c "^## Integration Approach$" modules/tuning/note-expression/README.md`
    - JUCE patch script referenced: `grep -c "apply-juce-patches.sh" modules/tuning/note-expression/README.md` returns at least `2`
    - Dorico setup instructions include the four-step sequence: `grep -c "Expression Maps\|Microtonality\|Endpoint Setup" modules/tuning/note-expression/README.md` returns at least `3`
    - "VST3 Note Expression" string appears at least 3 times: `grep -c "VST3 Note Expression" modules/tuning/note-expression/README.md`
    - Consumer-integration code blocks present: `grep -c "ouaricon_add_module" modules/tuning/note-expression/README.md` returns at least `2`, `grep -c "applyPendingTuning" modules/tuning/note-expression/README.md` returns at least `2`, `grep -c "drainAndUpdate" modules/tuning/note-expression/README.md` returns at least `1`
    - "JUCE-NE-PATCH" marker documented verbatim: `grep -c "JUCE-NE-PATCH" modules/tuning/note-expression/README.md` returns at least `1`
  </acceptance_criteria>
  <done>README covers all MOD-05 required topics (consumer integration, JUCE patch, Dorico setup) with working code blocks and references to actual script paths.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: Rebuild + clean install per CLAUDE.md protocol</name>
  <files>
    ~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3,
    ~/Library/Audio/Plug-Ins/Components/OLyrica.component
  </files>
  <read_first>
    - CLAUDE.md "Plugin Cache Clearing" section — full sequence is MANDATORY
    - .planning/phases/23-extract/23-03-SUMMARY.md (confirms Plan 03 left artifacts in build/ uninstalled for this plan)
  </read_first>
  <action>
    Execute the CLAUDE.md Plugin Cache Clearing protocol in exact sequence. This precedes the Task 4 Dorico smoke test — Dorico will see stale cache otherwise and LYR-03 verification will be invalid.

    **Step 1 — Rebuild with version bump** (VERSION changed in Task 1; CMake needs a reconfigure or full rebuild to pick it up):
    ```bash
    cmake --build build --target OLyrica_VST3 OLyrica_AU --parallel
    ```
    If the VERSION bump doesn't propagate (version strings baked at configure time in some JUCE versions), force a reconfigure:
    ```bash
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target OLyrica_VST3 OLyrica_AU --parallel
    ```

    **Step 2 — Clear macOS AU cache:**
    ```bash
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/
    rm -rf ~/Library/Caches/com.apple.audiounits.cache
    ```

    **Step 3 — Remove old plugin bundles:**
    ```bash
    rm -rf ~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/OLyrica.component
    ```

    **Step 4 — Install fresh bundles:**
    ```bash
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/OLyrica.vst3 \
          ~/Library/Audio/Plug-Ins/VST3/
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/OLyrica.component \
          ~/Library/Audio/Plug-Ins/Components/
    ```

    **Step 5 — Verify installation and AU registration:**
    ```bash
    test -d ~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3
    test -d ~/Library/Audio/Plug-Ins/Components/OLyrica.component
    auval -a 2>/dev/null | grep -i lyrica
    ```
    `auval -a` should list the OLyrica AU. If not, wait a few seconds (AU cache rebuild) and re-run.
  </action>
  <verify>
    <automated>test -d ~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/OLyrica.component && auval -a 2>/dev/null | grep -iq lyrica</automated>
  </verify>
  <acceptance_criteria>
    - Clean build succeeds: `cmake --build build --target OLyrica_VST3 OLyrica_AU` exits 0
    - AU cache cleared: after `killall -9 AudioComponentRegistrar` the process does not appear in `ps aux | grep -i AudioComponentRegistrar | grep -v grep` (Registrar will re-spawn on next AU scan — acceptable)
    - Old bundles removed, fresh bundles installed: both `test -d` checks exit 0
    - AU registration visible: `auval -a 2>/dev/null | grep -ic lyrica` returns at least `1`
    - VST3 bundle Info.plist reflects new version (best-effort — JUCE's version metadata):
      `grep -A 1 CFBundleShortVersionString ~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3/Contents/Info.plist 2>/dev/null | grep -c "2.3.0"` returns `1` (or `0` if JUCE does not populate this key — informational only, not a blocking gate)
  </acceptance_criteria>
  <done>Fresh VST3 + AU installed; AU cache cleared; auval sees the plugin.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 4: Dorico quarter-sharp smoke test (LYR-03 acceptance gate)</name>
  <what-built>
    Full Phase 23 stack: shared `note-expression` module (Plan 01), JUCE patch + CMake marker check (Plan 02), O-Lyrica refactor (Plan 03), O-Lyrica v2.3.0 + comprehensive README + fresh install (Plan 04 Tasks 1-3). The user now verifies the canonical Dorico quarter-sharp smoke test, which is the acceptance gate for LYR-03 and the reference test Phase 24's 7 plugins will each repeat.
  </what-built>
  <how-to-verify>
    Perform these steps in Dorico (version noted in test report). If any step fails, describe the symptom and the plan will revise.

    **Pre-flight (one-time):**
    1. Confirm `auval -a 2>/dev/null | grep -i lyrica` returns a hit.
    2. Open Dorico with a fresh project (or a scratch project).
    3. `Play -> Endpoint Setup`: add an instance of O-Lyrica (VST3 preferred; AU also valid).
    4. `Library -> Expression Maps`: duplicate the default Note Performer / GM expression map (or any existing map). Name the duplicate "Ouaricon VST3 NE" (or similar).
    5. In the expression map editor, set **Microtonality** to **"VST3 Note Expression"**. Save the map.
    6. Back in `Play -> Endpoint Setup`: assign the new expression map to O-Lyrica's channel.

    **Test 1 — Quarter-sharp C4 pitch accuracy:**
    1. Write a middle-C (C4) in Dorico's score.
    2. Apply a quarter-sharp accidental (via Dorico's tonality system or Notations palette).
    3. Play back. Listen and/or use a tuner app. Expected: C4 + 50¢ = 269.29 Hz (vs standard C4 = 261.63 Hz).
    4. PASS criteria: pitch is clearly above standard C4 by approximately a quarter-tone. FAIL criteria: pitch is standard C4 (NE not reaching plugin — check expression map Microtonality setting).

    **Test 2 — No attack zipper:**
    1. Listen to the attack transient of the quarter-sharp C4. The first ~20ms should sound at the tuned pitch, not glide up from 12-TET.
    2. PASS: clean attack at tuned pitch. FAIL: audible pitch glide on the attack (indicates composition order broken — applyPendingTuning running AFTER trigger instead of BEFORE).

    **Test 3 — noteId correlation (multi-note chord):**
    1. Write a chord: C4 (standard), C#4 (standard), D4 with quarter-sharp accidental.
    2. Play back. Expected: C4 and C#4 at standard pitches, D4 at +50¢ above standard.
    3. PASS: only the D4 is detuned. FAIL: all three notes detuned, or wrong note detuned (indicates noteId correlation broken).

    **Test 4 — TuningEngine composition (LYR-02):**
    1. In O-Lyrica's UI, load a non-12-TET tuning (e.g. a Scala preset like "Just Intonation" from the embedded library).
    2. Write a C4 + quarter-sharp in Dorico. Play back.
    3. Expected: pitch = (Just Intonation C4 base freq) * 2^(0.5/12). In Just Intonation, C4 is the tonic at exactly 261.63 Hz (or the master tune reference); quarter-sharp should still produce +50¢ relative.
    4. PASS: NE delta composes with the alternate tuning. FAIL: NE delta is absent (alternate tuning is ignoring NE — indicates voice code bypassed applyPendingTuning) OR NE delta is applied twice (indicates duplicate NE mechanics remain somewhere).

    **Test 5 — Register retrigger test (landmine 4 — retrigger safety):**
    1. Write two consecutive quarter-sharp C4 notes with a short gap.
    2. Play back. Expected: both notes at +50¢.
    3. Then write a quarter-sharp C4 followed by a standard C4 at the same pitch position.
    4. PASS: second C4 plays at STANDARD pitch (not +50¢). This proves `exchange(0.0)` in `applyPendingTuning` consumed the pending slot and didn't leak to the retrigger. FAIL: second C4 also plays at +50¢ (stale offset leaked — the helper's exchange is broken).

    Report the outcome of each of the 5 tests. If any fail, note the symptom and which test number.
  </how-to-verify>
  <resume-signal>
    Type one of:
    - `approved` — all 5 tests passed; LYR-03 is satisfied; phase is complete.
    - `fail: <test-number> <symptom>` — describe which test failed and what was heard / observed. This triggers a revision cycle.
  </resume-signal>
</task>

</tasks>

<verification>
Run this summary audit after Task 4 resumes with "approved":
1. Version bump: `grep -q 'VERSION "2.3.0"' plugins/O-Lyrica/CMakeLists.txt && grep -q "^## \[2.3.0\]" plugins/O-Lyrica/CHANGELOG.md`
2. README comprehensive: all required H2 sections present (see Task 2 acceptance).
3. Fresh install: `test -d ~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3 && test -d ~/Library/Audio/Plug-Ins/Components/OLyrica.component`
4. AU registered: `auval -a 2>/dev/null | grep -iq lyrica`
5. Dorico smoke test: human confirmation of all 5 test outcomes.
6. Full phase-wide strip audit: `grep -rE "neTrace|OLyrica::detail|LyricaVST3Extensions" plugins/O-Lyrica/ modules/tuning/note-expression/ | wc -l` returns `0`.
7. Full phase-wide fstream audit: `grep -rE "^#include <fstream>" plugins/O-Lyrica/Source/ modules/tuning/note-expression/ | wc -l` returns `0`.
</verification>

<success_criteria>
- MOD-05: `modules/tuning/note-expression/README.md` documents consumer integration, JUCE patch management, and Dorico end-user setup — all three MOD-05 subtopics covered with working code blocks and correct script references.
- LYR-03: Dorico quarter-sharp smoke test passes all 5 sub-tests — pitch at +50¢, clean attack (no zipper), noteId correlation works on multi-note chords, TuningEngine composition works (no raw pow bypass), retrigger safety (exchange(0.0) consumption works).
- LYR-04: O-Lyrica version bumped in CMakeLists (`VERSION "2.3.0"`) and CHANGELOG (new [2.3.0] entry with Added/Changed/Removed/Technical notes sections documenting shared-module adoption and microtonal NE support).
- Phase 23 complete: all 12 requirement IDs (MOD-01..08, LYR-01..04) satisfied across Plans 01-04.
</success_criteria>

<output>
After completion, create `.planning/phases/23-extract/23-04-SUMMARY.md` describing:
- Version bump: CMakeLists `VERSION "2.3.0"` location + CHANGELOG [2.3.0] entry content summary.
- README: final line count, section list, code-block count.
- Install log: AU cache clear outcome, `auval -a | grep lyrica` output, VST3/AU bundle paths.
- Dorico smoke-test outcomes (5 tests, pass/fail per test).
- Any revision cycles that occurred (if Task 4 resumed with `fail:` and the plan was revised).
- Phase 23 close-out checklist: all 12 requirement IDs checked off, cross-plan strip audit confirms 0 neTrace / 0 fstream residue.
- Handoff to Phase 24: module is stable; 7 target plugins can now consume via `/improve [PluginName]` per TRACK-01.
</output>
