---
phase: 25-package-docs
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in
  - modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in
  - modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in
  - modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
  - modules/tuning/note-expression/resources/README-microtonal-suite.txt
  - modules/tuning/note-expression/install-microtonal-suite.cmake.in
  - modules/tuning/note-expression/module.cmake
  - modules/tuning/note-expression/module.yaml
  - modules/tuning/note-expression/README.md
  - modules/cmake/OuariconModules.cmake
  - modules/registry.yaml
  - .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md
autonomous: false
requirements: [INST-01, INST-02, INST-04]
must_haves:
  truths:
    - "Wave 0 verification A2 confirms state-less .dorico_pt is accepted by Dorico (D-14)"
    - "Wave 0 verification A4 confirms drag-drop extraction lands files in PlaybackTemplateSpecs/ AND EndpointConfigs/ (D-14)"
    - "Canonical XML body (recovered from cd2c2c6) lives byte-exact at modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib (D-03, D-09)"
    - "playbacktemplatedeps.doricolib.in carries the same recovered XML body (D-01, D-09)"
    - "endpointconfig.xml.in declares 8 slots, one per cohort plugin, each with @<NAME>_PLUGINID@ token and the byte-exact xmap.ouaricon.vst3_note_expression expressionMapID (D-04, D-06, S-4)"
    - "ouaricon_extract_vst3_cids() helper exists in OuariconModules.cmake, honors OUARICON_DEV_SUFFIX, fails fast if VST3 bundle missing (D-06, D-07, S-3)"
    - "module.cmake packs Ouaricon-Microtonal-Suite.dorico_pt at build time via cmake -E tar cf --format=zip and installs both resources via install-microtonal-suite.cmake.in (D-08, S-2)"
    - "Module version bumped 1.0.0 -> 1.1.0 in module.yaml AND modules/registry.yaml (D-08, Pattern J)"
    - "O-Lyrica canary install proves end-to-end pipeline: .dorico_pt at ~/Library/Application Support/Ouaricon/Microtonal Suite/ AND template visible in Dorico Play -> Playback Template picker (D-10, D-11)"
    - "All stale Plan 25-01 v1 install-doricoexpmap-<Plugin>.cmake build outputs vanish on next clean build because the v1 .cmake.in template has been replaced by install-microtonal-suite.cmake.in (D-21)"
  artifacts:
    - path: "modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in"
      provides: "Top-level Playback Template routing rules (configure_file template)"
      contains: "<playbackTemplateSpec>"
    - path: "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in"
      provides: "8 plugin slots with per-plugin pluginID tokens"
      contains: "@OLYRICA_PLUGINID@"
    - path: "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in"
      provides: "Embedded copy of expression-map kScoreLibrary (recovered XML)"
      contains: "xmap.ouaricon.vst3_note_expression"
    - path: "modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib"
      provides: "Standalone expression-map library bundle for Default Library Additions"
      contains: "xmap.ouaricon.vst3_note_expression"
    - path: "modules/tuning/note-expression/resources/README-microtonal-suite.txt"
      provides: "User-facing fallback (INST-04)"
    - path: "modules/tuning/note-expression/install-microtonal-suite.cmake.in"
      provides: "Per-platform dual-write install script template (D-11)"
    - path: "modules/cmake/OuariconModules.cmake"
      provides: "ouaricon_extract_vst3_cids helper appended"
      contains: "function(ouaricon_extract_vst3_cids"
  key_links:
    - from: "module.cmake (note-expression)"
      to: "ouaricon_extract_vst3_cids in OuariconModules.cmake"
      via: "Per-consumer hook fire (S-2) calls helper to populate @<NAME>_PLUGINID@ vars"
      pattern: "ouaricon_extract_vst3_cids"
    - from: "endpointconfig.xml.in <expressionMapID>"
      to: "playbacktemplatedeps.doricolib.in + library/Ouaricon-VST3-NoteExpression.doricolib <entityID>"
      via: "Byte-exact string xmap.ouaricon.vst3_note_expression (S-4)"
      pattern: "xmap.ouaricon.vst3_note_expression"
    - from: "module.cmake add_custom_command (zip pack)"
      to: "Built artifact build/Ouaricon-Microtonal-Suite.dorico_pt"
      via: "cmake -E tar cf --format=zip with WORKING_DIRECTORY = stage dir (Pitfall 5: never include parent dir)"
      pattern: "cmake -E tar cf"
    - from: "install-microtonal-suite.cmake.in"
      to: "~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml AND .../Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib"
      via: "Probe Dorico 6 -> 5 -> 4; cmake -E tar xf for .dorico_pt; file(COPY) for .doricolib (D-11, D-12)"
      pattern: "Default Library Additions"
---

<objective>
Author the canonical Dorico Microtonal Suite asset pair (.dorico_pt + .doricolib), wire CMake plumbing into the note-expression module so any consumer of `ouaricon_add_module(<Plugin> note-expression)` automatically inherits dual-resource installation, and prove the pipeline end-to-end by installing O-Lyrica and verifying the template appears in Dorico's picker.

Purpose: Lay the single source of truth for Phase 25's distribution assets. Plan 25-02 (atomic 8-plugin installer sweep) and Plan 25-03 (internal docs) both depend on this plan being closed cleanly.

Output: 6 NEW files (4 XML resources + 1 README + 1 cmake template) + 5 MODIFIED files (module.cmake / module.yaml / module README / OuariconModules.cmake / registry.yaml) + 1 verification log file.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/REQUIREMENTS.md

@.planning/phases/25-package-docs/25-CONTEXT.md
@.planning/phases/25-package-docs/25-RESEARCH.md
@.planning/phases/25-package-docs/25-PATTERNS.md
@.planning/phases/25-package-docs/25-FINDING-playback-template-pivot.md

@modules/tuning/note-expression/module.cmake
@modules/tuning/note-expression/module.yaml
@modules/tuning/note-expression/README.md
@modules/cmake/OuariconModules.cmake
@modules/registry.yaml
@CLAUDE.md
@.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md

<interfaces>
<!-- Cohort name → CMake variable suffix mapping (load-bearing for @TOKEN@ substitution) -->
<!-- Source: PATTERNS.md Pattern C, lines 268-279 -->

| CMake target name | Suffix produced by string(TOUPPER) + REPLACE("-" "") | XML token |
|-------------------|------------------------------------------------------|-----------|
| OLyrica           | OLYRICA          | @OLYRICA_PLUGINID@ |
| O-Bells           | OBELLS           | @OBELLS_PLUGINID@ |
| O-IntonationPad   | OINTONATIONPAD   | @OINTONATIONPAD_PLUGINID@ |
| O-Prism           | OPRISM           | @OPRISM_PLUGINID@ |
| O-Wind            | OWIND            | @OWIND_PLUGINID@ |
| O-Reed            | OREED            | @OREED_PLUGINID@ |
| O-Bowed           | OBOWED           | @OBOWED_PLUGINID@ |
| O-Formant         | OFORMANT         | @OFORMANT_PLUGINID@ |

<!-- Recovered XML body (PATTERNS.md Pattern D, lines 291-326): -->
<!-- Use byte-exact via: git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap -->
<!-- Root: <kScoreLibrary> -->
<!-- Load-bearing entityID: xmap.ouaricon.vst3_note_expression -->
<!-- Load-bearing microtonalPlaybackMethod: kVST3NoteExpression -->

<!-- RESEARCH.md "Code Examples" lines 374-530 contain ready-to-use CMake — copy verbatim, do not reinvent. -->
</interfaces>
</context>

<tasks>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 0a: Wave 0 — A2 verification (state-less .dorico_pt accepted by Dorico)</name>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-14, D-05, D-16)
    - .planning/phases/25-package-docs/25-RESEARCH.md "Pattern 6" (lines 286-292) + Q3 (lines 593-611)
    - /tmp/ample_china_extracted/EndpointConfigs/Ample China/endpointconfig.xml (slot schema reference)
  </read_first>
  <what-built>
    A stripped `.dorico_pt` archive containing ONLY:
    - `PlaybackTemplateSpecs/Test State-less/playbacktemplatespec.xml` (catch-all entry pointing to one endpoint config)
    - `EndpointConfigs/Test State-less/endpointconfig.xml` with 1-2 slots referencing real installed plugin pluginIDs (e.g. O-Lyrica's dev CID `ABCDEF019182FAEB4F7544764F4C7972`) but NO accompanying `slot1.pluginstate` files
    - `EndpointConfigs/Test State-less/playbacktemplatedeps.doricolib` (minimal kScoreLibrary)
    Use this for the A2 verification ONLY. Do NOT commit; this is a throwaway probe.
  </what-built>
  <how-to-verify>
    1. From the dev machine, build the stripped `.dorico_pt` by hand:
       ```bash
       mkdir -p /tmp/a2-test/PlaybackTemplateSpecs/Test\ State-less
       mkdir -p /tmp/a2-test/EndpointConfigs/Test\ State-less
       # Author the 3 minimal XML files (see Pattern 3, Pattern 4 in RESEARCH.md for skeletons)
       # ... pluginID = O-Lyrica dev CID from ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3/Contents/Resources/moduleinfo.json
       cd /tmp/a2-test && zip -r /tmp/a2-test.dorico_pt PlaybackTemplateSpecs EndpointConfigs
       ```
    2. Drag `/tmp/a2-test.dorico_pt` onto Dorico 6 (or use Play -> Playback Template -> Import).
    3. Confirm "Test State-less" appears in `Play -> Playback Template`.
    4. Apply the template to a new project. Observe whether O-Lyrica loads on the assigned channel even though no `slot1.pluginstate` was shipped.
    5. Quarter-sharp C4 smoke test: write a quarter-sharp accidental on C4. Confirm playback at ~269.29 Hz (not 261.63 Hz).

    **Stop-on-first-failure (D-16):** If A2 fails (template appears but slots refuse to load without state), HALT this plan, escalate to D-05 reconsideration. Curated state authoring becomes mandatory; promote to `25-01-A2-FAIL-fix-PLAN.md`.

    Record observed Dorico behavior into `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` as a markdown section "## A2 Result" with timestamp, version, observed behavior, and pass/fail.
  </how-to-verify>
  <acceptance_criteria>
    - File exists: `test -f .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md`
    - File contains "## A2 Result" header
    - File contains "PASS" OR "FAIL" decision; if FAIL, contains escalation note
    - If PASS: file documents that template appeared, slot loaded with factory defaults, and quarter-sharp playback was observed
  </acceptance_criteria>
  <resume-signal>Type "A2 PASS — proceed" or "A2 FAIL — escalate to fix-plan"</resume-signal>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
  <action>
    1. From the dev machine, build the stripped `.dorico_pt` by hand:
           ```bash
           mkdir -p /tmp/a2-test/PlaybackTemplateSpecs/Test\ State-less
           mkdir -p /tmp/a2-test/EndpointConfigs/Test\ State-less
           # Author the 3 minimal XML files (see Pattern 3, Pattern 4 in RESEARCH.md for skeletons)
           # ... pluginID = O-Lyrica dev CID from ~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3/Contents/Resources/moduleinfo.json
           cd /tmp/a2-test && zip -r /tmp/a2-test.dorico_pt PlaybackTemplateSpecs EndpointConfigs
           ```
        2. Drag `/tmp/a2-test.dorico_pt` onto Dorico 6 (or use Play -> Playback Template -> Import).
        3. Confirm "Test State-less" appears in `Play -> Playback Template`.
        4. Apply the template to a new project. Observe whether O-Lyrica loads on the assigned channel even though no `slot1.pluginstate` was shipped.
        5. Quarter-sharp C4 smoke test: write a quarter-sharp accidental on C4. Confirm playback at ~269.29 Hz (not 261.63 Hz).
    
        **Stop-on-first-failure (D-16):** If A2 fails (template appears but slots refuse to load without state), HALT this plan, escalate to D-05 reconsideration. Curated state authoring becomes mandatory; promote to `25-01-A2-FAIL-fix-PLAN.md`.
    
        Record observed Dorico behavior into `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` as a markdown section "## A2 Result" with timestamp, version, observed behavior, and pass/fail.
  </action>
  <verify><automated>see acceptance_criteria above</automated></verify>
  <done>All <acceptance_criteria> conditions above are satisfied.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 0b: Wave 0 — A4 verification (drag-drop extraction is faithful)</name>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-14, D-11, D-16)
    - .planning/phases/25-package-docs/25-RESEARCH.md "A4" (lines 550) + Q6 (lines 656-674)
    - /tmp/ample_china/Ample China.dorico_pt (already on disk per RESEARCH.md)
    - /tmp/ample_china_extracted/ (already extracted)
  </read_first>
  <what-built>
    A drag-drop install of an already-known-good third-party `.dorico_pt` (Ample China) onto Dorico 6, to confirm the extracted destinations match the zip's internal layout.
  </what-built>
  <how-to-verify>
    1. Pre-state snapshot:
       ```bash
       ls "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/" 2>/dev/null
       ls "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/" 2>/dev/null
       ```
       Record whether `Ample China/` already exists in either location.
    2. Drag `/tmp/ample_china/Ample China.dorico_pt` onto a Dorico 6 project window (or the Hub).
    3. Post-state check:
       ```bash
       test -d "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ample China" && echo "PT-SPEC OK"
       test -d "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ample China" && echo "ENDPOINT OK"
       test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ample China/playbacktemplatespec.xml" && echo "SPEC FILE OK"
       test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ample China/endpointconfig.xml" && echo "ENDPOINT FILE OK"
       ```

    **Stop-on-first-failure (D-16):** If A4 fails (drag-drop extraction is partial — e.g., only `PlaybackTemplateSpecs/` extracts, not `EndpointConfigs/`), HALT this plan, escalate to D-11 reconsideration: drag-drop install is rejected; explicit `Play -> Playback Template -> Import` becomes the user-facing flow and the installer simply lands the file in `~/Library/Application Support/Ouaricon/Microtonal Suite/` for manual import. Promote to `25-01-A4-FAIL-fix-PLAN.md`.

    Append "## A4 Result" section to `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` with the bash output, confirmed destinations, and PASS/FAIL.
  </how-to-verify>
  <acceptance_criteria>
    - File contains "## A4 Result" header
    - File contains "PASS" OR "FAIL" decision; if FAIL, contains escalation note
    - If PASS: contains exact bash output showing both `PT-SPEC OK` and `ENDPOINT OK` (and both file existence checks)
  </acceptance_criteria>
  <resume-signal>Type "A4 PASS — proceed" or "A4 FAIL — escalate to fix-plan"</resume-signal>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
  <action>
    (checkpoint task — see <what-built> and <how-to-verify> below for the verification protocol)
    
    WHAT BUILT:
    A drag-drop install of an already-known-good third-party `.dorico_pt` (Ample China) onto Dorico 6, to confirm the extracted destinations match the zip's internal layout.
    
    HOW TO VERIFY:
    1. Pre-state snapshot:
           ```bash
           ls "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/" 2>/dev/null
           ls "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/" 2>/dev/null
           ```
           Record whether `Ample China/` already exists in either location.
        2. Drag `/tmp/ample_china/Ample China.dorico_pt` onto a Dorico 6 project window (or the Hub).
        3. Post-state check:
           ```bash
           test -d "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ample China" && echo "PT-SPEC OK"
           test -d "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ample China" && echo "ENDPOINT OK"
           test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ample China/playbacktemplatespec.xml" && echo "SPEC FILE OK"
           test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ample China/endpointconfig.xml" && echo "ENDPOINT FILE OK"
           ```
    
        **Stop-on-first-failure (D-16):** If A4 fails (drag-drop extraction is partial — e.g., only `PlaybackTemplateSpecs/` extracts, not `EndpointConfigs/`), HALT this plan, escalate to D-11 reconsideration: drag-drop install is rejected; explicit `Play -> Playback Template -> Import` becomes the user-facing flow and the installer simply lands the file in `~/Library/Application Support/Ouaricon/Microtonal Suite/` for manual import. Promote to `25-01-A4-FAIL-fix-PLAN.md`.
    
        Append "## A4 Result" section to `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` with the bash output, confirmed destinations, and PASS/FAIL.
  </action>
  <verify><automated>see acceptance_criteria above (human-verified checkpoint; automated gate is on the recorded VERIFICATION.md file)</automated></verify>
  <done>All <acceptance_criteria> conditions above are satisfied.</done>
</task>

<task type="auto">
  <name>Task 1: Recover canonical XML body and author standalone .doricolib + embedded deps copy</name>
  <read_first>
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern D (lines 285-339) and S-4 (lines 759-768)
    - .planning/phases/25-package-docs/25-RESEARCH.md "Pattern 5" (lines 278-284) and Q7 (lines 677-714)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-01, D-03, D-09)
    - The recovered file via: `git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap`
  </read_first>
  <action>
    Per D-03 + Pattern D — recover the XML body verbatim (do NOT re-author):

    ```bash
    # Recovery — use as the byte-exact source for both target files.
    mkdir -p modules/tuning/note-expression/resources/library
    git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap \
      > modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib

    mkdir -p "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite"
    git show cd2c2c6:modules/tuning/note-expression/resources/Ouaricon-VST3-NoteExpression.doricoexpmap \
      > "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in"
    ```

    Both files share the same byte-exact `<kScoreLibrary>` body, which carries:
    - `<entityID>xmap.ouaricon.vst3_note_expression</entityID>` (S-4 invariant — referenced from endpointconfig.xml.in)
    - `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` (D-02 / Landmine 3 invariant — never regress to kAuto / kPitchBend)

    The `.doricolib.in` extension is intentional even though no `@TOKEN@` substitution is required — the symmetry with `endpointconfig.xml.in` and `playbacktemplatespec.xml.in` keeps the `module.cmake` `configure_file` invocations uniform. Pattern A's CMake (RESEARCH.md lines 374-426) calls `configure_file(... @ONLY)` on this file too; with no `@TOKEN@`s present, `@ONLY` is a no-op (safe).

    NOTE: the git-show recovery includes the entire reverted file — confirm it has the kScoreLibrary root (not a Dorico-host-only stub). If the recovered file looks truncated or corrupted (size < 1KB), abort and escalate.
  </action>
  <verify>
    <automated>xmllint --noout modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib && xmllint --noout "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in" && grep -c "xmap.ouaricon.vst3_note_expression" modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in" && grep -c "kVST3NoteExpression" modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in"</automated>
  </verify>
  <acceptance_criteria>
    - `xmllint --noout` succeeds for both files
    - Each file contains exactly 1 occurrence of `xmap.ouaricon.vst3_note_expression`
    - Each file contains exactly 1 occurrence of `kVST3NoteExpression`
    - `wc -c modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` returns > 1000 bytes (sanity: full XML body, not stub)
    - Diff between the two files is zero (`diff -q library/Ouaricon-VST3-NoteExpression.doricolib playback-template/.../playbacktemplatedeps.doricolib.in` returns no output)
  </acceptance_criteria>
  <done>Both XML files exist, are well-formed, share byte-exact body, and carry the load-bearing invariants.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 2: Author playbacktemplatespec.xml.in and endpointconfig.xml.in (8 slots)</name>
  <read_first>
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern C (lines 222-281) and S-4
    - .planning/phases/25-package-docs/25-RESEARCH.md "Pattern 3" (lines 198-227) and "Pattern 4" (lines 236-273)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-04, D-06, D-09)
    - /tmp/ample_china_extracted/PlaybackTemplateSpecs/Ample China/playbacktemplatespec.xml (structural reference)
    - /tmp/ample_china_extracted/EndpointConfigs/Ample China/endpointconfig.xml (structural reference, 11-slot example)
  </read_first>
  <action>
    Author two NEW configure_file templates per RESEARCH.md verbatim XML.

    **File 1: `modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in`**

    Use this exact content (RESEARCH.md Pattern 3, lines 198-227):
    ```xml
    <?xml version="1.0" encoding="utf-8"?>
    <playbackTemplateSpec>
        <fileVersion>1.1416</fileVersion>
        <playbackTemplateSpecID>playbacktemplate.user.ouaricon_microtonal_suite</playbackTemplateSpecID>
        <name>Ouaricon Microtonal Suite</name>
        <creator>Ouaricon Audio</creator>
        <description>VST3 Note Expression routing for the Ouaricon v1.5 microtonal cohort (O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant).</description>
        <version>1</version>
        <associatedSpaceTemplateID/>
        <entries array="true">
            <entry>
                <instrumentFamilies/>
                <instruments/>
                <endpointConfig>
                    <configID>endpointconfig.user.ouaricon_microtonal_suite</configID>
                </endpointConfig>
            </entry>
        </entries>
    </playbackTemplateSpec>
    ```

    No `@TOKEN@`s required — `configure_file @ONLY` is a no-op for this file (kept symmetric with siblings).

    **File 2: `modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in`**

    Use the slot template from PATTERNS.md Pattern C (lines 234-263) repeated 8 times. Slot table — replicate this block 8 times, varying `<slotID>`, `<pluginID>`, `<pluginName>`, `<endpointConfigSlotIndex>` per row:

    | slotID | pluginID token              | pluginName        | endpointConfigSlotIndex |
    |--------|------------------------------|-------------------|--------------------------|
    | 1      | `@OLYRICA_PLUGINID@`         | O-Lyrica          | 0                        |
    | 2      | `@OBELLS_PLUGINID@`          | O-Bells           | 1                        |
    | 3      | `@OINTONATIONPAD_PLUGINID@`  | O-IntonationPad   | 2                        |
    | 4      | `@OPRISM_PLUGINID@`          | O-Prism           | 3                        |
    | 5      | `@OWIND_PLUGINID@`           | O-Wind            | 4                        |
    | 6      | `@OREED_PLUGINID@`           | O-Reed            | 5                        |
    | 7      | `@OBOWED_PLUGINID@`          | O-Bowed           | 6                        |
    | 8      | `@OFORMANT_PLUGINID@`        | O-Formant         | 7                        |

    Each slot's `<expressionMapID>` MUST be the byte-exact string `xmap.ouaricon.vst3_note_expression` (S-4 cross-file invariant).

    File header (preserved across all 8 slots):
    ```xml
    <?xml version="1.0" encoding="utf-8"?>
    <endpointConfig>
        <fileVersion>1.1416</fileVersion>
        <version>1</version>
        <name>Ouaricon Microtonal Suite</name>
        <configID>endpointconfig.user.ouaricon_microtonal_suite</configID>
        <slots array="true">
            <!-- 8 <slotData> blocks here, one per cohort plugin -->
        </slots>
    </endpointConfig>
    ```

    Use the per-slot block from PATTERNS.md Pattern C lines 236-263 verbatim, with the substitutions per the table above. Do NOT include `<pluginStateFile>` references — D-05 ships state-less.
  </action>
  <verify>
    <automated>xmllint --noout "modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in" && xmllint --noout "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in" && grep -v '^[[:space:]]*<!--' "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in" | grep -c '<slotData>' && grep -v '^[[:space:]]*<!--' "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in" | grep -c 'xmap.ouaricon.vst3_note_expression' && for tok in OLYRICA OBELLS OINTONATIONPAD OPRISM OWIND OREED OBOWED OFORMANT; do grep -q "@${tok}_PLUGINID@" "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in" || { echo "MISSING TOKEN: @${tok}_PLUGINID@"; exit 1; }; done && echo "ALL 8 TOKENS PRESENT"</automated>
  </verify>
  <acceptance_criteria>
    - `xmllint --noout` succeeds for both files
    - `endpointconfig.xml.in` contains exactly 8 `<slotData>` blocks (grep count after stripping comment lines)
    - `endpointconfig.xml.in` contains exactly 8 occurrences of `xmap.ouaricon.vst3_note_expression` (one per slot, S-4 invariant)
    - All 8 plugin tokens (`@OLYRICA_PLUGINID@`, `@OBELLS_PLUGINID@`, `@OINTONATIONPAD_PLUGINID@`, `@OPRISM_PLUGINID@`, `@OWIND_PLUGINID@`, `@OREED_PLUGINID@`, `@OBOWED_PLUGINID@`, `@OFORMANT_PLUGINID@`) are present (each at least once)
    - `playbacktemplatespec.xml.in` contains the literal string `endpointconfig.user.ouaricon_microtonal_suite` (cross-file linkage)
    - Final command echoes "ALL 8 TOKENS PRESENT"
  </acceptance_criteria>
  <done>Both templated XML files exist, are well-formed, contain 8 slots correctly tokenized, and link to the standalone .doricolib via byte-exact entityID.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 3: Author README-microtonal-suite.txt fallback (INST-04)</name>
  <read_first>
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern E (lines 342-355)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-13, D-19)
    - The recovered analog: `git show cd2c2c6:modules/tuning/note-expression/resources/README-doricoexpmap.txt`
  </read_first>
  <action>
    Author `modules/tuning/note-expression/resources/README-microtonal-suite.txt` (plain text). Preserve the 6-section structure from the recovered v1 README; revise content for the new dual-asset Playback Template flow.

    Required sections (per D-13 / Pattern E):

    1. **Title** with `===` underline: `Ouaricon Microtonal Suite — Dorico Integration Resources`
    2. **PURPOSE** (H1): describe the two files and what they enable (one-line: "Adds Dorico-aware microtonal playback for the Ouaricon v1.5 cohort via VST3 Note Expression").
    3. **INSTALL LOCATIONS** (H1): list canonical paths per platform. Concretely (D-11):
       - macOS shared (canonical): `~/Library/Application Support/Ouaricon/Microtonal Suite/`
       - macOS Dorico template extracted into: `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/`
       - macOS Dorico library: `~/Library/Application Support/Steinberg/Dorico [N]/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib`
       - Windows shared: `%APPDATA%\Ouaricon\Microtonal Suite\`
       - Windows Dorico template: `%APPDATA%\Steinberg\Dorico [N]\PlaybackTemplateSpecs\Ouaricon Microtonal Suite\`
       - Windows Dorico library: `%APPDATA%\Steinberg\Dorico [N]\DefaultLibraryAdditions\Ouaricon-VST3-NoteExpression.doricolib` (NOTE: NO spaces on Windows; spaces on macOS — Pitfall 3)
    4. **MANUAL IMPORT FALLBACK** (H1): if Dorico did not auto-discover after install:
       - Playback Template: `Play -> Playback Template -> Import...` -> select `Ouaricon-Microtonal-Suite.dorico_pt` from the shared dir
       - Expression Map library: `Library -> Import Library...` -> select `Ouaricon-VST3-NoteExpression.doricolib` from the shared dir
    5. **SOURCE OF TRUTH** (H1): name the canonical module-side source path: `modules/tuning/note-expression/resources/{playback-template,library}/` in the VST-development repo. Bug reports and revisions go upstream there, not against the installed copies.
    6. **SUPPORTED PLUGINS (v1.5 cohort)** (H1): bullet list of all 8: O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant. State that Dorico will warn for any missing plugin slot but the template still applies for installed plugins (D-04 — graceful missing-plugin behavior).

    Tone: technical, plain, developer-facing — honor DOCS-05 (no marketing copy).
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/resources/README-microtonal-suite.txt && grep -q "PURPOSE" modules/tuning/note-expression/resources/README-microtonal-suite.txt && grep -q "INSTALL LOCATIONS" modules/tuning/note-expression/resources/README-microtonal-suite.txt && grep -q "MANUAL IMPORT FALLBACK" modules/tuning/note-expression/resources/README-microtonal-suite.txt && grep -q "SOURCE OF TRUTH" modules/tuning/note-expression/resources/README-microtonal-suite.txt && grep -q "SUPPORTED PLUGINS" modules/tuning/note-expression/resources/README-microtonal-suite.txt && grep -q "Default Library Additions" modules/tuning/note-expression/resources/README-microtonal-suite.txt && grep -q "DefaultLibraryAdditions" modules/tuning/note-expression/resources/README-microtonal-suite.txt && for plug in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do grep -q "$plug" modules/tuning/note-expression/resources/README-microtonal-suite.txt || { echo "MISSING PLUGIN: $plug"; exit 1; }; done && echo "ALL 6 SECTIONS + ALL 8 PLUGINS + BOTH DIR-NAMES PRESENT"</automated>
  </verify>
  <acceptance_criteria>
    - File exists
    - All 6 named H1 sections present
    - Both `Default Library Additions` (macOS, with spaces) AND `DefaultLibraryAdditions` (Windows, NO spaces) appear in the file (Pitfall 3 documented for the user)
    - All 8 cohort plugin names present
    - Final command echoes "ALL 6 SECTIONS + ALL 8 PLUGINS + BOTH DIR-NAMES PRESENT"
  </acceptance_criteria>
  <done>User-facing fallback README authored, satisfies INST-04, honors directory-name asymmetry.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 4: Append ouaricon_extract_vst3_cids helper to OuariconModules.cmake</name>
  <read_first>
    - modules/cmake/OuariconModules.cmake (full file — append target is after line 188 `endfunction()` of `ouaricon_check_module_updates`)
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern B (lines 151-219) and S-3 (lines 748-757)
    - .planning/phases/25-package-docs/25-RESEARCH.md "CID extraction helper" (lines 432-474)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-06, D-07)
    - .planning/phases/25-package-docs/25-RESEARCH.md "Pattern 2" (lines 176-191) — table of all 8 dev CIDs for sanity-cross-check
  </read_first>
  <action>
    Append the `ouaricon_extract_vst3_cids` helper function to the END of `modules/cmake/OuariconModules.cmake`, AFTER line 188 (`endfunction()` of `ouaricon_check_module_updates`). Do NOT modify lines 1-188.

    Use the implementation from RESEARCH.md lines 432-474 verbatim, with these load-bearing details:

    1. The bundle path MUST honor `${OUARICON_DEV_SUFFIX}` (S-3 — dev/prod CID asymmetry mitigation, Pitfall 2):
       ```cmake
       set(moduleinfo "${CMAKE_BINARY_DIR}/plugins/${plugin_target}/${plugin_target}_artefacts/Release/VST3/${plugin_target}${OUARICON_DEV_SUFFIX}.vst3/Contents/Resources/moduleinfo.json")
       ```

    2. FATAL_ERROR if the moduleinfo.json is missing — this prevents silent CID drift if the helper is invoked before all 8 `_VST3` targets have built (mirror Pattern B's existing `ouaricon_add_module` FATAL_ERROR style).

    3. Python-based JSON parser (handles JUCE 8's trailing-comma quirk) per RESEARCH.md lines 449-466. Use `python3 -c "..."` via `execute_process` with `COMMAND_ERROR_IS_FATAL ANY`.

    4. The variable suffix derivation (must match the cohort name → suffix table in PATTERNS.md Pattern C lines 268-279):
       ```cmake
       string(TOUPPER "${plugin_target}" var_name)
       string(REPLACE "-" "" var_name "${var_name}")
       set("${var_name}_PLUGINID" "${cid}" PARENT_SCOPE)
       ```

    5. Status log line per existing OuariconModules.cmake convention (mirror line 50 `[Ouaricon] Adding module ...` shape):
       ```cmake
       message(STATUS "[Ouaricon] ${plugin_target} pluginID = ${cid}")
       ```

    6. Add a function header comment block matching the existing function-header convention (`# ====== / # function_name / # purpose / # ======`):
       ```cmake
       # ==============================================================================
       # ouaricon_extract_vst3_cids(OUTPUT_VAR <var> PLUGINS <list>)
       #
       # Reads each built VST3 bundle's Contents/Resources/moduleinfo.json (emitted by
       # JUCE 8) and extracts the canonical 32-hex Audio Module Class CID. Sets
       # per-plugin <NAME>_PLUGINID variables in PARENT_SCOPE for use with
       # configure_file @ONLY substitution in module.cmake.
       #
       # Honors ${OUARICON_DEV_SUFFIX} so dev installers ship dev CIDs and prod
       # installers ship prod CIDs (mitigates Pitfall 2 — see Phase 25 RESEARCH.md).
       #
       # Fails loud + fails fast at configure time if any plugin's VST3 bundle is
       # not yet built (mirrors ouaricon_add_module's FATAL_ERROR pattern).
       # ==============================================================================
       ```
  </action>
  <verify>
    <automated>grep -c "function(ouaricon_extract_vst3_cids" modules/cmake/OuariconModules.cmake | grep -q "^1$" && grep -c "OUARICON_DEV_SUFFIX" modules/cmake/OuariconModules.cmake | grep -qv "^0$" && grep -q "moduleinfo.json" modules/cmake/OuariconModules.cmake && grep -q "Audio Module Class" modules/cmake/OuariconModules.cmake && grep -q "PARENT_SCOPE" modules/cmake/OuariconModules.cmake && grep -q "FATAL_ERROR" modules/cmake/OuariconModules.cmake</automated>
  </verify>
  <acceptance_criteria>
    - File contains exactly 1 occurrence of `function(ouaricon_extract_vst3_cids`
    - File contains the literal string `OUARICON_DEV_SUFFIX` (S-3 honored)
    - File contains the literal string `moduleinfo.json` (correct JSON source)
    - File contains the literal string `Audio Module Class` (correct CID category)
    - File contains both `PARENT_SCOPE` and `FATAL_ERROR` (correct fail-fast semantics)
    - File still contains the original 3 functions (`ouaricon_add_module`, `ouaricon_list_modules`, `ouaricon_check_module_updates`) — sanity: `grep -c "^function(" modules/cmake/OuariconModules.cmake` returns 4
  </acceptance_criteria>
  <done>Helper function appended; existing functions preserved verbatim; honors dev/prod CID asymmetry.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 5: Author install-microtonal-suite.cmake.in (per-platform dual-write)</name>
  <read_first>
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern F (lines 358-418)
    - .planning/phases/25-package-docs/25-RESEARCH.md "install-microtonal-suite.cmake.in template" (lines 478-530)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-10, D-11, D-12, D-21)
  </read_first>
  <action>
    Author NEW file `modules/tuning/note-expression/install-microtonal-suite.cmake.in`. Use the verbatim content from RESEARCH.md lines 478-530 — research has already authored the template fully. Critical invariants the template must encode:

    1. **macOS dir name has spaces**: `Default Library Additions` (Pitfall 3)
    2. **Windows dir name has NO spaces**: `DefaultLibraryAdditions` (Pitfall 3)
    3. **Dorico version probe order**: 6 -> 5 -> 4 (D-12; descending; install to first found, then `break`)
    4. **Dual-write per platform**: shared canonical (Ouaricon dir) AND Dorico auto-scan (D-11)
    5. **`cmake -E tar xf` extraction** with `WORKING_DIRECTORY = ${DORICO_DIR}` so the .dorico_pt's internal `PlaybackTemplateSpecs/` and `EndpointConfigs/` subdirs land in the right places (verified by Wave 0 A4)
    6. **Idempotent overwrite**: `file(COPY)` overwrites in place; safe for all-8-installers writing identical content (D-10)
    7. **`@CMAKE_BINARY_DIR@` and `@CMAKE_CURRENT_LIST_DIR@`** substituted at configure time by `configure_file(... @ONLY)` invoked from module.cmake (Task 6)

    Do NOT add elseif(LINUX) — out of scope (the cohort is macOS+Windows only per phase boundary).

    File must end with informational message lines (per Pattern F):
    ```cmake
    message(STATUS "[Ouaricon] Microtonal Suite installed for Dorico ${_v}: ${DORICO_DIR}")
    ```
    so install-time logs surface the version and destination.
  </action>
  <verify>
    <automated>test -f modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "Default Library Additions" modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "DefaultLibraryAdditions" modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "cmake -E tar xf\|CMAKE_COMMAND.*-E.*tar.*xf" modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "foreach(_v 6 5 4)" modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "if(APPLE)" modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "elseif(WIN32)" modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "@CMAKE_BINARY_DIR@" modules/tuning/note-expression/install-microtonal-suite.cmake.in && grep -q "@CMAKE_CURRENT_LIST_DIR@" modules/tuning/note-expression/install-microtonal-suite.cmake.in</automated>
  </verify>
  <acceptance_criteria>
    - File exists
    - Contains `Default Library Additions` literal (macOS branch)
    - Contains `DefaultLibraryAdditions` literal (Windows branch)
    - Contains tar extraction invocation
    - Contains `foreach(_v 6 5 4)` literal (descending Dorico version probe)
    - Contains both `if(APPLE)` and `elseif(WIN32)` branches
    - Contains both `@CMAKE_BINARY_DIR@` and `@CMAKE_CURRENT_LIST_DIR@` substitution tokens
  </acceptance_criteria>
  <done>Cross-platform install template authored with both directory-name asymmetry AND descending Dorico version probe.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 6: Extend module.cmake with packing custom command + dual install rules</name>
  <read_first>
    - modules/tuning/note-expression/module.cmake (full file — preserve lines 1-42 verbatim, append after line 42)
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern A (lines 55-148) and S-2 (lines 736-746)
    - .planning/phases/25-package-docs/25-RESEARCH.md "Code Examples" §Build the canonical .dorico_pt at CMake install time (lines 374-426)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-08, D-09)
  </read_first>
  <action>
    Append the v2 packing + install logic to `modules/tuning/note-expression/module.cmake`. Lines 1-42 are PRESERVED VERBATIM (the existing JUCE-NE-PATCH marker check). The append target starts at line 43.

    Use the verbatim CMake from RESEARCH.md lines 374-426. Critical structural elements:

    1. **Stage directory**: `${CMAKE_BINARY_DIR}/_microtonal-suite/Ouaricon-Microtonal-Suite/` with subdirs `PlaybackTemplateSpecs/Ouaricon Microtonal Suite/` and `EndpointConfigs/Ouaricon Microtonal Suite/`.

    2. **`ouaricon_extract_vst3_cids` invocation**: pass all 8 cohort target names (matching CMake target names — note `OLyrica` is the target name even though directory is `O-Lyrica`):
       ```cmake
       ouaricon_extract_vst3_cids(
           OUTPUT_VAR PLUGIN_CIDS
           PLUGINS OLyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant
       )
       ```

    3. **3 `configure_file(... @ONLY)` invocations** to render the .in templates (substitute the 8 `@<NAME>_PLUGINID@` tokens into endpointconfig.xml; no-op for the other two but kept symmetric).

    4. **`add_custom_command` + `add_custom_target(ouaricon_microtonal_suite_pt ALL ...)`**: pack the .dorico_pt zip at build time via `cmake -E tar cf ... --format=zip` with `WORKING_DIRECTORY = ${DORICO_PT_STAGE}` so the zip's first entries are `PlaybackTemplateSpecs/...` and `EndpointConfigs/...` (NOT a wrapping parent dir — Pitfall 5).

    5. **`configure_file(install-microtonal-suite.cmake.in -> install-microtonal-suite-${TARGET_NAME}.cmake @ONLY)` + `install(SCRIPT ...)`** so each consumer's install pulls both resources to user systems.

    6. Add a section header comment block to delineate the v2 append from the v1 patch-marker check:
       ```cmake
       # ==============================================================================
       # Microtonal Suite — Dorico Playback Template + .doricolib distribution
       # Phase 25 v2 (Playback Template pivot — supersedes Plan 25-01 v1).
       #
       # Per D-08: this module owns BOTH distributable resources. Any consumer of
       # ouaricon_add_module(<plugin> note-expression) automatically inherits
       # dual-resource installation via install(SCRIPT ...) below.
       #
       # Build flow:
       #   1. ouaricon_extract_vst3_cids → reads each built VST3's moduleinfo.json
       #   2. configure_file @ONLY → substitutes @<NAME>_PLUGINID@ in endpointconfig.xml.in
       #   3. cmake -E tar cf --format=zip → packs Ouaricon-Microtonal-Suite.dorico_pt
       #
       # Install flow (per-consumer, per-platform):
       #   install-microtonal-suite-<TARGET>.cmake → cmake -E tar xf into
       #   ~/Library/Application Support/Steinberg/Dorico [N]/ + Default Library Additions/
       # ==============================================================================
       ```

    Note: the existing `OUARICON_DEV_SUFFIX` mechanism (S-3) flows through `ouaricon_extract_vst3_cids` into the configured `endpointconfig.xml` automatically — no extra wiring needed in this file.

    Note on `${TARGET_NAME}`: this variable is set by the calling `ouaricon_add_module(<TARGET_NAME> ...)` macro (per OuariconModules.cmake convention). Each consumer fires its own per-target configure_file output.
  </action>
  <verify>
    <automated>head -42 modules/tuning/note-expression/module.cmake | grep -q "JUCE-NE-PATCH markers verified" && grep -q "ouaricon_extract_vst3_cids" modules/tuning/note-expression/module.cmake && grep -q "DORICO_PT_STAGE" modules/tuning/note-expression/module.cmake && grep -q "Ouaricon-Microtonal-Suite.dorico_pt" modules/tuning/note-expression/module.cmake && grep -q "format=zip" modules/tuning/note-expression/module.cmake && grep -q "ouaricon_microtonal_suite_pt" modules/tuning/note-expression/module.cmake && grep -q "install-microtonal-suite.cmake.in" modules/tuning/note-expression/module.cmake && grep -q "install(SCRIPT" modules/tuning/note-expression/module.cmake && grep -q "configure_file" modules/tuning/note-expression/module.cmake</automated>
  </verify>
  <acceptance_criteria>
    - First 42 lines unchanged (`head -42` still contains the existing JUCE-NE-PATCH verification block)
    - Contains `ouaricon_extract_vst3_cids` invocation
    - Contains `DORICO_PT_STAGE` variable
    - Contains zip output filename literal
    - Contains `format=zip` flag (Pitfall 5 mitigation when paired with WORKING_DIRECTORY)
    - Contains `add_custom_target(ouaricon_microtonal_suite_pt ...)` literal `ouaricon_microtonal_suite_pt`
    - Contains `install-microtonal-suite.cmake.in` reference
    - Contains `install(SCRIPT` invocation
    - Contains at least 3 `configure_file` invocations (3 templates rendered)
  </acceptance_criteria>
  <done>module.cmake extended with v2 logic; v1 (line 1-42) preserved verbatim; per-consumer firing extends to dual-resource install via install(SCRIPT).</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="auto">
  <name>Task 7: Bump module 1.0.0 → 1.1.0 in module.yaml + registry.yaml + update README.md</name>
  <read_first>
    - modules/tuning/note-expression/module.yaml (full file)
    - modules/registry.yaml (lines 251-275 — note-expression entry)
    - modules/tuning/note-expression/README.md (full file — focus on existing "Dorico End-User Setup" section lines 155-173)
    - .planning/phases/25-package-docs/25-PATTERNS.md Pattern J (lines 611-643)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-08)
  </read_first>
  <action>
    Three coordinated edits:

    **(a) module.yaml**:
    - Line 7: change `version: 1.0.0` to `version: 1.1.0`
    - Append new changelog entry (after line 71) per Pattern J's v2 append target:
      ```yaml
        - version: 1.1.0
          date: 2026-04-26
          changes:
            - "Added Ouaricon Microtonal Suite distributable resources (.dorico_pt + .doricolib)"
            - "Added per-platform install() rules (Ouaricon shared + Dorico auto-scan)"
            - "Added ouaricon_extract_vst3_cids helper to OuariconModules.cmake (additive)"
            - "Module C++ source surface unchanged; additive resource surface only"
      ```

    **(b) modules/registry.yaml**:
    - Bump `version: 1.0.0` → `version: 1.1.0` in the `- name: note-expression` block (around line 253)
    - Do NOT modify the `used_by:` list (it carries 8 entries from Phase 24 — preserve verbatim per PATTERNS.md Pattern J line 641)

    **(c) modules/tuning/note-expression/README.md**:
    - REPLACE the existing "Dorico End-User Setup" section (lines 155-173 — the 4-step manual flow). New content describes the auto-discovery flow + manual-import fallback (D-19, Pattern E):
      ```markdown
      ## Dorico End-User Setup (v1.1.0+ auto-discovery flow)

      Starting in module v1.1.0, every cohort plugin's installer ships the
      Ouaricon Microtonal Suite Playback Template + expression-map library
      directly to Dorico's auto-scan directories. NO manual import is
      required for the typical case.

      **Auto-discovery (default):**
      1. Install any Ouaricon plugin (PKG on macOS or EXE on Windows).
      2. Restart Dorico.
      3. `Play -> Playback Template -> Ouaricon Microtonal Suite -> Apply and Close`.
      4. Quarter-sharp C4 verifies routing: pitch lands at +50¢ (~269.29 Hz).

      The installer writes to:
      - macOS: `~/Library/Application Support/Steinberg/Dorico [N]/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/` and `.../Default Library Additions/`
      - Windows: `%APPDATA%\Steinberg\Dorico [N]\PlaybackTemplateSpecs\Ouaricon Microtonal Suite\` and `...\DefaultLibraryAdditions\` (note: NO spaces on Windows)

      **Manual import fallback** (if Dorico did not pick up the auto-discovered files):
      1. `Play -> Playback Template -> Import...` -> `Ouaricon-Microtonal-Suite.dorico_pt` from `~/Library/Application Support/Ouaricon/Microtonal Suite/`
      2. `Library -> Import Library...` -> `Ouaricon-VST3-NoteExpression.doricolib` from the same shared directory
      ```
    Do NOT modify other sections of README.md.
  </action>
  <verify>
    <automated>grep -q "^version: 1.1.0$" modules/tuning/note-expression/module.yaml && grep -A1 "version: 1.1.0" modules/tuning/note-expression/module.yaml | grep -q "date: 2026-04-26" && awk '/- name: note-expression/,/used_by:/' modules/registry.yaml | grep -q "version: 1.1.0" && grep -q "auto-discovery flow" modules/tuning/note-expression/README.md && grep -q "Default Library Additions" modules/tuning/note-expression/README.md && grep -q "DefaultLibraryAdditions" modules/tuning/note-expression/README.md && grep -q "Ouaricon-Microtonal-Suite.dorico_pt" modules/tuning/note-expression/README.md</automated>
  </verify>
  <acceptance_criteria>
    - `module.yaml` line 7 (or equivalent) is now `version: 1.1.0`
    - `module.yaml` contains a v1.1.0 changelog entry with date 2026-04-26
    - `registry.yaml`'s note-expression entry has `version: 1.1.0`
    - `README.md` "Dorico End-User Setup" section now references "auto-discovery flow"
    - `README.md` documents BOTH `Default Library Additions` (macOS) AND `DefaultLibraryAdditions` (Windows)
    - `README.md` references `Ouaricon-Microtonal-Suite.dorico_pt` (the new asset name)
  </acceptance_criteria>
  <done>Module bumped to 1.1.0 in both yaml files; README documents the new auto-discovery flow; Phase 24's 8-entry used_by list preserved.</done>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 8: O-Lyrica canary install — end-to-end pipeline proof</name>
  <read_first>
    - CLAUDE.md (lines 9-26 — cache-clear protocol; lines 31-41 — Windows protocol; not needed for canary)
    - .planning/phases/25-package-docs/25-PATTERNS.md S-1 (lines 710-732) and S-5 (lines 770-780)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-10, D-11, D-12, D-15, D-16 — full validation matrix lives in Plan 25-02; this is the canary only)
    - .planning/phases/25-package-docs/25-RESEARCH.md Q6 + Pitfall 3
  </read_first>
  <what-built>
    The full module-side pipeline: build O-Lyrica's VST3 (which fires the note-expression module's `module.cmake` per-consumer hook → packs `Ouaricon-Microtonal-Suite.dorico_pt` → runs `install(SCRIPT ...)` to dual-write both resources to user systems).
  </what-built>
  <how-to-verify>
    Run from project root:
    ```bash
    # Step 1: Configure + build O-Lyrica VST3 (and all 8 cohort _VST3 targets, which the helper requires).
    # The helper FATAL_ERRORs if any plugin's moduleinfo.json is missing — so build all 8.
    cd build
    ninja OLyrica_VST3 O-Bells_VST3 O-IntonationPad_VST3 O-Prism_VST3 O-Wind_VST3 O-Reed_VST3 O-Bowed_VST3 O-Formant_VST3
    # Step 2: Build the dorico_pt packing target explicitly (it's ALL, so the previous step should have triggered it; this is belt-and-braces).
    ninja ouaricon_microtonal_suite_pt
    # Step 3: Verify the .dorico_pt was packed correctly (no parent-dir wrapping — Pitfall 5).
    unzip -l Ouaricon-Microtonal-Suite.dorico_pt | head -5
    # Expected: first non-zero entries should be `PlaybackTemplateSpecs/Ouaricon Microtonal Suite/...` and `EndpointConfigs/Ouaricon Microtonal Suite/...`
    unzip -t Ouaricon-Microtonal-Suite.dorico_pt
    # Step 4: Run install for O-Lyrica only (cmake --install with COMPONENT or per-target install).
    cmake --install . --component <O-Lyrica install component> 2>&1 | tee /tmp/canary-install.log
    # Step 5: Verify dual-write happened.
    test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-Microtonal-Suite.dorico_pt" && echo "SHARED DORICO_PT OK"
    test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib" && echo "SHARED DORICOLIB OK"
    test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml" && echo "DORICO PT OK"
    test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" && echo "DORICO ENDPOINT OK"
    test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib" && echo "DORICO LIB OK"
    # Step 6: CID sanity — confirm the dev O-Lyrica CID was substituted into the installed endpointconfig.xml.
    ACTUAL_CID=$(python3 -c "import json,re; raw=open('$HOME/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3/Contents/Resources/moduleinfo.json').read(); raw=re.sub(r',(\\s*[}\\]])',r'\\1',raw); d=json.loads(raw); print([c['CID'] for c in d['Classes'] if c['Category']=='Audio Module Class'][0])")
    grep -q "$ACTUAL_CID" "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" && echo "OLYRICA CID SUBSTITUTED OK"
    grep -c "@.*_PLUGINID@" "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
    # Expected: 0 — all tokens were substituted at configure time.
    ```

    Then **manual Dorico verification**:
    1. Open Dorico 6.
    2. Open `Play -> Playback Template`. Confirm "Ouaricon Microtonal Suite" appears in the picker.
    3. Open `Library -> Expression Maps`. Confirm "Ouaricon VST3 Note Expression" appears.
    4. Apply the template to a fresh project. Observe whether O-Lyrica loads on the first slot (the other 7 should produce missing-plugin warnings — graceful per D-04).
    5. Quarter-sharp C4 smoke: write a quarter-sharp accidental on C4, play. Confirm pitch lands at +50¢ (~269.29 Hz).

    **Append "## Canary Install Result" section to `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md`** documenting the bash output, manual-Dorico observations, and PASS/FAIL.

    **Stop-on-first-failure (D-18):** If pipeline breaks (any test fails, any token unsubstituted, template not appearing in Dorico), HALT and escalate to `25-01-canary-FAIL-fix-PLAN.md` rather than patching forward.
  </how-to-verify>
  <acceptance_criteria>
    - All 5 `test -f` checks output their respective "OK" markers
    - `unzip -t Ouaricon-Microtonal-Suite.dorico_pt` returns "No errors detected in compressed data"
    - `unzip -l ... | head` does NOT show a wrapping parent dir before `PlaybackTemplateSpecs/`
    - `OLYRICA CID SUBSTITUTED OK` is echoed (proves @TOKEN@ substitution worked end-to-end)
    - `grep -c "@.*_PLUGINID@" ...` returns `0` (no unsubstituted tokens leaked through)
    - File `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` contains "## Canary Install Result" with PASS marker
    - Manual Dorico observation: template appears in picker AND quarter-sharp playback at +50¢ confirmed (recorded in the verification log)
  </acceptance_criteria>
  <resume-signal>Type "Canary PASS — proceed to Wave 2" or "Canary FAIL — promote to fix-plan"</resume-signal>
  <files>See plan frontmatter `files_modified` and `<read_first>` block above.</files>
  <action>
    (checkpoint task — see <what-built> and <how-to-verify> below for the verification protocol)
    
    WHAT BUILT:
    The full module-side pipeline: build O-Lyrica's VST3 (which fires the note-expression module's `module.cmake` per-consumer hook → packs `Ouaricon-Microtonal-Suite.dorico_pt` → runs `install(SCRIPT ...)` to dual-write both resources to user systems).
    
    HOW TO VERIFY:
    Run from project root:
        ```bash
        # Step 1: Configure + build O-Lyrica VST3 (and all 8 cohort _VST3 targets, which the helper requires).
        # The helper FATAL_ERRORs if any plugin's moduleinfo.json is missing — so build all 8.
        cd build
        ninja OLyrica_VST3 O-Bells_VST3 O-IntonationPad_VST3 O-Prism_VST3 O-Wind_VST3 O-Reed_VST3 O-Bowed_VST3 O-Formant_VST3
        # Step 2: Build the dorico_pt packing target explicitly (it's ALL, so the previous step should have triggered it; this is belt-and-braces).
        ninja ouaricon_microtonal_suite_pt
        # Step 3: Verify the .dorico_pt was packed correctly (no parent-dir wrapping — Pitfall 5).
        unzip -l Ouaricon-Microtonal-Suite.dorico_pt | head -5
        # Expected: first non-zero entries should be `PlaybackTemplateSpecs/Ouaricon Microtonal Suite/...` and `EndpointConfigs/Ouaricon Microtonal Suite/...`
        unzip -t Ouaricon-Microtonal-Suite.dorico_pt
        # Step 4: Run install for O-Lyrica only (cmake --install with COMPONENT or per-target install).
        cmake --install . --component <O-Lyrica install component> 2>&1 | tee /tmp/canary-install.log
        # Step 5: Verify dual-write happened.
        test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-Microtonal-Suite.dorico_pt" && echo "SHARED DORICO_PT OK"
        test -f "$HOME/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib" && echo "SHARED DORICOLIB OK"
        test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml" && echo "DORICO PT OK"
        test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" && echo "DORICO ENDPOINT OK"
        test -f "$HOME/Library/Application Support/Steinberg/Dorico 6/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib" && echo "DORICO LIB OK"
        # Step 6: CID sanity — confirm the dev O-Lyrica CID was substituted into the installed endpointconfig.xml.
        ACTUAL_CID=$(python3 -c "import json,re; raw=open('$HOME/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3/Contents/Resources/moduleinfo.json').read(); raw=re.sub(r',(\\s*[}\\]])',r'\\1',raw); d=json.loads(raw); print([c['CID'] for c in d['Classes'] if c['Category']=='Audio Module Class'][0])")
        grep -q "$ACTUAL_CID" "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" && echo "OLYRICA CID SUBSTITUTED OK"
        grep -c "@.*_PLUGINID@" "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
        # Expected: 0 — all tokens were substituted at configure time.
        ```
    
        Then **manual Dorico verification**:
        1. Open Dorico 6.
        2. Open `Play -> Playback Template`. Confirm "Ouaricon Microtonal Suite" appears in the picker.
        3. Open `Library -> Expression Maps`. Confirm "Ouaricon VST3 Note Expression" appears.
        4. Apply the template to a fresh project. Observe whether O-Lyrica loads on the first slot (the other 7 should produce missing-plugin warnings — graceful per D-04).
        5. Quarter-sharp C4 smoke: write a quarter-sharp accidental on C4, play. Confirm pitch lands at +50¢ (~269.29 Hz).
    
        **Append "## Canary Install Result" section to `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md`** documenting the bash output, manual-Dorico observations, and PASS/FAIL.
    
        **Stop-on-first-failure (D-18):** If pipeline breaks (any test fails, any token unsubstituted, template not appearing in Dorico), HALT and escalate to `25-01-canary-FAIL-fix-PLAN.md` rather than patching forward.
  </action>
  <verify><automated>see acceptance_criteria above (human-verified checkpoint; automated gate is on the recorded VERIFICATION.md file)</automated></verify>
  <done>All <acceptance_criteria> conditions above are satisfied.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| `cmake --install` (developer machine) → user filesystem under `~/Library/Application Support/` | The install script runs with the developer's user privileges and writes to user-controlled paths. No elevation required for canary; PKG/EXE postinstalls (Plan 25-02) elevate. |
| `cmake -E tar cf` build-time zip pack | All inputs are repo-controlled (configured XML in build tree). No external/untrusted input enters the archive. |
| `python3 -c "..."` JSON parser invoked from CMake | Reads `moduleinfo.json` from the project's own build tree; not user-controlled. |
| `git show cd2c2c6:...` recovery | Recovered XML body is treated as trusted (under our git history; reverted but reviewed). |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-01-01 | Tampering | `endpointconfig.xml.in` cross-file ID coupling (S-4) | mitigate | XML-lint + grep gate in Task 2 verifies byte-exact `xmap.ouaricon.vst3_note_expression` presence in 3 locations (endpointconfig + library + playbacktemplatedeps). Plan checker re-validates at the wave merge. |
| T-25-01-02 | Information disclosure | Dev CID baked into prod installer (Pitfall 2) | mitigate | `ouaricon_extract_vst3_cids` honors `${OUARICON_DEV_SUFFIX}` (S-3) — dev installer reads dev `moduleinfo.json` → ships dev CIDs; prod installer reads prod → ships prod CIDs. |
| T-25-01-03 | Denial of service | `module.cmake` fires for every consumer + builds all 8 VST3s required by helper | accept | `ouaricon_extract_vst3_cids` FATAL_ERRORs if any of the 8 bundles is missing. Forces ordering: build all 8 _VST3 targets before packaging. Documented in error message. Acceptable cost (dev convenience) for security against silent CID drift. |
| T-25-01-04 | Elevation of privilege | Canary `cmake --install` (no elevation) | accept | Canary writes only to user-owned `~/Library/Application Support/`. No root operations. Idempotent overwrite is safe. |
| T-25-01-05 | Tampering / Path traversal | `cmake -E tar xf` extraction destination | mitigate | Destination is a hard-coded `${DORICO_DIR}` that probes a fixed list of paths (`~/Library/Application Support/Steinberg/Dorico 6/...` only). Zip's internal layout (`PlaybackTemplateSpecs/`, `EndpointConfigs/`) is repo-controlled — verified by Wave 0 A4 + by `unzip -l` in canary verify. |
| T-25-01-06 | Information disclosure | Stale `install-doricoexpmap-<Plugin>.cmake` build outputs (D-21) | accept | Files are under `build/` (gitignored) and contain only file paths (no secrets). They vanish on next clean build because the v1 `.cmake.in` template no longer exists in the source tree (this plan replaces it with `install-microtonal-suite.cmake.in`). |
| T-25-01-07 | Tampering | Recovered XML from `git show cd2c2c6:...` | mitigate | Both target files are byte-exact equal (Task 1 verifies via `diff -q`). xmllint validates well-formedness. The recovered body is structurally validated against Phase 25's recovered-XML inspection in PATTERNS.md Pattern D. |
| T-25-01-08 | Idempotency / partial-state | Canary install fails mid-run, leaves orphan files | mitigate | install-microtonal-suite.cmake.in's `file(COPY)` is idempotent (overwrite). Partial extraction would be visible in `test -f` checks of Task 8's verify. If detected, escalate to `25-01-canary-FAIL-fix-PLAN.md` per D-18. |
</threat_model>

<verification>
Cross-task gates:
- Wave 0 (Tasks 0a, 0b) MUST pass before Tasks 1-7 run. If either fails, escalate per D-16.
- All XML files must `xmllint --noout` clean (S-5).
- Built `.dorico_pt` must `unzip -t` clean (S-5).
- Token substitution gate: `grep -c "@.*_PLUGINID@" <installed-endpointconfig.xml>` MUST be 0 (Pitfall 2 mitigation evidence).
- Cross-file ID coupling gate (S-4): the byte-exact string `xmap.ouaricon.vst3_note_expression` MUST appear in:
  1. `library/Ouaricon-VST3-NoteExpression.doricolib` (1 occurrence)
  2. `playback-template/EndpointConfigs/.../playbacktemplatedeps.doricolib.in` (1 occurrence)
  3. `playback-template/EndpointConfigs/.../endpointconfig.xml.in` (8 occurrences — one per slot)
  Total (after stripping comment lines): exactly 10 across 3 files.
- Canary (Task 8) is the structural integration gate. Pipeline must work end-to-end on macOS for O-Lyrica before Plan 25-02 begins atomic 8-plugin sweep.
</verification>

<success_criteria>
- All 6 NEW files exist with correct content
- All 5 MODIFIED files updated correctly (lines 1-42 of module.cmake preserved verbatim; module.yaml + registry.yaml versions bumped; module README updated)
- Wave 0 verifications (A2, A4) PASS and recorded in `25-01-WAVE-0-VERIFICATION.md`
- Canary install on O-Lyrica writes both resources to BOTH canonical user paths
- `Ouaricon Microtonal Suite` appears in Dorico 6 `Play -> Playback Template`
- `Ouaricon VST3 Note Expression` appears in Dorico 6 `Library -> Expression Maps`
- Quarter-sharp C4 plays at +50¢ via the applied template (3-point gate satisfied for the canary plugin)
- All 8 `@<NAME>_PLUGINID@` tokens substituted (no unsubstituted tokens leak into installed `endpointconfig.xml`)
- Module v1.1.0 visible in `modules/registry.yaml` and `modules/tuning/note-expression/module.yaml`
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-01-SUMMARY.md` documenting:
- A2 + A4 + canary verification results (with timestamps and observed Dorico behavior)
- The exact `OLyrica` dev CID extracted (proof of `ouaricon_extract_vst3_cids` correctness)
- All 8 cohort plugin CIDs recorded by the helper (one-time table for downstream debugging)
- File-tree snapshot of `modules/tuning/note-expression/resources/` (post-creation)
- Pipeline gates passed: XML wellformedness × 3, zip integrity × 1, dual-write × 5
- Any A2/A4 escalation notes (if applicable)
- "Phase 25 Plan 25-02 unblocked" closing line

Update `.planning/STATE.md` with completion timestamp and module v1.1.0 note.
</output>
</content>
</invoke>