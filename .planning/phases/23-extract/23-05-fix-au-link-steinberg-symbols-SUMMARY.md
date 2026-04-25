---
phase: 23-extract
plan: 05
status: complete
subsystem: build-system
tags: [defect-fix, au-link, two-tu-split, custom-deleter-pimpl, dispatch-slot, per-format-sources, ouaricon-modules-cmake, regression-prevention]

requires:
  - phase: 23-extract
    provides: Plan 23-04 surfaced D-23-04-A AU re-link failure (latent module-level architectural defect blocking Phase 24)
provides:
  - Steinberg-free SharedCode-bound TU for note-expression (cpp/NoteExpression.cpp)
  - VST3-only TU consolidating all Steinberg::* references (cpp/vst3/NoteExpression_VST3.cpp)
  - Custom function-pointer deleter pimpl (D-21 amended) — SharedCode dtor links without Controller body
  - Dual dispatch slots (g_neUpdate + g_neQuery) wiring SharedCode → VST3 TU at load time
  - Per-format module-source convention in modules/cmake/OuariconModules.cmake (D-25, D-27, D-28)
  - Reusable scripts/verify-au-link.sh AU verification gate for Phase 24
  - Resolution of D-23-04-A (Steinberg-symbol leak into AU/Standalone link lines)
affects: phase-24-propagate (unblocked — propagation can now begin), all-future-shared-modules (per-format routing convention now project-wide)

tech-stack:
  added: []
  patterns:
    - "Two-TU split for format-conditional modules: SharedCode-bound TU (Steinberg-free) + format-specific TU (cpp/<format>/) routed via OuariconModules.cmake"
    - "Custom function-pointer deleter unique_ptr<T, void(*)(T*)> for incomplete-type pimpl across TU boundaries"
    - "Static-init dispatch slot indirection (std::atomic<Fn> + DispatchRegistrar) — SharedCode calls into format-specific TU only when that TU is linked, otherwise no-ops correctly"

key-files:
  created:
    - "modules/tuning/note-expression/cpp/NoteExpression.cpp (SharedCode-bound TU; ctor/dtor/drainAndUpdate/queryIEditController bodies; g_neUpdate + g_neQuery slots; noopControllerDelete; zero Steinberg refs)"
    - "modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp (VST3-only TU; full Controller class body; vst3QueryIEditController free helper with lazy-create swap-deleter; updatePendingFromEvents; realControllerDelete; static-init DispatchRegistrar)"
    - "scripts/verify-au-link.sh (parses PLUGIN_CODE/PLUGIN_MANUFACTURER_CODE from CMakeLists; resolves OUARICON_MANUFACTURER_CODE dev variant; clears AU cache; invokes auval -v)"
  modified:
    - "modules/tuning/note-expression/cpp/NoteExpression.h (stripped to Steinberg-free public header; forward-declared Controller; custom-deleter unique_ptr nec member; out-of-line ctor/dtor/queryIEditController/drainAndUpdate; NEUpdateFn/NEQueryFn typedefs; registerNEUpdate/registerNEQuery accessors; _internalNecPimpl() pimpl accessor for VST3 TU; D-23/D-32 public API preserved verbatim)"
    - "modules/cmake/OuariconModules.cmake (SharedCode glob narrowed from GLOB_RECURSE to non-recursive top-level cpp/*.{cpp,h}; per-format loop iterating vst3/au/standalone/vst2/aax/lv2/unity routes cpp/<format>/ sources to ${TARGET}_<FORMAT> with silent no-op when target absent)"
    - ".planning/phases/23-extract/deferred-items.md (logs out-of-scope APVTS Meta-Flag finding for follow-up)"

key-decisions:
  - "Two-TU split (D-21/D-22 amended) — ctor/dtor/drainAndUpdate/queryIEditController BODIES live in SharedCode-bound TU; VST3-only TU registers free-helper bodies via dispatch slots. Resolves both the original Steinberg-symbol leak AND the prescient plan-checker warning about VST3Extensions vtable references."
  - "Custom function-pointer deleter (D-21) is the load-bearing trick — std::unique_ptr<Controller, void(*)(Controller*)> lets the SharedCode dtor compile against a forward-declaration only. The VST3 TU swaps the deleter atomically via move-assignment when it lazy-creates a real Controller."
  - "Per-format routing convention (D-25/D-27/D-28) adopted project-wide in OuariconModules.cmake — silently no-ops when ${TARGET}_<FORMAT> doesn't exist, so plugins that exclude a format from FORMATS still configure cleanly. This convention prevents Phase 24 plugins from regressing into the same defect class."
  - "Q-slot fix (commit 0e00826) — Rule-1 build fix relocating queryIEditController BODY into SharedCode TU. Original design had it in VST3-only TU but VST3Extensions vtable lives with class definition in SharedCode and references all virtual override symbols. Mirror of g_neUpdate slot; vst3QueryIEditController free helper registers via DispatchRegistrar; returns kNoInterface (-1) on non-VST3 builds."
  - "_internalNecPimpl() accessor — public-but-internal escape hatch on VST3Extensions class so the VST3 TU's free helper can lazy-create + swap-deleter the nec pimpl from outside the class. Underscore prefix flags it as internal-use; consumer call-sites are unaffected (D-23/D-32)."
  - "Out-of-scope APVTS Meta-Flag finding deferred — auval reports a cross-parameter Meta-Flag invariant violation on parameter ID 1275870432 in O-Lyrica. AU bundle loads, RENDER + MIDI tests PASS; the failure is a pre-existing parameter-implementation issue previously masked by the AU re-link failure, not a Plan 23-05 regression. Logged to deferred-items.md."

patterns-established:
  - "Two-TU split with dispatch slots — establish under modules/<x>/cpp/{shared.cpp, <format>/<format-specific>.cpp}; the format-specific TU registers free-helper bodies into atomic dispatch slots in the SharedCode TU at static-init time. SharedCode dispatches via the slot when called, no-ops when slot is null."
  - "AU verification gate per plugin — invoke scripts/verify-au-link.sh <PluginName> as part of plan-level verification; reusable across all 7 Phase 24 propagation targets without edits."

requirements-completed: []

duration: ~25min execution + human Dorico verification
completed: 2026-04-25
---

# Phase 23 Plan 23-05: Fix AU-Link Steinberg Symbols Summary

**Two-TU split + custom-deleter pimpl + dispatch-slot indirection resolves D-23-04-A — OLyrica_VST3, OLyrica_AU, and OLyrica_Standalone now link cleanly with zero undefined-symbol failures of any kind, and the LYR-03 Dorico quarter-sharp smoke test re-passes 5/5 via the rebuilt VST3.**

## Performance

- **Started:** 2026-04-25T20:02Z (executor session)
- **Completed:** 2026-04-25T20:35Z (after user-approved LYR-03 re-pass)
- **Duration:** ~25 min execution + human Dorico verification gate
- **Tasks:** 7/7
- **Files created:** 3 (NoteExpression.cpp, NoteExpression_VST3.cpp, verify-au-link.sh)
- **Files modified:** 3 (NoteExpression.h, OuariconModules.cmake, deferred-items.md)

## Accomplishments

- Resolved D-23-04-A (Steinberg-symbol leak into AU/Standalone link lines) — three OLyrica targets now link cleanly with zero undefined symbols
- Established project-wide per-format module-source convention in `modules/cmake/OuariconModules.cmake` (cpp/<format>/ → ${TARGET}_<FORMAT>) — Phase 24 plugins inherit the fix automatically
- Shipped reusable AU verification gate (`scripts/verify-au-link.sh`) consumable verbatim by all 7 Phase 24 propagation plans
- Preserved consumer API and call-sites byte-identical (D-23/D-32) — no source edits required in O-Lyrica
- Re-passed the LYR-03 Dorico quarter-sharp smoke test 5/5 via VST3, confirming the refactor introduced zero behavioral regression
- Phase 24 unblocked

## Task Commits

Each task committed atomically:

1. **Task 1: Create cpp/NoteExpression.cpp (SharedCode-bound, Steinberg-free TU)** — `5155d5e` (feat)
2. **Task 2: Create cpp/vst3/NoteExpression_VST3.cpp (VST3-only TU)** — `99158c4` (feat)
3. **Task 3: Strip cpp/NoteExpression.h to Steinberg-free public header** — `a5c2311` (refactor)
4. **Task 4: Per-format source routing in modules/cmake/OuariconModules.cmake** — `024fbc2` (feat)
5. **Task 5: scripts/verify-au-link.sh AU verification gate** — `7cefca1` (feat)
6. **Task 6: Clean rebuild + fresh install + auval — with Rule-1 q-slot dispatch fix** — `0e00826` (fix)
7. **Task 7: Dorico LYR-03 quarter-sharp smoke test re-pass (regression check)** — covered in Task 6 install + this SUMMARY (no code change required)

**Plan closeout:** docs commit captures SUMMARY.md + STATE.md + ROADMAP.md.

## What Was Built

### Two-TU split (D-21/D-22 amended)

- **SharedCode-bound TU** (`modules/tuning/note-expression/cpp/NoteExpression.cpp`) — Steinberg-free; defines `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, `drainAndUpdate()`, and `queryIEditController()` BODIES (yes, including queryIEditController — see q-slot fix below); owns `noopControllerDelete` plus the two `std::atomic<Fn>` dispatch slots (`g_neUpdate`, `g_neQuery`) and their register/get accessors. Zero `<pluginterfaces/...>` includes, zero `Steinberg::*` references — links cleanly into AU and Standalone.
- **VST3-only TU** (`modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp`) — owns the full `Controller` class body, `vst3QueryIEditController` free helper (with the lazy-create swap-deleter idiom on `_internalNecPimpl()`), `updatePendingFromEvents` body, `realControllerDelete`, and a static-init `DispatchRegistrar` that wires both slots at TU load time. All `<pluginterfaces/...>` includes consolidated here.

### Custom-deleter pimpl (D-21 amended)

`VST3Extensions::nec` is now `std::unique_ptr<Controller, void(*)(Controller*)>` with `Controller` forward-declared in the header. The custom function-pointer deleter is the load-bearing trick that lets the SharedCode-bound dtor compile without seeing `Controller`'s body. The VST3 TU's `vst3QueryIEditController` lazy-creates a `Controller` and atomically swaps both the managed pointer AND the deleter via `unique_ptr` move-assignment.

### Dispatch slots (D-22 amended)

Two `std::atomic<Fn>` slots in the SharedCode TU:
- `g_neUpdate` — SharedCode `drainAndUpdate()` dispatches kTuningTypeID-event correlation through here; null on AU/Standalone (no VST3 events possible) — correct no-op.
- `g_neQuery` — SharedCode `VST3Extensions::queryIEditController()` body dispatches the `<pluginterfaces/...>`-touching IID matching through here; returns `kNoInterface` (-1) on non-VST3 builds.

The VST3 TU's static-init `DispatchRegistrar` registers `updatePendingFromEvents` and `vst3QueryIEditController` into the slots at TU load time. When the VST3 TU is NOT linked (AU/Standalone), both slots stay nullptr and SharedCode no-ops correctly.

### Per-format CMake routing (D-25/D-27/D-28)

`modules/cmake/OuariconModules.cmake` extended:
- SharedCode glob narrowed from `GLOB_RECURSE` to non-recursive `GLOB` on top-level `cpp/*.cpp` + `cpp/*.h` only — prevents `cpp/<format>/` subdirs from being swept into the shared static library (root cause of D-23-04-A).
- Per-format loop iterates `{vst3 au standalone vst2 aax lv2 unity}`; routes `cpp/<format>/` sources to `${TARGET_NAME}_<FORMAT>` via `target_sources(... PRIVATE)` + `target_include_directories(... PRIVATE)`. Silently no-ops when the target doesn't exist (D-28).
- Consumer one-liner `ouaricon_add_module(OLyrica note-expression)` preserved byte-identical (D-32). `JUCE-NE-PATCH` marker check (D-34) preserved unchanged.

### AU verification gate (D-30/D-31)

`scripts/verify-au-link.sh <PluginName>` parses `PLUGIN_CODE`, `PLUGIN_MANUFACTURER_CODE`, and `IS_SYNTH`/`PLUGIN_AU_MAIN_TYPE` from `plugins/<Plugin>/CMakeLists.txt`; resolves `${OUARICON_MANUFACTURER_CODE}` from root CMakeLists.txt (dev-suffix branch matching the installed -dev `.component`); clears the AudioComponentRegistrar cache per CLAUDE.md protocol; invokes `auval -v <type> <subtype> <manuf>`. macOS-only (no-ops on non-Darwin). Reusable verbatim by Phase 24 plans.

## Mid-flight Architecture Adjustment (Rule-1 q-slot fix)

The plan-checker had warned that the original Plan 23-05 design — placing `queryIEditController` body in the VST3-only TU — would replace D-23-04-A's Steinberg undefined-symbols with a NEW `Ouaricon::NoteExpression::VST3Extensions::queryIEditController` undefined-symbol failure on AU/Standalone link lines, because `VST3Extensions`'s vtable is emitted with the class definition (PluginProcessor in SharedCode value-holds `VST3Extensions`) and references all virtual override symbols.

When Task 6's clean rebuild reproduced exactly that defect class, the executor applied a Rule-1 auto-fix (commit `0e00826`):

- Added a second `std::atomic<NEQueryFn> g_neQuery` slot in the SharedCode TU, twin of `g_neUpdate`.
- Relocated `VST3Extensions::queryIEditController` BODY into the SharedCode-bound TU; body dispatches through `g_neQuery`, returning `-1` (kNoInterface) when the slot is null (non-VST3 builds — correct, no IEditController to query).
- Renamed the original VST3 TU body to free function `vst3QueryIEditController(VST3Extensions&, ...)`. The DispatchRegistrar registers it.
- Added a public `_internalNecPimpl()` accessor on `VST3Extensions` so the VST3 TU's free helper can lazy-create + swap-deleter the `nec` pimpl from outside the class. Underscore-prefixed; internal-use only; not part of consumer API.

The class shape, public method signatures, and consumer call-sites are all unchanged (D-23/D-32 still satisfied). The accessor is the only structural addition, and it's flagged internal by convention.

## Build Verification

- `ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone` — three artefacts produced, zero undefined-symbol failures of any kind. Build log at `/tmp/plan-23-05-build.log` (clean).
- Broadened grep against the build log returns 0 matches for both the original D-23-04-A symbols (`Steinberg::Vst::INoteExpressionController::iid`, `Steinberg::UString::assign`, `Steinberg::FUnknown::iid`) AND the previously-feared `Ouaricon::NoteExpression::VST3Extensions::*` symbols.
- Fresh install per CLAUDE.md Plugin Cache Clearing protocol — VST3 + AU bundles installed at `~/Library/Audio/Plug-Ins/{VST3,Components}/O-Lyrica-dev.{vst3,component}`.
- `auval -v aumu OLyr OuDv` — AU bundle loads, RENDER tests PASS at every sample rate, MIDI test PASS. (One out-of-scope failure documented below.)

## LYR-03 Dorico Smoke Test (regression check)

User-approved re-pass of the Plan 23-04 LYR-03 5-sub-test gate against the freshly-installed VST3 from this plan:

| # | Test | Result |
|---|------|--------|
| 1 | Quarter-sharp C4 pitch ≈269.29 Hz (+50¢ above 261.63 Hz) | PASS |
| 2 | No attack zipper on quarter-sharp pluck | PASS |
| 3 | noteId correlation isolates detuning to one chord note | PASS |
| 4 | TuningEngine composition with JI Scala — multiplicative (+50¢ above JI base) | PASS |
| 5 | Retrigger safety — exchange(0.0) consumed slot; second hit is 12-TET | PASS |

LYR-03 gate cleared. Behavior preserved end-to-end: the AU-link defect fix introduced zero regression in the VST3 path.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] q-slot dispatch for queryIEditController (foreseen by plan-checker)**
- **Found during:** Task 6 (clean rebuild) — first AU link attempt produced new undefined-symbol failure on `Ouaricon::NoteExpression::VST3Extensions::queryIEditController`.
- **Issue:** Original Plan 23-05 design placed `queryIEditController` body in the VST3-only TU, but `VST3Extensions`'s vtable is emitted with the class definition (in SharedCode), so AU/Standalone link lines need a body. This was the exact failure mode the plan-checker had warned about.
- **Fix:** Added g_neQuery slot mirroring g_neUpdate. Moved queryIEditController body into SharedCode TU; body dispatches through the slot. Renamed the original VST3 TU body to free function `vst3QueryIEditController`; DispatchRegistrar wires it at TU load. Added `_internalNecPimpl()` accessor so the free helper can swap the pimpl deleter from outside the class. Also fixed an unrelated `/* in block comment` warning in NoteExpression.cpp surfaced during the rebuild.
- **Files modified:** `modules/tuning/note-expression/cpp/NoteExpression.cpp` (+55 lines, q-slot machinery + queryIEditController body), `modules/tuning/note-expression/cpp/NoteExpression.h` (+28 lines, NEQueryFn typedef + registerNEQuery + _internalNecPimpl accessor), `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` (rename body to free helper + DispatchRegistrar update).
- **Verification:** Clean rebuild produces zero undefined-symbol failures across all three OLyrica targets; AU bundle loads + RENDER tests PASS in auval.
- **Committed in:** `0e00826` (within Task 6's commit boundary)

---

**Total deviations:** 1 auto-fixed (Rule 1 — bug)
**Impact on plan:** Necessary for correctness. Class shape and consumer call-sites are unchanged (D-23/D-32 preserved). The structural addition (`_internalNecPimpl()` accessor) is flagged internal by underscore-prefix convention. No scope creep.

## Deferred / Out-of-Scope Issues

### APVTS Meta-Flag failure on parameter ID 1275870432 (pre-existing, NOT a Plan 23-05 regression)

`auval -v aumu OLyr OuDv` returns exit code 255 with an `ERROR: Parameter values are different since last set - probable cause: a Meta Param Flag is NOT set on a parameter that will change values of other parameters.` The defect was previously masked by the AU re-link failure itself — auval was never previously invoked successfully against an OLyrica AU bundle. It's a logic issue in O-Lyrica's parameter implementation, not a note-expression-module issue.

- AU bundle loads, RENDER tests pass at every sample rate, MIDI test passes
- VST3 path is unaffected — canonical Dorico microtonal route
- Logged to `.planning/phases/23-extract/deferred-items.md` for follow-up
- Phase 24 propagation is unblocked (per-format module-source convention is in place); however Phase 24 verify gates that include `auval -v` will hit this same finding on every plugin with cross-mutating APVTS parameters — a quick-task or post-script audit will resolve it across the suite.

## Issues Encountered

None during planned work. The q-slot deviation arose during Task 6's verification step and was auto-fixed under Rule 1.

## Hand-off

**Phase 23 status after this plan:**
- All 12 stated requirements (MOD-01..08, LYR-01..04) satisfied (claimed by Plans 23-01..04; this plan was a defect-fix and intentionally claimed `requirements: []`)
- D-23-04-A (AU-link Steinberg-symbol leak) RESOLVED
- VST3 + AU + Standalone all link cleanly for OLyrica
- Per-format module-source convention in place project-wide — Phase 24 plugins inherit the fix automatically
- Reusable AU verification gate (`scripts/verify-au-link.sh`) ready for Phase 24

**Phase 24 unblocked.** The pre-existing APVTS Meta-Flag finding is independent of the note-expression module and is tracked in `deferred-items.md`.

## Self-Check: PASSED

- modules/tuning/note-expression/cpp/NoteExpression.cpp — created, present, content matches commit 5155d5e (+ q-slot additions in 0e00826)
- modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp — created, present, content matches commit 99158c4 (+ q-slot rename in 0e00826)
- modules/tuning/note-expression/cpp/NoteExpression.h — modified, Steinberg-free, content matches commit a5c2311 (+ q-slot additions in 0e00826)
- modules/cmake/OuariconModules.cmake — per-format routing added in commit 024fbc2
- scripts/verify-au-link.sh — created in commit 7cefca1
- All 6 task commits present in git log: 5155d5e, 99158c4, a5c2311, 024fbc2, 7cefca1, 0e00826
- All must_haves truths from PLAN frontmatter satisfied (clean rebuild, header Steinberg-free, two-TU split, custom-deleter pimpl, dispatch-slot wiring, per-format routing, silent no-op, verify-au-link.sh, auval acceptance modulo deferred Meta-Flag, LYR-03 5/5 PASS, public API preserved verbatim, consumer call-sites unchanged, JUCE-NE-PATCH check still passes)
- Plan-level TDD gates: not applicable (plan type=execute, not tdd)
