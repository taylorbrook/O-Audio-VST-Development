---
phase: 25-package-docs
plan: 01
subsystem: tuning/note-expression module
tags: [dorico, doricolib, microtuning, vst3-note-expression, cmake, path-b, library-manager-import]

dependency_graph:
  requires:
    - phase: 23-extract
      provides: "note-expression module v1.0.0 at modules/tuning/note-expression/ + 8 cohort consumers wired"
    - phase: 24-propagate
      provides: "All 8 plugins consume ouaricon_add_module(<P> note-expression); 3-point Dorico gate validated"
    - finding: "25-FINDING-path-b-validation.md (2026-04-27) — proves single .doricolib + Library Manager Import works end-to-end"
    - reference: "/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib (6,431 B Dorico-valid reference; verified 2026-04-27)"
  provides:
    - "Canonical Path B asset at modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib (full 48-container kScoreLibrary skeleton + injected ExpressionMapDefinition)"
    - "Surgically clean module/CMake state — no Path A residue (no .dorico_pt packing, no extract_vst3_cids helper, no playback-template/ subtree, no dual-write)"
    - "Single-write install script targeting platform-specific Ouaricon shared path only (~/Library/Application Support/Ouaricon/Microtonal Suite/ on macOS; %APPDATA%\\Ouaricon\\Microtonal Suite\\ on Windows)"
    - "Module v1.1.0 README + user-facing fallback README rewritten for Path B Library Manager Import flow"
    - "End-to-end O-Lyrica canary PASS proving build → install → import → quarter-sharp playback pipeline"
  affects:
    - "Plan 25-02 (8-plugin installer-bundling sweep) — consumes ouaricon_note_expression_<TARGET> install component for PKG/EXE bundling"
    - "Plan 25-03 (internal docs) — references canonical asset path + Path B import procedure"

tech-stack:
  added: [cmake-configure_file-AT_ONLY, cmake-install-SCRIPT, doricolib-kScoreLibrary-48-container-skeleton, library-manager-import-flow]
  patterns:
    - "Pattern (revised): module-owned single canonical asset + per-consumer install component"
    - "Pattern: factory-skeleton bootstrap + injected definition body (kScoreLibrary container + ExpressionMapDefinition)"
    - "Pattern: single-write to platform-specific Ouaricon shared path (D-07; no Dorico auto-discovery dual-write)"
    - "Pattern: surgical-amend-forward over full revert (D-10; preserves audit trail in git history)"

key-files:
  created: []
  modified:
    - "modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib (reauthored from factory skeleton + recovered ExpressionMapDefinition; byte-identical to verified reference)"
    - "modules/tuning/note-expression/module.cmake (lines 1-41 JUCE-NE-PATCH check preserved verbatim; lines 43-132 Path A block surgically replaced with ~25-line Path B block)"
    - "modules/cmake/OuariconModules.cmake (ouaricon_extract_vst3_cids helper deleted; 4 functions → 3)"
    - "modules/tuning/note-expression/install-microtonal-suite.cmake.in (collapsed from 87-line dual-write to ~40-line single-write)"
    - "modules/tuning/note-expression/resources/README-microtonal-suite.txt (Path B import-flow rewrite; 6-section structure preserved)"
    - "modules/tuning/note-expression/README.md (Dorico End-User Setup section rewritten for Path B; technical-mechanics paragraph preserved verbatim)"
    - ".planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md (appended Wave 0 v3 probe + canary PASS sections)"
  deleted:
    - "modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in"
    - "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in"
    - "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in"
    - "(empty playback-template/ subtree directories removed)"

key-decisions:
  - "D-01 (locked): Ship explicit Library Manager Import flow for v1.5; auto-discovery deferred to v1.6 deferred-ideas"
  - "D-03 (executed): Canonical .doricolib reauthored from factory skeleton + injected ExpressionMapDefinition; byte-identical to verified Path B reference"
  - "D-07 (executed): Install script collapsed to single-write per platform — no Dorico auto-discovery dual-write"
  - "D-10 (executed): Surgical amend-forward over full revert — module v1.1.0 bump, registry entry, README skeleton, version-probe pattern (preserved unused for v1.6 revival), and recovered XML body all kept; only Path A-specific files and code regions deleted"
  - "Wave 0 v3 auto-discovery probe: FAIL (informational only; non-blocking; logged for v1.6 deferred-ideas per D-08 carry-forward)"

patterns-established:
  - "Pattern: module-owned canonical asset under modules/<category>/<module>/resources/<asset-class>/ + per-consumer install component named ouaricon_<module>_<TARGET> + configure_file → install(SCRIPT) wiring"
  - "Pattern: dorico .doricolib authoring via HALion Sonic factory skeleton + injected definition body (preserves Dorico's required 48-container kScoreLibrary structure)"
  - "Pattern: amend-forward strategy for failed v2 work — preserve recoverable artifacts (registry entries, version bumps, README skeletons, recovered content) in atomic deletion commits with audit-trail-preserving messages"

requirements-completed: [INST-01, INST-02]

metrics:
  duration_minutes: ~25 (deterministic Tasks 1-4) + human canary verification (Task 5)
  tasks_completed: 5
  files_created: 0
  files_modified: 7
  files_deleted: 3 (+ 5 empty parent dirs)
  commits: 6
  completed_date: "2026-04-27"
---

# Phase 25 Plan 25-01 v3: Author + Install-Collapse Summary

**Single Dorico-valid `.doricolib` (full 48-container kScoreLibrary skeleton + injected ExpressionMapDefinition) authored as module's canonical asset; Path A artifacts surgically excised; install logic collapsed to single-write to Ouaricon shared path; O-Lyrica canary PASS end-to-end (build → cmake-install → Library Manager Import → quarter-sharp ~269 Hz).**

## Performance

- **Duration (deterministic Tasks 1-4):** ~25 minutes (sequential commits)
- **Started:** 2026-04-27 (continuing from prior agent's checkpoint at Task 5)
- **Completed:** 2026-04-27 (canary PASS verified by user on dev machine)
- **Tasks:** 5 (4 deterministic + 1 human-verified canary)
- **Files modified:** 7
- **Files deleted:** 3 (Path A subtree)

## Accomplishments

- **Canonical Dorico-valid `.doricolib`** at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` — byte-identical (`diff -q` clean) to the verified `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` reference (6,431 B; Library Manager Import + quarter-sharp PASS validated 2026-04-27).
- **All Path A artifacts excised** — `playback-template/` subtree (3 files + 5 empty parent dirs), `ouaricon_extract_vst3_cids` helper (~70 lines), `.dorico_pt` packing block in `module.cmake` (~90 lines), dual-write logic in `install-microtonal-suite.cmake.in` (collapsed 87 → ~40 lines).
- **Install script collapsed to single-write** — `file(COPY)` to `~/Library/Application Support/Ouaricon/Microtonal Suite/` (macOS) or `%APPDATA%\Ouaricon\Microtonal Suite\` (Windows) only. No Dorico auto-discovery directories touched.
- **Module READMEs rewritten** for the Path B Library Manager Import flow. Technical-mechanics paragraph (`kVST3NoteExpression` invariant) preserved verbatim. No Path A residue (`auto-discovery`, `dorico_pt`, `Default Library Additions`) anywhere.
- **O-Lyrica canary PASS** end-to-end on macOS 26.3.1 / Dorico 6 / O-Lyrica-dev — build clean, cmake-install lands assets at Ouaricon shared path, Dorico Library Manager imports without "invalid file format", quarter-sharp C4 plays at ~269 Hz target with no attack zipper, polyphonic isolation as expected.
- **Wave 0 v3 auto-discovery probe FAIL** logged as informational evidence for v1.6 deferred-ideas (per D-08 carry-forward). Non-blocking; v1.5 ships explicit-import as designed.

## Task Commits

Six atomic commits from `ad9e5e4` through `<this final tracking commit>`:

1. **Task 1: Reauthor canonical .doricolib (D-03)** — `ad9e5e4` (feat)
   `feat(25-01): reauthor canonical .doricolib with full kScoreLibrary skeleton (D-03)`
2. **Task 2: Surgical delete of Path A artifacts (D-10)** — `db20a04` (chore)
   `chore(25-01): surgical delete of Path A artifacts (D-10)`
   3 deletion groups in one atomic commit: playback-template/ subtree, `ouaricon_extract_vst3_cids` helper, `.dorico_pt` packing in module.cmake.
3. **Task 3: Collapse install script to single-write (D-07)** — `98479ba` (chore)
   `chore(25-01): collapse install script to single-write (D-07)`
4. **Task 4: Rewrite Path B import flow in module READMEs** — `93d29d6` (docs)
   `docs(25-01): rewrite Path B import flow in module READMEs`
5. **Task 5: Wave 0 probe (FAIL) + canary (PASS) verification log** — `c45703b` (docs)
   `docs(25-01): append Wave 0 probe (FAIL informational) + canary PASS results to verification log`
6. **SUMMARY + tracking advance** — `<final commit hash>` (docs)
   `docs(25-01): mark plan complete and advance tracking`

## What Was Preserved from `819b2b4` (D-10 Amend-Forward)

Per the D-10 surgical-amend strategy, NOT a full revert. Preserved from the v2 work:

| Artifact | Provenance | Why preserved |
|----------|------------|---------------|
| Module v1.1.0 bump in `module.yaml` + `registry.yaml` | v2 commit `02fdcc2` | Module advanced v1.0.0 → v1.1.0 with the v2 work; v3 keeps the bump (substantive ABI/feature changes warrant the minor) |
| Registry entry note-expression v1.1.0 + 8-entry `used_by:` list | v2 commit `02fdcc2` | Registry-system source of truth; not Path A-specific |
| README structural skeleton (6-section fallback README + module README "Dorico End-User Setup" subsection) | v2 commits `9290835`, `02fdcc2` | Structure is sound; only the Path A content under each heading was rewritten |
| Recovered `<ExpressionMapDefinition>` body (entityID, microtonalPlaybackMethod, creator, pitchBendRange) | v2 commit `06fb002` (originally from cd2c2c6) | Re-wrapped inside the new factory-skeleton-bootstrapped `<kScoreLibrary>` for v3; byte-equivalent expression-map content |
| Dorico-version probe pattern (`foreach(_v 6 5 4)`) | v2 commit `6caf768` | Pattern preserved as a v1.6 revival reference in `install-microtonal-suite.cmake.in` comment block: `git show 819b2b4:modules/tuning/note-expression/install-microtonal-suite.cmake.in` |

## What Was Surgically Deleted (Path A)

3 deletion groups, all in commit `db20a04`:

1. **`playback-template/` subtree** (3 files + 5 empty parent dirs)
   - `playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in`
   - `playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in`
   - `playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in`
2. **`ouaricon_extract_vst3_cids` helper** in `modules/cmake/OuariconModules.cmake` (~70 lines; function count 4 → 3)
3. **`.dorico_pt` packing block** in `modules/tuning/note-expression/module.cmake` (~90 lines lines 43-132 replaced with ~25-line Path B `configure_file` + `install(SCRIPT)` block; lines 1-41 JUCE-NE-PATCH marker check preserved byte-identical)

## Reauthored `.doricolib` Provenance

The canonical asset at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` was reauthored per D-03:

1. **Factory skeleton source:** `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml` — provides the authoritative 48-container `<kScoreLibrary>` structure that Dorico's parser requires for acceptance.
2. **Injected `<ExpressionMapDefinition>`:** Body recovered from cd2c2c6 — entityID `xmap.ouaricon.vst3_note_expression`, microtonalPlaybackMethod `kVST3NoteExpression` (Landmine 3 — never `kAuto`/`kPitchBend`), creator "Ouaricon Audio", `<pluginNames />` (CID-free per D-02), pitchBendRange 2, one playingTechniqueCombination `pt.natural`.
3. **47/48 containers emptied** to `<entities array="true" />`; `<expressionMapDefinitions>` retains the injected body. Special case: `<instrumentNames>` keeps `<language>kEnglish</language>` alongside `<entities array="true" />` (factory-skeleton invariant).
4. **Verification:** `diff -q` clean against `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib`; `xmllint --noout` exits 0; `xmllint --xpath 'count(/kScoreLibrary/*)'` returns 48.

## Single-Write Install Collapse Rationale (D-07)

`install-microtonal-suite.cmake.in` collapsed from 87-line dual-write (Ouaricon shared path + Dorico version-probed `Default Library Additions/`) to ~40-line single-write (Ouaricon shared path only). Rationale:

- **Path B distribution model** is "land asset at well-known location; user does one-time Library Manager Import" (D-01). Writing into Dorico's auto-discovery directories is unnecessary and leaks plugin-installer scope into Dorico's user-config space.
- **Idempotent overwrite:** `file(COPY DESTINATION)` overwrites in place, safe for the 8-plugin installer sweep where each plugin's installer writes the same content.
- **v1.6 revival reference:** comment block in the new file preserves a pointer to `git show 819b2b4:modules/tuning/note-expression/install-microtonal-suite.cmake.in` so the Dorico-version probe pattern is discoverable if auto-discovery work is taken up post-v1.5.

## Canary Install Result

**Verdict:** PASS (full step trace in `25-01-WAVE-0-VERIFICATION.md` `## Plan 25-01 v3 Canary` section)

| Step | Action | Result |
|------|--------|--------|
| 1 | `ninja OLyrica_VST3 OLyrica_AU` | exit 0 — no Path A regressions |
| 2 | Cache clear + fresh install per CLAUDE.md | VST3 + AU bundles relocated to system plugin folders |
| 3 | `cmake --install . --component ouaricon_note_expression_OLyrica` | `.doricolib` (6,431 B) + README landed at `~/Library/Application Support/Ouaricon/Microtonal Suite/` |
| 4 | Dorico 6 Library Manager Import | SUCCESS — no "invalid file format" error; "Ouaricon VST3 Note Expression" appears in `Library → Expression Maps` and `Play → Endpoints → Expression Map` dropdown |
| 5 | Quarter-sharp C4 smoke (3-point gate) | PASS — ~269 Hz target by ear; no attack zipper; polyphonic isolation as expected |

Granular Hz measurement was not separately captured at this checkpoint (already captured during Path B validation 2026-04-27, see `25-FINDING-path-b-validation.md`).

## Wave 0 v3 Auto-discovery Probe Result

**Verdict:** FAIL (informational only — non-blocking)

Probe: dropped reference asset into `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/`, restarted Dorico, opened a project with O-Lyrica-dev, checked `Play → Endpoints → Expression Map` dropdown.

**Result:** "Ouaricon VST3 Note Expression" did NOT appear without an explicit `Library → Library Manager → Import` action.

**Disposition:** Informational only — does not affect v3 ship behavior (D-01 ships explicit-import). Result logged for v1.6 deferred-ideas (per D-08 carry-forward + CONTEXT.md ## Deferred Ideas). Probe residue cleaned up by continuation executor (Dorico User expression-map dir empty).

## Decisions Made

- D-01 ratified: ship explicit Library Manager Import for v1.5
- D-03 executed: canonical .doricolib reauthored from factory skeleton + injected definition; byte-identical to verified reference
- D-07 executed: install collapsed to single-write to Ouaricon shared path
- D-10 executed: amend-forward over full revert
- Wave 0 v3 probe FAIL recorded as v1.6 deferred-ideas evidence

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Probe-residue cleanup mismatch**
- **Found during:** Continuation-executor pre-flight after Task 5 PASS
- **Issue:** Plan §`<how-to-verify>` Pre-Canary Step 6 specified cleanup of `Ouaricon-VST3-NoteExpression-v2.doricolib` from `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/`, but the user copied the reference under the canonical name `Ouaricon-VST3-NoteExpression.doricolib` (without the `-v2` suffix). The exact cleanup command in the plan was a no-op for the actual filename present.
- **Fix:** Continuation executor removed `Ouaricon-VST3-NoteExpression.doricolib` from the Dorico User dir; verified dir is now empty.
- **Files modified:** None in repo (filesystem-side cleanup outside the working tree).
- **Verification:** `ls -la "$HOME/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/"` returns the empty dir.
- **Committed in:** `c45703b` (verification-log appendage records the corrected cleanup path).

### Literal-grep vs intent discrepancy (plan-checker advisory; non-blocking)

The plan's automated acceptance check `grep -q '<pluginNames/>' modules/.../Ouaricon-VST3-NoteExpression.doricolib` (Task 1 acceptance criterion: "File contains `<pluginNames/>` (self-closing, empty — D-02 CID-free)") does NOT match the actual file content, which contains `<pluginNames />` (with a space, the form Dorico's HALion factory skeleton emits). The acceptance criterion's intent — "self-closing, empty, CID-free" — is satisfied by both the verified reference asset and the file copied from it. Both literally contain the space; the form is faithful to the validated source. Treated as compliant.

---

**Total deviations:** 1 auto-fixed (Rule 3 - blocking cleanup) + 1 literal-grep vs intent advisory (non-blocking, no action).
**Impact on plan:** No scope creep. The cleanup mismatch was housekeeping; the literal-grep advisory is a Task 1 acceptance-criterion phrasing choice that does not impact the load-bearing invariant.

## Issues Encountered

None during the deterministic Tasks 1-4 (each executed and committed atomically by the prior agent without inline fixes). Task 5 (canary) PASSed on user's first attempt on the dev machine.

## Self-Check: PASSED

**Files modified (verified in HEAD):**
- `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` — FOUND (6,431 B; `diff -q` against `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` clean)
- `modules/tuning/note-expression/module.cmake` — FOUND (Path B block; lines 1-41 byte-identical to pre-edit)
- `modules/cmake/OuariconModules.cmake` — FOUND (3 functions; `ouaricon_extract_vst3_cids` removed)
- `modules/tuning/note-expression/install-microtonal-suite.cmake.in` — FOUND (single-write; ~40 lines)
- `modules/tuning/note-expression/resources/README-microtonal-suite.txt` — FOUND (Path B rewrite)
- `modules/tuning/note-expression/README.md` — FOUND (Path B rewrite; mechanics paragraph preserved)
- `.planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md` — FOUND (Wave 0 probe + canary sections appended)

**Files deleted (verified absent):**
- `modules/tuning/note-expression/resources/playback-template/` subtree — ABSENT
- `ouaricon_extract_vst3_cids` function in OuariconModules.cmake — ABSENT
- `.dorico_pt` packing references in module.cmake — ABSENT

**Commits (verified in git log):**
- `ad9e5e4` (Task 1) — FOUND
- `db20a04` (Task 2) — FOUND
- `98479ba` (Task 3) — FOUND
- `93d29d6` (Task 4) — FOUND
- `c45703b` (Task 5 verification log) — FOUND
- Final tracking commit — pending (this SUMMARY commit + tracking-advance commit immediately follow)

## Threat Flags

No new threat surface introduced. All threats in plan §`<threat_model>` mitigated as designed:
- T-25-01-01 (canonical-bytes tampering): mitigated — `diff -q` against verified reference clean at commit time
- T-25-01-02 (load-bearing string tampering): mitigated — exactly 1 occurrence each of `xmap.ouaricon.vst3_note_expression` and `kVST3NoteExpression`
- T-25-01-05 (broken consumer from helper deletion): mitigated — Task 2 acceptance criteria verified zero remaining `ouaricon_extract_vst3_cids` references; only consumer was the Path A `module.cmake` block deleted in the same commit
- T-25-01-06 (privilege escalation via install script): mitigated — paths derived from `@CMAKE_CURRENT_LIST_DIR@` + `$ENV{HOME}` / `$ENV{APPDATA}`; no user-supplied input; CMake `file()` primitives reject relative-traversal

## Next Phase Readiness

**Plan 25-02 unblocked.** The canary PASS proves the install component `ouaricon_note_expression_<TARGET>` is wired correctly. Plan 25-02's 8-plugin installer-bundling sweep can consume this component for PKG (macOS) and Inno Setup EXE (Windows) bundling.

Implementation notes for Plan 25-02:
- Install component name: `ouaricon_note_expression_<TARGET_NAME>` (e.g. `ouaricon_note_expression_OLyrica`, `ouaricon_note_expression_O-Bells`)
- Canonical asset path: `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` (single source of truth)
- Companion fallback README: `modules/tuning/note-expression/resources/README-microtonal-suite.txt`
- Cross-platform validation gate (D-08): one representative install on each platform proves landing path + 3-point gate

---
*Phase: 25-package-docs*
*Plan: 25-01-author-and-install-collapse*
*Completed: 2026-04-27*
*v3 — Path B locked, canary PASS*
