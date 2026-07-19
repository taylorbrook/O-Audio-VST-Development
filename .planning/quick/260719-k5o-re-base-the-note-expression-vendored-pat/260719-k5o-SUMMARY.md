---
phase: quick-260719-k5o
plan: 01
subsystem: vendored-juce-fork
status: complete
tags:
  - juce
  - vendored-fork
  - note-expression
  - ci
dependency_graph:
  requires: []
  provides:
    - vendored JUCE-overrides re-based onto pristine 8.0.14
    - note-expression-juce-8.0.14.patch
  affects:
    - future JUCE_VERSION 8.0.9 → 8.0.14 bump
tech_stack:
  patterns:
    - both-paths-tolerant CMake header resolution (EXISTS headless else old-path)
key_files:
  created:
    - vendored/JUCE-overrides/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h
    - scripts/juce-patches/note-expression-juce-8.0.14.patch
  modified:
    - vendored/JUCE-overrides/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
    - .github/workflows/build-and-release.yml
    - modules/tuning/note-expression/module.cmake
    - scripts/apply-juce-patches.sh
  deleted:
    - vendored/JUCE-overrides/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h (moved to headless path)
  renamed:
    - scripts/juce-patches/note-expression-juce-8.0.4.patch → note-expression-juce-8.0.9.patch
decisions:
  - Companion juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.cpp NOT patched/vendored (defines only getCompatibleParameterIds + convertJuceParameterId, no NE code) — RESEARCH-locked.
  - module.cmake _NE_FILE1 resolved both-paths-tolerant (EXISTS headless else old path) so local 8.0.9 dev and scratch/CI 8.0.14 both pass.
  - 8.0.9 patch rename is literal (per locked decision); NOT regenerated — retains 8.0.4-vs-8.0.9 drift hunks (RESEARCH A1).
metrics:
  duration: ~12min
  completed: 2026-07-19
  tasks: 3
  files: 8
---

# Quick Task 260719-k5o: Re-base JUCE-NE-PATCH onto JUCE 8.0.14 Summary

Re-stitched the Ouaricon VST3 Note Expression vendored fork (JUCE-NE-PATCH) from JUCE 8.0.9 onto pristine 8.0.14, moving the header to its new `juce_audio_processors_headless/utilities/` module home, repointing all path touchpoints (both-paths-tolerant), regenerating the 8.0.14 patch, and proving it with a clean OLyrica_VST3 compile against a scratch 8.0.14 tree — local JUCE install and CI `JUCE_VERSION` untouched.

## What Was Done

### Task 1 — Re-vendor pristine 8.0.14 with NE re-stitched (commit 337fd46)
- Downloaded pristine JUCE 8.0.14 macOS release zip to the scratch root; verified `juce_core.h` reports `version: 8.0.14`, headless header present, old-path header absent.
- Created the new vendored header at `vendored/JUCE-overrides/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h` from the pristine 8.0.14 base, with the `struct Vst3RawEvent` + `virtual void onVst3RawEvent (const Vst3RawEvent&) {}` inserted verbatim between the destructor and `queryIEditController`.
- Removed the stale old-path header via `git rm` (tracked as rename R088).
- Overwrote the VST3 wrapper `.cpp` with pristine 8.0.14 and re-stitched the braced NE event-forward block (kNoteOn/kNoteOff/kNoteExpressionValue → `ext->onVst3RawEvent(raw)`) in place of the pristine one-liner, inside `#if JucePlugin_WantsMidiInput`.
- Both files retain the `JUCE-NE-PATCH` marker.

### Task 2 — Repoint CI grep gates + module.cmake (commit 7365b5f)
- CI `.h` grep gates (macOS + Windows) repointed to the headless path; 0 references to the old header path remain. `.cpp` gates, `cp -R` copy step, and `JUCE_VERSION` (8.0.9) untouched.
- `module.cmake` `_NE_FILE1` now resolves both-paths-tolerant: `EXISTS(headless)` → use it, else fall back to the pre-8.0.14 path. Verified to pass on both local 8.0.9 and scratch 8.0.14.
- Version-neutral FATAL_ERROR message (was hard-coded "Ensure JUCE 8.0.4 is installed").

### Task 3 — Patch rename + regenerated 8.0.14 patch (commit cc49065)
- `git mv note-expression-juce-8.0.4.patch → note-expression-juce-8.0.9.patch`.
- New `note-expression-juce-8.0.14.patch`: two NE hunks only (header on the headless path + `.cpp` braced block), `Target: JUCE 8.0.14`, LF-normalized. Dry-run-applies cleanly onto an LF-normalized pristine 8.0.14 tree.
- `apply-juce-patches.sh` `PATCH_FILE` + header comment → 8.0.9 filename; old 8.0.9 header path at :39 preserved (local install still 8.0.9).

## End-to-end Compile-Check (locked verification)

- Overlaid the re-based vendored overrides onto the scratch 8.0.14 tree (`cp -R`, mirrors CI). Markers confirmed at the headless path.
- Configured with `JUCE_DIR` pointed at the scratch tree — `module.cmake`'s both-paths guard found the marker (no FATAL_ERROR); configure done in ~72s.
- **Built `OLyrica_VST3` → exit 0**, artefact produced and ad-hoc signed at `$SCRATCH/ne-rebase-build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica-dev.vst3`. Only two benign pre-existing warnings (JUCE SDK `funknown.h` shadow; `NoteExpression_VST3.cpp` non-virtual-dtor delete) — unrelated to the re-base.
- Note: the plan's target name `O-Lyrica_VST3` is `OLyrica_VST3` in CMake (target ≠ PRODUCT_NAME; folder is O-Lyrica). Built the resolved target.
- **Local `/Users/taylorbrook/JUCE` stayed at 8.0.9; CI `JUCE_VERSION` stayed at 8.0.9** — both untouched.

## Deviations from Plan

**1. [Rule 3 - blocking] 8.0.14 patch dry-run against LF-normalized tree (CRLF-origin fix)**
- **Found during:** Task 3 verification.
- **Issue:** The official JUCE 8.0.14 macOS/Windows release zips ship **CRLF** line endings. The plan's `<verify>` dry-runs the LF patch against a raw (CRLF) copy of the pristine tree, which fails with "hunk failed" purely on line-ending mismatch (both hunks). This is not a defect in the patch content.
- **Fix:** Kept the patch LF-normalized (as the plan action mandates, and as required for the Windows CI path). Verified the patch dry-run-applies cleanly (both hunks, exit 0) against an **LF-normalized** pristine 8.0.14 tree — exactly the state `scripts/apply-juce-patches.sh` produces (it CRLF→LF normalizes the target before `patch -p1`). This is the production application path.
- **Files modified:** none beyond the patch itself.

**2. Scratch download path**
- Pristine 8.0.14 downloaded to `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/bae91ebe-fa18-45f2-a71d-9bb523f3c6a2/scratchpad` as `JUCE-8.0.14-pristine` (patch source-of-truth) + `JUCE-8.0.14` (compile-check overlay target). Never touched `/Users/taylorbrook/JUCE`.

## RESEARCH A1 Caveat (recorded per plan output spec)

The renamed `note-expression-juce-8.0.9.patch` is a literal rename of the old `-8.0.4.patch`. It **retains 8.0.4-vs-8.0.9 drift hunks** and is NOT a clean pristine-8.0.9 NE-only diff. Regenerating it as `diff pristine-8.0.9 vendored/` is out of scope here — the local install is already patched and `apply-juce-patches.sh` skips via the marker, so the drift is inert. Honored the locked rename decision literally.

## Branch Status — Unmergeable by Design

Branch `quick/260719-k5o-juce-ne-rebase-8014` is **intentionally unmergeable** until the future `JUCE_VERSION` 8.0.9 → 8.0.14 bump lands. Its CI is broken against 8.0.9 (the `cp -R` overlays 8.0.14 vendored files onto an 8.0.9 download; the `.h` grep gates point at the headless path that doesn't exist in an 8.0.9 tree). Do NOT merge to main until the bump follows.

## Commits

| Task | Commit | Description |
| ---- | ------ | ----------- |
| 1 | 337fd46 | Re-vendor JUCE-NE-PATCH onto pristine 8.0.14 (header → headless path) |
| 2 | 7365b5f | Repoint NE header grep gates + module.cmake both-paths guard |
| 3 | cc49065 | Rename NE patch → 8.0.9, add regenerated 8.0.14 patch |

## Self-Check: PASSED

- All 7 tracked/artifact files present; old-path header confirmed removed.
- All 3 commits (337fd46, 7365b5f, cc49065) present in git history.
- Docs artifacts (CONTEXT/RESEARCH/SUMMARY) left untracked for the orchestrator's docs commit (not committed here per task constraints).
