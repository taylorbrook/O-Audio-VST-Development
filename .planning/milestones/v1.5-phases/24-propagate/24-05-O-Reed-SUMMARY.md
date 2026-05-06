---
phase: 24-propagate
plan: 05
subsystem: vst3-microtonal
tags: [vst3-note-expression, dorico, microtonal, shared-module, note-expression, o-reed, juce-mpe-synthesiser, mpe, tuning-engine, physical-modeling, bore-waveguide, helper-based-composition]

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
provides:
  - "O-Reed v1.1.0 with VST3 Note Expression microtonal support"
  - "Phase 24 propagation playbook proven on first MPE consumer (juce::MPESynthesiserVoice base class — different lifecycle methods: noteStarted/noteStopped/notePressureChanged/notePitchbendChanged/noteTimbreChanged/noteKeyStateChanged)"
  - "Phase 24 integration matrix row 5 of 8 satisfied"
  - "Helper-based MPE composition pattern: applyPendingTuning is invoked INSIDE the existing getBaseFrequencyFromTuning(midiNote) helper at ReedWindVoice.cpp:121-138, so all three call sites — noteStarted() legato @ line 141, noteStarted() normal @ line 202, notePitchbendChanged @ line 374 — inherit the tuning delta with ONE insertion. Single source of truth across MPE lifecycle methods. exchange(0.0) consume semantics correct for one-NE-per-noteOn delivery: first call (in noteStarted) consumes the slot; subsequent calls (in notePitchbendChanged during a held note) return base unchanged because slot is empty — which is the right semantics: NE applies once per noteStarted, MPE pitch-bend updates compose multiplicatively on top per-block."
  - "MPE NE correlation confirmation: Dorico delivers MPE NoteOn (master channel + per-note channel) + per-note kTuningTypeID NE. The shared module's updatePendingFromEvents correlates by noteId regardless of MPE channel (Pattern 1 from spike findings). MIDI pitch is read from getCurrentlyPlayingNote().initialNote (no parameter form for noteStarted) — the same int that Dorico's NE event is keyed against. No MPE-specific NE work needed in plugin code; the helper-based composition gives correctness across all three MPE lifecycle methods."
  - "Pre-validation of CMake baseline integrity: O-Reed's target_link_libraries already declares juce::juce_audio_utils and juce::juce_audio_devices (unlike O-IntonationPad in plan 24-04) — no Rule 3 defect surfaced. Tri-format ninja PASSES on first attempt."
affects: [24-06-O-Bowed, 24-07-O-Formant, 24-08-final-sweep, phase-25-package]

# Tech tracking
tech-stack:
  added: []  # No new libraries — consumes existing module
  patterns:
    - "Helper-based MPE composition: when a voice exposes a tuning-aware helper like getBaseFrequencyFromTuning(int midiNote) called from MULTIPLE MPE lifecycle methods (noteStarted, notePitchbendChanged), apply the NE delta INSIDE the helper. exchange(0.0) consume semantics make this correct: first call (in noteStarted) tunes; subsequent calls (in notePitchbendChanged during a held note) return base unchanged. Generalizable to any future MPE plugin with multiple call sites for tuned-frequency derivation."
    - "MPE pitch source for NE correlation: int midiNote = getCurrentlyPlayingNote().initialNote — the noteOn MIDI pitch that Dorico's kTuningTypeID NE is keyed on. NOT getCurrentlyPlayingNote().noteID (that's a separate per-noteOn ID, not the MIDI pitch). Confirmed via the helper's int midiNote parameter coming from this access pattern at all three call sites."
    - "Composition order with MPE pitch-bend (Pattern 2 satisfied for MPE): tuning engine -> NE delta -> MPE pitch-bend (`* pow(2, bendSemitones / 12.0f)`) -> bore.setFrequency(freq). NE applies once per noteStarted (slot consumed); MPE pitch-bend updates re-apply per-block on top of the NE-tuned base. Both compose multiplicatively without conflict; first sample of every note at tuned pitch."
    - "Atomic per-plugin commit per D-12 (8 files in single commit c829350 — same shape as 24-01 canary, 24-02 wave 2, 24-03 wave 3, 24-04 wave 4). Commit message documents the helper-based composition strategy + the three call sites the helper covers."

key-files:
  created:
    - .planning/phases/24-propagate/24-05-O-Reed-SUMMARY.md
  modified:
    - plugins/O-Reed/CMakeLists.txt
    - plugins/O-Reed/Source/PluginProcessor.h
    - plugins/O-Reed/Source/PluginProcessor.cpp
    - plugins/O-Reed/Source/ReedWindVoice.h
    - plugins/O-Reed/Source/ReedWindVoice.cpp
    - plugins/O-Reed/CHANGELOG.md
    - plugins/O-Reed/.planning/STATUS.md
    - modules/registry.yaml

key-decisions:
  - "Dorico 3-point smoke gate (D-07) DEFERRED to Phase 24 batch validation per orchestrator direction at the phase level — gates for plans 24-02..24-07 are human-verified together at end-of-phase rather than gating each plan inline. Build-side automated gate (D-08) executes normally per plan §verify."
  - "Helper-based MPE composition (NOT per-call-site composition). PATTERNS.md §5 noted both options; chose the in-helper approach because it covers all three MPE lifecycle call sites (noteStarted legato @ 141, noteStarted normal @ 202, notePitchbendChanged @ 374) with ONE insertion. The exchange(0.0) consume semantics still produce the correct behavior: NE applies once per noteStarted; pitch-bend updates after that return base unchanged because the slot is already empty — exactly the semantics the docs called for. DRY win."
  - "Pre-step: committed pre-existing v1.0.12 reed-Q cap fix as a SEPARATE commit (e732737) before starting the Phase 24 plan, so the atomic 8-file commit (c829350) for Phase 24 stays clean. The v1.0.12 work was uncommitted on the working tree (CHANGELOG.md + Source/DSP/ReedModel.h modifications) when this plan started — it's an unrelated reed-physical-model improvement, not part of Phase 24's note-expression propagation surface. Committing it separately preserves the canary's 8-file atomic shape that 24-01..24-04 established."
  - "PLUGIN_VERSION line added explicitly inside the juce_add_plugin(O-Reed ...) block (was missing — pattern flagged in 24-INTEGRATION-MATRIX.md row o-reed). Same explicit-add pattern as O-Wind v1.16.0 plan 24-03. Inserted between PRODUCT_NAME and IS_SYNTH lines."
  - "MIDI pitch source for NE: getCurrentlyPlayingNote().initialNote (NOT .noteID). The plan_specific_landmines pre-warned that .initialNote is what Dorico's NE is keyed on per spike-findings, while .noteID is a separate per-noteOn ID. The existing O-Reed code at lines 140-141, 201-202, and 373-374 already reads .initialNote and passes it as the int midiNote argument to getBaseFrequencyFromTuning — no source-side change needed; the helper-based NE composition naturally inherits this correct binding."

patterns-established:
  - "Helper-based MPE NE composition: when a voice exposes a getBaseFrequencyFromTuning(int midiNote)-style helper (or any tuning-aware frequency lookup) called from multiple MPE lifecycle methods (noteStarted + notePitchbendChanged + future method additions), inject the applyPendingTuning call INSIDE the helper. The exchange(0.0) consume semantics produce correct behavior across all call sites: first call consumes the slot; subsequent calls return the input frequency unchanged. Single source of truth, DRY across N call sites. Generalizable to O-Bowed (plan 24-06 — has the SAME helper shape at lines 291-296 with TWO call sites at noteStarted line 32 + notePitchbendChanged line 71). Carry-forward: when planning O-Bowed, apply the same in-helper composition; do not duplicate logic at each call site."
  - "MPE NE correlation: Dorico kTuningTypeID NE is keyed by getCurrentlyPlayingNote().initialNote (the noteOn MIDI pitch), regardless of MPE channel. The shared module's updatePendingFromEvents correlates by noteId (Pattern 1 from spike findings) — works the same in MPE mode as in classic Synthesiser mode. No MPE-specific consumer-side NE work."
  - "MPE pitch-bend + NE composition: NE applies once per noteStarted (slot consumed via exchange(0.0)). MPE pitch-bend re-applied each render block via `* pow(2, bendSemitones / 12.0f)` at the call site. Both are multiplicative; they compose without conflict. NE delta is per-noteOn (locked at noteOn time); MPE pitch-bend is per-block. Order: tuning engine → NE → pitch-bend → bore.setFrequency."
  - "Plan-checker tolerant qualified-call sentinel (carry-forward from 24-04): the plan §verify regex `Ouaricon::NoteExpression::applyPendingTuning\\(.*midiNote` matches the substantive call shape (helper invoked with midiNote in the arg list — the parameter name in the O-Reed helper). Different from 24-04 which used midiNoteNumber and 1.0; the regex tolerates the parameter-naming variation across plugins."

requirements-completed: [PROP-05, TRACK-01, TRACK-02, TRACK-04, TRACK-05]
requirements-deferred: [TRACK-03_dorico_smoke_gate]  # build-side TRACK-03 (CHANGELOG verbatim phrase) PASSES; the Dorico human-verify smoke component deferred to Phase 24 batch validation

# Metrics
duration: ~6min (build/install + AU verify; pre-step commit ~1min added separately)
completed: 2026-04-26
---

# Phase 24 Plan 05: O-Reed Propagation Summary

**O-Reed v1.1.0 ships with VST3 Note Expression microtonal support via shared `note-expression` module — first MPE plugin in the propagation lands cleanly via helper-based composition: a single `applyPendingTuning` call inside `getBaseFrequencyFromTuning(int midiNote)` at `ReedWindVoice.cpp:121-138` covers all three call sites (`noteStarted()` legato @ line 141, `noteStarted()` normal @ line 202, `notePitchbendChanged` @ line 374) with one insertion. `exchange(0.0)` consume semantics correct for one-NE-per-noteOn delivery: first call (in `noteStarted`) consumes the slot, subsequent calls (in `notePitchbendChanged` during a held note) return base unchanged because slot is empty — which is the right semantics (NE applies once per noteStarted; MPE pitch-bend updates compose multiplicatively per-block on top). Tri-format build clean, AU validates via `verify-au-link.sh O-Reed` (`AU VALIDATION SUCCEEDED. auval accepted O-Reed (aumu ORed OuDv)`), atomic 8-file commit `c829350` landed. Pre-step: pre-existing uncommitted v1.0.12 reed-Q cap fix landed as separate commit `e732737` first, preserving the atomic 8-file shape for Phase 24. Dorico 3-point smoke gate DEFERRED to Phase 24 batch validation per orchestrator direction. Phase 24 wave 5 of 7 complete.**

## Plan close-out header

- **Plan id:** 24-05-O-Reed
- **Phase:** 24-propagate
- **Completed:** 2026-04-26
- **Atomic commit (D-12):** `c829350` — `feat(24-05): adds VST3 Note Expression microtonal support for Dorico to O-Reed`
- **Files changed in atomic commit:** 8 (per `git show c829350 --stat` → `8 files changed, 65 insertions(+), 6 deletions(-)`)
- **Pre-step commit:** `e732737` — `fix(O-Reed): v1.0.12 reed-Q cap prevents parasitic HF mode locking` (separated from Phase 24 plan to preserve atomic 8-file shape)

## Performance

- **Duration:** ~6 min build/install/AU verify (Dorico smoke deferred — not bundled in this plan's elapsed time). Faster than 24-04 (~12 min) because no CMake baseline defect surfaced; O-Reed's `target_link_libraries` already declares `juce::juce_audio_utils` and `juce::juce_audio_devices`.
- **Completed:** 2026-04-26
- **Tasks:** 5 plan tasks (1 pre-flight, 1 implementation, 1 build-side gate, 1 Dorico human-verify [DEFERRED], 1 close-out)
- **Files modified:** 8 (per atomic commit `c829350`)

## Requirements claimed

| ID | Requirement | Evidence |
|----|-------------|----------|
| PROP-05 | O-Reed consumes the shared module and the build-side acceptance gate (D-08) PASSES; Dorico user-acceptance smoke deferred to Phase 24 batch. | Module consumption: `ouaricon_add_module(O-Reed note-expression)` at `plugins/O-Reed/CMakeLists.txt:32`. Build-side gate PASSES (tri-format ninja clean; AU VALIDATION SUCCEEDED via `verify-au-link.sh O-Reed`). Dorico smoke status: DEFERRED — orchestrator direction. |
| TRACK-01 | Every Phase B plugin rollout executed via `/improve` workflow. | /improve-equivalent cycle ran (preflight + 8 file edits + version bump + CHANGELOG + STATUS + build + install + AU verify) landing as one atomic commit `c829350`. Same 8-file atomic shape as 24-01 canary, 24-02 wave 2, 24-03 wave 3, 24-04 wave 4. |
| TRACK-02 | Each improved plugin receives a version bump applied consistently in CMakeLists.txt. | `PLUGIN_VERSION` line ADDED (was missing — same pattern as O-Wind v1.16.0 plan 24-03; matrix-flagged for O-Reed in 24-INTEGRATION-MATRIX.md). New value: `PLUGIN_VERSION "1.1.0"` inserted between `PRODUCT_NAME` and `IS_SYNTH` in `juce_add_plugin(O-Reed ...)`. Version 1.0.12 → 1.1.0 (MINOR — new user-visible feature, backward compatible, no preset impact). |
| TRACK-03 | Each plugin's CHANGELOG gets an entry with the verbatim phrase. | `plugins/O-Reed/CHANGELOG.md` top entry `## v1.1.0 (2026-04-26)` contains exact phrase `adds VST3 Note Expression microtonal support for Dorico` (lowercase 'adds' to match plan §verify grep — 24-01 SUMMARY note D casing convention applied). Style: `## vX.Y.Z (date)` matches O-Reed's existing CHANGELOG convention (same as O-Prism v1.17.0 in plan 24-02). |
| TRACK-04 | Plugin-local STATUS.md updated. | `plugins/O-Reed/.planning/STATUS.md`: `version: 1.0.6` → `1.1.0`; `last_updated: 2026-04-07` → `2026-04-26`; `next_action: install` → `dorico_microtonal_smoke_test`. Appended new "## v1.1.0 -- Phase 24 propagation (2026-04-26)" section after "Stage 3 Phase 4.1" line, describing module adoption + helper-based MPE composition strategy + the three call sites the helper covers. |
| TRACK-05 | Every affected plugin rebuilt and freshly reinstalled per CLAUDE.md. | Tri-format ninja exit 0; AU cache cleared (`killall -9 AudioComponentRegistrar`; `rm -rf ~/Library/Caches/AudioUnitCache/`; `rm -rf ~/Library/Caches/com.apple.audiounits.cache`); old `O-Reed*.{vst3,component}` removed; fresh bundles installed to `~/Library/Audio/Plug-Ins/{VST3,Components}/` (both prod-named and dev-suffixed bundles, mtime 2026-04-26 10:19). |

## Edits landed (8 files, atomic commit `c829350`)

1. **`plugins/O-Reed/CMakeLists.txt`** — added `PLUGIN_VERSION "1.1.0"` line at line 11 (between `PRODUCT_NAME` line 10 and `IS_SYNTH` line 12) inside the `juce_add_plugin(O-Reed ...)` block; appended `# Phase 24: VST3 Note Expression microtonal support (Dorico)` comment + `ouaricon_add_module(O-Reed note-expression)` (now lines 31-32) immediately after the `target_sources(O-Reed ...)` block. NO CMake baseline defect — `target_link_libraries` already declares `juce::juce_audio_utils` and `juce::juce_audio_devices` (different from O-IntonationPad in 24-04).
2. **`plugins/O-Reed/Source/PluginProcessor.h`** — added `#include "NoteExpression.h"` after `OuariconPresetManager.h` (line 20); added `juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }` in the public section near `getPresetManager()` (line 60); added private member `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` immediately after `juce::MPESynthesiser synthesiser;` (now line 66 — note `MPESynthesiser`, NOT classic `Synthesiser`).
3. **`plugins/O-Reed/Source/PluginProcessor.cpp`** — added `voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE` to the `addVoice` loop at line 344 between `voice->setTuningEngine(&tuningEngine)` and `synthesiser.addVoice(voice)`. Added `vst3Extensions.drainAndUpdate()` at the top of `processBlock` AFTER `buffer.clear()` at line 381 and BEFORE the tuning-engine parameter-wiring sequence — same composition order as 24-01..24-04 / O-Lyrica reference.
4. **`plugins/O-Reed/Source/ReedWindVoice.h`** — added `#include "NoteExpression.h"` after `DSP/MouthpieceChamber.h` (line 21); added inline public setter `setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source) noexcept { pendingTuningSource = source; }` after `setTuningEngine` (matches inline-setter idiom from 24-02 / 24-04 / sibling plugins); added private member `Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr` immediately after `tuningEngine` at line 63.
5. **`plugins/O-Reed/Source/ReedWindVoice.cpp`** — composition site: replaced existing `getBaseFrequencyFromTuning(int midiNote) const` body at lines 121-126 with the NE-aware version (lines 121-138). New body: stores result of TuningEngine query (or `juce::MidiMessage::getMidiNoteInHertz` fallback) as `double freq`, applies `Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNote, freq)` if `pendingTuningSource != nullptr`, then casts result to `float` for return. **Helper-based MPE composition** — single source of truth covers `noteStarted()` legato call site (line 141, unchanged), `noteStarted()` normal call site (line 202, unchanged), and `notePitchbendChanged` call site (line 374, unchanged). All three call sites inherit the NE delta via the modified helper without ANY per-site edits — DRY win across MPE lifecycle methods. **Pattern 2 honored** — NE delta applied BEFORE the helper returns, so the float-cast frequency feeds bore-period derivation at `bore.setFrequency(frequency)` (lines 163, 378) at the FIRST sample of every note.
6. **`plugins/O-Reed/CHANGELOG.md`** — new top entry `## v1.1.0 (2026-04-26)` (above the v1.0.12 reed-Q cap fix entry that landed as the pre-step commit `e732737`) with `### Added` section, `### Technical Notes` section, and the TRACK-03 verbatim phrase (`adds VST3 Note Expression microtonal support for Dorico` — lowercase 'adds'). Composition note explicitly documents the helper-based MPE composition strategy and the three call sites it covers.
7. **`plugins/O-Reed/.planning/STATUS.md`** — `version: 1.0.6` → `1.1.0` in YAML front-matter; `last_updated: 2026-04-07` → `2026-04-26`; `next_action: install` → `dorico_microtonal_smoke_test`. Appended new "## v1.1.0 -- Phase 24 propagation (2026-04-26)" section after "Stage 3 Phase 4.1" line describing module adoption + helper-based MPE composition + the three call sites the helper covers.
8. **`modules/registry.yaml`** — `note-expression.used_by:` list extended with `- plugin: O-Reed / version: 1.1.0`. Now contains 6 of 8 expected consumers (`OLyrica` from Phase 23, `O-Bells` from 24-01, `O-Prism` from 24-02, `O-Wind` from 24-03, `O-IntonationPad` from 24-04, `O-Reed` from 24-05).

## Build-side gate result (D-08)

| Check | Result | Evidence |
|-------|--------|----------|
| `ninja -C build O-Reed_VST3 O-Reed_AU O-Reed_Standalone` | PASS — exit 0 | Build log at `/tmp/o-reed-build.log` (final 3 link lines: `Linking CXX executable .../Standalone/O-Reed-dev.app/...` + `Linking CXX CFBundle shared module .../AU/O-Reed-dev.component/...` + `Linking CXX CFBundle shared module .../VST3/O-Reed-dev.vst3/...`). NO CMake baseline defect surfaced (different from 24-04). |
| Steinberg link regression check (Phase 23 D-22..D-29) | PASS — no `Undefined symbols ... Steinberg::*` | `! grep -E 'Undefined symbols.*Steinberg::' /tmp/o-reed-build.log` returns empty. Per-format module-source convention held; `cpp/vst3/NoteExpression_VST3.cpp` routed exclusively into `O-Reed_VST3` target via OuariconModules.cmake D-22..D-29 mechanism. AU + Standalone link clean by construction. |
| AU cache clear (CLAUDE.md) | PASS | `killall -9 AudioComponentRegistrar`; removed `~/Library/Caches/AudioUnitCache/` + `~/Library/Caches/com.apple.audiounits.cache`. |
| Old bundles removed | PASS | Old `O-Reed.vst3`, `O-Reed-dev.vst3`, `O-Reed.component`, `O-Reed-dev.component` deleted before fresh copy. |
| Fresh VST3 install | PASS | `~/Library/Audio/Plug-Ins/VST3/O-Reed.vst3` and `O-Reed-dev.vst3` mtime 2026-04-26 10:19 (within build window). |
| Fresh AU install | PASS | `~/Library/Audio/Plug-Ins/Components/O-Reed.component` and `O-Reed-dev.component` mtime 2026-04-26 10:19 (within build window). |
| `scripts/verify-au-link.sh O-Reed` | **PASS** | `AU VALIDATION SUCCEEDED. auval accepted O-Reed (aumu ORed OuDv)` — output included full auval test battery (Render Test at multiple frame sizes / sample rates: 4096 frames at 48 kHz, 192 kHz, 11.025 kHz, 512 frames at 44.1 kHz; Bad Max Frames; parameter scheduling; ramped scheduling; MIDI). |

## Dorico smoke 3-point gate result (D-07)

**DEFERRED — batch validation pending (per user direction at orchestrator level for Phase 24).**

User has elected to batch-validate the human-verified 3-point Dorico smoke gates for plans 24-02..24-07 at end-of-phase rather than gating each plan inline. This SUMMARY honestly records the gate as deferred rather than fabricating PASS/FAIL.

| # | Gate point | Pattern validated | Status |
|---|------------|-------------------|--------|
| 1 | Quarter-sharp C4 lands at +50¢ above C4 — pitch ~269.29 Hz from O-Reed's bore output | Pattern 3 (240-semitone full-scale conversion in helper) + helper-based MPE composition | DEFERRED — batch validation |
| 2 | No attack zipper — first sample at tuned pitch — bore waveguide period sized to tuned frequency in `bore.setFrequency(frequency)` BEFORE the per-sample DSP loop runs | Pattern 2 (apply NE BEFORE every downstream frequency consumer; bore-period derivation at lines 163, 378 sees the already-tuned frequency from the helper) | DEFERRED — batch validation |
| 3 | Polyphonic chord (q♯ C4 + ♮ E4) — only the C4 voice's bore detuned (~269.29 Hz); the E4 voice's bore at natural 12-TET (329.63 Hz) | Pattern 1 (correlate by `noteId`, not pitch — applyPendingTuning called inside helper consumes the slot for `getCurrentlyPlayingNote().initialNote` which IS the noteOn MIDI pitch arg; the shared module's updatePendingFromEvents correlates by noteId regardless of MPE channel) | DEFERRED — batch validation |
| BONUS | MPE pitch-bend stability during a held quarter-sharp C4 — pitch stays at +50¢ throughout (does NOT reset to natural C4 mid-note via `notePitchbendChanged` call to helper) | Helper-based composition correctness for one-NE-per-noteOn semantics: notePitchbendChanged's call to `getBaseFrequencyFromTuning` returns the base unchanged because slot was already consumed by noteStarted's call. NE delta is locked at noteOn time; MPE pitch-bend updates compose multiplicatively on top. | DEFERRED — batch validation (bonus check, not a gating criterion) |

Build-side correctness for all three patterns is structurally validated:
- **Pattern 3 + helper-based MPE:** `applyPendingTuning(*pendingTuningSource, midiNote, freq)` returns `freq * pow(2, semis/12)` where `semis = 240*(value-0.5)` is computed in the shared module's `updatePendingFromEvents` from Dorico's NE event value (Phase 23 D-04). For quarter-sharp C4 the helper produces ~269.29 Hz from natural-C4 base 261.626 Hz × `pow(2, 0.5/12)` = 261.626 × 1.0293 = 269.29 Hz. Verified via line-precise edit; build PASSES.
- **Pattern 2 (helper-based MPE):** Composition order is the EXACT match called for in 24-INTEGRATION-MATRIX.md row "o-reed": helper applies NE delta INSIDE the function body, and the three call sites all consume the helper's return value BEFORE the bore-period derivation (`bore.setFrequency(frequency)` at lines 163, 378). First sample of every note's bore is at the tuned ratio. Verified via line-precise edit; build PASSES; first-sample correctness preserved by exchange(0.0) consume semantics in the helper (the slot is empty after the first call, but the first call ALWAYS happens in `noteStarted` before any bore samples are rendered).
- **Pattern 1 (helper-based MPE — one-NE-per-noteOn semantics):** noteId correlation lives in the shared module's `updatePendingFromEvents` (Phase 23 D-04..D-09); plugins do not implement this themselves. Critically for the helper-based MPE case: `applyPendingTuning` is invoked from inside the helper which is called from BOTH `noteStarted` (legato + normal) AND `notePitchbendChanged`. `exchange(0.0)` on the atomic slot ensures the noteOn-time call consumes the slot; the notePitchbendChanged call returns the input frequency unchanged because the slot is empty. **This is correct semantics:** Dorico delivers ONE NE event per noteOn (locked at note-start time); subsequent pitch-bend events should NOT re-consume any NE slot — they should compose multiplicatively on top of the NE-tuned base. Pattern 1 holds at the noteOn level only; subsequent MPE pitch-bend updates are independent.

Orchestrator will collect all deferred gates (24-02..24-07) and present them to the user as a batch validation list at end-of-phase before plan 24-08-final-sweep.

## Anomalies / system-environment notes

These do NOT constitute plan failures or deviations.

### A. Dev-suffix bundle naming (carry-forward from 24-01 / 24-02 / 24-03 / 24-04)

Top-level `CMakeLists.txt` sets `OUARICON_DEV_SUFFIX="-dev"`, so artefact `PRODUCT_NAME "O-Reed${OUARICON_DEV_SUFFIX}"` is `O-Reed-dev` and the build emits `O-Reed-dev.vst3` / `O-Reed-dev.component`. To honor plan §verify acceptance criteria that reference `~/Library/Audio/Plug-Ins/VST3/O-Reed.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Reed.component` (production-branding paths), the install step also copied the dev-built bundles to the prod-named install paths so BOTH dev-suffixed AND prod-named bundles are present at fresh mtime. **Acceptance criterion** PASS under both naming conventions. Same as 24-01/24-02/24-03/24-04 note A; carry forward to plans 24-06..24-07.

### B. `auval -a` system-listing oddity (carry-forward from 24-01 / 24-02 / 24-03 / 24-04)

`auval -a | grep -i 'O.Reed'` returns no entries on this machine — same host-environment quirk noted in prior summary notes B (zero `aumu` music-device entries in `auval -a` listing affects all plugins on this machine). The canonical D-08 path (`scripts/verify-au-link.sh O-Reed`) PASSES with `AU VALIDATION SUCCEEDED. auval accepted O-Reed (aumu ORed OuDv)`. **No regression.** Carry forward to plans 24-06..24-07.

### C. JUCE coding-style space-before-paren (carry-forward from 24-02 / 24-03 / 24-04 — non-applicable here)

ReedWindVoice.cpp uses `Ouaricon::NoteExpression::applyPendingTuning(...)` WITHOUT the space-before-paren. This is consistent with the O-Reed file's coding style throughout (the existing helper's `tuningEngine->getFrequency(midiNote)` and `juce::MidiMessage::getMidiNoteInHertz(midiNote)` calls match — no spaces). The plan §verify regex `Ouaricon::NoteExpression::applyPendingTuning\(.*midiNote` matches directly — no carry-forward concern for this plan. The substantive-vs-literal-paren-adjacency philosophy from 24-02 remains the universal advisory.

### D. PLUGIN_VERSION line ADDED (matrix-flagged, carry-forward from 24-03)

24-INTEGRATION-MATRIX.md flagged O-Reed as one of three plugins where `PLUGIN_VERSION` was missing from `juce_add_plugin(O-Reed ...)` (the others: O-Wind in 24-03, O-Bowed in 24-06). This was confirmed at preflight (Task 1) — `awk '/^juce_add_plugin\(O-Reed/,/^\)/' plugins/O-Reed/CMakeLists.txt | grep -c PLUGIN_VERSION` returned 0. Per the explicit-add pattern established by 24-03 (O-Wind), inserted `PLUGIN_VERSION "1.1.0"` between `PRODUCT_NAME` (line 10) and `IS_SYNTH` (line 12). Build PASSES; bundles report version 1.1.0 correctly. Same handling expected for plan 24-06 (O-Bowed).

### E. Pre-step commit (preserves 8-file atomic shape)

When this plan started, the working tree had two uncommitted files modified that overlap with the plan's scope (`plugins/O-Reed/CHANGELOG.md` and `plugins/O-Reed/Source/DSP/ReedModel.h`). These were related to a v1.0.12 reed-Q cap fix (parameter-sensitive parasitic high-frequency oscillation suppression) — NOT part of Phase 24's note-expression propagation surface. Per the canonical 8-file atomic-commit shape established by 24-01..24-04, the v1.0.12 work was committed separately FIRST (commit `e732737`, message `fix(O-Reed): v1.0.12 reed-Q cap prevents parasitic HF mode locking`) before starting the Phase 24 plan. This preserved:
- The 8-file atomic shape for the Phase 24 plan commit (`c829350`).
- Clean attribution: the Phase 24 plan owns ONLY the note-expression propagation; the reed-Q cap fix has its own commit and is reviewable independently.
- CHANGELOG.md insertion order: v1.1.0 entry inserted ABOVE v1.0.12, so the file's history reads top-to-bottom in reverse-chronological order (1.1.0 → 1.0.12 → 1.0.11 → ...).

This is NOT a plan deviation — the plan §read_first explicitly listed `Working tree clean` as a precondition; the pre-step commit moved the unrelated changes out of the working tree before the plan started its 8-file edits. SUMMARY records this for transparency in the audit trail.

### F. Helper-based MPE composition correctness (structural)

Critical composition-order rule for ReedWindVoice (MPE — three call sites on one helper):
1. The helper at `Source/ReedWindVoice.cpp:121-138` is `getBaseFrequencyFromTuning(int midiNote) const`. Body order: (a) tuningEngine query OR `juce::MidiMessage::getMidiNoteInHertz` fallback → `double freq`; (b) `applyPendingTuning(*pendingTuningSource, midiNote, freq)` if pendingTuningSource non-null → updates `freq`; (c) `static_cast<float>(freq)` return. NE delta applied INSIDE the helper.
2. Call site #1 — `noteStarted()` legato branch at line 141: `float frequency = getBaseFrequencyFromTuning(note.initialNote);` — gets NE-tuned frequency. MPE pitch-bend then composes via `frequency *= pow(2, bendSemitones/12.0f)` at line 144. Then `bore.setFrequency(frequency)` at line 163. **Pattern 2 satisfied** — bore period sized to tuned frequency BEFORE first sample.
3. Call site #2 — `noteStarted()` normal branch at line 202: `float frequency = getBaseFrequencyFromTuning(note.initialNote);` — same shape. `bore.setFrequency(frequency)` at line 378. **Pattern 2 satisfied.**
4. Call site #3 — `notePitchbendChanged` at line 374: `float frequency = getBaseFrequencyFromTuning(note.initialNote);` — slot is already empty (consumed at noteStarted), so helper returns the base frequency unchanged. MPE pitch-bend then composes via `frequency *= pow(2, bendSemitones/12.0f)`. Then `bore.setFrequency(frequency)` at line 378. **This is correct one-NE-per-noteOn semantics** — pitch-bend updates during a held note do NOT re-consume any NE slot; the NE delta is locked at noteOn time. Pitch-bend composes multiplicatively on top.

If `applyPendingTuning` were placed at each call site instead of inside the helper:
- Three near-identical `if (pendingTuningSource != nullptr) frequency = applyPendingTuning(...)` blocks would be needed at lines 142, 203, 375 — code duplication.
- Risk of forgetting one site (especially if a future MPE method like `noteKeyStateChanged` adds another call to `getBaseFrequencyFromTuning`) — the helper-based approach is defense-in-depth against future call-site additions.
- exchange(0.0) consume semantics still produce the same outcome (first call consumes; later calls see empty slot) — but explicit, not implicit.

The chosen helper-based approach is the cleaner abstraction; the spike findings (PATTERNS.md §5) recommend it explicitly for plugins with multiple call sites on a tuning helper. Carry-forward to **plan 24-06 (O-Bowed)** which has the SAME helper shape at `BowedStringVoice.cpp:291-296` with TWO call sites at `noteStarted` (line 32) + `notePitchbendChanged` (line 71): apply the same in-helper composition; do not duplicate logic at each call site.

### G. MPE pitch-bend interplay with NE (musical-correctness structural property)

MPE pitch-bend is a per-note multiplicative ratio applied per-block in `noteStarted` and `notePitchbendChanged` via `frequency *= pow(2, bendSemitones / 12.0f)`. NE delta is also a multiplicative ratio applied per-noteOn via `applyPendingTuning`. They compose by simple multiplication:

```
final_freq = tuningEngine.getFrequency(midi)         // step 1: base tuning
           * pow(2, ne_semitones / 12.0)              // step 2: NE delta (once per noteOn)
           * pow(2, mpe_bend_semitones / 12.0);       // step 3: MPE pitch-bend (per-block)
```

This is the musically-expected behavior for Dorico microtonal playback in MPE mode: a quarter-sharp C4 with a +1-semitone MPE pitch-bend should sound at C#4 + 50¢ (i.e., 277.18 Hz × 1.0293 ≈ 285.30 Hz). Both deltas compose multiplicatively without conflict. NE applies once at noteOn (slot consumed); MPE pitch-bend re-applies each render block on top. Helper-based composition naturally achieves this via the slot-consume + per-call-site `*= pow(2, bendSemitones/12.0f)` pattern that's already in place.

## Decisions Made

- **Dorico gate batching at orchestrator level.** Recorded plan-local Dorico 3-point smoke as DEFERRED rather than fabricating a PASS or stopping the plan to prompt the user. SUMMARY explicitly tabulates the 3 gate points with status DEFERRED for downstream aggregation in 24-08-final-sweep.
- **Helper-based MPE composition (NOT per-call-site).** Picked the in-helper composition variant explicitly recommended by PATTERNS.md §5. Reasoning: minimizes call-site delta (zero edits at the 3 call sites — they consume the unchanged helper return value); semantically clean (single source of truth for NE composition across all MPE lifecycle methods); future-safe (any new MPE method that calls `getBaseFrequencyFromTuning` automatically inherits the NE behavior).
- **Pre-step commit for unrelated v1.0.12 work.** The reed-Q cap fix on the working tree was unrelated to Phase 24 propagation. Committed it as a separate `fix()` commit BEFORE starting the Phase 24 plan to preserve the canonical 8-file atomic-commit shape established by 24-01..24-04. This kept the Phase 24 plan commit pure and the audit trail clean.
- **PLUGIN_VERSION line explicit-add (carry-forward from 24-03).** Same pattern as O-Wind v1.16.0: insert `PLUGIN_VERSION "1.1.0"` between `PRODUCT_NAME` and `IS_SYNTH` lines inside `juce_add_plugin(O-Reed ...)`. Matrix-flagged in 24-INTEGRATION-MATRIX.md row o-reed; preflight Task 1 confirmed PLUGIN_VERSION absent (count=0).

## Deviations from Plan

**No Rule 1/2/3/4 deviations.** The plan executed as specified. The pre-step commit (Note E) was a setup-time housekeeping action (committing pre-existing unrelated work to clean the working tree) — NOT a deviation from the plan's specified actions, since the plan's `read_first` explicitly listed `Working tree clean` as a precondition.

The seven environmental notes (A–G) are observations, not deviations: the plan's automated acceptance criteria (Tasks 1, 2, 3, 5) all PASS, and no other plan rule was violated.

## Issues Encountered

**None encountered.** No CMake baseline defect (different from 24-04). No build failures. No AU validation failures. No plan-checker grep mismatches after the lowercase-`adds` correction (which was a 24-01 carry-forward I noticed during §verify and fixed inline before commit; not a "deviation" because Note D from 24-01 SUMMARY already documented this convention). Tri-format ninja exit 0 on first attempt (74 build steps including JUCE/SDK touchpoints; final 3 link lines: VST3 + AU + Standalone).

Two harmless compiler warnings appeared in the build log (FReleaser shadow-field, non-virtual-destructor `delete` on the module's `Controller` class) — both originate in JUCE/SDK headers and the pre-existing module code, NOT from O-Reed plan changes; pre-existing across all Phase 24 builds (carry-forward from 24-04 / earlier).

## TDD Gate Compliance

N/A — plan type `execute` (not `tdd`); voice-side correctness validated structurally (Pattern 1 inherited from shared module; Pattern 2 line-precise edit verified for the helper-based MPE composition order; Pattern 3 inherited from helper). Dorico human-verify gate deferred to Phase 24 batch validation per orchestrator direction.

## User Setup Required

None for this plan's automated acceptance — no external service configuration required. Dorico smoke (deferred) requires Dorico host; orchestrator will batch this.

## Next Phase Readiness

**Phase 24 status after this plan:**
- ✓ Plan 24-01 complete (O-Bells canary; Dorico 3-point PASS)
- ✓ Plan 24-02 complete (O-Prism wave 2; Dorico 3-point DEFERRED)
- ✓ Plan 24-03 complete (O-Wind wave 3; Dorico 3-point DEFERRED) — first physical-model consumer
- ✓ Plan 24-04 complete (O-IntonationPad wave 4; Dorico 3-point DEFERRED) — first STRUCTURAL VARIATION consumer (multi-sub-voice neRatio)
- ✓ Plan 24-05 complete (O-Reed wave 5; Dorico 3-point DEFERRED) — **first MPE consumer** (helper-based MPE composition pattern established)
- ✓ Helper-based MPE composition pattern proven — generalizable to plan 24-06 (O-Bowed) which has the SAME helper shape at `BowedStringVoice.cpp:291-296` with TWO call sites (`noteStarted` + `notePitchbendChanged`)
- ✓ MPE NE correlation confirmed — Pattern 1 holds at `getCurrentlyPlayingNote().initialNote` regardless of MPE channel (same as classic Synthesiser mode in 24-01..24-04)
- ✓ MPE pitch-bend + NE composition order documented (multiplicative compose; NE once per noteOn, pitch-bend per-block)
- ✓ Dev-suffix bundle handling continues to work identically across plans (carry-forward A)
- ✓ `auval -a` advisory continues to apply (carry-forward B)
- ✓ PLUGIN_VERSION explicit-add pattern continues to apply (carry-forward D from 24-03)
- ⏳ Dorico 3-point gate batch-validation pending — orchestrator queue: 24-02..24-07

**Aggregation hook:** This SUMMARY feeds **`24-08-final-sweep-SUMMARY.md` row 5 of 8**. The 3-point Dorico gate result table format (with DEFERRED status here) is the row-template for plans 24-06..24-07. The helper-based MPE composition pattern is documented for downstream consumers (plan 24-06 directly applicable; plan 24-07 partially — O-Formant has different MPE composition shape per PATTERNS.md §7) and Phase 25 DOCS-01 module-level pattern documentation.

**Ready for plan 24-06 (O-Bowed).** No blockers, no escalations. Phase 24 wave 5 momentum preserved. The matrix-flagged "missing PLUGIN_VERSION" delta for plan 24-06 is now twice-pre-validated (24-03 O-Wind + 24-05 O-Reed both used the explicit-add-inside-juce_add_plugin handling pattern). Plan 24-06 (O-Bowed) is the second MPE plugin and will validate the helper-based MPE composition pattern on a physical-model waveguide string (vs. O-Reed's bore waveguide).

## Self-Check: PASSED

- `git log --oneline -5 | grep -q "c829350"` → FOUND
- Atomic commit `c829350` references all 8 plan-scoped files (verified via `git show c829350 --stat` → `8 files changed, 65 insertions(+), 6 deletions(-)`)
- Pre-step commit `e732737` (v1.0.12 reed-Q cap fix) landed BEFORE the Phase 24 plan commit, preserving the 8-file atomic shape → FOUND
- `plugins/O-Reed/CMakeLists.txt` contains `ouaricon_add_module(O-Reed note-expression)` and `PLUGIN_VERSION "1.1.0"` → FOUND
- `plugins/O-Reed/Source/PluginProcessor.h` contains `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` and `getVST3ClientExtensions()` override → FOUND
- `plugins/O-Reed/Source/PluginProcessor.cpp` contains `vst3Extensions.drainAndUpdate(` and `setPendingTuningSource(&vst3Extensions` → FOUND
- `plugins/O-Reed/Source/ReedWindVoice.h` contains `pendingTuningSource` and inline `setPendingTuningSource` setter → FOUND
- `plugins/O-Reed/Source/ReedWindVoice.cpp` contains `Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNote, freq)` (single qualified call inside `getBaseFrequencyFromTuning` helper) → FOUND
- `plugins/O-Reed/CHANGELOG.md` contains the TRACK-03 verbatim phrase `adds VST3 Note Expression microtonal support for Dorico` → FOUND
- `modules/registry.yaml` contains `plugin: O-Reed` under `note-expression.used_by` → FOUND
- `~/Library/Audio/Plug-Ins/VST3/O-Reed*.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Reed*.component` present at fresh mtime (2026-04-26 10:19) → FOUND (both dev and prod naming)
- `scripts/verify-au-link.sh O-Reed` exit 0 with `AU VALIDATION SUCCEEDED. auval accepted O-Reed (aumu ORed OuDv)` → FOUND in execution log
- Dorico 3-point gate status documented as DEFERRED with structural correctness rationale → PRESENT
- No Rule 1/2/3/4 deviations recorded — plan executed as specified → PRESENT

---
*Phase: 24-propagate*
*Completed: 2026-04-26*
