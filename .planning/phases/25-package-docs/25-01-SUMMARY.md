---
phase: 25-package-docs
plan: 01
subsystem: tuning/note-expression module
tags: [dorico, playback-template, doricolib, microtuning, vst3-note-expression, cmake, configure_file, install-script]
dependency_graph:
  requires:
    - "Phase 23 (note-expression module v1.0.0 extracted at modules/tuning/note-expression/)"
    - "Phase 24 (8 cohort plugins consume ouaricon_add_module(<P> note-expression))"
    - "Plan 25-01 v1 reverted at d2c86c5 — XML body recoverable from cd2c2c6"
  provides:
    - "Two distributable Dorico assets (.dorico_pt + .doricolib) authored at modules/tuning/note-expression/resources/"
    - "ouaricon_extract_vst3_cids CMake helper (reads moduleinfo.json, sets <NAME>_PLUGINID vars)"
    - "module.cmake-driven .dorico_pt packing + dual install(SCRIPT) plumbing"
    - "Per-platform install-microtonal-suite.cmake.in template (macOS/Windows; Dorico 6/5/4 probe)"
    - "Module v1.1.0 in module.yaml + registry.yaml; README documents auto-discovery flow"
  affects:
    - "Plan 25-02 (atomic 8-plugin installer sweep) — installer configs consume the module's two staged resources"
    - "Plan 25-03 (internal docs) — referenced canonical asset paths"
tech_stack:
  added: [cmake-configure_file-AT_ONLY, cmake-tar-zip-pack, cmake-install-SCRIPT, python3-json-trailing-comma-strip]
  patterns: [pattern-A-module-owned-asset, pattern-B-cmake-helper-fn, pattern-C-xml-token-substitution, pattern-D-recovered-XML, pattern-E-recovered-README, pattern-F-per-platform-dual-write, pattern-J-semver-minor-bump]
key_files:
  created:
    - "modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib"
    - "modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in"
    - "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in"
    - "modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in"
    - "modules/tuning/note-expression/resources/README-microtonal-suite.txt"
    - "modules/tuning/note-expression/install-microtonal-suite.cmake.in"
    - ".planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md"
  modified:
    - "modules/cmake/OuariconModules.cmake"
    - "modules/tuning/note-expression/module.cmake"
    - "modules/tuning/note-expression/module.yaml"
    - "modules/tuning/note-expression/README.md"
    - "modules/registry.yaml"
decisions:
  - "Per-consumer install component naming follows ouaricon_note_expression_<TARGET> convention (consistent with Phase 23 module-owned-asset shape)"
  - "Single-target guard via `if(NOT TARGET ouaricon_microtonal_suite_pt)` avoids duplicate target definition when multiple consumers fire module.cmake"
  - "Plan 25-01 v2 deterministic file-authoring (Tasks 1-7) executed in parallel worktree; checkpoint tasks (0a A2, 0b A4, 8 canary) deferred to user-on-dev-machine post-merge — they require physical Dorico 6 interaction and full 8-cohort VST3 builds"
metrics:
  duration_minutes: ~15
  tasks_completed: 7
  tasks_deferred: 3
  files_created: 7
  files_modified: 5
  commits: 8
  completed_date: "2026-04-26"
---

# Phase 25 Plan 25-01 v2: Microtonal Suite Author + Plumbing Summary

VST3 Note Expression Dorico Playback Template + companion .doricolib library bundle authored, with single-source-of-truth ownership in the note-expression module's `resources/` tree and CMake plumbing that auto-installs the dual-resource pair to Dorico's auto-scan directories whenever any consumer of `ouaricon_add_module(<P> note-expression)` runs `cmake --install`.

## What Was Built

**Two distributable Dorico assets** (single source at `modules/tuning/note-expression/resources/`):
1. `Ouaricon-Microtonal-Suite.dorico_pt` — packed at build time as a zip from the `playback-template/` source tree. Contains `playbacktemplatespec.xml`, `endpointconfig.xml` (8 plugin slots, one per cohort), and embedded `playbacktemplatedeps.doricolib` (the canonical kScoreLibrary expression-map body).
2. `Ouaricon-VST3-NoteExpression.doricolib` — standalone expression-map library bundle, byte-exact copy of the same kScoreLibrary body. Lands in Dorico's `Default Library Additions/` for Library → Expression Maps reuse.

**CMake plumbing** in three coordinated touchpoints:
- `modules/cmake/OuariconModules.cmake` — appended `ouaricon_extract_vst3_cids()` helper. Reads each built `.vst3` bundle's `Contents/Resources/moduleinfo.json` and emits per-plugin `<NAME>_PLUGINID` variables in PARENT_SCOPE. Honors `${OUARICON_DEV_SUFFIX}` so dev installers ship dev CIDs and prod ship prod (Pitfall 2 mitigation, S-3).
- `modules/tuning/note-expression/module.cmake` — appended Microtonal Suite block (lines 1-41 preserved verbatim). Stages the templated XML files into `${CMAKE_BINARY_DIR}/_microtonal-suite/`, runs `configure_file @ONLY` × 3 to substitute the 8 `@<NAME>_PLUGINID@` tokens into endpointconfig.xml, packs the `.dorico_pt` via `cmake -E tar cf --format=zip` with stage as WORKING_DIRECTORY (no parent-dir wrapping, Pitfall 5 mitigation), and registers a per-consumer `install(SCRIPT ouaricon_note_expression_<TARGET>)` rule.
- `modules/tuning/note-expression/install-microtonal-suite.cmake.in` — per-platform dual-write template. macOS branch + Windows branch each: copy both files to Ouaricon shared canonical path, then probe Dorico 6/5/4 in descending order, extract `.dorico_pt` zip into the first found Dorico version dir, and copy `.doricolib` into the version's `Default Library Additions/` (macOS, with spaces) or `DefaultLibraryAdditions/` (Windows, NO spaces — intentional Pitfall 3 asymmetry).

**Module metadata bumped** 1.0.0 → 1.1.0 in both `module.yaml` and `registry.yaml`. New v1.1.0 changelog entry. README "Dorico End-User Setup" section rewritten as v1.1.0+ auto-discovery flow with manual-import fallback. The 8-entry `used_by:` list in registry.yaml preserved verbatim.

**User-facing fallback README** (`README-microtonal-suite.txt`) authored with 6-section structure recovered from the v1 `README-doricoexpmap.txt` (cd2c2c6) but content fully revised for the new dual-asset Playback Template flow. Captures the macOS/Windows directory-name asymmetry explicitly so users know where files land on their platform.

## Per-Task Commits (chronological)

| Task | Commit | Title | Files |
|------|--------|-------|-------|
| 1 | `06fb002` | feat(25-01): recover canonical doricolib XML body from cd2c2c6 | library/Ouaricon-VST3-NoteExpression.doricolib + playbacktemplatedeps.doricolib.in |
| 2 | `9f0c8ff` | feat(25-01): author Playback Template spec + 8-slot endpoint config | playbacktemplatespec.xml.in + endpointconfig.xml.in |
| 3 | `9290835` | docs(25-01): author Microtonal Suite fallback README (INST-04) | README-microtonal-suite.txt |
| 4 | `ef64ec4` | feat(25-01): add ouaricon_extract_vst3_cids helper for CID substitution | OuariconModules.cmake (appended) |
| 5 | `6caf768` | feat(25-01): author per-platform install script template | install-microtonal-suite.cmake.in |
| 6 | `33f0597` | feat(25-01): extend module.cmake with .dorico_pt packing + dual install | module.cmake (appended; lines 1-41 preserved) |
| 7 | `02fdcc2` | feat(25-01): bump note-expression module 1.0.0 -> 1.1.0 | module.yaml + registry.yaml + README.md |
| Wave 0 | `1815c9a` | docs(25-01): create Wave 0 + canary verification log | 25-01-WAVE-0-VERIFICATION.md |

## Pipeline Gates Passed

| Gate | Result |
|------|--------|
| `xmllint --noout` × 4 (library, deps, endpoint, spec) | PASS |
| Cross-file ID coupling (S-4) — `xmap.ouaricon.vst3_note_expression` | PASS — 10 occurrences total (1 library + 1 deps + 8 endpoint slots) |
| 8 plugin tokens present (`@OLYRICA_PLUGINID@` … `@OFORMANT_PLUGINID@`) | PASS — all 8 found |
| spec → endpoint linkage (`endpointconfig.user.ouaricon_microtonal_suite`) | PASS |
| Recovered XML byte-exact between library and embedded deps copy | PASS — `diff -q` clean |
| README contains all 6 sections + both dir-name variants + all 8 plugins | PASS |
| OuariconModules.cmake function count 3 → 4 | PASS |
| module.cmake lines 1-41 preserved verbatim | PASS — `head -41` byte-exact match against parent commit |
| module.yaml v1.1.0 + 2026-04-26 changelog entry | PASS |
| registry.yaml note-expression v1.1.0 + 8-entry used_by preserved | PASS |
| README.md "auto-discovery flow" + both dir-name variants + dorico_pt asset name | PASS |

## Deviations from Plan

### Auto-fixed Issues — None

The deterministic Tasks 1-7 executed exactly as planned with no inline fixes required. The plan's `<read_first>` blocks pointed to ready-to-use verbatim XML/CMake content from RESEARCH.md and PATTERNS.md, which was used directly.

### Minor verification-criterion clarification

Plan §Task 1 acceptance criterion states "Each file contains exactly 1 occurrence of `kVST3NoteExpression`". The recovered XML body (cd2c2c6, treated as byte-exact source per D-03) contains the load-bearing `<microtonalPlaybackMethod>kVST3NoteExpression</microtonalPlaybackMethod>` element AND a trailing comment that mentions the value name. `grep -c kVST3NoteExpression` returns 2, not 1. The XML element occurrence is correct (one per file); the comment is incidental and is part of the byte-exact recovered content. **Treating this as compliant** — the load-bearing invariant (microtonalPlaybackMethod set to kVST3NoteExpression, NOT kAuto/kPitchBend, Landmine 3) is satisfied. The recovered comment carries forward verbatim per the plan's mandate to "Recover, do not re-author" (D-03). No code change needed.

## Deferred Verifications (Checkpoint Tasks 0a, 0b, 8)

The plan's three `checkpoint:human-verify` tasks (Wave 0 A2, Wave 0 A4, Task 8 canary) require physical interaction with Dorico 6 on the user's dev machine and cannot be executed by a parallel worktree:

| Task | Why deferred | Where to resume |
|------|--------------|-----------------|
| **0a (A2)** — state-less .dorico_pt accepted by Dorico | Requires hand-authoring a stripped `.dorico_pt`, dragging into Dorico 6, applying template, quarter-sharp playback | `25-01-WAVE-0-VERIFICATION.md` `## A2 Result` section |
| **0b (A4)** — drag-drop extraction faithful | Requires drag-drop of `/tmp/ample_china/Ample China.dorico_pt` onto Dorico 6 + post-state inspection | `25-01-WAVE-0-VERIFICATION.md` `## A4 Result` section |
| **8 (Canary)** — O-Lyrica end-to-end pipeline proof | Requires building all 8 cohort `_VST3` targets, running `cmake --install . --component ouaricon_note_expression_OLyrica`, and confirming Dorico picker + quarter-sharp playback | `25-01-WAVE-0-VERIFICATION.md` `## Canary Install Result` section |

The deterministic work (Tasks 1-7) is independent of A2/A4 outcomes — even if A2 or A4 fails, the recovered XML, helper, install template, and module.cmake extensions are reusable. Only the install-time strategy (file extraction destination vs file landing) would change. If A2 fails, escalate to D-05 reconsideration (curated `.pluginstate` authoring becomes mandatory) per `25-01-A2-FAIL-fix-PLAN.md`. If A4 fails, escalate to D-11 reconsideration per `25-01-A4-FAIL-fix-PLAN.md`. If the canary fails, escalate to `25-01-canary-FAIL-fix-PLAN.md`.

## Implementation Notes for Plan 25-02

The CMake install component name is **`ouaricon_note_expression_<TARGET_NAME>`** (e.g. `ouaricon_note_expression_OLyrica`, `ouaricon_note_expression_O-Bells`). Plan 25-02's PKG/EXE installer scripts should consume this component name when invoking `cmake --install . --component <name>` to stage the suite resources for bundling. Each consumer's install rule fires once via the per-target `configure_file` output `install-microtonal-suite-<TARGET_NAME>.cmake`.

The `.dorico_pt` artifact lives at `${CMAKE_BINARY_DIR}/Ouaricon-Microtonal-Suite.dorico_pt`. The `.doricolib` lives at the canonical source path `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`. Plan 25-02's per-plugin `inno-template.iss` and `pkg-creation.md` postinstall script need both paths.

**Build ordering**: `ouaricon_extract_vst3_cids` FATAL_ERRORs if any of the 8 cohort plugins' `.vst3` bundle is missing. In Plan 25-02, each plugin's installer build pipeline must build all 8 `_VST3` targets before invoking `cmake --install` for the suite component.

## Threat Flags

No new threat surface introduced. All threats in the plan's `<threat_model>` are mitigated as designed:
- T-25-01-01 (cross-file ID coupling): mitigated — verified byte-exact `xmap.ouaricon.vst3_note_expression` in 3 source files (10 total occurrences).
- T-25-01-02 (dev/prod CID divergence): mitigated — `${OUARICON_DEV_SUFFIX}` honored in helper.
- T-25-01-05 (path traversal in tar xf): mitigated — destination is hard-coded probed Dorico version dir; zip's internal layout is repo-controlled.
- T-25-01-07 (recovered XML tampering): mitigated — byte-exact diff verified library vs deps; xmllint on all 4 XML files.

## Self-Check: PASSED

**Files created (verified exist):**
- modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib — FOUND
- modules/tuning/note-expression/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in — FOUND
- modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in — FOUND
- modules/tuning/note-expression/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in — FOUND
- modules/tuning/note-expression/resources/README-microtonal-suite.txt — FOUND
- modules/tuning/note-expression/install-microtonal-suite.cmake.in — FOUND
- .planning/phases/25-package-docs/25-01-WAVE-0-VERIFICATION.md — FOUND

**Commits (verified in git log):**
- 06fb002 — FOUND
- 9f0c8ff — FOUND
- 9290835 — FOUND
- ef64ec4 — FOUND
- 6caf768 — FOUND
- 33f0597 — FOUND
- 02fdcc2 — FOUND
- 1815c9a — FOUND

Phase 25 Plan 25-02 unblocked (subject to A2/A4/canary PASS on dev machine).
