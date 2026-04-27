---
phase: 25-package-docs
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
  - modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in
  - modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in
  - modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in
  - modules/tuning/note-expression/resources/README-microtonal-suite.txt
  - modules/tuning/note-expression/module.cmake
  - modules/tuning/note-expression/install-microtonal-suite.cmake.in
  - modules/tuning/note-expression/README.md
  - modules/cmake/OuariconModules.cmake
autonomous: false
requirements: [INST-01, INST-02]
tags: [dorico, doricolib, microtuning, vst3-note-expression, cmake, path-b]

must_haves:
  truths:
    - "A Dorico-valid Ouaricon-VST3-NoteExpression.doricolib (full 48-container kScoreLibrary skeleton + injected ExpressionMapDefinition) lives in the repo at modules/tuning/note-expression/resources/library/."
    - "Path A artifacts (.dorico_pt packing, ouaricon_extract_vst3_cids helper, three .xml.in templates, embedded .doricolib.in) are deleted from the working tree."
    - "module.cmake's install logic is single-write of one .doricolib + README to the Ouaricon shared path; no Dorico auto-discovery dual-write."
    - "Wave 0 informational auto-discovery probe result is logged into 25-01-WAVE-0-VERIFICATION.md (PASS or FAIL — informational, non-blocking)."
    - "O-Lyrica canary install lands the .doricolib at ~/Library/Application Support/Ouaricon/Microtonal Suite/ and Library Manager Import + quarter-sharp C4 (~269 Hz) PASS."
  artifacts:
    - path: "modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib"
      provides: "Canonical Path B asset — full kScoreLibrary skeleton with injected ExpressionMapDefinition"
      contains: "<entityID>xmap.ouaricon.vst3_note_expression</entityID>"
    - path: "modules/tuning/note-expression/resources/README-microtonal-suite.txt"
      provides: "User-facing fallback docs for Path B import flow (INST-04 partial)"
    - path: "modules/tuning/note-expression/install-microtonal-suite.cmake.in"
      provides: "Single-write install script: .doricolib + README to Ouaricon shared path only"
    - path: "modules/tuning/note-expression/module.cmake"
      provides: "Module-owned install rule (no .dorico_pt packing, no CID extraction)"
    - path: ".planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md"
      provides: "Wave 0 auto-discovery probe result + canary install log"
  key_links:
    - from: "modules/tuning/note-expression/module.cmake"
      to: "modules/tuning/note-expression/install-microtonal-suite.cmake.in"
      via: "configure_file @ONLY → install(SCRIPT)"
      pattern: "configure_file.*install-microtonal-suite\\.cmake\\.in"
    - from: "install-microtonal-suite.cmake.in"
      to: "~/Library/Application Support/Ouaricon/Microtonal Suite/"
      via: "file(COPY) + file(MAKE_DIRECTORY)"
      pattern: "Ouaricon/Microtonal Suite"
    - from: "Ouaricon-VST3-NoteExpression.doricolib"
      to: "Dorico expression-map dropdown"
      via: "user one-time Library Manager Import"
      pattern: "xmap.ouaricon.vst3_note_expression"
---

<objective>
Author the canonical Path B asset (single Dorico-valid `.doricolib`), surgically delete all Path A artifacts from commit `819b2b4`, collapse module install logic to single-write, rewrite module-side READMEs for the import flow, and prove the pipeline with an O-Lyrica canary install + Library Manager Import + quarter-sharp smoke test.

Purpose: Replace the truncated-fragment `.doricolib` with a Dorico-valid library bundle (full 48-container `<kScoreLibrary>` skeleton + injected `<ExpressionMapDefinition>`) and remove the over-engineered Path A plumbing the v2 work introduced. After this plan completes, the module owns one canonical asset that installs to one platform-specific Ouaricon shared path; Plan 25-02 will then sweep installer plumbing across all 8 plugins.

Output: A reauthored `.doricolib` byte-functionally equivalent to `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B Dorico-valid reference), surgically clean module/CMake state with no Path A artifacts, and a verified O-Lyrica canary install proving the single-asset single-path pipeline works on macOS Dorico 6.

Per D-10 (amend-forward, not full revert): preserve module v1.1.0 bump, registry entry, README skeleton, version-probe pattern, and recovered XML body. Surgically delete only Path A-specific files and code regions, each in its own atomic commit.
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
@$HOME/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/phases/25-package-docs/25-CONTEXT.md
@.planning/phases/25-package-docs/25-FINDING-path-b-validation.md
@.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md
@.planning/phases/25-package-docs/25-01-SUMMARY.md
@.planning/phases/25-package-docs/25-PATTERNS.md
@CLAUDE.md
@modules/tuning/note-expression/module.cmake
@modules/tuning/note-expression/install-microtonal-suite.cmake.in
@modules/tuning/note-expression/README.md
@modules/cmake/OuariconModules.cmake
@modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib

<interfaces>
<!-- Reference asset (Dorico-valid; Library Manager Import + quarter-sharp PASS verified 2026-04-27) -->
File: /tmp/Ouaricon-VST3-NoteExpression-v2.doricolib (6,431 bytes, 199 lines)
- Root: <kScoreLibrary> with EXACTLY 48 top-level child containers (verified via xmllint: count(/kScoreLibrary/*) == 48)
- 47 of 48 containers are empty: <containerName><entities array="true" /></containerName>
- 1 container populated: <expressionMapDefinitions><entities array="true"><ExpressionMapDefinition>...</ExpressionMapDefinition></entities></expressionMapDefinitions>
- Special case: <instrumentNames> has <entities array="true" /> + <language>kEnglish</language> as siblings.

Factory skeleton source (read this for the 48-container structure):
/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml
- Provides authoritative 48-container <kScoreLibrary> skeleton.

Recovered ExpressionMapDefinition body (already in current invalid file, lines 6-56):
modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib lines 6-56
- entityID: xmap.ouaricon.vst3_note_expression  (LOAD-BEARING — must match across all references)
- microtonalPlaybackMethod: kVST3NoteExpression  (LOAD-BEARING — Landmine 3, never kAuto/kPitchBend)
- name: "Ouaricon VST3 Note Expression"
- creator: "Ouaricon Audio"
- pluginNames: <pluginNames/> (CID-free per D-02)
- pitchBendRange: 2
- One playingTechniqueCombination: pt.natural

CMake function signatures (preserved/modified):
- ouaricon_add_module(TARGET_NAME MODULE_NAME [CONFIG <yaml>])  -- preserved verbatim
- ouaricon_extract_vst3_cids(OUTPUT_VAR <var> PLUGINS <list>)   -- DELETED in this plan (D-10)
- include path for module hook: ${MODULE_DIR}/module.cmake fires per-consumer

Install destinations (Path B, D-07):
- macOS:   ~/Library/Application Support/Ouaricon/Microtonal Suite/
- Windows: %APPDATA%/Ouaricon/Microtonal Suite/
NO writes to Dorico auto-discovery directories under v3.
</interfaces>
</context>

<tasks>

<task type="checkpoint:human-verify" gate="non-blocking">
  <name>Task 0: Wave 0 informational auto-discovery probe (D-08)</name>
  <files>.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md</files>
  <read_first>
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-08 spec)
    - .planning/phases/25-package-docs/25-FINDING-path-b-validation.md (Path B context)
    - .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md (existing Wave 0 log to append into)
  </read_first>
  <action>Execute the human-verified Wave 0 informational auto-discovery probe described in <how-to-verify>. The task pauses for the user to perform the probe steps and report PASS/FAIL/SKIPPED via the resume-signal. No autonomous code action is performed by the executor for this task — it only records the result in the verification log file.</action>
  <what-built>
    Reference asset already exists at `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B, Dorico-valid). This task does NOT modify production files. It runs a single ~5 min informational probe to inform v1.6 deferred-ideas.
  </what-built>
  <how-to-verify>
    Inform the user that Claude is about to run an informational probe (non-blocking, regardless of outcome the plan continues with explicit-import per D-01). Then have the user run these steps and report back:

    1. Quit Dorico if open: `osascript -e 'quit app "Dorico 6"'` (or close manually)
    2. Copy the reference asset into Dorico's user expression-map directory:
       `mkdir -p "$HOME/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User"`
       `cp /tmp/Ouaricon-VST3-NoteExpression-v2.doricolib "$HOME/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/"`
    3. Launch Dorico 6, open any project that contains an Ouaricon plugin (e.g. O-Lyrica-dev).
    4. Open Play → Endpoints → click the channel of the Ouaricon plugin → look at the "Expression Map" dropdown.
    5. Report: does "Ouaricon VST3 Note Expression" appear in the dropdown WITHOUT performing any Library → Library Manager → Import action?
       - If YES → auto-discovery PASS (informational; logged for v1.6).
       - If NO → auto-discovery FAIL (expected; explicit import is the v1.5 ship behavior).
    6. Cleanup (regardless of result):
       `rm "$HOME/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/Ouaricon-VST3-NoteExpression-v2.doricolib"`

    Append the result (date, PASS/FAIL, Dorico version, build flavor used) to `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` under a new H2 section `## Wave 0 v3 — Auto-discovery Probe Result`.
  </how-to-verify>
  <acceptance_criteria>
    - `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` contains a new H2 section `## Wave 0 v3 — Auto-discovery Probe Result`
    - Section records: probe date (YYYY-MM-DD), Dorico version, plugin build flavor, PASS or FAIL verdict, and one-line evidence (e.g., "Map appeared in Endpoints dropdown" or "Map did NOT appear; explicit Library Manager Import required")
    - Section explicitly states "Informational only — does not affect v3 ship behavior (D-01 ships explicit-import)"
    - The temporary `.doricolib` was removed from `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/` after the probe (verify with `ls`)
  </acceptance_criteria>
  <resume-signal>Type "probe-pass", "probe-fail", or "probe-skipped" along with any notes; planner continues regardless.</resume-signal>
</task>

<task type="auto">
  <name>Task 1: Reauthor canonical .doricolib from factory skeleton + injected ExpressionMapDefinition (D-03)</name>
  <files>modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib</files>
  <read_first>
    - /tmp/Ouaricon-VST3-NoteExpression-v2.doricolib (the 6,431 B Dorico-valid reference; produce a byte-functionally-equivalent file)
    - /Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml (factory 48-container skeleton authority)
    - modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib (current invalid fragment; lines 6-56 contain the recovered <ExpressionMapDefinition> body to inject)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-03 protocol; D-02 CID-free constraint)
    - .planning/phases/25-package-docs/25-FINDING-path-b-validation.md (why fragment was rejected)
  </read_first>
  <action>
    Reauthor `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` per D-03. The simplest correct path is to copy the verified reference asset and make it the new canonical file:

    ```bash
    cp /tmp/Ouaricon-VST3-NoteExpression-v2.doricolib \
       modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
    ```

    The reference asset was constructed exactly per D-03:
    1. Loaded factory skeleton from `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml` (the authoritative 48-container `<kScoreLibrary>`).
    2. Emptied every container except `<expressionMapDefinitions>`. Each emptied container has form `<ContainerName><entities array="true" /></ContainerName>`. Special case: `<instrumentNames>` retains its `<language>kEnglish</language>` element alongside `<entities array="true" />`.
    3. Injected the recovered `<ExpressionMapDefinition>` (entityID `xmap.ouaricon.vst3_note_expression`, microtonalPlaybackMethod `kVST3NoteExpression`, creator "Ouaricon Audio", pluginNames empty per D-02) into `<expressionMapDefinitions>/<entities array="true">`.
    4. The result was Library-Manager-Import PASS + quarter-sharp C4 ~269 Hz PASS on macOS Dorico 6 / O-Lyrica-dev (2026-04-27).

    Verify the copied file matches the reference byte-for-byte (`diff -q` clean), is XML-well-formed, has 48 top-level `<kScoreLibrary>` children, and preserves the load-bearing invariants. Stage as a single atomic commit.
  </action>
  <verify>
    <automated>
      diff -q /tmp/Ouaricon-VST3-NoteExpression-v2.doricolib modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib && \
      xmllint --noout modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib && \
      test "$(xmllint --xpath 'count(/kScoreLibrary/*)' modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib)" = "48" && \
      grep -c '&lt;entityID&gt;xmap.ouaricon.vst3_note_expression&lt;/entityID&gt;' modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
    </automated>
  </verify>
  <acceptance_criteria>
    - `diff -q /tmp/Ouaricon-VST3-NoteExpression-v2.doricolib modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` exits 0 (byte-identical to validated reference)
    - `xmllint --noout` on the file exits 0 (XML well-formed)
    - `xmllint --xpath 'count(/kScoreLibrary/*)' file` returns `48` (full container skeleton present)
    - File contains exactly 1 occurrence of `<entityID>xmap.ouaricon.vst3_note_expression</entityID>` (load-bearing ID)
    - File contains exactly 1 occurrence of `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` (load-bearing method, never kAuto/kPitchBend)
    - File contains `<pluginNames/>` (self-closing, empty — D-02 CID-free)
    - File contains `<creator>Ouaricon Audio</creator>`
    - File size is 6,431 bytes (`wc -c` matches reference)
    - Atomic git commit `feat(25-01): reauthor canonical .doricolib with full kScoreLibrary skeleton (D-03)` is the only file in the commit
  </acceptance_criteria>
  <done>
    Canonical `.doricolib` is byte-identical to the verified reference; XML well-formed; 48 top-level containers; load-bearing entityID + microtonalPlaybackMethod present; CID-free; committed atomically.
  </done>
</task>

<task type="auto">
  <name>Task 2: Surgical deletion of Path A artifacts (D-10) — playback-template subtree + helper + .dorico_pt packing</name>
  <files>
    modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in
    modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in
    modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in
    modules/cmake/OuariconModules.cmake
    modules/tuning/note-expression/module.cmake
  </files>
  <read_first>
    - modules/tuning/note-expression/module.cmake (current state; lines 1-41 are the JUCE-NE-PATCH marker check that MUST be preserved verbatim; lines 43-132 are the Path A Microtonal Suite block to surgically replace)
    - modules/cmake/OuariconModules.cmake (current state; lines 1-188 are preserved verbatim; lines 190-258 are the `ouaricon_extract_vst3_cids` helper — DELETE entirely)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-10 surgical-amend strategy; D-04 single-asset ownership)
    - .planning/phases/25-package-docs/25-FINDING-path-b-validation.md (Trash section: list of artifacts to delete)
    - .planning/phases/25-package-docs/25-01-SUMMARY.md (audit trail of what was added in 819b2b4 — confirms the 7 files this task touches were added by Plan 25-01 v2)
  </read_first>
  <action>
    Surgically remove Path A artifacts per D-10. This task makes three groups of deletions, committed as ONE atomic commit (or three sub-commits if you prefer cleaner audit trail; see <done>):

    **Group 1 — Delete playback-template subtree (3 files):**
    ```bash
    git rm modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon\ Microtonal\ Suite/playbacktemplatespec.xml.in
    git rm modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon\ Microtonal\ Suite/endpointconfig.xml.in
    git rm modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon\ Microtonal\ Suite/playbacktemplatedeps.doricolib.in
    # Remove now-empty directories:
    rmdir "modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite"
    rmdir "modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs"
    rmdir "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite"
    rmdir "modules/tuning/note-expression/resources/playback-template/EndpointConfigs"
    rmdir "modules/tuning/note-expression/resources/playback-template"
    ```

    **Group 2 — Delete `ouaricon_extract_vst3_cids` from `modules/cmake/OuariconModules.cmake`:**
    Remove the entire block from the comment header `# ==============================================================================` immediately preceding `# ouaricon_extract_vst3_cids(OUTPUT_VAR <var> PLUGINS <list>)` (currently around line 190) through the closing `endfunction()` (currently around line 257). Preserve the file's first 188 lines (the existing `ouaricon_add_module`, `ouaricon_list_modules`, `ouaricon_check_module_updates` functions and section headers) verbatim.

    After the edit, `modules/cmake/OuariconModules.cmake` should end at the `endfunction()` of `ouaricon_check_module_updates` and contain exactly 3 functions (down from 4): `ouaricon_add_module`, `ouaricon_list_modules`, `ouaricon_check_module_updates`.

    **Group 3 — Delete `.dorico_pt` packing block + collapse module.cmake (D-04, D-06, D-07):**
    Edit `modules/tuning/note-expression/module.cmake`:
    - PRESERVE lines 1-41 verbatim (JUCE-NE-PATCH marker check; verify with `head -41 module.cmake | diff` against pre-edit version).
    - REPLACE lines 43-132 (Microtonal Suite Path A block + per-consumer install hook) with a NEW Path B block. The new block:

    ```cmake
    # ==============================================================================
    # Microtonal Suite — Dorico .doricolib distribution (Path B, Phase 25 v3)
    #
    # Per D-04: this module owns the single distributable resource. Any consumer of
    # ouaricon_add_module(<plugin> note-expression) automatically inherits a
    # per-consumer install(SCRIPT ...) rule that single-writes the .doricolib +
    # README to the platform-specific Ouaricon shared path (D-07):
    #   macOS:   ~/Library/Application Support/Ouaricon/Microtonal Suite/
    #   Windows: %APPDATA%/Ouaricon/Microtonal Suite/
    #
    # User performs a one-time Library Manager Import per machine (D-01). No
    # auto-discovery, no Playback Template, no plugin CID extraction.
    # ==============================================================================

    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/install-microtonal-suite.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
            COMPONENT "ouaricon_note_expression_${TARGET_NAME}")
    ```

    The replacement block is purely the configure_file + install(SCRIPT) pair — no `_OUARICON_SUITE_PT_FILE`, no `DORICO_PT_STAGE`, no `add_custom_command(... tar cf --format=zip ...)`, no `add_custom_target(ouaricon_microtonal_suite_pt ALL ...)`, no `if(NOT TARGET ouaricon_microtonal_suite_pt)` guard, no call to `ouaricon_extract_vst3_cids`. The Path B block is ~25 lines vs Path A's ~90 lines.

    Stage all three groups together. Recommended commit message: `chore(25-01): surgical delete of Path A artifacts (D-10)` with body listing the three groups.
  </action>
  <verify>
    <automated>
      test ! -d modules/tuning/note-expression/resources/playback-template && \
      ! grep -q 'ouaricon_extract_vst3_cids' modules/cmake/OuariconModules.cmake && \
      ! grep -q 'dorico_pt' modules/tuning/note-expression/module.cmake && \
      ! grep -q 'tar cf' modules/tuning/note-expression/module.cmake && \
      ! grep -q 'DORICO_PT_STAGE' modules/tuning/note-expression/module.cmake && \
      ! grep -q 'ouaricon_microtonal_suite_pt' modules/tuning/note-expression/module.cmake && \
      grep -q 'JUCE-NE-PATCH markers verified' modules/tuning/note-expression/module.cmake && \
      grep -q 'install-microtonal-suite-\${TARGET_NAME}\.cmake' modules/tuning/note-expression/module.cmake && \
      grep -q 'COMPONENT "ouaricon_note_expression_\${TARGET_NAME}"' modules/tuning/note-expression/module.cmake
    </automated>
  </verify>
  <acceptance_criteria>
    - Directory `modules/tuning/note-expression/resources/playback-template/` does NOT exist
    - `git ls-files modules/tuning/note-expression/resources/playback-template/` returns empty
    - `modules/cmake/OuariconModules.cmake` does NOT contain string `ouaricon_extract_vst3_cids` (case-sensitive grep)
    - `modules/cmake/OuariconModules.cmake` contains exactly 3 `^function\(` lines (down from 4): `ouaricon_add_module`, `ouaricon_list_modules`, `ouaricon_check_module_updates`
    - `modules/tuning/note-expression/module.cmake` does NOT contain strings: `dorico_pt`, `tar cf`, `DORICO_PT_STAGE`, `ouaricon_microtonal_suite_pt`, `add_custom_command`, `add_custom_target`, `_OUARICON_SUITE_PT_FILE`, `ouaricon_extract_vst3_cids`
    - `modules/tuning/note-expression/module.cmake` lines 1-41 are byte-identical to the pre-edit version (verify with `git show HEAD~:modules/tuning/note-expression/module.cmake | head -41 | diff -q - <(head -41 modules/tuning/note-expression/module.cmake)` returns clean)
    - `modules/tuning/note-expression/module.cmake` contains exactly one `configure_file(` call and exactly one `install(SCRIPT` directive
    - `modules/tuning/note-expression/module.cmake` contains string `install-microtonal-suite-${TARGET_NAME}.cmake` (per-consumer per-target install script name preserved)
    - Module's `note-expression/cpp/` source surface is untouched (verified by `git diff --name-only HEAD~ -- modules/tuning/note-expression/cpp/` returning empty)
  </acceptance_criteria>
  <done>
    Path A artifacts deleted; module.cmake collapsed to Path B (configure_file + install(SCRIPT) only); JUCE-NE-PATCH marker check preserved verbatim; OuariconModules.cmake has 3 functions remaining; module source surface untouched. One atomic commit (or three sub-commits per D-10's audit-trail-favoring guidance — choose based on feedback).
  </done>
</task>

<task type="auto">
  <name>Task 3: Collapse install-microtonal-suite.cmake.in to single-write (D-07)</name>
  <files>modules/tuning/note-expression/install-microtonal-suite.cmake.in</files>
  <read_first>
    - modules/tuning/note-expression/install-microtonal-suite.cmake.in (current 87-line dual-write template; preserve overall shape, collapse content)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-07 single-write spec)
    - .planning/phases/25-package-docs/25-FINDING-path-b-validation.md (Trash section: dual-write logic to remove)
  </read_first>
  <action>
    Rewrite `modules/tuning/note-expression/install-microtonal-suite.cmake.in` to a single-write per platform per D-07. The new template writes the `.doricolib` + README to the Ouaricon shared path only — no Dorico auto-discovery dual-write, no .dorico_pt extraction, no `Default Library Additions/` copy.

    Replace file contents with the following (whole-file rewrite):

    ```cmake
    # ==============================================================================
    # install-microtonal-suite.cmake.in — per-platform single-write install script
    #
    # Phase 25 v3 (Path B). Configured by module.cmake via configure_file(... @ONLY)
    # at build time; executed by `cmake --install` at install time. Writes the
    # canonical .doricolib + user-facing README to the platform-specific Ouaricon
    # shared path (D-07):
    #   macOS:   ~/Library/Application Support/Ouaricon/Microtonal Suite/
    #   Windows: %APPDATA%/Ouaricon/Microtonal Suite/
    #
    # User performs a one-time Library Manager Import per machine (D-01). No
    # writes to Dorico auto-discovery directories.
    #
    # NOTE (D-07 carry-forward): the v2 template's Dorico-version probe (foreach
    # over 6 -> 5 -> 4) is intentionally absent here. The Ouaricon shared path is
    # Dorico-version-agnostic. The probe pattern remains a v1.6 revival candidate
    # if auto-discovery work is added; reference the v2 template at git show
    # 819b2b4:modules/tuning/note-expression/install-microtonal-suite.cmake.in.
    #
    # Idempotent overwrite: file(COPY) overwrites in place; safe for all-8-
    # installers writing identical content.
    # ==============================================================================

    set(SUITE_LIB    "@CMAKE_CURRENT_LIST_DIR@/resources/library/Ouaricon-VST3-NoteExpression.doricolib")
    set(SUITE_README "@CMAKE_CURRENT_LIST_DIR@/resources/README-microtonal-suite.txt")

    if(APPLE)
        set(SHARED_DIR "$ENV{HOME}/Library/Application Support/Ouaricon/Microtonal Suite")
    elseif(WIN32)
        set(SHARED_DIR "$ENV{APPDATA}/Ouaricon/Microtonal Suite")
    else()
        message(STATUS "[Ouaricon] Microtonal Suite install: unsupported platform; skipping")
        return()
    endif()

    file(MAKE_DIRECTORY "${SHARED_DIR}")
    file(COPY "${SUITE_LIB}" DESTINATION "${SHARED_DIR}")
    if(EXISTS "${SUITE_README}")
        file(COPY "${SUITE_README}" DESTINATION "${SHARED_DIR}")
    endif()
    message(STATUS "[Ouaricon] Microtonal Suite installed: ${SHARED_DIR}")
    ```

    The new file is ~40 lines (down from 87). It contains: NO `SUITE_PT` variable, NO `cmake -E tar xf` call, NO `foreach(_v 6 5 4)` loop, NO `IS_DIRECTORY "${DORICO_DIR}"` check, NO `Default Library Additions` or `DefaultLibraryAdditions` mkdir, NO second `file(COPY)` to a Dorico version path. Single `file(COPY)` for the .doricolib, optional `file(COPY)` for the README, both targeting `${SHARED_DIR}` only. Stage as a single atomic commit `chore(25-01): collapse install script to single-write (D-07)`.
  </action>
  <verify>
    <automated>
      ! grep -q 'SUITE_PT' modules/tuning/note-expression/install-microtonal-suite.cmake.in && \
      ! grep -q 'tar xf' modules/tuning/note-expression/install-microtonal-suite.cmake.in && \
      ! grep -q 'foreach.*_v' modules/tuning/note-expression/install-microtonal-suite.cmake.in && \
      ! grep -q 'Default Library Additions' modules/tuning/note-expression/install-microtonal-suite.cmake.in && \
      ! grep -q 'DefaultLibraryAdditions' modules/tuning/note-expression/install-microtonal-suite.cmake.in && \
      ! grep -q 'Steinberg/Dorico' modules/tuning/note-expression/install-microtonal-suite.cmake.in && \
      test "$(grep -c '^[^#]*file(COPY' modules/tuning/note-expression/install-microtonal-suite.cmake.in)" = "2" && \
      grep -q 'Ouaricon/Microtonal Suite' modules/tuning/note-expression/install-microtonal-suite.cmake.in && \
      grep -q 'SUITE_LIB' modules/tuning/note-expression/install-microtonal-suite.cmake.in
    </automated>
  </verify>
  <acceptance_criteria>
    - File does NOT contain strings: `SUITE_PT`, `tar xf`, `Default Library Additions`, `DefaultLibraryAdditions`, `Steinberg/Dorico`, `dorico_pt`
    - File does NOT contain a `foreach(_v` loop (Dorico-version probe absent)
    - File contains exactly 2 non-comment `file(COPY` lines (one for SUITE_LIB, one for SUITE_README inside `if(EXISTS)`)
    - File contains both `if(APPLE)` (with `~/Library/Application Support/Ouaricon/Microtonal Suite`) and `elseif(WIN32)` (with `$ENV{APPDATA}/Ouaricon/Microtonal Suite`) branches
    - File defines `SUITE_LIB` pointing at `@CMAKE_CURRENT_LIST_DIR@/resources/library/Ouaricon-VST3-NoteExpression.doricolib`
    - File total non-empty non-comment lines is < 25 (sanity: collapsed from ~70 substantive lines down to <25)
    - File preserves the comment block referencing v2 carry-forward (mentions `git show 819b2b4` so future revival is discoverable)
    - Atomic commit `chore(25-01): collapse install script to single-write (D-07)` modifies only this file
  </acceptance_criteria>
  <done>
    install-microtonal-suite.cmake.in is single-write to Ouaricon shared path on macOS + Windows; no Dorico-version probe, no .dorico_pt extraction, no Default Library Additions write; comment block preserves v2 carry-forward reference for v1.6 revival.
  </done>
</task>

<task type="auto">
  <name>Task 4: Rewrite README-microtonal-suite.txt + module README "Dorico End-User Setup" for Path B import flow</name>
  <files>
    modules/tuning/note-expression/resources/README-microtonal-suite.txt
    modules/tuning/note-expression/README.md
  </files>
  <read_first>
    - modules/tuning/note-expression/resources/README-microtonal-suite.txt (current Path A content; structure preserved, content rewritten)
    - modules/tuning/note-expression/README.md lines 155-200 (current "Dorico End-User Setup (v1.1.0+ auto-discovery flow)" Path A section to rewrite)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-01 manual-import flow; D-12 doc structure)
    - .planning/phases/25-package-docs/25-FINDING-path-b-validation.md ("What works (Path B — validated 2026-04-27)" section: 5-step user flow)
  </read_first>
  <action>
    Two coordinated content rewrites for Path B. Structure preserved in both files; content rewritten to describe the explicit-import flow.

    **(A) `modules/tuning/note-expression/resources/README-microtonal-suite.txt`** — User-facing fallback. Preserve 6-section structure (Title / PURPOSE / INSTALL LOCATIONS / MANUAL IMPORT / SOURCE OF TRUTH / SUPPORTED PLUGINS) but rewrite content for Path B. Sections:

    - Title: `OUARICON MICROTONAL SUITE — DORICO EXPRESSION MAP` with `===` underline
    - PURPOSE: One paragraph explaining this `.doricolib` adds an "Ouaricon VST3 Note Expression" expression map to Dorico, which routes microtonal pitches as VST3 Note Expression events to Ouaricon plugins. Manual one-time import per machine.
    - INSTALL LOCATIONS: 2 paths (one per platform):
      - macOS: `~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib`
      - Windows: `%APPDATA%\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib`
    - MANUAL IMPORT (one-time per machine):
      1. Open Dorico (any project).
      2. `Library → Library Manager → Import…`
      3. Select the `.doricolib` from the install location above.
      4. Confirm; the expression map "Ouaricon VST3 Note Expression" now appears under `Library → Expression Maps`.
      5. Per project: load any Ouaricon plugin (`Play → Endpoints → Add Plug-in`), then assign the expression map to the plugin's channel via `Play → Endpoints → Expression Map` dropdown.
    - SOURCE OF TRUTH: This file ships from `modules/tuning/note-expression/resources/library/` in the Ouaricon plugin source repo. DO NOT edit installed copies — edit the canonical source.
    - SUPPORTED PLUGINS (v1.5 cohort): O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant.
    - Trailing signature: `— Ouaricon Audio, v1.5 (Phase 25 v3 / Path B)`.

    Tone: terse, technical. No `.dorico_pt`, no Playback Template, no auto-discovery, no `Default Library Additions` references.

    **(B) `modules/tuning/note-expression/README.md`** — replace lines 155-end-of-section (currently "## Dorico End-User Setup (v1.1.0+ auto-discovery flow)" through and including the auto-discovery + manual fallback subsections) with a NEW "## Dorico End-User Setup (v1.1.0+ Path B import flow)" section. The new section uses 3 subsections (preserve structure from D-12 / per CONTEXT recommendation):
    - **Quick Start** (3 bullets): install plugin → run Library Manager Import once → assign map per-project
    - **Manual Import Steps** (numbered list, mirrors README-microtonal-suite.txt section MANUAL IMPORT)
    - **Source of Truth** (one paragraph: canonical asset path in repo; install destinations; reminder that auto-discovery is a v1.6 candidate, not shipped in v1.5)

    The new section MUST NOT mention: `.dorico_pt`, `PlaybackTemplateSpecs`, `Default Library Additions`, `DefaultLibraryAdditions`, "Auto-discovery", "Playback Template apply".

    The "Underlying mechanics" paragraph from the existing README (which describes the kVST3NoteExpression invariant) should be preserved verbatim — it remains technically accurate under Path B.

    Stage as a single atomic commit `docs(25-01): rewrite Path B import flow in module READMEs`.
  </action>
  <verify>
    <automated>
      ! grep -q 'dorico_pt\|PlaybackTemplateSpecs\|Default Library Additions\|DefaultLibraryAdditions\|auto-discovery\|Playback Template -> Ouaricon' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'Library Manager.*Import' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'Ouaricon/Microtonal Suite' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-Lyrica' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-Bells' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-IntonationPad' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-Prism' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-Wind' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-Reed' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-Bowed' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      grep -q 'O-Formant' modules/tuning/note-expression/resources/README-microtonal-suite.txt && \
      ! grep -q 'dorico_pt\|PlaybackTemplateSpecs\|Default Library Additions\|DefaultLibraryAdditions' modules/tuning/note-expression/README.md && \
      grep -q 'Library Manager' modules/tuning/note-expression/README.md && \
      grep -q 'kVST3NoteExpression' modules/tuning/note-expression/README.md
    </automated>
  </verify>
  <acceptance_criteria>
    - `README-microtonal-suite.txt` does NOT contain strings: `dorico_pt`, `PlaybackTemplateSpecs`, `Default Library Additions`, `DefaultLibraryAdditions`, `auto-discovery` (case-insensitive), `Playback Template`
    - `README-microtonal-suite.txt` contains string `Library Manager` and `Import…` (or `Import...`)
    - `README-microtonal-suite.txt` contains both install paths: `~/Library/Application Support/Ouaricon/Microtonal Suite/` AND `%APPDATA%\Ouaricon\Microtonal Suite\`
    - `README-microtonal-suite.txt` lists all 8 cohort plugin names: O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant
    - `README-microtonal-suite.txt` contains 6 H1-style section headers (PURPOSE, INSTALL LOCATIONS, MANUAL IMPORT, SOURCE OF TRUTH, SUPPORTED PLUGINS, plus title)
    - `modules/tuning/note-expression/README.md` does NOT contain `dorico_pt`, `PlaybackTemplateSpecs`, `Default Library Additions`, `DefaultLibraryAdditions`
    - `modules/tuning/note-expression/README.md` contains string `Library Manager`
    - `modules/tuning/note-expression/README.md` preserves the "Underlying mechanics" paragraph mentioning `kVST3NoteExpression` (verify presence; this technical-mechanics paragraph survives the rewrite)
    - `modules/tuning/note-expression/README.md` contains a section heading containing both `Path B` and `Dorico End-User Setup`
    - Atomic commit `docs(25-01): rewrite Path B import flow in module READMEs` modifies exactly these 2 files
  </acceptance_criteria>
  <done>
    Both READMEs describe Path B's manual-import flow with no Path A residue. Module README preserves technical mechanics paragraph; user-facing README preserves 6-section structure with Path B content. Single atomic commit.
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 5: O-Lyrica canary install + Library Manager Import + quarter-sharp smoke test</name>
  <files>.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md</files>
  <read_first>
    - CLAUDE.md (Plugin Cache Clearing protocol — mandatory before AU smoke; rebuild + fresh-install procedure)
    - .planning/phases/25-package-docs/25-CONTEXT.md (D-08 cross-platform validation gate; canary on O-Lyrica per Phase 23 precedent)
    - .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md (existing verification log to append into)
    - modules/tuning/note-expression/install-microtonal-suite.cmake.in (single-write target; verify the file ends up at the documented Ouaricon shared path)
  </read_first>
  <action>Execute the human-verified O-Lyrica canary install + Library Manager Import + quarter-sharp smoke procedure described in <how-to-verify>. The task pauses for the user to run the 6 steps and report verdict via the resume-signal. No autonomous code action is performed by the executor for this task — it records results in the verification log file.</action>
  <what-built>
    Tasks 1-4 produced: a Dorico-valid `.doricolib`, surgically clean module/CMake state with no Path A residue, and a single-write install script. This checkpoint exercises the end-to-end pipeline by building O-Lyrica, running `cmake --install` to land the asset at the Ouaricon shared path, then proves the full user flow in Dorico.
  </what-built>
  <how-to-verify>
    Run on the dev machine (macOS Dorico 6 / O-Lyrica-dev). Append all results to `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` under a new H2 section `## Plan 25-01 v3 Canary — O-Lyrica Path B End-to-End`.

    **Step 1 — Configure + build O-Lyrica:**
    ```bash
    cd /Users/taylorbrook/Dev/VST-development/build
    ninja OLyrica_VST3 OLyrica_AU
    ```
    Capture: ninja exit code (must be 0). If build fails on a CMake configure error referencing the deleted `ouaricon_extract_vst3_cids` or any deleted Path A file path, that indicates an in-task regression — escalate to `25-01-canary-FAIL-fix-PLAN.md` per D-11.

    **Step 2 — Cache clear + fresh install per CLAUDE.md:**
    ```bash
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
    rm -rf ~/Library/Audio/Plug-Ins/VST3/OLyrica.vst3 ~/Library/Audio/Plug-Ins/VST3/OLyrica-dev.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/OLyrica.component ~/Library/Audio/Plug-Ins/Components/OLyrica-dev.component
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/OLyrica*.vst3 ~/Library/Audio/Plug-Ins/VST3/
    cp -R build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/OLyrica*.component ~/Library/Audio/Plug-Ins/Components/
    ```

    **Step 3 — Run module install component to land the .doricolib:**
    ```bash
    rm -rf ~/Library/Application\ Support/Ouaricon/Microtonal\ Suite/  # clean slate
    cd /Users/taylorbrook/Dev/VST-development/build
    cmake --install . --component ouaricon_note_expression_OLyrica
    ```
    Expected console output: `[Ouaricon] Microtonal Suite installed: /Users/<user>/Library/Application Support/Ouaricon/Microtonal Suite`

    Verify the install landed:
    ```bash
    ls -la ~/Library/Application\ Support/Ouaricon/Microtonal\ Suite/
    # Must show: Ouaricon-VST3-NoteExpression.doricolib (6431 bytes), README-microtonal-suite.txt
    ```

    **Step 4 — Library Manager Import in Dorico 6:**
    1. Launch Dorico 6, open any project containing O-Lyrica-dev (or add it via Play → Endpoints → Add Plug-in).
    2. `Library → Library Manager → Import…`
    3. Navigate to `~/Library/Application Support/Ouaricon/Microtonal Suite/` and select `Ouaricon-VST3-NoteExpression.doricolib`.
    4. Click Import. Expected: import succeeds with no "invalid file format" error.
    5. Verify: `Library → Expression Maps` shows "Ouaricon VST3 Note Expression" in the list.
    6. Verify: `Play → Endpoints` → click the O-Lyrica-dev channel → Expression Map dropdown contains "Ouaricon VST3 Note Expression". Select it.

    **Step 5 — Quarter-sharp smoke (Phase 24 3-point gate):**
    1. Place a C4 with a quarter-sharp accidental in the score (or use an existing test project).
    2. Press play. Confirm pitch sounds at ~269 Hz (between C4 = 261.63 Hz and C♯ = 277.18 Hz). Use the spike-findings reference from `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` for verification methodology.
    3. Confirm: no attack zipper at note onset.
    4. Optional 3-point: place a polyphonic chord (C4 quarter-sharp + E4) — only C4 should be detuned; E4 plays at 12-TET 329.63 Hz.

    **Step 6 — Append result to log:**
    Record in `25-01-WAVE-0-VERIFICATION.md` under `## Plan 25-01 v3 Canary — O-Lyrica Path B End-to-End`:
    - Date, Dorico version, OS version, plugin build flavor (O-Lyrica-dev or O-Lyrica)
    - Step 1 (build): exit code, any warnings
    - Step 2 (install): VST3 + AU bundle paths
    - Step 3 (cmake --install): console line containing "Microtonal Suite installed" and the absolute destination path; ls output
    - Step 4 (Library Manager Import): PASS or FAIL with error text if any
    - Step 5 (quarter-sharp): measured frequency, attack-zipper observation, polyphonic-isolation observation
    - Verdict: PASS or FAIL
  </how-to-verify>
  <acceptance_criteria>
    - `ninja OLyrica_VST3 OLyrica_AU` exits 0
    - `cmake --install . --component ouaricon_note_expression_OLyrica` exits 0 and prints a STATUS line containing `Microtonal Suite installed`
    - `~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib` exists with size 6431 bytes (`stat -f%z` matches)
    - `~/Library/Application Support/Ouaricon/Microtonal Suite/README-microtonal-suite.txt` exists
    - Dorico 6 Library Manager Import of the canonical `.doricolib` reports SUCCESS (NOT "Error opening file: invalid file format")
    - Dorico's `Play → Endpoints → Expression Map` dropdown contains "Ouaricon VST3 Note Expression" after import
    - Quarter-sharp C4 plays at ~269 Hz (within ±2 Hz tolerance) when O-Lyrica-dev's channel has the expression map assigned
    - No attack zipper at note onset (subjective; record observation)
    - `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` contains a new H2 section `## Plan 25-01 v3 Canary — O-Lyrica Path B End-to-End` with all 6 step results recorded and a final `Verdict: PASS` line
  </acceptance_criteria>
  <resume-signal>Type "canary-pass" to advance Plan 25-02; "canary-fail" to escalate to a 25-01-canary-FAIL-fix-PLAN.md per D-11; or describe specific issues for in-plan triage.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Build → installed `.doricolib` | Reauthored XML file shipped to user systems via `cmake --install`. Source-of-truth is in-repo at `modules/tuning/note-expression/resources/library/`; reproducibility requires the canonical bytes be committed (not relying on `/tmp/`). |
| installed `.doricolib` → Dorico parser | Dorico Library Manager imports the file at user request. Malformed XML → graceful "invalid file format" error (D-04 finding); known-good 48-container skeleton mitigates. |
| install-script paths | `cmake --install` writes to `$ENV{HOME}` or `$ENV{APPDATA}` joined to fixed strings. No user-controlled input. |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-25-01-01 | Tampering | Reference asset at `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` could be swapped between authoring and copy-into-repo | mitigate | Task 1 commits the canonical bytes into `modules/tuning/note-expression/resources/library/` from the verified reference. After commit, the reproducibility chain is repo-rooted. The `/tmp/` reference is a one-time bootstrap source; verify byte-equality with `diff -q` against the verified-PASS reference at copy time. |
| T-25-01-02 | Tampering | Recovered `<ExpressionMapDefinition>` body content (entityID, microtonalPlaybackMethod) could be modified during reauthoring | mitigate | Task 1 acceptance criteria pin the load-bearing strings: exactly 1 occurrence of `xmap.ouaricon.vst3_note_expression`, exactly 1 occurrence of `kVST3NoteExpression`. Any deviation fails verification. |
| T-25-01-03 | Tampering | `.doricolib` integrity in transit (between build host and end-user) | accept | Distribution is via PKG (signed by macOS distribution-installer cert) and Inno Setup EXE on Windows. Plan 25-02 covers the package-level signature gate; plan 25-01 ships only repo-side authoring. |
| T-25-01-04 | Information Disclosure | Recovered XML body could leak unintended internal data | accept | The XML element is purely declarative (expression-map name, ID, microtonal method, technique combinations). No PII, secrets, or build-host data. Reviewed via existing in-repo content. |
| T-25-01-05 | Denial of Service | Surgical deletion of `ouaricon_extract_vst3_cids` could break a still-existing consumer | mitigate | Task 2 acceptance criteria verify zero remaining references to the helper across the codebase via grep. Plan 25-01 v2's only consumer was the Path A `module.cmake` block, which Task 2 deletes in the same atomic operation. |
| T-25-01-06 | Elevation of Privilege | `install(SCRIPT)` executes during `cmake --install` with user privileges; could write outside the intended dir if SUITE_LIB or SHARED_DIR were attacker-controlled | accept | Both paths are derived from CMake-time configure values (`@CMAKE_CURRENT_LIST_DIR@` resolves to a repo path; `$ENV{HOME}` / `$ENV{APPDATA}` are user-context environment variables). No user-supplied input crosses the install boundary. ASVS L1 path-traversal mitigation: `file(MAKE_DIRECTORY)` + `file(COPY DESTINATION)` are CMake primitives that don't accept relative-traversal inputs. |
| T-25-01-07 | Repudiation | Surgical deletion of Path A artifacts loses the failed v2 implementation history | accept | All v2 commits remain reachable via git history (commit `819b2b4` and its component commits). D-10's amend-forward strategy explicitly preserves audit trail by adding new deletion commits rather than rewriting history. |

**Severity:** All HIGH severity threats (Tampering of canonical bytes, Tampering of load-bearing values, DoS from broken consumer) are mitigated. MEDIUM/LOW threats accepted with documented rationale.

</threat_model>

<verification>
- `xmllint --noout` on the canonical `.doricolib` exits 0
- `xmllint --xpath 'count(/kScoreLibrary/*)'` returns `48`
- No Path A strings (`dorico_pt`, `tar cf`, `Default Library Additions`, `DefaultLibraryAdditions`, `ouaricon_extract_vst3_cids`, `DORICO_PT_STAGE`, `ouaricon_microtonal_suite_pt`) remain in `modules/tuning/note-expression/module.cmake`, `modules/cmake/OuariconModules.cmake`, `modules/tuning/note-expression/install-microtonal-suite.cmake.in`, `modules/tuning/note-expression/README.md`, `modules/tuning/note-expression/resources/README-microtonal-suite.txt`
- Directory `modules/tuning/note-expression/resources/playback-template/` does not exist
- O-Lyrica canary: build PASS, cmake --install PASS, Library Manager Import PASS, quarter-sharp ~269 Hz PASS
- Wave 0 informational auto-discovery probe result is logged (PASS/FAIL/SKIPPED, all acceptable)
</verification>

<success_criteria>
1. Canonical `.doricolib` exists at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`, byte-identical to the verified `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` reference, contains the load-bearing entityID and microtonalPlaybackMethod values, and is XML-well-formed with 48 top-level `<kScoreLibrary>` children.
2. All Path A artifacts deleted: `playback-template/` subtree, `ouaricon_extract_vst3_cids` helper, `.dorico_pt` packing in `module.cmake`, dual-write logic in `install-microtonal-suite.cmake.in`. Verified by grep returning zero matches for each banned string.
3. `module.cmake` lines 1-41 byte-identical to pre-edit (JUCE-NE-PATCH preserved); collapsed Microtonal Suite block is configure_file + install(SCRIPT) only.
4. Module README and user-facing fallback README describe Path B's Library Manager Import flow; no Path A residue.
5. O-Lyrica canary install end-to-end PASS: build, install, Library Manager Import, quarter-sharp ~269 Hz, all logged in `25-01-WAVE-0-VERIFICATION.md` with `Verdict: PASS`.
6. Wave 0 informational auto-discovery probe result is appended to the verification log (any verdict acceptable; result feeds v1.6 deferred-ideas per D-08 carry-forward note).
</success_criteria>

<output>
After completion, create `.planning/phases/25-package-docs/25-01-SUMMARY.md` (overwrite the existing v2 SUMMARY with a v3 SUMMARY documenting:
- Surgical deletions (3 groups) and what was preserved from `819b2b4`
- Reauthored canonical `.doricolib` provenance (factory skeleton + recovered ExpressionMapDefinition; byte-identical to verified reference)
- Single-write install collapse rationale (D-07; v1.6 revival reference)
- Canary install result (PASS/FAIL with full step trace)
- Wave 0 auto-discovery probe result (PASS/FAIL; informational)
- Per-task commits list
)
</output>
