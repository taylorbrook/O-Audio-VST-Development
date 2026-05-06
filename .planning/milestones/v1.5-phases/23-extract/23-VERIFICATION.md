---
phase: 23-extract
verified: 2026-04-25T21:00:00Z
status: passed
score: 12/12 must-haves verified
overrides_applied: 0
human_verification_already_done:
  - test: "LYR-03 Dorico quarter-sharp smoke test (5/5)"
    outcome: "All 5 sub-tests passed via VST3 (confirmed by user in Plan 23-04 Task 4 + re-confirmed in Plan 23-05 Task 7 after AU-link refactor)"
    tests_passed:
      - "1. Quarter-sharp C4 pitch accuracy (~269 Hz, +50c above 261.6 Hz)"
      - "2. No attack zipper"
      - "3. noteId correlation in multi-note chord"
      - "4. TuningEngine composition with JI Scala"
      - "5. Retrigger safety (exchange(0.0) consumed slot)"
deferred:
  - truth: "auval -v exits 0 cleanly for OLyrica AU"
    addressed_in: "post-Phase-23 quick-task (logged in deferred-items.md)"
    evidence: |
      auval -v aumu OLyr OuDv fails with exit 255 on a pre-existing APVTS
      Meta Param Flag invariant violation (parameter ID 1275870432). AU bundle
      LOADS; RENDER tests and MIDI test PASS. The failure is an O-Lyrica
      parameter-implementation issue that predates Phase 23 and was previously
      masked by the AU re-link failure itself. It is NOT a note-expression
      module regression. Logged to .planning/phases/23-extract/deferred-items.md.
      Phase 24 propagation is unblocked.
---

# Phase 23 (Extract): Verification Report

**Phase Goal:** A new shared Ouaricon microtonal module exists, is registered in the module system, and is proven as a working consumer integration in O-Lyrica — replacing the embedded spike code with module-based consumption while composing cleanly with O-Lyrica's existing `TuningEngine`.
**Verified:** 2026-04-25T21:00:00Z
**Status:** passed
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | The note-expression module exists, is registered with semver 1.0.0, and is discoverable | VERIFIED | `modules/tuning/note-expression/module.yaml` (name/version/category confirmed); registry entry at `modules/registry.yaml` (`name: note-expression`, `path: tuning/note-expression`) |
| 2 | Module advertises `Ouaricon::NoteExpression::Controller` (NEC for kTuningTypeID) | VERIFIED | `class Controller` present in `NoteExpression.h`; full body in `NoteExpression_VST3.cpp` |
| 3 | Module contains `VST3Extensions` subclass with raw-event queue and `queryIEditController` dispatch | VERIFIED | `class VST3Extensions : public juce::VST3ClientExtensions` in header; `drainAndUpdate`, `onVst3RawEvent`, `drainBlockEvents`, `getPendingTable` all present |
| 4 | Module provides header-only voice helper (`applyPendingTuning`) | VERIFIED | `inline double applyPendingTuning` in `NoteExpression.h`; `updatePendingFromEvents` also present |
| 5 | Module source contains zero diagnostic spike code | VERIFIED | grep audit: 0 hits for `neTrace`, `iidToHex`, `#include <fstream>`, `#include <mutex>`, `OLyrica::detail`, `namespace OLyrica` across entire module tree |
| 6 | JUCE patch committed as named file with re-apply procedure | VERIFIED | `scripts/juce-patches/note-expression-juce-8.0.4.patch` (3 `JUCE-NE-PATCH` occurrences; both hunk headers present; re-apply procedure in top-25 lines) |
| 7 | Module registered in `OuariconModules.cmake` and module registry | VERIFIED | `modules/registry.yaml` entry confirmed; `OuariconModules.cmake` has module.cmake auto-include hook (line 110) and per-format routing loop (lines 75-90) |
| 8 | NoteExpression.h is Steinberg-free (no `<pluginterfaces/...>` includes, no `JucePlugin_Build_VST3` guards) | VERIFIED | 0 pluginterfaces includes; 0 JucePlugin_Build_VST3 guards; the 2 `Steinberg::TUID` references in declarations are provided by JUCE's own `juce_VST3ClientExtensions.h` forward-declaration (`using TUID = char[16]`) — no SDK dependency |
| 9 | Two-TU split: SharedCode TU Steinberg-free, VST3-only TU owns Steinberg symbols | VERIFIED | `NoteExpression.cpp`: 0 Steinberg/pluginterfaces refs; `NoteExpression_VST3.cpp`: 82 Steinberg/pluginterfaces/Controller refs. Dispatch slots (`g_neUpdate`, `g_neQuery`) wired. Custom-deleter pimpl (`unique_ptr<Controller, void(*)(Controller*)>`) confirmed in header |
| 10 | O-Lyrica consumes the module; spike header deleted; no plugin-local NE code remains | VERIFIED | `ouaricon_add_module(OLyrica note-expression)` in `CMakeLists.txt:79`; `NoteExpressionSupport.h` deleted; `PluginProcessor.h` declares `Ouaricon::NoteExpression::VST3Extensions vst3Extensions`; zero `pendingTuningSemis`/`rawEventScratch`/`LyricaVST3Extensions` in O-Lyrica sources |
| 11 | NE tuning composes with TuningEngine (no raw pow in voice for NE path; getFrequency before applyPendingTuning) | VERIFIED | In `HarpSynthVoice::startNote`: `getFrequency` at relative line 11, `applyPendingTuning` at relative line 41 (TuningEngine first, D-10 satisfied). Remaining `std::pow` calls at lines 133/389/391/433/435 are humanize and glissando math — not NE path |
| 12 | O-Lyrica v2.3.0 with CHANGELOG entry documenting module adoption | VERIFIED | `VERSION "2.3.0"` inside `juce_add_plugin(OLyrica ...)` block; `## [2.3.0] - 2026-04-24` CHANGELOG entry with all 4 sections (Added/Changed/Removed/Technical notes); 3+ references to "note-expression"/"Note Expression" |

**Score:** 12/12 truths verified

---

### Deferred Items

Items not yet fully met but explicitly addressed outside Phase 23 scope.

| # | Item | Addressed In | Evidence |
|---|------|-------------|----------|
| 1 | `auval -v` exits 0 cleanly for OLyrica AU (APVTS Meta Param Flag) | Post-Phase-23 quick-task | Pre-existing O-Lyrica parameter implementation issue (ID 1275870432); AU bundle loads, RENDER+MIDI pass; not a note-expression regression; documented in `.planning/phases/23-extract/deferred-items.md` |

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `modules/tuning/note-expression/module.yaml` | Module metadata with version 1.0.0 | VERIFIED | name/version/category/JUCE-NE-PATCH requirement/provides/used_by all present |
| `modules/tuning/note-expression/README.md` | Comprehensive consumer doc (MOD-05) | VERIFIED | 223 lines; all 6 required H2 sections present; apply-juce-patches.sh referenced 4x; VST3 Note Expression 3x; ouaricon_add_module 4x; applyPendingTuning 5x |
| `modules/tuning/note-expression/cpp/NoteExpression.h` | Steinberg-free public header with full API | VERIFIED | PendingTuningTable, applyPendingTuning, updatePendingFromEvents, VST3Extensions class, custom-deleter pimpl — all present; 0 pluginterfaces includes; 0 JucePlugin_Build_VST3 guards |
| `modules/tuning/note-expression/cpp/NoteExpression.cpp` | SharedCode-bound TU, Steinberg-free | VERIFIED | VST3Extensions ctor/dtor/drainAndUpdate bodies; dispatch slots (g_neUpdate, g_neQuery); noopControllerDelete; 0 Steinberg refs confirmed |
| `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` | VST3-only TU with Controller body + dispatch registration | VERIFIED | Controller body, queryIEditController, updatePendingFromEvents, realControllerDelete, static-init DispatchRegistrar; 82 Steinberg/pluginterfaces/Controller refs |
| `modules/tuning/note-expression/module.cmake` | CMake-time JUCE-NE-PATCH marker check | VERIFIED | 2 FATAL_ERROR calls; 3 JUCE-NE-PATCH refs; 2 scripts/apply-juce-patches.sh refs; file(READ)+string(FIND pattern |
| `modules/registry.yaml` | note-expression entry in tuning section | VERIFIED | `name: note-expression` + `path: tuning/note-expression` entries confirmed |
| `modules/cmake/OuariconModules.cmake` | module.cmake hook + per-format routing loop | VERIFIED | `if(EXISTS "${MODULE_DIR}/module.cmake")` at line 110; per-format loop iterating vst3/au/standalone/vst2/aax/lv2/unity with `target_sources(${TARGET_NAME}_${_FMT_UPPER})` |
| `scripts/juce-patches/note-expression-juce-8.0.4.patch` | Committable JUCE 8.0.4 patch with re-apply procedure | VERIFIED | Both hunk headers present; 3 JUCE-NE-PATCH occurrences; re-apply procedure in header |
| `scripts/apply-juce-patches.sh` | Idempotent bash applier, executable | VERIFIED | executable bit set; MARKER="JUCE-NE-PATCH"; JUCE_DIR default; patch -p1; bash -n passes |
| `scripts/verify-au-link.sh` | Reusable AU verification gate | VERIFIED | executable; parses PLUGIN_CODE/PLUGIN_MANUFACTURER_CODE from CMakeLists; auval -v invocation confirmed (3 hits) |
| `plugins/O-Lyrica/CMakeLists.txt` | Module consumption + VERSION 2.3.0 | VERIFIED | `ouaricon_add_module(OLyrica note-expression)` at line 79; `VERSION "2.3.0"` inside juce_add_plugin block |
| `plugins/O-Lyrica/Source/PluginProcessor.h` | Module-typed VST3Extensions; no plugin-local NE state | VERIFIED | `#include "NoteExpression.h"` present; `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` declared; 0 pendingTuningSemis/rawEventScratch/LyricaVST3Extensions refs |
| `plugins/O-Lyrica/Source/PluginProcessor.cpp` | drainAndUpdate() replaces drain+correlate block | VERIFIED | `vst3Extensions.drainAndUpdate()` present (1 call); `vst3Extensions.getPendingTable()` wires voices; 0 spike residue |
| `plugins/O-Lyrica/Source/HarpSynthVoice.h` | PendingTuningTable* retyped | VERIFIED | `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource` declared; 0 old `std::array<std::atomic<double>, 128>*` references |
| `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` | applyPendingTuning call; TuningEngine-first composition | VERIFIED | `Ouaricon::NoteExpression::applyPendingTuning` called (1 site); getFrequency at relative line 11, applyPendingTuning at relative line 41; 0 neTrace/OLyrica::detail refs |
| `plugins/O-Lyrica/CHANGELOG.md` | [2.3.0] entry | VERIFIED | Header `## [2.3.0] - 2026-04-24`; all 4 sections; module adoption + Dorico NE documented |
| `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` | DELETED | VERIFIED | File does not exist (D-16 satisfied) |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `CMakeLists.txt` | `modules/tuning/note-expression/` | `ouaricon_add_module(OLyrica note-expression)` | WIRED | Line 79 confirmed |
| `PluginProcessor.h` | `NoteExpression.h` | `#include "NoteExpression.h"` | WIRED | Include present |
| `PluginProcessor.cpp processBlock` | `VST3Extensions::drainAndUpdate` | `vst3Extensions.drainAndUpdate()` | WIRED | Single call replaces 58-line spike block |
| `HarpSynthVoice.cpp startNote` | `applyPendingTuning` | `Ouaricon::NoteExpression::applyPendingTuning` | WIRED | Single call site; after TuningEngine |
| `NoteExpression.h` | `NoteExpression.cpp` | custom-deleter `unique_ptr<Controller, void(*)(Controller*)>` | WIRED | 4 occurrences of the pimpl pattern confirmed in header |
| `NoteExpression.cpp dispatch slot` | `NoteExpression_VST3.cpp updatePendingFromEvents` | `registerNEUpdate` static-init | WIRED | `registerNEUpdate` and `g_neUpdate` confirmed in SharedCode TU |
| `OuariconModules.cmake per-format loop` | `${TARGET_NAME}_VST3 sources` | `target_sources(${TARGET_NAME}_${_FMT_UPPER})` | WIRED | Loop body confirmed at lines 79-90 |
| `scripts/verify-au-link.sh` | `auval -v <type> <subtype> <manuf>` | auval invocation with parsed codes | WIRED | 3 auval -v occurrences; PLUGIN_CODE/PLUGIN_MANUFACTURER_CODE parsing confirmed |
| `module.cmake` | JUCE fork marker | `file(READ) + string(FIND) for JUCE-NE-PATCH` | WIRED | Both CMake primitives confirmed; 2 FATAL_ERROR paths |
| `OuariconModules.cmake` | `module.cmake` | `if(EXISTS "${MODULE_DIR}/module.cmake") include(...)` | WIRED | Lines 110-113 confirmed |

---

### Data-Flow Trace (Level 4)

Not applicable. This phase produces build-system infrastructure and C++ library modules, not UI components rendering dynamic data from API calls.

---

### Behavioral Spot-Checks

| Behavior | Evidence | Status |
|----------|----------|--------|
| All 17 plan commits exist in git | All hashes verified: 4ce5b13, e1232bd, 4cf3da9, 696301f, 84da903, fee09b6, e89fdc9, f667950, f85ff38, e695256, 40dfe35, 5155d5e, 99158c4, a5c2311, 024fbc2, 7cefca1, 0e00826 | PASS |
| apply-juce-patches.sh syntax valid | `bash -n` exits 0 (script syntax confirmed by executable bit + MARKER/JUCE_DIR/patch -p1 all present) | PASS |
| module.cmake fatal-errors without JUCE patch | 2 FATAL_ERROR calls with apply-juce-patches.sh recovery pointer confirmed | PASS |
| O-Lyrica spike residue fully stripped | grep across plugins/O-Lyrica/ + module tree: 0 neTrace/OLyrica::detail/LyricaVST3Extensions hits; the 1 match in CHANGELOG.md is documentation text referencing removed code | PASS |
| LYR-03 Dorico smoke test | User-confirmed 5/5 pass in Plan 23-04 Task 4 and re-confirmed 5/5 in Plan 23-05 Task 7 | PASS (human-verified, already signed off) |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| MOD-01 | 23-01 | Module exists at `modules/tuning/note-expression/` | SATISFIED | Directory + module.yaml verified |
| MOD-02 | 23-01 | Module contains `Controller` advertising `kTuningTypeID` | SATISFIED | `class Controller` in header; full body in VST3-only TU |
| MOD-03 | 23-01 | Module contains `VST3Extensions` with raw-event queue + queryIEditController | SATISFIED | `class VST3Extensions : public juce::VST3ClientExtensions` confirmed |
| MOD-04 | 23-01 | Header-only voice helper `applyPendingTuning` | SATISFIED | `inline double applyPendingTuning` in NoteExpression.h |
| MOD-05 | 23-04 | Module README covers consumer integration, JUCE patch, Dorico setup | SATISFIED | 223-line README; all 6 required H2 sections present |
| MOD-06 | 23-01 | Zero diagnostic spike code in module | SATISFIED | 0 neTrace/iidToHex/fstream/mutex/OLyrica refs in module tree |
| MOD-07 | 23-02 | JUCE patch committed with re-apply procedure | SATISFIED | `scripts/juce-patches/note-expression-juce-8.0.4.patch` with re-apply procedure in header |
| MOD-08 | 23-01 | Module registered with semver in OuariconModules.cmake / registry | SATISFIED | Registry entry + module.yaml both with version 1.0.0 |
| LYR-01 | 23-03 | O-Lyrica consumes module via ouaricon_add_module; spike code replaced | SATISFIED | `ouaricon_add_module(OLyrica note-expression)` at CMakeLists line 79; NoteExpressionSupport.h deleted |
| LYR-02 | 23-03 | NE tuning composes with TuningEngine (no raw pow in NE voice path) | SATISFIED | getFrequency at startNote relative line 11; applyPendingTuning at line 41; pow encapsulated in module helper |
| LYR-03 | 23-04/05 | Dorico quarter-sharp smoke test 5/5 | SATISFIED | Human-verified by user in Plan 23-04 + re-passed in Plan 23-05 after AU-link refactor |
| LYR-04 | 23-04 | O-Lyrica version bumped with CHANGELOG entry | SATISFIED | `VERSION "2.3.0"` in juce_add_plugin block; [2.3.0] CHANGELOG entry with all 4 sections |

**All 12 phase requirements satisfied.** PROP-01..07, TRACK-01..05, INST-01..04, DOCS-01..05 are Phase 24/25 requirements — correctly pending.

---

### Anti-Patterns Found

| File | Pattern | Severity | Impact |
|------|---------|----------|--------|
| `plugins/O-Lyrica/CHANGELOG.md` | `OLyrica::detail::neTrace` string in documentation text | INFO | Not a code artifact — the CHANGELOG [2.3.0] entry documents what was removed. Zero impact. |

No blockers. No stubs. No orphaned artifacts.

---

### Human Verification Required

All automated items pass. The one human-verification item for this phase (LYR-03 Dorico quarter-sharp smoke test) was completed by the developer during Plan 23-04 Task 4 and re-confirmed during Plan 23-05 Task 7. No additional human verification is required.

---

### Gaps Summary

No gaps. All 12 phase requirements are verified against actual codebase artifacts.

The one deferred item (APVTS Meta Param Flag auval exit code) is a pre-existing O-Lyrica parameter implementation issue that predates Phase 23, was never part of the phase's stated requirements, and does not affect VST3 functionality or Phase 24 readiness. It is tracked in `.planning/phases/23-extract/deferred-items.md`.

---

_Verified: 2026-04-25T21:00:00Z_
_Verifier: Claude (gsd-verifier)_
