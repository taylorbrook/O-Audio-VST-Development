---
phase: 23-extract
fixed_at: 2026-04-25T20:38:54Z
review_path: .planning/phases/23-extract/23-REVIEW.md
iteration: 1
findings_in_scope: 1
fixed: 1
skipped: 0
status: all_fixed
---

# Phase 23: Code Review Fix Report

**Fixed at:** 2026-04-25T20:38:54Z
**Source review:** .planning/phases/23-extract/23-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 1 (1 high; 1 low excluded by scope)
- Fixed: 1
- Skipped: 0

## Fixed Issues

### HR-01: Per-format CMake routing silently fails for Standalone, AAX, LV2, Unity, AUv3

**Files modified:** `modules/cmake/OuariconModules.cmake`
**Commit:** 1692197
**Applied fix:** Replaced `string(TOUPPER ${fmt} _FMT_UPPER)` with parallel
lookup-table lists (`_OUA_FMT_DIRS` / `_OUA_FMT_TARGETS`) that map lowercase
on-disk directory names to JUCE's exact mixed-case target suffixes
(`Standalone`, `AAX`, `LV2`, `AUv3`, `Unity` plus the all-uppercase `VST3`,
`AU`, `VST2`). Iterates by index using `list(GET ...)` for portability across
CMake versions (no `ZIP_LISTS` requirement). Added an explanatory comment
block documenting the JUCE casing requirement and the failure mode of the
prior `TOUPPER` approach.

**Verification:**
- Tier 1: re-read modified region (lines 68-101); fix present, surrounding
  function intact.
- Tier 2: `cmake -S . -B build` configure succeeded (Configuring done 11.3s,
  Generating done 0.3s, build files written). Confirmed
  `note-expression/cpp/vst3` continues to route to `OLyrica_VST3` via the
  status message `Added note-expression/cpp/vst3 sources to OLyrica_VST3`.

## Skipped Issues

### LR-01: drainBlockEvents — redundant clear after swap

**File:** `modules/tuning/note-expression/cpp/NoteExpression.h:178-184`
**Reason:** Out of scope (severity: low; fix_scope = critical_warning).
**Original issue:** Redundant `blockEvents.clear()` after `out.swap(blockEvents)`;
no correctness impact, code-quality cleanup only.

---

_Fixed: 2026-04-25T20:38:54Z_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
