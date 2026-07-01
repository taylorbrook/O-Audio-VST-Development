---
phase: quick-260701-k0a
plan: 01
subsystem: build-scripts-ci
tags: [build-scripts, ci, juce-patch, resolver, hardening]
status: complete
requires:
  - .planning/quick/260701-in8-complete-a-review-of-this-system-for-cre/260701-in8-REVIEW.md
provides:
  - scripts/resolve-target.sh
  - scripts/juce-patches/note-expression-juce-8.0.9.patch
affects:
  - scripts/build-and-install.sh
  - .github/workflows/build-and-release.yml
  - scripts/apply-juce-patches.sh
  - modules/registry.yaml
tech-stack:
  added: []
  patterns:
    - "Single-source-of-truth bash library (sourceable + directly executable) shared by local script and CI"
    - "patch --forward + --dry-run preflight + authoritative marker recount (exit-code-independent verification)"
key-files:
  created:
    - scripts/resolve-target.sh
  modified:
    - scripts/build-and-install.sh
    - .github/workflows/build-and-release.yml
    - scripts/apply-juce-patches.sh
    - scripts/juce-patches/note-expression-juce-8.0.9.patch (renamed from 8.0.4)
    - modules/registry.yaml
    - modules/tuning/note-expression/README.md
    - modules/tuning/note-expression/module.yaml
decisions:
  - "Updated 3 additional 8.0.4 references (README.md + module.yaml x2) beyond the plan's enumerated 3 sites — the plan's own grep gate requires zero 8.0.4 refs in scripts/ and modules/ (Rule 3 goal-completion)"
metrics:
  duration: ~4min
  completed: 2026-07-01
  tasks: 3
  files: 8
---

# Phase quick-260701-k0a Plan 01: Fix build-script & CI defects Summary

One-liner: Factored the robust plugin target/product-name resolver into a shared `scripts/resolve-target.sh` (sourced by both `build-and-install.sh` and all 5 CI parse sites), hardened `build-and-install.sh` under `set -euo pipefail`, and made the JUCE patch applier `--forward`-safe with a marker-recount gate — eliminating the local/CI drift that would break an O-Texture (`${PROJECT_NAME}`) release.

## What Was Built

### Task 1 — Shared resolver + build-and-install.sh hardening (commit `cf5696b`)
- New `scripts/resolve-target.sh` (executable, sourceable + CLI-dispatchable). Two pure functions that echo one result to stdout, warnings to stderr only:
  - `resolve_cmake_target_for <folder>` — ports the exact awk/${VAR}/artefacts/folder resolution logic; verified `O-Texture -> OuariconTexture` (via `set(PROJECT_NAME OuariconTexture)` + `juce_add_plugin(${PROJECT_NAME})`), `O-Chorus -> OuariconChorus`, `O-Gain -> O-Gain`.
  - `resolve_product_name_for <folder>` — mirrors the CI PRODUCT_NAME parse (quoted value, `${...}` strip, folder default); verified all three -> `O-Texture` / `O-Chorus` / `O-Gain`.
  - CLI guarded by `[ "${BASH_SOURCE[0]}" = "${0}" ]` so sourcing does not run dispatch.
- `build-and-install.sh`: `set -e` -> `set -euo pipefail` (BUG-06a); sources the shared library near the top; `resolve_cmake_target()` reduced to a thin wrapper delegating to `resolve_cmake_target_for "$PLUGIN_NAME"` (CMAKE_TARGET global contract preserved, Phases 2/3/5 untouched); `${JUCE_DIR:-}` guard (BUG-06b); `execute()` captures `rc` per-branch instead of a clobber-prone trailing `return ${PIPESTATUS[0]}` (BUG-06c).

### Task 2 — CI wired to the shared resolver (commit `1166b2c`)
- Replaced all 5 naive parse sites in `.github/workflows/build-and-release.yml` with `bash scripts/resolve-target.sh ...` calls: 2 CMAKE_TARGET sites (macOS ~87, Windows ~439) and 3 PRODUCT_NAME sites (macOS PKG ~235, Windows ~453, release job ~566). Multi-line grep|sed + `${...}`-strip + default collapsed to a single resolver call each. Triggers/env/signing untouched; no `workflow_dispatch` introduced.

### Task 3 — JUCE patch rename + `--forward` preflight (commit `8dd245a`)
- `git mv note-expression-juce-8.0.4.patch -> ...-8.0.9.patch` (history preserved).
- Updated every `8.0.4` reference (apply-juce-patches.sh comment + PATCH_FILE, registry.yaml, plus README.md + module.yaml x2) so `grep -R note-expression-juce-8.0.4 scripts modules` returns nothing.
- `apply-juce-patches.sh` (BUG-05): `patch -p1 --forward --dry-run` preflight + `patch -p1 --forward` real apply (both `|| true` under `set -e`), followed by an authoritative POST marker recount across both target files — success gate depends on the recount (`POST -ge 2`), not on patch's exit code. Existing JUCE_DIR preflight, patch-existence check, `FOUND -ge 2` idempotency skip, and CRLF->LF normalization left unchanged.
- Unrelated `modules/registry.yaml` O-simplePhysicalModelSynth WIP hunk isolated via `git stash push -- modules/registry.yaml` before the edit and restored via `git stash pop` after the commit (verified restored + patch ref intact).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking/goal-completion] Two additional files carried stale 8.0.4 references**
- **Found during:** Task 3 verification
- **Issue:** The plan enumerated 3 rename-reference sites (apply-juce-patches.sh line 11 + 22, registry.yaml line 288), but `modules/tuning/note-expression/README.md` (1 ref) and `modules/tuning/note-expression/module.yaml` (2 refs) also referenced `note-expression-juce-8.0.4.patch`. The plan's own verify gate requires `grep -R note-expression-juce-8.0.4 scripts modules` to return nothing.
- **Fix:** Updated all 3 additional references to `8.0.9` (both files were clean at HEAD, unrelated to the pre-staged WIP).
- **Files modified:** modules/tuning/note-expression/README.md, modules/tuning/note-expression/module.yaml
- **Commit:** 8dd245a

## Threat Mitigations Applied
- T-k0a-01 (patch partial/repeat apply): `--forward` skip-applied + `--dry-run` preflight + marker recount gate.
- T-k0a-02 (CI target resolution failing on `${PROJECT_NAME}`): shared robust resolver at all 5 CI sites; verified locally against O-Texture/O-Chorus/O-Gain.
- T-k0a-03 (exit-masking + unbound-var footgun): `set -euo pipefail` + `${JUCE_DIR:-}` + per-branch rc capture.
- T-k0a-04 (accidental CI dispatch): all verification static/local — no push, tag, or workflow_dispatch; verify asserts `workflow_dispatch` absent.

## Verification (static + local, NO CI triggered)
- Task 1: `bash -n` both scripts; resolver returns 3 verified targets + 3 verified product names; `set -euo pipefail` present once; sources resolve-target.sh; `${JUCE_DIR:-}` present. PASS.
- Task 2: PyYAML `safe_load` parses the workflow; `resolve-target.sh` referenced exactly 5x; zero `juce_add_plugin(` sed parses; zero `PRODUCT_NAME "` grep parses; no `workflow_dispatch`. PASS.
- Task 3: `bash -n` apply-juce-patches.sh; patch renamed (new exists, old gone); zero 8.0.4 refs in scripts/modules; both 8.0.9 refs present; `--forward --dry-run` + `--forward` apply present; O-simplePhysicalModelSynth WIP restored. PASS.

## Commits
- `cf5696b` fix(quick-260701-k0a): factor shared resolve-target.sh + harden build-and-install.sh (BUG-01/IMP-03/BUG-06) — scripts/resolve-target.sh, scripts/build-and-install.sh
- `1166b2c` fix(quick-260701-k0a): CI resolves cmake target + product name via shared resolve-target.sh (BUG-01/IMP-03) — .github/workflows/build-and-release.yml
- `8dd245a` fix(quick-260701-k0a): rename JUCE patch to 8.0.9 + --forward preflight in apply-juce-patches (UPD-01/BUG-05) — scripts/apply-juce-patches.sh, patch rename, modules/registry.yaml, modules/tuning/note-expression/README.md, modules/tuning/note-expression/module.yaml

Each commit staged only its own files via explicit pathspecs; the pre-existing unrelated staged/unstaged WIP (O-DigiDelay, O-Freeze, O-Gain, O-Formant, O-MicrotonalSampler, etc.) was untouched. No CI run triggered.

## Known Stubs
None.

## Self-Check: PASSED
- scripts/resolve-target.sh — FOUND (executable)
- scripts/juce-patches/note-expression-juce-8.0.9.patch — FOUND
- Commit cf5696b — FOUND
- Commit 1166b2c — FOUND
- Commit 8dd245a — FOUND
