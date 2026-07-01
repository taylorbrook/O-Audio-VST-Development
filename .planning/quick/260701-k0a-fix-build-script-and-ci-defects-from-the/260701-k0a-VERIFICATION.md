---
phase: quick-260701-k0a
verified: 2026-07-01T22:00:00Z
status: passed
score: 5/5 must-haves verified
behavior_unverified: 0
overrides_applied: 0
---

# Quick Task 260701-k0a: Fix build-script and CI defects Verification Report

**Task Goal:** Fix build-script and CI defects from the 260701-in8 system review: (1) BUG-01/IMP-03 — factor robust `resolve_cmake_target()` into shared `scripts/resolve-target.sh` used by both `scripts/build-and-install.sh` and 5 CI call sites; (2) UPD-01 — rename the JUCE patch `...8.0.4.patch` → `...8.0.9.patch` and update all references; (3) BUG-05 — `patch --forward --dry-run` preflight in `apply-juce-patches.sh`; (4) BUG-06 — `set -uo pipefail` in `build-and-install.sh` and fix the `eval | tee` exit-masking. No CI run triggered.

**Verified:** 2026-07-01
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `scripts/resolve-target.sh` resolves O-Texture → OuariconTexture (never the literal `${PROJECT_NAME}`), O-Chorus → OuariconChorus, O-Gain → O-Gain | ✓ VERIFIED | Ran locally: `bash scripts/resolve-target.sh cmake-target O-Texture` → `OuariconTexture`; `O-Chorus` → `OuariconChorus`; `O-Gain` → `O-Gain`. Product-name variants also verified: `O-Texture`/`O-Chorus`/`O-Gain` respectively. |
| 2 | The CI workflow no longer contains any naive same-line `juce_add_plugin` target parse; it calls the shared resolver instead | ✓ VERIFIED | `grep -c 'juce_add_plugin(' .github/workflows/build-and-release.yml` = 0; `grep -c 'PRODUCT_NAME "' ...` = 0; `grep -c 'resolve-target.sh' ...` = 5 (2 cmake-target sites at lines 87, 436; 3 product-name sites at lines 235, 450, 561). All 5 sites confirmed by direct read of surrounding context — repo-root-relative `bash scripts/resolve-target.sh ...` calls, correctly gated by `shell: bash` (Windows) or default-bash (macOS/ubuntu runners). |
| 3 | `build-and-install.sh` sources the shared resolver, runs clean under `set -euo pipefail`, and `execute()` returns the real left-hand command exit status | ✓ VERIFIED | Line 2 is `set -euo pipefail` (exactly once, confirmed by grep). Line 6-7 source `resolve-target.sh` via `SCRIPT_DIR`. `resolve_cmake_target()` (line 193-198) is a thin wrapper delegating to `resolve_cmake_target_for "$PLUGIN_NAME"`. `${JUCE_DIR:-}` guard present at line 177. `execute()` (lines 104-127) captures `rc` immediately after each branch (`local rc=${PIPESTATUS[0]}` for the verbose/tee path, `local rc=$?` for the non-verbose path) then `return "$rc"`. Behaviorally re-implemented both branches in an isolated test harness under `set -euo pipefail` and confirmed `if ! execute false` correctly detects failure and `if ! execute true` correctly detects success, in both VERBOSE and non-VERBOSE modes. |
| 4 | `apply-juce-patches.sh` points at `note-expression-juce-8.0.9.patch` and a partial prior apply is skipped (not re-applied or reversed), verified by a post-apply marker recount | ✓ VERIFIED | `PATCH_FILE` at line 22 points to `note-expression-juce-8.0.9.patch` (file exists at `scripts/juce-patches/`; old `8.0.4` filename is gone — confirmed via `git mv` in commit `8dd245a`). Lines 70-74 run `patch -p1 --forward --dry-run` preflight then `patch -p1 --forward` real apply, both guarded with `\|\| true`. Lines 78-91 recount the `JUCE-NE-PATCH` marker across both target files independently of patch's exit code and gate success/failure (`POST -ge 2`) on the recount. Zero remaining `note-expression-juce-8.0.4` references anywhere under `scripts/` or `modules/` (grep confirmed clean; the only remaining 8.0.4 hits in the repo are historical/archived docs under `.planning/milestones/...` and `research/...`, which are out of the plan's declared scope of `scripts modules`). |
| 5 | No CI run is triggered: no pushes, tags, or workflow_dispatch; all verification is static + local | ✓ VERIFIED | `git tag --contains cf5696b` / `--contains 8dd245a` both empty (no tags created). `grep -q workflow_dispatch .github/workflows/build-and-release.yml` fails (absent — confirmed unmodified from before). No `git push` executed during this verification session; all checks were `bash -n`, PyYAML parse, local script execution, and grep/read. |

**Score:** 5/5 truths verified (0 present-but-behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `scripts/resolve-target.sh` | Shared sourceable+executable resolver library | ✓ VERIFIED | Exists, executable (`test -x` passes), `bash -n` clean, both functions present with correct awk/${VAR}/artefacts-fallback/folder-fallback logic, CLI dispatch guarded correctly. |
| `scripts/juce-patches/note-expression-juce-8.0.9.patch` | Renamed patch (history preserved) | ✓ VERIFIED | File exists; old `8.0.4` filename gone; `git show --name-only 8dd245a` shows the rename (git tracked as a rename with 0 content changes). |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|----|--------|---------|
| CI steps at yml:87, :436 (cmake-target) | `scripts/resolve-target.sh` | `bash scripts/resolve-target.sh cmake-target "$PLUGIN"` | ✓ WIRED | Confirmed by direct read — both the macOS (line 87) and Windows (line 436) "Resolve CMake Target" steps call the resolver identically. |
| CI steps at yml:235, :450, :561 (product-name) | `scripts/resolve-target.sh` | `bash scripts/resolve-target.sh product-name "$PLUGIN"` | ✓ WIRED | Confirmed by direct read — macOS PKG installer (235), Windows "Get Product Name" (450), and release job "Get Product Name" (561, runs on ubuntu-latest with `actions/checkout@v4` present) all call the resolver identically. |
| `build-and-install.sh` | `scripts/resolve-target.sh` | `source "$SCRIPT_DIR/resolve-target.sh"`; `resolve_cmake_target()` delegates to `resolve_cmake_target_for` | ✓ WIRED | Source line present near top of file; wrapper function confirmed at lines 193-198; `CMAKE_TARGET` global contract preserved, called from Phases 2/3/5 unchanged. |
| `apply-juce-patches.sh` `PATCH_FILE` | renamed 8.0.9 patch | direct assignment | ✓ WIRED | Line 22: `PATCH_FILE="$PATCH_DIR/note-expression-juce-8.0.9.patch"`. |
| `modules/registry.yaml:288` | renamed 8.0.9 patch reference | prose reference | ✓ WIRED | Confirmed updated to `note-expression-juce-8.0.9.patch`; the unrelated O-simplePhysicalModelSynth WIP hunk (line ~342) was correctly isolated out of this commit via `git stash`/`pop` and remains only in the working tree (unstaged), not in commit `8dd245a`. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Resolver resolves `${PROJECT_NAME}`-backed target | `bash scripts/resolve-target.sh cmake-target O-Texture` | `OuariconTexture` | ✓ PASS |
| Resolver resolves literal target != folder | `bash scripts/resolve-target.sh cmake-target O-Chorus` | `OuariconChorus` | ✓ PASS |
| Resolver resolves literal target == folder | `bash scripts/resolve-target.sh cmake-target O-Gain` | `O-Gain` | ✓ PASS |
| Resolver product-name (all 3 plugins) | `bash scripts/resolve-target.sh product-name {O-Texture,O-Chorus,O-Gain}` | `O-Texture` / `O-Chorus` / `O-Gain` | ✓ PASS |
| `execute()` propagates real exit status under `set -euo pipefail`, non-verbose branch | Isolated reproduction of the exact function body, `if ! execute false` / `if ! execute true` | failure detected / success detected | ✓ PASS |
| `execute()` propagates real exit status, verbose (`eval \| tee`) branch | Same, with `VERBOSE=true` | failure detected via `PIPESTATUS[0]` | ✓ PASS |
| All 3 shell scripts parse cleanly | `bash -n scripts/{resolve-target,build-and-install,apply-juce-patches}.sh` | exit 0 each | ✓ PASS |
| CI workflow YAML still parses | `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/build-and-release.yml'))"` | `yaml-ok` | ✓ PASS |
| No debt markers introduced | `grep -n 'TODO\|FIXME\|XXX\|TBD\|PLACEHOLDER\|HACK'` on the 3 touched scripts | no matches | ✓ PASS |

### Requirements Coverage

| Requirement | Source | Description | Status | Evidence |
|-------------|--------|--------------|--------|----------|
| BUG-01 | 260701-in8-REVIEW.md | CI naive `sed` parse fails on `${PROJECT_NAME}`-style targets (breaks O-Texture) | ✓ SATISFIED | Shared resolver correctly resolves `${PROJECT_NAME}` case; both CI cmake-target sites call it. |
| IMP-03 | 260701-in8-REVIEW.md | CI duplicates weaker resolution logic; factor into shared script | ✓ SATISFIED | `scripts/resolve-target.sh` created and sourced/called by both local script and all 5 CI sites. |
| UPD-01 | 260701-in8-REVIEW.md | Patch filename says 8.0.4, content is 8.0.9 | ✓ SATISFIED | `git mv` rename; all references in `scripts/` and `modules/` updated (including 2 files beyond the plan's enumerated 3, self-corrected by executor per its own grep gate). |
| BUG-05 | 260701-in8-REVIEW.md | Partial prior patch apply gets re-applied/reversed on rerun | ✓ SATISFIED | `--forward --dry-run` preflight + `--forward` real apply + authoritative marker recount gate, independent of patch's exit code. |
| BUG-06 | 260701-in8-REVIEW.md | `set -e` only; `eval \| tee` exit-masking; unset `JUCE_DIR` under `set -u` | ✓ SATISFIED | `set -euo pipefail`; `${JUCE_DIR:-}` guard; per-branch `rc` capture verified behaviorally in both VERBOSE and non-VERBOSE modes. |

No orphaned requirements — all 5 map 1:1 to plan tasks and are addressed.

### Anti-Patterns Found

None. No TODO/FIXME/XXX/TBD/PLACEHOLDER/HACK markers in any of the 3 touched shell scripts. No empty-return stubs. No hardcoded static returns replacing real logic.

### Human Verification Required

None. All must-haves are statically/locally verifiable and were verified directly (no visual, real-time, or external-service-dependent behavior in scope). The plan explicitly scoped verification to static + local checks with no CI run — that constraint was honored throughout this verification pass as well (no push, tag, or workflow_dispatch was issued).

### Gaps Summary

No gaps. All 5 must-have truths verified against the actual codebase (not just SUMMARY.md claims): the resolver was executed locally and produced the exact expected outputs for all 3 test plugins (both `cmake-target` and `product-name` subcommands); the CI workflow was read directly and confirmed to call the resolver at all 5 sites with zero remaining naive parses; `build-and-install.sh`'s `execute()` rc-capture logic was behaviorally re-tested in isolation under `set -euo pipefail` for both branches; the JUCE patch rename and `--forward`/dry-run/marker-recount logic in `apply-juce-patches.sh` were read directly and match the plan's BUG-05 design; all three commits (`cf5696b`, `1166b2c`, `8dd245a`) were confirmed to exist and to stage exactly the files each claims, with the unrelated `O-simplePhysicalModelSynth` registry.yaml WIP hunk correctly isolated out of `8dd245a` via the stash/pop sequence (confirmed still present, unstaged, in the working tree). No CI run was triggered by this task or by this verification.

---

_Verified: 2026-07-01_
_Verifier: Claude (gsd-verifier)_
