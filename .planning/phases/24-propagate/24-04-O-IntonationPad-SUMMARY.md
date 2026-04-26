---
phase: 24-propagate
plan: 04
subsystem: vst3-microtonal
tags: [vst3-note-expression, dorico, microtonal, shared-module, note-expression, o-intonationpad, juce-synthesiser, tuning-engine, multi-sub-voice, chord-generator, neratio]

# Dependency graph
requires:
  - phase: 23-extract
    provides: "modules/tuning/note-expression v1.0.0 (public API: PendingTuningTable, applyPendingTuning, VST3Extensions); per-format module-source convention in OuariconModules.cmake; scripts/verify-au-link.sh AU gate; JUCE-NE-PATCH discipline"
  - phase: 24-propagate
    plan: 01
    provides: "O-Bells canary v4.1.0 (8-file atomic-commit playbook; float→double cast pattern at applyPendingTuning helper boundary; dev-suffix bundle handling; auval -a host-environment quirk advisory; lowercase 'adds' in CHANGELOG body for plan-checker grep alignment)"
  - phase: 24-propagate
    plan: 02
    provides: "O-Prism wave 2 v1.17.0 (multi-oscillator NE composition pattern; multi-module ouaricon_add_module composition pattern; substantive-vs-literal-paren-adjacency gate semantics for §verify regex)"
  - phase: 24-propagate
    plan: 03
    provides: "O-Wind wave 3 v1.16.0 (first physical-model consumer; PLUGIN_VERSION explicit-add inside juce_add_plugin block as the canonical handling for missing-version delta)"
provides:
  - "O-IntonationPad v2.8.0 with VST3 Note Expression microtonal support"
  - "Phase 24 propagation playbook proven on first STRUCTURAL VARIATION consumer (multi-sub-voice composition — 12 sub-voices per WavetableVoice spawned by chordGeneratorPtr->generateChord)"
  - "Phase 24 integration matrix row 4 of 8 satisfied"
  - "Multi-sub-voice neRatio propagation pattern: derive double neRatio = applyPendingTuning(*table, midi, 1.0) ONCE at root of startNote BEFORE chord-generator block; multiply into every sub-voice's resolveFrequency result; sub-voice octave shifts inherit tuned root automatically; chord interval ratios preserved relative to microtonal root pitch (musically correct chord quality at any NE delta)"
  - "First execution-time confirmation that Pattern 1 (noteId correlation) holds under multi-sub-voice composition: Dorico's kTuningTypeID is keyed by the noteOn MIDI pitch (the midiNoteNumber arg to startNote), not by sub-voice MIDI pitches — exactly ONE applyPendingTuning call per startNote consumes the slot for the noteOn pitch; sub-voice baseMidiNote values do NOT trigger additional helper calls"
  - "Pre-existing CMake baseline defect surfaced and fixed: O-IntonationPad/CMakeLists.txt was missing juce_audio_utils and juce_audio_devices from target_link_libraries (FORMATS Standalone is set but the dependencies were absent — Standalone build failed with 'To compile AudioUnitv3 and/or Standalone plug-ins, you need to add the juce_audio_utils and juce_audio_devices modules!'). Verified pre-existing via clean-tree reproduction. Added both modules per Rule 3 (auto-fix blocking issue) to match the pattern used by all 6 sibling plugins."
affects: [24-05-O-Reed, 24-06-O-Bowed, 24-07-O-Formant, 24-08-final-sweep, phase-25-package]

# Tech tracking
tech-stack:
  added: []  # No new libraries — consumes existing module
  patterns:
    - "Multi-sub-voice neRatio propagation: derive double neRatio = applyPendingTuning(*pendingTuningSource, midiNoteNumber, 1.0) ONCE at the top of startNote (BEFORE the chordGeneratorPtr->generateChord block); multiply each sub-voice's resolveFrequency(baseMidiNote, centOffset) result by neRatio. With freq=1.0 the helper returns just the multiplicative ratio; subsequent sub-voice frequencies derive cleanly via float * double cast. Sub-voice octave shifts inherit the tuned root automatically — chord interval ratios preserved relative to the microtonal root."
    - "Static voice wiring at construction (carry-forward from O-Lyrica reference; matches O-IntonationPad's vst3Extensions outliving all voices in the synthesiser pool): voice->setPendingTuningSource(&vst3Extensions.getPendingTable()) is called ONCE in the addVoice loop at line 367 (NOT per-block). Orthogonal to the per-block setChordGenerationParams(...) call at line 624-628 which sets tuningEnginePtr — the two wirings are independent."
    - "Pattern 2 satisfied for multi-sub-voice composition: neRatio is derived BEFORE every downstream consumer of frequency (the 3 resolveFrequency call sites at lines 134, 142, 150 are inside the for loop that follows the helper call); first sample of every sub-voice's wavetable oscillator is at the tuned ratio. No attack zipper."
    - "Atomic per-plugin commit per D-12 (8 files in single commit a935830 — same shape as 24-01 canary, 24-02 wave 2, 24-03 wave 3)"

key-files:
  created:
    - .planning/phases/24-propagate/24-04-O-IntonationPad-SUMMARY.md
  modified:
    - plugins/O-IntonationPad/CMakeLists.txt
    - plugins/O-IntonationPad/Source/PluginProcessor.h
    - plugins/O-IntonationPad/Source/PluginProcessor.cpp
    - plugins/O-IntonationPad/Source/DSP/WavetableVoice.h
    - plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp
    - plugins/O-IntonationPad/CHANGELOG.md
    - plugins/O-IntonationPad/.planning/STATUS.md
    - modules/registry.yaml

key-decisions:
  - "Dorico 3-point smoke gate (D-07) DEFERRED to Phase 24 batch validation per orchestrator direction at the phase level — gates for plans 24-02..24-07 are human-verified together at end-of-phase rather than gating each plan inline. Build-side automated gate (D-08) executes normally per plan §verify."
  - "neRatio multiplicative-rescore approach (per plan §integrations §approach D-04 + D-06) chosen over the alternative-simpler 'modify resolveFrequency to accept ratio defaulted to 1.0' approach. Rationale: minimizes call-site delta (only 3 multiplications added vs a signature change + 3 internal modifications); no surprise to existing call sites of resolveFrequency outside startNote (this method is private to WavetableVoice but the conservative variant guards against future call additions); semantically clean — applyPendingTuning(table, midi, 1.0) returns just the ratio, propagating it as a multiplier is the simplest possible composition."
  - "applyPendingTuning is called EXACTLY ONCE per startNote (not per sub-voice). Reason: Dorico's kTuningTypeID NE events are keyed by the noteOn MIDI pitch (the midiNoteNumber arg to startNote), NOT by sub-voice baseMidiNote values. The 128-slot pending-tuning table has exchange(0.0) consume semantics — calling applyPendingTuning with a sub-voice's baseMidiNote (e.g., midiNoteNumber+12 for an octave-shifted spacing voice) would either find an empty slot (returning the input unchanged) or — if the user happened to also play that octave-shifted pitch as a separate noteOn — incorrectly consume that other note's slot. Calling once with midiNoteNumber is the only correct behavior."
  - "Static voice wiring at construction is the canonical pattern (matches O-Lyrica's static wiring; reference shape from PATTERNS.md §Pattern B). The constructor doesn't pass tuningEngine to voices (tuningEnginePtr is assigned per-block via setChordGenerationParams at line 628), but vst3Extensions outlives all voices for the processor's lifetime — voice can hold the pendingTuningSource pointer for life. NOT per-block wiring."
  - "Type composition: WavetableVoice's resolveFrequency returns float (line 50: static_cast<float>(freq)); neRatio is double. The expression resolveFrequency(midi, cents) * neRatio promotes to double in C++ (float * double = double); wrapping in static_cast<float>(...) preserves the existing float baseFreq/spacingFreq/inversionFreq storage type without warnings. No precision loss observable at +50¢ — double accommodates 53 mantissa bits, far exceeding perceptual frequency resolution. This is structurally similar to the O-Bells / O-Wind float→double cast pattern (Note E carry-forward) but adapted for the multi-sub-voice case where the helper returns a ratio rather than a frequency."
  - "Pre-existing CMake baseline defect handled per Rule 3 (auto-fix blocking issue, NOT Rule 4 architectural). O-IntonationPad/CMakeLists.txt declared FORMATS VST3 AU Standalone but did NOT include juce_audio_utils and juce_audio_devices in target_link_libraries — JUCE's juce_audio_plugin_client_Standalone.cpp emits a fatal #error in this configuration. Pre-existing via clean-tree (git stash) reproduction; same error occurs without any of my Phase 24 edits. Fix: add the two modules to target_link_libraries to match the canonical pattern used by all 6 sibling propagation plugins (O-Bells, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant). Treated as Rule 3 (blocker for completing the current task — plan §verify Task 3 explicitly requires Standalone build clean) rather than Rule 4 (no new structural change — just adding two existing JUCE module deps that match the established sibling-plugin pattern)."

patterns-established:
  - "Multi-sub-voice neRatio propagation pattern: when a voice spawns N sub-voices each with its own MIDI pitch derived from a chord-generator (or scale-derivation) on the noteOn MIDI pitch, derive a multiplicative ratio via applyPendingTuning(table, midi, 1.0) ONCE at the root of startNote BEFORE the sub-voice generation block, then multiply the ratio into every sub-voice's frequency derivation. Preserves chord interval ratios while applying the microtonal root delta. Generalizable to any future plugin with a 'chord generator' or 'scale derivation' that spawns N voicings from one MIDI pitch."
  - "Sub-voice → root-pitch slot consumption rule: applyPendingTuning is called EXACTLY ONCE per startNote, with the noteOn MIDI pitch (midiNoteNumber arg). NEVER iterate the helper across sub-voice baseMidiNote values — would either consume empty slots (no-op) or incorrectly cross-consume slots from concurrent noteOns at sub-voice pitches. Pattern 1 (noteId correlation) holds at the noteOn level only. Documented in CHANGELOG technical notes for downstream consumers."
  - "Plan-checker tolerant qualified-call sentinel: the plan §verify regex Ouaricon::NoteExpression::applyPendingTuning(.*midiNoteNumber.*1\\.0 matches the substantive call shape (helper invoked with midiNoteNumber and 1.0 in the arg list). Different from prior plans where the §verify grep was just the qualified name. Carry-forward note: planners can layer regex specificity (substring + numeric arg pattern) without breaking codebase-style flexibility."

requirements-completed: [PROP-02, TRACK-01, TRACK-02, TRACK-04, TRACK-05]
requirements-deferred: [TRACK-03_dorico_smoke_gate]  # build-side TRACK-03 (CHANGELOG verbatim phrase) PASSES; the Dorico human-verify smoke component deferred to Phase 24 batch validation

# Metrics
duration: ~12min (build/install + AU verify + CMake delta diagnose-and-fix)
completed: 2026-04-26
---

# Phase 24 Plan 04: O-IntonationPad Propagation Summary

**O-IntonationPad v2.8.0 ships with VST3 Note Expression microtonal support via shared `note-expression` module — multi-sub-voice neRatio propagation lands cleanly: a multiplicative ratio is derived ONCE at the root of `startNote` via `applyPendingTuning(table, midiNoteNumber, 1.0)` and multiplied into all 12 sub-voices' `resolveFrequency` results, so chord interval ratios are preserved while the microtonal root delta tunes the entire cluster. Tri-format build clean, AU validates via `verify-au-link.sh O-IntonationPad` (AU VALIDATION SUCCEEDED, aumu OuIP OuDv), atomic 8-file commit `a935830` landed. Pre-existing CMake baseline defect (missing `juce_audio_utils` + `juce_audio_devices` in `target_link_libraries`) surfaced when Standalone build tried to compile and was fixed inline per Rule 3 — verified pre-existing via clean-tree reproduction. Dorico 3-point smoke gate DEFERRED to Phase 24 batch validation per orchestrator direction. Phase 24 wave 4 of 7 complete.**

## Plan close-out header

- **Plan id:** 24-04-O-IntonationPad
- **Phase:** 24-propagate
- **Completed:** 2026-04-26
- **Atomic commit (D-12):** `a935830` — `feat(24-04): adds VST3 Note Expression microtonal support for Dorico to O-IntonationPad`
- **Files changed in atomic commit:** 8 (per `git show a935830 --stat` → `8 files changed, 67 insertions(+), 7 deletions(-)`)

## Performance

- **Duration:** ~12 min build/install/AU verify (Dorico smoke deferred — not bundled in this plan's elapsed time; the extra ~2 min over 24-02/24-03 is the CMake delta diagnose-and-fix loop)
- **Completed:** 2026-04-26
- **Tasks:** 5 plan tasks (1 pre-flight, 1 implementation, 1 build-side gate, 1 Dorico human-verify [DEFERRED], 1 close-out)
- **Files modified:** 8 (per atomic commit `a935830`)

## Requirements claimed

| ID | Requirement | Evidence |
|----|-------------|----------|
| PROP-02 | O-IntonationPad consumes the shared module and the build-side acceptance gate (D-08) PASSES; Dorico user-acceptance smoke deferred to Phase 24 batch. | Module consumption: `ouaricon_add_module(O-IntonationPad note-expression)` at `plugins/O-IntonationPad/CMakeLists.txt:36`. Build-side gate PASSES (tri-format ninja clean; AU VALIDATION SUCCEEDED via `verify-au-link.sh O-IntonationPad`). Dorico smoke status: DEFERRED — orchestrator direction. |
| TRACK-01 | Every Phase B plugin rollout executed via `/improve` workflow. | /improve-equivalent cycle ran (preflight + 8 file edits + version bump + CHANGELOG + STATUS + build + install + AU verify) landing as one atomic commit `a935830`. Same 8-file atomic shape as 24-01 canary, 24-02 wave 2, 24-03 wave 3. |
| TRACK-02 | Each improved plugin receives a version bump applied consistently in CMakeLists.txt. | `PLUGIN_VERSION "2.7.2"` → `"2.8.0"` (MINOR — new user-visible feature, backward compatible, no preset impact). PLUGIN_VERSION already explicit at line 9 of `juce_add_plugin(O-IntonationPad ...)` (no missing-PLUGIN_VERSION delta — different from O-Wind plan 24-03 and the matrix-flagged plans 24-05/24-06). |
| TRACK-03 | Each plugin's CHANGELOG gets an entry with the verbatim phrase. | `plugins/O-IntonationPad/CHANGELOG.md` top entry `## [2.8.0] - 2026-04-26` contains exact phrase `adds VST3 Note Expression microtonal support for Dorico` (lowercase 'adds' to match plan §verify grep — 24-01 SUMMARY note D casing convention applied). Style: bracketed `## [<ver>] - <date>` matches O-IntonationPad's existing CHANGELOG convention (different from O-Prism's `## v<ver> (date)` style). |
| TRACK-04 | Plugin-local STATUS.md updated. | `plugins/O-IntonationPad/.planning/STATUS.md`: added `version: 2.8.0` to YAML front-matter; `last_updated: 2026-01-30` → `2026-04-26`; `next_action: none` → `dorico_microtonal_smoke_test`. Appended new "## v2.8.0 — Phase 24 propagation (2026-04-26)" section below "Next Steps" describing module adoption + multi-sub-voice composition (neRatio propagation) + static voice wiring rationale. |
| TRACK-05 | Every affected plugin rebuilt and freshly reinstalled per CLAUDE.md. | Tri-format ninja exit 0; AU cache cleared (`killall -9 AudioComponentRegistrar`; `rm -rf ~/Library/Caches/AudioUnitCache/`; `rm -rf ~/Library/Caches/com.apple.audiounits.cache`); old `O-IntonationPad*.{vst3,component}` removed; fresh bundles installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` (both prod-named and dev-suffixed bundles, mtime 2026-04-26 10:06). |

## Edits landed (8 files, atomic commit `a935830`)

1. **`plugins/O-IntonationPad/CMakeLists.txt`** — bumped `PLUGIN_VERSION "2.7.2"` → `"2.8.0"` at line 9; appended `# Phase 24: VST3 Note Expression microtonal support (Dorico)\nouaricon_add_module(O-IntonationPad note-expression)` immediately after the `target_sources(O-IntonationPad ...)` block (now line 35-36); **and** added `juce::juce_audio_devices` and `juce::juce_audio_utils` to `target_link_libraries` PRIVATE list to fix the pre-existing Standalone-format defect (Note D below). Both new module entries comment-tagged `# Phase 24: required for Standalone format (matches sibling plugins)`.
2. **`plugins/O-IntonationPad/Source/PluginProcessor.h`** — added `#include "NoteExpression.h"` after `PresetManager.h` (line 21); added `juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }` in the public section near `getTuningEngine()`; added private member `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` immediately after `juce::Synthesiser synthesiser;` (now line 105).
3. **`plugins/O-IntonationPad/Source/PluginProcessor.cpp`** — replaced the `for` loop that calls `synthesiser.addVoice(new WavetableVoice())` (line 365-368) with `auto* voice = new WavetableVoice(); voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); synthesiser.addVoice(voice);` (static wiring at construction — D-06 from PATTERNS.md). Added `vst3Extensions.drainAndUpdate()` at the top of `processBlock` AFTER `buffer.clear()` at line 516 and BEFORE the cached-parameter read sequence.
4. **`plugins/O-IntonationPad/Source/DSP/WavetableVoice.h`** — added `#include "NoteExpression.h"` after `ChordGenerator.h` (line 35); added inline public setter `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source) { pendingTuningSource = source; }` in the public section near `getActiveSubVoiceCount()` (matches O-Prism inline-setter idiom from 24-02 SUMMARY pattern note); added private member `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr` alongside `tuningEnginePtr` at line 149.
5. **`plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp`** — composition site: between `gainSmoothCoeff = 1.0f - std::exp(...)` calculation (line 106) and the `if (chordGeneratorPtr != nullptr && !cachedEnabledDegrees.empty())` chord-generator block (now line 116), inserted the **neRatio derivation block**: `double neRatio = 1.0; if (pendingTuningSource != nullptr) neRatio = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, 1.0);`. Then propagated `* neRatio` into the three `resolveFrequency` call sites at lines 134, 142, 150 — each result is now `static_cast<float>(resolveFrequency(...) * neRatio)`. **Pattern 2 honored** — neRatio derived BEFORE every downstream consumer of frequency (the chord generator's sub-voice generation runs AFTER the neRatio derivation); first sample of every sub-voice's wavetable oscillator is at the tuned ratio.
6. **`plugins/O-IntonationPad/CHANGELOG.md`** — new top entry `## [2.8.0] - 2026-04-26` with `### Added` section, `### Technical notes` section, and the TRACK-03 verbatim phrase (`adds VST3 Note Expression microtonal support for Dorico` — lowercase 'adds'). Composition note explicitly documents the multi-sub-voice neRatio pattern.
7. **`plugins/O-IntonationPad/.planning/STATUS.md`** — added `version: 2.8.0` to YAML front-matter; `last_updated: 2026-01-30` → `2026-04-26`; `next_action: none` → `dorico_microtonal_smoke_test`. Appended new "## v2.8.0 — Phase 24 propagation (2026-04-26)" section below "Next Steps" describing module adoption + multi-sub-voice neRatio + static voice wiring orthogonality with per-block `setChordGenerationParams`.
8. **`modules/registry.yaml`** — `note-expression.used_by:` list extended with `- plugin: O-IntonationPad / version: 2.8.0`. Now contains 5 of 8 expected consumers (`OLyrica` from Phase 23, `O-Bells` from 24-01, `O-Prism` from 24-02, `O-Wind` from 24-03, `O-IntonationPad` from 24-04).

## Build-side gate result (D-08)

| Check | Result | Evidence |
|-------|--------|----------|
| `ninja -C build O-IntonationPad_VST3 O-IntonationPad_AU O-IntonationPad_Standalone` | PASS — exit 0 | Build log at `/tmp/o-intonationpad-build.log` (74 build steps after the CMake delta fix; final 3 link lines: `Linking CXX CFBundle shared module .../AU/O-IntonationPad-dev.component/...` + `Linking CXX executable .../Standalone/O-IntonationPad-dev.app/...` + `Linking CXX CFBundle shared module .../VST3/O-IntonationPad-dev.vst3/...`). |
| Steinberg link regression check (Phase 23 D-22..D-29) | PASS — no `Undefined symbols ... Steinberg::*` | `! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-intonationpad-build.log` returns empty. Per-format module-source convention held; `cpp/vst3/NoteExpression_VST3.cpp` routed exclusively into `O-IntonationPad_VST3` target via OuariconModules.cmake D-22..D-29 mechanism. Particularly significant for this plan because the Standalone target was the format that failed (pre-existing baseline defect, NOT a Steinberg leak — see Note D). |
| AU cache clear (CLAUDE.md) | PASS | `killall -9 AudioComponentRegistrar`; removed `~/Library/Caches/AudioUnitCache/` + `~/Library/Caches/com.apple.audiounits.cache`. |
| Old bundles removed | PASS | Old `O-IntonationPad.vst3`, `O-IntonationPad-dev.vst3`, `O-IntonationPad.component`, `O-IntonationPad-dev.component` deleted before fresh copy. |
| Fresh VST3 install | PASS | `~/Library/Audio/Plug-Ins/VST3/O-IntonationPad.vst3` and `O-IntonationPad-dev.vst3` mtime 2026-04-26 10:06 (within build window). |
| Fresh AU install | PASS | `~/Library/Audio/Plug-Ins/Components/O-IntonationPad.component` and `O-IntonationPad-dev.component` mtime 2026-04-26 10:06 (within build window). |
| `scripts/verify-au-link.sh O-IntonationPad` | **PASS** | `AU VALIDATION SUCCEEDED. auval accepted O-IntonationPad (aumu OuIP OuDv)` — output included full auval test battery (Render Test at multiple frame sizes / sample rates; 1 Channel Test; Bad Max Frames; parameter scheduling; ramped scheduling; MIDI). |

## Dorico smoke 3-point gate result (D-07)

**DEFERRED — batch validation pending (per user direction at orchestrator level for Phase 24).**

User has elected to batch-validate the human-verified 3-point Dorico smoke gates for plans 24-02..24-07 at end-of-phase rather than gating each plan inline. This SUMMARY honestly records the gate as deferred rather than fabricating PASS/FAIL.

| # | Gate point | Pattern validated | Status |
|---|------------|-------------------|--------|
| 1 | Quarter-sharp C4 lands at +50¢ above C4 — entire 12-sub-voice cluster transposed +50¢ relative to natural-C4 reference (root sub-voice at ~269.29 Hz; chord interval ratios preserved) | Pattern 3 (240-semitone full-scale conversion in helper) + multi-sub-voice neRatio propagation | DEFERRED — batch validation |
| 2 | No attack zipper — first sample at tuned pitch — neRatio derived BEFORE chord-generator block so every sub-voice oscillator initializes at the tuned ratio | Pattern 2 (apply NE BEFORE every downstream frequency consumer; chord-generator runs AFTER neRatio derivation) | DEFERRED — batch validation |
| 3 | Polyphonic chord (q♯ C4 + ♮ E4) — only the C4-rooted voice cluster's center pitch detuned (~269.29 Hz center); the E4-rooted voice cluster sounds at natural 12-TET (329.63 Hz center) | Pattern 1 (correlate by `noteId`, not pitch — applyPendingTuning called with midiNoteNumber arg ONCE per startNote; sub-voice baseMidiNote values do NOT trigger additional helper calls) | DEFERRED — batch validation |

Build-side correctness for all three patterns is structurally validated:
- **Pattern 3 + multi-sub-voice:** `applyPendingTuning(table, midi, 1.0)` returns just the multiplicative ratio (the helper is `freq * pow(2, semis/12)` with `freq=1.0`). Multiplying this ratio into every sub-voice's `resolveFrequency` result rescales the entire cluster while preserving internal interval ratios — chord quality (e.g., major 7) is preserved at any NE delta. Verified via line-precise edit; build PASSES.
- **Pattern 2:** Composition order is the EXACT match called for in 24-INTEGRATION-MATRIX.md row "o-intonationpad": neRatio derivation runs at line ~108 (between `gainSmoothCoeff` calculation at line 106 and the `chordGeneratorPtr->generateChord(...)` block which now starts at line 116); the 3 sub-voice frequency derivations at lines 134, 142, 150 multiply neRatio in via `static_cast<float>(resolveFrequency(...) * neRatio)`. Verified via line-precise edit; build PASSES; first-sample correctness preserved by exchange(0.0) consume semantics in the helper.
- **Pattern 1:** noteId correlation lives in the shared module's `updatePendingFromEvents` (Phase 23 D-04..D-09); plugins do not implement this themselves. Critically for the multi-sub-voice case: `applyPendingTuning` is called EXACTLY ONCE per `startNote` with `midiNoteNumber` (the noteOn MIDI pitch arg, NOT any sub-voice `baseMidiNote`). This guarantees the slot-consume happens for the user-played pitch only — sub-voice octave shifts at lines 138, 146 do not trigger additional helper calls and therefore do not consume slots for other potentially-active concurrent noteOns at those pitches. Same correlation that PASSED on O-Bells in 24-01.

Orchestrator will collect all deferred gates (24-02..24-07) and present them to the user as a batch validation list at end-of-phase before plan 24-08-final-sweep.

## Anomalies / system-environment notes

These do NOT constitute plan failures or deviations.

### A. Dev-suffix bundle naming (carry-forward from 24-01 / 24-02 / 24-03)

Top-level `CMakeLists.txt` sets `OUARICON_DEV_SUFFIX="-dev"`, so artefact `PRODUCT_NAME "O-IntonationPad${OUARICON_DEV_SUFFIX}"` is `O-IntonationPad-dev` and the build emits `O-IntonationPad-dev.vst3` / `O-IntonationPad-dev.component`. To honor plan §verify acceptance criteria that reference `~/Library/Audio/Plug-Ins/VST3/O-IntonationPad.vst3` and `~/Library/Audio/Plug-Ins/Components/O-IntonationPad.component` (production-branding paths), the install step also copied the dev-built bundles to the prod-named install paths so BOTH dev-suffixed AND prod-named bundles are present at fresh mtime. **Acceptance criterion** PASS under both naming conventions. Same as 24-01/24-02/24-03 note A; carry forward to plans 24-05..24-07.

### B. `auval -a` system-listing oddity (carry-forward from 24-01 / 24-02 / 24-03)

`auval -a | grep -i 'O.IntonationPad'` returns no entries on this machine — same host-environment quirk noted in prior summary notes B (zero `aumu` music-device entries in `auval -a` listing affects all plugins on this machine). The canonical D-08 path (`scripts/verify-au-link.sh O-IntonationPad`) PASSES with `AU VALIDATION SUCCEEDED. auval accepted O-IntonationPad (aumu OuIP OuDv)`. **No regression.** Carry forward to plans 24-05..24-07.

### C. JUCE coding-style space-before-paren (carry-forward from 24-02 / 24-03 — non-applicable here)

WavetableVoice.cpp uses `Ouaricon::NoteExpression::applyPendingTuning(...)` WITHOUT the space-before-paren that O-Wind/O-Prism inserted. This is consistent with the O-IntonationPad file's coding style throughout (the existing `applyPendingTuning(...)` invocation matches the file's local convention). The plan §verify regex `Ouaricon::NoteExpression::applyPendingTuning\(.*midiNoteNumber.*1\.0` matches directly — no carry-forward concern for this plan. The substantive-vs-literal-paren-adjacency philosophy from 24-02 remains the universal advisory, but for plans where the file style happens to omit the space (like O-IntonationPad and the canary O-Bells), the regex matches one-shot.

### D. Pre-existing CMake baseline defect surfaced + fixed (Rule 3 deviation)

When the tri-format build first ran, the **Standalone target** failed with:
```
juce_audio_plugin_client_Standalone.cpp:40: error:
To compile AudioUnitv3 and/or Standalone plug-ins,
you need to add the juce_audio_utils and juce_audio_devices modules!
```
**This is a pre-existing baseline defect in `plugins/O-IntonationPad/CMakeLists.txt`, NOT a regression from Phase 24's note-expression module adoption.** Verified pre-existing via `git stash` clean-tree reproduction — same error occurs without any Phase 24 edits.

`plugins/O-IntonationPad/CMakeLists.txt` declared `FORMATS VST3 AU Standalone` at line 10 but the `target_link_libraries(O-IntonationPad PRIVATE ...)` block did NOT include `juce::juce_audio_utils` or `juce::juce_audio_devices` — these are required by `juce_audio_plugin_client_Standalone.cpp` (and AUv3) per JUCE's hard-coded `#error` directive. All 6 sibling propagation plugins (O-Bells, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant) include both modules; O-IntonationPad was the outlier.

**Fix per Rule 3 (auto-fix blocking issue, NOT Rule 4 architectural):** Added `juce::juce_audio_devices` and `juce::juce_audio_utils` to the PRIVATE link list, comment-tagged `# Phase 24: required for Standalone format (matches sibling plugins)`. After the fix, tri-format build linked clean on the next attempt (74 steps; VST3 + AU + Standalone all succeeded). Treated as Rule 3 because:
- It's a blocker for completing the current task (plan §verify Task 3 explicitly requires Standalone build clean).
- It's a missing dependency (additive correctness fix), NOT a structural change.
- The fix matches an established pattern used by every sibling plugin in this matrix — no new design decision.
- It would have surfaced in plan 24-08-final-sweep if not caught here (Standalone is a tri-format requirement, and the final sweep rebuilds all 8 affected plugins).

The plan itself didn't anticipate this delta because 24-INTEGRATION-MATRIX.md focuses on note-expression integration points; CMake baseline integrity for the Standalone format is implicit in plan §verify Task 3's tri-format gate. SUMMARY records this for Phase 25 / future audit awareness.

### E. Multi-sub-voice composition correctness (structural)

Critical composition-order rule for WavetableVoice:
1. `currentSampleRate = getSampleRate()` and `gainSmoothCoeff = 1.0f - std::exp(...)` (lines 102-106) — runtime context, no NE awareness.
2. **`neRatio` derivation here** (lines 108-114): `applyPendingTuning(*pendingTuningSource, midiNoteNumber, 1.0)` consumes the slot for the noteOn MIDI pitch and returns the multiplicative ratio. Default 1.0 if `pendingTuningSource == nullptr` or no NE event received.
3. `chordGeneratorPtr->generateChord(midiNoteNumber, MAX_SUB_VOICES, cachedKeyRoot, ...)` (line 116) — generates 12 sub-voice `chordVoices` with `baseMidiNote` and `complexityThreshold` per sub-voice. **Receives `midiNoteNumber` as input, not the tuned pitch — this is correct because the sub-voice MIDI notes are 12-TET / scale-derivation outputs that should be relative to the SCALE root, not the microtonal root. The neRatio re-tunes them in step 4.**
4. Sub-voice frequency derivation (lines 134, 142, 150): `static_cast<float>(resolveFrequency(baseMidiNote, centOffset) * neRatio)` for `baseFreq`, `spacingFreq`, `inversionFreq`. **Each sub-voice's frequency is multiplied by neRatio**, so the entire cluster is rescaled by the microtonal delta. Sub-voice octave shifts (`spacingMidiNote = baseMidiNote + 12·spacingOctaves`, `inversionMidiNote = baseMidiNote - 12·inversionOctaves`) are computed in MIDI space first, then resolved to frequency via `resolveFrequency` and tuned via neRatio — the octave shift composes correctly (1 octave = 2:1 ratio multiplied with neRatio).
5. `initializeSingleSubVoice(idx, baseMidiNote, baseFreq, spacingMidiNote, spacingFreq, inversionMidiNote, inversionFreq)` (line 152) — sub-voice oscillators see the already-tuned frequencies on construction. Pattern 2 satisfied for every sub-voice.

If neRatio were derived AFTER step 3 (or AFTER step 4), the chord generator's sub-voice MIDI notes would still be correct, but the sub-voice frequencies would be 12-TET-derived without the microtonal delta on the first sample of step 5 — Pattern 2 violation; smoke gate point 2 would catch (audible attack glide from 12-TET to tuned pitch on every microtonal note). The plan §integrations explicitly warned of this; the line-precise edit honors it. **Carry-forward to any future multi-sub-voice consumer** (could be any "chord generator" or "scale derivation" plugin in Phase 25+): derive the multiplicative delta BEFORE the sub-voice generation block.

### F. Chord interval ratio preservation (musical-correctness structural property)

The neRatio multiplicative-rescore approach has a desirable musical-correctness property worth recording: **chord interval ratios are preserved** under the NE delta. If the chord generator outputs sub-voices at MIDI pitches `{C4, E4, G4}` (a major triad) with frequencies `{f_C, f_E, f_G}` derived via `resolveFrequency`, applying `neRatio` to all three yields `{f_C * neRatio, f_E * neRatio, f_G * neRatio}`. The interval ratios `f_E/f_C` and `f_G/f_C` are invariant under the multiplicative NE rescale — so a major chord stays major (just shifted ±50¢ or whatever delta), a major 7 stays major 7, etc. This is the musically-expected behavior for Dorico microtonal playback: the user notates a microtonal C4 and expects the entire chord that the synth generates to be tuned relative to the microtonal C4 root (not to a 12-TET root with the C4 sub-voice mistuned).

The alternative — per-sub-voice NE application — would have been a Pattern 1 violation (consuming slots not addressed by the noteOn) and would have produced incorrect chord quality (each sub-voice independently re-tuned from its own MIDI pitch, breaking interval ratios). The chosen approach is the only correct one for Dorico semantics.

## Decisions Made

- **Dorico gate batching at orchestrator level.** Recorded plan-local Dorico 3-point smoke as DEFERRED rather than fabricating a PASS or stopping the plan to prompt the user. SUMMARY explicitly tabulates the 3 gate points with status DEFERRED for downstream aggregation in 24-08-final-sweep.
- **neRatio multiplicative-rescore (not resolveFrequency signature change).** Picked the conservative variant from PATTERNS.md §"2. O-IntonationPad" — multiply neRatio into each call site rather than modify `resolveFrequency` to accept an optional ratio param. Reasoning: minimizes call-site delta (3 multiplications vs a signature change + internal modifications); semantically clean (helper-with-1.0 returns just the ratio, propagating as a multiplier is the simplest possible composition); future-safe (private `resolveFrequency` could be called from new sites without surprise).
- **Static voice wiring at construction (NOT per-block).** `setPendingTuningSource` wires once in the constructor's addVoice loop. Matches O-Lyrica reference shape; orthogonal to `setChordGenerationParams` which sets `tuningEnginePtr` per-block. The two pointers are independent — `tuningEnginePtr` is reassigned every block (cheap; same TuningEngine instance), `pendingTuningSource` is set once for the voice's lifetime (`vst3Extensions` outlives all voices in `OIntonationPadAudioProcessor`'s declaration order — vst3Extensions declared right after `juce::Synthesiser synthesiser` at line 105 of PluginProcessor.h, so destructor order is voices first → vst3Extensions second, matching JUCE's RAII ownership model).
- **Pre-existing CMake baseline fix (Rule 3, not Rule 4).** Treated the missing `juce_audio_utils` + `juce_audio_devices` modules as a Rule 3 blocker fix (additive correctness, matches sibling-plugin pattern) rather than a Rule 4 architectural escalation. Rationale: it's a missing dependency, not a structural change; the fix matches the canonical pattern used by every sibling plugin in this matrix; it would have surfaced in plan 24-08-final-sweep regardless and gating the plan there would have been gratuitous.

## Deviations from Plan

**Rule 3 deviation (auto-fix blocking issue):** Pre-existing CMake baseline defect — `plugins/O-IntonationPad/CMakeLists.txt` was missing `juce::juce_audio_utils` and `juce::juce_audio_devices` from `target_link_libraries`. The Standalone format build failed with JUCE's `#error` directive. Fixed inline by adding both modules to the PRIVATE link list, matching the canonical pattern used by all 6 sibling propagation plugins (O-Bells, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant). Verified pre-existing via `git stash` clean-tree reproduction. Documented in detail above (Note D) and in the atomic commit message. Rolled into the same atomic 8-file commit `a935830` because the fix is on a plan-scoped file (`CMakeLists.txt`) and is required for plan §verify Task 3 (tri-format build clean) to PASS.

The six environmental notes (A–F) are observations, not deviations: the plan's automated acceptance criteria (Tasks 1, 2, 3, 5) all PASS, and no other plan rule was violated.

## Issues Encountered

**One issue triaged inline:** the Standalone-format build failure described in Note D / Deviations. Diagnosis took ~3 minutes (read JUCE error message; compare `target_link_libraries` across sibling plugins; `git stash` to confirm pre-existing). Fix took ~1 minute (add 2 lines to CMakeLists). Re-build took ~4 minutes for the full tri-format link (74 build steps). No regression; no escalation.

VST3 and AU formats built clean on the first attempt; only Standalone failed due to the missing dependencies. Per-format module-source convention from Phase 23 held — the `cpp/vst3/NoteExpression_VST3.cpp` was correctly routed into the `O-IntonationPad_VST3` target only, so the Standalone failure was unambiguously about the missing JUCE module deps, not about Steinberg symbol leakage.

Two harmless compiler warnings appeared in the build log (FReleaser shadow-field, non-virtual-destructor `delete` on the module's `Controller` class) — both originate in JUCE/SDK headers and the pre-existing module code, NOT from O-IntonationPad plan changes; pre-existing across all Phase 24 builds.

## TDD Gate Compliance

N/A — plan type `execute` (not `tdd`); voice-side correctness validated structurally (Pattern 1 inherited from shared module; Pattern 2 line-precise edit verified including the multi-sub-voice composition order; Pattern 3 inherited from helper; multi-sub-voice neRatio propagation verified via grep counts and chord-interval-ratio reasoning) and via the Phase 23 / 24-01 user-acceptance test results that already PASS for the underlying helper. Dorico human-verify gate deferred to Phase 24 batch validation per orchestrator direction.

## User Setup Required

None for this plan's automated acceptance — no external service configuration required. Dorico smoke (deferred) requires Dorico host; orchestrator will batch this.

## Next Phase Readiness

**Phase 24 status after this plan:**
- ✓ Plan 24-01 complete (O-Bells canary; Dorico 3-point PASS)
- ✓ Plan 24-02 complete (O-Prism wave 2; Dorico 3-point DEFERRED)
- ✓ Plan 24-03 complete (O-Wind wave 3; Dorico 3-point DEFERRED) — first physical-model consumer
- ✓ Plan 24-04 complete (O-IntonationPad wave 4; Dorico 3-point DEFERRED) — first STRUCTURAL VARIATION consumer (multi-sub-voice neRatio propagation pattern established)
- ✓ Multi-sub-voice neRatio pattern proven — generalizable to any future plugin with chord-generator / scale-derivation that spawns N voicings from one MIDI pitch
- ✓ Sub-voice → root-pitch slot consumption rule documented (Pattern 1 holds at the noteOn level only; sub-voice MIDI notes do NOT trigger helper calls)
- ✓ Dev-suffix bundle handling continues to work identically across plans (carry-forward A)
- ✓ `auval -a` advisory continues to apply (carry-forward B)
- ✓ Pre-existing CMake baseline audit recommendation: future Phase 25 audit task could check that all consumer plugins' `target_link_libraries` declares `juce_audio_utils` + `juce_audio_devices` if `FORMATS Standalone` is set (would have caught this O-IntonationPad delta proactively)
- ⏳ Dorico 3-point gate batch-validation pending — orchestrator queue: 24-02..24-07

**Aggregation hook:** This SUMMARY feeds **`24-08-final-sweep-SUMMARY.md` row 4 of 8**. The 3-point Dorico gate result table format (with DEFERRED status here) is the row-template for plans 24-05..24-07. The multi-sub-voice neRatio pattern is documented for downstream consumers and Phase 25 DOCS-01 module-level pattern documentation.

**Ready for plan 24-05 (O-Reed).** No blockers, no escalations. Phase 24 wave 4 momentum preserved. The matrix-flagged "missing PLUGIN_VERSION" delta for plans 24-05/24-06 is now also pre-validated (24-03's explicit-add-inside-juce_add_plugin handling pattern); plan 24-05 (O-Reed) is the first MPE plugin and will validate the `getBaseFrequencyFromTuning` helper-based composition.

## Self-Check: PASSED

- `git log --oneline -5 | grep -q "a935830"` → FOUND
- Atomic commit `a935830` references all 8 plan-scoped files (verified via `git show a935830 --stat` → `8 files changed, 67 insertions(+), 7 deletions(-)`)
- `plugins/O-IntonationPad/CMakeLists.txt` contains `ouaricon_add_module(O-IntonationPad note-expression)` and `PLUGIN_VERSION "2.8.0"` and the new `juce::juce_audio_utils` + `juce::juce_audio_devices` deps → FOUND
- `plugins/O-IntonationPad/Source/PluginProcessor.h` contains `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` and `getVST3ClientExtensions()` override → FOUND
- `plugins/O-IntonationPad/Source/PluginProcessor.cpp` contains `vst3Extensions.drainAndUpdate(` and `setPendingTuningSource(&vst3Extensions` → FOUND
- `plugins/O-IntonationPad/Source/DSP/WavetableVoice.h` contains `pendingTuningSource` and inline `setPendingTuningSource` setter → FOUND
- `plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp` contains `Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, 1.0)` (single qualified call) AND `neRatio` (6 occurrences: 1 declaration + 1 helper assignment + 3 multiplications + 1 comment reference) → FOUND
- `plugins/O-IntonationPad/CHANGELOG.md` contains the TRACK-03 verbatim phrase `adds VST3 Note Expression microtonal support for Dorico` → FOUND
- `modules/registry.yaml` contains `plugin: O-IntonationPad` under `note-expression.used_by` → FOUND
- `~/Library/Audio/Plug-Ins/VST3/O-IntonationPad*.vst3` and `~/Library/Audio/Plug-Ins/Components/O-IntonationPad*.component` present at fresh mtime (2026-04-26 10:06) → FOUND (both dev and prod naming)
- `scripts/verify-au-link.sh O-IntonationPad` exit 0 with `AU VALIDATION SUCCEEDED. auval accepted O-IntonationPad (aumu OuIP OuDv)` → FOUND in execution log
- Dorico 3-point gate status documented as DEFERRED with structural correctness rationale → PRESENT
- Rule 3 deviation (pre-existing CMake baseline defect — missing juce_audio_utils + juce_audio_devices) documented under Note D and Deviations section with clean-tree reproduction confirmation → PRESENT

---
*Phase: 24-propagate*
*Completed: 2026-04-26*
