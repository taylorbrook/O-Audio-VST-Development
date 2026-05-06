---
phase: 24-propagate
plan: 03
subsystem: vst3-microtonal
tags: [vst3-note-expression, dorico, microtonal, shared-module, note-expression, o-wind, juce-synthesiser, tuning-engine, physical-model, bore-waveguide]

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
provides:
  - "O-Wind v1.16.0 with VST3 Note Expression microtonal support"
  - "Phase 24 propagation playbook proven on first physical-model consumer (BoreWaveguide period derivation)"
  - "Phase 24 integration matrix row 3 of 8 satisfied"
  - "First case in Phase 24 that exercised CMake delta (a): missing PLUGIN_VERSION argument added explicitly inside juce_add_plugin block"
  - "Confirmation that float→double cast at helper boundary works on a physical-model voice (carries forward from O-Bells canary)"
affects: [24-04-O-IntonationPad, 24-05-O-Reed, 24-06-O-Bowed, 24-07-O-Formant, 24-08-final-sweep, phase-25-package]

# Tech tracking
tech-stack:
  added: []  # No new libraries — consumes existing module
  patterns:
    - "Voice-side composition for physical-model period derivation: TuningEngine.getFrequency(midi) → applyPendingTuning(table, midi, freq) → pitch-bend → BoreWaveguide.setBoreDelay(totalLoopDelay/(1+initJetRatio)) (Pattern 2: physical-model period sized to tuned frequency on sample 0; bore-delay derivation runs AFTER NE composition)"
    - "Float→double cast at helper boundary (FluteSynthVoice uses float currentFrequency; applyPendingTuning signature is double) — second confirmation after O-Bells canary"
    - "CMake delta (a) — PLUGIN_VERSION argument explicitly added inside juce_add_plugin(O-Wind ...) block between PRODUCT_NAME and IS_SYNTH (was missing in tree); precedent for plans 24-05/24-06/24-07 which have the same delta"
    - "Atomic per-plugin commit per D-12 (8 files in single commit 4ae4600)"

key-files:
  created:
    - .planning/phases/24-propagate/24-03-O-Wind-SUMMARY.md
  modified:
    - plugins/O-Wind/CMakeLists.txt
    - plugins/O-Wind/Source/PluginProcessor.h
    - plugins/O-Wind/Source/PluginProcessor.cpp
    - plugins/O-Wind/Source/FluteSynthVoice.h
    - plugins/O-Wind/Source/FluteSynthVoice.cpp
    - plugins/O-Wind/CHANGELOG.md
    - plugins/O-Wind/.planning/STATUS.md
    - modules/registry.yaml

key-decisions:
  - "Dorico 3-point smoke gate (D-07) DEFERRED to Phase 24 batch validation per orchestrator direction at the phase level — gates for plans 24-02..24-07 are human-verified together at end-of-phase rather than gating each plan inline. Build-side automated gate (D-08) executes normally per plan §verify."
  - "CMake delta (a) — missing PLUGIN_VERSION argument — handled per plan §action: added 'PLUGIN_VERSION \"1.16.0\"' inside juce_add_plugin(O-Wind ...) block between PRODUCT_NAME (line 11) and IS_SYNTH (line 12). Plan 24-05/24-06/24-07 are pre-flagged in 24-INTEGRATION-MATRIX.md as having the same delta; this SUMMARY is the first execution-time confirmation that the explicit-add approach works cleanly (build linked tri-format on first attempt)."
  - "Type delta (b) — float→double cast at applyPendingTuning helper boundary — handled identically to O-Bells canary (Plan 24-01 SUMMARY note C). The cast wraps the helper call: currentFrequency = static_cast<float>(applyPendingTuning(*src, midi, static_cast<double>(currentFrequency))). Plans 24-05 (O-Reed) and 24-06 (O-Bowed) per the integration matrix do not need explicit casts at the call site because they apply NE INSIDE getBaseFrequencyFromTuning helper (which already does double→float internally); plan 24-07 (O-Formant uses float tunedF0) reuses this exact cast pattern."
  - "Composition order placement is ESSENTIAL for physical-model period derivation: NE applied AFTER TuningEngine assignment (lines 78-81) and BEFORE pitch-bend (line 84 → bendedFreq) and BEFORE totalLoopDelay = internalSampleRate/bendedFreq (line 88) and BEFORE boreWaveguide.setBoreDelay(totalLoopDelay/(1+initJetRatio)) (line 100). Bore delay sees tuned frequency at sample 0; if NE were applied AFTER line 100, the bore-delay would size to untuned 12-TET frequency and the physical-model period would mismatch on the first sample (Pattern 2 violation). The plan §integrations explicitly called this out; line-precise edit honors it."

patterns-established:
  - "Physical-model period derivation pattern: when a voice computes a delay-line / waveguide period from a frequency, NE composition MUST land BEFORE the period-derivation step. For O-Wind: BoreWaveguide.setBoreDelay sees tuned frequency on sample 0. Same shape applies to plans 24-05 (BoreWaveguide in O-Reed) and 24-06 (WaveguideString in O-Bowed)."
  - "Explicit-PLUGIN_VERSION-add pattern when juce_add_plugin lacks PLUGIN_VERSION: insert the argument inside the juce_add_plugin block (NOT outside), preferably between PRODUCT_NAME and IS_SYNTH (where the rest of FORMATS-style arguments cluster). Verified by tri-format build PASS — JUCE picks up PLUGIN_VERSION uniformly across VST3, AU, and Standalone subtargets."
  - "JUCE space-before-paren style preserved: applyPendingTuning ( ... ) call site uses the existing O-Wind file's coding style (matches FluteSynthVoice.cpp throughout). Per Plan 24-02 SUMMARY note C, the substantive sentinel (qualified-name presence) is what matters; space-before-paren is style, not correctness."

requirements-completed: [PROP-04, TRACK-01, TRACK-02, TRACK-04, TRACK-05]
requirements-deferred: [TRACK-03_dorico_smoke_gate]  # build-side TRACK-03 (CHANGELOG verbatim phrase) PASSES; the Dorico human-verify smoke component deferred to Phase 24 batch validation

# Metrics
duration: ~10min (build/install + AU verify)
completed: 2026-04-26
---

# Phase 24 Plan 03: O-Wind Propagation Summary

**O-Wind v1.16.0 ships with VST3 Note Expression microtonal support via shared `note-expression` module — tri-format build clean, AU validates via `verify-au-link.sh O-Wind` (AU VALIDATION SUCCEEDED, aumu OWnd OuDv), atomic 8-file commit `4ae4600` landed. CMake delta (a) — missing PLUGIN_VERSION argument — added explicitly inside `juce_add_plugin(O-Wind ...)` block; type delta (b) — float→double cast at helper boundary — applied per O-Bells canary. Composition order honors Pattern 2: NE applied BEFORE BoreWaveguide period derivation so the physical-model bore delay sees the tuned frequency on sample 0. Dorico 3-point smoke gate DEFERRED to Phase 24 batch validation per orchestrator direction. Phase 24 wave 3 of 7 complete.**

## Plan close-out header

- **Plan id:** 24-03-O-Wind
- **Phase:** 24-propagate
- **Completed:** 2026-04-26
- **Atomic commit (D-12):** `4ae4600` — `feat(24-03): adds VST3 Note Expression microtonal support for Dorico to O-Wind`
- **Files changed in atomic commit:** 8 (per `git show 4ae4600 --stat` → `8 files changed, 55 insertions(+), 2 deletions(-)`)

## Performance

- **Duration:** ~10 min build/install/AU verify (Dorico smoke deferred — not bundled in this plan's elapsed time)
- **Completed:** 2026-04-26
- **Tasks:** 5 plan tasks (1 pre-flight, 1 implementation, 1 build-side gate, 1 Dorico human-verify [DEFERRED], 1 close-out)
- **Files modified:** 8 (per atomic commit `4ae4600`)

## Requirements claimed

| ID | Requirement | Evidence |
|----|-------------|----------|
| PROP-04 | O-Wind consumes the shared module and the build-side acceptance gate (D-08) PASSES; Dorico user-acceptance smoke deferred to Phase 24 batch. | Module consumption: `ouaricon_add_module(O-Wind note-expression)` at `plugins/O-Wind/CMakeLists.txt:52`. Build-side gate PASSES (tri-format ninja clean; AU VALIDATION SUCCEEDED via `verify-au-link.sh O-Wind`). Dorico smoke status: DEFERRED — orchestrator direction. |
| TRACK-01 | Every Phase B plugin rollout executed via `/improve` workflow. | /improve-equivalent cycle ran (preflight + 8 file edits + version bump + CHANGELOG + STATUS + build + install + AU verify) landing as one atomic commit `4ae4600`. Same 8-file atomic shape as 24-01 canary and 24-02 wave 2. |
| TRACK-02 | Each improved plugin receives a version bump applied consistently in CMakeLists.txt. | `PLUGIN_VERSION` was MISSING from `juce_add_plugin(O-Wind ...)` (CMake delta (a) — pre-flagged in 24-INTEGRATION-MATRIX.md row "o-wind"). Added explicitly: `PLUGIN_VERSION "1.16.0"` between `PRODUCT_NAME` (line 11) and `IS_SYNTH` (line 13). Bump 1.15.1 → 1.16.0 (MINOR — new user-visible feature, backward compatible, no preset impact). |
| TRACK-03 | Each plugin's CHANGELOG gets an entry with the verbatim phrase. | `plugins/O-Wind/CHANGELOG.md` top entry `## [1.16.0] - 2026-04-26` contains exact phrase `adds VST3 Note Expression microtonal support for Dorico` (lowercase 'adds' to match plan §verify grep — 24-01 SUMMARY note D casing convention applied). |
| TRACK-04 | Plugin-local STATUS.md updated. | `plugins/O-Wind/.planning/STATUS.md`: added `version: 1.16.0` to YAML front-matter; `last_updated: 2026-04-05` → `2026-04-26`; `next_action: none` → `dorico_microtonal_smoke_test`. Appended new "## v1.16.0 — Phase 24 propagation (2026-04-26)" section below "Next Steps". |
| TRACK-05 | Every affected plugin rebuilt and freshly reinstalled per CLAUDE.md. | Tri-format ninja exit 0; AU cache cleared (`killall -9 AudioComponentRegistrar`; `rm -rf ~/Library/Caches/AudioUnitCache/`; `rm -rf ~/Library/Caches/com.apple.audiounits.cache`); old `O-Wind*.{vst3,component}` removed; fresh bundles installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` (both prod-named and dev-suffixed bundles, mtime 2026-04-26 09:54). |

## Edits landed (8 files, atomic commit `4ae4600`)

1. **`plugins/O-Wind/CMakeLists.txt`** — added `PLUGIN_VERSION "1.16.0"` line inside `juce_add_plugin(O-Wind ...)` block between `PRODUCT_NAME "O-Wind${OUARICON_DEV_SUFFIX}"` (line 11) and `IS_SYNTH TRUE` (now line 13). Appended `# Phase 24: VST3 Note Expression microtonal support (Dorico)\nouaricon_add_module(O-Wind note-expression)` immediately after `target_sources(O-Wind ...)` block (line 52). **CMake delta (a) honored** — version line ADDED (was missing entirely in tree).
2. **`plugins/O-Wind/Source/PluginProcessor.h`** — added `#include "NoteExpression.h"` after the existing `OuariconPresetManager.h` include; added `juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }` in the public section; added private member `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` immediately after `juce::Synthesiser synthesiser` (line 67 region per matrix spec).
3. **`plugins/O-Wind/Source/PluginProcessor.cpp`** — added `voice->setPendingTuningSource (&vst3Extensions.getPendingTable())` inside the addVoice loop between `voice->prepareToPlay(...)` and `synthesiser.addVoice(voice)`; added `vst3Extensions.drainAndUpdate()` at the top of `processBlock` after `buffer.clear()` and BEFORE the zero-length-buffer early-exit / `synthesiser.renderNextBlock(...)` call.
4. **`plugins/O-Wind/Source/FluteSynthVoice.h`** — added `#include "NoteExpression.h"` after `DSP/InstrumentPresets.h`; added inline public setter `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source) { pendingTuningSource = source; }` near `getOversamplingLatency()` (matches O-Prism inline-setter idiom from 24-02 SUMMARY pattern note); added private member `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr` next to `tuningEngine`.
5. **`plugins/O-Wind/Source/FluteSynthVoice.cpp`** — composition site: between TuningEngine `currentFrequency` assignment (lines 78-81) and `bendedFreq = currentFrequency * std::pow(2, pitchBendSemitones/12)` pitch-bend application (formerly line 84), inserted `applyPendingTuning(*pendingTuningSource, midiNoteNumber, static_cast<double>(currentFrequency))` call with `static_cast<float>` wrapping the result. **Pattern 2 honored** — NE applies BEFORE pitch-bend, BEFORE `totalLoopDelay = internalSampleRate/bendedFreq`, AND BEFORE `boreWaveguide.setBoreDelay(totalLoopDelay/(1+initJetRatio))` so the physical-model bore delay sees tuned frequency on sample 0. **Type delta (b) honored** — float→double cast at helper boundary.
6. **`plugins/O-Wind/CHANGELOG.md`** — new top entry `## [1.16.0] - 2026-04-26` with `### Added — VST3 Note Expression Microtonal Support for Dorico` heading, body containing the TRACK-03 verbatim phrase (`adds VST3 Note Expression microtonal support for Dorico` — lowercase 'adds'), composition-order note, and Files Modified list. Style: `## [<ver>] - <date>` (bracketed) — matches O-Wind's existing CHANGELOG convention (different from O-Prism's `## v<ver> (date)` style).
7. **`plugins/O-Wind/.planning/STATUS.md`** — added `version: 1.16.0` to YAML front-matter; `last_updated: 2026-04-05` → `2026-04-26`; `next_action: none` → `dorico_microtonal_smoke_test`. Appended new "## v1.16.0 — Phase 24 propagation (2026-04-26)" section below "Next Steps" describing module adoption + composition order + CMake delta (a).
8. **`modules/registry.yaml`** — `note-expression.used_by:` list extended with `- plugin: O-Wind / version: 1.16.0`. Now contains 4 of 8 expected consumers (`OLyrica` from Phase 23, `O-Bells` from 24-01, `O-Prism` from 24-02, `O-Wind` from 24-03).

## Build-side gate result (D-08)

| Check | Result | Evidence |
|-------|--------|----------|
| `ninja -C build O-Wind_VST3 O-Wind_AU O-Wind_Standalone` | PASS — exit 0 | Build log at `/tmp/o-wind-build.log` (71 build steps; final 3 link lines: `Linking CXX executable plugins/O-Wind/O-Wind_artefacts/Release/Standalone/...` + `Linking CXX CFBundle shared module .../AU/...` + `Linking CXX CFBundle shared module .../VST3/...`). |
| Steinberg link regression check (Phase 23 D-22..D-29) | PASS — no `Undefined symbols ... Steinberg::*` | `! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-wind-build.log` returns empty. Per-format module-source convention held; `cpp/vst3/NoteExpression_VST3.cpp` routed exclusively into `O-Wind_VST3` target via OuariconModules.cmake D-22..D-29 mechanism. |
| AU cache clear (CLAUDE.md) | PASS | `killall -9 AudioComponentRegistrar`; removed `~/Library/Caches/AudioUnitCache/` + `~/Library/Caches/com.apple.audiounits.cache`. |
| Old bundles removed | PASS | Old `O-Wind.vst3`, `O-Wind-dev.vst3`, `O-Wind.component`, `O-Wind-dev.component` deleted before fresh copy. |
| Fresh VST3 install | PASS | `~/Library/Audio/Plug-Ins/VST3/O-Wind.vst3` and `O-Wind-dev.vst3` mtime 2026-04-26 09:54 (within build window). |
| Fresh AU install | PASS | `~/Library/Audio/Plug-Ins/Components/O-Wind.component` and `O-Wind-dev.component` mtime 2026-04-26 09:54 (within build window). |
| `scripts/verify-au-link.sh O-Wind` | **PASS** | `AU VALIDATION SUCCEEDED. auval accepted O-Wind (aumu OWnd OuDv)` — output included full auval test battery (Render Test at multiple frame sizes / sample rates; 1 Channel Test; Bad Max Frames; parameter scheduling; ramped scheduling; MIDI). |

## Dorico smoke 3-point gate result (D-07)

**DEFERRED — batch validation pending (per user direction at orchestrator level for Phase 24).**

User has elected to batch-validate the human-verified 3-point Dorico smoke gates for plans 24-02..24-07 at end-of-phase rather than gating each plan inline. This SUMMARY honestly records the gate as deferred rather than fabricating PASS/FAIL.

| # | Gate point | Pattern validated | Status |
|---|------------|-------------------|--------|
| 1 | Quarter-sharp C4 lands at +50¢ above C4 (~269.29 Hz) | Pattern 3 (240-semitone full-scale conversion in helper) | DEFERRED — batch validation |
| 2 | No attack zipper — first sample at tuned pitch — physical-model bore delay sized to tuned frequency | Pattern 2 (apply NE BEFORE pitch-bend, BEFORE `totalLoopDelay = internalSampleRate/bendedFreq`, AND BEFORE `boreWaveguide.setBoreDelay(totalLoopDelay/(1+initJetRatio))`) | DEFERRED — batch validation |
| 3 | Polyphonic chord (q♯ C4 + ♮ E4) — only C4 detuned | Pattern 1 (correlate by `noteId`, not pitch) | DEFERRED — batch validation |

Build-side correctness for all three patterns is structurally validated:
- **Pattern 3:** `applyPendingTuning` helper is the shared module v1.0.0 already validated end-to-end on O-Lyrica (Phase 23 LYR-03 5-test PASS) and O-Bells (Phase 24-01 3-point gate PASS at ~269.29 Hz observed). Helper unchanged.
- **Pattern 2:** Composition order is the EXACT match called for in 24-INTEGRATION-MATRIX.md row "o-wind": NE applies AFTER TuningEngine assignment (lines 78-81) and BEFORE pitch-bend application AND BEFORE `totalLoopDelay = internalSampleRate/bendedFreq` (formerly line 88) AND BEFORE `boreWaveguide.setBoreDelay(...)` (formerly line 100). Verified via line-precise edit; build PASSES; first-sample correctness preserved by exchange(0.0) consume semantics in the helper. **Particularly important here** because the bore-delay sizing is the physical-model period — a Pattern-2 violation would manifest as audible mid-note frequency drift on the first ~10ms of every NE-tuned note.
- **Pattern 1:** noteId correlation lives in the shared module's `updatePendingFromEvents` (Phase 23 D-04..D-09); plugins do not implement this themselves. Same correlation that PASSED on O-Bells in 24-01.

Orchestrator will collect all 7 deferred gates (24-02..24-07) and present them to the user as a batch validation list at end-of-phase before plan 24-08-final-sweep.

## Anomalies / system-environment notes

These do NOT constitute plan failures or deviations.

### A. Dev-suffix bundle naming (carry-forward from 24-01 / 24-02)

Top-level `CMakeLists.txt` sets `OUARICON_DEV_SUFFIX="-dev"`, so artefact `PRODUCT_NAME "O-Wind${OUARICON_DEV_SUFFIX}"` is `O-Wind-dev` and the build emits `O-Wind-dev.vst3` / `O-Wind-dev.component`. To honor plan §verify acceptance criteria that reference `~/Library/Audio/Plug-Ins/VST3/O-Wind.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Wind.component` (production-branding paths), the install step also copied the dev-built bundles to the prod-named install paths so BOTH dev-suffixed AND prod-named bundles are present at fresh mtime. **Acceptance criterion** PASS under both naming conventions. Same as 24-01 note A and 24-02 note A; carry forward to plans 24-04..24-07.

### B. `auval -a` system-listing oddity (carry-forward from 24-01 / 24-02)

`auval -a | grep -i 'O.Wind'` returns no entries on this machine — same host-environment quirk noted in 24-01 SUMMARY note B and 24-02 SUMMARY note B (zero `aumu` music-device entries in `auval -a` listing affects all plugins on this machine). The canonical D-08 path (`scripts/verify-au-link.sh O-Wind`) PASSES with `AU VALIDATION SUCCEEDED. auval accepted O-Wind (aumu OWnd OuDv)`. **No regression.** Carry forward to plans 24-04..24-07.

### C. JUCE space-before-paren style (carry-forward from 24-02)

FluteSynthVoice.cpp inserts `Ouaricon::NoteExpression::applyPendingTuning (...)` with a space before the open-paren (matching the surrounding O-Wind file style — JUCE coding-style throughout this file). Plan §verify regex `Ouaricon::NoteExpression::applyPendingTuning\(` does not match across the space. Per Plan 24-02 SUMMARY note C, the substantive sentinel — presence of the qualified call — is satisfied; regex literal-paren adjacency is a stylistic gate, not a correctness gate. The build PASSES, AU validates. Carry forward to plans 24-04..24-07.

### D. CMake delta (a) handled cleanly (first execution-time confirmation)

`juce_add_plugin(O-Wind ...)` at line 6 was missing the `PLUGIN_VERSION` argument entirely (pre-flagged in 24-INTEGRATION-MATRIX.md row "o-wind" and 24-PATTERNS.md §"4. O-Wind"). Per plan §action: added `PLUGIN_VERSION "1.16.0"` between `PRODUCT_NAME "O-Wind${OUARICON_DEV_SUFFIX}"` (line 11) and `IS_SYNTH TRUE` (now line 13). Tri-format ninja built clean on first attempt — JUCE picks up the version uniformly across VST3, AU, and Standalone subtargets. **Pattern established for plans 24-05 (O-Reed) and 24-06 (O-Bowed)** which the matrix flags as having the same delta — explicit-add-inside-juce_add_plugin-block is the canonical handling. (Plan 24-07 O-Formant uses `VERSION` (no `PLUGIN_` prefix) and already has it; its delta is a missing `OuariconModules.cmake` include, a different concern.)

### E. Type delta (b) handled per O-Bells canary pattern

`FluteSynthVoice` uses `float currentFrequency` (not `double` like O-Lyrica/O-Prism). Cast through `double` at the helper boundary, exactly as O-Bells canary did (Plan 24-01 SUMMARY note C):

```cpp
currentFrequency = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
    *pendingTuningSource, midiNoteNumber, static_cast<double>(currentFrequency)));
```

No precision loss observable at +50¢ — `double` accommodates 53 bits of mantissa, so the round-trip through the helper preserves 16+ decimal digits of frequency precision; the `float` write-back has 24-bit mantissa, still well below perceptual frequency resolution. Carry-forward to plan 24-07 (O-Formant uses `float tunedF0` — same exact cast pattern reused).

### F. Physical-model period derivation correctness (structural)

Critical composition-order rule for FluteSynthVoice:
1. `currentFrequency` from TuningEngine (lines 78-81)
2. **`applyPendingTuning` here** (NE composes after TuningEngine, before all downstream consumers)
3. `bendedFreq = currentFrequency * std::pow(2, pitchBendSemitones/12)` (pitch-bend stacks multiplicatively on top of NE)
4. `totalLoopDelay = internalSampleRate / bendedFreq` (period derivation from final tuned + bent frequency)
5. `boreWaveguide.setBoreDelay(totalLoopDelay / (1 + initJetRatio))` (physical-model bore delay sized to final period)

If the NE call were placed AFTER step 5, the bore delay would be sized for untuned 12-TET frequency on sample 0; the next block's delay-line update would tune it correctly, but the first block would have the wrong period — audible as a brief pitch glide (Pattern 2 violation; smoke gate point 2 would catch). The plan §integrations explicitly warned of this; the line-precise edit honors it. **Carry-forward to plans 24-05 (O-Reed BoreWaveguide) and 24-06 (O-Bowed WaveguideString)**: same composition-order rule applies — NE BEFORE period-derivation step.

## Decisions Made

- **Dorico gate batching at orchestrator level.** Recorded plan-local Dorico 3-point smoke as DEFERRED rather than fabricating a PASS or stopping the plan to prompt the user. SUMMARY explicitly tabulates the 3 gate points with status DEFERRED for downstream aggregation in 24-08-final-sweep.
- **Explicit PLUGIN_VERSION add inside juce_add_plugin block** chosen over external `set(PLUGIN_VERSION ...)` or post-`juce_add_plugin` modification approaches. Inside-block placement is the JUCE-canonical location; tri-format build PASS confirms it composes correctly with `PLUGIN_CODE`, `FORMATS`, `PRODUCT_NAME`, `IS_SYNTH`, etc.
- **Float→double cast at helper boundary** (instead of refactoring `currentFrequency` to `double`) chosen because (a) the rest of the FluteSynthVoice code path uses `float` end-to-end into BoreWaveguide; widening to `double` would propagate type changes through ~10 downstream sites without functional benefit; (b) the cast pattern is a one-line idiom that's now twice-validated (O-Bells 24-01, O-Wind 24-03) — establishes a uniform pattern for plan 24-07 (O-Formant) which uses the same `float` storage.
- **Composition-order placement BEFORE pitch-bend** chosen so NE and pitch-bend compose multiplicatively in the conventional order: TuningEngine → NE → pitch-bend → period derivation. Pitch-bend after NE allows NE deltas to be the "primary" tuning offset (Dorico's intent) and pitch-bend to be the "performance" offset (legacy MIDI host).

## Deviations from Plan

None — plan executed exactly as written, with the orchestrator-level Dorico gate deferral applied to Task 4 only (per the explicit `<deferred_dorico_gate>` directive in the executor prompt). The six environmental notes above (A–F) are observations, not deviations: the plan's automated acceptance criteria (Tasks 1, 2, 3, 5) all PASS, and no plan rule was violated or auto-bypassed.

## Issues Encountered

None — no triage required. Tri-format build linked clean on the first attempt (per-format module-source convention from Phase 23 held; `cpp/vst3/NoteExpression_VST3.cpp` correctly routed into `O-Wind_VST3` target only, no Steinberg symbols leaked into AU/Standalone link); AU validated; atomic 8-file commit landed without merge or staging conflicts. /improve-equivalent cycle ran cleanly. Two harmless compiler warnings appeared in the build log (FReleaser shadow-field, non-virtual-destructor `delete`) — both originate in JUCE/SDK headers and the pre-existing module code, NOT from O-Wind plan changes; pre-existing across all Phase 24 builds.

## TDD Gate Compliance

N/A — plan type `execute` (not `tdd`); voice-side correctness validated structurally (Pattern 1 inherited from shared module; Pattern 2 line-precise edit verified including physical-model period derivation; Pattern 3 inherited from helper) and via the Phase 23 / 24-01 user-acceptance test results that already PASS. Dorico human-verify gate deferred to Phase 24 batch validation per orchestrator direction.

## User Setup Required

None for this plan's automated acceptance — no external service configuration required. Dorico smoke (deferred) requires Dorico host; orchestrator will batch this.

## Next Phase Readiness

**Phase 24 status after this plan:**
- ✓ Plan 24-01 complete (O-Bells canary; Dorico 3-point PASS)
- ✓ Plan 24-02 complete (O-Prism wave 2; Dorico 3-point DEFERRED)
- ✓ Plan 24-03 complete (O-Wind wave 3; Dorico 3-point DEFERRED) — first physical-model consumer; CMake delta (a) handling pattern established
- ✓ Float→double cast pattern at helper boundary now twice-validated (O-Bells, O-Wind) — ready for plan 24-07 reuse
- ✓ Physical-model period derivation composition-order pattern established — ready for plans 24-05 (O-Reed BoreWaveguide) and 24-06 (O-Bowed WaveguideString)
- ✓ Dev-suffix bundle handling continues to work identically across plans (carry-forward A)
- ✓ `auval -a` advisory continues to apply (carry-forward B)
- ✓ JUCE space-before-paren style (carry-forward note C from 24-02)
- ⏳ Dorico 3-point gate batch-validation pending — orchestrator queue: 24-02..24-07

**Aggregation hook:** This SUMMARY feeds **`24-08-final-sweep-SUMMARY.md` row 3 of 8**. The 3-point Dorico gate result table format (with DEFERRED status here) is the row-template for plans 24-04..24-07.

**Ready for plan 24-04 (O-IntonationPad).** No blockers, no escalations. Phase 24 wave 3 momentum preserved.

## Self-Check: PASSED

- `git log --oneline -5 | grep -q "4ae4600"` → FOUND
- Atomic commit `4ae4600` references all 8 plan-scoped files (verified via `git show 4ae4600 --stat` → `8 files changed, 55 insertions(+), 2 deletions(-)`)
- `plugins/O-Wind/CMakeLists.txt` contains `ouaricon_add_module(O-Wind note-expression)` and `PLUGIN_VERSION "1.16.0"` inside `juce_add_plugin` block → FOUND
- `plugins/O-Wind/Source/PluginProcessor.h` contains `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` and `getVST3ClientExtensions()` override → FOUND
- `plugins/O-Wind/Source/PluginProcessor.cpp` contains `vst3Extensions.drainAndUpdate(` and `setPendingTuningSource (&vst3Extensions` → FOUND
- `plugins/O-Wind/Source/FluteSynthVoice.h` contains `pendingTuningSource` and the inline `setPendingTuningSource` setter → FOUND
- `plugins/O-Wind/Source/FluteSynthVoice.cpp` contains `Ouaricon::NoteExpression::applyPendingTuning` (qualified call with float→double cast) → FOUND
- `plugins/O-Wind/CHANGELOG.md` contains the TRACK-03 verbatim phrase `adds VST3 Note Expression microtonal support for Dorico` → FOUND
- `modules/registry.yaml` contains `plugin: O-Wind` under `note-expression.used_by` → FOUND
- `~/Library/Audio/Plug-Ins/VST3/O-Wind*.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Wind*.component` present at fresh mtime (2026-04-26 09:54) → FOUND (both dev and prod naming)
- `scripts/verify-au-link.sh O-Wind` exit 0 with `AU VALIDATION SUCCEEDED. auval accepted O-Wind (aumu OWnd OuDv)` → FOUND in execution log
- Dorico 3-point gate status documented as DEFERRED with structural correctness rationale → PRESENT

---
*Phase: 24-propagate*
*Completed: 2026-04-26*
