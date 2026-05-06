# Phase 23: Extract — Context

**Gathered:** 2026-04-24
**Updated:** 2026-04-25 (Plan 23-05 added — AU-link defect fix via per-format module sources)
**Updated:** 2026-04-25 (D-21/D-22 amended: plan-checker caught that placing `VST3Extensions` ctor/dtor + `drainAndUpdate` in the VST3-only TU produces a NEW AU/Standalone undefined-symbol failure — same class as D-23-04-A. Revised to a two-TU split: SharedCode-bound `cpp/NoteExpression.cpp` for ctor/dtor/drainAndUpdate using a custom function-pointer deleter for the pimpl + dispatch slot for NE correlation; VST3-only `cpp/vst3/NoteExpression_VST3.cpp` for `Controller` body, `queryIEditController`, `updatePendingFromEvents`, and the static-init that registers the dispatch.)
**Status:** Plans 23-01..04 complete; Plan 23-05 ready for re-planning against amended D-21/D-22.

<domain>
## Phase Boundary

A new shared Ouaricon microtonal module (`modules/tuning/note-expression`) is extracted from the cleaned spike code (O-Lyrica spikes 001–003), registered in the module system, and proven end-to-end on O-Lyrica as the reference consumer. O-Lyrica consumes the module via `/module-add`, composes NE tuning with its existing `TuningEngine`, and passes the Dorico quarter-sharp smoke test. The local JUCE patch is promoted from spike-findings markdown hunks into a real `.patch` file with an idempotent apply script and a CMake-time verification check.

**Plan 23-05 addendum (added 2026-04-25):** restore clean OLyrica_AU + OLyrica_Standalone link by relocating Steinberg-touching code out of SharedCode into a VST3-target-only translation unit, and establish a project-wide per-format module-source convention (with an `OuariconModules.cmake` extension) so the same defect cannot recur during Phase 24 propagation.

**In scope:** module creation, O-Lyrica refactor, JUCE patch tooling, O-Lyrica version bump + CHANGELOG, AU-link defect fix, per-format module-source pattern, `OuariconModules.cmake` extension.
**Out of scope** (owned by later phases): 7 other pitched plugins (Phase 24), `.doricoexpmap` bundling in installers (Phase 25), website-ready DOCS-01..05 (Phase 25), Windows verification (FUT-01), MTS-ESP (FUT-03).

</domain>

<decisions>
## Implementation Decisions

### Module Identity & Placement

- **D-01:** Module path = `modules/tuning/note-expression`. Lives in existing `tuning/` category alongside `scala-tuning-engine` — semantically accurate (NE is a per-note tuning mechanism) and avoids adding an unused `dsp/` category. Overrides the REQUIREMENTS.md candidate `dsp/note-expression` (which was provisional).
- **D-02:** Registry name = `note-expression`. Scoped by category path, matches existing module name conventions (`preset-manager`, `vu-meter`).
- **D-03:** Starting semver = `1.0.0`. Spike-validated + shipping on O-Lyrica this phase + adopted by 7 plugins in Phase 24 = stable public API from day one. Matches registry convention.

### Module API Surface

- **D-04:** Public namespace = `Ouaricon::NoteExpression`. Modern nested-namespace convention; leaves room for sibling `Ouaricon::*` modules.
- **D-05:** Public classes:
  - `Ouaricon::NoteExpression::Controller` — was `TuningNoteExpressionController` in the spike; advertises `kTuningTypeID` NE.
  - `Ouaricon::NoteExpression::VST3Extensions` — was `LyricaVST3Extensions` in the spike; subclass of `juce::VST3ClientExtensions`. **Owns** the 128-slot pending tuning table.
- **D-06:** Public type: `Ouaricon::NoteExpression::PendingTuningTable = std::array<std::atomic<double>, 128>`.
- **D-07:** Voice helper signature (header-only, MOD-04):
  ```cpp
  double Ouaricon::NoteExpression::applyPendingTuning (
      PendingTuningTable& table, int midiNoteNumber, double currentFrequency);
  ```
  Returns new frequency. `pow(2, semis/12)` is encapsulated inside the helper — voice code never calls `pow` directly. Consumes the pending slot via `exchange(0.0)` so retriggered notes don't inherit stale offsets.
- **D-08:** Drain + correlate helper:
  ```cpp
  void Ouaricon::NoteExpression::updatePendingFromEvents (
      const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>& events,
      PendingTuningTable& table);
  ```
  Implements the two-pass logic: (1) build `std::map<noteId, midi pitch>` from NoteOns; (2) for each `kTuningTypeID` NE, look up pitch by `noteId`, compute `semitones = 240.0 * (value - 0.5)`, store into `table[pitch]`. Moves this logic out of the plugin's `processBlock` — plugin just calls it after `drainBlockEvents`.
- **D-09:** Pending-table ownership lives on `VST3Extensions` inside the module, not on the plugin's `PluginProcessor`. Processor calls `m_extensions.drainBlockEvents(events)` then `NoteExpression::updatePendingFromEvents(events, m_extensions.getPendingTable())`. Voices get the pointer via `&m_extensions.getPendingTable()`. Phase 24 plugins don't need to re-declare the 128-slot table in their own processors.

### TuningEngine Composition (O-Lyrica-specific, pattern generalizes)

- **D-10:** Composition order: voice computes base frequency via `TuningEngine.getFrequency(midi)` **first**, then passes that frequency through `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)`. NE delta is always in 12-TET semitones (Dorico computes the offset against the EDO12 neighbor) — multiplicative compose is mathematically correct for any base tuning. Satisfies LYR-02 "no raw pow bypass": pow is inside the module helper, not in voice code.
- **D-11:** `note-expression` module has **no dependency** on `scala-tuning-engine`. Composition happens at the call site (in the voice), not inside either module. Preserves portability to plugins that don't use `scala-tuning-engine`.

### JUCE Patch Management

- **D-12:** Patch format = unified `.patch` file (generated once via `git diff` against a temp-init'd JUCE tree), compatible with `git apply` / `patch -p1`. Upgrades the spike's markdown-hunks approach.
- **D-13:** Patch location = `scripts/juce-patches/note-expression-juce-8.0.4.patch`. Matches MOD-07 spec ("named patch file in `scripts/`"). Filename encodes target JUCE version for disambiguation across future upgrades.
- **D-14:** Apply script = `scripts/apply-juce-patches.sh`. Idempotent: greps target JUCE tree for the `JUCE-NE-PATCH` marker first, skips application if already present. Fails with actionable error if JUCE path (`/Users/taylorbrook/JUCE` per CLAUDE.md memory) is missing.
- **D-15:** Verification = CMake-time marker check. On every `cmake configure`, a CMake snippet greps the JUCE tree for `JUCE-NE-PATCH` markers; fatal error if missing, with message pointing to `scripts/apply-juce-patches.sh`. Fails loud + fails fast — prevents the silent regression where a JUCE upgrade leaves plugins building but microtones quietly broken. Enforcement lives at the module level (in the module's CMake include) so it only gates plugins that actually consume `note-expression`.

### O-Lyrica Refactor Shape

- **D-16:** `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` is **deleted entirely**. No plugin-local shim. O-Lyrica sets the clean reference pattern for Phase 24.
- **D-17:** O-Lyrica consumes the module via `/module-add note-expression`. `PluginProcessor` instantiates `Ouaricon::NoteExpression::VST3Extensions m_extensions`; `getVST3ClientExtensions()` returns `&m_extensions`. Voices `#include` the module header and call `applyPendingTuning` after their `TuningEngine` frequency query.
- **D-18:** Diagnostic spike code stripped per MOD-06: zero `OLyrica::detail::neTrace(...)` call sites remain in O-Lyrica sources; `detail::neTrace` / `detail::iidToHex` helpers deleted; `#include <fstream>` removed from any O-Lyrica or module source. Grep-verify as acceptance.
- **D-19:** O-Lyrica version bump = **2.2.2 → 2.3.0** (minor). New user-visible feature (Dorico microtonal playback via VST3 Note Expression). CHANGELOG entry documents "adds VST3 Note Expression microtonal support for Dorico" + shared-module adoption.

### Plan 23-05: AU-Link Defect Fix (added 2026-04-25)

**Background.** Plan 23-04 surfaced D-23-04-A: `OLyrica_AU` re-link failed with `Undefined symbols: Steinberg::Vst::INoteExpressionController::iid, Steinberg::UString::assign, Steinberg::FUnknown::iid`. Root cause: the `#if JucePlugin_Build_VST3` guards added in Plan 23-03 (`f85ff38`) are evaluated per translation unit. SharedCode (`libO-Lyrica-dev_SharedCode.a`) compiles with `JucePlugin_Build_VST3=1` because the plugin's FORMATS list includes VST3 — so the guarded Steinberg references leak into SharedCode IR. The AU/Standalone link lines do NOT link `pluginterfaces`, so the symbols go unresolved. VST3 path was unaffected (Dorico smoke test 5/5 PASS); AU/Standalone broken at link time. Architectural defect; fix must be at the module level, not per-plugin.

#### Code Split (header → .cpp)

- **D-20:** `Controller` class definition + `VST3Extensions::queryIEditController` body relocate from `modules/tuning/note-expression/cpp/NoteExpression.h` into a new `modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp`. Header keeps a forward-declaration of `Controller` and an out-of-line declaration of `queryIEditController`. All `<pluginterfaces/...>` includes leave the header.
- **D-21:** `VST3Extensions::nec` member becomes a pimpl owning a forward-declared `Controller`, using a **custom function-pointer deleter** so the destructor is linkable from non-VST3 translation units without seeing `Controller`'s full type. Concretely: `using ControllerDeleter = void (*)(Controller*); std::unique_ptr<Controller, ControllerDeleter> nec { nullptr, &noopControllerDelete };`. The default `noopControllerDelete` is defined in the SharedCode-bound TU and is a no-op (only ever invoked when `nec` is null). The VST3-only TU defines `realControllerDelete` (which actually calls `delete` on the complete-type `Controller*`); the static-init dispatch path swaps the deleter to `realControllerDelete` when it constructs the real Controller (D-22 dispatch). `VST3Extensions::VST3Extensions()` and `~VST3Extensions()` are declared in the header and **defined out-of-line in the SharedCode-bound `cpp/NoteExpression.cpp`** (NOT the VST3-only TU) — they must link into every per-format target because PluginProcessor (in SharedCode) holds `VST3Extensions` as a value member and constructs/destructs it from every format's link line. The custom deleter pattern is what allows the SharedCode-bound dtor to compile without needing `Controller` complete. One heap allocation per processor instance, lazy on first VST3 query — negligible vs the existing 128-slot atomic table.
- **D-22:** Two-TU split for out-of-line definitions:
  - **Header (`cpp/NoteExpression.h`)** keeps only Steinberg-free symbols: `PendingTuningTable` (atomic-array typedef), `applyPendingTuning` (voice-side, `inline`, just `pow`), `VST3Extensions::drainBlockEvents` (no Steinberg refs), forward-declaration of `Controller`, `VST3Extensions` class declaration with pimpl `nec` member + out-of-line ctor/dtor + out-of-line `queryIEditController` + out-of-line `drainAndUpdate`. Free helper `updatePendingFromEvents` is forward-declared. Zero `<pluginterfaces/...>` includes; zero `#if JucePlugin_Build_VST3` guards (all guards from Plan 23-03 are removed).
  - **SharedCode-bound TU (`cpp/NoteExpression.cpp`, NEW, top-level — picked up by the new non-recursive `file(GLOB cpp/*.cpp ...)` SharedCode glob in D-27)**: `VST3Extensions` ctor (initializes `nec(nullptr, &noopControllerDelete)`, reserves the event buffers); `VST3Extensions::~VST3Extensions() = default` (custom-deleter pattern handles incomplete-type rule); `VST3Extensions::drainAndUpdate()` body (calls `drainBlockEvents` then dispatches NE-correlation work via a function pointer registered at static init by the VST3 TU — when no VST3 TU is linked, the pointer is null and `drainAndUpdate` skips correlation, which is correct because no VST3 host can deliver `kTuningTypeID` events to a non-VST3 build); `noopControllerDelete` definition; the dispatch function-pointer storage (`static std::atomic<NEUpdateFn> g_neUpdate { nullptr };` or equivalent, with thread-safe registration). **Zero Steinberg refs** — this TU links into AU, Standalone, and any other format wrapper without symbol-leak risk.
  - **VST3-only TU (`cpp/vst3/NoteExpression_VST3.cpp`, NEW — routed only into `${TARGET}_VST3` by D-27's per-format loop)**: full `Controller` class body (subclass of `Steinberg::Vst::INoteExpressionController`); `VST3Extensions::queryIEditController` body (references `INoteExpressionController::iid`, `Steinberg::FUnknown::iid`, `Steinberg::FUnknownPrivate::iidEqual`, `Steinberg::UString::assign` — all Steinberg-touching code consolidated here); `updatePendingFromEvents` free helper body (references `Steinberg::Vst::kTuningTypeID`); `realControllerDelete` (the actual deleter); a static-init mechanism that on translation-unit load (a) registers the function pointer in the SharedCode-bound TU's dispatch slot to point at `updatePendingFromEvents` and (b) installs `realControllerDelete` as the deleter on the pimpl when `queryIEditController` lazy-creates the Controller. All `<pluginterfaces/...>` includes live in this file.
  - **Net effect on linkage:** AU/Standalone link lines see SharedCode references to `VST3Extensions::VST3Extensions()`, `~VST3Extensions()`, and `drainAndUpdate()` — all defined in `cpp/NoteExpression.cpp` which compiles into SharedCode → resolved cleanly, zero Steinberg references. VST3 link line additionally pulls in `cpp/vst3/NoteExpression_VST3.cpp` which carries Controller + Steinberg-touching code → resolves the dispatch, lazy-creates the Controller on first `queryIEditController` call. The original Plan 23-03 `#if JucePlugin_Build_VST3` guards are removed entirely — no longer needed once the symbol-touching code is segregated by file placement instead of preprocessor guards.
- **D-23:** Module public API surface is preserved verbatim. `Ouaricon::NoteExpression::PendingTuningTable`, `applyPendingTuning(...)`, `VST3Extensions` public methods (`queryIEditController`, `onVst3RawEvent`, `drainBlockEvents`, `drainAndUpdate`, `getPendingTable`), and free `updatePendingFromEvents(...)` keep their existing signatures. `Controller` is internal — never appeared in the public consumer surface. O-Lyrica's existing call-sites (PluginProcessor, HarpSynthVoice) compile unchanged.

#### Per-Format Source Convention (new project-wide pattern)

- **D-24:** Convention: source files under `modules/<category>/<module>/cpp/<format>/` (lowercase JUCE format name) compile only into the JUCE `${TARGET}_<FORMAT>` per-format target. Files directly under `cpp/` continue to compile into SharedCode (umbrella target). Recognized format subdirectories: `cpp/vst3/`, `cpp/au/`, `cpp/standalone/`, `cpp/vst2/`, `cpp/aax/`, `cpp/lv2/`, `cpp/unity/`. Mirrors how JUCE itself organizes platform-specific code (`native/win32/`, `native/mac/`).
- **D-25:** `cpp/<format>/` include directories are added with PRIVATE visibility on the `${TARGET}_<FORMAT>` target only — non-format translation units cannot accidentally include format-specific headers. Headers placed under `cpp/<format>/` are themselves format-private.
- **D-26:** Convention applies to all current and future Ouaricon modules, not just `note-expression`. Phase 24 plugins inherit the fix automatically with no consumer-side change.

#### OuariconModules.cmake Extension

- **D-27:** `ouaricon_add_module()` is extended to auto-detect the per-format convention. Plugin call-sites (`ouaricon_add_module(<plugin> note-expression)`) remain UNCHANGED — the existing one-liner contract is preserved across O-Lyrica and all 7 Phase 24 plugins. Sketch of the new behavior (planner finalizes exact form):
  ```cmake
  # SharedCode glob narrows to top-level cpp/ only (excludes cpp/<format>/ subdirs):
  file(GLOB MODULE_CPP_SOURCES "${MODULE_DIR}/cpp/*.cpp" "${MODULE_DIR}/cpp/*.h")
  target_sources(${TARGET_NAME} PRIVATE ${MODULE_CPP_SOURCES})
  target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")

  # Per-format routing:
  set(_OUA_JUCE_FORMATS vst3 au standalone vst2 aax lv2 unity)
  foreach(fmt ${_OUA_JUCE_FORMATS})
      string(TOUPPER ${fmt} _FMT_UPPER)
      set(_FMT_DIR "${MODULE_DIR}/cpp/${fmt}")
      if(EXISTS "${_FMT_DIR}" AND TARGET "${TARGET_NAME}_${_FMT_UPPER}")
          file(GLOB_RECURSE _FMT_SOURCES "${_FMT_DIR}/*.cpp" "${_FMT_DIR}/*.h")
          if(_FMT_SOURCES)
              target_sources(${TARGET_NAME}_${_FMT_UPPER} PRIVATE ${_FMT_SOURCES})
              target_include_directories(${TARGET_NAME}_${_FMT_UPPER} PRIVATE "${_FMT_DIR}")
              message(STATUS "[Ouaricon]   Added ${MODULE_NAME}/cpp/${fmt} sources to ${TARGET_NAME}_${_FMT_UPPER}")
          endif()
      endif()
  endforeach()
  ```
  Two structural changes from today: (a) the SharedCode `file(GLOB_RECURSE ...)` becomes a non-recursive `file(GLOB ...)` so it doesn't sweep up `cpp/<format>/` files; (b) a per-format loop adds those subdirs to `${TARGET}_<FORMAT>` IF that target exists.
- **D-28:** Per-format routing silently no-ops when `${TARGET_NAME}_<FORMAT>` does not exist — e.g., a plugin that excludes AU from FORMATS still gets a clean configure when consuming a module that ships `cpp/au/`. Symmetric with the existing JS-copy block, which silently no-ops when `js/` is absent. Loud-error path is reserved for the existing "module not found" case.
- **D-29:** No change to `module.yaml` schema, no change to `registry.yaml`, no new helper macros. Convention-over-configuration: zero per-module CMake plumbing once `OuariconModules.cmake` is extended. The existing `module.cmake` hook (used by `note-expression` for the `JUCE-NE-PATCH` marker check) keeps its current contract.

#### AU Regression-Prevention Gate

- **D-30:** Plan 23-05 verify gate (and inherited as the Phase 24 per-plugin gate template):
  1. Clean rebuild: `ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone` — all three link cleanly with no `Undefined symbols for architecture arm64` errors mentioning `Steinberg::*`.
  2. Fresh install per CLAUDE.md (AU cache cleared, `.vst3` and `.component` re-installed).
  3. AU runtime check: `auval -v <type> <subtype> <manuf>`. Codes are extracted at verify time from `plugins/<Plugin>/CMakeLists.txt` (`PLUGIN_CODE` / `PLUGIN_MANUFACTURER_CODE` lines), with the AU type derived from `PLUGIN_AU_MAIN_TYPE` (e.g., `kAudioUnitType_MusicDevice → aumu`). Same one-liner generalizes across all Phase 24 plugins.
  4. Dorico smoke test (LYR-03 5-test set) re-passes via VST3 — proves the refactor preserved behavior.
- **D-31:** D-30's `auval` invocation is lifted into a small reusable shell script (e.g., `scripts/verify-au-link.sh <PluginName>`) so each Phase 24 plan inherits the same one-liner. Exact script location/naming is Claude's discretion during planning. Captures the discipline at the convention level — Phase 24 propagation cannot ship with a silently-broken AU again.

#### Scope Boundary (what Plan 23-05 does NOT do)

- **D-32:** No change to module public namespace (`Ouaricon::NoteExpression`), public class names, public method signatures, `module.yaml`, or `registry.yaml`. Internal layout change only — every consumer's call-site compiles unchanged.
- **D-33:** No version bump on the `note-expression` module (stays 1.0.0). Public API is unchanged; module hasn't shipped to any plugin other than O-Lyrica's local dev install. Whether O-Lyrica patches to 2.3.1 (defect fix) or stays at 2.3.0 (never released externally) is a Plan 23-05 planning question — leaning "no version bump, defect fix internal to Phase 23 closeout". Planner decides.
- **D-34:** No change to JUCE patch tooling, no re-generation of `scripts/juce-patches/note-expression-juce-8.0.4.patch`, no change to `apply-juce-patches.sh` or the `JUCE-NE-PATCH` marker check. Plan 23-05 is purely a Steinberg-symbol-leakage fix.

### Claude's Discretion

- Module README structure (format, TOC depth) — follow `scala-tuning-engine`'s README pattern.
- Exact apply-script error message wording — Claude chooses so long as it names the script path and explains the recovery action.
- CMake marker-check implementation (shell `execute_process` vs CMake `file(READ)` + `string(FIND)`) — Claude picks the cleaner option.
- Test ordering within the O-Lyrica refactor — Claude sequences the safe-rebuild checkpoints.

### Plan 23-05: Claude's Discretion

- Exact filenames for the two new translation units — `cpp/NoteExpression.cpp` (SharedCode-bound) and `cpp/vst3/NoteExpression_VST3.cpp` (VST3-only) are the suggested names; alternatives like `cpp/VST3Extensions.cpp` + `cpp/vst3/Controller.cpp` are equally valid. Keep the SharedCode TU Steinberg-free and the VST3 TU as the single home for `<pluginterfaces/...>` includes.
- Exact dispatch mechanism for `drainAndUpdate`'s NE-correlation hand-off (D-22) — function-pointer slot with `std::atomic<NEUpdateFn>` is the recommended baseline; alternatives include a Steinberg-free polymorphic interface registered at static init, or a callable wrapped in `std::function` (avoid heap if possible). Whichever path Claude picks must be thread-safe under JUCE's audio-thread/message-thread model and guarantee that non-VST3 builds emit exactly one TU-load-time registration of a no-op (or skip registration entirely and let the null-pointer dispatch path handle it).
- `noopControllerDelete` and `realControllerDelete` symbol names + linkage — internal-linkage statics are fine; only the function-pointer values need to cross the TU boundary.
- Whether to wire `auval` as a CTest target vs a verify-step shell command — Claude picks the cleaner integration.
- Naming and exact location of the AU verify script (`scripts/verify-au-link.sh` or `scripts/au-smoke.sh` or similar).
- Whether O-Lyrica patches to 2.3.1 (D-33).
- Final exact form of the `OuariconModules.cmake` per-format loop — sketch in D-27 is the spec, planner finalizes the cmake_parse_arguments / message wording / variable naming.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Spike Findings (implementation bible)
- `.claude/skills/spike-findings-VST-development/SKILL.md` — auto-loaded findings index.
- `.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md` — validated patterns 1–5, landmines 1–5, constraints. Primary reference.
- `.claude/skills/spike-findings-VST-development/sources/shared-code/juce-patch.md` — exact hunks for both JUCE files (source of truth for the generated `.patch`).
- `.claude/skills/spike-findings-VST-development/sources/shared-code/NoteExpressionSupport.spike.h` — pre-strip spike code (Controller + Extensions).
- `.claude/skills/spike-findings-VST-development/sources/shared-code/processor-drain.cpp` — drain + two-pass NE→pitch correlation logic (moves into `updatePendingFromEvents`).
- `.claude/skills/spike-findings-VST-development/sources/shared-code/voice-startNote.cpp` — voice-side `exchange(0.0)` pattern (moves into `applyPendingTuning`).

### Milestone Scoping
- `.planning/REQUIREMENTS.md` §MODULE, §LYRICA — MOD-01..08 + LYR-01..04 binding requirements for this phase.
- `.planning/ROADMAP.md` §Phase 23 — goal statement + 5 success criteria.
- `.planning/seeds/microtonal-shared-module.md` — original seed; extraction rationale and candidate extractables table.
- `.planning/notes/dorico-microtonal-vst-research.md` — upstream research on Dorico's NE behavior.

### Current O-Lyrica Spike Sites (source of truth for refactor targets)
- `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` — deletion target.
- `plugins/O-Lyrica/Source/PluginProcessor.h` lines 22, 48, 127, 210 — `pendingTuningSemis` array + `queryPendingTuningSource()` accessor; moves to module.
- `plugins/O-Lyrica/Source/PluginProcessor.cpp` lines 506, 716–759 — drain + correlation site; becomes `updatePendingFromEvents` call.
- `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` lines 13, 87, 142–158 — voice-side `exchange` + neTrace call sites; voice code post-refactor is one `applyPendingTuning` call.
- `plugins/O-Lyrica/Source/HarpSynthVoice.h` line 128 — `pendingTuningSource` pointer; switches to pointing at module's table.

### Module System
- `modules/registry.yaml` — target registration entry (tuning category).
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module()` integration pattern.
- `modules/tuning/scala-tuning-engine/module.yaml` — reference shape for module metadata.
- `modules/tuning/scala-tuning-engine/README.md` — reference shape for consumer integration docs.

### JUCE Patch Targets (local fork)
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h` line 64 — `Vst3RawEvent` struct + `onVst3RawEvent` virtual. Patch hunk 1/2.
- `/Users/taylorbrook/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` line 3699 — raw event forwarding before `MidiEventList::toMidiBuffer`. Patch hunk 2/2.

### Plugin Conventions
- `CLAUDE.md` — Plugin Cache Clearing protocol (AU cache + re-install sequence) applies to post-refactor O-Lyrica build.
- `plugins/O-Lyrica/CHANGELOG.md` — style reference for the 2.3.0 entry.

### Plan 23-05 Sources (AU-link defect)
- `.planning/phases/23-extract/23-04-version-readme-dorico-smoketest-SUMMARY.md` §"D-23-04-A: AU re-link failure exposed module-level architectural defect" — full root-cause analysis, exact `Undefined symbols` output, why Plan 23-03's "AU built clean" claim was incorrect (stale Apr 13 artefact). Required reading before planning Plan 23-05.
- `modules/tuning/note-expression/cpp/NoteExpression.h` — current state. The `#if JucePlugin_Build_VST3` guards (lines 28–33, 126, 227, 249–270, 304–306) and value-composed `Controller nec` member (line 305) are the deletion / relocation targets. The header-level "Per-format guarding (added Plan 23-03)" comment block describes the broken approach being replaced.
- `modules/cmake/OuariconModules.cmake` lines 30–107 (`ouaricon_add_module()`) — extension target. Specifically: line 54's `file(GLOB_RECURSE ...)` becomes a top-level-only `file(GLOB ...)`; new per-format loop appended after the existing C++ source block (line 64) and before the JS-copy block (line 66).
- `modules/tuning/note-expression/module.cmake` — unchanged by Plan 23-05; reference for how `module.cmake` hooks plug into the per-module CMake flow without modifying `OuariconModules.cmake`.
- `plugins/O-Lyrica/CMakeLists.txt:10` (`FORMATS VST3 AU Standalone`) — defines which `${OLyrica}_<FORMAT>` targets JUCE creates and therefore which per-format routes the new `ouaricon_add_module()` extension activates.
- `plugins/O-Lyrica/CMakeLists.txt:6` (`juce_add_plugin(OLyrica ...)`) — must be invoked BEFORE `ouaricon_add_module(OLyrica note-expression)` so per-format subtargets exist when the macro runs. Already true today; document as a pre-condition.

### JUCE Per-Format Targets (reference)
- `JUCE/extras/Build/CMake/JUCEUtils.cmake` — defines how `juce_add_plugin(<TARGET> ... FORMATS VST3 AU Standalone ...)` creates per-format subtargets named `${TARGET}_VST3`, `${TARGET}_AU`, `${TARGET}_Standalone` plus the umbrella SharedCode static library. Per-format targets are what Plan 23-05's auto-detect routing attaches sources to.
- `JUCE/extras/Build/CMake/JUCEModuleSupport.cmake` — JUCE's own pattern for routing module sources by category (`native/win32/`, `native/mac/`) is the design precedent for the new `cpp/<format>/` convention.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`modules/cmake/OuariconModules.cmake`** — `ouaricon_add_module()` handles CMake sources, module.yaml discovery by category path, and JS copy. The new module plugs into this without extending the function.
- **`modules/tuning/scala-tuning-engine/`** — canonical shape for a tuning-category module (module.yaml, README.md, cpp/, snippets/, docs/). New `note-expression` mirrors this layout minus the JS (pure C++ / no UI).
- **Existing JUCE patch already applied** at `/Users/taylorbrook/JUCE/` — markers confirmed at 2 locations via `grep -rn "JUCE-NE-PATCH"`. Patch file is generated from these already-applied edits, not re-applied during this phase.
- **O-Lyrica `TuningEngine`** (`Source/DSP/TuningEngine.cpp`) — exposes per-note frequency lookup; composition point for D-10.

### Established Patterns
- **Module registry categories** live in `registry.yaml`; `tuning` category already present — no category addition needed (path decision D-01).
- **Semver + used_by tracking** — every registered module carries version and a `used_by:` list; Phase 23 adds O-Lyrica, Phase 24 appends 7 more.
- **Markdown hunks for JUCE patches** — current `spike-findings` convention. This phase upgrades to real `.patch` files (D-12) — sets a new convention going forward.
- **`CLAUDE.md` build-and-install protocol** — every rebuild clears AU cache + reinstalls fresh. LYR-03 smoke test depends on this discipline.

### Integration Points
- **`modules/registry.yaml` `modules:` list** — append new entry at end of `tuning` section.
- **O-Lyrica `CMakeLists.txt`** — `ouaricon_add_module(OLyrica note-expression)` line near the existing `include(OuariconModules.cmake)`.
- **O-Lyrica `PluginProcessor.h`** — swap 128-slot table declaration for `Ouaricon::NoteExpression::VST3Extensions m_extensions;`.
- **CMake marker check** — lives inside the module's CMake glue (`modules/tuning/note-expression/` or registered via `ouaricon_add_module` hook), so it only activates for plugins that consume the module.

### Plan 23-05 Touch Points

**Module-side (the fix) — three files in concert:**
- **`modules/tuning/note-expression/cpp/NoteExpression.h`** — strip Steinberg includes (lines 28–33), strip all `#if JucePlugin_Build_VST3` guards, forward-declare `Controller`, change member to `std::unique_ptr<Controller, void(*)(Controller*)> nec` (custom function-pointer deleter so dtor links from non-VST3 TUs without seeing `Controller`'s body), declare ctor/dtor + `queryIEditController` + `drainAndUpdate` out-of-line. `applyPendingTuning` and `drainBlockEvents` stay in header (Steinberg-free); `updatePendingFromEvents` keeps its forward-declaration only.
- **`modules/tuning/note-expression/cpp/NoteExpression.cpp` (new, top-level → SharedCode)** — `VST3Extensions` ctor (`nec(nullptr, &noopControllerDelete)`, vector reserves), dtor (`= default`), `drainAndUpdate` body that calls `drainBlockEvents` and dispatches NE-correlation through a function-pointer slot (registered at static init by the VST3 TU; null in non-VST3 builds → correlation skipped, which is correct because non-VST3 hosts can't deliver `kTuningTypeID` events anyway), `noopControllerDelete` definition, dispatch slot storage. **Zero Steinberg references.** Picked up by D-27's narrowed `file(GLOB cpp/*.cpp ...)` SharedCode glob; links into AU, Standalone, and any future format wrapper.
- **`modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` (new, VST3-only → `${TARGET}_VST3`)** — full `Controller` class body, `VST3Extensions::queryIEditController` body (lazy-creates `nec` with `realControllerDelete` on first call), `updatePendingFromEvents` free helper body, `realControllerDelete`, static-init that registers `updatePendingFromEvents` into the SharedCode dispatch slot. All `<pluginterfaces/...>` includes consolidated here. Routed only into `${TARGET}_VST3` by D-27's per-format loop.

**CMake-side (the convention):**
- **`modules/cmake/OuariconModules.cmake`** — narrow the SharedCode glob (line 54) to non-recursive `cpp/*.cpp` + `cpp/*.h`; append a per-format loop iterating `vst3 au standalone vst2 aax lv2 unity` and routing matching `cpp/<format>/` directories to `${TARGET_NAME}_<FORMAT>` when that target exists. Update the `[Ouaricon]` STATUS message wording to distinguish SharedCode adds from per-format adds.

**Verification-side (the gate):**
- **`scripts/verify-au-link.sh` (new, name TBD)** — small shell helper that takes a plugin target name, parses `PLUGIN_CODE` / `PLUGIN_MANUFACTURER_CODE` / `PLUGIN_AU_MAIN_TYPE` from `plugins/<Plugin>/CMakeLists.txt`, and invokes `auval -v <type> <subtype> <manuf>`. Reused verbatim by Plan 23-05 verify and every Phase 24 plan.

**Consumer-side (zero change):**
- **`plugins/O-Lyrica/CMakeLists.txt`** — `ouaricon_add_module(OLyrica note-expression)` call-site untouched. Same for all 7 Phase 24 plugins when they adopt the module.
- **`plugins/O-Lyrica/Source/PluginProcessor.{h,cpp}`** — `Ouaricon::NoteExpression::VST3Extensions m_extensions` member, `getVST3ClientExtensions()` accessor, `drainAndUpdate()` call from `processBlock` — all untouched. Public API preservation (D-23) means no consumer-side code edits.

</code_context>

<specifics>
## Specific Ideas

- **"Ouaricon::NoteExpression:: feels right."** The spike's `OLyrica::` prefix was plugin-specific; generic namespace is the natural extraction.
- **Reference-consumer pattern:** O-Lyrica is the template Phase 24 plugins copy. Keeping O-Lyrica free of any plugin-local NE code (no shim) means Phase 24 follow-up plugins have an unambiguous target shape.
- **Dorico quarter-sharp smoke test is the canonical acceptance gate:** pitch = +50¢ above C4, no attack zipper, NE events correlated by `noteId`. Every Phase 24 plugin will repeat this test.
- **JUCE-NE-PATCH marker is the load-bearing convention** — both hunks already carry it (spike 001), the `.patch` file preserves it, the CMake check greps for it. Do not rename this marker.

### Plan 23-05 Specifics

- **"Convention over configuration."** The chosen `cpp/<format>/` subdirectory pattern means modules declare per-format intent by file placement — no `module.yaml` schema growth, no per-module CMake boilerplate, no kwargs in consumer call-sites. Mirrors how JUCE itself organizes platform-specific source under `native/`.
- **"The one-liner contract is non-negotiable."** Every plugin in Phase 24 keeps consuming the module via `ouaricon_add_module(<plugin> note-expression)` — same line, no new args, no per-format awareness in the plugin CMakeLists. The fix lives entirely in the module + the macro.
- **"Symbol leakage was structural, not accidental."** D-23-04-A is not a coding error to be patched per-plugin — it's a translation-unit-scoping defect that any future `Steinberg::*`-touching module would re-introduce without the per-format convention. Plan 23-05 is the structural fix, not a workaround.
- **"AU has to load, not just link."** `ninja OLyrica_AU` succeeding is necessary but not sufficient — `auval` must accept the .component too (catches code-signing, plist, and IID-registration failures that only surface at host-load time). The AU verify gate is the load test, not just the link test.
- **"Pimpl is justified by the symbol-leakage class, not by encapsulation aesthetics."** `unique_ptr<Controller>` is the cleanest way to keep `Controller`'s definition out of the header — the heap allocation cost is irrelevant against the existing 128-slot atomic table. Don't second-guess the pimpl during planning.

</specifics>

<deferred>
## Deferred Ideas

- **MOD-05 README vs Phase 25 DOCS overlap** — Phase 23 ships a functional module README (consumer integration + JUCE patch re-apply + basic end-user Dorico expression-map setup). Phase 25 produces the comprehensive, website-ready `research/microtonal-dorico-integration.md` (DOCS-01..05). Brief overlap is acceptable; the Phase 25 version supersedes for end-user authoring.
- **Automated regression test for NE** — currently manual Dorico smoke test only. Preserving a spike-era test harness is deferred; noted as a future-phase candidate if Phase 24 reveals recurring regression risk.
- **Windows VST3 verification** — FUT-01. Patch is in cross-platform wrapper code; should work unchanged. Out of scope this phase.
- **MTS-ESP as orthogonal microtonal path** — FUT-03. Different protocol, not suitable for Dorico's per-note deltas.
- **Cross-block `noteId → voice` map** — FUT-04. Dorico emits NE in same block; sufficient for this phase.
- **Custom NE types beyond `kTuningTypeID`** (per-note timbre, vibrato depth) — FUT-02. Reserved ID range `[100000, 200000]` noted in spike findings.

### Plan 23-05 Deferred

- **CMake-time include-grep assertion** — a structural check that scans `cpp/*.cpp` for `#include <pluginterfaces/...>` and fails configure if found outside `cpp/vst3/`. Considered as an extra belt-and-suspenders guard against future regressions, but rejected for Plan 23-05: brittle (relies on greppable include paths), and the per-format convention + verify gate (D-30) already prevent the defect at the structural level. Re-open if Phase 24 reveals recurring symbol-leakage incidents.
- **CTest integration of `auval` smoke check** — wiring the AU verify gate as a CTest target (instead of a verify-step shell command) was considered. Deferred to Claude's discretion during Plan 23-05 planning (D-30/D-31 footnote); not a binding decision.
- **Windows AU equivalent** — there is none (AU is macOS-only). Plan 23-05's gate runs only on macOS; Windows verification (FUT-01) is unaffected.
- **Per-format pattern for non-source assets** — the new `cpp/<format>/` convention covers C++ sources only. Per-format JS, per-format YAML configs, per-format resources are not in scope. Re-open only when a real consumer needs them.
- **Refactoring `module.cmake` hooks** — the per-module CMake hook (used by `note-expression` for the `JUCE-NE-PATCH` marker check) keeps its current contract (D-29). Any restructure of the hook system is out of scope.

</deferred>

---

*Phase: 23-extract*
*Context gathered: 2026-04-24*
*Plan 23-05 context appended: 2026-04-25*
*Plan 23-05 D-21/D-22 amended: 2026-04-25 (two-TU split with custom-deleter pimpl + dispatch slot — fixes plan-checker BLOCKER on AU/Standalone link)*
