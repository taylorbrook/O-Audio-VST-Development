---
phase: 23-extract
plan: 03
status: complete
duration: ~25min
tasks: 4
files_modified: 6
requirements_satisfied: [LYR-01, LYR-02]
deviations: 1
---

# Plan 23-03: O-Lyrica Consume + Refactor — SUMMARY

## Outcome

O-Lyrica now consumes the shared `note-expression` module produced by plans 23-01 and 23-02. The plugin's source contains zero plugin-local NE support code; voice tuning composes through `Ouaricon::NoteExpression::applyPendingTuning` after `TuningEngine.getFrequency`, with the 128-slot pending table owned by the module's `VST3Extensions` (D-09). `OLyrica_VST3` and `OLyrica_AU` both build cleanly.

## Tasks completed

| # | Task | Commit |
|---|------|--------|
| 1 | Wire OLyrica → note-expression module in CMakeLists | `fee09b6` |
| 2 | Refactor PluginProcessor (.h/.cpp) onto module's VST3Extensions | `e89fdc9` |
| 3 | Refactor HarpSynthVoice (.h/.cpp) onto module's PendingTuningTable + applyPendingTuning | `f667950` |
| 4 | Build verify (OLyrica_VST3 + OLyrica_AU) | (no separate commit — artefacts produced) |

Closeout commit: this SUMMARY + STATE/REQUIREMENTS/ROADMAP updates.

## Key files

- `plugins/O-Lyrica/CMakeLists.txt:79` — `ouaricon_add_module(OLyrica note-expression)` (triggers JUCE-NE-PATCH marker check from plan 23-02)
- `plugins/O-Lyrica/Source/PluginProcessor.h:22,200` — `#include "NoteExpression.h"` + `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;`
- `plugins/O-Lyrica/Source/PluginProcessor.cpp:506,708` — voice receives `vst3Extensions.getPendingTable()`; `processBlock` calls `vst3Extensions.drainAndUpdate()` (replaces ~60-line drain+correlate block)
- `plugins/O-Lyrica/Source/HarpSynthVoice.h:21,88,129` — `#include "NoteExpression.h"`; `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable*)`; `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource`
- `plugins/O-Lyrica/Source/HarpSynthVoice.cpp:84,145` — `setPendingTuningSource` body retyped; `startNote` composes `TuningEngine.getFrequency` → `Ouaricon::NoteExpression::applyPendingTuning` (LYR-02)
- `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` — DELETED (D-16, spike file)

## Build artefacts

- `build/plugins/O-Lyrica/OLyrica_artefacts/Release/VST3/O-Lyrica-dev.vst3` ✓
- `build/plugins/O-Lyrica/OLyrica_artefacts/Release/AU/O-Lyrica-dev.component` ✓

Both built cleanly. Plan 23-04 owns the system install + Dorico smoke test (LYR-03).

## Must-have audit

- ✓ `ouaricon_add_module(OLyrica note-expression)` present in CMakeLists.txt
- ✓ `plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` no longer exists (D-16)
- ✓ Zero `OLyrica::detail::neTrace`, `iidToHex`, `<fstream>`, `OLyrica::detail` references in O-Lyrica source (D-18)
- ✓ Zero references to plugin-local `pendingTuningSemis`, `rawEventScratch`, `OLyrica::LyricaVST3Extensions`
- ✓ `vst3Extensions.drainAndUpdate()` is the only NE drain/correlate call in `processBlock`
- ✓ `Ouaricon::NoteExpression::applyPendingTuning` is the only NE pitch path in voice startup; remaining `std::pow(2.0, semitones/12.0)` calls in HarpSynthVoice.cpp lines 389/391/433/435 belong to **glissando target-frequency math** (chromatic + free glissando direction setup), not NE tuning — out of scope for LYR-02

## Deviations

**D-23-03-A: AU/Standalone link error in NoteExpression.h required Steinberg-symbol guard.**
Discovered during task 4 build verify. The Controller class and parts of VST3Extensions reference `Steinberg::Vst::INoteExpressionController::iid`, `Steinberg::FUnknown::iid`, and `Steinberg::UString::assign` — symbols only linked into the JUCE VST3 client target. Non-VST3 formats (AU, Standalone, VST2, AAX, LV2, Unity) link the shared-code static library but not `pluginterfaces`, so referencing those symbols unconditionally produced "undefined symbols" at AU link.

**Fix:** Guard pluginterfaces includes, the `Controller` class, the `queryIEditController` body, and the `Controller` member of `VST3Extensions` behind `#if JucePlugin_Build_VST3`. Non-VST3 builds get a stripped `VST3Extensions` whose `queryIEditController` returns `kNoInterface` numerically — VST3-only hosts are the only callers anyway, and consumers can still declare a `VST3Extensions` member unconditionally.

**Commit:** `f85ff38` `fix(23-01): guard Steinberg SDK references behind JucePlugin_Build_VST3`. The fix amends plan 23-01's deliverable but the discovery happened in plan 23-03's build gate, so commit lives under 23-01.

**Impact on Phase 24 propagation:** All 7 downstream plugins build AU + VST3 — they would have hit the same linker error. The guard is a one-time fix at the module level; consumers do nothing different.

## Hand-off to plan 23-04

- Version bump in `plugins/O-Lyrica/CMakeLists.txt` from 2.2.x → 2.3.0 (LYR-04)
- CHANGELOG entry documenting shared-module adoption + microtonal NE support (LYR-04)
- Comprehensive `modules/tuning/note-expression/README.md` (MOD-05) — Plan 01 shipped a stub; Plan 04 owns the full content
- macOS Plugin Cache Clearing protocol per CLAUDE.md → install fresh VST3 + AU to system folders
- Dorico quarter-sharp smoke test (LYR-03) — pitch should land at +50¢ above C4 with NE events correlated by `noteId`, no attack zipper

## Self-Check: PASSED
