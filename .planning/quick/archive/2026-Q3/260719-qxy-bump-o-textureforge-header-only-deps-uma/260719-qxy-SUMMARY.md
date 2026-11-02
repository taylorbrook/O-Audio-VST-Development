---
phase: 260719-qxy-bump-o-textureforge-header-only-deps-uma
plan: 01
subsystem: infra
tags: [cmake, fetchcontent, umappp, nanoflann, o-textureforge, dependency-bump, irlba, def-l26-01]

requires: []
provides:
  - O-TextureForge nanoflann pin bumped v1.6.2 -> 1.10.1
  - O-TextureForge umappp pin bumped v3.2.0 -> v3.3.2 with num_threads_optimize migration
  - DEF-L26-01 (umappp/irlba transitive drift) cleared — fresh re-fetch build compiles clean
  - verify-suite-battery.sh KNOWN-FAIL early-return guard removed (O-TextureForge now built + gated)
affects: [o-textureforge, verify-suite-battery, umappp, irlba-durable-pin-followup]

tech-stack:
  added: [nanoflann 1.10.1, umappp v3.3.2]
  patterns: [force-clean FetchContent re-fetch to exercise the genuine fresh-build path]

key-files:
  created: []
  modified:
    - plugins/O-TextureForge/CMakeLists.txt
    - plugins/O-TextureForge/Source/dsp/UMAPProjection.cpp
    - scripts/verify-suite-battery.sh

key-decisions:
  - "nanoflann pinned to unprefixed 1.10.1 (no v1.10.1 tag exists upstream)"
  - "umappp Options::parallel_optimization -> num_threads_optimize=1; added num_threads_spectral=1 for deterministic irlba spectral init"
  - "Removed both the functional KNOWN-FAIL guard block AND its stale header comment; updated header to document DEF-L26-01 resolution"

patterns-established:
  - "Force-clean build/_deps/{umappp,irlba,knncolle,nanoflann,eigen}-* before rebuild so a stale dep tree cannot mask transitive drift"

requirements-completed: [QXY-DEP-BUMP]

coverage:
  - id: D1
    description: "nanoflann 1.10.1 + umappp v3.3.2 pins with num_threads_optimize migration; fresh re-fetch compiles clean (DEF-L26-01 signature absent)"
    requirement: "QXY-DEP-BUMP"
    verification:
      - kind: integration
        ref: "./scripts/build-and-install.sh O-TextureForge (fresh re-fetch, umappp@v3.3.2 nanoflann@1.10.1 confirmed on disk, no 'converged'/irlba::Results error)"
        status: pass
    human_judgment: false
  - id: D2
    description: "auval + pluginval strictness-8 gates pass on the installed O-TextureForge-dev VST3"
    requirement: "QXY-DEP-BUMP"
    verification:
      - kind: integration
        ref: "bash scripts/verify-au-link.sh O-TextureForge => AU VALIDATION SUCCEEDED"
        status: pass
      - kind: integration
        ref: "pluginval --strictness-level 8 --skip-gui-tests --validate O-TextureForge-dev.vst3 => SUCCESS (rc=0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "UMAP 2D scatter map + KD-tree grain-query behavior is audibly unchanged vs the pre-bump build (drone/scatter regression class)"
    verification: []
    human_judgment: true
    rationale: "UMAP grain selection is load-bearing and only observable in a DAW/Standalone; compile-clean is insufficient. Pending human DAW verification (Task 3 checkpoint)."

duration: ~15min
completed: 2026-07-19
status: complete
---

# Quick Task 260719-qxy: Bump O-TextureForge header-only deps Summary

**nanoflann 1.6.2→1.10.1 and umappp 3.2.0→v3.3.2 bumped with the required `num_threads_optimize` Options migration; a force-clean FetchContent re-fetch compiled clean — clearing the DEF-L26-01 umappp/irlba transitive-drift KNOWN-FAIL — with auval + pluginval strictness-8 both green.**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-07-19T19:30:00Z (approx)
- **Completed:** 2026-07-19T19:45:00Z (approx)
- **Tasks:** 2 of 3 automatable tasks complete; Task 3 is a human-verify DAW checkpoint (pending)
- **Files modified:** 3

## Accomplishments
- Bumped nanoflann pin `v1.6.2` → `1.10.1` (highest tag is unprefixed) and umappp pin `v3.2.0` → `v3.3.2`.
- Migrated the umappp v3.3.0 breaking change: `Options::parallel_optimization = false` → `num_threads_optimize = 1`, and added `num_threads_spectral = 1` for deterministic irlba spectral init. Without this migration the bump does not compile.
- Force-cleaned `build/_deps/{umappp,irlba,knncolle,nanoflann,eigen}-*` and rebuilt — genuinely exercising the fresh-build path. On-disk confirmation: umappp fetched at `v3.3.2`, nanoflann at `1.10.1`, new `num_threads_optimize` field present in fetched header, old field absent, **no `converged`/`irlba::Results` DEF-L26-01 signature** in the build log.
- auval (`verify-au-link.sh O-TextureForge`) → AU VALIDATION SUCCEEDED.
- pluginval `--strictness-level 8 --skip-gui-tests` on the installed `O-TextureForge-dev.vst3` → SUCCESS (rc=0).
- Removed the `O-TextureForge` KNOWN-FAIL early-return guard in `scripts/verify-suite-battery.sh` (only after all three gates passed) and updated the stale header comment to document the DEF-L26-01 resolution.

## Task Commits

1. **Task 1: Bump pins + migrate umappp Options field** — `263fd53` (chore)
2. **Task 2: Force-clean re-fetch, build, install, gate + drop KNOWN-FAIL guard** — `c54ca09` (chore)

_Docs (SUMMARY/STATE) commit is handled by the orchestrator._

## Files Created/Modified
- `plugins/O-TextureForge/CMakeLists.txt` — nanoflann `1.10.1`, umappp `v3.3.2` FetchContent pins
- `plugins/O-TextureForge/Source/dsp/UMAPProjection.cpp` — `num_threads_optimize`/`num_threads_spectral` Options migration
- `scripts/verify-suite-battery.sh` — removed O-TextureForge KNOWN-FAIL guard block + refreshed header comment

## Decisions Made
- nanoflann written as unprefixed `1.10.1` (no `v1.10.1` tag exists upstream — verified in RESEARCH.md via git ls-remote).
- Added `num_threads_spectral = 1` (RESEARCH-recommended, optional) alongside the required `num_threads_optimize = 1` for run-to-run determinism given the SPECTRAL init path.
- Removed the KNOWN-FAIL guard's stale header comment in addition to the functional block, updating it to describe the resolution rather than leave contradictory documentation.

## Deviations from Plan
None - plan executed exactly as written. All three automatable gates (fresh build, auval, pluginval strictness-8) passed, so the KNOWN-FAIL guard removal proceeded as specified.

Minor note: `build-and-install.sh` Phase 2 reported "Build directory exists, skipping configure" — but ninja re-populated the force-cleaned deps via the FetchContent subbuild targets. Confirmed on disk that umappp@v3.3.2 and nanoflann@1.10.1 were actually re-fetched and the new headers compiled in, so the fresh-build path was genuinely exercised (no `rm -rf build` escalation was needed).

## Issues Encountered
None. The escalation path (`rm -rf build` full fresh configure) noted in the plan/RESEARCH was not required — the force-clean of `build/_deps/*` plus ninja's FetchContent subbuild re-population was sufficient and compiled clean.

## Durable-Fix Follow-up (out of scope for this bump)
- **umappp transitive irlba/knncolle `GIT_TAG master` (unpinned moving ref):** umappp v3.3.2's `extern/CMakeLists.txt` still declares irlba and knncolle at `GIT_TAG master` (irlba repo moved `LTLA/CppIrlba` → `libscran/irlba`). This bump clears the *current* drift and the force-clean re-fetch proved it compiles today, but it does **not** structurally pin the transitive dep — irlba master can drift again upstream. Durable fix: add explicit `FetchContent_Declare(irlba ... GIT_TAG <pinned tag>)` + knncolle overrides *before* `FetchContent_MakeAvailable(umappp)` in O-TextureForge's CMakeLists.txt. Threat register T-QXY-02 (medium, accepted) tracks this.

## KNOWN-FAIL Guard Status
**Removed.** The `if [ "$plugin" = "O-TextureForge" ]` early-return block (formerly ~lines 129–135) is deleted, and the stale header comment updated. The verify-suite-battery now builds + gates O-TextureForge like every other plugin. Removal was gated on all three checks passing, per the plan.

## Pending Human Verification (Task 3 — DAW checkpoint, NOT blocking this task)
UMAP grain-selection is load-bearing (see O-TextureForge drone/scatter bug history in MEMORY.md — scatterX/Y + UMAP/PCA position bias). Compile-clean is not sufficient. Manual DAW checklist to run before considering the bump behaviorally verified:
1. Open `O-TextureForge-dev` in a DAW (or Standalone).
2. Load a corpus, let the UMAP projection compute — confirm the 2D scatter map renders and points spread as before (not collapsed/degenerate).
3. Move scatterX / scatterY and the position controls — confirm grain selection audibly follows the UMAP scatter position (the drone/scatter regression class).
4. Confirm KD-tree nearest-grain querying still returns sensible neighbors (no silence, no stuck single-grain drone).

Expected: scatter map + grain query behave identically to the pre-bump build. Resume signal: "approved" or describe any scatter/grain-query regressions.

## Submodule Guard
`plugins/O-Orbit/libs/SAF` guard ran clean before both task commits — no staged path fell inside the submodule.

## Next Phase Readiness
- Dependency bump is code-complete and passes all automatable gates; DEF-L26-01 is cleared and the suite battery no longer short-circuits O-TextureForge.
- One open item: human DAW verification of UMAP scatter + grain-query behavior (Task 3), and the durable irlba pin follow-up above.

## Self-Check: PASSED

- All 3 modified source/script files present on disk.
- Both task commits (`263fd53`, `c54ca09`) exist in git history.
- Installed `O-TextureForge-dev.vst3` present in `~/Library/Audio/Plug-Ins/VST3/`.

---
*Phase: 260719-qxy-bump-o-textureforge-header-only-deps-uma*
*Completed: 2026-07-19*
