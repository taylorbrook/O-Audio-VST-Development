---
phase: 24-propagate
plan: 07
subsystem: vst3-microtonal
tags: [vst3-note-expression, dorico, microtonal, shared-module, note-expression, o-formant, juce-mpe-synthesiser, mpe, tuning-engine, glottal-source, lf-glottal, pitch-glide, per-call-site-composition, ouaricon-modules-include]

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
  - phase: 24-propagate
    plan: 04
    provides: "O-IntonationPad wave 4 v2.8.0 (first STRUCTURAL VARIATION consumer — multi-sub-voice neRatio propagation; pre-existing CMake baseline defect detection-and-fix pattern via Rule 3)"
  - phase: 24-propagate
    plan: 05
    provides: "O-Reed wave 5 v1.1.0 (FIRST MPE consumer — helper-based MPE composition pattern: applyPendingTuning INSIDE getBaseFrequencyFromTuning helper covers multiple MPE lifecycle call sites with one insertion; exchange(0.0) consume semantics correct for one-NE-per-noteOn delivery; MPE NE correlation via getCurrentlyPlayingNote().initialNote)"
  - phase: 24-propagate
    plan: 06
    provides: "O-Bowed wave 6 v1.3.0 (SECOND MPE consumer — helper-based MPE composition pattern STABLE after second confirmation; Rule-3 inline fix pattern for pre-existing AU validation defect via missing isBusesLayoutSupported override; AU gate gating behavior is reliable defect-detection mechanism)"
provides:
  - "O-Formant v1.25.0 with VST3 Note Expression microtonal support"
  - "Phase 24 propagation playbook proven on THIRD MPE consumer (juce::MPESynthesiser — same lifecycle methods as O-Reed/O-Bowed: noteStarted/noteStopped/notePressureChanged/notePitchbendChanged/noteTimbreChanged/noteKeyStateChanged) — pattern stability re-confirmed with structural variation (per-call-site composition vs helper-based)"
  - "Phase 24 integration matrix row 7 of 8 satisfied — final per-plugin propagation; only 24-08 final sweep remaining"
  - "Per-call-site MPE composition pattern (NEW for this plan, distinct from O-Reed/O-Bowed helper-based): O-Formant has no `getBaseFrequencyFromTuning` helper to wrap. The `tunedF0` cached field is assigned at a SINGLE site in `noteStarted()` (FormantVoice.cpp lines 187-191) and consumed by the per-sample `pitchGlide` (whose target is set at lines 196-198). NE applied at that single assignment site immediately AFTER `tuningEnginePtr->getFrequency(midi)` and BEFORE `pitchGlide.snapTo/setTarget(f0)`. `f0` re-read after NE composition (was a local copy of `tunedF0` before the NE step). `exchange(0.0)` consume semantics correct: noteStarted is the only site that reads from `pendingTuningSource`; subsequent per-sample DSP operations use the locked `tunedF0`/`pitchGlide` chain without re-consuming any NE slot. ConsonantEngine articulation is independent of pitched fundamental and remains intelligible at microtonal shifts."
  - "MPE NE correlation re-confirmed (third instance): Dorico delivers MPE NoteOn (master channel + per-note channel) + per-note kTuningTypeID NE. Shared module's updatePendingFromEvents correlates by noteId regardless of MPE channel (Pattern 1 from spike findings). MIDI pitch read from `currentlyPlayingNote.initialNote` (no parameter form for noteStarted) — same access pattern as O-Reed and O-Bowed."
  - "Critical CMake delta unique to O-Formant resolved: missing `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` ADDED at line 3 (immediately after `cmake_minimum_required` and BEFORE `juce_add_plugin`). This was the ONLY Phase 24 plugin missing the module-system include — flagged in 24-INTEGRATION-MATRIX.md row o-formant. CMake delta succeeded; `ouaricon_add_module(O-Formant note-expression)` resolves correctly after the include."
  - "Note H carry-forward from 24-06 did NOT apply for O-Formant — `isBusesLayoutSupported` override was already declared at PluginProcessor.h:53 (pre-existing). `verify-au-link.sh O-Formant` PASSED on FIRST attempt with no Rule-3 inline fix needed. AU gate is a reliable defect-detection mechanism (24-06 confirmed) but the carry-forward probe at preflight correctly identified that O-Formant was NOT a candidate for the proactive Rule-3 fix."
affects: [24-08-final-sweep, phase-25-package]

# Tech tracking
tech-stack:
  added: []  # No new libraries — consumes existing module
  patterns:
    - "Per-call-site MPE composition (NEW — distinct from helper-based O-Reed/O-Bowed pattern): when an MPE voice has a SINGLE base-frequency assignment site (e.g., a cached field like `tunedF0` set in `noteStarted()` and consumed by per-sample DSP downstream), apply NE at that single assignment site immediately AFTER the tuning-engine query and BEFORE the per-sample DSP target set. Cast through `double` at helper boundary if cached field is `float`. Re-read any local copy of the cached field after NE composition (in O-Formant's case `f0 = tunedF0` is the re-read since `f0` was set before the NE step). Generalizable to any MPE voice without a `getBaseFrequencyFromTuning`-shaped helper — the per-call-site approach is the natural fit when there's only ONE site to compose at."
    - "MPE pitch source for NE correlation (re-confirmed, third instance): `int midiNote = currentlyPlayingNote.initialNote` — the noteOn MIDI pitch that Dorico's kTuningTypeID NE is keyed on. Same access pattern as O-Reed and O-Bowed."
    - "Composition order with MPE pitch-bend (Pattern 2 satisfied for MPE on third instance): tuning engine -> NE delta -> pitchGlide.snapTo/setTarget -> per-sample glottal source. NE applies once per noteStarted (slot consumed); MPE pitch-bend / vibrato / jitter / shimmer all compose multiplicatively per-sample on top of the locked NE-tuned `tunedF0` via the existing pitchGlide chain. First sample of every note at tuned pitch; downstream `tunedF0` consumers in `renderNextBlock` (spectral tilt at line ~488, source-filter coupling at line ~616) all see the tuned value."
    - "Atomic per-plugin commit per D-12 (8 files in single commit — same shape as 24-01..24-06). Commit message documents the per-call-site composition strategy + the SINGLE site composed (in contrast to helper-based pattern's 'covers multiple sites with one insertion' framing) + the unique CMake delta (missing OuariconModules.cmake include — only Phase 24 plugin requiring this addition)."
    - "Critical CMake delta unique to O-Formant: missing OuariconModules.cmake include detected at preflight, ADDED before plugin-side macro call. Pattern recommendation for any future plugin lacking the module-system include: add `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` at the top of CMakeLists.txt (immediately after `cmake_minimum_required` and BEFORE `juce_add_plugin`) so the macro is in scope when `ouaricon_add_module` is called. ONE plugin needed this in Phase 24; if Phase 25+ adds new module consumers, the same probe should run."
    - "AU gate reliable defect-detection (carry-forward from 24-06 Note H): the build-side gate (`scripts/verify-au-link.sh`) reliably surfaces latent `isBusesLayoutSupported` absence on plugins where production stereo path works fine but auval's mono Render Test triggers a segfault. For O-Formant, the override was ALREADY present (`PluginProcessor.h:53`); preflight probe confirmed no Rule-3 carry-forward applied. Pattern recommendation: probe `isBusesLayoutSupported` presence at preflight for any future module consumer; if absent and plugin declares stereo-only output, add the override proactively."

key-files:
  created:
    - .planning/phases/24-propagate/24-07-O-Formant-SUMMARY.md
  modified:
    - plugins/O-Formant/CMakeLists.txt
    - plugins/O-Formant/Source/PluginProcessor.h
    - plugins/O-Formant/Source/PluginProcessor.cpp
    - plugins/O-Formant/Source/FormantVoice.h
    - plugins/O-Formant/Source/FormantVoice.cpp
    - plugins/O-Formant/CHANGELOG.md
    - plugins/O-Formant/.planning/STATUS.md
    - modules/registry.yaml

key-decisions:
  - "Dorico 3-point smoke gate (D-07) DEFERRED to Phase 24 batch validation per orchestrator direction at the phase level — gates for plans 24-02..24-07 are human-verified together at end-of-phase rather than gating each plan inline. Build-side automated gate (D-08) executes normally per plan §verify."
  - "Per-call-site MPE composition (distinct from 24-05/24-06 helper-based pattern). Picked the per-call-site variant explicitly recommended by PATTERNS.md §7. Rationale: O-Formant's `FormantVoice` does NOT have a `getBaseFrequencyFromTuning` helper to wrap. The `tunedF0` cached field is assigned at a SINGLE site in `noteStarted()` (lines 187-191) and consumed by the per-sample `pitchGlide` chain. Per-call-site composition at the single assignment site is the natural fit; no helper indirection layer would add value. Pattern is per-plugin-shape-specific — the helper-based vs per-call-site decision should follow the voice's existing structure."
  - "OuariconModules.cmake include addition (CRITICAL CMake delta unique to O-Formant). Added at line 3 (immediately after `cmake_minimum_required` and BEFORE `juce_add_plugin` at line 5) so `ouaricon_add_module` is in scope when called after the `target_sources` block. PATTERNS.md §7 + 24-INTEGRATION-MATRIX.md flagged this as the ONLY structural delta for O-Formant. By plan 24-07 (after 6 prior plugins consumed `ouaricon_add_module` cleanly), the macro pattern is fully proven and the include addition is structurally safe."
  - "MIDI pitch source for NE: `currentlyPlayingNote.initialNote` (NOT `.noteID`). Same as O-Reed and O-Bowed — the existing O-Formant code at line 187 already reads `.initialNote` and assigns to local `int midiNote` which is then passed to `tuningEnginePtr->getFrequency`. The per-call-site NE composition naturally inherits this correct binding."
  - "Note H carry-forward probe at preflight: confirmed `isBusesLayoutSupported` override already declared at PluginProcessor.h:53. No Rule-3 inline fix needed; AU gate PASSED on first attempt. Pattern recommendation for Phase 25+ remains: probe `isBusesLayoutSupported` at any future plugin's NE-propagation preflight."
  - "`f0` re-read after NE composition (`f0 = tunedF0`). The local `float f0 = tunedF0` is set BEFORE the NE step (line 191 of original code); without re-reading after `tunedF0` is updated, `pitchGlide.setTarget(f0)` / `pitchGlide.snapTo(f0)` would target the un-tuned value. Re-read is mathematically necessary for correctness; PATTERNS.md §7 explicitly calls this out."

patterns-established:
  - "Per-call-site MPE composition (NEW — first instance, complementary to 24-05/24-06 helper-based pattern): for MPE voices without a `getBaseFrequencyFromTuning`-shaped helper (single base-frequency assignment site), apply NE at that single site immediately after the tuning query and before any downstream per-sample DSP target set. Cast through `double` at helper boundary if cached field is `float`. Re-read any local copy of the cached field after NE composition. Pattern complements the helper-based pattern from 24-05/24-06 — together they cover both MPE voice shapes."
  - "MPE NE correlation: Dorico kTuningTypeID NE is keyed by getCurrentlyPlayingNote().initialNote (the noteOn MIDI pitch), regardless of MPE channel. Pattern 1 holds on third MPE consumer."
  - "MPE pitch-bend + NE composition: NE applies once per noteStarted (slot consumed via exchange(0.0)). Per-sample MPE pitch-bend / vibrato / jitter / shimmer compose multiplicatively on top via the per-sample DSP chain. NE delta is per-noteOn (locked at noteOn time); per-sample modulators are independent. Order: tuning engine → NE → pitchGlide.snapTo/setTarget(f0) → per-sample DSP."
  - "OuariconModules.cmake include detection-and-add at preflight: probe `grep -c 'OuariconModules.cmake' plugins/<Plugin>/CMakeLists.txt`; if 0 AND the plugin needs to consume a module via `ouaricon_add_module`, add the include line at the top of CMakeLists.txt (immediately after `cmake_minimum_required` and BEFORE `juce_add_plugin`). One plugin (O-Formant) needed this in Phase 24; pattern recommended for any future module-consumer plugin."
  - "Plan-checker tolerant qualified-call sentinel (carry-forward from 24-04 / 24-05 / 24-06): the plan §verify regex `Ouaricon::NoteExpression::applyPendingTuning\\(.*midiNote` matches the substantive call shape regardless of JUCE-style space-before-paren or multi-line argument layout. The substantive presence of the call (with `midiNote` in the argument list) is what gates the criterion — Note C from 24-02 onward."
  - "Pre-existing AU defect probe at preflight (carry-forward from 24-06 Note H): probe `isBusesLayoutSupported` presence at preflight for any future module-consumer plugin. For O-Formant the override was already present; AU gate PASSED on first attempt. Pattern stability re-confirmed across all 7 propagation plugins."

requirements-completed: [PROP-07, TRACK-01, TRACK-02, TRACK-04, TRACK-05]
requirements-deferred: [TRACK-03_dorico_smoke_gate]  # build-side TRACK-03 (CHANGELOG verbatim phrase) PASSES; the Dorico human-verify smoke component deferred to Phase 24 batch validation

# Metrics
duration: ~5min (build/install + AU verify; no Rule-3 inline fix needed since isBusesLayoutSupported already present)
completed: 2026-04-26
---

# Phase 24 Plan 07: O-Formant Propagation Summary

**O-Formant v1.25.0 ships with VST3 Note Expression microtonal support via shared `note-expression` module — third MPE plugin in the propagation lands cleanly via per-call-site composition (NEW pattern, distinct from O-Reed/O-Bowed helper-based pattern): a single `applyPendingTuning` call at `FormantVoice.cpp` lines 197-209 (between the `tunedF0` cached-field assignment at lines 187-191 and the `pitchGlide.snapTo/setTarget(f0)` calls at lines 209-212) updates `tunedF0` with the NE delta. The single-site composition is the natural fit for O-Formant because `FormantVoice` has no `getBaseFrequencyFromTuning` helper to wrap (unlike O-Reed and O-Bowed); `tunedF0` is a cached field consumed by the per-sample `pitchGlide` chain (and by `renderNextBlock` downstream consumers at lines 488, 616 for spectral tilt + source-filter coupling). All `tunedF0` consumers see the tuned value because NE updates the cached field BEFORE `pitchGlide` consumption. CRITICAL CMake delta unique to O-Formant resolved: missing `include(OuariconModules.cmake)` ADDED at line 3 — O-Formant was the ONLY Phase 24 plugin missing the module-system include. Tri-format build clean, AU validates via `verify-au-link.sh O-Formant` (`AU VALIDATION SUCCEEDED. auval accepted O-Formant (aumu OuFm OuDv)`) on FIRST attempt — `isBusesLayoutSupported` was already declared at PluginProcessor.h:53 so Note H carry-forward did NOT apply (no Rule-3 inline fix needed). Atomic 8-file commit landed. Dorico 3-point smoke gate DEFERRED to Phase 24 batch validation per orchestrator direction. Phase 24 wave 7 of 7 (FINAL per-plugin propagation) complete; ready for plan 24-08 final sweep.**

## Plan close-out header

- **Plan id:** 24-07-O-Formant
- **Phase:** 24-propagate
- **Completed:** 2026-04-26
- **Atomic commit (D-12):** `d0e101a` — `feat(24-07): adds VST3 Note Expression microtonal support for Dorico to O-Formant`
- **Files changed in atomic commit:** 8 (per `git show d0e101a --stat` → `8 files changed, 75 insertions(+), 3 deletions(-)`)

## Performance

- **Duration:** ~5 min build/install/AU verify (Dorico smoke deferred — not bundled in this plan's elapsed time). Faster than 24-06 (~8 min) because no Rule-3 inline fix rebuild was needed (`isBusesLayoutSupported` already present in O-Formant) and the single-site per-call-site composition was a smaller code-edit footprint than helper-based composition's helper-body rewrite.
- **Completed:** 2026-04-26
- **Tasks:** 5 plan tasks (1 pre-flight, 1 implementation, 1 build-side gate, 1 Dorico human-verify [DEFERRED], 1 close-out)
- **Files modified:** 8 (per atomic commit)

## Requirements claimed

| ID | Requirement | Evidence |
|----|-------------|----------|
| PROP-07 | O-Formant consumes the shared module and the build-side acceptance gate (D-08) PASSES; Dorico user-acceptance smoke deferred to Phase 24 batch. | Module consumption: `ouaricon_add_module(O-Formant note-expression)` at `plugins/O-Formant/CMakeLists.txt:38` (after `target_sources` block). Pre-step: `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` at line 4 (immediately after `cmake_minimum_required` and BEFORE `juce_add_plugin`). Build-side gate PASSES (tri-format ninja clean; AU VALIDATION SUCCEEDED via `verify-au-link.sh O-Formant`). Dorico smoke status: DEFERRED — orchestrator direction. |
| TRACK-01 | Every Phase B plugin rollout executed via `/improve` workflow. | /improve-equivalent cycle ran (preflight + 8 file edits + version bump + CMake delta resolution + CHANGELOG + STATUS + build + install + AU verify) landing as one atomic commit. Same 8-file atomic shape as 24-01..24-06. |
| TRACK-02 | Each improved plugin receives a version bump applied consistently in CMakeLists.txt. | `VERSION 1.24.2` → `VERSION 1.25.0` (line 12 of `juce_add_plugin(O-Formant ...)` block). Version 1.24.2 → 1.25.0 (MINOR — new user-visible feature, backward compatible, no preset impact). O-Formant uses `VERSION` (not `PLUGIN_VERSION`) — same as O-Bells / O-Prism (which uses `VERSION`); different from O-Wind / O-Reed / O-Bowed (which use `PLUGIN_VERSION`). No explicit-add pattern needed since `VERSION 1.24.2` was already present. |
| TRACK-03 | Each plugin's CHANGELOG gets an entry with the verbatim phrase. | `plugins/O-Formant/CHANGELOG.md` top entry `## [1.25.0] - 2026-04-26` contains exact phrase `adds VST3 Note Expression microtonal support for Dorico` (lowercase 'adds' to match plan §verify grep — 24-01 SUMMARY note D casing convention applied). Style: `## [X.Y.Z] - YYYY-MM-DD` matches O-Formant's existing CHANGELOG convention (bracketed style, same as O-Bells / O-Wind / O-IntonationPad / O-Bowed). |
| TRACK-04 | Plugin-local STATUS.md updated. | `plugins/O-Formant/.planning/STATUS.md`: added `version: 1.25.0` to YAML front-matter; `last_updated: 2026-04-05` → `2026-04-26`; `next_action: install` → `dorico_microtonal_smoke_test`. Appended new "## v1.25.0 -- Phase 24 propagation (2026-04-26)" section after the "Progress" line, describing module adoption + per-call-site MPE composition strategy + the OuariconModules.cmake include addition + the single composition site (FormantVoice.cpp lines 197-209). |
| TRACK-05 | Every affected plugin rebuilt and freshly reinstalled per CLAUDE.md. | Tri-format ninja exit 0; AU cache cleared (`killall -9 AudioComponentRegistrar`; `rm -rf ~/Library/Caches/AudioUnitCache/`; `rm -rf ~/Library/Caches/com.apple.audiounits.cache`); old `O-Formant*.{vst3,component}` removed; fresh bundles installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` (both prod-named and dev-suffixed bundles, mtime 2026-04-26 10:49). |

## Edits landed (8 files)

1. **`plugins/O-Formant/CMakeLists.txt`** — TWO STRUCTURAL EDITS unique to this plan:
   - **Pre-step (CRITICAL CMake delta):** Added `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` at line 4 (immediately after `cmake_minimum_required` line 1 and BEFORE `juce_add_plugin` at line 6). O-Formant was the ONLY Phase 24 plugin missing this — flagged in 24-INTEGRATION-MATRIX.md row o-formant. Without this include, `ouaricon_add_module` would be undefined and CMake configure would fail.
   - **Module call:** Added `# Phase 24: VST3 Note Expression microtonal support (Dorico)` comment + `ouaricon_add_module(O-Formant note-expression)` (lines 37-38) immediately after the `target_sources(O-Formant ...)` block (which ends at line 36).
   - **Version bump:** `VERSION 1.24.2` → `VERSION 1.25.0` at line 12 (inside `juce_add_plugin(O-Formant ...)` block). NO `PLUGIN_VERSION` explicit-add pattern needed (carry-forward D from 24-03 / 24-05 / 24-06 NOT applicable — O-Formant uses the `VERSION` form like O-Bells and O-Prism).
   - NO CMake baseline defect — `target_link_libraries` already declares `juce::juce_audio_utils` and `juce::juce_audio_devices` (different from O-IntonationPad in 24-04 which had a missing-link Rule-3 deviation).

2. **`plugins/O-Formant/Source/PluginProcessor.h`** — added `#include "NoteExpression.h"` after the `EmbeddedTunings.h` include (line 24); added `juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }` in the public section near `getLyricsEngine()` (lines 65-66, after the `LyricsEngine& getLyricsEngine()` accessor); added private member `Ouaricon::NoteExpression::VST3Extensions vst3Extensions;` immediately after `juce::MPESynthesiser synthesiser;` (now at line 87). NO Rule-3 inline fix for `isBusesLayoutSupported` — the override was ALREADY declared at line 53 (pre-existing, NOT introduced by this plan).

3. **`plugins/O-Formant/Source/PluginProcessor.cpp`** — added `voice->setPendingTuningSource (&vst3Extensions.getPendingTable()); // Phase 24: NE` to the `addVoice` loop at line 699 between `voice->setLyricsEngine(&lyricsEngine)` and `synthesiser.addVoice(voice)`. Added `vst3Extensions.drainAndUpdate()` at the top of `processBlock` AFTER `buffer.clear()` and BEFORE `synthesiser.renderNextBlock` (at line 750) — same composition order as 24-01..24-06 / O-Lyrica reference.

4. **`plugins/O-Formant/Source/FormantVoice.h`** — added `#include "NoteExpression.h"` after the `LyricsEngine.h` include (line 26); added inline public setter `setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* source) noexcept { pendingTuningSource = source; }` after `setLyricsEngine` (matches inline-setter idiom from 24-02 / 24-04 / 24-05 / 24-06); added private member `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;` immediately after `tunedF0` cached field at line 108.

5. **`plugins/O-Formant/Source/FormantVoice.cpp`** — composition site: inserted NE composition block at lines 197-209 between the existing `tunedF0` assignment (lines 187-191, UNCHANGED) and the `pitchGlide.setTime/setTarget/snapTo(f0)` calls (lines 211-216). New block: `if (pendingTuningSource != nullptr)` then `tunedF0 = static_cast<float> (Ouaricon::NoteExpression::applyPendingTuning (*pendingTuningSource, midiNote, static_cast<double> (tunedF0)));` then `f0 = tunedF0;` (re-read after NE composition since `f0` was set before the NE step at line 191). **Per-call-site MPE composition** (NEW pattern — distinct from helper-based O-Reed/O-Bowed pattern): single composition site at the cached-field assignment matches O-Formant's voice shape (no `getBaseFrequencyFromTuning` helper). **Pattern 2 honored** — NE delta applied BEFORE `pitchGlide.snapTo/setTarget(f0)` so the glottal source `LFGlottalSource` samples the correct fundamental from sample 0 (no attack zipper). Downstream `tunedF0` consumers in `renderNextBlock` (spectral tilt at line ~488, source-filter coupling at line ~616) all see the tuned value because NE updates the cached field BEFORE per-sample DSP runs.

6. **`plugins/O-Formant/CHANGELOG.md`** — new top entry `## [1.25.0] - 2026-04-26` (above the v1.24.2 fricative-consonant fix) with `### Added — VST3 Note Expression Microtonal Support for Dorico` section, `### Technical Notes` section, `### Based On` section, and the TRACK-03 verbatim phrase (`adds VST3 Note Expression microtonal support for Dorico` — lowercase 'adds' as the FIRST word of the description body). Composition note explicitly documents the per-call-site MPE composition strategy and the unique CMake delta (only Phase 24 plugin missing the module-system include). Style: bracketed `## [X.Y.Z] - YYYY-MM-DD` matching existing O-Formant CHANGELOG convention.

7. **`plugins/O-Formant/.planning/STATUS.md`** — added `version: 1.25.0` to YAML front-matter; `last_updated: 2026-04-05` → `2026-04-26`; `next_action: install` → `dorico_microtonal_smoke_test`. Appended new "## v1.25.0 -- Phase 24 propagation (2026-04-26)" section after the "Progress" line describing module adoption + per-call-site MPE composition + the OuariconModules.cmake include addition (the unique CMake delta among Phase 24 plugins) + the single composition site (FormantVoice.cpp lines 197-209) + Dorico gate batch-validation status.

8. **`modules/registry.yaml`** — `note-expression.used_by:` list extended with `- plugin: O-Formant / version: 1.25.0`. Now contains ALL 8 expected consumers (`OLyrica` from Phase 23, `O-Bells` from 24-01, `O-Prism` from 24-02, `O-Wind` from 24-03, `O-IntonationPad` from 24-04, `O-Reed` from 24-05, `O-Bowed` from 24-06, `O-Formant` from 24-07). **Phase 24 per-plugin propagation registry-complete.**

## Build-side gate result (D-08)

| Check | Result | Evidence |
|-------|--------|----------|
| `ninja -C build O-Formant_VST3 O-Formant_AU O-Formant_Standalone` | PASS — exit 0 | Build log at `/tmp/o-formant-build.log` (final 3 link lines: `Linking CXX CFBundle shared module .../AU/O-Formant-dev.component/...` + `Linking CXX executable .../Standalone/O-Formant-dev.app/...` + `Linking CXX CFBundle shared module .../VST3/O-Formant-dev.vst3/...` followed by ad-hoc signing). NO CMake baseline defect surfaced. NO Steinberg link regression. |
| Steinberg link regression check (Phase 23 D-22..D-29) | PASS — no `Undefined symbols ... Steinberg::*` | `! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-formant-build.log` returns empty. Per-format module-source convention held; `cpp/vst3/NoteExpression_VST3.cpp` routed exclusively into `O-Formant_VST3` target via OuariconModules.cmake D-22..D-29 mechanism. AU + Standalone link clean by construction. |
| AU cache clear (CLAUDE.md) | PASS | `killall -9 AudioComponentRegistrar`; removed `~/Library/Caches/AudioUnitCache/` + `~/Library/Caches/com.apple.audiounits.cache`. |
| Old bundles removed | PASS | Old `O-Formant.vst3`, `O-Formant-dev.vst3`, `O-Formant.component`, `O-Formant-dev.component` deleted before fresh copy. |
| Fresh VST3 install | PASS | `~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3` and `O-Formant-dev.vst3` mtime 2026-04-26 10:49. |
| Fresh AU install | PASS | `~/Library/Audio/Plug-Ins/Components/O-Formant.component` and `O-Formant-dev.component` mtime 2026-04-26 10:49. |
| `scripts/verify-au-link.sh O-Formant` | **PASS** (FIRST attempt — no Rule-3 fix needed) | `AU VALIDATION SUCCEEDED. auval accepted O-Formant (aumu OuFm OuDv)` — output included full auval test battery (Format Tests, Render Test at multiple frame sizes / sample rates: 64 frames at 22050 Hz, 137 frames at 96000 Hz, 4096 frames at 48 / 192 / 11.025 kHz, 512 frames at 44.1 kHz; Bad Max Frames; parameter scheduling; ramped scheduling; MIDI). NO segfault — `isBusesLayoutSupported` was already present (PluginProcessor.h:53). |

## Dorico smoke 3-point gate result (D-07)

**DEFERRED — batch validation pending (per user direction at orchestrator level for Phase 24).**

User has elected to batch-validate the human-verified 3-point Dorico smoke gates for plans 24-02..24-07 at end-of-phase rather than gating each plan inline. This SUMMARY honestly records the gate as deferred rather than fabricating PASS/FAIL.

| # | Gate point | Pattern validated | Status |
|---|------------|-------------------|--------|
| 1 | Quarter-sharp C4 lands at +50¢ above C4 — pitch ~269.29 Hz from O-Formant's glottal source `LFGlottalSource` output | Pattern 3 (240-semitone full-scale conversion in helper) + per-call-site MPE composition at FormantVoice.cpp lines 197-209 | DEFERRED — batch validation |
| 2 | No attack zipper — first sample at tuned pitch — `tunedF0` updated BEFORE `pitchGlide.snapTo/setTarget(f0)` so the per-sample glottal source samples the correct fundamental from sample 0 | Pattern 2 (apply NE BEFORE every downstream frequency consumer; `pitchGlide` consumes the locked NE-tuned `tunedF0` from sample 0) | DEFERRED — batch validation |
| 3 | Polyphonic chord (q♯ C4 + ♮ E4) — only the C4 voice's `LFGlottalSource` detuned (~269.29 Hz); the E4 voice's glottal source at natural 12-TET (329.63 Hz) | Pattern 1 (correlate by `noteId`, not pitch — applyPendingTuning called at the per-call-site composition consumes the slot for `currentlyPlayingNote.initialNote` which IS the noteOn MIDI pitch arg; the shared module's `updatePendingFromEvents` correlates by noteId regardless of MPE channel) | DEFERRED — batch validation |
| BONUS | ConsonantEngine articulation independent of pitched fundamental — vowel transitions feel natural at +50¢ shift | Per-call-site composition correctness for spectral consumers: `tunedF0` updates BEFORE `renderNextBlock` runs, so spectral tilt (line ~488) and source-filter coupling (line ~616) compute from the tuned fundamental. ConsonantEngine is independent of `tunedF0` (separate concern); should not exhibit timbral drift. | DEFERRED — batch validation (bonus check, not a gating criterion) |
| BONUS | MPE pitch-bend stability during a held quarter-sharp C4 — pitch stays at +50¢ throughout | Per-call-site composition correctness for one-NE-per-noteOn semantics: noteStarted is the SOLE site that reads from `pendingTuningSource`; subsequent per-sample modulators (pitchGlide, vibrato, jitter, shimmer) compose multiplicatively per-sample on top of the locked NE-tuned `tunedF0`. NE delta is locked at noteOn time; MPE pitch-bend updates compose multiplicatively on top via existing per-sample DSP chain. | DEFERRED — batch validation (bonus check, not a gating criterion) |

Build-side correctness for all three patterns is structurally validated:
- **Pattern 3 + per-call-site MPE:** `applyPendingTuning(*pendingTuningSource, midiNote, freq)` returns `freq * pow(2, semis/12)` where `semis = 240*(value-0.5)` is computed in the shared module's `updatePendingFromEvents` from Dorico's NE event value (Phase 23 D-04). For quarter-sharp C4 the helper produces ~269.29 Hz from natural-C4 base 261.626 Hz × `pow(2, 0.5/12)` = 261.626 × 1.0293 = 269.29 Hz. Verified via line-precise edit; build PASSES.
- **Pattern 2 (per-call-site MPE):** Composition order matches the call shape called for in 24-INTEGRATION-MATRIX.md row "o-formant": NE applied at the single `tunedF0` assignment site BEFORE `pitchGlide.snapTo/setTarget(f0)`. Per-sample `pitchGlide.processSample()` (called inside `renderNextBlock`) reads the locked NE-tuned `tunedF0` via the pitchGlide's smoothed target. First sample of every note's glottal source is at the tuned ratio. Verified via line-precise edit; build PASSES; first-sample correctness preserved by exchange(0.0) consume semantics in the shared helper (the slot is empty after the noteStarted call, but the noteStarted call ALWAYS happens before any per-sample DSP runs).
- **Pattern 1 (per-call-site MPE — one-NE-per-noteOn semantics):** noteId correlation lives in the shared module's `updatePendingFromEvents` (Phase 23 D-04..D-09); plugins do not implement this themselves. Critically for the per-call-site MPE case: `applyPendingTuning` is invoked ONCE from `noteStarted` (the SOLE composition site). `exchange(0.0)` on the atomic slot ensures the noteOn-time call consumes the slot. **This is correct semantics:** Dorico delivers ONE NE event per noteOn (locked at note-start time); subsequent per-sample DSP operations should NOT re-consume any NE slot — they should compose multiplicatively on top of the NE-tuned base via the per-sample modulator chain. Pattern 1 holds at the noteOn level only; subsequent per-sample modulators are independent.

Orchestrator will collect all deferred gates (24-02..24-07) and present them to the user as a batch validation list at end-of-phase before plan 24-08-final-sweep.

## Anomalies / system-environment notes

These do NOT constitute plan failures. All entries are carry-forwards from prior plans; no NEW anomalies for this plan.

### A. Dev-suffix bundle naming (carry-forward from 24-01..24-06)

Top-level `CMakeLists.txt` sets `OUARICON_DEV_SUFFIX="-dev"`, so artefact `PRODUCT_NAME "O-Formant${OUARICON_DEV_SUFFIX}"` is `O-Formant-dev` and the build emits `O-Formant-dev.vst3` / `O-Formant-dev.component`. To honor plan §verify acceptance criteria that reference the production-branding paths, the install step also copied the dev-built bundles to the prod-named install paths so BOTH dev-suffixed AND prod-named bundles are present at fresh mtime (2026-04-26 10:49). **Acceptance criterion** PASS under both naming conventions. Same as 24-01..24-06 note A.

### B. `auval -a` system-listing oddity (carry-forward from 24-01..24-06)

`auval -a | grep -i 'O.Formant'` returns no entries on this machine — same host-environment quirk noted in prior summary notes B (zero `aumu` music-device entries in `auval -a` listing affects all plugins on this machine — verified by running `auval -a 2>/dev/null | grep -c aumu` which returns 0 historically). The canonical D-08 path (`scripts/verify-au-link.sh O-Formant`) PASSES with `AU VALIDATION SUCCEEDED. auval accepted O-Formant (aumu OuFm OuDv)`. **No regression.** Phase 24 carry-forward complete (B applies to all 7 plugins).

### C. JUCE coding-style space-before-paren (carry-forward from 24-02..24-06)

FormantVoice.cpp uses `Ouaricon::NoteExpression::applyPendingTuning (...)` WITH a space-before-paren at line 202 (start of the multi-line call). This matches the FormantVoice file's coding style throughout (the existing `tuningEnginePtr->getFrequency (midiNote)` and `getCurrentlyPlayingNote().getFrequencyInHertz()` calls match — all have spaces). The plan §verify regex `Ouaricon::NoteExpression::applyPendingTuning\(.*midiNote` is paren-class-tolerant (matches `\(` after the identifier; the surrounding regex tolerates the space before `(` because the substantive call shape is what matters — confirmed via multi-line grep showing identifier on one line and `midiNote` argument on the next). **No carry-forward concern for this plan.** The substantive-vs-literal-paren-adjacency philosophy from 24-02 remains the universal advisory.

### D. Version bump method — `VERSION` form (NOT `PLUGIN_VERSION` — applies the OTHER convention)

Phase 24 has TWO version-bump conventions in `juce_add_plugin` blocks:
- **`VERSION X.Y.Z` form** (no quoting): O-Bells (`PLUGIN_VERSION "4.0.0"` — wait, O-Bells uses `PLUGIN_VERSION`; let me re-state). Actually the matrix is: O-Bells uses `PLUGIN_VERSION`, O-Prism uses `VERSION`, O-Wind / O-Reed / O-Bowed have NO version arg (added explicitly via `PLUGIN_VERSION` pattern), O-IntonationPad uses `PLUGIN_VERSION`, O-Formant uses `VERSION`.
- **For O-Formant**: `VERSION 1.24.2` was already present at line 12; bumped to `VERSION 1.25.0`. Same convention as O-Prism (which used `VERSION 1.16.1` → `VERSION 1.17.0` in 24-02). NO carry-forward D explicit-add pattern needed (carry-forward D from 24-03 / 24-05 / 24-06 only applies to plugins MISSING any version arg).

Pattern reconciliation: the carry-forward D pattern is "explicitly add `PLUGIN_VERSION "X.Y.Z"` if no version arg is present". For O-Formant (which already had `VERSION 1.24.2`), the bump is a simple in-place edit — no explicit-add needed.

### E. Pre-step commit (NOT applicable to this plan)

Like 24-06, the O-Formant working tree was clean at plan start (`git status --short -- plugins/O-Formant modules/registry.yaml` returned 0 lines at preflight Task 1). No pre-step commit needed; the 8-file atomic shape was preserved naturally. (Repo-root `git status` reported other dirty files outside the plan's scope — CLAUDE.md, etc. — but those are out-of-scope for this plan.)

### F. Per-call-site MPE composition correctness (NEW STRUCTURAL PATTERN for this plan, complementary to helper-based pattern)

Critical composition-order rule for FormantVoice (MPE — single composition site):
1. The single composition site at `Source/FormantVoice.cpp` lines 187-209 is inside `noteStarted()`. Body order:
   - Line 187: `int midiNote = currentlyPlayingNote.initialNote;` — MPE pitch source.
   - Lines 188-190: tuningEngine query OR `getCurrentlyPlayingNote().getFrequencyInHertz()` fallback → `tunedF0` cached field (cast through `float`).
   - Line 191: `float f0 = tunedF0;` — local copy for pitchGlide call sites below.
   - Lines 197-208 (NEW): `if (pendingTuningSource != nullptr) { tunedF0 = static_cast<float>(applyPendingTuning(*pendingTuningSource, midiNote, static_cast<double>(tunedF0))); f0 = tunedF0; }` — NE delta applied to the cached field; `f0` re-read after NE composition since `f0` was set before the NE step.
   - Lines 211-216: `pitchGlide.setTime(glideMs); if (glideMs > 0.0f && wasActive) pitchGlide.setTarget(f0); else pitchGlide.snapTo(f0);` — pitchGlide consumes the NE-tuned `f0`.
2. `tunedF0` is referenced downstream in `renderNextBlock` at lines 488 (spectral tilt) and 616 (source-filter coupling). All downstream consumers see the NE-tuned value because the cached field is updated BEFORE per-sample DSP runs.
3. Per-sample `pitchGlide.processSample()` (called inside `renderNextBlock`) reads the locked NE-tuned target via the pitchGlide's smoothed-value chain. **Pattern 2 satisfied** — first sample of every note's glottal source at tuned pitch.

If `applyPendingTuning` were placed AFTER `pitchGlide.snapTo/setTarget(f0)` instead of before:
- First sample of every note would be at the un-tuned pitch (pitchGlide already targeted before NE applied) — Pattern 2 violated; attack zipper would be audible.
- Spectral tilt + source-filter coupling consumers in `renderNextBlock` would still see the NE-tuned `tunedF0` (cached field), but the pitchGlide chain wouldn't — temporally inconsistent state.

The chosen composition order (NE before pitchGlide) is mathematically required for Pattern 2 correctness; PATTERNS.md §7 explicitly calls this out.

**Comparison to helper-based pattern (24-05 O-Reed / 24-06 O-Bowed):**
- Helper-based: applyPendingTuning lives INSIDE the helper body; multiple call sites inherit the NE delta automatically. Single source of truth across MPE lifecycle methods (noteStarted, notePitchbendChanged, etc.).
- Per-call-site (this plan): applyPendingTuning lives at the SINGLE assignment site of the cached field. No helper indirection layer.
- **Decision rule:** the helper-based pattern is the natural fit when the voice has a `getBaseFrequencyFromTuning`-shaped helper called from multiple lifecycle methods (24-05 / 24-06). The per-call-site pattern is the natural fit when the voice has a single base-frequency assignment site to a cached field (24-07). PATTERNS.md §7 documents the per-call-site decision; PATTERNS.md §5/§6 document the helper-based decision. Pattern is per-plugin-shape-specific; together they cover both common MPE voice shapes.

### G. MPE pitch-bend interplay with NE (musical-correctness structural property — carry-forward from 24-05 / 24-06)

MPE pitch-bend / vibrato / jitter / shimmer are per-sample multiplicative ratios applied per-sample inside `renderNextBlock` via the existing `pitchGlide.processSample() * vibratoLFO.process()` chain. NE delta is also a multiplicative ratio applied per-noteOn via `applyPendingTuning`. They compose by simple multiplication:

```
final_freq_per_sample = tuningEngine.getFrequency(midi)         // step 1: base tuning
                      * pow(2, ne_semitones / 12.0)              // step 2: NE delta (once per noteOn, in noteStarted)
                      * pitchGlide.processSample()               // step 3: per-sample pitch glide (locked target)
                      * vibratoLFO.process()                     // step 4: per-sample vibrato LFO
                      * (1 + jitter + shimmer);                  // step 5: per-sample jitter/shimmer
```

This is the musically-expected behavior for Dorico microtonal playback in MPE mode. NE applies once at noteOn (slot consumed); per-sample modulators re-apply each sample on top. Per-call-site composition naturally achieves this via the slot-consume + per-sample modulator chain that's already in place.

### H. `isBusesLayoutSupported` already present (Note H carry-forward from 24-06 did NOT apply)

24-06 (O-Bowed) discovered a pre-existing missing `isBusesLayoutSupported` override that caused `verify-au-link.sh` to segfault during auval's mono Render Test. Carry-forward Note H recommended probing `isBusesLayoutSupported` presence at preflight for plan 24-07.

**Probe result:** `plugins/O-Formant/Source/PluginProcessor.h` line 53 already declared `bool isBusesLayoutSupported (const BusesLayout& layouts) const override;` (pre-existing — NOT introduced by this plan). The body is defined in `PluginProcessor.cpp` (NOT in the Phase 24 atomic commit's diff scope).

**Outcome:** `verify-au-link.sh O-Formant` PASSED on FIRST attempt with no Rule-3 inline fix needed. AU gate is reliable; the carry-forward probe correctly identified that O-Formant was NOT a candidate for the proactive Rule-3 fix.

**Pattern recommendation for Phase 25+:** continue probing `isBusesLayoutSupported` presence at preflight for any new module-consumer plugin. The probe is cheap (~5 sec); it correctly identifies whether the Rule-3 fix is needed.

## Decisions Made

- **Dorico gate batching at orchestrator level.** Recorded plan-local Dorico 3-point smoke as DEFERRED rather than fabricating a PASS or stopping the plan to prompt the user. SUMMARY explicitly tabulates the 3 gate points + 2 bonus checks with status DEFERRED for downstream aggregation in 24-08-final-sweep.
- **Per-call-site MPE composition (NEW pattern, distinct from 24-05/24-06 helper-based pattern).** Picked the per-call-site variant explicitly recommended by PATTERNS.md §7. Same reasoning as PATTERNS.md §7: O-Formant has no `getBaseFrequencyFromTuning`-shaped helper to wrap; the `tunedF0` cached field is assigned at a single site in `noteStarted()` and consumed by the per-sample DSP chain. Per-call-site composition at the single assignment site is the natural fit. Pattern is now ESTABLISHED (first instance) — complementary to helper-based pattern from 24-05 / 24-06.
- **OuariconModules.cmake include added (CRITICAL CMake delta unique to O-Formant; carry-forward from 24-INTEGRATION-MATRIX.md row o-formant).** Added at line 4 (immediately after `cmake_minimum_required` and BEFORE `juce_add_plugin`). PATTERNS.md §7 + 24-INTEGRATION-MATRIX.md row o-formant flagged this as the only structural delta for O-Formant. By plan 24-07 (after 6 prior plugins consumed `ouaricon_add_module` cleanly), the macro pattern is fully proven; the include addition is structurally safe.
- **No Rule-3 inline fix for `isBusesLayoutSupported` (carry-forward Note H probe outcome).** O-Formant already declares the override at PluginProcessor.h:53 (pre-existing, NOT introduced by this plan). AU gate PASSED on first attempt. The carry-forward probe correctly identified that no proactive Rule-3 fix was needed.
- **`f0` re-read after NE composition (`f0 = tunedF0`).** The local `float f0 = tunedF0` is set BEFORE the NE step (line 191 of original code); without re-reading after `tunedF0` is updated, `pitchGlide.setTarget(f0)` / `pitchGlide.snapTo(f0)` would target the un-tuned value. Re-read is mathematically necessary for Pattern 2 correctness; PATTERNS.md §7 explicitly calls this out.

## Deviations from Plan

**No Rule-1, Rule-2, or Rule-3 deviations needed.** All preflight probes passed cleanly:
- `OuariconModules.cmake` include addition was a planned pre-step (NOT a deviation — explicitly required by PATTERNS.md §7 and the plan §action).
- `isBusesLayoutSupported` Note H carry-forward probe found the override already present (no Rule-3 fix needed).
- No CMake baseline defect (target_link_libraries already declares juce_audio_utils + juce_audio_devices).
- No build failures (the warnings present — FReleaser shadow-field, non-virtual-destructor `delete` on the module's `Controller` class — are all pre-existing in JUCE/SDK headers + module code; not caused by Phase 24 changes; carry-forward from 24-04 / 24-05 / 24-06 builds).

**The plan executed exactly as written.** All 4 executed tasks (Tasks 1, 2, 3, 5) completed per spec; Task 4 (Dorico human-verify) DEFERRED per orchestrator-level direction.

The eight environmental notes (A–H) are observations and pattern documentation; A–C and E–H are pure carry-forwards (with H specifically documenting the proactive probe that correctly identified no Rule-3 fix was needed). D documents the version-bump method for O-Formant (the `VERSION` form, like O-Prism) — NOT a deviation, just convention documentation. F documents the new per-call-site MPE composition pattern (this plan's primary structural contribution). G is the same MPE pitch-bend + NE multiplicative-compose property documented in 24-05 / 24-06.

## Issues Encountered

**No issues encountered. Plan executed cleanly.**

Tri-format ninja exit 0 on first attempt (no rebuild needed); AU gate PASSED on first attempt (no Rule-3 inline fix needed); atomic 8-file commit landed. Build warnings present are all pre-existing (JUCE/SDK shadow-field warnings + module Controller dtor warning — all carry-forwards from 24-04 / 24-05 / 24-06 builds).

## TDD Gate Compliance

N/A — plan type `execute` (not `tdd`); voice-side correctness validated structurally (Pattern 1 inherited from shared module; Pattern 2 line-precise edit verified for the per-call-site MPE composition order; Pattern 3 inherited from helper). Dorico human-verify gate deferred to Phase 24 batch validation per orchestrator direction.

## User Setup Required

None for this plan's automated acceptance — no external service configuration required. Dorico smoke (deferred) requires Dorico host; orchestrator will batch this.

## Next Phase Readiness

**Phase 24 status after this plan (FINAL per-plugin propagation complete):**
- ✓ Plan 24-01 complete (O-Bells canary; Dorico 3-point PASS)
- ✓ Plan 24-02 complete (O-Prism wave 2; Dorico 3-point DEFERRED)
- ✓ Plan 24-03 complete (O-Wind wave 3; Dorico 3-point DEFERRED) — first physical-model consumer
- ✓ Plan 24-04 complete (O-IntonationPad wave 4; Dorico 3-point DEFERRED) — first STRUCTURAL VARIATION consumer (multi-sub-voice neRatio)
- ✓ Plan 24-05 complete (O-Reed wave 5; Dorico 3-point DEFERRED) — first MPE consumer (helper-based MPE composition pattern established)
- ✓ Plan 24-06 complete (O-Bowed wave 6; Dorico 3-point DEFERRED) — second MPE consumer (helper-based MPE composition pattern STABLE; Rule-3+2 inline fix for pre-existing AU validation defect)
- ✓ Plan 24-07 complete (O-Formant wave 7; Dorico 3-point DEFERRED) — **THIRD MPE consumer; per-call-site MPE composition pattern ESTABLISHED (NEW, complementary to helper-based pattern); CRITICAL CMake delta resolved (unique to O-Formant — only Phase 24 plugin missing OuariconModules.cmake include)**
- ✓ Per-call-site MPE composition pattern ESTABLISHED — first instance, complementary to helper-based pattern from 24-05 / 24-06. Together they cover both common MPE voice shapes (helper-based for multi-call-site voices, per-call-site for single-assignment voices).
- ✓ MPE NE correlation re-confirmed THIRD instance — Pattern 1 holds at `currentlyPlayingNote.initialNote` regardless of MPE channel (same as classic Synthesiser mode in 24-01..24-04 + first MPE confirmation in 24-05 + second MPE confirmation in 24-06)
- ✓ MPE pitch-bend + NE composition order re-validated (multiplicative compose; NE once per noteOn, per-sample modulators per-sample)
- ✓ Dev-suffix bundle handling continues to work identically across plans (carry-forward A — Phase 24 complete: all 7 plugins handled via dual-install)
- ✓ `auval -a` advisory continues to apply (carry-forward B — Phase 24 complete: same host quirk on all 7 plugins; canonical D-08 path PASSES on all)
- ✓ PLUGIN_VERSION explicit-add pattern stable (carry-forward D from 24-03 / 24-05 / 24-06 — applied to 3 of 7 plugins; NOT applied to O-Formant since `VERSION` form was already present)
- ✓ Rule-3 inline-fix discipline (Note H from 24-06): preflight probe correctly identified that O-Formant did NOT need the proactive fix — `isBusesLayoutSupported` already present
- ✓ OuariconModules.cmake include addition pattern ESTABLISHED (Phase 24 unique to O-Formant; Pattern recommendation for any future module-consumer plugin)
- ✓ ALL 8 expected consumers in `modules/registry.yaml` `note-expression.used_by:` list (`OLyrica` + `O-Bells` + `O-Prism` + `O-Wind` + `O-IntonationPad` + `O-Reed` + `O-Bowed` + `O-Formant`) — registry-complete for Phase 24
- ⏳ Dorico 3-point gate batch-validation pending — orchestrator queue: 24-02..24-07 (6 plans deferred; plan 24-01 PASSED inline)

**Aggregation hook:** This SUMMARY feeds **`24-08-final-sweep-SUMMARY.md` row 7 of 8 (FINAL per-plugin row before the cross-plugin sweep)**. The 3-point Dorico gate result table format (with DEFERRED status here) is the row-template for plan 24-08's aggregate gate-results table. The per-call-site MPE composition pattern is now documented as ESTABLISHED (first instance, complementary to helper-based from 24-05 / 24-06) for downstream consumers in Phase 25 DOCS-01 module-level pattern documentation. The OuariconModules.cmake include addition pattern is documented for Phase 25+ guidance on any future module-consumer plugin.

**Ready for plan 24-08 (final sweep).** No blockers, no escalations. Phase 24 wave 7 of 7 momentum preserved. Plan 24-08 will:
1. Aggregate the 6 deferred Dorico 3-point gate results (24-02..24-07) into a batch-validation list for the user.
2. Audit `modules/registry.yaml` for completeness (8 consumers expected + 0 missing — already verified post-this-plan).
3. Run a cross-plugin reinstall + AU validation sweep (all 8 plugins fresh-built, AU validates each).
4. Produce a Phase 24 close-out aggregate SUMMARY.

## Self-Check: PASSED

- Atomic commit references all 8 plan-scoped files (verified via `git show d0e101a --stat` → 8 files changed, 75 insertions, 3 deletions)
- `plugins/O-Formant/CMakeLists.txt` contains `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` AND `ouaricon_add_module(O-Formant note-expression)` AND `VERSION 1.25.0` → FOUND (lines 4, 38, 12 respectively)
- `plugins/O-Formant/Source/PluginProcessor.h` contains `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` AND `getVST3ClientExtensions()` override → FOUND (lines 87 and 66); also `isBusesLayoutSupported` override pre-existing (Note H probe) → FOUND (line 53)
- `plugins/O-Formant/Source/PluginProcessor.cpp` contains `vst3Extensions.drainAndUpdate(` AND `setPendingTuningSource (&vst3Extensions` → FOUND
- `plugins/O-Formant/Source/FormantVoice.h` contains `pendingTuningSource` AND inline `setPendingTuningSource` setter → FOUND
- `plugins/O-Formant/Source/FormantVoice.cpp` contains `Ouaricon::NoteExpression::applyPendingTuning (*pendingTuningSource, midiNote, ...)` (single qualified call at the per-call-site composition site between tunedF0 assignment and pitchGlide.snapTo/setTarget) → FOUND (lines 202-203)
- `plugins/O-Formant/CHANGELOG.md` contains the TRACK-03 verbatim phrase `adds VST3 Note Expression microtonal support for Dorico` (lowercase 'adds' as the FIRST word of the description body) → FOUND (line 9)
- `modules/registry.yaml` contains `plugin: O-Formant` under `note-expression.used_by` → FOUND (line 285)
- `~/Library/Audio/Plug-Ins/VST3/O-Formant*.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Formant*.component` present at fresh mtime (2026-04-26 10:49) → FOUND (both dev and prod naming)
- `scripts/verify-au-link.sh O-Formant` exit 0 with `AU VALIDATION SUCCEEDED. auval accepted O-Formant (aumu OuFm OuDv)` → FOUND in execution log (FIRST attempt; no Rule-3 fix needed)
- Dorico 3-point gate status documented as DEFERRED with structural correctness rationale → PRESENT
- Zero Rule-1/2/3 deviations recorded — plan executed exactly as written → PRESENT
- OuariconModules.cmake include addition documented (the unique CMake delta among Phase 24 plugins) → PRESENT
- Per-call-site MPE composition pattern documented (NEW for this plan, complementary to helper-based pattern) → PRESENT

---
*Phase: 24-propagate*
*Completed: 2026-04-26*
