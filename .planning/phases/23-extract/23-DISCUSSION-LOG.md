# Phase 23: Extract — Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-24 (initial); 2026-04-25 (Plan 23-05 addendum)
**Phase:** 23-extract
**Areas discussed:** Module identity & placement, Module API surface, JUCE patch management, O-Lyrica refactor shape, Plan 23-05: Code split shape, Per-format file convention, OuariconModules.cmake API, AU regression-prevention, Controller member layout, AU verify gate sourcing

---

## Module identity & placement

### Module path

| Option | Description | Selected |
|--------|-------------|----------|
| tuning/note-expression | Sits alongside scala-tuning-engine in existing 'tuning' category. No new category needed. Semantically accurate. | ✓ |
| dsp/note-expression | Match REQUIREMENTS.md candidate verbatim. Requires adding new 'dsp' category to registry.yaml. | |
| core/vst3-note-expression | Treat as VST3 protocol infrastructure. Co-located with licensing/resource-provider. | |

**User's choice:** tuning/note-expression (Recommended)
**Notes:** REQUIREMENTS.md candidate `dsp/note-expression` was provisional; no `dsp` category exists. NE is fundamentally a per-note tuning mechanism, so tuning category is semantically correct.

### Starting semver

| Option | Description | Selected |
|--------|-------------|----------|
| 1.0.0 | Spike-validated, shipping on O-Lyrica, used by 7 plugins in Phase 24. Matches registry convention. | ✓ |
| 0.1.0 | Pre-1.0 signaling API may shift as 7 plugins adopt it. | |

**User's choice:** 1.0.0 (Recommended)

### Registry name

| Option | Description | Selected |
|--------|-------------|----------|
| note-expression | Short, matches pattern (preset-manager, vu-meter, analog-eq-unit). | ✓ |
| vst3-note-expression | Explicitly flags the format dependency (VST3 only). | |
| microtonal-note-expression | Leads with the user-facing use case. | |

**User's choice:** note-expression (Recommended)

---

## Module API surface

### Voice helper signature shape

| Option | Description | Selected |
|--------|-------------|----------|
| Return new frequency | `double applyPendingTuning(PendingTuningTable&, int midiNote, double currentFrequency)`. Pow inside helper; voice code stays clean. | ✓ |
| Mutate in place | `void applyPendingTuning(..., double& currentFrequency)`. Terser but implicit mutation. | |
| Return semitones only | `double getPendingSemitones(...)`. Voice applies pow — re-introduces LYR-02 concern. | |

**User's choice:** Return new frequency (Recommended)

### Public namespace / class naming

| Option | Description | Selected |
|--------|-------------|----------|
| Ouaricon::NoteExpression:: | Nested namespace. Classes: Controller, VST3Extensions. Room for sibling Ouaricon:: modules. | ✓ |
| OuariconNoteExpression:: (flat) | Matches OuariconPresetManager / OuariconLicense style. Less nesting. | |
| ouaricon::note_expression:: | Snake_case lowercase nested. Matches Steinberg/JUCE aesthetic. | |

**User's choice:** Ouaricon::NoteExpression:: (Recommended)

### TuningEngine composition pattern

| Option | Description | Selected |
|--------|-------------|----------|
| TuningEngine first, then NE multiplier | Voice: `freq = tuningEngine.getFrequency(m); freq = NoteExpression::applyPendingTuning(..., freq);`. NE delta is always 12-TET — multiplicative compose is correct for any base tuning. | ✓ |
| Extend TuningEngine with getFrequencyWithSemitoneOffset() | Couples module to TuningEngine knowledge. Not portable. | |
| Module helper takes TuningEngine* arg | Hard dependency from note-expression on scala-tuning-engine. Rules out non-Scala plugins. | |

**User's choice:** TuningEngine first, then NE multiplier (Recommended)

---

## JUCE patch management

### Patch format

| Option | Description | Selected |
|--------|-------------|----------|
| Unified .patch + apply script | Real .patch file + idempotent apply script (marker-gated). Machine-applyable. | ✓ |
| Shell script with embedded hunks | Self-contained bash script editing via sed/grep. Harder to review diffs. | |
| Markdown hunks (current spike-findings style) | Copy-paste approach. Highest human error rate. | |

**User's choice:** Unified .patch + apply script (Recommended)

### Patch file location

| Option | Description | Selected |
|--------|-------------|----------|
| scripts/juce-patches/ | Matches MOD-07 spec. Top-level, easy to find. | ✓ |
| modules/tuning/note-expression/patches/ | Co-located with module. More structure for a single patch. | |

**User's choice:** scripts/juce-patches/ (Recommended)

### Re-apply verification strategy

| Option | Description | Selected |
|--------|-------------|----------|
| CMake-time marker check | Fatal error on configure if JUCE-NE-PATCH marker missing. Fails loud, fails fast. Prevents silent regression. | ✓ |
| README-only, manual procedure | Documented but unenforced. Relies on memory after every JUCE upgrade. | |
| Pre-commit hook | Runs on commit, not build — misses 'upgrade JUCE, build immediately' case. | |

**User's choice:** CMake-time marker check (Recommended)
**Notes:** Addresses regression sensitivity — silent-breakage scenario ("plugin builds, microtones quietly broken after JUCE upgrade") is exactly the failure mode this gate prevents.

---

## O-Lyrica refactor shape

### Plugin-local NoteExpressionSupport.h fate

| Option | Description | Selected |
|--------|-------------|----------|
| Delete entirely | Module owns all NE code. Voices #include module header. Zero plugin-local NE code. Cleanest reference consumer for Phase 24. | ✓ |
| Keep a tiny shim | Leave a plugin-local header with module includes + glue. Sets less-clean precedent. | |

**User's choice:** Delete entirely (Recommended)

### Pending-table ownership

| Option | Description | Selected |
|--------|-------------|----------|
| Module's VST3Extensions owns it | 128-slot array inside Ouaricon::NoteExpression::VST3Extensions. Processor uses m_ext.getPendingTable(). Phase 24 plugins don't re-declare table. | ✓ |
| Processor owns it (current spike) | Table on PluginProcessor. 7 Phase 24 plugins each re-declare the table. | |

**User's choice:** Module's VST3Extensions owns it (Recommended)

### O-Lyrica version bump level

| Option | Description | Selected |
|--------|-------------|----------|
| Minor: 2.2.2 → 2.3.0 | New user-visible feature (Dorico microtonal playback). Semver-correct backward-compatible feature addition. | ✓ |
| Patch: 2.2.2 → 2.2.3 | Conservative. Understates the capability. | |
| Major: 2.2.2 → 3.0.0 | No API-breaking change. Overkill for semver. | |

**User's choice:** Minor: 2.2.2 → 2.3.0 (Recommended)

---

## Claude's Discretion

Areas where the user deferred to Claude's judgment (captured in CONTEXT.md `<decisions>` section):
- Module README structure — follow scala-tuning-engine pattern.
- Apply-script error message wording.
- CMake marker-check implementation mechanism.
- Test sequencing within the O-Lyrica refactor.

## Deferred Ideas

- MOD-05 README vs Phase 25 DOCS — brief overlap acceptable; Phase 25 supersedes for website authoring.
- Automated regression test for NE — manual Dorico smoke test only this phase.
- Windows VST3 verification — FUT-01, deferred.
- MTS-ESP orthogonal path — FUT-03, deferred.
- Cross-block noteId→voice map — FUT-04, deferred.
- Custom NE type IDs beyond kTuningTypeID — FUT-02, deferred.

---

# Plan 23-05 Addendum (2026-04-25)

**Trigger:** Plan 23-04 surfaced AU-link defect (D-23-04-A in 23-04-SUMMARY.md). Plan 23-05 added to roadmap to fix it before Phase 24 propagation can begin. User requested: "add Plan 23-05 context (the AU-link defect, the per-format module-source pattern, OuariconModules.cmake extension)".

## Code split (header → .cpp)

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal: Controller class only | Move just Controller body to .cpp. queryIEditController stays inline guarded. Header still needs pluginterfaces include in VST3 builds. | |
| Standard: Controller + queryIEditController | Move both to .cpp. Header forward-declares Controller, no pluginterfaces include in header. Mirrors how JUCE splits VST3 wrapper code. | ✓ |
| Aggressive: pimpl entire VST3 surface | Move Controller, queryIEditController, AND updatePendingFromEvents to .cpp. Header has zero Steinberg refs. Largest diff. | |

**User's choice:** Standard: Controller + queryIEditController (Recommended)
**Notes:** During follow-up the boundary effectively expanded to "anything that references Steinberg::*" — `updatePendingFromEvents` references `Steinberg::Vst::kTuningTypeID` and the Steinberg includes are leaving the header anyway, so it MUST move to the .cpp too (D-22). Net effect lands close to the "Aggressive" option in scope, but framed as the natural consequence of the Standard split rather than as a separate decision.

## Per-format file convention

| Option | Description | Selected |
|--------|-------------|----------|
| Subdirectory: cpp/vst3/, cpp/au/, cpp/standalone/ | Files under cpp/<fmt>/ auto-route to ${TARGET}_<FMT>. Mirrors JUCE's native/win32/, native/mac/ pattern. Self-documenting. | ✓ |
| Filename suffix: *.vst3.cpp, *.au.cpp | Compact for small per-format footprint. Less self-documenting than directories. | |
| Explicit module.cmake declaration | New helper macro listing per-format files explicitly. Most flexible. Every module re-states routing. | |

**User's choice:** Subdirectory: cpp/vst3/, cpp/au/ (Recommended)

## OuariconModules.cmake API shape

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-detect inside ouaricon_add_module() | Macro scans for cpp/<fmt>/ convention and routes to ${TARGET}_<FMT> if it exists. Zero call-site change for any plugin. Phase 24 inherits the fix automatically. | ✓ |
| Helper invoked from module.cmake | New macro ouaricon_module_add_format_sources(...) callable from module.cmake. Modules opt-in to per-format routing explicitly. | |
| Add VST3_SOURCES/AU_SOURCES kwargs | Plugin call-site grows. Pushes module internals into every consumer's CMakeLists. Breaks the one-liner contract. | |

**User's choice:** Auto-detect inside ouaricon_add_module() (Recommended)

## AU regression-prevention

| Option | Description | Selected |
|--------|-------------|----------|
| Plan 23-05 verify gate only | Clean rebuild (VST3+AU+Standalone) + auval check + Dorico smoke test. Convention + reference plugin enforce it. Phase 24 inherits the gate per plugin. | ✓ |
| Verify gate + module CMake assertion | Add a configure-time grep for `#include <pluginterfaces/...>` outside cpp/vst3/. Structural enforcement; defect literally cannot ship. Slightly brittle. | |
| Verify gate + per-plugin AU-link smoke in build-and-install script | Catch silent install-time skip (the scenario that masked D-23-04-A). | |

**User's choice:** Plan 23-05 verify gate only (Recommended)
**Notes:** CMake-time include-grep assertion captured as deferred (re-open if Phase 24 reveals recurring symbol-leakage incidents).

## Controller member layout

| Option | Description | Selected |
|--------|-------------|----------|
| Pimpl: unique_ptr<Controller> | Header forward-declares Controller; member is std::unique_ptr<Controller>. Header is fully Steinberg-free, no #if guards anywhere. One heap allocation per processor instance. | ✓ |
| Guarded value member: #if JucePlugin_Build_VST3 Controller nec; #endif | Avoids heap allocation but keeps the conditional in the header AND requires Controller to be a complete type at the member-declaration point — contradicts the chosen split goal. | |

**User's choice:** Pimpl: unique_ptr<Controller> (Recommended)

## AU verify gate sourcing

| Option | Description | Selected |
|--------|-------------|----------|
| Read codes from <Plugin>/CMakeLists.txt at verify time | Parse PLUGIN_CODE / PLUGIN_MANUFACTURER_CODE / PLUGIN_AU_MAIN_TYPE per plugin. Generalizes across all Phase 24 plugins without per-plugin hardcoding. | ✓ |
| auval -a \| grep -i lyrica | Lighter check: confirm AU appears in global validation list. Matches CLAUDE.md smoke check. | |
| Skip auval, rely on link success + manual DAW open | Fastest gate, weakest signal. | |

**User's choice:** Read codes from <Plugin>/CMakeLists.txt at verify time (Recommended)

---

## Plan 23-05: Claude's Discretion

Areas where the user deferred to Claude's judgment (captured in CONTEXT.md `<decisions>` section):
- Exact filename for the new VST3-only translation unit (`NoteExpression_VST3.cpp` vs `Controller.cpp` vs `VST3Bridge.cpp`).
- Whether `VST3Extensions` ctor/dtor are `=default` in the .cpp.
- Naming and exact location of the AU verify shell script.
- CTest target vs verify-step shell command for `auval` integration.
- Whether O-Lyrica patches to 2.3.1 (defect fix never released externally) — D-33.
- Final exact form of the `OuariconModules.cmake` per-format loop (sketch in D-27 is the spec).

## Plan 23-05 Deferred

- CMake-time include-grep assertion (`#include <pluginterfaces/...>` outside `cpp/vst3/`) — rejected as brittle for now; re-open if Phase 24 reveals recurring symbol-leakage.
- CTest integration of `auval` smoke check — deferred to Claude's discretion during planning.
- Per-format pattern for non-source assets (per-format JS, YAML, resources) — out of scope; re-open when a real consumer needs them.
- Refactoring `module.cmake` hook system — out of scope.
- Windows AU equivalent — does not exist (AU is macOS-only).
