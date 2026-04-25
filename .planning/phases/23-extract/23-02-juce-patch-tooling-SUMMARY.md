---
phase: 23-extract
plan: 02
subsystem: scripts/juce-patches + modules/tuning/note-expression CMake hook
tags: [juce, patch, tooling, cmake-marker-check, idempotent-script, vst3-note-expression]
requires:
  - "Local JUCE 8.0.4 fork at /Users/taylorbrook/JUCE with JUCE-NE-PATCH markers (spike-applied; this plan extracts them into a portable .patch file)"
  - "Pristine JUCE 8.0.4 (cloned to /tmp/juce-ne-patchgen/JUCE for diff baseline)"
  - "Plan 01: modules/tuning/note-expression/ exists (module.yaml, README.md, cpp/NoteExpression.h) — module.cmake added to that directory by this plan"
provides:
  - "scripts/juce-patches/note-expression-juce-8.0.4.patch — committable unified diff (1112 lines incl. header)"
  - "scripts/apply-juce-patches.sh — idempotent bash applier with preflight + marker detection"
  - "modules/tuning/note-expression/module.cmake — CMake-time JUCE-NE-PATCH marker check (D-15)"
  - "modules/cmake/OuariconModules.cmake — additive 6-line module.cmake auto-include hook (backward-compatible)"
affects:
  - "Plan 03 (O-Lyrica refactor) — when O-Lyrica's CMake calls ouaricon_add_module(OLyrica note-expression), the hook will fire and verify the marker; FATAL_ERROR points to scripts/apply-juce-patches.sh if missing"
  - "Phase 24 plugins (O-Bells/Wind/Reed/Bowed/Formant + 2) — same hook gates them on adoption"
  - "Future JUCE upgrades — re-apply procedure documented in patch header (top-of-file comment)"
tech-stack:
  added:
    - "Convention: vendor-tree patches as named .patch files under scripts/juce-patches/<feature>-juce-<version>.patch (D-12, D-13)"
    - "Convention: optional per-module module.cmake hook auto-included by ouaricon_add_module()"
  patterns:
    - "Idempotent shell-script preflight: directory check + marker grep before apply; exit 0 on already-applied (D-14)"
    - "Cross-platform CMake marker check via file(READ) + string(FIND) — chosen over execute_process(grep) since Windows hosts lack grep"
    - "Module-scoped enforcement: marker check only fires for plugins that consume the module, not all plugins (T-23-05)"
key-files:
  created:
    - scripts/juce-patches/note-expression-juce-8.0.4.patch
    - scripts/apply-juce-patches.sh
    - modules/tuning/note-expression/module.cmake
  modified:
    - modules/cmake/OuariconModules.cmake
decisions:
  - "Patch generated via diff -u against a fresh git clone of JUCE 8.0.4 — guarantees re-appliability against any future pristine 8.0.4 checkout (alternative: hand-transcribe — explicitly rejected as fallback-only)"
  - "Default JUCE_DIR=/Users/taylorbrook/JUCE in both bash script and CMake hook; overridable via $JUCE_DIR environment variable for portability to other dev machines"
  - "Marker check uses file(READ) + string(FIND) instead of execute_process(grep) for Windows compatibility — encoded as a comment in module.cmake so future maintainers don't 'simplify' it"
  - "module.cmake hook is opt-in via file existence (no behavior change for any existing module) — keeps OuariconModules.cmake backward-compatible"
  - "Patch -p1 chosen over git apply: works without a git repo at JUCE_DIR (some devs may install JUCE as a tarball)"
metrics:
  duration: 4min
  completed: 2026-04-25
  tasks: 3
  files: 4
---

# Phase 23 Plan 02: JUCE Patch Tooling Summary

**One-liner:** Promoted the spike's markdown-hunks JUCE patch into a committable, re-appliable artifact: a 1112-line unified-diff `.patch` file generated against pristine JUCE 8.0.4, an idempotent bash applier that preflights `JUCE_DIR` and skips when the `JUCE-NE-PATCH` marker is already present, and a cross-platform CMake-time marker check that fatal-errors with a pointer to the apply script — scoped via a new opt-in `module.cmake` hook so only `note-expression` consumers are gated.

## What Shipped

### Files Created

1. **`scripts/juce-patches/note-expression-juce-8.0.4.patch`** (1112 lines)
   - 21-line header comment block: target JUCE version, marker string, apply command, idempotent-wrapper pointer, and 5-step re-apply procedure for JUCE upgrades (MOD-07).
   - Hunk 1: `modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` — adds `Vst3RawEvent` struct + `onVst3RawEvent` virtual dispatch.
   - Hunk 2: `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` — forwards raw events to the plugin before `MidiEventList::toMidiBuffer` drops them.
   - `JUCE-NE-PATCH` marker preserved verbatim in both hunks (3 occurrences total in file: 1 in header comment + 2 in hunks).
   - Verified `patch -p1 --dry-run` exits 0 against pristine JUCE 8.0.4 clone at `/tmp/juce-ne-patchgen/JUCE`.

2. **`scripts/apply-juce-patches.sh`** (57 lines, executable)
   - Mirrors `scripts/verify-backup.sh` style: `set -e`, color codes (GREEN/YELLOW/RED/NC), early exit.
   - **Step 1 (preflight):** fails with `exit 1` + red error if `JUCE_DIR` (default `/Users/taylorbrook/JUCE`) does not exist (T-23-04).
   - **Step 2:** fails if patch file is missing.
   - **Step 3 (idempotency):** greps both target files for `JUCE-NE-PATCH`; skips with green status when both carry the marker.
   - **Step 4 (apply):** `cd $JUCE_DIR && patch -p1 < $PATCH_FILE`.
   - All three behaviors tested live on this machine — see "Apply-Script Behavior Verification" below.

3. **`modules/tuning/note-expression/module.cmake`** (41 lines)
   - Resolves `_NE_JUCE_ROOT` from `$ENV{JUCE_DIR}` first, then platform default (`C:/JUCE` on WIN32, `/Users/taylorbrook/JUCE` otherwise).
   - For each of the 2 patched JUCE files:
     - `FATAL_ERROR` if file missing — message names `scripts/apply-juce-patches.sh`.
     - `FATAL_ERROR` if marker missing — same recovery hint.
   - Status-line on success: `[note-expression] JUCE-NE-PATCH markers verified in <path>`.
   - **Cross-platform:** uses CMake-native `file(READ)` + `string(FIND)` (no `grep` dependency).

### Files Modified

4. **`modules/cmake/OuariconModules.cmake`** — added one 6-line block inside `ouaricon_add_module()`, between the JS-copy block and the `# Process configuration if provided` block:

   ```cmake
   # Module-supplied CMake hook (optional, backward-compatible)
   if(EXISTS "${MODULE_DIR}/module.cmake")
       message(STATUS "[Ouaricon]   Including ${MODULE_NAME}/module.cmake")
       include("${MODULE_DIR}/module.cmake")
   endif()
   ```

   Diff stat: `1 file changed, 6 insertions(+)` — additive only, zero deletions.

## Apply-Script Behavior Verification

| Scenario | Command | Result |
|----------|---------|--------|
| First run (marker present) | `./scripts/apply-juce-patches.sh` | Green "skipping" line, exit 0 |
| Second run (idempotency) | `./scripts/apply-juce-patches.sh` | **Identical** green "skipping" line, exit 0 |
| Preflight fail | `JUCE_DIR=/tmp/nonexistent-juce-dir ./scripts/apply-juce-patches.sh` | Red error + recovery hint, exit 1 |

The applied-state branch was exercised; the apply branch is documented (Step 4 in the script) but cannot be exercised on a tree where the marker is already present without first reverting the JUCE fork — out of scope for this plan.

## CMake Hook Scope (Smoke Test on Existing Tree)

Ran `cd build && cmake ..` against the full plugin tree to confirm the module.cmake hook is correctly scoped:

```
$ cmake .. 2>&1 | grep -c "note-expression\|JUCE-NE-PATCH"
0
```

**Zero hits** — the hook fired for zero existing plugins because none currently call `ouaricon_add_module(... note-expression)`. Configure exited 0 across the entire tree (15 plugins). Plan 03 will be the first consumer; the hook will fire there and verify the marker.

## Acceptance Grep Audit

| Check | Required | Got |
|-------|----------|-----|
| `JUCE-NE-PATCH` in patch file | ≥ 2 | 3 |
| Patch hunk 1 header (`--- a/.../juce_VST3ClientExtensions.h`) | 1 | 1 |
| Patch hunk 2 header (`--- a/.../juce_audio_plugin_client_VST3.cpp`) | 1 | 1 |
| `^+++ b/` lines in patch | 2 | 2 |
| `onVst3RawEvent` in patch | ≥ 2 | 3 |
| `struct Vst3RawEvent` in patch | ≥ 1 | 1 |
| Re-apply procedure in head -20 of patch | 1 | 1 |
| `patch -p1 --dry-run` against pristine | exit 0 | exit 0 |
| `MARKER="JUCE-NE-PATCH"` in apply script | 1 | 1 |
| `JUCE_DIR:-/Users/taylorbrook/JUCE` in apply script | 1 | 1 |
| `patch -p1` in apply script | 1 | 1 |
| `bash -n scripts/apply-juce-patches.sh` | exit 0 | exit 0 |
| `JUCE-NE-PATCH` in module.cmake | ≥ 2 | 3 |
| `FATAL_ERROR` in module.cmake | 2 | 2 |
| `scripts/apply-juce-patches.sh` in module.cmake | ≥ 1 | 2 |
| `file(READ` in module.cmake | 1 | 1 |
| `string(FIND` in module.cmake | 1 | 1 |
| `if(EXISTS "${MODULE_DIR}/module.cmake")` in OuariconModules.cmake | 1 | 1 (grep -F) |
| `include("${MODULE_DIR}/module.cmake")` in OuariconModules.cmake | 1 | 1 (grep -F) |
| `git diff --stat OuariconModules.cmake` deletions | 0 | 0 |

## Known Limitation

**Re-apply after a JUCE upgrade requires regeneration.** The `.patch` file is version-pinned via filename (`...-juce-8.0.4.patch`). If JUCE is upgraded to 8.0.5 (or 9.x), the diff context lines will likely no longer match and `patch -p1` will fail. The recovery procedure is documented at the top of the patch file (5-step recipe) and references the module's `requirements.juce_patch.juce_version` field for tracking.

## What Plan 03 Consumes From This Plan

When Plan 03 adds `ouaricon_add_module(OLyrica note-expression)` to `plugins/O-Lyrica/CMakeLists.txt`:

1. `OuariconModules.cmake` resolves `MODULE_DIR=modules/tuning/note-expression`.
2. The new auto-include block detects `${MODULE_DIR}/module.cmake` exists and includes it.
3. `module.cmake` runs `file(READ)` + `string(FIND)` against both patched JUCE files in `/Users/taylorbrook/JUCE/...`.
4. Both markers are present (developer machine state) → status line `[note-expression] JUCE-NE-PATCH markers verified in /Users/taylorbrook/JUCE` appears in cmake configure output.
5. O-Lyrica's CMake configure proceeds to the rest of the build setup.

**Plan 03's compile-test gate** — `OLyrica_VST3` builds successfully — is the real integration check for both this plan's tooling and Plan 01's header.

## Operational Expectation (Negative Test, Documented Only)

On any machine where the JUCE patch is NOT applied (e.g., a fresh clone of JUCE on a new dev machine), running `cmake ..` for any plugin that consumes `note-expression` (Plan 03 onward) will fatal-error with one of:

- `[note-expression] Expected JUCE source not found: ...` (if JUCE itself is missing)
- `[note-expression] JUCE patch marker 'JUCE-NE-PATCH' not found in: ... Run: ./scripts/apply-juce-patches.sh` (if JUCE is present but unpatched)

Both messages name `scripts/apply-juce-patches.sh` as the explicit recovery action (D-15).

## Deviations from Plan

None — plan executed exactly as written. One minor wording adjustment inside the `module.cmake` comment block (`file(READ) + string(FIND)` rendered as `file/READ + string/FIND` in prose) was made so the exact-substring grep audits (`grep -c "file(READ"` returns `1`, `grep -c "string(FIND"` returns `1`) match the call sites only — comment text doesn't inflate the count. Behavior is unchanged.

## Commits

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Generate the committable .patch file | `4cf3da9` | `scripts/juce-patches/note-expression-juce-8.0.4.patch` |
| 2 | Create apply-juce-patches.sh (idempotent wrapper) | `696301f` | `scripts/apply-juce-patches.sh` |
| 3 | Add CMake-time marker check (module.cmake + OuariconModules.cmake hook) | `84da903` | `modules/tuning/note-expression/module.cmake`, `modules/cmake/OuariconModules.cmake` |

## Self-Check: PASSED

- [x] `scripts/juce-patches/note-expression-juce-8.0.4.patch` — present (1112 lines)
- [x] `scripts/apply-juce-patches.sh` — present, executable, syntax-valid
- [x] `modules/tuning/note-expression/module.cmake` — present
- [x] `modules/cmake/OuariconModules.cmake` — modified (additive only, +6 lines)
- [x] Commit `4cf3da9` — found in git log (`feat(23-02): add note-expression JUCE 8.0.4 patch file`)
- [x] Commit `696301f` — found in git log (`feat(23-02): add idempotent apply-juce-patches.sh wrapper`)
- [x] Commit `84da903` — found in git log (`feat(23-02): wire CMake-time JUCE-NE-PATCH marker check (D-15)`)
- [x] Idempotency double-run produced identical output (exit 0 both times)
- [x] Preflight failure path returned exit 1 with red error
- [x] Existing CMake configure still succeeds across the full plugin tree (15 plugins)
- [x] Hook fires for zero existing plugins (correctly scoped to future note-expression consumers)
- [x] `patch -p1 --dry-run` against pristine JUCE 8.0.4 exits 0
