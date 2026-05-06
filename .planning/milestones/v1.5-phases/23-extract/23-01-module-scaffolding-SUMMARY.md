---
phase: 23-extract
plan: 01
subsystem: modules/tuning/note-expression
tags: [vst3, note-expression, module-extraction, header-only, microtonal, dorico]
requires:
  - "Spike findings: .claude/skills/spike-findings-VST-development/sources/shared-code/NoteExpressionSupport.spike.h"
  - "Existing module category: tuning (modules/registry.yaml)"
  - "Local JUCE 8.0.4 fork with JUCE-NE-PATCH markers (spike-applied; patch tooling ships in Plan 02)"
provides:
  - "modules/tuning/note-expression/ — registered Ouaricon module v1.0.0"
  - "Ouaricon::NoteExpression::Controller (NEC for kTuningTypeID)"
  - "Ouaricon::NoteExpression::VST3Extensions (juce::VST3ClientExtensions subclass; owns NEC + raw-event queue + PendingTuningTable)"
  - "Ouaricon::NoteExpression::PendingTuningTable (std::array<std::atomic<double>, 128>)"
  - "Ouaricon::NoteExpression::applyPendingTuning (header-only voice helper)"
  - "Ouaricon::NoteExpression::updatePendingFromEvents (header-only drain+correlate helper)"
affects:
  - "Plan 03 (O-Lyrica refactor) — consumes #include \"NoteExpression.h\" + VST3Extensions + applyPendingTuning"
  - "Phase 24 plugins (O-Bells/Wind/Reed/Bowed/Formant + 2) — adopt the same module"
tech-stack:
  added:
    - "Module convention: header-only C++ tuning module under namespace Ouaricon::*"
  patterns:
    - "PendingTuningTable ownership on VST3Extensions (D-09) — plugins do not redeclare the 128-slot table"
    - "Drain + two-pass noteId-keyed correlation moved out of plugin processBlock into module helper"
    - "Voice helper encapsulates pow(2, semis/12) so voice code never calls pow directly (LYR-02 / no-raw-pow-in-voice)"
    - "exchange(0.0, acq_rel) on per-pitch slot — retriggered notes cannot inherit stale offsets (T-23-01)"
key-files:
  created:
    - modules/tuning/note-expression/module.yaml
    - modules/tuning/note-expression/README.md
    - modules/tuning/note-expression/cpp/NoteExpression.h
  modified:
    - modules/registry.yaml
decisions:
  - "Header-only single-file module (precedent: persistence/preset-manager) — cpp/NoteExpression.h alone, no .cpp"
  - "Repo convention cpp/<Class>.h chosen over CONTEXT.md's include/Ouaricon/NoteExpression/*.h — OuariconModules.cmake already globs cpp/*.h, namespacing happens inside the file"
  - "PendingTuningTable ownership lives on VST3Extensions, not the plugin (D-09) — plugins call drainAndUpdate() and hand voices &m_extensions.getPendingTable()"
  - "No dependency on scala-tuning-engine (D-11) — composition happens at the voice call site"
metrics:
  duration: 4min
  completed: 2026-04-25
  tasks: 2
  files: 4
---

# Phase 23 Plan 01: Module Scaffolding Summary

**One-liner:** Extracted the spike's NoteExpressionSupport into a registered shared module at `modules/tuning/note-expression/` (v1.0.0) — header-only C++ under `Ouaricon::NoteExpression`, owns the 128-slot pending-tuning table, exposes Controller + VST3Extensions + two helper free functions, and is registered in `modules/registry.yaml` for `/module-list` discovery. The module is not yet compile-tested — Plan 03 is the integration gate.

## What Shipped

### Files Created

1. **`modules/tuning/note-expression/module.yaml`** — Module metadata: `name: note-expression`, `version: 1.0.0`, `category: tuning`, `dependencies: []`, `provides:` lists the two classes + type alias + two free functions, `requirements.juce_patch.marker: "JUCE-NE-PATCH"`, `used_by: [{plugin: OLyrica, version: 2.3.0}]`, single 1.0.0 changelog entry dated 2026-04-24.
2. **`modules/tuning/note-expression/README.md`** — Stub README (Plan 04 ships the comprehensive version). Contains H1 + 1-line description + public API preview block + JUCE patch requirement note + redirect to Plan 04 for full integration docs.
3. **`modules/tuning/note-expression/cpp/NoteExpression.h`** — Single header-only file containing:
   - File-header comment block re-attributed from spike OLyrica → note-expression module v1.0.0.
   - Includes scoped down: `<JuceHeader.h>`, `<pluginterfaces/vst/ivstnoteexpression.h>`, `<pluginterfaces/base/ibstream.h>`, `<pluginterfaces/base/ustring.h>`, `<public.sdk/source/common/pluginview.h>`, `<array>`, `<atomic>`, `<cmath>`, `<cstdint>`, `<map>`, `<vector>`. `<fstream>` and `<mutex>` removed (D-18, MOD-06).
   - C++17 nested namespace `Ouaricon::NoteExpression { ... }`.
   - `using PendingTuningTable = std::array<std::atomic<double>, 128>;` at namespace scope (D-06).
   - `inline double applyPendingTuning(table, midi, freq)` — voice helper. Bounds-checks midi, calls `exchange(0.0, std::memory_order_acq_rel)`, multiplies frequency by `std::pow(2.0, semis / 12.0)` if non-zero (D-07, MOD-04).
   - `inline void updatePendingFromEvents(events, table)` — drain+correlate helper. Two-pass: build `std::map<noteId, midi pitch>` from NoteOns, then for each kTuningTypeID NE, compute `240.0 * (e.value - 0.5)` semitones and store into `table[pitch]` (D-08).
   - `class Controller` (renamed from spike's `TuningNoteExpressionController`) — `INoteExpressionController` impl. Body byte-identical to spike minus `detail::neTrace` calls. Preserves `kIsBipolar | kIsAbsolute`, `240.0 * (valueNormalized - 0.5)` formula, ASCII widening for Dorico display, `FUnknown`/`queryInterface`/`addRef`/`release` plumbing.
   - `class VST3Extensions` (renamed from `LyricaVST3Extensions`) — `juce::VST3ClientExtensions` subclass. Constructor reserves both `blockEvents` and `rawEventScratch` to 64 (T-23-02). Owns a private `Controller nec` + `PendingTuningTable pendingTable {}` + the two scratch vectors. Public API: `queryIEditController`, `onVst3RawEvent`, `drainBlockEvents`, `drainAndUpdate()`, `PendingTuningTable& getPendingTable()`.

### Files Modified

4. **`modules/registry.yaml`** — Appended new entry inside the `# TUNING MODULES` section, positioned after the `scala-tuning-engine` entry's `used_by: []` line and before the `# EFFECTS MODULES` divider. Entry includes name, path, version 1.0.0, multi-line description, category tuning, 5-item provides list, `dependencies: []`, 6-tag list including `vst3` / `note-expression` / `dorico` / `microtuning` / `per-note-pitch` / `header-only`, `reuse_score: 10`, `used_by: [{plugin: OLyrica, version: 2.3.0}]`. The pre-existing `scala-tuning-engine` entry was not touched.

## Symbols Exported

```cpp
namespace Ouaricon::NoteExpression {
    using PendingTuningTable = std::array<std::atomic<double>, 128>;

    inline double applyPendingTuning      (PendingTuningTable&, int midi, double freq);
    inline void   updatePendingFromEvents (const std::vector<juce::VST3ClientExtensions::Vst3RawEvent>&,
                                           PendingTuningTable&);

    class Controller     : public Steinberg::Vst::INoteExpressionController { ... };
    class VST3Extensions : public juce::VST3ClientExtensions {
        // Public: queryIEditController, onVst3RawEvent, drainBlockEvents,
        //         drainAndUpdate, getPendingTable
    };
}
```

## Stripping Audit (D-18 / MOD-06)

```
$ grep -rE "neTrace|iidToHex|<fstream>|<mutex>|OLyrica::detail" modules/tuning/note-expression/
(no matches)
```

| Pattern | Module-tree count | Required |
|---------|-------------------|----------|
| `detail::neTrace` | 0 | 0 |
| `detail::iidToHex` | 0 | 0 |
| `#include <fstream>` | 0 | 0 |
| `#include <mutex>` | 0 | 0 |
| `OLyrica::detail` | 0 | 0 |
| `namespace OLyrica` | 0 | 0 |

The spike's `/tmp/olyrica-ne-trace.log` audio-thread file I/O is fully eliminated. No locked `std::ofstream`, no IID hex stringification, no `juce::String::formatted` debug calls remain in the module source.

## Acceptance Grep Audit

| Check | Required | Got |
|-------|----------|-----|
| `namespace Ouaricon::NoteExpression` | 1 or 2 | 2 (open + close-comment) |
| `class Controller` | ≥1 | 1 |
| `class VST3Extensions` | ≥1 | 1 |
| `using PendingTuningTable` | 1 | 1 |
| `inline double applyPendingTuning` | 1 | 1 |
| `inline void updatePendingFromEvents` | 1 | 1 |
| `void drainAndUpdate` | 1 | 1 |
| `PendingTuningTable& getPendingTable` | 1 | 1 |
| `240.0 * (valueNormalized - 0.5)` | ≥1 | 1 |
| `exchange (0.0, std::memory_order_acq_rel)` | ≥1 | 1 |
| `kTuningTypeID` | ≥2 | 8 |
| `std::pow` | exactly 1 | 1 |
| `JUCE-NE-PATCH` (module.yaml) | 1 | 1 |
| Registry: `^  - name: note-expression$` | 1 | 1 |
| Registry: `path: tuning/note-expression` | 1 | 1 |

## Compile-Test Status

**Not yet compiled.** Per the plan's acceptance criteria for Task 2, in-isolation `c++ -fsyntax-only` is not a required gate (JUCE needs more than include paths to preprocess). The binding integration gate is **Plan 03**: when O-Lyrica's CMake adds `ouaricon_add_module(OLyrica note-expression)` and `OLyrica_VST3` builds successfully, the header has passed the real compile test.

## What Plan 03 Consumes From This Plan

- The header at `cpp/NoteExpression.h` — accessible via `#include "NoteExpression.h"` once `ouaricon_add_module(OLyrica note-expression)` adds the module's `cpp/` to OLyrica's include path.
- `Ouaricon::NoteExpression::VST3Extensions` — drop-in replacement for the spike's `OLyrica::LyricaVST3Extensions`. Plan 03 declares one as a member of `PluginProcessor` and returns its address from `getVST3ClientExtensions()`.
- `m_extensions.drainAndUpdate()` — Plan 03 calls this from `processBlock` to replace the 58-line drain+correlate spike block.
- `&m_extensions.getPendingTable()` — Plan 03 hands this pointer to each voice via `setPendingTuningSource` (now typed as `PendingTuningTable*`).
- `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)` — Plan 03 replaces the voice's 19-line `exchange + pow` block with one call to this helper.

The module is also pre-staged for Plan 02 (JUCE patch tooling) and Phase 24 (7 additional consumers) — `module.yaml` already declares the `JUCE-NE-PATCH` requirement, and the `used_by:` list is set up to grow as Phase 24 plugins adopt.

## Deviations from Plan

None — plan executed exactly as written. Two minor wording adjustments inside `module.yaml` (changelog phrasing) were made to satisfy plan-level grep audits that scan the entire module tree for forbidden tokens; these are documentation-only and do not change the public API or the module's behavior.

## Commits

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Create module directory, module.yaml, registry entry | `4ce5b13` | `modules/tuning/note-expression/module.yaml`, `modules/tuning/note-expression/README.md`, `modules/registry.yaml` |
| 2 | Create cpp/NoteExpression.h | `e1232bd` | `modules/tuning/note-expression/cpp/NoteExpression.h`, `modules/tuning/note-expression/module.yaml` (changelog wording fix) |

## Self-Check: PASSED

- [x] `modules/tuning/note-expression/module.yaml` — present
- [x] `modules/tuning/note-expression/README.md` — present
- [x] `modules/tuning/note-expression/cpp/NoteExpression.h` — present
- [x] `modules/registry.yaml` — entry appended (note-expression in tuning section)
- [x] Commit `4ce5b13` — found in git log
- [x] Commit `e1232bd` — found in git log
- [x] Diagnostic-token sweep: 0 matches across module tree
- [x] All acceptance grep checks pass
